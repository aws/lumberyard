/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
*or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "AudioControlBuilderWorker.h"

#include <AzCore/IO/SystemFile.h>
#include <AzCore/JSON/rapidjson.h>
#include <AzCore/JSON/document.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace CopyDependencyBuilder
{
    namespace Internal
    {
        const char NodeNameAudioPreloads[] = "AudioPreloads";
        const char NodeNamePreloadRequest[] = "ATLPreloadRequest";
        const char NodeNameAtlPlatforms[] = "ATLPlatforms";
        const char NodeNamePlatform[] = "Platform";
        const char AttributeNameAtlName[] = "atl_name";
        const char AttributeNameConfigGroupName[] = "atl_config_group_name";
        const char NodeNameConfigGroup[] = "ATLConfigGroup";
        const char NodeNameWwiseFile[] = "WwiseFile";
        const char AttributeNameWwiseName[] = "wwise_name";
        const char SoundsDirectoryPrefix[] = "sounds/wwise/";
        const char NodeNameAudioTriggers[] = "AudioTriggers";
        const char NodeNameTrigger[] = "ATLTrigger";
        const char NodeNameWwiseEvent[] = "WwiseEvent";
        const char JsonEventsKey[] = "includedEvents";
        const char SoundbankDependencyFileExtension[] = ".bankdeps";

        const char NodeDoesNotExistMessage[] = "%s node does not exist. Please be sure that you have defined at least one %s for this Audio Control file.";
        const char MalformedNodeMissingAttributeMessage[] = "%s node is malformed: does not have an attribute %s defined. This is likely the result of manual editing. Please resave the Audio Control file.";
        const char MalformedNodeMissingChildNodeMessage[] = "%s node does not contain a child %s node. This is likely the result of manual editing. Please resave the Audio Control file.";

        using AtlConfigGroupMap = AZStd::unordered_map<AZStd::string, const AZ::rapidxml::xml_node<char>*>;

        AZStd::string GetAtlPlatformName(const AZStd::string& requestPlatform)
        {
            AZStd::string atlPlatform;
            AZStd::string platform = requestPlatform;
            // When debugging a builder using a debug task, it replaces platform tags with "debug platform". Make sure the builder
            //  actually uses the host platform identifier when going through this function in this case.
            if (platform == "debug platform")
            {
                if (AZ::g_currentPlatform == AZ::PlatformID::PLATFORM_WINDOWS_32
                    || AZ::g_currentPlatform == AZ::PlatformID::PLATFORM_WINDOWS_64)
                {
                    platform = "pc";
                }
                else
                {
                    platform = AZ::GetPlatformName(AZ::g_currentPlatform);
                    AZStd::to_lower(platform.begin(), platform.end());
                }
            }

            if (platform == "pc")
            {
                atlPlatform = "windows";
            }
            else if (platform == "osx")
            {
                atlPlatform = "mac";
            }
            else if (platform == "linux")
            {
                atlPlatform = "linux";
            }
            else if (platform == "android")
            {
                atlPlatform = "android";
            }
            else if (platform == "ios")
            {
                atlPlatform = "ios";
            }
            else if (platform == "appletv")
            {
                atlPlatform == "appletv";
            }
            else
            {
                atlPlatform = "unknown";
            }
            return AZStd::move(atlPlatform);
        }

        AZ::Outcome<void, AZStd::string> BuildConfigGroupMap(const AZ::rapidxml::xml_node<char>* preloadRequestNode, AtlConfigGroupMap& configGroupMap)
        {
            if (!preloadRequestNode)
            {
                return AZ::Failure(AZStd::string::format(NodeDoesNotExistMessage, NodeNamePreloadRequest, "preload request"));
            }

            AZ::rapidxml::xml_node<char>* configGroupNode = preloadRequestNode->first_node(NodeNameConfigGroup);
            if (!configGroupNode)
            {
                // if no config groups are defined, then this is just an empty preload request with no banks referenced, which is 
                //  valid. return a success here.
                return AZ::Success();
            }

            // Populate the config group map by iterating over all ATLConfigGroup nodes, and place each one in the map keyed by the 
            //  group's atl_name attribute
            do
            {
                AZ::rapidxml::xml_attribute<char>* configGroupNameAttr = configGroupNode->first_attribute(AttributeNameAtlName);
                if (!configGroupNameAttr)
                {
                    return AZ::Failure(AZStd::string::format(MalformedNodeMissingAttributeMessage, NodeNameConfigGroup, AttributeNameAtlName));
                }
                
                configGroupMap.emplace(configGroupNameAttr->value(), configGroupNode);

                configGroupNode = configGroupNode->next_sibling(NodeNameConfigGroup);
            } while (configGroupNode);
            
            return AZ::Success();
        }

        AZ::Outcome<void, AZStd::string> BuildAtlEventList(const AZ::rapidxml::xml_node<char>* triggersNode, AZStd::vector<AZStd::string>& eventNames)
        {
            if (!triggersNode)
            {
                return AZ::Failure(AZStd::string::format(NodeDoesNotExistMessage, NodeNameAudioTriggers, "trigger"));
            }

            const AZ::rapidxml::xml_node<char>* triggerNode = triggersNode->first_node(NodeNameTrigger);
            if (!triggerNode)
            {
                return AZ::Failure(AZStd::string::format(MalformedNodeMissingChildNodeMessage, NodeNameAudioTriggers, NodeNameTrigger));
            }

            // For each audio trigger defined, if it has been assigned a wwise event to invoke, push the name of the wwise event into
            //  the list passed in.
            do 
            {
                const AZ::rapidxml::xml_node<char>* eventNode = triggerNode->first_node(NodeNameWwiseEvent);
                // it's okay for an ATLTrigger node to not have a wwise event associated with it, as the ATL trigger was defined
                //  but not assigned a wwise event yet.
                if (eventNode)
                {
                    const AZ::rapidxml::xml_attribute<char>* eventNameAttribute = eventNode->first_attribute(AttributeNameWwiseName);
                    if (!eventNameAttribute)
                    {
                        return AZ::Failure(AZStd::string::format(MalformedNodeMissingAttributeMessage, NodeNameWwiseEvent, AttributeNameWwiseName));
                    }
                    eventNames.push_back(eventNameAttribute->value());
                }

                triggerNode = triggerNode->next_sibling(NodeNameTrigger);
            } while (triggerNode);

            return AZ::Success();
        }

        AZ::Outcome<void, AZStd::string> GetBanksFromAtlPreloads(const AZ::rapidxml::xml_node<char>* preloadsNode, const AZStd::string& atlPlatformIdentifier, AZStd::vector<AZStd::string>& banksReferenced)
        {
            if (!preloadsNode)
            {
                return AZ::Failure(AZStd::string::format(NodeDoesNotExistMessage, NodeNameAudioPreloads, "preload request"));
            }

            AZ::rapidxml::xml_node<char>* preloadRequestNode = preloadsNode->first_node(NodeNamePreloadRequest);
            if (!preloadRequestNode)
            {
                return AZ::Failure(AZStd::string::format(NodeDoesNotExistMessage, NodeNamePreloadRequest, "preload request"));
            }

            // For each preload request in the control file, determine which config group is used for this platform and register each 
            //  bank listed in that preload request as a dependency.
            do
            {
                AtlConfigGroupMap configGroupMap;
                AZ::Outcome<void, AZStd::string> configGroupMapResult = BuildConfigGroupMap(preloadRequestNode, configGroupMap);
                // If the map returned is empty, then there are no banks that are referenced in the preload request, so 
                //  return the result here.
                if (!configGroupMapResult.IsSuccess() || configGroupMap.size() == 0)
                {
                    return configGroupMapResult;
                }

                AZStd::string configGroupName;
                const AZ::rapidxml::xml_node<char>* platformsNode = preloadRequestNode->first_node(NodeNameAtlPlatforms);
                if (!platformsNode)
                {
                    return AZ::Failure(AZStd::string::format(MalformedNodeMissingChildNodeMessage, NodeNamePreloadRequest, NodeNameAtlPlatforms));
                }

                const AZ::rapidxml::xml_node<char>* platformNode = platformsNode->first_node(NodeNamePlatform);
                if (!platformNode)
                {
                    return AZ::Failure(AZStd::string::format(MalformedNodeMissingChildNodeMessage, NodeNameAtlPlatforms, NodeNamePlatform));
                }

                // For each platform node in the platform list, check the atl_name to see if it matches the platform the request is
                //  intended for. If it is, grab the name of the config group that is used for that platform to load it.
                do
                {
                    const AZ::rapidxml::xml_attribute<char>* atlNameAttr = platformNode->first_attribute(AttributeNameAtlName);
                    if (!atlNameAttr)
                    {
                        return AZ::Failure(AZStd::string::format(MalformedNodeMissingAttributeMessage, NodeNamePlatform, AttributeNameAtlName));
                    }
                    else if (atlPlatformIdentifier == atlNameAttr->value())
                    {
                        // We've found the right platform that matches the request, so grab the group name and stop looking through
                        //  the list
                        const AZ::rapidxml::xml_attribute<char>* configGroupNameAttr = platformNode->first_attribute(AttributeNameConfigGroupName);
                        if (!configGroupNameAttr)
                        {
                            return AZ::Failure(AZStd::string::format(MalformedNodeMissingAttributeMessage, NodeNamePlatform, AttributeNameConfigGroupName));
                        }
                        configGroupName = configGroupNameAttr->value();
                        break;
                    }
                    
                    platformNode = platformNode->next_sibling(NodeNamePlatform);
                } while (platformNode);
                
                const AZ::rapidxml::xml_node<char>* configGroupNode = configGroupMap[configGroupName];
                if (!configGroupNode)
                {
                    // The config group this platform uses isn't defined in the control file. This might be intentional, so just 
                    //  generate a warning and keep going to the next preload node.
                    AZ_TracePrintf("Audio Control Builder", "%s node for config group %s is not defined, so no banks are referenced.", NodeNameConfigGroup, configGroupName.c_str());
                }
                else
                {
                    const AZ::rapidxml::xml_node<char>* wwiseFileNode = configGroupNode->first_node(NodeNameWwiseFile);
                    if (!wwiseFileNode)
                    {
                        return AZ::Failure(AZStd::string::format(MalformedNodeMissingChildNodeMessage, NodeNameConfigGroup, NodeNameWwiseFile));
                    }

                    // For each WwiseFile (soundbank) referenced in the config group, grab the file name and add it to the reference list
                    do
                    {
                        const AZ::rapidxml::xml_attribute<char>* bankNameAttribute = wwiseFileNode->first_attribute(AttributeNameWwiseName);
                        if (!bankNameAttribute)
                        {
                            return AZ::Failure(AZStd::string::format(MalformedNodeMissingAttributeMessage, NodeNameWwiseFile, AttributeNameWwiseName));
                        }

                        // Prepend the bank name with the relative path to the wwise sounds folder to get relative path to the bank from
                        //  the @assets@ alias and push that into the list of banks referenced.
                        AZStd::string soundsPrefix = SoundsDirectoryPrefix;
                        banksReferenced.emplace_back(soundsPrefix + bankNameAttribute->value());

                        wwiseFileNode = wwiseFileNode->next_sibling(NodeNameWwiseFile);
                    } while (wwiseFileNode);
                }
                
                preloadRequestNode = preloadRequestNode->next_sibling(Internal::NodeNamePreloadRequest);
            } while (preloadRequestNode);

            return AZ::Success();
        }
        
        AZ::Outcome<void, AZStd::string> GetEventsFromBankMetadata(const rapidjson::Value& rootObject, AZStd::set<AZStd::string>& eventNames)
        {
            if (!rootObject.IsObject())
            {
                return AZ::Failure(AZStd::string("The root of the metadata file is not an object. Please regenerate the metadata for this soundbank."));
            }

            // If the file doesn't define an events field, then there are no events in the bank
            if (!rootObject.HasMember(JsonEventsKey))
            {
                return AZ::Success();
            }

            const rapidjson::Value& eventsArray = rootObject[JsonEventsKey];
            if (!eventsArray.IsArray())
            {
                return AZ::Failure(AZStd::string("Events field is not an array. Please regenerate the metadata for this soundbank."));
            }

            for (rapidjson::SizeType eventIndex = 0; eventIndex < eventsArray.Size(); ++eventIndex)
            {
                eventNames.emplace(eventsArray[eventIndex].GetString());
            }
            return AZ::Success();
        }

        AZ::Outcome<void, AZStd::string> GetEventsFromBank(const AZStd::string& bankMetadataPath, AZStd::set<AZStd::string>& eventNames)
        {
            if (!AZ::IO::SystemFile::Exists(bankMetadataPath.c_str()))
            {
                return AZ::Failure(AZStd::string::format("Failed to find the soundbank metadata file %s. Full dependency information cannot be determined without the metadata file. Please regenerate the metadata for this soundbank.", bankMetadataPath.c_str()));
            }

            AZ::u64 fileSize = AZ::IO::SystemFile::Length(bankMetadataPath.c_str());
            if (fileSize == 0)
            {
                return AZ::Failure(AZStd::string::format("Soundbank metadata file at path %s is an empty file. Please regenerate the metadata for this soundbank.", bankMetadataPath.c_str()));
            }

            AZStd::vector<char> buffer(fileSize + 1);
            buffer[fileSize] = 0;
            if (!AZ::IO::SystemFile::Read(bankMetadataPath.c_str(), buffer.data()))
            {
                return AZ::Failure(AZStd::string::format("Failed to read the soundbank metadata file at path %s. Please make sure the file is not open or being edited by another program.", bankMetadataPath.c_str()));
            }

            rapidjson::Document bankMetadataDoc;
            bankMetadataDoc.Parse(buffer.data());
            if (bankMetadataDoc.GetParseError() != rapidjson::ParseErrorCode::kParseErrorNone)
            {
                return AZ::Failure(AZStd::string::format("Failed to parse soundbank metadata at path %s into JSON. Please regenerate the metadata for this soundbank.", bankMetadataPath.c_str()));
            }

            return GetEventsFromBankMetadata(bankMetadataDoc, eventNames);
        }
    }

    AudioControlBuilderWorker::AudioControlBuilderWorker()
        : XmlFormattedAssetBuilderWorker("Audio Control", true, true)
        , m_globalScopeControlsPath("libs/gameaudio/")
    {
        AzFramework::StringFunc::Path::Normalize(m_globalScopeControlsPath);
    }

    void AudioControlBuilderWorker::RegisterBuilderWorker()
    {
        AssetBuilderSDK::AssetBuilderDesc audioControlBuilderDescriptor;
        audioControlBuilderDescriptor.m_name = "AudioControlBuilderWorker";
        // pattern finds all Audio Control xml files in the libs/gameaudio folder and any of its subfolders.
        audioControlBuilderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("(.*libs\\/gameaudio\\/).*\\.xml", AssetBuilderSDK::AssetBuilderPattern::PatternType::Regex));
        audioControlBuilderDescriptor.m_busId = azrtti_typeid<AudioControlBuilderWorker>();
        audioControlBuilderDescriptor.m_version = 2;
        audioControlBuilderDescriptor.m_createJobFunction =
            AZStd::bind(&AudioControlBuilderWorker::CreateJobs, this, AZStd::placeholders::_1, AZStd::placeholders::_2);
        audioControlBuilderDescriptor.m_processJobFunction =
            AZStd::bind(&AudioControlBuilderWorker::ProcessJob, this, AZStd::placeholders::_1, AZStd::placeholders::_2);

        BusConnect(audioControlBuilderDescriptor.m_busId);

        AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBusTraits::RegisterBuilderInformation, audioControlBuilderDescriptor);
    }

    void AudioControlBuilderWorker::UnregisterBuilderWorker()
    {
        BusDisconnect();
    }

    AZ::Data::AssetType AudioControlBuilderWorker::GetAssetType(const AZStd::string& fileName) const
    {
        return AZ::Data::AssetType::CreateNull();
    }

    bool AudioControlBuilderWorker::ParseXmlFile(
        const AZ::rapidxml::xml_node<char>* node,
        const AZStd::string& fullPath,
        const AZStd::string& sourceFile,
        const AZStd::string& platformIdentifier,
        AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies,
        AssetBuilderSDK::ProductPathDependencySet& pathDependencies)
    {
        if (!node)
        {
            return false;
        }

        AddProductDependencies(node, fullPath, sourceFile, platformIdentifier, productDependencies, pathDependencies);
        return true;
    }

    void AudioControlBuilderWorker::AddProductDependencies(
        const AZ::rapidxml::xml_node<char>* node,
        const AZStd::string& fullPath,
        const AZStd::string& sourceFile,
        const AZStd::string& platformIdentifier,
        AZStd::vector<AssetBuilderSDK::ProductDependency>& /*productDependencies*/,
        AssetBuilderSDK::ProductPathDependencySet& pathDependencies)
    {
        // Convert platform name to platform name that is used by wwise and ATL.
        AZStd::string atlPlatformName = AZStd::move(Internal::GetAtlPlatformName(platformIdentifier));
        
        AZStd::vector<AZStd::string> banksReferenced;
        const AZ::rapidxml::xml_node<char>* preloadsNode = node->first_node(Internal::NodeNameAudioPreloads);
        if (!preloadsNode)
        {
            // No preloads were defined in this control file, so we can return. If triggers are defined in this preload file, we can't
            // validate that they'll be playable because we are unsure of what other control files for the given scope are defined.
            return;
        }
        
        AZ::Outcome<void, AZStd::string> gatherBankReferencesResult = Internal::GetBanksFromAtlPreloads(preloadsNode, atlPlatformName, banksReferenced);
        if (!gatherBankReferencesResult.IsSuccess())
        {
            AZ_Warning("Audio Control Builder", false, "Failed to gather product dependencies for Audio Control file %s. %s", sourceFile.c_str(), gatherBankReferencesResult.GetError().c_str());
            return;
        }
        else if (banksReferenced.size() == 0)
        {
            // If there are no banks referenced, then there are no dependencies to register, so just return.
            return;
        }
        
        for (const AZStd::string& relativeBankPath : banksReferenced)
        {
            pathDependencies.emplace(relativeBankPath, AssetBuilderSDK::ProductPathDependencyType::ProductFile);
        }


        // For each bank figure out what events are included in the bank, then run through every event referenced in the file and 
        //  make sure it is in the list gathered from the banks.
        AZStd::vector<AZStd::string> eventsReferenced;
        const AZ::rapidxml::xml_node<char>* triggersNode = node->first_node(Internal::NodeNameAudioTriggers);
        if (!triggersNode)
        {
            // No triggers were defined in this file, so we don't need to do any event validation.
            return;
        }

        AZ::Outcome<void, AZStd::string> gatherEventReferencesResult = Internal::BuildAtlEventList(triggersNode, eventsReferenced);
        if (!gatherEventReferencesResult.IsSuccess())
        {
            AZ_Warning("Audio Control Builder", false, "Failed to gather list of events referenced by Audio Control file %s. %s", sourceFile.c_str(), gatherEventReferencesResult.GetError().c_str());
            return;
        }

        AZStd::string projectSourcePath = fullPath;
        AZ::u64 firstSubDirectoryIndex = AzFramework::StringFunc::Find(projectSourcePath, m_globalScopeControlsPath);
        AzFramework::StringFunc::LKeep(projectSourcePath, firstSubDirectoryIndex);

        AZStd::set<AZStd::string> wwiseEventsInReferencedBanks;
        
        // Load all bankdeps files for all banks referenced and aggregate the list of events in those files. 
        for (const AZStd::string& relativeBankPath : banksReferenced)
        {
            // Create the full path to the bankdeps file from the bank file.
            AZStd::string bankMetadataPath;
            AzFramework::StringFunc::Path::Join(projectSourcePath.c_str(), relativeBankPath.c_str(), bankMetadataPath);
            AzFramework::StringFunc::Path::ReplaceExtension(bankMetadataPath, Internal::SoundbankDependencyFileExtension);

            AZ::Outcome<void, AZStd::string> getReferencedEventsResult = Internal::GetEventsFromBank(bankMetadataPath, wwiseEventsInReferencedBanks);
            if (!getReferencedEventsResult.IsSuccess())
            {
                // only warn if we couldn't get info from a bankdeps file. Won't impact registering dependencies, but used to help
                // customers potentially debug issues.
                AZ_Warning("Audio Control Builder", false, "Failed to gather list of events referenced by soundbank %s. %s", relativeBankPath.c_str(), getReferencedEventsResult.GetError().c_str());
            }
        }

        // Confirm that each event referenced by the file exists in the list of events available from the banks referenced.
        for (const AZStd::string& eventInControlFile : eventsReferenced)
        {
            if (wwiseEventsInReferencedBanks.find(eventInControlFile) == wwiseEventsInReferencedBanks.end())
            {
                AZ_Warning("Audio Control Builder", false, "Failed to find Wwise event %s in the list of events contained in banks referenced by %s. Event may fail to play properly.", eventInControlFile.c_str(), sourceFile.c_str());
            }
        }
    }
}
