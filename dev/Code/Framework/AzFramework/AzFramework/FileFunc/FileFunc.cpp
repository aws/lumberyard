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
#include "FileFunc.h"

#include <regex>

#include <AzCore/std/string/string.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/TextStreamWriters.h>
#include <AzCore/JSON/document.h>
#include <AzCore/JSON/rapidjson.h>
#include <AzCore/JSON/error/error.h>
#include <AzCore/JSON/error/en.h>
#include <AzCore/JSON/prettywriter.h>

#include <AzFramework/StringFunc/StringFunc.h>

#define MAX_TEXT_LINE_BUFFER_SIZE   512

namespace AzFramework
{
    namespace FileFunc
    {
        AZ::Outcome<rapidjson::Document, AZStd::string> ReadJsonFile(const AZStd::string& jsonFilePath, AZ::IO::FileIOBase* overrideFileIO /*= nullptr*/)
        {
            AZ::IO::FileIOBase* fileIo = overrideFileIO != nullptr ? overrideFileIO : AZ::IO::FileIOBase::GetInstance();
            if (fileIo == nullptr)
            {
                return AZ::Failure(AZStd::string("No FileIO instance present."));
            }

            AZ::IO::HandleType fileHandle;
            if (!fileIo->Open(jsonFilePath.c_str(), AZ::IO::OpenMode::ModeRead, fileHandle))
            {
                fileIo->Close(fileHandle);
                return AZ::Failure(AZStd::string("Failed to open."));
            }

            AZ::u64 fileSize = 0;
            if (!fileIo->Size(fileHandle, fileSize))
            {
                fileIo->Close(fileHandle);
                return AZ::Failure(AZStd::string::format("Failed to read size of file."));
            }

            AZStd::string jsonText;
            jsonText.resize(fileSize);
            if (!fileIo->Read(fileHandle, jsonText.data(), fileSize, true))
            {
                fileIo->Close(fileHandle);
                return AZ::Failure(AZStd::string::format("Failed to read file."));
            }
            fileIo->Close(fileHandle);

            rapidjson::Document document;
            if (document.Parse(jsonText.c_str()).HasParseError())
            {
                return AZ::Failure(
                    AZStd::string::format("Error parsing json contents (offset %u): %s",
                        document.GetErrorOffset(),
                        rapidjson::GetParseError_En(document.GetParseError())));
            }

            return AZ::Success(AZStd::move(document));
        }

        AZ::Outcome<void, AZStd::string> WriteJsonFile(const rapidjson::Document& jsonDoc, const AZStd::string& jsonFilePath)
        {
            AZ::IO::FileIOStream       outputFileStream;
            if (!outputFileStream.Open(jsonFilePath.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeText))
            {
                return AZ::Failure(AZStd::string::format("Error opening file '%s' for writing", jsonFilePath.c_str()));
            }

            AZ::IO::RapidJSONStreamWriter jsonStreamWriter(&outputFileStream);

            rapidjson::PrettyWriter<AZ::IO::RapidJSONStreamWriter> writer(jsonStreamWriter);

            bool jsonWriteResult = jsonDoc.Accept(writer);

            outputFileStream.Close();

            if (!jsonWriteResult)
            {
                return AZ::Failure(AZStd::string::format("Unable to write json document to file '%s'", jsonFilePath.c_str()));
            }

            return AZ::Success();
        }

        namespace Internal
        {
            static bool FindFilesInPath(const AZStd::string& folder, const AZStd::string& filter, bool recurseSubFolders, AZStd::list<AZStd::string>& fileList)
            {
                AZStd::string file_filter = folder;
                file_filter.append(AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING).append(filter.empty() ? "*" : filter.c_str());

                AZ::IO::SystemFile::FindFiles(file_filter.c_str(), [folder, filter, recurseSubFolders, &fileList](const char* item, bool is_file)
                    {
                        // Skip the '.' and '..' folders
                        if ((azstricmp(".", item) == 0) || (azstricmp("..", item) == 0))
                        {
                            return true;
                        }

                        // Continue if we can
                        AZStd::string fullPath;
                        if (!AzFramework::StringFunc::Path::Join(folder.c_str(), item, fullPath))
                        {
                            return false;
                        }

                        if (is_file)
                        {
                            if (AZ::IO::NameMatchesFilter(item, filter.c_str()))
                            {
                                fileList.push_back(fullPath);
                            }
                        }
                        else
                        {
                            if (recurseSubFolders)
                            {
                                return FindFilesInPath(fullPath, filter, recurseSubFolders, fileList);
                            }
                        }
                        return true;
                    });
                return true;
            }

            /**
            * Creates a regex pattern string to find anything matching the pattern "[key] = [value]", with all possible whitespace variations accounted for.
            *
            * \param[in] key         The key value for the pattern
            * \return string of the regex pattern based on the key
            */
            static AZStd::string BuildConfigKeyValueRegex(const AZStd::string& key)
            {
                return AZStd::string("(^|\\n\\s*)" + key + "([ \\t]*)=([ \\t]*)(.*)");
            }

            /**
            * Replaces a value in an ini-style content.
            *
            * \param[in] cfgContents       The contents of the ini-style file
            * \param[in] header            The optional group title for the key
            * \param[in] key               The key of the value to replace
            * \param[in] newValue          The value to assign to the key
            *
            * \returns Void on success, error message on failure.
            *
            * If key has multiple values, it does not preserve other values.
            *      enabled_game_projects = oldProject1, oldProject2, oldProject3
            *      becomes
            *      enabled_game_projects = newProject
            * Cases to handle:
            *      1. key exists, value is identical -> do nothing
            *          This can occur if a user modifies a config file while the tooling is open.
            *          Example:
            *              1) Load Project Configurator.
            *              2) Modify your bootstrap.cfg to a different project.
            *              3) Tell Project Configurator to set your project to the project you set in #2.
            *      2. Key exists, value is missing -> stomps over key with new key value pair.
            *      3. Key exists, value is different -> stomps over key with new key value pair. Ignores previous values.
            *      4. Key does not exist, header is not required -> insert "key = value" at the beginning of file.
            *      5. Key does not exist, required header does not exist -> insert header + a newline to begining of file.
            *      6. Key does not exist, required header does exist -> insert "key = value" after first newline after header.
            *          The header will always exist at this point, it was created in the previous check if it was missing.
            */
            AZ::Outcome<void, AZStd::string> UpdateCfgContents(AZStd::string& cfgContents, const AZStd::string& header, const AZStd::string& key, const AZStd::string& value)
            {
                // Generate regex str and replacement str
                AZStd::string lhsStr = key;
                AZStd::string regexStr = BuildConfigKeyValueRegex(lhsStr);
                AZStd::string gameFolderAssignment = "$1" + lhsStr + "$2=$03" + value;

                // Replace the current key with the new value
                // AZStd's regex was not functional at the time this was authored, so convert to std from AZStd.
                std::regex sysGameRegex(regexStr.c_str());
                std::smatch matchResults;
                std::string settingsFileContentsStdStr = cfgContents.c_str();
                bool matchFound = std::regex_search(settingsFileContentsStdStr, matchResults, sysGameRegex);

                // Case 1, 2, and 3 - Key value pair exists, stomp over the key with a new value pair.
                std::string result;
                if (matchFound)
                {
                    result = std::regex_replace(cfgContents.c_str(), sysGameRegex, gameFolderAssignment.c_str());
                }
                // Cases 4 through 6 - the key does not exist.
                if (!matchFound)
                {
                    result = cfgContents.c_str();
                    std::size_t insertLocation = 0;
                    // Case 5 & 6 - key does not exist, header is required
                    if (!header.empty())
                    {
                        insertLocation = result.find(header.c_str());
                        // Case 6 - key does not exist, header does exist.
                        //      Set the key insertion point to right after the header.
                        if (insertLocation != std::string::npos)
                        {
                            insertLocation += header.length();
                        }
                        // Case 5 - key does not exist, required header does not exist.
                        //      Insert the header at the top of the file, set the key insertion point after the header.
                        else
                        {
                            AZStd::string headerAndCr = header + "\n";
                            result.insert(0, headerAndCr.c_str());
                            insertLocation = header.length();
                        }
                    }
                    // Case 4 - key does not exist, header is not required.
                    // Case 5 & 6 - the header has been added.
                    //  Insert the key at the tracked insertion point
                    std::string newConfigData = std::string("\n") +
                        std::string(key.c_str()) +
                        std::string(" = ") +
                        std::string(value.c_str());
                    result.insert(insertLocation, newConfigData);
                }
                cfgContents = result.c_str();
                return AZ::Success();
            }

            /**
            * Updates a configuration (ini) style contents string representation with a list of replacement string rules
            *
            * \param[in] cfgContents    The contents of the ini file to update
            * \param[in] updateRules    The update rules (list of update strings, see below) to apply
            *
            * The replace string rule format follows the following:
            *       ([header]/)[key]=[value]
            *
            *    where:
            *       header (optional) : Optional group title for the key
            *       key               : The key to lookup or create for the value
            *       value             : The value to update or create
            */
            AZ::Outcome<void, AZStd::string> UpdateCfgContents(AZStd::string& cfgContents, const AZStd::list<AZStd::string>& updateRules)
            {
                AZStd::string updateCfgContents = cfgContents;
                for (const AZStd::string& updateRule : updateRules)
                {
                    // Since we are parsing manually anyways, no need to do a regex validation
                    AZStd::string::size_type headerKeySep = updateRule.find_first_of('/');
                    bool hasHeader = (headerKeySep != AZStd::string::npos);
                    AZStd::string::size_type keyValueSep = updateRule.find_first_of('=', !hasHeader ? 0 : headerKeySep + 1);
                    if (keyValueSep == AZStd::string::npos)
                    {
                        return AZ::Failure(AZStd::string::format("Invalid update config rule '%s'.  Must be in the format ([header]/)[key]=[value]", updateRule.c_str()));
                    }
                    AZStd::string header = hasHeader ? updateRule.substr(0, headerKeySep) : AZStd::string("");
                    AZStd::string key = hasHeader ? updateRule.substr(headerKeySep + 1, keyValueSep - (headerKeySep + 1)) : updateRule.substr(0, keyValueSep);
                    AZStd::string value = updateRule.substr(keyValueSep + 1);

                    auto updateResult = UpdateCfgContents(updateCfgContents, header, key, value);
                    if (!updateResult.IsSuccess())
                    {
                        return updateResult;
                    }
                }
                cfgContents = updateCfgContents;
                return AZ::Success();
            }
        }

        AZ::Outcome<AZStd::list<AZStd::string>, AZStd::string> FindFilesInPath(const AZStd::string& folder, const AZStd::string& filter, bool recurseSubFolders)
        {
            AZStd::list<AZStd::string> fileList;

            if (!Internal::FindFilesInPath(folder, filter, recurseSubFolders, fileList))
            {
                return AZ::Failure(AZStd::string::format("Error finding files in path '%s'", folder.c_str()));
            }
            else
            {
                return AZ::Success(fileList);
            }
        }

        AZ::Outcome<void, AZStd::string> ReadTextFileByLine(const AZStd::string& filePath, AZStd::function<bool(const char* line)> perLineCallback)
        {
            FILE* file_handle = nullptr;
            azfopen(&file_handle, filePath.c_str(), "rt");
            if (!file_handle)
            {
                return AZ::Failure(AZStd::string::format("Error opening file '%s' for reading", filePath.c_str()));
            }
            char line[MAX_TEXT_LINE_BUFFER_SIZE] = { '\0' };
            while (fgets(line, sizeof(line), static_cast<FILE*>(file_handle)) != nullptr)
            {
                if (line[0])
                {
                    if (!perLineCallback(line))
                    {
                        break;
                    }
                }
            }
            fclose(file_handle);
            return AZ::Success();
        }

        AZ::Outcome<void, AZStd::string> ReplaceInCfgFile(const AZStd::string& filePath, const AZStd::list<AZStd::string>& updateRules)
        {
            if (!AZ::IO::SystemFile::Exists(filePath.c_str()))
            {
                return AZ::Failure(AZStd::string::format("Cfg file '%s' does not exist.", filePath.c_str()));
            }

            // Load the settings file into a string
            AZ::IO::SystemFile settingsFile;
            if (!settingsFile.Open(filePath.c_str(), AZ::IO::SystemFile::SF_OPEN_READ_ONLY))
            {
                return AZ::Failure(AZStd::string::format("Error reading cfg file '%s'.", filePath.c_str()));
            }

            AZStd::string settingsFileContents(settingsFile.Length(), '\0');
            settingsFile.Read(settingsFileContents.size(), settingsFileContents.data());
            settingsFile.Close();

            auto updateContentResult = Internal::UpdateCfgContents(settingsFileContents, updateRules);
            if (!updateContentResult.IsSuccess())
            {
                return updateContentResult;
            }

            // Write the result.
            if (!settingsFile.ReOpen(AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY | AZ::IO::SystemFile::SF_OPEN_TRUNCATE))
            {
                return AZ::Failure(AZStd::string::format("Failed to open settings file %s.", filePath.c_str()));
            }
            auto bytesWritten = settingsFile.Write(settingsFileContents.c_str(), settingsFileContents.size());
            settingsFile.Close();

            if (bytesWritten != settingsFileContents.size())
            {
                return AZ::Failure(AZStd::string::format("Failed to write to config file %s.", filePath.c_str()));
            }

            return AZ::Success();
        }

        AZ::Outcome<void, AZStd::string> ReplaceInCfgFile(const AZStd::string& filePath, const char* header, const AZStd::string& key, const AZStd::string& newValue)
        {
            if (key.length() <= 0)
            {
                return AZ::Failure(AZStd::string("'key' parameter for ReplaceInCfgFile must not be empty"));
            }

            AZStd::string updateRule = (header != nullptr) ? AZStd::string::format("%s/%s=%s", header, key.c_str(), newValue.c_str()) : AZStd::string::format("%s=%s", key.c_str(), newValue.c_str());
            AZStd::list<AZStd::string> updateRules{
                updateRule
            };
            auto replaceResult = ReplaceInCfgFile(filePath, updateRules);
            return replaceResult;
        }

        AZ::Outcome<AZStd::string, AZStd::string> GetValueForKeyInCfgFile(const AZStd::string& filePath, const char* key)
        {
            if (!AZ::IO::SystemFile::Exists(filePath.c_str()))
            {
                return AZ::Failure(AZStd::string::format("Cfg file '%s' does not exist.", filePath.c_str()));
            }

            // Load the settings file into a string
            AZ::IO::SystemFile settingsFile;
            if (!settingsFile.Open(filePath.c_str(), AZ::IO::SystemFile::SF_OPEN_READ_ONLY))
            {
                return AZ::Failure(AZStd::string::format("Error reading cfg file '%s'.", filePath.c_str()));
            }

            AZStd::string settingsFileContents(settingsFile.Length(), '\0');
            settingsFile.Read(settingsFileContents.size(), settingsFileContents.data());
            settingsFile.Close();

            // Generate regex str and replacement str
            AZStd::string lhsStr = key;
            AZStd::string regexStr = Internal::BuildConfigKeyValueRegex(lhsStr);

            std::regex sysGameRegex(regexStr.c_str());
            std::cmatch matchResult;
            if (!std::regex_search(settingsFileContents.c_str(), matchResult, sysGameRegex))
            {
                return AZ::Failure(AZStd::string::format("Could not find key %s in config file %s", key, filePath.c_str()));
            }

            if (matchResult.size() <= 4)
            {
                return AZ::Failure(AZStd::string::format("No results for key %s in config file %s", key, filePath.c_str()));
            }

            return AZ::Success(AZStd::string(matchResult[4].str().c_str()));
        }
    } // namespace FileFunc
} // namespace AzFramework