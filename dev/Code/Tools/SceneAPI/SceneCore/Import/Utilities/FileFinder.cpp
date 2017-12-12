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

#include <AzCore/IO/SystemFile.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <SceneAPI/SceneCore/Import/Utilities/FileFinder.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Import
        {
            namespace Utilities
            {
                const char* FileFinder::s_manifestExtension = "scenesettings";
                const char* FileFinder::s_fbxSceneExtension = "fbx";

                bool FileFinder::FindMatchingFileName(std::string& outFileName, const std::string& inputFileName, const std::string& extension)
                {
                    AZStd::string anotherFileName(inputFileName.c_str());
                    AzFramework::StringFunc::Path::ReplaceExtension(anotherFileName, extension.c_str());

                    if (AZ::IO::SystemFile::Exists(anotherFileName.c_str()))
                    {
                        outFileName = anotherFileName.c_str();
                        return true;
                    }
                    return false;
                }

                void FileFinder::FindMatchingFileNames(std::vector<std::string>& outFileNames, const std::string& inputFileName,
                    const std::vector<std::string>& extensions)
                {
                    for (uint32_t i = 0; i < extensions.size(); ++i)
                    {
                        std::string outFileName;
                        if (FindMatchingFileName(outFileName, inputFileName, extensions[i]))
                        {
                            outFileNames.push_back(std::move(outFileName));
                        }
                    }
                }

                const char* FileFinder::GetManifestExtension()
                {
                    return s_manifestExtension;
                }

                bool FileFinder::FindMatchingManifestFile(std::string& manifestFileName, const std::string& inputFileName)
                {
                    return FindMatchingFileName(manifestFileName, inputFileName, std::string(s_manifestExtension));
                }

                bool FileFinder::IsValidSceneFile(const std::string& sceneFileName, const std::string& sceneExtension)
                {
                    return AzFramework::StringFunc::Path::IsExtension(sceneFileName.c_str(), sceneExtension.c_str());
                }

                bool FileFinder::IsValidFbxSceneFile(const std::string& sceneFileName)
                {
                    return IsValidSceneFile(sceneFileName, std::string(s_fbxSceneExtension));
                }
            }
        }
    }
}