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
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/FileFunc/FileFunc.h>

#if !defined(TOOL_EXECUTABLE_NAME)
#error "TOOL_EXECUTABLE_NAME not defined for this tools launcher"
#endif // !defined(TOOL_EXECUTABLE_NAME)

static const char* s_traceWindow = "tool launcher";

namespace LyToolLauncher
{

    int ToolLauncher::Execute()
    {
        int result = 0;

        AZ::AllocatorInstance<AZ::SystemAllocator>::Create();

        LyToolLauncher::ToolLauncher toolLauncher(TOOL_EXECUTABLE_NAME);
        {
            AZ_TracePrintf(s_traceWindow, "Launching " TOOL_EXECUTABLE_NAME "...");

            auto getLaunchCommandResult = toolLauncher.Launch();
            if (!getLaunchCommandResult.IsSuccess())
            {
                AZ_TracePrintf(s_traceWindow, "Error: %s\n", getLaunchCommandResult.GetError().c_str());
                result = 1;
            }
        }

        AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
        return result;
    }

    static const char* s_engineConfigFileName = "engine.json";
    static const char* s_engineConfigExternalEnginePathKey = "ExternalEnginePath";
    static const char* s_engineConfigEngineVersionKey = "LumberyardVersion";

    ToolLauncher::ToolLauncher(const char* executableToolName)
        : m_executableName(executableToolName)
    {
    }

    AZ::Outcome<void, AZStd::string> ToolLauncher::Launch()
    {
        auto getLaunchCommandResult = GetLaunchCommandAndArgument();
        if (!getLaunchCommandResult.IsSuccess())
        {
            return AZ::Failure(getLaunchCommandResult.TakeError());
        }

        auto launchToolResult = LaunchTool(getLaunchCommandResult.GetValue());
        if (!launchToolResult.IsSuccess())
        {
            return AZ::Failure(launchToolResult.TakeError());
        }

        return AZ::Success();
    }

    /// Determine the engine executable path and argument for launching the tool
    AZ::Outcome<ToolLauncher::LaunchArg, AZStd::string> ToolLauncher::GetLaunchCommandAndArgument()
    {
        // Calcuate the path based on the location of the tool launcher
        AZStd::string moduleFullFolderPath = GetModuleFolderPath();

        // The launcher must exist within a root folder (i.e. its current folder must not sit at the root (drive) folder, but in a subfolder)
        auto rootAndBinPathResult = SplitRootPathAndSubfolder(moduleFullFolderPath);
        if (!rootAndBinPathResult.IsSuccess())
        {
            return AZ::Failure(rootAndBinPathResult.TakeError());
        }
        auto rootAndBinFolder = rootAndBinPathResult.TakeValue();

        // Sanity check: The app root path must have an engine.json and bootstrap.cfg
        static const char* sanityCheckFiles[] = { "bootstrap.cfg", "engine.json" };
        for (const char* sanityCheckFile : sanityCheckFiles)
        {
            if (!AZ::IO::SystemFile::Exists(AZStd::string::format("%s%s", rootAndBinFolder.m_rootPath.c_str(), sanityCheckFile).c_str()))
            {
                return AZ::Failure(AZStd::string::format("Invalid project root path (%s).  Missing '%s'.", rootAndBinFolder.m_rootPath.c_str(), sanityCheckFile));
            }
        }

        // Read the engine root from engine.json
        auto readEngineRootResult = ReadAndValidateEngineRootFromConfig(rootAndBinFolder.m_rootPath);
        if (!readEngineRootResult.IsSuccess())
        {
            return AZ::Failure(readEngineRootResult.TakeError());
        }
        auto engineRoot = readEngineRootResult.TakeValue();

        // Construct the full path to the tool
        AZStd::string executablePath = GetExecutableToolPath(engineRoot.m_engineRoot, rootAndBinFolder.m_binSubPath);

        if (!AZ::IO::SystemFile::Exists(executablePath.c_str()))
        {
            return AZ::Failure(AZStd::string::format("Unable to launch %s.  Make sure the path is correct or the target has been built for the current configuration.", executablePath.c_str()));
        }

        LaunchArg arg;

        arg.m_executablePath = executablePath;
        arg.m_appRootArg = rootAndBinFolder.m_rootPath;

        // The app root argument must not have a trailing path separator
        auto appRootLen = arg.m_appRootArg.length();
        if (appRootLen > 0)
        {
            if ((arg.m_appRootArg.at(appRootLen - 1) == AZ_CORRECT_FILESYSTEM_SEPARATOR) ||
                (arg.m_appRootArg.at(appRootLen - 1) == AZ_WRONG_FILESYSTEM_SEPARATOR))
            {
                arg.m_appRootArg = arg.m_appRootArg.substr(0, appRootLen - 1);
            }
        }

        return AZ::Success(arg);
    }


    /// Given a full folder path, this function will determine the subfolder and parent path.  Useful for determining
    /// App Root and relative bin folder, i.e.
    /// 
    /// input:  
    ///     f:\\workspace\\mygame\\Bin64vc140\\
    /// output: 
    ///     root = f:\\workspace\\mygame\\
    ///     subfolder = Bin64vc140\\
    ///
    AZ::Outcome<ToolLauncher::RootAndBinPath, AZStd::string> ToolLauncher::SplitRootPathAndSubfolder(const AZStd::string& fullFolderPath)
    {
        AZStd::string trimmedFolderPath = fullFolderPath;
        AzFramework::StringFunc::TrimWhiteSpace(trimmedFolderPath, true, true);
        auto path_len = fullFolderPath.length();
        auto last_path_sep_index = fullFolderPath.find_last_of(AZ_CORRECT_FILESYSTEM_SEPARATOR);
        AZStd::string::size_type binFolderIndex = 0;
        if (last_path_sep_index == (path_len - 1))
        {
            binFolderIndex = fullFolderPath.find_last_of(AZ_CORRECT_FILESYSTEM_SEPARATOR, (last_path_sep_index - 1));
            if (binFolderIndex == AZStd::string::npos)
            {
                return AZ::Failure(AZStd::string::format("Invalid location for tool launcher: %s", fullFolderPath.c_str()));
            }
        }
        else
        {
            trimmedFolderPath.append(AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING);
            binFolderIndex = last_path_sep_index;
        }

        RootAndBinPath result;
        result.m_binSubPath = trimmedFolderPath.substr(binFolderIndex + 1);
        result.m_rootPath = trimmedFolderPath.substr(0, binFolderIndex + 1);

        return AZ::Success(result);
    }

    ///
    /// Read an engine.json file and perform specific validations:
    ///
    ///   1. Its a valid engine.json file
    ///   2. It points to an engine that is external to the current folder 
    ///   3. The folder it points to is aan engine root folder
    ///
    AZ::Outcome<ToolLauncher::EngineConfig, AZStd::string> ToolLauncher::ReadAndValidateEngineRootFromConfig(const AZStd::string& rootPath)
    {
        // Determine the engine root from the engine.json
        auto readEngineConfigResult = ReadEngineConfig(rootPath);
        if (!readEngineConfigResult.IsSuccess())
        {
            return readEngineConfigResult;
        }
        auto engineConfig = readEngineConfigResult.TakeValue();

        // Make sure the engine root that was read is valid
        AZStd::string engineRootConfig = AZStd::string::format("%s" AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING "engine.json", engineConfig.m_engineRoot.c_str());
        if (!AZ::IO::SystemFile::Exists(engineRootConfig.c_str()))
        {
            return AZ::Failure(AZStd::string::format("Invalid target engine path found in %s/engine.json: %s", rootPath.c_str(), engineRootConfig.c_str()));
        }

        // Read the engine.json in the engine root
        auto readEngineRootConfigResult = ReadEngineConfig(engineConfig.m_engineRoot);
        if (!readEngineRootConfigResult.IsSuccess())
        {
            return AZ::Failure(AZStd::string::format("Invalid target engine config file '%s': %s", engineRootConfig.c_str(), readEngineRootConfigResult.GetError().c_str()));
        }
        auto engineRootconfig = readEngineRootConfigResult.TakeValue();

        // The engine root must not have an engine config that points to another engine
        if ((engineRootconfig.m_engineRoot.length() > 0) && (engineRootconfig.m_engineRoot.compare(engineConfig.m_engineRoot) != 0))
        {
            return AZ::Failure(AZStd::string::format("Invalid target engine config file '%s': Configuration points to another engine root path", engineRootConfig.c_str()));
        }

        // The engine root config version must match the root path's configured version (version check)
        if (engineConfig.m_engineVersion.compare(engineRootconfig.m_engineVersion) != 0)
        {
            return AZ::Failure(AZStd::string::format("Invalid target engine config file '%s': Version mismatch", engineRootConfig.c_str()));
        }

        return AZ::Success(engineConfig);
    }


    ///
    /// Read the engine.json configuration file from a path
    ///
    AZ::Outcome<ToolLauncher::EngineConfig, AZStd::string> ToolLauncher::ReadEngineConfig(const AZStd::string& rootPath)
    {
        // Calculate the engine root by reading the engine.json file
        AZStd::string  engineJsonPath = AZStd::string::format("%s%s", rootPath.c_str(), s_engineConfigFileName);
        AZ::IO::LocalFileIO localFileIO;
        auto readJsonResult = AzFramework::FileFunc::ReadJsonFile(engineJsonPath, &localFileIO);
        if (!readJsonResult.IsSuccess())
        {
            return AZ::Failure(readJsonResult.TakeError());
        }

        rapidjson::Document engineJson = readJsonResult.TakeValue();
        EngineConfig config;

        auto externalEngineVersion = engineJson.FindMember(s_engineConfigEngineVersionKey);
        if (externalEngineVersion == engineJson.MemberEnd())
        {
            return AZ::Failure(AZStd::string::format("Invalid engine config: %sengine.json.  Missing '%s' value.", rootPath.c_str(), s_engineConfigEngineVersionKey));
        }
        config.m_engineVersion = AZStd::string(externalEngineVersion->value.GetString());

        auto externalEnginePath = engineJson.FindMember(s_engineConfigExternalEnginePathKey);
        if (externalEnginePath != engineJson.MemberEnd())
        {
            AZStd::string engineRoot = externalEnginePath->value.GetString();
            auto engineRootLen = engineRoot.length();
            if (engineRootLen > 0)
            {
                if (AzFramework::StringFunc::Path::IsRelative(engineRoot.c_str()))
                {
                    // the default trailing args for join will automatically normalize the path 
                    // to remove any upward relative directories
                    AzFramework::StringFunc::Path::Join(rootPath.c_str(), engineRoot.c_str(), config.m_engineRoot);
                }
                else
                {
                    config.m_engineRoot = AZStd::move(engineRoot);
                }

                // Make sure the engine root path has a trailing path separator
                if ((config.m_engineRoot.at(engineRootLen - 1) != AZ_CORRECT_FILESYSTEM_SEPARATOR) &&
                    (config.m_engineRoot.at(engineRootLen - 1) != AZ_WRONG_FILESYSTEM_SEPARATOR))
                {
                    config.m_engineRoot.append(AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING);
                }
            }
            else
            {
                return AZ::Failure(AZStd::string::format("Invalid engine config: %sengine.json.  Empty '%s' value.", rootPath.c_str(), s_engineConfigExternalEnginePathKey));
            }
        }

        return AZ::Success(config);
    }


};


