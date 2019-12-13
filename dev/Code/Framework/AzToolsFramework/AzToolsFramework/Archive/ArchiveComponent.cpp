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
    const unsigned int g_sleepDuration = 1;
    const char extractArchiveCmd[] = R"(x -mmt=off "%s" -o"%s\*" -aos)";
    const char extractArchiveWithoutRootCmd[] = R"(x -mmt=off "%s" -o"%s" -aos)";

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
                    readBuffer.resize_no_construct(readBufferSize + 1);
                    readBuffer[readBufferSize] = '\0';
                    m_communicator->ReadOutput(readBuffer.data(), readBufferSize);
                    EchoBuffer(readBuffer);
                }
                readBufferSize = m_communicator->PeekError();
                if (readBufferSize)
                {
                    readBuffer.resize_no_construct(readBufferSize + 1);
                    readBuffer[readBufferSize] = '\0';
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
                    bool isEmptyMessage = (endIndex - startIndex == 1) && (buffer[startIndex] == '\r');
                    if (!isEmptyMessage)
                    {
                        AZ_Printf(s_traceName, "%s", buffer.substr(startIndex, endIndex - startIndex).c_str());
                    }
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

    void SevenZipComponent::CreateArchive(const AZStd::string& archivePath, const AZStd::string& dirToArchive, AZ::Uuid taskHandle, const ArchiveResponseOutputCallback& respCallback)
    {
        AZStd::string commandLineArgs = AZStd::string::format(R"(a -tzip -mx=1 "%s" -r "%s\*")", archivePath.c_str(), dirToArchive.c_str());
        Launch7z(commandLineArgs, respCallback, taskHandle);
    }

    bool SevenZipComponent::CreateArchiveBlocking(const AZStd::string& archivePath, const AZStd::string& dirToArchive)
    {
        AZStd::string commandLineArgs = AZStd::string::format(R"(a -tzip -mx=1 "%s" -r "%s\*")", archivePath.c_str(), dirToArchive.c_str());
        bool success = false;
        auto createArchiveCallback = [&success](bool result, AZStd::string consoleOutput) {
            success = result;
        };
        Launch7z(commandLineArgs, createArchiveCallback);
        return success;
    }

    void SevenZipComponent::ExtractArchive(const AZStd::string& archivePath, const AZStd::string& destinationPath, AZ::Uuid taskHandle, const ArchiveResponseCallback& respCallback)
    {
        ArchiveResponseOutputCallback responseHandler = [respCallback](bool result, AZStd::string /*outputStr*/) { respCallback(result); };
        ExtractArchiveOutput(archivePath, destinationPath, taskHandle, responseHandler);
    }

    void SevenZipComponent::ExtractArchiveOutput(const AZStd::string& archivePath, const AZStd::string& destinationPath, AZ::Uuid taskHandle, const ArchiveResponseOutputCallback& respCallback)
    {
        // Extract archive path to destionationPath\<archiveFileName> and skipping extracting of existing files (aos)
        AZStd::string commandLineArgs = AZStd::string::format(extractArchiveCmd, archivePath.c_str(), destinationPath.c_str());
        Launch7z(commandLineArgs, respCallback, taskHandle);
    }

    void SevenZipComponent::ExtractArchiveWithoutRoot(const AZStd::string& archivePath, const AZStd::string& destinationPath, AZ::Uuid taskHandle, const ArchiveResponseOutputCallback& respCallback)
    {
        AZStd::string commandLineArgs;
        // Extract archive path to destionationPath\<archiveFileName> and skipping extracting of existing files (aos)
        commandLineArgs = AZStd::string::format(extractArchiveWithoutRootCmd, archivePath.c_str(), destinationPath.c_str());
        Launch7z(commandLineArgs, respCallback, taskHandle);
    }

    bool SevenZipComponent::ExtractArchiveBlocking(const AZStd::string& archivePath, const AZStd::string& destinationPath, bool extractWithRootDirectory)
    {
        AZStd::string commandLineArgs;
        if (extractWithRootDirectory)
        {
            // Extract archive path to destionationPath\<archiveFileName> and skipping extracting of existing files (aos)
            commandLineArgs = AZStd::string::format(extractArchiveCmd, archivePath.c_str(), destinationPath.c_str());
        }
        else
        {
            // Extract archive path to destionationPath and skipping extracting of existing files (aos)
            commandLineArgs = AZStd::string::format(extractArchiveWithoutRootCmd, archivePath.c_str(), destinationPath.c_str());
        }
        bool success = false;
        auto extractArchiveCallback = [&success](bool result, AZStd::string consoleOutput) {
            success = result;
        };
        Launch7z(commandLineArgs, extractArchiveCallback);
        return success;
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

    void SevenZipComponent::Launch7z(const AZStd::string& commandLineArgs, const ArchiveResponseOutputCallback& respCallback, AZ::Uuid taskHandle, const AZStd::string& workingDir, bool captureOutput)
    {
        auto sevenZJob = [=]()
        {
            if (!taskHandle.IsNull())
            {
                AZStd::unique_lock<AZStd::mutex> lock(m_threadControlMutex);
                m_threadInfoMap[taskHandle].threads.insert(AZStd::this_thread::get_id());
                m_cv.notify_all();
            }

            ProcessLauncher::ProcessLaunchInfo info;
            info.m_commandlineParameters = m_7zPath + " " + commandLineArgs;
            info.m_showWindow = false;
            if (!workingDir.empty())
            {
                info.m_workingDirectory = workingDir;
            }
            AZStd::unique_ptr<ProcessWatcher> watcher(ProcessWatcher::LaunchProcess(info, ProcessCommunicationType::COMMUNICATOR_TYPE_STDINOUT));

            AZStd::string consoleOutput;
            AZ::u32 exitCode = static_cast<AZ::u32>(SevenZipExitCode::UserStoppedProcess);
            if (watcher)
            {
                // callback requires output captured from 7z 
                if (captureOutput)
                {
                    AZStd::string consoleBuffer;
                    while (watcher->IsProcessRunning())
                    {
                        if (!taskHandle.IsNull())
                        {
                            AZStd::unique_lock<AZStd::mutex> lock(m_threadControlMutex);
                            if (m_threadInfoMap[taskHandle].shouldStop)
                            {
                                watcher->TerminateProcess(static_cast<AZ::u32>(SevenZipExitCode::UserStoppedProcess));
                            }
                        }
                        watcher->WaitForProcessToExit(g_sleepDuration);
                        AZ::u32 outputSize = watcher->GetCommunicator()->PeekOutput();
                        if (outputSize)
                        {
                            consoleBuffer.resize(outputSize);
                            watcher->GetCommunicator()->ReadOutput(consoleBuffer.data(), outputSize);
                            consoleOutput += consoleBuffer;
                        }
                    }
                }
                else
                {
                    ConsoleEchoCommunicator echoCommunicator(watcher->GetCommunicator());
                    while (watcher->IsProcessRunning(&exitCode))
                    {
                        if (!taskHandle.IsNull())
                        {
                            AZStd::unique_lock<AZStd::mutex> lock(m_threadControlMutex);
                            if (m_threadInfoMap[taskHandle].shouldStop)
                            {
                                watcher->TerminateProcess(static_cast<AZ::u32>(SevenZipExitCode::UserStoppedProcess));
                            }
                        }
                        watcher->WaitForProcessToExit(g_sleepDuration);
                        echoCommunicator.Pump();
                    }
                }
            }

            if (taskHandle.IsNull())
            {
                respCallback(exitCode == static_cast<AZ::u32>(SevenZipExitCode::NoError), AZStd::move(consoleOutput));
            }
            else
            {
                AZ::TickBus::QueueFunction(respCallback, (exitCode == static_cast<AZ::u32>(SevenZipExitCode::NoError)), AZStd::move(consoleOutput));
            }

            if (!taskHandle.IsNull())
            {
                AZStd::unique_lock<AZStd::mutex> lock(m_threadControlMutex);
                ThreadInfo& tInfo = m_threadInfoMap[taskHandle];
                tInfo.threads.erase(AZStd::this_thread::get_id());
                m_cv.notify_all();
            }
        };
        if (!taskHandle.IsNull())
        {
            AZStd::thread processThread(sevenZJob);
            AZStd::unique_lock<AZStd::mutex> lock(m_threadControlMutex);
            ThreadInfo& info = m_threadInfoMap[taskHandle];
            m_cv.wait(lock, [&info, &processThread]() {
                return info.threads.find(processThread.get_id()) != info.threads.end();
            });
            processThread.detach();
        }
        else
        {
            sevenZJob();
        }
    }
} // namespace AzToolsFramework
