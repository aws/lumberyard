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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/FilterIterator.h>
#include <SceneAPI/SceneCore/Events/GraphMetaInfoBus.h>
#include <SceneAPI/SceneCore/Utilities/SceneGraphSelector.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IMeshGroup.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IMaterialRule.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/ITouchBendingRule.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneData/Rules/PhysicsRule.h>
#include <SceneAPI/SceneData/Rules/MaterialRule.h>
#include <SceneAPI/SceneData/Behaviors/PhysicsRuleBehavior.h>
#include <SceneAPI/SceneData/ManifestBase/SceneNodeSelectionList.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneData
        {
            void PhysicsRuleBehavior::Activate()
            {
                Events::ManifestMetaInfoBus::Handler::BusConnect();
                Events::AssetImportRequestBus::Handler::BusConnect();
                Events::GraphMetaInfoBus::Handler::BusConnect();
            }

            void PhysicsRuleBehavior::Deactivate()
            {
                Events::GraphMetaInfoBus::Handler::BusDisconnect();
                Events::AssetImportRequestBus::Handler::BusDisconnect();
                Events::ManifestMetaInfoBus::Handler::BusDisconnect();
            }

            void PhysicsRuleBehavior::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<PhysicsRuleBehavior, BehaviorComponent>()->Version(1);
                }
            }

            void PhysicsRuleBehavior::InitializeObject(const Containers::Scene& scene, DataTypes::IManifestObject& target)
            {
                if (target.RTTI_IsTypeOf(DataTypes::IMeshGroup::TYPEINFO_Uuid()))
                {
                    //It is only allowed to add PhysicsRule when ITouchBendingRule is not present.
                    const SceneAPI::DataTypes::IMeshGroup* group = azrtti_cast<const SceneAPI::DataTypes::IMeshGroup*>(&target);
                    const SceneAPI::Containers::RuleContainer& rules = group->GetRuleContainerConst();
                    if (rules.ContainsRuleOfType<SceneAPI::DataTypes::ITouchBendingRule>())
                    {
                        return;
                    }

                    SceneData::SceneNodeSelectionList selection;
                    size_t physicsCount = SelectPhysicsMeshes(scene, selection);

                    if (physicsCount > 0)
                    {
                        AZStd::shared_ptr<SceneData::PhysicsRule> physicsRule = AZStd::make_shared<SceneData::PhysicsRule>();
                        selection.CopyTo(physicsRule->GetNodeSelectionList());
                        
                        DataTypes::IMeshGroup* meshGroup = azrtti_cast<DataTypes::IMeshGroup*>(&target);
                        
                        Containers::RuleContainer& meshRules = meshGroup->GetRuleContainer();
                        meshRules.AddRule(AZStd::move(physicsRule));

                        if (!meshRules.ContainsRuleOfType<DataTypes::IMaterialRule>())
                        {
                            AZStd::shared_ptr<SceneData::MaterialRule> materialRule = AZStd::make_shared<SceneData::MaterialRule>();
                            EBUS_EVENT(Events::ManifestMetaInfoBus, InitializeObject, scene, *materialRule);
                            meshRules.AddRule(AZStd::move(materialRule));
                        }
                    }
                }
                else if (target.RTTI_IsTypeOf(SceneData::PhysicsRule::TYPEINFO_Uuid()))
                {
                    SceneData::PhysicsRule* rule = azrtti_cast<SceneData::PhysicsRule*>(&target);
                    SelectPhysicsMeshes(scene, rule->GetSceneNodeSelectionList());
                }
            }

            size_t PhysicsRuleBehavior::SelectPhysicsMeshes(const Containers::Scene& scene, DataTypes::ISceneNodeSelectionList& selection) const
            {
                Utilities::SceneGraphSelector::SelectAll(scene.GetGraph(), selection);
                size_t physicsMeshCount = 0;

                const Containers::SceneGraph& graph = scene.GetGraph();
                
                auto contentStorage = graph.GetContentStorage();
                auto nameStorage = graph.GetNameStorage();                
                auto keyValueView = Containers::Views::MakePairView(nameStorage, contentStorage);
                auto filteredView = Containers::Views::MakeFilterView(keyValueView, Containers::DerivedTypeFilter<DataTypes::IMeshData>());
                for (auto it = filteredView.begin(); it != filteredView.end(); ++it)
                {
                    AZStd::set<Crc32> types;
                    auto keyValueIterator = it.GetBaseIterator();
                    Containers::SceneGraph::NodeIndex index = graph.ConvertToNodeIndex(keyValueIterator.GetFirstIterator());
                    EBUS_EVENT(Events::GraphMetaInfoBus, GetVirtualTypes, types, scene, index);
                    if (types.find(AZ_CRC("PhysicsMesh", 0xc75d4ff1)) == types.end() || 
                        types.find(Events::GraphMetaInfo::s_ignoreVirtualType) != types.end())
                    {
                        selection.RemoveSelectedNode(it->first.GetPath());
                    }
                    else
                    {
                        physicsMeshCount++;
                    }
                }
                return physicsMeshCount;
            }

            Events::ProcessingResult PhysicsRuleBehavior::UpdateManifest(Containers::Scene& scene, ManifestAction action,
                RequestingApplication /*requester*/)
            {
                if (action == ManifestAction::Update)
                {
                    UpdatePhysicsRules(scene);
                    return Events::ProcessingResult::Success;
                }
                else
                {
                    return Events::ProcessingResult::Ignored;
                }
            }

            void PhysicsRuleBehavior::GetVirtualTypeName(AZStd::string& name, Crc32 type)
            {
                if (type == AZ_CRC("PhysicsMesh", 0xc75d4ff1))
                {
                    name = "PhysicsMesh";
                }
            }

            void PhysicsRuleBehavior::GetAllVirtualTypes(AZStd::set<Crc32>& types)
            {
                if (types.find(AZ_CRC("PhysicsMesh", 0xc75d4ff1)) == types.end())
                {
                    types.insert(AZ_CRC("PhysicsMesh", 0xc75d4ff1));
                }
            }

            void PhysicsRuleBehavior::UpdatePhysicsRules(Containers::Scene& scene) const
            {
                Containers::SceneManifest& manifest = scene.GetManifest();
                auto valueStorage = manifest.GetValueStorage();
                auto view = Containers::MakeDerivedFilterView<DataTypes::IMeshGroup>(valueStorage);
                for (DataTypes::IMeshGroup& group : view)
                {
                    AZ_TraceContext("Mesh group", group.GetName());

                    const Containers::RuleContainer& rules = group.GetRuleContainerConst();
                    const size_t ruleCount = rules.GetRuleCount();
                    for (size_t index = 0; index < ruleCount; ++index)
                    {
                        SceneData::PhysicsRule* rule = azrtti_cast<SceneData::PhysicsRule*>(rules.GetRule(index).get());
                        if (rule)
                        {
                            Utilities::SceneGraphSelector::UpdateNodeSelection(scene.GetGraph(), rule->GetSceneNodeSelectionList());
                        }
                    }
                }
            }

            void PhysicsRuleBehavior::GetAvailableModifiers(SceneAPI::Events::ManifestMetaInfo::ModifiersList& modifiers,
                const SceneAPI::Containers::Scene& /*scene*/,
                const SceneAPI::DataTypes::IManifestObject& target)
            {
                if (!target.RTTI_IsTypeOf(SceneAPI::DataTypes::IMeshGroup::TYPEINFO_Uuid()))
                {
                    return;
                }

                //When the "Add Modifier" Button in the FBX Settings Editor is clicked,
                //the expectation is that only those Modifiers(aka Rules) that have not been added to the mesh group
                //should be displayed for further selection.
                const SceneAPI::DataTypes::IMeshGroup* group = azrtti_cast<const SceneAPI::DataTypes::IMeshGroup*>(&target);
                const SceneAPI::Containers::RuleContainer& rules = group->GetRuleContainerConst();

                const size_t ruleCount = rules.GetRuleCount();
                for (size_t i = 0; i < ruleCount; ++i)
                {
                    AZStd::shared_ptr< SceneAPI::DataTypes::IRule> rule = rules.GetRule(i);
                    if (rule->RTTI_IsTypeOf(SceneAPI::DataTypes::ITouchBendingRule::TYPEINFO_Uuid()))
                    {
                        //IPhysicsRule is already added into the MeshGroup.
                        return;
                    }
                    if (rule->RTTI_IsTypeOf(SceneAPI::DataTypes::IPhysicsRule::TYPEINFO_Uuid()))
                    {
                        //If ITouchBendingRule is already present, then adding PhysicsRule is not an option.
                        return;
                    }
                }
                modifiers.push_back(PhysicsRule::TYPEINFO_Uuid());
            }
        } // namespace SceneData
    } // namespace SceneAPI
} // namespace AZ
