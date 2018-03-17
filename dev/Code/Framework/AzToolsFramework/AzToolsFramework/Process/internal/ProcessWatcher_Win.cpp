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

#include "StdAfx.h"

#ifdef AZ_PLATFORM_WINDOWS // Only windows support currently

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/PlatformIncl.h>

#include <AzToolsFramework/Process/ProcessWatcher.h>
#include <AzToolsFramework/Process/ProcessCommunicator.h>
#include <AzToolsFramework/Process/internal/ProcessCommon_Win.h>

#include <iostream>

namespace AzToolsFramework
{
    ProcessData::ProcessData()
    {
        Init(false);
    }

    void ProcessData::Init(bool stdCommunication)
    {
        ZeroMemory(&startupInfo, sizeof(STARTUPINFO));
        startupInfo.cb = sizeof(STARTUPINFO);
        ZeroMemory(&processInformation, sizeof(PROCESS_INFORMATION));
        if (stdCommunication)
        {
            startupInfo.dwFlags |= STARTF_USESTDHANDLES;
            inheritHandles = TRUE;
        }
        else
        {
            inheritHandles = FALSE;
        }
    }

    bool ProcessLauncher::LaunchUnwatchedProcess(const ProcessLaunchInfo& processLaunchInfo)
    {
        ProcessData processData;
        processData.Init(false);
        return LaunchProcess(processLaunchInfo, processData);
    }

    bool ProcessLauncher::LaunchProcess(const ProcessLaunchInfo& processLaunchInfo, ProcessData& processData)
    {
        BOOL result = FALSE;

        // Windows API requires non-const char* command line string
        AZStd::wstring editableCommandLine;
        AZStd::wstring processExecutableString;
        AZStd::wstring workingDirectory;
        AZStd::to_wstring(editableCommandLine, processLaunchInfo.m_commandlineParameters);
        AZStd::to_wstring(processExecutableString, processLaunchInfo.m_processExecutableString);
        AZStd::to_wstring(workingDirectory, processLaunchInfo.m_workingDirectory);

        AZStd::string environmentVariableBlock;
        if (processLaunchInfo.m_environmentVariables)
        {
            for (const auto& environmentVariable : *processLaunchInfo.m_environmentVariables)
            {
                environmentVariableBlock += environmentVariable;
                environmentVariableBlock.append(1, '\0');
            }
            environmentVariableBlock.append(processLaunchInfo.m_environmentVariables->size() ? 1 : 2, '\0'); // Double terminated, only need one if we ended with a null terminated string already
        }

        // Show or hide window
        processData.startupInfo.dwFlags |= STARTF_USESHOWWINDOW;
        processData.startupInfo.wShowWindow = processLaunchInfo.m_showWindow ? SW_SHOW : SW_HIDE;

        DWORD createFlags = 0;
        switch (processLaunchInfo.m_processPriority)
        {
        case PROCESSPRIORITY_BELOWNORMAL:
            createFlags |= BELOW_NORMAL_PRIORITY_CLASS;
            break;
        case PROCESSPRIORITY_IDLE:
            createFlags |= IDLE_PRIORITY_CLASS;
            break;
        }

        // Create the child process.
        result = CreateProcessW(processExecutableString.size() ? processExecutableString.c_str() : NULL,
                editableCommandLine.size() ? editableCommandLine.data() : NULL, // command line
                NULL,  // process security attributes
                NULL,  // primary thread security attributes
                processData.inheritHandles,// handles might be inherited
                createFlags,     // creation flags
                environmentVariableBlock.size() ? environmentVariableBlock.data() : NULL, // environmentVariableBlock is a proper double null terminated block constructed above
                workingDirectory.empty() ? nullptr : workingDirectory.c_str(),  // use parent's current directory
                &processData.startupInfo, // STARTUPINFO pointer
                &processData.processInformation); // receives PROCESS_INFORMATION

        if (result != TRUE)
        {
            if (GetLastError() == ERROR_FILE_NOT_FOUND)
            {
                processLaunchInfo.m_launchResult = PLR_MissingFile;
            }
        }

        // Close inherited handles
        CloseHandle(processData.startupInfo.hStdInput);
        CloseHandle(processData.startupInfo.hStdOutput);
        CloseHandle(processData.startupInfo.hStdError);
        return result == TRUE;
    }


    ProcessWatcher* ProcessWatcher::LaunchProcess(const ProcessLauncher::ProcessLaunchInfo& processLaunchInfo, ProcessCommunicationType communicationType)
    {
        ProcessWatcher* pWatcher = aznew ProcessWatcher {};
        if (!pWatcher->SpawnProcess(processLaunchInfo, communicationType))
        {
            delete pWatcher;
            return nullptr;
        }
        return pWatcher;
    }


    ProcessWatcher::ProcessWatcher()
        : m_pCommunicator(nullptr)
    {
        m_pWatcherData = aznew ProcessData {};
    }

    ProcessWatcher::~ProcessWatcher()
    {
        if (IsProcessRunning())
        {
            TerminateProcess(0);
        }

        delete m_pCommunicator;
        CloseHandle(m_pWatcherData->processInformation.hProcess);
        CloseHandle(m_pWatcherData->processInformation.hThread);
        delete m_pWatcherData;
        m_pWatcherData = nullptr;
    }

    StdProcessCommunicator* ProcessWatcher::CreateStdCommunicator()
    {
        return aznew StdInOutProcessCommunicator();
    }

    StdProcessCommunicatorForChildProcess* ProcessWatcher::CreateStdCommunicatorForChildProcess()
    {
        return aznew StdInOutProcessCommunicatorForChildProcess();
    }

    void ProcessWatcher::InitProcessData(bool stdCommunication)
    {
        m_pWatcherData->Init(stdCommunication);
    }

    namespace
    {
        // Returns true if process exited, false if still running
        bool CheckExitCode(HANDLE hProcess, AZ::u32* outExitCode = nullptr)
        {
            // Check exit code
            DWORD exitCode;
            BOOL result;
            result = GetExitCodeProcess(hProcess, &exitCode);
            if (!result)
            {
                exitCode = 0;
                AZ_TracePrintf("ProcessWatcher", "GetExitCodeProcess failed (%d), assuming process either failed to launch or terminated unexpectedly\n", GetLastError());
            }

            if (exitCode != STILL_ACTIVE)
            {
                if (outExitCode)
                {
                    *outExitCode = static_cast<AZ::u32>(exitCode);
                }
                return true;
            }
            return false;
        }
    }

    bool ProcessWatcher::IsProcessRunning(AZ::u32* outExitCode)
    {
        AZ_Assert(m_pWatcherData, "No watcher data");
        if (!m_pWatcherData)
        {
            return false;
        }

        if (CheckExitCode(m_pWatcherData->processInformation.hProcess, outExitCode))
        {
            return false;
        }

        // Verify process is not signaled
        DWORD waitResult = WaitForSingleObject(m_pWatcherData->processInformation.hProcess, 0);

        // if wait timed out, process still running.
        return waitResult == WAIT_TIMEOUT;
    }

    bool ProcessWatcher::WaitForProcessToExit(AZ::u32 waitTimeInSeconds)
    {
        if (CheckExitCode(m_pWatcherData->processInformation.hProcess))
        {
            // Already exited
            return true;
        }

        // Verify process is not signaled
        DWORD waitResult = WaitForSingleObject(m_pWatcherData->processInformation.hProcess, waitTimeInSeconds * 1000);

        // if wait timed out, process still running.
        return waitResult != WAIT_TIMEOUT;
    }

    void ProcessWatcher::TerminateProcess(AZ::u32 exitCode)
    {
        AZ_Assert(m_pWatcherData, "No watcher data");
        if (!m_pWatcherData)
        {
            return;
        }

        if (!IsProcessRunning())
        {
            return;
        }

        ::TerminateProcess(m_pWatcherData->processInformation.hProcess, exitCode);
    }
} // namespace AzToolsFramework

#endif // AZ_PLATFORM_WINDOWS 