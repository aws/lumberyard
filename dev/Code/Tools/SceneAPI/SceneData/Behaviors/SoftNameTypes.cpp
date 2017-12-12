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

#include <AzCore/IO/FileIO.h>
#include <AzCore/std/utils.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/JSON/reader.h>
#include <AzCore/JSON/error/en.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/DataTypes/IGraphObject.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IAnimationData.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneData/Behaviors/SoftNameTypes.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneData
        {
            AZ_CLASS_ALLOCATOR_IMPL(SoftNameTypes, SystemAllocator, 0);

            SoftNameTypes::VirtualType::VirtualType(AZStd::string&& name)
            {
                SetName(AZStd::move(name));
            }

            void SoftNameTypes::VirtualType::SetName(AZStd::string&& name)
            {
                m_nameId = Crc32(name.c_str());
                m_name = AZStd::move(name);
            }
            
            const AZStd::string& SoftNameTypes::VirtualType::GetName() const
            {
                return m_name;
            }

            Crc32 SoftNameTypes::VirtualType::GetNameId() const
            {
                return m_nameId;
            }

            SoftNameTypes::NodeNameBasedVirtualType::NodeNameBasedVirtualType(AZStd::string&& name, SceneCore::PatternMatcher&& matcher, bool includeChildren)
                : VirtualType(AZStd::move(name))
                , m_matcher(AZStd::move(matcher))
                , m_includeChildren(includeChildren)
            {
            }

            SoftNameTypes::NodeNameBasedVirtualType::NodeNameBasedVirtualType()
                : m_includeChildren(false)
            {
            }

            AZStd::unique_ptr<SoftNameTypes::NodeNameBasedVirtualType> SoftNameTypes::NodeNameBasedVirtualType::LoadFromJson(
                AZStd::string&& name, rapidjson::Document::ConstMemberIterator member)
            {
                AZStd::unique_ptr<NodeNameBasedVirtualType> virtualType(new NodeNameBasedVirtualType());

                if (!virtualType->m_matcher.LoadFromJson(member))
                {
                    return nullptr;
                }

                if (member->value.HasMember("IncludeChildren"))
                {
                    const auto& includeChildrenValue = member->value["IncludeChildren"];
                    if (!includeChildrenValue.IsBool())
                    {
                        AZ_TracePrintf(Utilities::WarningWindow, "Element 'IncludeChildren' is not a boolean.");
                    }
                    else if (includeChildrenValue.GetBool())
                    {
                        virtualType->m_includeChildren = true;
                    }
                }

                virtualType->SetName(AZStd::move(name));

                return virtualType;
            }

            bool SoftNameTypes::NodeNameBasedVirtualType::IsVirtualType(const Containers::Scene& scene, Containers::SceneGraph::NodeIndex node) const
            {
                const Containers::SceneGraph& graph = scene.GetGraph();
                if (m_includeChildren)
                {
                    while (node.IsValid())
                    {
                        bool nameMatch = false;
                        const Containers::SceneGraph::Name& name = graph.GetNodeName(node);
                        switch (m_matcher.GetMatchApproach())
                        {
                        case SceneCore::PatternMatcher::MatchApproach::PreFix:
                            nameMatch = m_matcher.MatchesPattern(name.GetName(), name.GetNameLength());
                            break;
                        case SceneCore::PatternMatcher::MatchApproach::PostFix:
                            nameMatch = m_matcher.MatchesPattern(name.GetPath(), name.GetPathLength());
                            break;
                        case SceneCore::PatternMatcher::MatchApproach::Regex:
                            nameMatch = m_matcher.MatchesPattern(name.GetPath(), name.GetPathLength());
                            break;
                        default:
                            AZ_Assert(false, "Unknown option '%i' for pattern matcher.", m_matcher.GetMatchApproach());
                            return false;
                        }
                        if (nameMatch)
                        {
                            return true;
                        }
                        node = graph.GetNodeParent(node);
                    }
                    return false;
                }
                else
                {
                    const Containers::SceneGraph::Name& name = graph.GetNodeName(node);
                    switch (m_matcher.GetMatchApproach())
                    {
                    case SceneCore::PatternMatcher::MatchApproach::PreFix:
                        return m_matcher.MatchesPattern(name.GetName(), name.GetNameLength());
                    case SceneCore::PatternMatcher::MatchApproach::PostFix:
                        return m_matcher.MatchesPattern(name.GetPath(), name.GetPathLength());
                    case SceneCore::PatternMatcher::MatchApproach::Regex:
                        return m_matcher.MatchesPattern(name.GetPath(), name.GetPathLength());
                    default:
                        AZ_Assert(false, "Unknown option '%i' for pattern matcher.", m_matcher.GetMatchApproach());
                        return false;
                    }
                }
            }

            SoftNameTypes::FileNameBasedVirtualType::FileNameBasedVirtualType(AZStd::string&& name, SceneCore::PatternMatcher&& matcher,
                AZStd::vector<AZ::Uuid>&& types, bool inclusiveMatch)
                : VirtualType(AZStd::move(name))
                , m_nodeTypes(AZStd::move(types))
                , m_matcher(AZStd::move(matcher))
                , m_matchInclusive(inclusiveMatch)
                , m_cachedScene(nullptr)
                , m_cachedNameMatch(false)
            {
            }

            SoftNameTypes::FileNameBasedVirtualType::FileNameBasedVirtualType()
                : m_matchInclusive(false)
                , m_cachedScene(nullptr)
                , m_cachedNameMatch(false)
            {
            }

            AZStd::unique_ptr<SoftNameTypes::FileNameBasedVirtualType> SoftNameTypes::FileNameBasedVirtualType::LoadFromJson(AZStd::string&& name,
                rapidjson::Document::ConstMemberIterator member)
            {
                AZStd::unique_ptr<FileNameBasedVirtualType> virtualType(new FileNameBasedVirtualType());

                if (!virtualType->m_matcher.LoadFromJson(member))
                {
                    return nullptr;
                }

                if (member->value.HasMember("NodeTypes"))
                {
                    const auto& nodeTypes = member->value["NodeTypes"];
                    if (nodeTypes.IsString())
                    {
                        virtualType->m_nodeTypes.push_back(Uuid::CreateString(nodeTypes.GetString(), nodeTypes.GetStringLength()));
                    }
                    else if (nodeTypes.IsArray())
                    {
                        for (auto type = nodeTypes.Begin(); type != nodeTypes.End(); ++type)
                        {
                            if (type->IsString())
                            {
                                virtualType->m_nodeTypes.push_back(Uuid::CreateString(type->GetString(), type->GetStringLength()));
                            }
                        }
                    }
                    else
                    {
                        AZ_TracePrintf(Utilities::WarningWindow, "Element 'NodeTypes' is expected to be a string or an array of strings.");
                        return nullptr;
                    }
                }
                else
                {
                    AZ_TracePrintf(Utilities::WarningWindow, "Mandatory element 'NodeTypes' is not found.");
                    return nullptr;
                }

                if (member->value.HasMember("InclusiveMatch"))
                {
                    const auto& inclusiveFilter = member->value["InclusiveMatch"];
                    if (!inclusiveFilter.IsBool())
                    {
                        AZ_TracePrintf(Utilities::WarningWindow, "Element 'InclusiveMatch' is not a boolean.");
                    }
                    else
                    {
                        virtualType->m_matchInclusive = inclusiveFilter.GetBool();
                    }
                }

                virtualType->SetName(AZStd::move(name));

                return virtualType;
            }

            bool SoftNameTypes::FileNameBasedVirtualType::IsVirtualType(const Containers::Scene& scene, Containers::SceneGraph::NodeIndex node) const
            {
                bool nameMatch = false;
                if (m_cachedScene != &scene)
                {
                    switch (m_matcher.GetMatchApproach())
                    {
                    case SceneCore::PatternMatcher::MatchApproach::PreFix:
                        nameMatch = m_matcher.MatchesPattern(scene.GetName());
                        break;
                    case SceneCore::PatternMatcher::MatchApproach::PostFix:
                        nameMatch = m_matcher.MatchesPattern(scene.GetName());
                        break;
                    case SceneCore::PatternMatcher::MatchApproach::Regex:
                        nameMatch = m_matcher.MatchesPattern(scene.GetSourceFilename());
                        break;
                    default:
                        AZ_Assert(false, "Unknown option '%i' for pattern matcher.", m_matcher.GetMatchApproach());
                        nameMatch = false;
                    }

                    m_cachedScene = &scene;
                    m_cachedNameMatch = nameMatch;
                }
                else
                {
                    nameMatch = m_cachedNameMatch;
                }
                if (!nameMatch)
                {
                    return false;
                }

                AZStd::shared_ptr<const DataTypes::IGraphObject> object = scene.GetGraph().GetNodeContent(node);
                for (const AZ::Uuid& type : m_nodeTypes)
                {
                    if (object->RTTI_IsTypeOf(type))
                    {
                        return m_matchInclusive;
                    }
                }

                return m_matchInclusive == false;
            }
                
            SoftNameTypes::SoftNameTypes()
                : m_connected(false)
            {
            }

            SoftNameTypes::~SoftNameTypes()
            {
                Clear();
            }

            bool SoftNameTypes::InitializeFromConfigFile(const char* filePath)
            {
                AZ_TraceContext("Virtual type config file", filePath);
                
                Clear();

                AZStd::string json = LoadTextFile(filePath);
                if (json.empty())
                {
                    return false;
                }

                if (!ParseJson(json))
                {
                    return false;
                }

                BusConnect();
                m_connected = true;
                
                return true;
            }

            bool SoftNameTypes::InitializeFromConfigFile(const AZStd::string& filePath)
            {
                return InitializeFromConfigFile(filePath.c_str());
            }

            void SoftNameTypes::InitializeWithDefaults()
            {
                Clear();

                // If these default values are updated they also need to be mirrored in "Gems/AssetImporterConfig/Assets/virtual_types.json"
                AddNodeNameBasedVirtualType("PhysicsMesh", SceneCore::PatternMatcher("_phys", SceneCore::PatternMatcher::MatchApproach::PostFix), true);
                AddNodeNameBasedVirtualType("LODMesh1", SceneCore::PatternMatcher("_lod1", SceneCore::PatternMatcher::MatchApproach::PostFix), true);
                AddNodeNameBasedVirtualType("LODMesh2", SceneCore::PatternMatcher("_lod2", SceneCore::PatternMatcher::MatchApproach::PostFix), true);
                AddNodeNameBasedVirtualType("LODMesh3", SceneCore::PatternMatcher("_lod3", SceneCore::PatternMatcher::MatchApproach::PostFix), true);
                AddNodeNameBasedVirtualType("LODMesh4", SceneCore::PatternMatcher("_lod4", SceneCore::PatternMatcher::MatchApproach::PostFix), true);
                AddNodeNameBasedVirtualType("LODMesh5", SceneCore::PatternMatcher("_lod5", SceneCore::PatternMatcher::MatchApproach::PostFix), true);
                AddNodeNameBasedVirtualType("Ignore", SceneCore::PatternMatcher("_ignore", SceneCore::PatternMatcher::MatchApproach::PostFix), false);
                AddFileNameBasedVirtualType("Ignore", SceneCore::PatternMatcher("_anim", SceneCore::PatternMatcher::MatchApproach::PostFix),
                    {
                        DataTypes::IAnimationData::TYPEINFO_Uuid()
                    }, false);

                BusConnect();
                m_connected = true;
            }

            void SoftNameTypes::AddNodeNameBasedVirtualType(const AZStd::string& name, const SceneCore::PatternMatcher& matcher, bool includeChildren)
            {
                AddNodeNameBasedVirtualType(AZStd::string(name), SceneCore::PatternMatcher(matcher), includeChildren);
            }

            void SoftNameTypes::AddNodeNameBasedVirtualType(AZStd::string&& name, const SceneCore::PatternMatcher& matcher, bool includeChildren)
            {
                AddNodeNameBasedVirtualType(AZStd::move(name), SceneCore::PatternMatcher(matcher), includeChildren);
            }
            
            void SoftNameTypes::AddNodeNameBasedVirtualType(const AZStd::string& name, SceneCore::PatternMatcher&& matcher, bool includeChildren)
            {
                AddNodeNameBasedVirtualType(AZStd::string(name), AZStd::move(matcher), includeChildren);
            }

            void SoftNameTypes::AddNodeNameBasedVirtualType(AZStd::string&& name, SceneCore::PatternMatcher&& matcher, bool includeChildren)
            {
                m_types.emplace_back(new NodeNameBasedVirtualType(AZStd::move(name), AZStd::move(matcher), includeChildren));
            }

            void SoftNameTypes::AddFileNameBasedVirtualType(const AZStd::string& name, const SceneCore::PatternMatcher& matcher, const AZStd::vector<AZ::Uuid>& types, bool inclusiveMatch)
            {
                AddFileNameBasedVirtualType(AZStd::string(name), SceneCore::PatternMatcher(matcher), AZStd::vector<AZ::Uuid>(types), inclusiveMatch);
            }

            void SoftNameTypes::AddFileNameBasedVirtualType(AZStd::string&& name, const SceneCore::PatternMatcher& matcher, const AZStd::vector<AZ::Uuid>& types, bool inclusiveMatch)
            {
                AddFileNameBasedVirtualType(AZStd::move(name), SceneCore::PatternMatcher(matcher), AZStd::vector<AZ::Uuid>(types), inclusiveMatch);
            }

            void SoftNameTypes::AddFileNameBasedVirtualType(const AZStd::string& name, SceneCore::PatternMatcher&& matcher, const AZStd::vector<AZ::Uuid>& types, bool inclusiveMatch)
            {
                AddFileNameBasedVirtualType(AZStd::string(name), AZStd::move(matcher), AZStd::vector<AZ::Uuid>(types), inclusiveMatch);
            }

            void SoftNameTypes::AddFileNameBasedVirtualType(AZStd::string&& name, SceneCore::PatternMatcher&& matcher, const AZStd::vector<AZ::Uuid>& types, bool inclusiveMatch)
            {
                AddFileNameBasedVirtualType(AZStd::move(name), AZStd::move(matcher), AZStd::vector<AZ::Uuid>(types), inclusiveMatch);
            }

            void SoftNameTypes::AddFileNameBasedVirtualType(const AZStd::string& name, const SceneCore::PatternMatcher& matcher, AZStd::vector<AZ::Uuid>&& types, bool inclusiveMatch)
            {
                AddFileNameBasedVirtualType(AZStd::string(name), SceneCore::PatternMatcher(matcher), AZStd::move(types), inclusiveMatch);
            }

            void SoftNameTypes::AddFileNameBasedVirtualType(AZStd::string&& name, const SceneCore::PatternMatcher& matcher, AZStd::vector<AZ::Uuid>&& types, bool inclusiveMatch)
            {
                AddFileNameBasedVirtualType(AZStd::move(name), SceneCore::PatternMatcher(matcher), AZStd::move(types), inclusiveMatch);
            }

            void SoftNameTypes::AddFileNameBasedVirtualType(const AZStd::string& name, SceneCore::PatternMatcher&& matcher, AZStd::vector<AZ::Uuid>&& types, bool inclusiveMatch)
            {
                AddFileNameBasedVirtualType(AZStd::string(name), AZStd::move(matcher), AZStd::move(types), inclusiveMatch);
            }

            void SoftNameTypes::AddFileNameBasedVirtualType(AZStd::string&& name, SceneCore::PatternMatcher&& matcher, AZStd::vector<AZ::Uuid>&& types, bool inclusiveMatch)
            {
                m_types.emplace_back(new FileNameBasedVirtualType(AZStd::move(name), AZStd::move(matcher), AZStd::move(types), inclusiveMatch));
            }

            bool SoftNameTypes::IsVirtualType(const Containers::Scene& scene, Containers::SceneGraph::NodeIndex node)
            {
                for (const AZStd::unique_ptr<VirtualType>& it : m_types)
                {
                    if (it->IsVirtualType(scene, node))
                    {
                        return true;
                    }
                }
                return false;
            }

            void SoftNameTypes::GetVirtualTypes(AZStd::set<Crc32>& types, const Containers::Scene& scene,
                Containers::SceneGraph::NodeIndex node)
            {
                for (const AZStd::unique_ptr<VirtualType>& it : m_types)
                {
                    if (types.find(it->GetNameId()) != types.end())
                    {
                        // This type has already been added.
                        continue;
                    }

                    if (it->IsVirtualType(scene, node))
                    {
                        types.insert(it->GetNameId());
                    }
                }
            }

            void SoftNameTypes::GetVirtualTypeName(AZStd::string& name, Crc32 type)
            {
                for (const AZStd::unique_ptr<VirtualType>& it : m_types)
                {
                    if (it->GetNameId() == type)
                    {
                        name = it->GetName();
                        return;
                    }
                }
            }

            void SoftNameTypes::Clear()
            {
                if (m_connected)
                {
                    BusDisconnect();
                    m_connected = false;
                }
                m_types.clear();
            }

            AZStd::string SoftNameTypes::LoadTextFile(const char* filePath) const
            {
                IO::FileIOBase* fileSystem = IO::FileIOBase::GetInstance();
                if (!fileSystem || !fileSystem->Exists(filePath))
                {
                    return AZStd::string();
                }
                    
                IO::HandleType handle;
                if (!fileSystem->Open(filePath, IO::OpenMode::ModeRead | IO::OpenMode::ModeText, handle))
                {
                    AZ_TracePrintf(Utilities::WarningWindow, "File found but unable to open for reading.");
                    return AZStd::string();
                }
                
                u64 size = 0;
                if (!fileSystem->Size(handle, size))
                {
                    fileSystem->Close(handle);
                    AZ_TracePrintf(Utilities::ErrorWindow, "File opened but unable to retrieve file length.");
                    return AZStd::string();
                }

                if (size == 0)
                {
                    fileSystem->Close(handle);
                    AZ_TracePrintf(Utilities::WarningWindow, "File doesn't have any content.");
                    return AZStd::string();
                }

                AZStd::string result;
                result.resize_no_construct(size);

                if (!fileSystem->Read(handle, result.data(), size))
                {
                    fileSystem->Close(handle);
                    AZ_TracePrintf(Utilities::ErrorWindow, "File opened but unable to read content.");
                    return AZStd::string();
                }

                if (!fileSystem->Close(handle))
                {
                    AZ_TracePrintf(Utilities::WarningWindow, "Unable to close opened file.");
                }
                return result;
            }

            bool SoftNameTypes::ParseJson(const AZStd::string& json)
            {
                rapidjson::Document document;
                rapidjson::ParseResult parseResult = document.Parse(json.c_str());
                if (!parseResult)
                {
                    AZ_TracePrintf(Utilities::ErrorWindow, "Failed to parse content due to '%s' at offset %i.",
                        rapidjson::GetParseError_En(parseResult.Code()), parseResult.Offset());
                    return false;
                }

                if (!document.IsObject())
                {
                    AZ_TracePrintf(Utilities::ErrorWindow, "Expected an object at the root.");
                    return false;
                }

                for (rapidjson::Document::ConstMemberIterator it = document.MemberBegin(); it != document.MemberEnd(); ++it)
                {
                    AZStd::string name = it->name.GetString();
                    if (name == "__comment")
                    {
                        continue;
                    }
                    
                    AZ_TraceContext("JSON element", name);

                    if (!it->value.HasMember("Type"))
                    {
                        AZ_TracePrintf(Utilities::WarningWindow, "Missing element 'Type'.");
                        continue;
                    }

                    const auto& typeValue = it->value["Type"];
                    if (typeValue == "NodeName")
                    {
                        AZStd::unique_ptr<NodeNameBasedVirtualType> virtualType = NodeNameBasedVirtualType::LoadFromJson(AZStd::move(name), it);
                        if (virtualType)
                        {
                            m_types.push_back(AZStd::move(virtualType));
                        }
                    }
                    else if (typeValue == "FileName")
                    {
                        AZStd::unique_ptr<FileNameBasedVirtualType> virtualType = FileNameBasedVirtualType::LoadFromJson(AZStd::move(name), it);
                        if (virtualType)
                        {
                            m_types.push_back(AZStd::move(virtualType));
                        }
                    }
                    else
                    {
                        if (typeValue.IsString())
                        {
                            AZ_TracePrintf(Utilities::WarningWindow, "Unknown value for 'Type': %s.", typeValue.GetString());
                        }
                        else
                        {
                            AZ_TracePrintf(Utilities::WarningWindow, "Unknown value type for 'Type'.");
                        }
                        continue;
                    }
                }

                return true;
            }
        } // SceneData
    } // SceneAPI
} // AZ
