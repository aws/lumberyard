
#include "CloudGemDefectReporter_precompiled.h"

#include <RunExternalCmd_windows.h>

#include <windows.h>

namespace CloudGemDefectReporter
{
    bool RunExternalCmd::Run(const AZStd::string& cmdName, const AZStd::string& parameters, const char* outputFilePath, int timeoutMilliSeconds)
    {
        char sysPath[MAX_PATH];
        GetSystemDirectory(sysPath, sizeof(sysPath));

        AZStd::string cmdPath = AZStd::string::format("%s\\%s", sysPath, "cmd.exe");

        STARTUPINFOA si = { sizeof(si) };
        PROCESS_INFORMATION pi;

        AZStd::string arg;
        if (outputFilePath)
        {
            arg = AZStd::string::format("/C %s %s > %s", cmdName.c_str(), parameters.c_str(), outputFilePath);
        }
        else
        {
            arg = AZStd::string::format("/C %s %s", cmdName.c_str(), parameters.c_str());
        }        

        if (CreateProcess(cmdPath.c_str(), (LPSTR)arg.c_str(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
        {
            DWORD result = WaitForSingleObject(pi.hProcess, timeoutMilliSeconds);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);

            if (result != WAIT_OBJECT_0)
            {
                return false;
            }
        }
        else
        {
            return false;
        }

        return true;
    }
}