/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "stdafx.h"

#include <AzToolsFramework/SourceControl/PerforceComponent.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/platformincl.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/lock.h>

#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/IO/SystemFile.h>

#include <AzToolsFramework/Process/ProcessCommunicator.h>
#include <AzToolsFramework/Process/ProcessWatcher.h>

#define SCC_WINDOW "Source Control"

namespace AzToolsFramework
{
    class PerforceConnection;

    namespace
    {
        const int changeList_Default = -1;
        const int changeList_None = 0;

        PerforceConnection* s_perforceConn = nullptr;
    }

    class PerforceCommand
    {
    public:
        ProcessOutput m_rawOutput;
        PerforceMap m_commandOutputMap; // doesn't allow duplicate kvp's
        AZStd::vector<PerforceMap> m_commandOutputMapList; // allows duplicate kvp's

        PerforceCommand()
            : endOfFileChar(static_cast<char>(26)) {}
        ~PerforceCommand() {}

        AZStd::string GetCurrentChangelistNumber() const { return this->GetOutputValue("change"); }
        AZStd::string GetHaveRevision() const { return this->GetOutputValue("haveRev"); }
        AZStd::string GetHeadRevision() const { return this->GetOutputValue("headRev"); }
        AZStd::string GetOtherUserCheckedOut() const { return this->GetOutputValue("otherOpen0"); }
        int GetOtherUserCheckOutCount() const { return atoi(this->GetOutputValue("otherOpen").c_str()); }

        bool CurrentActionIsAdd() const { return this->GetOutputValue("action") == "add"; }
        bool CurrentActionIsEdit() const { return this->GetOutputValue("action") == "edit"; }
        bool CurrentActionIsDelete() const { return this->GetOutputValue("action") == "delete"; }
        bool FileExists() const { return m_rawOutput.errorResult.find("no such file(s)") == AZStd::string::npos; }
        bool HasRevision() const { return atoi(this->GetOutputValue("haveRev").c_str()) >= 0; }
        bool HeadActionIsDelete() const { return this->GetOutputValue("headAction") == "delete"; }
        bool IsMarkedForAdd() const { return m_rawOutput.outputResult.find("can't edit (already opened for add)") != AZStd::string::npos; }
        bool NeedsReopening() const { return m_rawOutput.outputResult.find("use 'reopen'") != AZStd::string::npos; }
        bool IsOpenByOtherUsers() const { return this->OutputKeyExists("otherOpen"); }
        bool IsOpenByCurrentUser() const { return this->OutputKeyExists("action"); }

        bool HasTrustIssue() const
        {
            if (m_rawOutput.errorResult.find("The authenticity of ") != AZStd::string::npos &&
                m_rawOutput.errorResult.find("can't be established,") != AZStd::string::npos)
            {
                return true;
            }

            return false;
        }

        bool ExclusiveOpen() const
        {
            AZStd::string fileType = GetOutputValue("headType");
            if (!fileType.empty())
            {
                size_t modLocation = fileType.find('+');
                if (modLocation != AZStd::string::npos)
                {
                    return fileType.find('l', modLocation) != AZStd::string::npos;
                }
            }
            return false;
        }

        AZStd::string GetOutputValue(const AZStd::string& key) const
        {
            PerforceMap::const_iterator kvp = this->m_commandOutputMap.find(key);
            if (kvp != this->m_commandOutputMap.end())
            {
                return kvp->second;
            }
            else
            {
                return "";
            }
        }

        AZStd::string GetOutputValue(const AZStd::string& key, PerforceMap perforceMap) const
        {
            PerforceMap::const_iterator kvp = perforceMap.find(key);
            if (kvp != perforceMap.end())
            {
                return kvp->second;
            }
            else
            {
                return "";
            }
        }

        bool OutputKeyExists(const AZStd::string& key) const
        {
            PerforceMap::const_iterator kvp = this->m_commandOutputMap.find(key);
            return kvp != this->m_commandOutputMap.end();
        }

        bool OutputKeyExists(const AZStd::string& key, PerforceMap perforceMap) const
        {
            PerforceMap::const_iterator kvp = perforceMap.find(key);
            return kvp != perforceMap.end();
        }

        AZStd::vector<PerforceMap>::iterator FindMapWithPartiallyMatchingValueForKey(const AZStd::string& key, const AZStd::string& value)
        {
            for (AZStd::vector<PerforceMap>::iterator perforceIter = m_commandOutputMapList.begin();
                 perforceIter != m_commandOutputMapList.end(); ++perforceIter)
            {
                const PerforceMap currentMap = *perforceIter;
                PerforceMap::const_iterator kvp = currentMap.find(key);
                if (kvp != currentMap.end() && kvp->second.find(value) != AZStd::string::npos)
                {
                    return perforceIter;
                }
            }

            return nullptr;
        }

        AZStd::string CreateChangelistForm(const AZStd::string& client, const AZStd::string& user, const AZStd::string& description)
        {
            AZStd::string changelistForm = "Change:	new\n\nClient:	";
            changelistForm.append(client);

            if (!user.empty() && user != "*unknown*")
            {
                changelistForm.append("\n\nUser:	");
                changelistForm.append(user);
            }

            changelistForm.append("\n\nStatus:	new\n\nDescription:\n	");
            changelistForm.append(description);
            changelistForm.append("\n\n");
            changelistForm += endOfFileChar;
            return changelistForm;
        }

        void ExecuteAdd(const AZStd::string& changelist, const AZStd::string& filePath)
        {
            m_commandArgs = "add -c " + changelist + " \"" + filePath + "\"";
            ExecuteCommand();
        }

        void ExecuteClaimChangedFile(const AZStd::string& filePath, const AZStd::string& changeList)
        {
            m_commandArgs = "reopen -c " + changeList + " \"" + filePath + "\"";
            ExecuteCommand();
        }

        void ExecuteDelete(const AZStd::string& changelist, const AZStd::string& filePath)
        {
            m_commandArgs = "delete -c " + changelist + " \"" + filePath + "\"";
            ExecuteCommand();
        }

        void ExecuteEdit(const AZStd::string& changelist, const AZStd::string& filePath)
        {
            m_commandArgs = "edit -c " + changelist + " \"" + filePath + "\"";
            ExecuteCommand();
        }

        void ExecuteFstat(const AZStd::string& filePath)
        {
            m_commandArgs = "fstat \"" + filePath + "\"";
            ExecuteCommand();
        }

        void ExecuteSet()
        {
            m_commandArgs = "set";
            ExecuteRawCommand();
        }

        void ExecuteSet(const AZStd::string& key, const AZStd::string& value)
        {
            m_commandArgs = "set " + key + '=' + value;
            ExecuteIOCommand();
        }

        void ExecuteInfo()
        {
            m_commandArgs = "info";
            ExecuteCommand();
        }

        void ExecuteShortInfo()
        {
            m_commandArgs = "info -s";
            ExecuteCommand();
        }

        void ExecuteTicketStatus()
        {
            m_commandArgs = "login -s";
            ExecuteCommand();
        }

        void ExecuteTrust(bool enable, const AZStd::string& fingerprint)
        {
            if (enable)
            {
                m_commandArgs = "trust -i ";
            }
            else
            {
                m_commandArgs = "trust -d ";
            }
            m_commandArgs += fingerprint;
            ExecuteCommand();
        }

        ProcessWatcher* ExecuteNewChangelistInput()
        {
            m_commandArgs = "change -i";
            return ExecuteIOCommand();
        }

        void ExecuteNewChangelistOutput()
        {
            m_commandArgs = "change -o";
            ExecuteRawCommand();
        }

        void ExecuteRevert(const AZStd::string& filePath)
        {
            m_commandArgs = "revert \"" + filePath + "\"";
            ExecuteCommand();
        }

        void ExecuteShowChangelists(const AZStd::string& currentUser, const AZStd::string& currentClient)
        {
            m_commandArgs = "changes -s pending -c " + currentClient;
            if (!currentUser.empty() && currentUser != "*unknown*")
            {
                m_commandArgs += " -u " + currentUser;
            }
            ExecuteCommand();
        }

        void ThrowWarningMessage()
        {
            // This can happen all the time, for various reasons.  AZ_Warning will actually cause the application to
            // take a stack dump and will load pdbs, which can introduce a serious delay during startup.  As such,
            // we send it as a Trace, not a warning.  Background threads retrying may hit this many times.
            AZ_TracePrintf(SCC_WINDOW, "Perforce Warning - Command has failed '%s'\n", m_commandArgs.c_str());
        }

    private:
        char endOfFileChar;

        AZStd::string m_commandArgs;

        void ExecuteCommand()
        {
            m_rawOutput.Clear();
            ProcessLauncher::ProcessLaunchInfo processLaunchInfo;
            processLaunchInfo.m_commandlineParameters = "p4 -ztag " + m_commandArgs;
            processLaunchInfo.m_showWindow = false;
            ProcessWatcher::LaunchProcessAndRetrieveOutput(processLaunchInfo, ProcessCommunicationType::COMMUNICATOR_TYPE_STDINOUT, m_rawOutput);
        }

        ProcessWatcher* ExecuteIOCommand()
        {
            ProcessLauncher::ProcessLaunchInfo processLaunchInfo;
            processLaunchInfo.m_commandlineParameters = "p4 " + m_commandArgs;
            processLaunchInfo.m_showWindow = false;
            return ProcessWatcher::LaunchProcess(processLaunchInfo, ProcessCommunicationType::COMMUNICATOR_TYPE_STDINOUT);
        }

        void ExecuteRawCommand()
        {
            m_rawOutput.Clear();
            ProcessLauncher::ProcessLaunchInfo processLaunchInfo;
            processLaunchInfo.m_commandlineParameters = "p4 " + m_commandArgs;
            processLaunchInfo.m_showWindow = false;
            ProcessWatcher::LaunchProcessAndRetrieveOutput(processLaunchInfo, ProcessCommunicationType::COMMUNICATOR_TYPE_STDINOUT, m_rawOutput);
        }
    };

    class PerforceConnection
    {
    public:
        PerforceMap m_infoResultMap;
        PerforceCommand m_command;

        PerforceConnection() {}
        ~PerforceConnection() {}

        AZStd::string GetUser() const { return this->GetInfoValue("userName"); }
        AZStd::string GetClientName() const { return this->GetInfoValue("clientName"); }
        AZStd::string GetClientRoot() const { return this->GetInfoValue("clientRoot"); }
        AZStd::string GetServerAddress() const { return this->GetInfoValue("serverAddress"); }
        AZStd::string GetServerUptime() const { return this->GetInfoValue("serverUptime"); }

        AZStd::string GetInfoValue(const AZStd::string& key) const
        {
            PerforceMap::const_iterator kvp = this->m_infoResultMap.find(key);
            if (kvp != this->m_infoResultMap.end())
            {
                return kvp->second;
            }
            else
            {
                return "";
            }
        }

        bool CommandHasOutput() const { return this->m_command.m_rawOutput.HasOutput(); }
        bool CommandHasError() const { return this->m_command.m_rawOutput.HasError(); }
        bool CommandHasTrustIssue() const { return this->m_command.HasTrustIssue(); }
        AZStd::string GetCommandOutput() const { return this->m_command.m_rawOutput.outputResult; }
        AZStd::string GetCommandError() const { return this->m_command.m_rawOutput.errorResult; }

        bool CommandHasFailed()
        {
            if (!CommandHasOutput())
            {
                if (CommandHasError())
                {
                    AZ_TracePrintf(SCC_WINDOW, "Perforce - ERRORS\n%s\n", GetCommandError().c_str());
                    AZ_TracePrintf(SCC_WINDOW, "Perforce - Error = %s\n", !m_command.FileExists() ? "No such file exists" : "Unknown error");
                }

                m_command.ThrowWarningMessage();
                return true;
            }

            return false;
        }
    };

    void PerforceSettingResult::UpdateSettingInfo(const AZStd::string& value)
    {
        if (value.empty())
        {
            m_settingInfo.m_status = SourceControlSettingStatus::Unset;
            return;
        }

        AZStd::size_t pos = value.find(" (set)");
        if (pos != AZStd::string::npos)
        {
            m_settingInfo.m_status = SourceControlSettingStatus::Set;
            m_settingInfo.m_value = value.substr(0, pos);
            return;
        }

        pos = value.find(" (config");
        if (pos != AZStd::string::npos)
        {
            m_settingInfo.m_status = SourceControlSettingStatus::Config;
            m_settingInfo.m_value = value.substr(0, pos);
            AZStd::size_t fileStart = value.find('\'', pos);
            AZStd::size_t fileEnd = value.find('\'', pos + 10);  // 10: (config '
            if (fileStart != AZStd::string::npos && fileEnd > fileStart)
            {
                m_settingInfo.m_context = value.substr(fileStart, fileEnd - fileStart);
            }
            return;
        }

        m_settingInfo.m_status = SourceControlSettingStatus::None;
        m_settingInfo.m_value = value;
    }

    void PerforceComponent::Activate()
    {
        AZ_Assert(!s_perforceConn, "You may only have one Perforce component.\n");
        m_shutdownThreadSignal = false;
        m_waitingOnTrust = false;
        m_autoChangelistDescription = "*Lumberyard Auto";
        m_connectionState = SourceControlState::Disabled;
        m_validConnection = false;
        m_testConnection = false;
        m_offlineMode = true;
        m_trustedKey = false;
        m_resolveKey = true;
        m_testTrust = false;

        // set up signals before we start thread.
        m_shutdownThreadSignal = false;
        m_WorkerThread = AZStd::thread(AZStd::bind(&PerforceComponent::ThreadWorker, this));

        SourceControlConnectionRequestBus::Handler::BusConnect();
        SourceControlCommandBus::Handler::BusConnect();
    }

    void PerforceComponent::Deactivate()
    {
        SourceControlCommandBus::Handler::BusDisconnect();
        SourceControlConnectionRequestBus::Handler::BusDisconnect();

        m_shutdownThreadSignal = true; // tell the thread to die.
        m_WorkerSemaphore.release(1); // wake up the thread so that it sees the signal
        m_WorkerThread.join(); // wait for the thread to finish.
        m_WorkerThread = AZStd::thread();

        delete s_perforceConn;
        s_perforceConn = nullptr;
    }

    void PerforceComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<PerforceComponent, AZ::Component>()
                ->SerializerForEmptyClass()
            ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<PerforceComponent>(
                    "Perforce Connectivity", "Manages Perforce connectivity and executes Perforce commands.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Editor")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                ;
            }
        }
    }

    void PerforceComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("SourceControlService", 0x67f338fd));
        provided.push_back(AZ_CRC("PerforceService", 0x74b25961));
    }

    void PerforceComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("PerforceService", 0x74b25961));
    }

    void PerforceComponent::GetFileInfo(const char* fullFilePath, const SourceControlResponseCallback& callbackFn)
    {
        AZ_Assert(fullFilePath, "Must specify a file path");
        AZ_Assert(callbackFn, "Must specify a callback!");

        PerforceJobRequest topReq;
        topReq.m_requestPath = fullFilePath;
        topReq.m_requestType = PerforceJobRequest::PJR_Stat;
        topReq.m_callback = callbackFn;
        QueueJobRequest(AZStd::move(topReq));
    }

    void PerforceComponent::RequestEdit(const char* fullFilePath, bool allowMultiCheckout, const SourceControlResponseCallback& callbackFn)
    {
        AZ_Assert(fullFilePath, "Must specify a file path");
        AZ_Assert(callbackFn, "Must specify a callback!");

        PerforceJobRequest topReq;
        topReq.m_requestPath = fullFilePath;
        topReq.m_requestType = PerforceJobRequest::PJR_Edit;
        topReq.m_allowMultiCheckout = allowMultiCheckout;
        topReq.m_callback = callbackFn;
        QueueJobRequest(AZStd::move(topReq));
    }

    void PerforceComponent::RequestDelete(const char* fullFilePath, const SourceControlResponseCallback& callbackFn)
    {
        AZ_Assert(fullFilePath, "Must specify a file path");
        AZ_Assert(callbackFn, "Must specify a callback!");

        PerforceJobRequest topReq;
        topReq.m_requestPath = fullFilePath;
        topReq.m_requestType = PerforceJobRequest::PJR_Delete;
        topReq.m_callback = callbackFn;
        QueueJobRequest(AZStd::move(topReq));
    }

    void PerforceComponent::RequestRevert(const char* fullFilePath, const SourceControlResponseCallback& callbackFn)
    {
        AZ_Assert(fullFilePath, "Must specify a file path");
        AZ_Assert(callbackFn, "Must specify a callback!");

        PerforceJobRequest topReq;
        topReq.m_requestPath = fullFilePath;
        topReq.m_requestType = PerforceJobRequest::PJR_Revert;
        topReq.m_callback = callbackFn;
        QueueJobRequest(AZStd::move(topReq));
    }

    void PerforceComponent::EnableSourceControl(bool enable)
    {
        m_testTrust = enable;
        m_testConnection = enable;
        m_resolveKey = enable;

        m_offlineMode = !enable;
        m_WorkerSemaphore.release(1);
    }

    void PerforceComponent::EnableTrust(bool enable, AZStd::string fingerprint)
    {
        s_perforceConn->m_command.ExecuteTrust(enable, fingerprint);
        m_trustedKey = IsTrustKeyValid();
        m_waitingOnTrust = false;

        // If our key is now valid, prompt user if it expires / becomes invalid
        // If our key is now invalid, wait until we receive a bus message to prompt user
        m_resolveKey = m_trustedKey ? true : false;
        m_WorkerSemaphore.release(1);
    }

    void PerforceComponent::SetConnectionSetting(const char* key, const char* value, const SourceControlSettingCallback& respCallBack)
    {
        PerforceSettingResult result;
        result.m_callback = respCallBack;
        if (ExecuteAndParseSet(key, value))
        {
            result.UpdateSettingInfo(s_perforceConn->m_command.GetOutputValue(key));
        }
        QueueSettingResponse(result);
    }

    void PerforceComponent::GetConnectionSetting(const char* key, const SourceControlSettingCallback& respCallBack)
    {
        PerforceSettingResult result;
        result.m_callback = respCallBack;
        if (ExecuteAndParseSet(nullptr, nullptr))
        {
            result.UpdateSettingInfo(s_perforceConn->m_command.GetOutputValue(key));
        }
        QueueSettingResponse(result);
    }

    SourceControlFileInfo PerforceComponent::GetFileInfo(const char* filePath)
    {
        SourceControlFileInfo newInfo;
        newInfo.m_status = SCS_ProviderError;
        newInfo.m_filePath = filePath;

        if (AZ::IO::SystemFile::IsWritable(filePath) || !AZ::IO::SystemFile::Exists(filePath))
        {
            newInfo.m_flags |= SCF_Writeable;
        }

        if (!CheckConnectivityForAction("file info", filePath))
        {
            newInfo.m_status = m_trustedKey ? SCS_ProviderIsDown : SCS_CertificateInvalid;
            return newInfo;
        }

        bool sourceAwareFile = false;
        if (!ExecuteAndParseFstat(filePath, sourceAwareFile))
        {
            if (!sourceAwareFile)
            {
                AZ_TracePrintf(SCC_WINDOW, "Perforce - File is not tracked\n");
                newInfo.m_status = SCS_NotTracked;
            }

            return newInfo;
        }

        bool deletedAtHeadRev = s_perforceConn->m_command.HeadActionIsDelete();
        bool hasRevision = s_perforceConn->m_command.HasRevision();
        if (!(deletedAtHeadRev && !hasRevision))
        {
            if (s_perforceConn->m_command.GetHeadRevision() != s_perforceConn->m_command.GetHaveRevision())
            {
                newInfo.m_flags |= SCF_OutOfDate;
                AZ_TracePrintf(SCC_WINDOW, "Perforce - File is out of date\n");
            }
        }

        if (!s_perforceConn->m_command.ExclusiveOpen())
        {
            newInfo.m_flags |= SCF_MultiCheckOut;
        }

        bool openByOthers = s_perforceConn->m_command.IsOpenByOtherUsers();
        if (openByOthers)
        {
            newInfo.m_flags |= SCF_OtherOpen;
            newInfo.m_StatusUser = s_perforceConn->m_command.GetOtherUserCheckedOut();
        }

        if (s_perforceConn->m_command.IsOpenByCurrentUser())
        {
            newInfo.m_status = SCS_OpenByUser;
            if (s_perforceConn->m_command.CurrentActionIsAdd())
            {
                newInfo.m_flags |= SCF_PendingAdd;
                AZ_TracePrintf(SCC_WINDOW, "Perforce - File is marked for add\n");
            }
            else if (s_perforceConn->m_command.CurrentActionIsDelete())
            {
                newInfo.m_flags |= SCF_PendingDelete;
                AZ_TracePrintf(SCC_WINDOW, "Perforce - File is marked for delete\n");
            }
            else
            {
                AZ_TracePrintf(SCC_WINDOW, "Perforce - File is checked out by you\n");
                if (openByOthers)
                {
                    AZ_TracePrintf(SCC_WINDOW, "          and Others\n");
                }
            }
        }
        else if (openByOthers)
        {
            newInfo.m_status = SCS_Tracked;
            AZ_TracePrintf(SCC_WINDOW, "Perforce - File is opened by %s\n", newInfo.m_StatusUser.c_str());
        }
        else if (deletedAtHeadRev && !hasRevision)
        {
            newInfo.m_status = SCS_NotTracked;
            AZ_TracePrintf(SCC_WINDOW, "Perforce - File is deleted at head revision\n");
        }
        else
        {
            newInfo.m_status = SCS_Tracked;
            AZ_TracePrintf(SCC_WINDOW, "Perforce - File is Checked In\n");
        }

        AZ_TracePrintf(SCC_WINDOW, "Perforce - Received file info %s\n\n", newInfo.m_filePath.c_str());
        return newInfo;
    }

    bool PerforceComponent::RequestEdit(const char* filePath, bool allowMultiCheckout)
    {
        if (!CheckConnectivityForAction("checkout", filePath))
        {
            return false;
        }

        return ExecuteEdit(filePath, allowMultiCheckout);
    }

    bool PerforceComponent::RequestDelete(const char* filePath)
    {
        if (!CheckConnectivityForAction("delete", filePath))
        {
            return false;
        }

        return ExecuteDelete(filePath);
    }

    bool PerforceComponent::GetLatest(const char* filePath)
    {
        AZ_Assert(m_ProcessThreadID != AZStd::thread::id(), "The perforce worker thread has not started.");
        AZ_Assert(AZStd::this_thread::get_id() == m_ProcessThreadID, "You may only call this function from the perforce worker thread.");

        (void)filePath;
        AZ_Assert(false, "Get Latest not implemented.");
        return false;
    }

    bool PerforceComponent::RequestRevert(const char* filePath)
    {
        if (!CheckConnectivityForAction("revert", filePath))
        {
            return false;
        }

        return ExecuteRevert(filePath);
    }

    // claim changed file just moves the file to the new changelist - it assumes its already on your changelist
    bool PerforceComponent::ClaimChangedFile(const char* path, int changeListNumber)
    {
        AZStd::string changeListStr = AZStd::string::format("claim changed file to changelist %i", changeListNumber);
        if (!CheckConnectivityForAction(changeListStr.c_str(), path))
        {
            return false;
        }

        changeListStr = AZStd::to_string(changeListNumber);
        s_perforceConn->m_command.ExecuteClaimChangedFile(path, changeListStr);
        return CommandSucceeded();
    }

    bool PerforceComponent::ExecuteAdd(const char* filePath)
    {
        bool sourceAwareFile = false;
        if (!ExecuteAndParseFstat(filePath, sourceAwareFile))
        {
            if (sourceAwareFile)
            {
                return false;
            }
        }

        int changeListNumber = GetOrCreateOurChangelist();
        if (changeListNumber <= 0)
        {
            // error will have already been emitted.
            return false;
        }

        AZStd::string changeListNumberStr = AZStd::to_string(changeListNumber);

        s_perforceConn->m_command.ExecuteAdd(changeListNumberStr, filePath);
        if (!CommandSucceeded())
        {
            return false;
        }

        // Interesting result of our intention based system - 
        // The perforce Add operation does not remove 'read only' status from a file; however,
        // since our intention is to say "place this file in a state ready for work," we need
        // to remove read only status (otherwise subsequent save operations would fail)
        AZ::IO::SystemFile::SetWritable(filePath, true);

        ExecuteAndParseFstat(filePath, sourceAwareFile);
        if (!s_perforceConn->m_command.CurrentActionIsAdd())
        {
            AZ_Warning(SCC_WINDOW, false, "Perforce - Unable to add file '%s' (Perforce is connected but file was not added)\n", filePath);
            return false;
        }

        AZ_TracePrintf(SCC_WINDOW, "Perforce - Successfully added file %s\n", filePath);
        return true;
    }

    bool PerforceComponent::ExecuteEdit(const char* filePath, bool allowMultiCheckout)
    {
        bool sourceAwareFile = false;
        if (!ExecuteAndParseFstat(filePath, sourceAwareFile))
        {
            if (sourceAwareFile)
            {
                return false;
            }

            if (!AZ::IO::SystemFile::Exists(filePath))
            {
                sourceAwareFile = false;
            }
        }

        if (sourceAwareFile && (s_perforceConn->m_command.IsOpenByOtherUsers() && !allowMultiCheckout))
        {
            AZ_Warning(SCC_WINDOW, false, "Perforce - Unable to edit file %s, it's already checked out by %s\n",
                filePath, s_perforceConn->m_command.GetOtherUserCheckedOut().c_str());
            return false;
        }

        if (!sourceAwareFile)
        {
            if (!ExecuteAdd(filePath))
            {
                AZ_Warning(SCC_WINDOW, false, "Perforce - Unable to add file %s\n", filePath);
                // if the file is writable, this does not count as an error as we are likely
                // simply outside the workspace.
                return AZ::IO::SystemFile::IsWritable(filePath);
            }

            return true;
        }

        // find or create 'the changelist' that we will use to handle our stuff:
        int changeListNumber = GetOrCreateOurChangelist();
        if (changeListNumber <= 0)
        {
            // error will have already been emitted.
            return false;
        }

        AZStd::string changeListNumberStr = AZStd::to_string(changeListNumber);

        if (s_perforceConn->m_command.IsOpenByCurrentUser())
        {
            if (s_perforceConn->m_command.CurrentActionIsEdit() || s_perforceConn->m_command.CurrentActionIsAdd())
            {
                // Claim the file to our working changelist if it's in the default changelist.
                const AZStd::string& currentCL = s_perforceConn->m_command.GetCurrentChangelistNumber();
                if (currentCL == "default")
                {
                    if (!ClaimChangedFile(filePath, changeListNumber))
                    {
                        AZ_TracePrintf(SCC_WINDOW, "Perforce - Unable to move file %s to changelist %s\n", filePath, changeListNumberStr.c_str());
                    }
                }

                AZ_TracePrintf(SCC_WINDOW, "Perforce - File %s is already checked out by you\n", filePath);
                return true;
            }
            else if (s_perforceConn->m_command.CurrentActionIsDelete())
            {
                // we have it checked out for delete by us - we need to revert the delete before we can check it out
                AZ_TracePrintf(SCC_WINDOW, "Perforce - Reverting deleted file %s in order to check it out\n", filePath);
                if (!ExecuteRevert(filePath))
                {
                    AZ_Warning(SCC_WINDOW, false, "Perforce - Failed to revert deleted file %s\n", filePath);
                    return false;
                }
            }
            else
            {
                AZ_Warning(SCC_WINDOW, false, "Perforce - Requesting edit on file %s opened by current user, but not marked for add, edit or delete", filePath);
                return false;
            }
        }
        else if (s_perforceConn->m_command.HeadActionIsDelete())
        {
            // it's currently deleted - we need to fix that before we can check it out
            // this Add operation is all we need since it puts the file under our control
            AZ_TracePrintf(SCC_WINDOW, "Perforce - re-Add fully deleted file %s in order to check it out\n", filePath);
            if (!ExecuteAdd(filePath))
            {
                AZ_Warning(SCC_WINDOW, false, "Perforce - Failed to revert deleted file %s in order to check it out\n", filePath);
                return false;
            }
            return true;
        }

        // If we get here it means it's not opened by us
        // attempt check out.
        s_perforceConn->m_command.ExecuteEdit(changeListNumberStr, filePath);
        if (!CommandSucceeded())
        {
            return false;
        }

        if (s_perforceConn->m_command.NeedsReopening() && !ClaimChangedFile(filePath, changeListNumber))
        {
            AZ_Warning(SCC_WINDOW, false, "Perforce - Failed to reopen file %s\n", filePath);
            return false;
        }

        ExecuteAndParseFstat(filePath, sourceAwareFile);
        if (s_perforceConn->m_command.IsMarkedForAdd())
        {
            AZ_Warning(SCC_WINDOW, false, "Perforce - Can't edit file %s, already marked for add\n", filePath);
            return false;
        }

        if (!s_perforceConn->m_command.CurrentActionIsEdit())
        {
            AZ_TracePrintf(SCC_WINDOW, "Perforce - Failed to edit file %s", filePath);
            return false;
        }

        AZ_TracePrintf(SCC_WINDOW, "Perforce - Checked out file %s\n", filePath);
        return true;
    }

    bool PerforceComponent::ExecuteDelete(const char* filePath)
    {
        bool sourceAwareFile = false;
        if (!ExecuteAndParseFstat(filePath, sourceAwareFile) || !sourceAwareFile)
        {
            return false;
        }

        if (s_perforceConn->m_command.IsOpenByCurrentUser())
        {
            if (!s_perforceConn->m_command.CurrentActionIsDelete())
            {
                if (!ExecuteRevert(filePath))
                {
                    AZ_Warning(SCC_WINDOW, false, "Perforce - Unable to revert file %s\n", filePath);
                    return false;
                }
            }
        }

        if (!s_perforceConn->m_command.CurrentActionIsDelete() || !s_perforceConn->m_command.HeadActionIsDelete())
        {
            /* not already being deleted by current or head action - delete destructively (DANGER) */
            int changeListNumber = GetOrCreateOurChangelist();
            if (changeListNumber <= 0)
            {
                AZ_Warning(SCC_WINDOW, false, "Perforce - Unable to find our changelist %s\n", m_autoChangelistDescription.c_str());
                return false;
            }

            AZStd::string changeListNumberStr = AZStd::to_string(changeListNumber);

            s_perforceConn->m_command.ExecuteDelete(changeListNumberStr, filePath);
            if (!CommandSucceeded())
            {
                AZ_Warning(SCC_WINDOW, false, "Perforce - Failed to delete file %s\n", filePath);
                return false;
            }

            AZ_TracePrintf(SCC_WINDOW, "Perforce - Deleted file %s\n", filePath);
        }

        // this will execute even if the file isn't part of SCS yet
        AZ::IO::SystemFile::Delete(filePath);

        return true;
    }

    bool PerforceComponent::ExecuteRevert(const char* filePath)
    {
        bool sourceAwareFile = false;
        if (!ExecuteAndParseFstat(filePath, sourceAwareFile) || !sourceAwareFile)
        {
            AZ_TracePrintf(SCC_WINDOW, "Perforce - Unable to revert file '%s' (Perforce is connected but file is not able to be reverted)\n", filePath);
            return false;
        }

        if (!s_perforceConn->m_command.IsOpenByCurrentUser())
        {
            AZ_TracePrintf(SCC_WINDOW, "Perforce - Unable to revert file '%s' (Perforce is connected but file is not open by current user)\n", filePath);
            return false;
        }

        s_perforceConn->m_command.ExecuteRevert(filePath);
        if (!CommandSucceeded())
        {
            return false;
        }

        ExecuteAndParseFstat(filePath, sourceAwareFile);
        if (s_perforceConn->m_command.IsOpenByCurrentUser())
        {
            AZ_Warning(SCC_WINDOW, false, "Perforce - Unable to revert file '%s' (Perforce is connected but file was not reverted)\n", filePath);
            return false;
        }

        AZ_TracePrintf(SCC_WINDOW, "Perforce - Revert succeeded for %s\n", filePath);
        return true;
    }

    void PerforceComponent::QueueJobRequest(PerforceJobRequest&& jobRequest)
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_WorkerQueueMutex);
        m_workerQueue.push(AZStd::move(jobRequest));
        m_WorkerSemaphore.release(1);
    }

    void PerforceComponent::QueueSettingResponse(const PerforceSettingResult& result)
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_SettingsQueueMutex);
        m_settingsQueue.push(AZStd::move(result));
        EBUS_QUEUE_FUNCTION(AZ::TickBus, &PerforceComponent::ProcessResultQueue, this);
    }

    bool PerforceComponent::CheckConnectivityForAction(const char* actionDesc, const char* filePath)
    {
        (void)actionDesc;
        AZ_Assert(m_ProcessThreadID != AZStd::thread::id(), "The perforce worker thread has not started.");
        AZ_Assert(AZStd::this_thread::get_id() == m_ProcessThreadID, "You may only call this function from the perforce worker thread.");

        AZStd::string fileMessage = AZStd::string::format(" for file %s", filePath);
        AZ_TracePrintf(SCC_WINDOW, "Perforce - Requested %s%s\n", actionDesc, filePath ? fileMessage.c_str() : "");

        if (m_connectionState != SourceControlState::Active)
        {
            fileMessage = AZStd::string::format(" file %s", filePath);
            AZ_Warning(SCC_WINDOW, false, "Perforce - Unable to %s%s (Perforce is not connected)\n", actionDesc, filePath ? fileMessage.c_str() : "");
            return false;
        }

        return true;
    }

    bool PerforceComponent::ExecuteAndParseFstat(const char* filePath, bool& sourceAwareFile)
    {
        sourceAwareFile = false;
        s_perforceConn->m_command.ExecuteFstat(filePath);
        if (CommandSucceeded())
        {
            sourceAwareFile = true;
        }

        return ParseOutput(s_perforceConn->m_command.m_commandOutputMap, s_perforceConn->m_command.m_rawOutput.outputResult);
    }

    bool PerforceComponent::ExecuteAndParseSet(const char* key, const char* value)
    {
        // a valid key will attempt to set the setting
        // a null value will clear the setting
        if (key)
        {
            AZStd::string keystr = key;
            AZStd::string valstr = value;
            s_perforceConn->m_command.ExecuteSet(keystr, valstr);
        }

        s_perforceConn->m_command.ExecuteSet();
        if (s_perforceConn->CommandHasFailed())
        {
            return false;
        }

        return ParseOutput(s_perforceConn->m_command.m_commandOutputMap, s_perforceConn->m_command.m_rawOutput.outputResult, "=");
    }

    bool PerforceComponent::CommandSucceeded()
    {
        if (s_perforceConn->CommandHasFailed())
        {
            m_testTrust = true;
            m_testConnection = true;
            return false;
        }
        return true;
    }

    bool PerforceComponent::UpdateTrust()
    {
        AZ_Assert(m_ProcessThreadID != AZStd::thread::id(), "The perforce worker thread has not started.");
        AZ_Assert(AZStd::this_thread::get_id() == m_ProcessThreadID, "You may only call this function from the perforce worker thread.");

        if (m_testTrust)
        {
            TestConnectionTrust(m_resolveKey);
        }

        if (!m_trustedKey && m_waitingOnTrust)
        {
            return false;
        }

        return true;
    }

    bool PerforceComponent::IsTrustKeyValid() const
    {
        s_perforceConn->m_command.ExecuteInfo();
        if (s_perforceConn->CommandHasError() && s_perforceConn->m_command.HasTrustIssue())
        {
            return false;
        }
        return true;
    }

    void PerforceComponent::TestConnectionTrust(bool attemptResolve)
    {
        AZ_Assert(m_ProcessThreadID != AZStd::thread::id(), "The perforce worker thread has not started.");
        AZ_Assert(AZStd::this_thread::get_id() == m_ProcessThreadID, "You may only call this function from the perforce worker thread.");

        if (!s_perforceConn)
        {
            s_perforceConn = aznew PerforceConnection();
        }

        m_trustedKey = IsTrustKeyValid();
        if (!m_trustedKey && attemptResolve)
        {
            if (SourceControlNotificationBus::HasHandlers())
            {
                m_waitingOnTrust = true;
                AZStd::string errOutput = s_perforceConn->m_command.m_rawOutput.errorResult;

                // tokenize the buffer into line by line key/value pairs
                AZStd::vector<AZStd::string> tokens;
                AzFramework::StringFunc::Tokenize(errOutput.c_str(), tokens, "\r\n");

                // the fingerprint is on the 4th line of error output
                static const AZ::u32 fingerprintIdx = 3;
                if (tokens.size() > fingerprintIdx)
                {
                    AZStd::string fingerprint = tokens[fingerprintIdx];

                    if (AZ::TickBus::IsFunctionQueuing())
                    {
                        // Push to the main thread for convenience.
                        AZStd::function<void()> trustNotify =
                            [this, fingerprint]()
                        {
                            SourceControlNotificationBus::Broadcast(&SourceControlNotificationBus::Events::RequestTrust, fingerprint.c_str());
                        };

                        AZ::TickBus::QueueFunction(trustNotify);
                    }
                    
                }
            }
            else
            {
                // No one is listening.  No need to hold up the worker thread.
                m_waitingOnTrust = false;
            }
        }

        m_testTrust = false;
    }

    bool PerforceComponent::IsConnectionValid() const
    {
        s_perforceConn->m_command.ExecuteTicketStatus();
        if (s_perforceConn->CommandHasError())
        {
            return false;
        }

        if (!CacheClientConfig())
        {
            return false;
        }

        return true;
    }

    bool PerforceComponent::CacheClientConfig() const
    {
        s_perforceConn->m_command.ExecuteInfo();
        if (s_perforceConn->CommandHasFailed() || !ParseOutput(s_perforceConn->m_infoResultMap, s_perforceConn->m_command.m_rawOutput.outputResult))
        {
            return false;
        }

        // P4 workaround, according to P4 documentation and forum posts, in order to keep P4 info fast it may
        // not do a server query to fetch the userName field.
        // In that case, we will do an additional p4 info -s and update the original m_infoResultMap with the
        // retrieved user name.
        if (s_perforceConn->GetUser().compare("*unknown*") == 0)
        {
            PerforceMap shortInfoMap;
            s_perforceConn->m_command.ExecuteShortInfo();
            if (s_perforceConn->CommandHasFailed() || !ParseOutput(shortInfoMap, s_perforceConn->m_command.m_rawOutput.outputResult))
            {
                return false;
            }

            s_perforceConn->m_infoResultMap["userName"] = shortInfoMap["userName"];
        }

        if (s_perforceConn->GetUser().empty() || s_perforceConn->GetClientName().empty())
        {
            AZ_WarningOnce(SCC_WINDOW, false, "Perforce - Your client or user is empty, Perforce not available!\n");
            return false;
        }

        if (s_perforceConn->GetClientName().compare("*unknown*") == 0)
        {
            AZ_WarningOnce(SCC_WINDOW, false, "Perforce - client spec not found, Perforce not available!\n");
            return false;
        }

        if (s_perforceConn->GetClientRoot().empty())
        {
            AZ_WarningOnce(SCC_WINDOW, false, "Perforce - Your workspace root is empty, Perforce not available!\n");
            return false;
        }

        if (s_perforceConn->GetServerAddress().empty() || s_perforceConn->GetServerUptime().empty())
        {
            AZ_WarningOnce(SCC_WINDOW, false, "Perforce - Could not get server information, Perforce not available!\n");
        }

        AZ_TracePrintf(SCC_WINDOW, "Perforce - Connected, User: %s | Client: %s\n", s_perforceConn->GetUser().c_str(), s_perforceConn->GetClientName().c_str());
        return true;
    }

    void PerforceComponent::TestConnectionValid()
    {
        AZ_Assert(m_ProcessThreadID != AZStd::thread::id(), "The perforce worker thread has not started.");
        AZ_Assert(AZStd::this_thread::get_id() == m_ProcessThreadID, "You may only call this function from the perforce worker thread.");

        if (!s_perforceConn)
        {
            s_perforceConn = aznew PerforceConnection();
        }

        m_validConnection = IsConnectionValid();
        m_testConnection = false;
    }

    bool PerforceComponent::UpdateConnectivity()
    {
        AZ_Assert(m_ProcessThreadID != AZStd::thread::id(), "The perforce worker thread has not started.");
        AZ_Assert(AZStd::this_thread::get_id() == m_ProcessThreadID, "You may only call this function from the perforce worker thread.");

        if (m_testConnection)
        {
            TestConnectionValid();
        }

        SourceControlState currentState = SourceControlState::Disabled;
        if (!m_offlineMode)
        {
            currentState = (m_trustedKey && m_validConnection) ? SourceControlState::Active : SourceControlState::ConfigurationInvalid;
        }

        if (currentState != m_connectionState)
        {
            m_connectionState = currentState;
           
            if (AZ::TickBus::IsFunctionQueuing())
            {
                // Push to the main thread for convenience.
                AZStd::function<void()> connectivityNotify =
                    [this, currentState]()
                {
                    SourceControlNotificationBus::Broadcast(&SourceControlNotificationBus::Events::ConnectivityStateChanged, currentState);
                };
                AZ::TickBus::QueueFunction(connectivityNotify);
            }
        }

        return true;
    }

    void PerforceComponent::DropConnectivity()
    {
        AZ_Assert(m_ProcessThreadID != AZStd::thread::id(), "The perforce worker thread has not started.");
        AZ_Assert(AZStd::this_thread::get_id() == m_ProcessThreadID, "You may only call this function from the perforce worker thread.");

        if (!s_perforceConn)
        {
            return;
        }

        delete s_perforceConn;
        s_perforceConn = nullptr;

        AZ_TracePrintf(SCC_WINDOW, "Perforce connectivity dropped\n");
    }

    int PerforceComponent::GetOrCreateOurChangelist()
    {
        if (!CheckConnectivityForAction("list changelists", nullptr))
        {
            return 0;
        }

        int changelistNum = FindExistingChangelist();
        if (changelistNum > 0)
        {
            return changelistNum;
        }

        ProcessWatcher* pWatcher = s_perforceConn->m_command.ExecuteNewChangelistInput();
        AZ_Assert(pWatcher, "No process found for p4!");
        if (!pWatcher)
        {
            return 0;
        }

        ProcessCommunicator* pCommunicator = pWatcher->GetCommunicator();
        AZ_Assert(pCommunicator, "No communicator found for p4!");
        if (!pCommunicator)
        {
            return 0;
        }

        AZStd::string changelistForm = s_perforceConn->m_command.CreateChangelistForm(s_perforceConn->GetClientName(), s_perforceConn->GetUser(), m_autoChangelistDescription);
        AZ_TracePrintf(SCC_WINDOW, "Perforce - Changelist Form\n%s", changelistForm.c_str());

        pCommunicator->WriteInput(changelistForm.data(), static_cast<AZ::u32>(changelistForm.size()));
        if (!pWatcher->WaitForProcessToExit(30))
        {
            pWatcher->TerminateProcess(0);
        }
        delete pWatcher;

        changelistNum = FindExistingChangelist();
        if (changelistNum > 0)
        {
            AZ_TracePrintf(SCC_WINDOW, "Perforce - Changelist number is %i\n", changelistNum);
        }
        else
        {
            AZ_TracePrintf(SCC_WINDOW, "Perforce - %s changelist could not be found, problem creating changelist!\n", m_autoChangelistDescription.c_str());
        }

        return changelistNum;
    }

    int PerforceComponent::FindExistingChangelist()
    {
        s_perforceConn->m_command.ExecuteShowChangelists(s_perforceConn->GetUser(), s_perforceConn->GetClientName());
        if (!CommandSucceeded())
        {
            return 0;
        }

        if (ParseDuplicateOutput(s_perforceConn->m_command.m_commandOutputMapList, s_perforceConn->m_command.m_rawOutput.outputResult))
        {
            int foundChangelist = 0;
            AZStd::vector<PerforceMap>::iterator perforceIterator = s_perforceConn->m_command.FindMapWithPartiallyMatchingValueForKey("desc", m_autoChangelistDescription);
            if (perforceIterator != nullptr)
            {
                const PerforceMap foundPerforceMap = *perforceIterator;
                if (s_perforceConn->m_command.OutputKeyExists("change", foundPerforceMap))
                {
                    foundChangelist = atoi(s_perforceConn->m_command.GetOutputValue("change", foundPerforceMap).c_str());
                }
            }

            if (foundChangelist > 0)
            {
                return foundChangelist;
            }
        }

        return 0;
    }

    void PerforceComponent::ThreadWorker()
    {
        m_ProcessThreadID = AZStd::this_thread::get_id();
        while (1)
        {
            // block until signaled:
            m_WorkerSemaphore.acquire();
            if (m_shutdownThreadSignal)
            {
                break; // abandon ship!
            }

            // wait for trust issues to be resolved
            if (!UpdateTrust())
            {
                continue;
            }

            if (!UpdateConnectivity())
            {
                continue;
            }

            if (m_workerQueue.empty())
            {
                continue;
            }

            // if we get here, a job is waiting for us!  pop it off the queue (use the queue for as short a time as possible
            PerforceJobRequest topReq;
            {
                AZStd::lock_guard<AZStd::mutex> locker(m_WorkerQueueMutex);
                topReq = m_workerQueue.front();
                m_workerQueue.pop();
            }

            // execute the queued item
            if (!m_offlineMode)
            {
                ProcessJob(topReq);
            }
            else
            {
                ProcessJobOffline(topReq);
            }
        }
        DropConnectivity();
        m_ProcessThreadID = AZStd::thread::id();
    }

    void PerforceComponent::ProcessJob(const PerforceJobRequest& request)
    {
        AZ_Assert(m_ProcessThreadID != AZStd::thread::id(), "The perforce worker thread has not started.");
        AZ_Assert(AZStd::this_thread::get_id() == m_ProcessThreadID, "You may only call this function from the perforce worker thread.");

        PerforceJobResult resp;
        bool validPerforceRequest = true;
        switch (request.m_requestType)
        {
        case PerforceJobRequest::PJR_Stat:
        {
            resp.m_fileInfo = AZStd::move(GetFileInfo(request.m_requestPath.c_str()));
            resp.m_succeeded = resp.m_fileInfo.m_status > SCS_NUM_ERRORS;
        }
        break;
        case PerforceJobRequest::PJR_Edit:
        {
            resp.m_succeeded = RequestEdit(request.m_requestPath.c_str(), request.m_allowMultiCheckout);
            resp.m_fileInfo = AZStd::move(GetFileInfo(request.m_requestPath.c_str()));
        }
        break;
        case PerforceJobRequest::PJR_Delete:
        {
            resp.m_succeeded = RequestDelete(request.m_requestPath.c_str());
            resp.m_fileInfo = AZStd::move(GetFileInfo(request.m_requestPath.c_str()));
        }
        break;
        case PerforceJobRequest::PJR_Revert:
        {
            resp.m_succeeded = RequestRevert(request.m_requestPath.c_str());
            resp.m_fileInfo = AZStd::move(GetFileInfo(request.m_requestPath.c_str()));
        }
        break;
        default:
        {
            validPerforceRequest = false;
            AZ_Assert(false, "Invalid type of perforce job request.");
        }
        break;
        }

        if (validPerforceRequest)
        {
            resp.m_callback = request.m_callback;
            {
                AZStd::lock_guard<AZStd::mutex> locker(m_ResultQueueMutex);
                m_resultQueue.push(AZStd::move(resp));
                EBUS_QUEUE_FUNCTION(AZ::TickBus, &PerforceComponent::ProcessResultQueue, this);// narrow events to the main thread.
            }
        }
    }

    void PerforceComponent::ProcessJobOffline(const PerforceJobRequest& request)
    {
        AZ_Assert(m_ProcessThreadID != AZStd::thread::id(), "The perforce worker thread has not started.");
        AZ_Assert(AZStd::this_thread::get_id() == m_ProcessThreadID, "You may only call this function from the perforce worker thread.");
        AZ_Assert(m_offlineMode, "Processing Source Control Jobs in offline mode when we are connected to Source Control");

        switch (request.m_requestType)
        {
        case PerforceJobRequest::PJR_Stat:
            m_nullSCComponent.GetFileInfo(request.m_requestPath.c_str(), request.m_callback);
            break;
        case PerforceJobRequest::PJR_Edit:
            m_nullSCComponent.RequestEdit(request.m_requestPath.c_str(), request.m_allowMultiCheckout, request.m_callback);
            break;
        case PerforceJobRequest::PJR_Delete:
            m_nullSCComponent.RequestDelete(request.m_requestPath.c_str(), request.m_callback);
            break;
        case PerforceJobRequest::PJR_Revert:
            m_nullSCComponent.RequestRevert(request.m_requestPath.c_str(), request.m_callback);
            break;
        default:
            AZ_Assert(false, "Invalid type of perforce job request.");
            break;
        }
    }

    // narrow events to the main thread.
    void PerforceComponent::ProcessResultQueue()
    {
        while (!m_resultQueue.empty())
        {
            PerforceJobResult resp;
            {
                AZStd::lock_guard<AZStd::mutex> locker(m_ResultQueueMutex);
                resp = AZStd::move(m_resultQueue.front());
                m_resultQueue.pop();
            }
            // dispatch the callback outside the scope of the lock so that more elements could be queued by threads...
            resp.m_callback(resp.m_succeeded, resp.m_fileInfo);
        }

        while (!m_settingsQueue.empty())
        {
            PerforceSettingResult result;
            {
                AZStd::lock_guard<AZStd::mutex> locker(m_SettingsQueueMutex);
                result = AZStd::move(m_settingsQueue.front());
                m_settingsQueue.pop();
            }
            // dispatch the callback outside the scope of the lock so that more elements could be queued by threads...
            result.m_callback(result.m_settingInfo);
        }
    }

    bool PerforceComponent::ParseOutput(PerforceMap& perforceMap, AZStd::string& perforceOutput, const char* lineDelim) const
    {
        const char* delimiter = lineDelim ? lineDelim : " ";

        // replace "..." with empty strings, each line is in the format of "... key value"
        AzFramework::StringFunc::Replace(perforceOutput, "...", "");

        // tokenize the buffer into line by line key/value pairs
        AZStd::vector<AZStd::string> lineTokens;
        AzFramework::StringFunc::Tokenize(perforceOutput.c_str(), lineTokens, "\r\n");

        // add all key/value pairs to a map
        perforceMap.clear();
        AZStd::vector<AZStd::string> keyValueTokens;
        AZStd::string key, value;
        for (auto lineToken : lineTokens)
        {
            // tokenize line, add kvp, clear tokens
            AzFramework::StringFunc::Tokenize(lineToken.c_str(), keyValueTokens, delimiter);

            // ensure we found a key
            if (keyValueTokens.size() > 0)
            {
                key = keyValueTokens[0];
            }
            else
            {
                continue;
            }

            // check for a value, not all keys will have values e.g. isMapped (although most keys do)
            value = "";
            if (keyValueTokens.size() > 1)
            {
                for (int i = 1; i < keyValueTokens.size(); i++)
                {
                    value += keyValueTokens[i];
                }
            }

            perforceMap.insert(AZStd::make_pair(key, value));
            keyValueTokens.clear();
        }

        return !perforceMap.empty();
    }

    bool PerforceComponent::ParseDuplicateOutput(AZStd::vector<PerforceMap>& perforceMapList, AZStd::string& perforceOutput) const
    {
        // replace "..." with empty strings, each line is in the format of "... key value"
        AzFramework::StringFunc::Replace(perforceOutput, "...", "");

        /* insert a group divider in the middle of "\r\n\r\n", a new group of key/values seems to be noted by a double carriage return
           when the group divider is found, the current mapped values will be inserted into the vector and a new map will be started */
        AzFramework::StringFunc::Replace(perforceOutput, "\r\n\r\n", "\r\n|-|\r\n");

        // tokenize the buffer into line by line key/value pairs
        AZStd::vector<AZStd::string> lineTokens;
        AzFramework::StringFunc::Tokenize(perforceOutput.c_str(), lineTokens, "\r\n");

        // add all key/value pairs to a map
        perforceMapList.clear();
        AZStd::vector<AZStd::string> keyValueTokens;
        AZStd::string key, value;
        PerforceMap currentMap;
        for (auto lineToken : lineTokens)
        {
            // tokenize line, add kvp, clear tokens
            AzFramework::StringFunc::Tokenize(lineToken.c_str(), keyValueTokens, " ");

            // ensure we found a key
            if (keyValueTokens.size() > 0)
            {
                key = keyValueTokens[0];

                // insert the map into the vector when the kvp group divider is found
                if (key == "|-|")
                {
                    perforceMapList.push_back(currentMap);
                    currentMap.clear();
                    keyValueTokens.clear();
                    continue;
                }
            }
            else
            {
                continue;
            }

            // check for a value, not all keys will have values e.g. isMapped (although most keys do)
            value = "";
            if (keyValueTokens.size() > 1)
            {
                for (int i = 1; i < keyValueTokens.size(); i++)
                {
                    value.append(keyValueTokens[i]);

                    if (i != keyValueTokens.size() - 1)
                    {
                        value.append(" ");
                    }
                }
            }

            currentMap.insert(AZStd::make_pair(key, value));
            keyValueTokens.clear();
        }

        return !perforceMapList.empty();
    }
} // namespace AzToolsFramework