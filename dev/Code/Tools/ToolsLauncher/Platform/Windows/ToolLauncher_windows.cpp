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

#include "ToolLauncher.h"
#include <AzCore/PlatformDef.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/conversions.h>

#include <AzCore/IO/SystemFile.h>
#if defined(AZ_PLATFORM_WINDOWS)

#if defined(AZ_COMPILER_MSVC)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ShellAPI.h>
#else
#   pragma message ("Tool launcher on windows only supports MSVC compilers")
#endif

int APIENTRY WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    return LyToolLauncher::ToolLauncher::Execute();
}


namespace LyToolLauncher
{
    AZ::Outcome<void, AZStd::string> ToolLauncher::LaunchTool(const LaunchArg& launchArg)
    {
        AZStd::string fullCommandLine = AZStd::string::format("\"%s\" --app-root \"%s\"", launchArg.m_executablePath.c_str(), launchArg.m_appRootArg.c_str());
        char commandLineBuffer[AZ_MAX_PATH_LEN];
        azstrncpy(commandLineBuffer, fullCommandLine.c_str(), AZ_ARRAY_SIZE(commandLineBuffer));

        STARTUPINFOW si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        AZStd::wstring commandLineWide;
        AZStd::to_wstring(commandLineWide, fullCommandLine);

        if (!CreateProcessW(NULL, commandLineWide.data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))           // Pointer to PROCESS_INFORMATION structure
        {
            DWORD createProcessLastError = GetLastError();
            return AZ::Failure(AZStd::string::format("Error launching " TOOL_EXECUTABLE_NAME " from %s.  (Error code %08x)", launchArg.m_executablePath.c_str(), createProcessLastError));
        }

        // Close process and thread handles.
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return AZ::Success();
    }


    AZStd::string ToolLauncher::GetModuleFolderPath()
    {
        wchar_t currentFileName[AZ_MAX_PATH_LEN] = { 0 };
        wchar_t driveName[AZ_MAX_PATH_LEN] = { 0 };
        wchar_t directoryName[AZ_MAX_PATH_LEN] = { 0 };
        wchar_t fileName[AZ_MAX_PATH_LEN] = { 0 };
        wchar_t fileExtension[AZ_MAX_PATH_LEN] = { 0 };

        // Get the module file full path for the current exectable and extract the current executable's folder
        ::GetModuleFileNameW(nullptr, currentFileName, AZ_MAX_PATH_LEN);
        _wsplitpath_s(currentFileName, driveName, AZ_MAX_PATH_LEN, directoryName, AZ_MAX_PATH_LEN, fileName, AZ_MAX_PATH_LEN, fileExtension, AZ_MAX_PATH_LEN);
        _wmakepath_s(currentFileName, driveName, directoryName, nullptr, nullptr);

        const unsigned int codepage = CP_UTF8;
        int utf8bufSize = ::WideCharToMultiByte(codepage, 0, currentFileName, -1, NULL, 0, 0, 0);
        char* utf8buf = (char*) alloca(utf8bufSize);
        AZ_Assert(utf8bufSize > 1, "Unable to allocate memory for utf8 conversion with alloca");

        WideCharToMultiByte(codepage, 0, currentFileName, -1, utf8buf, utf8bufSize, 0, 0);
        return AZStd::string(utf8buf, utf8buf + utf8bufSize - 1);
    }

    AZStd::string ToolLauncher::GetExecutableToolPath(const AZStd::string& engineRoot, const AZStd::string& binSubPath)
    {
        AZStd::string executablePath = AZStd::string::format("%s%s" TOOL_EXECUTABLE_NAME ".exe", engineRoot.c_str(), binSubPath.c_str(), m_executableName.c_str());
        return executablePath;
    }

} // namespace LyToolLauncher
#else
#error Not supported for current platform
#endif
