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

#include "SevenZipComponent.h"

#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/EditContext.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/Process/ProcessCommunicator.h>
#include <AzToolsFramework/Process/ProcessWatcher.h>

namespace AzToolsFramework
{
    static const char* s_traceName = "7z";

    // Echoes all results of stdout and stderr to console and never blocks
    class ConsoleEchoCommunicator
    {
    public:
        ConsoleEchoCommunicator(AzToolsFramework::ProcessCommunicator* communicator)
            : m_communicator(communicator)
        {
        }

        ~ConsoleEchoCommunicator()
        {
        }

        // Call this periodically to drain the buffers
        void Pump()
        {
            if (m_communicator->IsValid())
            {
                AZ::u32 readBufferSize = 0;
                AZStd::string readBuffer;
                // Don't call readOutput unless there is output or else it will block...
                readBufferSize = m_communicator->PeekOutput();
                if (readBufferSize)
                {
                    readBuffer.resize_no_construct(readBufferSize);
                    m_communicator->ReadOutput(readBuffer.data(), readBufferSize);
                    EchoBuffer(readBuffer);
                }
                readBufferSize = m_communicator->PeekError();
                if (readBufferSize)
                {
                    readBuffer.resize_no_construct(readBufferSize);
                    m_communicator->ReadError(readBuffer.data(), readBufferSize);
                    EchoBuffer(readBuffer);
                }
            }
        }

    private:
        void EchoBuffer(const AZStd::string& buffer)
        {
            size_t startIndex = 0;
            size_t endIndex = 0;
            const size_t bufferSize = buffer.size();
            for (size_t i = 0; i < bufferSize; ++i)
            {
                if (buffer[i] == '\n' || buffer[i] == '\0')
                {
                    endIndex = i;
                    AZ_Printf(s_traceName, "%s", buffer.substr(startIndex, endIndex - startIndex).c_str());
                    startIndex = endIndex + 1;
                }
            }
        }

        AzToolsFramework::ProcessCommunicator* m_communicator = nullptr;
    };

    void SevenZipComponent::Activate()
    {
        const char* rootPath = nullptr;
        EBUS_EVENT_RESULT(rootPath, AZ::ComponentApplicationBus, GetAppRoot);
        AzFramework::StringFunc::Path::ConstructFull(rootPath, "Tools", "7za", ".exe", m_7zPath);

        ArchiveCommands::Bus::Handler::BusConnect();
    }

    void SevenZipComponent::Deactivate()
    {
        ArchiveCommands::Bus::Handler::BusDisconnect();

        AZStd::unique_lock<AZStd::mutex> lock(m_threadControlMutex);
        for (auto pair : m_threadInfoMap)
        {
            ThreadInfo& info = pair.second;
            info.shouldStop = true;
            m_cv.wait(lock, [&info]() {
                return info.threads.size() == 0;
            });
        }
        m_threadInfoMap.clear();
    }

    void SevenZipComponent::Reflect(AZ::ReflectContext * context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SevenZipComponent, AZ::Component>()
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<SevenZipComponent>(
                    "Seven Zip", "Handles creation and extraction of zip archives.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Editor")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ;
            }
        }
    }

    void SevenZipComponent::ExtractArchive(const AZStd::string& archivePath, const AZStd::string& destinationPath, AZ::Uuid taskHandle, const ArchiveResponseCallback& respCallback)
    {
        // Extract archive path to destionationPath\<archiveFileName> and skipping extracting of existing files (aos)
        AZStd::string commandLineArgs = AZStd::string::format(R"(x "%s" -o"%s\*" -aos)", archivePath.c_str(), destinationPath.c_str());
        Launch7zAsync(commandLineArgs, taskHandle, respCallback);
    }

    void SevenZipComponent::CancelTasks(AZ::Uuid taskHandle)
    {
        AZStd::unique_lock<AZStd::mutex> lock(m_threadControlMutex);

        auto it = m_threadInfoMap.find(taskHandle);
        if (it == m_threadInfoMap.end())
        {
            return;
        }

        ThreadInfo& info = it->second;
        info.shouldStop = true;
        m_cv.wait(lock, [&info]() {
            return info.threads.size() == 0;
        });
        m_threadInfoMap.erase(it);
    }

    void SevenZipComponent::Launch7zAsync(const AZStd::string& commandLineArgs, AZ::Uuid taskHandle, const ArchiveResponseCallback& respCallback)
    {
        AZStd::unique_lock<AZStd::mutex> lock(m_threadControlMutex);
        m_threadInfoMap[taskHandle].shouldStop = false;

        AZStd::thread processThread([=]()
        {
            {
                AZStd::unique_lock<AZStd::mutex> lock(m_threadControlMutex);
                m_threadInfoMap[taskHandle].threads.insert(AZStd::this_thread::get_id());
                m_cv.notify_all();
            }

            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(5000));

            ProcessLauncher::ProcessLaunchInfo pInfo;
            pInfo.m_commandlineParameters = m_7zPath + " " + commandLineArgs;
            pInfo.m_showWindow = false;
            AZStd::unique_ptr<ProcessWatcher> watcher(ProcessWatcher::LaunchProcess(pInfo, ProcessCommunicationType::COMMUNICATOR_TYPE_STDINOUT));

            AZ::u32 exitCode = static_cast<AZ::u32>(SevenZipExitCode::UserStoppedProcess);
            if (watcher)
            {
                ConsoleEchoCommunicator echoCommunicator(watcher->GetCommunicator());
                while (watcher->IsProcessRunning(&exitCode))
                {
                    {
                        AZStd::unique_lock<AZStd::mutex> lock(m_threadControlMutex);
                        if (m_threadInfoMap[taskHandle].shouldStop)
                        {
                            watcher->TerminateProcess(static_cast<AZ::u32>(SevenZipExitCode::UserStoppedProcess));
                        }
                    }
                    echoCommunicator.Pump();
                }
            }
            EBUS_QUEUE_FUNCTION(AZ::TickBus, respCallback, (exitCode == static_cast<AZ::u32>(SevenZipExitCode::NoError)));

            {
                AZStd::unique_lock<AZStd::mutex> lock(m_threadControlMutex);
                ThreadInfo& tInfo = m_threadInfoMap[taskHandle];
                tInfo.threads.erase(AZStd::this_thread::get_id());
                m_cv.notify_all();
            }
        });
        ThreadInfo& info = m_threadInfoMap[taskHandle];
        m_cv.wait(lock, [&info, &processThread]() {
            return info.threads.find(processThread.get_id()) != info.threads.end();
        });
        processThread.detach();
    }
} // namespace AzToolsFramework
