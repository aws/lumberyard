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

#include <AzToolsFramework/SourceControl/PerforceConnection.h>
#include <AzToolsFramework/Process/ProcessWatcher.h>

#define SCC_WINDOW "Source Control"

namespace AzToolsFramework
{
    namespace
    {
        char s_EndOfFileChar(26);
    }

    AZStd::string PerforceCommand::GetCurrentChangelistNumber() const
    {
        return GetOutputValue("change");
    }

    AZStd::string PerforceCommand::GetHaveRevision() const
    {
        return GetOutputValue("haveRev"); 
    }

    AZStd::string PerforceCommand::GetHeadRevision() const
    {
        return GetOutputValue("headRev");
    }

    AZStd::string PerforceCommand::GetOtherUserCheckedOut() const
    {
        return GetOutputValue("otherOpen0");
    }

    int PerforceCommand::GetOtherUserCheckOutCount() const
    {
        return atoi(GetOutputValue("otherOpen").c_str());
    }

    bool PerforceCommand::CurrentActionIsAdd() const
    {
        return GetOutputValue("action") == "add";
    }

    bool PerforceCommand::CurrentActionIsEdit() const
    {
        return GetOutputValue("action") == "edit";
    }

    bool PerforceCommand::CurrentActionIsDelete() const
    {
        return GetOutputValue("action") == "delete";
    }

    bool PerforceCommand::FileExists() const
    {
        return m_rawOutput.errorResult.find("no such file(s)") == AZStd::string::npos;
    }

    bool PerforceCommand::HasRevision() const
    {
        return atoi(GetOutputValue("haveRev").c_str()) > 0;
    }

    bool PerforceCommand::HeadActionIsDelete() const
    {
        return GetOutputValue("headAction") == "delete";
    }

    bool PerforceCommand::IsMarkedForAdd() const
    {
        return m_rawOutput.outputResult.find("can't edit (already opened for add)") != AZStd::string::npos;
    }

    bool PerforceCommand::NeedsReopening() const
    {
        return m_rawOutput.outputResult.find("use 'reopen'") != AZStd::string::npos;
    }

    bool PerforceCommand::IsOpenByOtherUsers() const
    {
        return OutputKeyExists("otherOpen");
    }

    bool PerforceCommand::IsOpenByCurrentUser() const 
    {
        return OutputKeyExists("action");
    }

    bool PerforceCommand::NewFileAfterDeletedRev() const
    {
        return (HeadActionIsDelete() && !HasRevision());
    }

    bool PerforceCommand::ApplicationFound() const
    {
        return m_applicationFound;
    }

    bool PerforceCommand::HasTrustIssue() const
    {
        if (m_rawOutput.errorResult.find("The authenticity of ") != AZStd::string::npos &&
            m_rawOutput.errorResult.find("can't be established,") != AZStd::string::npos)
        {
            return true;
        }

        return false;
    }

    bool PerforceCommand::ExclusiveOpen() const
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

    AZStd::string PerforceCommand::GetOutputValue(const AZStd::string& key) const
    {
        PerforceMap::const_iterator kvp = m_commandOutputMap.find(key);
        if (kvp != m_commandOutputMap.end())
        {
            return kvp->second;
        }
        else
        {
            return "";
        }
    }

    AZStd::string PerforceCommand::GetOutputValue(const AZStd::string& key, PerforceMap perforceMap) const
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

    bool PerforceCommand::OutputKeyExists(const AZStd::string& key) const
    {
        PerforceMap::const_iterator kvp = m_commandOutputMap.find(key);
        return kvp != m_commandOutputMap.end();
    }

    bool PerforceCommand::OutputKeyExists(const AZStd::string& key, PerforceMap perforceMap) const
    {
        PerforceMap::const_iterator kvp = perforceMap.find(key);
        return kvp != perforceMap.end();
    }

    AZStd::vector<PerforceMap>::iterator PerforceCommand::FindMapWithPartiallyMatchingValueForKey(const AZStd::string& key, const AZStd::string& value)
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

    AZStd::string PerforceCommand::CreateChangelistForm(const AZStd::string& client, const AZStd::string& user, const AZStd::string& description)
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
        changelistForm += s_EndOfFileChar;
        return changelistForm;
    }

    void PerforceCommand::ExecuteAdd(const AZStd::string& changelist, const AZStd::string& filePath)
    {
        m_commandArgs = "add -c " + changelist + " \"" + filePath + "\"";
        ExecuteCommand();
    }

    void PerforceCommand::ExecuteClaimChangedFile(const AZStd::string& filePath, const AZStd::string& changeList)
    {
        m_commandArgs = "reopen -c " + changeList + " \"" + filePath + "\"";
        ExecuteCommand();
    }

    void PerforceCommand::ExecuteDelete(const AZStd::string& changelist, const AZStd::string& filePath)
    {
        m_commandArgs = "delete -c " + changelist + " \"" + filePath + "\"";
        ExecuteCommand();
    }

    void PerforceCommand::ExecuteEdit(const AZStd::string& changelist, const AZStd::string& filePath)
    {
        m_commandArgs = "edit -c " + changelist + " \"" + filePath + "\"";
        ExecuteCommand();
    }

    void PerforceCommand::ExecuteFstat(const AZStd::string& filePath)
    {
        m_commandArgs = "fstat \"" + filePath + "\"";
        ExecuteCommand();
    }

    void PerforceCommand::ExecuteSync(const AZStd::string& filePath)
    {
        m_commandArgs = "sync -f \"" + filePath + "\"";
        ExecuteCommand();
    }

    void PerforceCommand::ExecuteMove(const AZStd::string& changelist, const AZStd::string& sourcePath, const AZStd::string& destPath)
    {
        m_commandArgs = "move -c " + changelist + " \"" + sourcePath + "\"" + " \"" + destPath + "\"";
        ExecuteCommand();
    }

    void PerforceCommand::ExecuteSet()
    {
        m_commandArgs = "set";
        ExecuteRawCommand();
    }

    void PerforceCommand::ExecuteSet(const AZStd::string& key, const AZStd::string& value)
    {
        m_commandArgs = "set " + key + '=' + value;
        ExecuteIOCommand();
    }

    void PerforceCommand::ExecuteInfo()
    {
        m_commandArgs = "info";
        ExecuteCommand();
    }

    void PerforceCommand::ExecuteShortInfo()
    {
        m_commandArgs = "info -s";
        ExecuteCommand();
    }

    void PerforceCommand::ExecuteTicketStatus()
    {
        m_commandArgs = "login -s";
        ExecuteCommand();
    }

    void PerforceCommand::ExecuteTrust(bool enable, const AZStd::string& fingerprint)
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

    ProcessWatcher* PerforceCommand::ExecuteNewChangelistInput()
    {
        m_commandArgs = "change -i";
        return ExecuteIOCommand();
    }

    void PerforceCommand::ExecuteNewChangelistOutput()
    {
        m_commandArgs = "change -o";
        ExecuteRawCommand();
    }

    void PerforceCommand::ExecuteRevert(const AZStd::string& filePath)
    {
        m_commandArgs = "revert \"" + filePath + "\"";
        ExecuteCommand();
    }

    void PerforceCommand::ExecuteShowChangelists(const AZStd::string& currentUser, const AZStd::string& currentClient)
    {
        m_commandArgs = "changes -s pending -c " + currentClient;
        if (!currentUser.empty() && currentUser != "*unknown*")
        {
            m_commandArgs += " -u " + currentUser;
        }
        ExecuteCommand();
    }

    void PerforceCommand::ThrowWarningMessage()
    {
        // This can happen all the time, for various reasons.  AZ_Warning will actually cause the application to
        // take a stack dump and will load pdbs, which can introduce a serious delay during startup.  As such,
        // we send it as a Trace, not a warning.  Background threads retrying may hit this many times.
        AZ_TracePrintf(SCC_WINDOW, "Perforce Warning - Command has failed '%s'\n", m_commandArgs.c_str());
    }

    void PerforceCommand::ExecuteCommand()
    {
        m_rawOutput.Clear();
        ProcessLauncher::ProcessLaunchInfo processLaunchInfo;
        processLaunchInfo.m_commandlineParameters = "p4 -ztag " + m_commandArgs;
        processLaunchInfo.m_showWindow = false;
        ProcessWatcher::LaunchProcessAndRetrieveOutput(processLaunchInfo, ProcessCommunicationType::COMMUNICATOR_TYPE_STDINOUT, m_rawOutput);
        m_applicationFound = processLaunchInfo.m_launchResult == ProcessLauncher::PLR_MissingFile ? false : true;
    }

    ProcessWatcher* PerforceCommand::ExecuteIOCommand()
    {
        ProcessLauncher::ProcessLaunchInfo processLaunchInfo;
        processLaunchInfo.m_commandlineParameters = "p4 " + m_commandArgs;
        processLaunchInfo.m_showWindow = false;
        ProcessWatcher* processWatcher = ProcessWatcher::LaunchProcess(processLaunchInfo, ProcessCommunicationType::COMMUNICATOR_TYPE_STDINOUT);
        m_applicationFound = processLaunchInfo.m_launchResult == ProcessLauncher::PLR_MissingFile ? false : true;
        return processWatcher;
    }

    void PerforceCommand::ExecuteRawCommand()
    {
        m_rawOutput.Clear();
        ProcessLauncher::ProcessLaunchInfo processLaunchInfo;
        processLaunchInfo.m_commandlineParameters = "p4 " + m_commandArgs;
        processLaunchInfo.m_showWindow = false;
        ProcessWatcher::LaunchProcessAndRetrieveOutput(processLaunchInfo, ProcessCommunicationType::COMMUNICATOR_TYPE_STDINOUT, m_rawOutput);
        m_applicationFound = processLaunchInfo.m_launchResult == ProcessLauncher::PLR_MissingFile ? false : true;
    }

    AZStd::string PerforceConnection::GetUser() const
    {
        return GetInfoValue("userName");
    }

    AZStd::string PerforceConnection::GetClientName() const
    { 
        return GetInfoValue("clientName");
    }

    AZStd::string PerforceConnection::GetClientRoot() const
    { 
        return GetInfoValue("clientRoot");
    }

    AZStd::string PerforceConnection::GetServerAddress() const
    { 
        return GetInfoValue("serverAddress");
    }

    AZStd::string PerforceConnection::GetServerUptime() const
    { 
        return GetInfoValue("serverUptime");
    }

    AZStd::string PerforceConnection::GetInfoValue(const AZStd::string& key) const
    {
        PerforceMap::const_iterator kvp = m_infoResultMap.find(key);
        if (kvp != m_infoResultMap.end())
        {
            return kvp->second;
        }
        return "";
    }

    bool PerforceConnection::CommandHasOutput() const
    { 
        return m_command.m_rawOutput.HasOutput();
    }

    bool PerforceConnection::CommandHasError() const
    { 
        return m_command.m_rawOutput.HasError();
    }

    bool PerforceConnection::CommandHasTrustIssue() const
    { 
        return m_command.HasTrustIssue();
    }

    bool PerforceConnection::CommandApplicationFound() const
    {
        return m_command.ApplicationFound();
    }

    AZStd::string PerforceConnection::GetCommandOutput() const
    { 
        return m_command.m_rawOutput.outputResult;
    }

    AZStd::string PerforceConnection::GetCommandError() const
    {
        return m_command.m_rawOutput.errorResult;
    }

    bool PerforceConnection::CommandHasFailed()
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
} // namespace AzToolsFramework