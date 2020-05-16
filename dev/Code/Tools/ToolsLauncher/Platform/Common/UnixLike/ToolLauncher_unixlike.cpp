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

#include <AzCore/IO/SystemFile.h> // for AZ_MAX_PATH_LEN

#include <libgen.h>
#include <unistd.h>


namespace LyToolLauncher
{
    extern bool GetExePath(char* buffer, unsigned int bufferSize);

    AZStd::string ToolLauncher::GetModuleFolderPath()
    {
        char appPath[AZ_MAX_PATH_LEN] = { 0 };
        unsigned int bufSize = AZ_MAX_PATH_LEN;

        if (!GetExePath(appPath, bufSize))
        {
            return AZStd::string();
        }

        // it's possible GetExeFullPath will return a path with a '.', to represent a subfolder
        // in between, i.e.
        //
        // \myproject\Bin<host>\.\Tool
        //
        // So we will calculate the dirname, which in the above case will produce
        //
        // \myproject\Bin<host>\.
        //
        // and rely on the dir name length to check where we want to null terminate the full path
        // to get the directory name
        const char* dirName = dirname(appPath);
        auto dirNameLen = strlen(dirName);
        if ((dirNameLen > 0) && (appPath[dirNameLen - 1] == '.'))
        {
            appPath[dirNameLen - 1] = '\0';
        }
        else
        {
            appPath[dirNameLen] = '\0';
        }

        return appPath;
    }

    AZStd::string ToolLauncher::GetExecutableToolPath(const AZStd::string& engineRoot, const AZStd::string& binSubPath)
    {
        return AZStd::string::format("%s%s" TOOL_EXECUTABLE_NAME, engineRoot.c_str(), binSubPath.c_str());
    }

    AZ::Outcome<void, AZStd::string> ToolLauncher::LaunchTool(const LaunchArg& launchArg)
    {
        AZStd::string fullCommandLine;

        AZStd::string localLibSearchPath = GetModuleFolderPath();
        for (auto envVar : {"LD_LIBRARY_PATH", "DYLD_LIBRARY_PATH", "DYLD_FALLBACK_LIBRARY_PATH"})
        {
            AZ_TracePrintf("tool launcher", "Adding to command line: %s=%s\n", envVar, localLibSearchPath.c_str());
            fullCommandLine += AZStd::string::format("%s=%s:$%s ", envVar, localLibSearchPath.c_str(), envVar);
        }

        fullCommandLine += AZStd::string::format("\"%s\" --app-root \"%s\"",
            launchArg.m_executablePath.c_str(),
            launchArg.m_appRootArg.c_str());

        AZ_TracePrintf("tool launcher", "Running command\n%s\n", fullCommandLine.c_str());
        system(fullCommandLine.c_str());

        return AZ::Success();
    }
} // namespace LyToolLauncher


int main(int, char**)
{
    return LyToolLauncher::ToolLauncher::Execute();
}
