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

#pragma once

#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/string/string.h>

namespace LyToolLauncher
{
    class ToolLauncher
    {
    public:
        static int Execute();
    private:

        ToolLauncher(const char* executableToolName);

        AZ::Outcome<void, AZStd::string> Launch();

        struct LaunchArg
        {
            AZStd::string m_executablePath;
            AZStd::string m_appRootArg;
        };

        struct RootAndBinPath
        {
            AZStd::string m_rootPath;
            AZStd::string m_binSubPath;
        };

        struct EngineConfig
        {
            AZStd::string m_engineRoot;
            AZStd::string m_engineVersion;
        };

        /// Determine the engine executable path and argument for launching the tool
        AZ::Outcome<LaunchArg, AZStd::string> GetLaunchCommandAndArgument();

        /// Perform the Launch command
        AZ::Outcome<void, AZStd::string> LaunchTool(const LaunchArg& launchArg);

        /// Get the path of the currently running executable
        AZStd::string GetModuleFolderPath();

        /// Calculate the full path of the target tool's executable filename
        AZStd::string GetExecutableToolPath(const AZStd::string& engineRoot, const AZStd::string& binSubPath);

        /// Given a full folder path, this function will determine the subfolder and parant path.  
        AZ::Outcome<RootAndBinPath, AZStd::string> SplitRootPathAndSubfolder(const AZStd::string& fullFolderPath);

        /// Read an engine.json file and perform specific validations:
        AZ::Outcome<EngineConfig, AZStd::string> ReadAndValidateEngineRootFromConfig(const AZStd::string& rootPath);

        /// Read the engine.json configuration file from a path
        AZ::Outcome<EngineConfig, AZStd::string> ReadEngineConfig(const AZStd::string& rootPath);

        AZStd::string m_executableName;

    };

};

