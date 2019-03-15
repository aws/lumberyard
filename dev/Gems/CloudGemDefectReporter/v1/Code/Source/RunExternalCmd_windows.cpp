/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
*
*/

#include "CloudGemDefectReporter_precompiled.h"

#include <RunExternalCmd_windows.h>

#include <windows.h>

namespace CloudGemDefectReporter
{
    bool RunExternalCmd::Run(const AZStd::string& cmdName, const AZStd::string& parameters, const char* outputFilePath, int timeoutMilliSeconds)
    {
        // Generate the cmd path and arguments
        char sysPath[MAX_PATH];
        GetSystemDirectory(sysPath, sizeof(sysPath));

        AZStd::string cmdPath = AZStd::string::format("%s\\%s", sysPath, "cmd.exe");

        AZStd::string arg;
        if (outputFilePath)
        {
            arg = AZStd::string::format("/C %s %s > %s", cmdName.c_str(), parameters.c_str(), outputFilePath);
        }
        else
        {
            arg = AZStd::string::format("/C %s %s", cmdName.c_str(), parameters.c_str());
        }

        // WaitForSingleObject can usually be used to to wait for the process to terminate. However, when calling the dxdiag command on Windows 7, 
        // we get a success exit code while the child process is still running. The solution is to use an I/O completion port and a job object to
        // monitor the process tree and wait for all processes in the created job to finish. 

        // Create a job object and an I/O completion port to monitor the process tree 
        HANDLE jobObject(CreateJobObject(nullptr, nullptr));
        if (!jobObject)
        {
            AZ_Warning("CloudCanvas", false, "Failed to create job object");
            return false;
        }

        HANDLE IOPort(CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1));
        if (!IOPort)
        {
            AZ_Warning("CloudCanvas", false, "Failed to create I/O completion port");
            return false;
        }

        JOBOBJECT_ASSOCIATE_COMPLETION_PORT completionPort;
        completionPort.CompletionKey = jobObject;
        completionPort.CompletionPort = IOPort;
        if (!SetInformationJobObject(jobObject, JobObjectAssociateCompletionPortInformation, &completionPort, sizeof(completionPort)))
        {
            AZ_Warning("CloudCanvas", false, "Failed to set information job object");
            return false;
        }

        STARTUPINFOA si = { sizeof(si) };
        PROCESS_INFORMATION pi;

        // Launch the process into the job
        // Need to create the process suspended so that we can put it into the job before it exits
        if (!CreateProcess(cmdPath.c_str(), (LPSTR)arg.c_str(), NULL, NULL, FALSE, CREATE_NO_WINDOW | CREATE_SUSPENDED, NULL, NULL, &si, &pi))
        {
            AZ_Warning("CloudCanvas", false, "Failed to create process");
            return false;
        }

        if (!AssignProcessToJobObject(jobObject, pi.hProcess))
        {
            AZ_Warning("CloudCanvas", false, "Failed to assign process to job object");
            return false;
        }

        ResumeThread(pi.hThread);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);

        DWORD CompletionCode;
        ULONG_PTR CompletionKey;
        LPOVERLAPPED Overlapped;

        // Wait for all processes in the created job to finish or time out
        while (GetQueuedCompletionStatus(IOPort, &CompletionCode, &CompletionKey, &Overlapped, timeoutMilliSeconds)
            && ((HANDLE)CompletionKey != jobObject || CompletionCode != JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO))
        {
            //AZ_TracePrintf("CloudCanvas", "Wait for all processes in the created job to finish or time out");
        }

        return true;
    }
}