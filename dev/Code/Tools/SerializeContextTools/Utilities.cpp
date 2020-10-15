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

#include <AzCore/Serialization/SerializeContext.h> // Needs to be on top due to missing include in AssetSerializer.h
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Module/Module.h>
#include <AzCore/Module/ModuleManagerBus.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/std/containers/queue.h>
#include <AzCore/std/functional.h>
#include <AzFramework/CommandLine/CommandLine.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <Application.h>
#include <Utilities.h>

namespace AZ
{
    namespace SerializeContextTools
    {
        AZStd::vector<AZStd::string_view> Utilities::SplitString(AZStd::string_view input, char delimiter)
        {
            AZStd::vector<AZStd::string_view> result;

            size_t left = 0;
            while (left < input.length())
            {
                size_t right = input.find_first_of(delimiter, left);

                size_t end = right != AZStd::string_view::npos ? right : input.length();
                if (left != end)
                {
                    result.push_back(input.substr(left, end - left));
                }
                left = end + 1;
            }

            return result;
        }

        AZStd::string Utilities::ReadOutputTargetFromCommandLine(Application& application, const char* defaultFileOrFolder)
        {
            AZStd::string output;
            if (application.GetCommandLine()->HasSwitch("output"))
            {
                output = application.GetCommandLine()->GetSwitchValue("output", 0);
                if (AzFramework::StringFunc::Path::IsRelative(output.c_str()))
                {
                    AzFramework::StringFunc::Path::Join(application.GetGameRoot(), output.c_str(), output);
                }
            }
            else
            {
                AzFramework::StringFunc::Path::Join(application.GetGameRoot(), defaultFileOrFolder, output);
            }
            return output;
        }

        AZStd::vector<AZStd::string> Utilities::ReadFileListFromCommandLine(Application& application, AZStd::string_view switchName)
        {
            AZStd::vector<AZStd::string> result;

            const AzFramework::CommandLine* commandLine = application.GetCommandLine();
            if (!commandLine)
            {
                AZ_Error("SerializeContextTools", false, "Command line not available.");
                return result;
            }

            if (!commandLine->HasSwitch(switchName))
            {
                AZ_Error("SerializeContextTools", false, "Missing command line argument '-%*s' which should contain the requested files.", 
                    aznumeric_cast<int>(switchName.size()), switchName.data());
                return result;
            }

            const AZStd::string& files = commandLine->GetSwitchValue(switchName, 0);
            AZStd::vector<AZStd::string_view> fileList = Utilities::SplitString(files, ';');
            return Utilities::ExpandFileList(application.GetGameRoot(), fileList);
        }

        AZStd::vector<AZStd::string> Utilities::ExpandFileList(const char* root, const AZStd::vector<AZStd::string_view>& fileList)
        {
            AZStd::vector<AZStd::string> result;
            result.reserve(fileList.size());

            for (const AZStd::string_view& file : fileList)
            {
                if (HasWildCard(file))
                {
                    AZStd::string filterPath = file;

                    AZStd::string folderPath;
                    if (!AzFramework::StringFunc::Path::GetFolderPath(filterPath.c_str(), folderPath))
                    {
                        folderPath = root;
                    }
                    else if (AzFramework::StringFunc::Path::IsRelative(folderPath.c_str()))
                    {
                        if (!AzFramework::StringFunc::Path::Join(root, folderPath.c_str(), folderPath))
                        {
                            AZ_Error("SerializeContextTools", false, "Unable to construct full path for '%s'.", filterPath.c_str());
                            continue;
                        }
                    }
                    AZStd::string fileFilter;
                    if (!AzFramework::StringFunc::Path::GetFullFileName(filterPath.c_str(), fileFilter))
                    {
                        AZ_Error("SerializeContextTools", false, "Unable to get folder path for '%s'.", filterPath.c_str());
                        continue;
                    }

                    AZStd::queue<AZStd::string> pendingFolders;
                    pendingFolders.push(folderPath);
                    while (!pendingFolders.empty())
                    {
                        AZStd::string& folder = pendingFolders.front();
                        if (!folder.ends_with('\\') && !folder.ends_with('/'))
                        {
                            folder += AZ_CORRECT_FILESYSTEM_SEPARATOR;
                        }
                        folder += '*';
                        
                        auto callback = [&pendingFolders, &folder, &fileFilter, &result](const char* item, bool isFile) -> bool
                        {
                            if ((azstricmp(".", item) == 0) || (azstricmp("..", item) == 0))
                            {
                                return true;
                            }

                            AZStd::string fullPath;
                            if (!AzFramework::StringFunc::Path::Join(folder.c_str(), item, fullPath))
                            {
                                return false;
                            }

                            if (isFile)
                            {
                                if (AZ::IO::NameMatchesFilter(item, fileFilter.c_str()))
                                {
                                    result.push_back(AZStd::move(fullPath));
                                }
                            }
                            else
                            {
                                pendingFolders.push(AZStd::move(fullPath));
                            }
                            return true;
                        };
                        AZ::IO::SystemFile::FindFiles(folder.c_str(), callback);
                        pendingFolders.pop();
                    }
                }
                else
                {
                    AZStd::string filePath = file;
                    if (AzFramework::StringFunc::Path::IsRelative(filePath.c_str()))
                    {
                        if (!AzFramework::StringFunc::Path::Join(root, filePath.c_str(), filePath))
                        {
                            AZ_Error("SerializeContextTools", false, "Unable to construct full path for '%s'.", filePath.c_str());
                            continue;
                        }
                    }
                    result.push_back(AZStd::move(filePath));
                }
            }

            return result;
        }

        bool Utilities::HasWildCard(AZStd::string_view string)
        {
            // Wild cars vary between platforms, but these are the most common ones.
            return string.find_first_of("*?[]!@#", 0) != AZStd::string_view::npos;
        }

        void Utilities::SanitizeFilePath(AZStd::string& filePath)
        {
            auto invalidCharacters = [](char letter)
            {
                return
                    letter == ':' || letter == '"' || letter == '\'' ||
                    letter == '{' || letter == '}' || 
                    letter == '<' || letter == '>';
            };
            AZStd::replace_if(filePath.begin(), filePath.end(), invalidCharacters, '_');
        }

        bool Utilities::IsSerializationPrimitive(const AZ::Uuid& classId)
        {
            JsonRegistrationContext* registrationContext;
            AZ::ComponentApplicationBus::BroadcastResult(registrationContext, &AZ::ComponentApplicationBus::Events::GetJsonRegistrationContext);
            if (!registrationContext)
            {
                AZ_Error("SerializeContextTools", false, "Failed to retrieve json registration context.");
                return false;
            }
            return registrationContext->GetSerializerForType(classId) != nullptr;
        }

        AZStd::vector<AZ::Uuid> Utilities::GetSystemComponents(const Application& application)
        {
            AZStd::vector<AZ::Uuid> result = application.GetRequiredSystemComponents();
            auto getModuleSystemComponentsCB = [&result](const ModuleData& moduleData) -> bool
            {
                if (AZ::Module* module = moduleData.GetModule())
                {
                    AZ::ComponentTypeList moduleRequiredComponents = module->GetRequiredSystemComponents();
                    result.reserve(result.size() + moduleRequiredComponents.size());
                    result.insert(result.end(), moduleRequiredComponents.begin(), moduleRequiredComponents.end());
                }
                return true;
            };
            ModuleManagerRequestBus::Broadcast(&ModuleManagerRequests::EnumerateModules, getModuleSystemComponentsCB);

            return result;
        }

        bool Utilities::InspectSerializedFile(const AZStd::string& filePath, SerializeContext* sc, const ObjectStream::ClassReadyCB& classCallback)
        {
            if (!AZ::IO::SystemFile::Exists(filePath.c_str()))
            {
                AZ_Error("Verify", false, "Unable to open file '%s' as it doesn't exist.", filePath.c_str());
                return false;
            }

            u64 fileLength = AZ::IO::SystemFile::Length(filePath.c_str());
            if (fileLength == 0)
            {
                AZ_Error("Verify", false, "File '%s' doesn't have content.", filePath.c_str());
                return false;
            }

            AZStd::vector<u8> data;
            data.resize_no_construct(fileLength);
            u64 bytesRead = AZ::IO::SystemFile::Read(filePath.c_str(), data.data());
            if (bytesRead != fileLength)
            {
                AZ_Error("Verify", false, "Unable to read file '%s'.", filePath.c_str());
                return false;
            }

            AZ::IO::MemoryStream stream(data.data(), fileLength);
            
            auto assetFilter = [](const AZ::Data::Asset<AZ::Data::AssetData>& /*asset*/)
            {
                // Never load dependencies. That's another file that would need to be processed
                // separately from this one.
                return false;
            };
            ObjectStream::FilterDescriptor filter;
            filter.m_flags = ObjectStream::FILTERFLAG_IGNORE_UNKNOWN_CLASSES;
            filter.m_assetCB = assetFilter;
            if (!ObjectStream::LoadBlocking(&stream, *sc, classCallback, filter))
            {
                AZ_Printf("Verify", "Failed to deserialize '%s'\n", filePath.c_str());
                return false;
            }
            return true;
        }
    } // namespace SerializeContextTools
} // namespace AZ