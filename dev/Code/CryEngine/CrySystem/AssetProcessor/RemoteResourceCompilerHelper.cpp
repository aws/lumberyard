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
#include "RemoteResourceCompilerHelper.h"
#include "IEngineConnection.h"
#include "LineStreamBuffer.h"

#include <string>
#include <sstream>
#include <functional>

namespace // internal
{
    unsigned int s_nextJobId = 1;

    class RemoteProcessHandle
        : public IResourceCompilerProcess
    {
    public:
        RemoteProcessHandle(int rcJobID, IResourceCompilerListener* listener)
            : m_rcJobID(rcJobID)
            , m_messageListener(listener)
            , m_isDone(false)
            , m_exitCode(eRcExitCode_Pending)

        {
        }

        virtual ~RemoteProcessHandle()
        {
        }

        void HandleLine(const char* line)
        {
            if (!m_messageListener || !line)
            {
                return;
            }

            // check the first three characters to see if it's a warning or error.
            bool bHasPrefix;
            IResourceCompilerListener::MessageSeverity severity;
            if ((line[0] == 'E') && (line[1] == ':') && (line[2] == ' '))
            {
                bHasPrefix = true;
                severity = IResourceCompilerListener::MessageSeverity_Error;
                line += 3;  // skip the prefix
            }
            else if ((line[0] == 'W') && (line[1] == ':') && (line[2] == ' '))
            {
                bHasPrefix = true;
                severity = IResourceCompilerListener::MessageSeverity_Warning;
                line += 3;  // skip the prefix
            }
            else if ((line[0] == ' ') && (line[1] == ' ') && (line[2] == ' '))
            {
                bHasPrefix = true;
                severity = IResourceCompilerListener::MessageSeverity_Info;
                line += 3;  // skip the prefix
            }
            else
            {
                bHasPrefix = false;
                severity = IResourceCompilerListener::MessageSeverity_Info;
            }

            if (bHasPrefix)
            {
                // skip thread info "%d>", if present
                {
                    const char* p = line;
                    while (*p == ' ')
                    {
                        ++p;
                    }
                    if (isdigit(*p))
                    {
                        while (isdigit(*p))
                        {
                            ++p;
                        }
                        if (*p == '>')
                        {
                            line = p + 1;
                        }
                    }
                }

                // skip time info "%d:%d", if present
                {
                    const char* p = line;
                    while (*p == ' ')
                    {
                        ++p;
                    }
                    if (isdigit(*p))
                    {
                        while (isdigit(*p))
                        {
                            ++p;
                        }
                        if (*p == ':')
                        {
                            ++p;
                            if (isdigit(*p))
                            {
                                while (isdigit(*p))
                                {
                                    ++p;
                                }
                                while (*p == ' ')
                                {
                                    ++p;
                                }
                                line = p;
                            }
                        }
                    }
                }
            }

            m_messageListener->OnRCMessage(severity, line);
        }

        bool IsDone() override
        {
            return m_isDone;
        }

        void WaitForDone() override
        {
            if (m_isDone)
            {
                return;
            }

            m_doneEvent.Wait();
        }

        void Terminate() override
        {
            // send a message to the remote interface.  It will tell you that you've been terminated
            // this function is expected to not return until actually terminated, though
            // so wait for the response here.

            char buffer[32] = { 0 };
            sprintf_s(buffer, 32, "%u|%u", m_rcJobID, CCrc32::Compute("terminate"));
            gEnv->pEngineConnection->SendMsg(CCrc32::Compute("rcRequest"), buffer, strlen(buffer));

            // block until done
            WaitForDone();

            // it's invalid to return from this function without setting listener to null
            m_messageListener = nullptr;
        }

        int GetExitCode() override
        {
            return m_exitCode;
        }

        unsigned int GetJobID() const { return m_rcJobID; }

        void ConsumeMessage(uint32_t messageID, std::string const& dataString)
        {
            // depending on contents of message, update current values;
            // e.g. if its done, set m_isDone to true and fill in the exit code
            // note that if its some sort of console log message, and listener is NON-NULL, we need to pump the message
            // to the listener's message function.

            // extract exit code
            std::stringstream(dataString.substr(0, dataString.find('|'))) >> m_exitCode;

            // extract rc output
            std::string rcOutString(dataString.substr(dataString.find('|') + 1));

            // pass rcOut to listener
            if (m_messageListener)
            {
                // Read all the output
                LineStreamBuffer lineBuffer(this, &RemoteProcessHandle::HandleLine);
                lineBuffer.HandleText(rcOutString.c_str(), rcOutString.length());
            }

            // wake up anyone waiting for this job
            m_isDone = true;
            m_doneEvent.Set();
        }

    protected:
        unsigned int m_rcJobID;
        volatile bool m_isDone; // volatile because it can be modified by another thread.

        CryEvent m_doneEvent;

        int m_exitCode;
        IResourceCompilerListener* m_messageListener;
    };
}

namespace AssetProcessor
{
    RemoteResourceCompilerHelper::RemoteResourceCompilerHelper()
    {
        // std::function doesn't bind to a member function, it binds to a static function
        // we use std::bind to generate an adapter to the member function belonging to THIS
        // and then we pass-thru the existing 3 parameters (id, void pointer, and length) as the 3 placeholders to that function

        gEnv->pEngineConnection->AddTypeCallback(CCrc32::Compute("rcResponse"), std::bind(&RemoteResourceCompilerHelper::OnRcResponseReceived, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    }

    RemoteResourceCompilerHelper::~RemoteResourceCompilerHelper()
    {
    }

    IResourceCompilerHelper::ERcCallResult RemoteResourceCompilerHelper::CallResourceCompiler(
        const char* szFileName,
        const char* szAdditionalSettings,
        IResourceCompilerListener* listener,
        bool bMayShowWindow,
        IResourceCompilerHelper::ERcExePath rcExePath,
        bool bSilent,
        bool bNoUserDialog,
        const wchar_t* szWorkingDirectory,
        const wchar_t* szRootPath)
    {
        // start a remote async job, block until its done.
        // when its done, extract its exit code, and return it.
        IResourceCompilerHelper::RCProcessHandle handle = AsyncCallResourceCompiler(szFileName, szAdditionalSettings, bMayShowWindow, rcExePath, bSilent, bNoUserDialog, szWorkingDirectory, szRootPath, listener);
        if (!handle)
        {
            return eRcCallResult_notFound; // rc not present or not ready
        }
        handle->WaitForDone();

        return ConvertResourceCompilerExitCodeToResultCode(handle->GetExitCode());
    }

    // this is an ASYNCHRONOUS call.  The object returned will let you check the state of, and also manipulate, the RC process executing.
    IResourceCompilerHelper::RCProcessHandle RemoteResourceCompilerHelper::AsyncCallResourceCompiler(
        const char* szFileName,
        const char* szAdditionalSettings,
        bool bMayShowWindow,
        IResourceCompilerHelper::ERcExePath rcExePath,
        bool bSilent,
        bool bNoUserDialog,
        const wchar_t* szWorkingDirectory,
        const wchar_t* szRootPath,
        IResourceCompilerListener* listener)
    {
        // check gEnv, gEnv->pEngineConnection, and gEnv->pEngineConnection->GetConnectionState()
        // to make sure we are ready to send messages
        if (!gEnv || !gEnv->pEngineConnection || gEnv->pEngineConnection->GetConnectionState() != IEngineConnection::EConnectionState::Connected)
        {
            // engine connection not ready or not connected to server
            return nullptr;
        }

        unsigned int newJobID = s_nextJobId++;
        RemoteProcessHandle* pHandle = new RemoteProcessHandle(newJobID, listener);

        std::string commandString;

        if (!szFileName)
        {
            commandString.append(" /userdialog=0 ");
            commandString.append(szAdditionalSettings);
            commandString.append(" ");
        }
        else
        {
            commandString.append(" \"");
            commandString.append(szFileName);
            commandString.append("\" ");
            commandString.append(bNoUserDialog ? " /userdialog=0 " : " /userdialog=1 ");
            commandString.append(szAdditionalSettings);
            commandString.append(" ");
        }

        char buffer[2048] = {0};
        sprintf_s(buffer, 2048, "%u|%u|%s", newJobID, CCrc32::Compute("execute"), commandString.c_str());

        // note:  Because we're sending strlen(buffer) bytes, and not strlen(buffer) + 1 bytes, we will not be sending the null terminator
        // this means that on the other side you cannot just cast to char*
        // maybe for simplicity sake, consider sending strlen(buffer) + 1 bytes (to include the null), and then on the other side
        // you can just reinterpret as char* and it will be properly terminated and std::strings can be constructed around it.

        // make the request
        gEnv->pEngineConnection->SendMsg(CCrc32::Compute("rcRequest"), buffer, strlen(buffer));

        // add handle to list of handles
        RCProcessHandle rcProcessHandle = std::shared_ptr<IResourceCompilerProcess>(pHandle, [](IResourceCompilerProcess* proc) { delete proc; });
        m_currentHandles.push_back(rcProcessHandle);

        // return handle
        return rcProcessHandle;
    }

    void RemoteResourceCompilerHelper::OnRcResponseReceived(uint32_t messageID, const void* data, size_t dataSize)
    {
        // the message we expect is of type "rcResponse" and has contents "jobID|exitCode|rcOut"

        // cast data to string
        std::string dataString((char*)data, dataSize);

        // extract job ID
        unsigned int jobID;
        std::stringstream(dataString.substr(0, dataString.find('|'))) >> jobID;

        // remove job ID from front of message (not needed)
        dataString = dataString.substr(dataString.find('|') + 1);

        bool foundJob = false;
        for (auto it = m_currentHandles.begin(); it != m_currentHandles.end(); ++it)
        {
            auto handle = *it;
            RemoteProcessHandle* proc = static_cast<RemoteProcessHandle*>(handle.get());
            if (proc->GetJobID() == jobID)
            {
                // this message is relevant to proc's interests.
                foundJob = true;

                // send the rest of the message to RemoteProcessHandle
                // (messageID not actually needed in ConsumeMessage, consider removing)
                proc->ConsumeMessage(messageID, dataString);
                if (proc->IsDone())
                {
                    m_currentHandles.erase(it); // remove it, no more messages will come through for this handle.
                }
                break;
            }
        }

        if (!foundJob)
        {
            // received a message with a jobID we were not expecting...
            CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "urceCompilerHelper::OnRcResponseReceived - Received a message with unexpected job ID %u", jobID);
        }
    }
}