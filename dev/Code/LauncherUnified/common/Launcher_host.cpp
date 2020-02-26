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
#include <Launcher_precompiled.h>
#include <Launcher.h>
#include "Launcher_host.h"

#include <AzGameFramework/Application/GameApplication.h>
#include <IGameFramework.h>
#include <IGameStartup.h>
#include <CryPath.h>
#include <ParseEngineConfig.h>


namespace LumberyardLauncher
{
    //-------------------------------------------------------------------------------------
    // Resolve the app resource path for this host platform. 
    // Host platform targets have the ability to launch directly from the build target path (BinXXX)
    // as well as a layout folder. 
    //-------------------------------------------------------------------------------------
    const char* GetAppResourcePathForHostPlatforms()
    {
        static char descriptorPath[AZ_MAX_PATH_LEN] = { 0 };

        LumberyardLauncher::CryAllocatorsRAII cryAllocatorsRAII;

        // Use the engine config to get the information to calculate the descriptor path
        CEngineConfig       engineConfig;
        SSystemInitParams   systemInitParams;
        engineConfig.CopyToStartupParams(systemInitParams);

        char configPathOriginal[AZ_MAX_PATH_LEN] = { 0 };
        AzGameFramework::GameApplication::GetGameDescriptorPath(configPathOriginal, engineConfig.m_gameFolder);

#if !AZ_TRAIT_LAUNCHER_LOWER_CASE_PATHS
        // If we are not forcing a lower-case of paths, then take into consideration the original path AND a lower-cased
        // version of it
        char configPathLower[AZ_MAX_PATH_LEN] = { 0 };
        AzGameFramework::GameApplication::GetGameDescriptorPath(configPathLower, engineConfig.m_gameFolder);
        AZStd::to_lower(configPathLower, configPathLower + strlen(configPathLower));

        const char* configPathCandidates[] = {
            configPathOriginal,
            configPathLower
        };
#else
        const char* configPathCandidates[] = {
            configPathOriginal
        };
#endif // AZ_TRAIT_LAUNCHER_LOWER_CASE_PATHS

        char workingConfigPath[AZ_MAX_PATH_LEN] = { 0 };
        char configFullPath[AZ_MAX_PATH_LEN] = { 0 };

        // The following represents the order of the base directory in which we will look for the config paath (configPath)
        const char* appResourceCheckPaths[] = {
            ".",                            // First check: The current working directory (cwd)
            systemInitParams.rootPathCache, // Second check: The calculated Cache/<game folder>/<platform>/ folder
            systemInitParams.rootPath,      // Third check: The calculated root path
        };
        // Default to a blank string to indicate whether or not an application descriptor was found
        descriptorPath[0] = '\0';

        // Iterate through the possible root paths above to attempt to locate the game descriptor
        for (const char* configPath: configPathCandidates)
        {
            for (const char* checkPath: appResourceCheckPaths)
            {
                if (AZ::IO::SystemFile::Exists(checkPath))
                {
                    // If the resolved path to the game descriptor path exist, use it as the result
                    azsnprintf(configFullPath, AZ_ARRAY_SIZE(configFullPath), "%s/%s", checkPath, configPath);
                    if (AZ::IO::SystemFile::Exists(configFullPath))
                    {
                        azstrncpy(descriptorPath, AZ_ARRAY_SIZE(descriptorPath), checkPath, strlen(checkPath)+1);
                        return descriptorPath;
                    }
                }
            }
        }
        return descriptorPath;
    }
}
