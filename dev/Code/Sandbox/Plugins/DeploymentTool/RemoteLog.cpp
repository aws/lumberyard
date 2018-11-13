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
#include "DeploymentTool_precompiled.h"
#include "RemoteLog.h"

#include <AzCore/Socket/AzSocket.h>
#include <AzCore/std/smart_ptr/scoped_ptr.h>

#include <ILevelSystem.h>
#include <ISystem.h>
#include <RemoteConsoleCore.h>

#include "DeployNotificationsBus.h"


namespace
{
    typedef SStringEvent<eCET_LogMessage>* InfoLogEventPtr;
    typedef SStringEvent<eCET_LogWarning>* WarnLogEventPtr;
    typedef SStringEvent<eCET_LogError>* ErrorLogEventPtr;

    typedef SStringEvent<eCET_ConsoleCommand> ConsoleCommandEvent;


    const int defaultBufferSize = 4096;

    const int delayAfterSocketErrorInMilliseconds = 1000; // 1 second delay


    ILevelSystem* GetLevelSystem()
    {
        if (gEnv && gEnv->pSystem)
        {
            return gEnv->pSystem->GetILevelSystem();
        }
        return nullptr;
    }
}


namespace DeployTool
{
    RemoteLog::RemoteLog()
        : m_worker()

        , m_ipAddress()
        , m_port()

        , m_socket(AZ_SOCKET_INVALID)

        , m_loadCurrentLevel(true)

        , m_isRunning(false)
        , m_stopRequested(false)
    {
    }

    RemoteLog::~RemoteLog()
    {
        Stop();
    }

    void RemoteLog::Start(const AZStd::string& deviceIpAddress, AZ::u16 port, bool loadCurrentLevel)
    {
        Stop();
        m_stopRequested = false;

        AZStd::string startingMessage = AZStd::move(AZStd::string::format("[INFO] Starting remote log, searching for %s:%hu", deviceIpAddress.c_str(), port));
        REMOTE_LOG_INFO(startingMessage.c_str());
        REMOTE_LOG_INFO("--------------------------------");

        m_ipAddress = deviceIpAddress;
        m_port = port;

        m_loadCurrentLevel = loadCurrentLevel;

        AZStd::thread_desc threadDesc;
        threadDesc.m_isJoinable = true;
        threadDesc.m_name = "Deploy Tool Remote Log";

        m_worker = AZStd::thread(AZStd::bind(&RemoteLog::Run, this), &threadDesc);
    }

    void RemoteLog::Stop()
    {
        if (m_isRunning)
        {
            m_stopRequested = true;
            m_worker.join();
        }
        m_isRunning = false;
    }

    void RemoteLog::Run()
    {
        AZ::AzSock::AzSocketAddress socketAddress;
        if (!socketAddress.SetAddress(m_ipAddress, m_port))
        {
            REMOTE_LOG_DEBUG("LOCAL | [ERROR] Failed to set the address for remote log connection.");
            m_isRunning = false;
            return;
        }

        AZ::s32 result = 0;
        AZTIMEVAL timeOut = { 1, 0 };

        m_isRunning = true;

        char buffer[defaultBufferSize] = { 0 };
        int size;
        SNoDataEvent<eCET_Noop> noopEvent;

        AZStd::scoped_ptr<ConsoleCommandEvent> mapEvent;
        if (m_loadCurrentLevel)
        {
            if (ILevelSystem* levelSystem = GetLevelSystem())
            {
                if (ILevel* level = levelSystem->GetCurrentLevel())
                {
                    if (ILevelInfo* levelInfo = level->GetLevelInfo())
                    {
                        const char* mapName = levelInfo->GetName();

                        AZStd::string mapCommand = AZStd::move(AZStd::string::format("map %s", mapName));
                        REMOTE_LOG_DEBUG("LOCAL | [INFO] Command: %s", mapCommand.c_str());

                        mapEvent.reset(new ConsoleCommandEvent(mapCommand.c_str()));
                    }
                }
            }

            if (!mapEvent)
            {
                REMOTE_LOG_ERROR("--------------------------------");
                REMOTE_LOG_ERROR("Failed to get the current level name");
                REMOTE_LOG_ERROR("--------------------------------");
            }
        }

        while (!m_stopRequested)
        {
            REMOTE_LOG_DEBUG("LOCAL | Running the connection loop");

            m_socket = AZ::AzSock::Socket();
            if (!AZ::AzSock::IsAzSocketValid(m_socket))
            {
                REMOTE_LOG_DEBUG("LOCAL | [ERROR] Failed to create socket for remote log connection.");
                break;
            }

            AZFD_SET read;
            FD_ZERO(&read);
            FD_SET(m_socket, &read);

            result = AZ::AzSock::Select(m_socket, &read, nullptr, nullptr, &timeOut);
            if (AZ::AzSock::SocketErrorOccured(result))
            {
                REMOTE_LOG_DEBUG("LOCAL | [ERROR] Failed assign a timeout when reading from the remote log socket.");
                break;
            }

            // macOS will start returning EINVAL after the first failed attempt, meaning it will never establish the connection
            // unless the socket is torn down and recreated.  This doesn't appear to happen on windows and will seemingly return
            // ECONNREFUSED indefinitely until the connection is accepted.  In the interest of keeping the code clean, socket tear
            // down on connection failure will be the default behaviour.
            result = AZ::AzSock::Connect(m_socket, socketAddress);
            if (AZ::AzSock::SocketErrorOccured(result))
            {
                REMOTE_LOG_DEBUG("LOCAL | [WARN] Connection attempt failed.  Error code: %s", AZ::AzSock::GetStringForError(result));
            }
            else
            {
                DeployTool::Notifications::Bus::Broadcast(&DeployTool::Notifications::RemoteLogConnectionStateChange, true);

                bool ok = true;
                while (ok && !m_stopRequested)
                {
                    if (mapEvent)
                    {
                        SRemoteEventFactory::GetInst()->WriteToBuffer(mapEvent.get(), buffer, size, defaultBufferSize);
                        if (Send(buffer, size))
                        {
                            mapEvent.reset(nullptr);
                        }
                    }

                    ok &= Receive(buffer, size);
                    IRemoteEvent* event = SRemoteEventFactory::GetInst()->CreateEventFromBuffer(buffer, size);
                    if (event)
                    {
                        switch (event->GetType())
                        {
                            case eCET_LogMessage:
                            {
                                InfoLogEventPtr logEvent = reinterpret_cast<InfoLogEventPtr>(event);
                                REMOTE_LOG_INFO(logEvent->GetData());
                                break;
                            }
                            case eCET_LogWarning:
                            {
                                WarnLogEventPtr logEvent = reinterpret_cast<WarnLogEventPtr>(event);
                                REMOTE_LOG_WARN(logEvent->GetData());
                                break;
                            }
                            case eCET_LogError:
                            {
                                ErrorLogEventPtr logEvent = reinterpret_cast<ErrorLogEventPtr>(event);
                                REMOTE_LOG_ERROR(logEvent->GetData());
                                break;
                            }

                            default:
                                break;
                        }

                        delete event;
                    }

                    SRemoteEventFactory::GetInst()->WriteToBuffer(&noopEvent, buffer, size, defaultBufferSize);
                    ok &= Send(buffer, size);
                }

                DeployTool::Notifications::Bus::Broadcast(&DeployTool::Notifications::RemoteLogConnectionStateChange, false);
            }

            REMOTE_LOG_DEBUG("LOCAL | Shutting down remote log socket");

            AZ::AzSock::Shutdown(m_socket, SD_BOTH);
            AZ::AzSock::CloseSocket(m_socket);

            m_socket = AZ_SOCKET_INVALID;

            TrySleepThread();
        }

        m_isRunning = false;
    }

    bool RemoteLog::Send(const char* buffer, int size)
    {
        int ret, idx = 0;
        int left = size + 1;
        AZ_Assert(buffer[size] == '\0', "Attempting to send buffer that is not null terminated!");
        while (left > 0)
        {
            ret = AZ::AzSock::Send(m_socket, &buffer[idx], left, 0);
            if (AZ::AzSock::SocketErrorOccured(ret))
            {
                REMOTE_LOG_DEBUG("LOCAL | [ERROR] Failed to send data to device.  Error Code: %s", AZ::AzSock::GetStringForError(ret));
                return false;
            }
            left -= ret;
            idx += ret;
        }
        return true;
    }

    bool RemoteLog::Receive(char* buffer, int& size)
    {
        size = 0;
        int ret, idx = 0;
        do
        {
            ret = AZ::AzSock::Recv(m_socket, buffer + idx, defaultBufferSize - idx, 0);
            if (AZ::AzSock::SocketErrorOccured(ret))
            {
                REMOTE_LOG_DEBUG("LOCAL | [ERROR] Failed to Receive data from device.  Error Code: %s", AZ::AzSock::GetStringForError(ret));
                return false;
            }
            idx += ret;
        } while (buffer[idx - 1] != '\0');
        size = idx;
        return true;
    }

    void RemoteLog::TrySleepThread()
    {
        if (!m_stopRequested)
        {
            REMOTE_LOG_DEBUG("LOCAL | Sleeping remote log thread...");
            Sleep(delayAfterSocketErrorInMilliseconds);
        }
    }

} // namespace DeployTool