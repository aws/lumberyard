#pragma once

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

#include <string>
#include <vector>
#include <SceneAPI/SceneCore/SceneCoreConfiguration.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Import
        {
            namespace Utilities
            {
                // FileFilder provides a few utilities for seeking files that meet specific conditions.
                class FileFinder
                {
                public:
                    SCENE_CORE_API static bool FindMatchingFileName(std::string& outFileName, const std::string& inputFileName, const std::string& extension);
                    SCENE_CORE_API static void FindMatchingFileNames(std::vector<std::string>& outFileNames, const std::string& inputFileName, const std::vector<std::string>& extensions);

                    SCENE_CORE_API static const char* GetManifestExtension();
                    SCENE_CORE_API static bool FindMatchingManifestFile(std::string& manifestFileName, const std::string& inputFileName);
                    SCENE_CORE_API static bool IsValidSceneFile(const std::string& sceneFileName, const std::string& sceneExtension);
                    SCENE_CORE_API static bool IsValidFbxSceneFile(const std::string& sceneFileName);

                private:
                    static const char* s_manifestExtension;
                    static const char* s_fbxSceneExtension;
                };
            }
        }
    }
}