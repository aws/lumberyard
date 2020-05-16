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
#include "NvCloth_precompiled.h"

#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>

#include <SceneAPI/SceneCore/DataTypes/Groups/IMeshGroup.h>
#include <EMotionFX/Pipeline/SceneAPIExt/Groups/IActorGroup.h>

#include <Pipeline/SceneAPIExt/ClothRuleBehavior.h>
#include <Pipeline/SceneAPIExt/ClothRule.h>

namespace NvCloth
{
    namespace Pipeline
    {
        void ClothRuleBehavior::Activate()
        {
            AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler::BusConnect();
            AZ::SceneAPI::Events::AssetImportRequestBus::Handler::BusConnect();
        }

        void ClothRuleBehavior::Deactivate()
        {
            AZ::SceneAPI::Events::AssetImportRequestBus::Handler::BusDisconnect();
            AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler::BusDisconnect();
        }

        void ClothRuleBehavior::Reflect(AZ::ReflectContext* context)
        {
            ClothRule::Reflect(context);

            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<ClothRuleBehavior, BehaviorComponent>()->Version(1);
            }
        }

        void ClothRuleBehavior::GetAvailableModifiers(
            AZ::SceneAPI::Events::ManifestMetaInfo::ModifiersList& modifiers,
            const AZ::SceneAPI::Containers::Scene& scene,
            const AZ::SceneAPI::DataTypes::IManifestObject& target)
        {
            AZ_UNUSED(scene);
            if (target.RTTI_IsTypeOf(AZ::SceneAPI::DataTypes::ISceneNodeGroup::TYPEINFO_Uuid()))
            {
                const AZ::SceneAPI::DataTypes::ISceneNodeGroup* group = azrtti_cast<const AZ::SceneAPI::DataTypes::ISceneNodeGroup*>(&target);
                if (IsValidGroupType(*group))
                {
                    modifiers.push_back(ClothRule::TYPEINFO_Uuid());
                }
            }
        }

        void ClothRuleBehavior::InitializeObject(
            const AZ::SceneAPI::Containers::Scene& scene,
            AZ::SceneAPI::DataTypes::IManifestObject& target)
        {
            // When a cloth rule is created in the FBX Editor Settings...
            if (target.RTTI_IsTypeOf(ClothRule::TYPEINFO_Uuid()))
            {
                ClothRule* clothRule = azrtti_cast<ClothRule*>(&target);

                // Set default values
                clothRule->SetMeshNodeName(ClothRule::m_defaultChooseNodeName);
                clothRule->SetVertexColorStreamName(ClothRule::m_defaultInverseMassString);
            }
        }

        AZ::SceneAPI::Events::ProcessingResult ClothRuleBehavior::UpdateManifest(
            AZ::SceneAPI::Containers::Scene& scene,
            ManifestAction action,
            RequestingApplication requester)
        {
            AZ_UNUSED(requester);
            // When the manifest is updated let's check the content is still valid for cloth rules
            if (action == ManifestAction::Update)
            {
                bool updated = UpdateClothRules(scene);
                return updated ? AZ::SceneAPI::Events::ProcessingResult::Success : AZ::SceneAPI::Events::ProcessingResult::Ignored;
            }
            else
            {
                return AZ::SceneAPI::Events::ProcessingResult::Ignored;
            }
        }

        bool ClothRuleBehavior::IsValidGroupType(const AZ::SceneAPI::DataTypes::ISceneNodeGroup& group) const
        {
            // Cloth rules are available in Mesh and Actor Groups
            return group.RTTI_IsTypeOf(AZ::SceneAPI::DataTypes::IMeshGroup::TYPEINFO_Uuid())
                || group.RTTI_IsTypeOf(EMotionFX::Pipeline::Group::IActorGroup::TYPEINFO_Uuid());
        }

        bool ClothRuleBehavior::UpdateClothRules(AZ::SceneAPI::Containers::Scene& scene)
        {
            bool rulesUpdated = false;

            auto& manifest = scene.GetManifest();
            auto valueStorage = manifest.GetValueStorage();
            auto view = AZ::SceneAPI::Containers::MakeDerivedFilterView<AZ::SceneAPI::DataTypes::ISceneNodeGroup>(valueStorage);

            // For each scene group...
            for (auto& group : view)
            {
                bool isValidGroupType = IsValidGroupType(group);

                AZStd::vector<size_t> rulesToRemove;

                auto& groupRules = group.GetRuleContainer();
                for (size_t index = 0; index < groupRules.GetRuleCount(); ++index)
                {
                    ClothRule* clothRule = azrtti_cast<ClothRule*>(groupRules.GetRule(index).get());
                    if (clothRule)
                    {
                        if (isValidGroupType)
                        {
                            rulesUpdated = UpdateClothRule(scene, group, *clothRule) || rulesUpdated;
                        }
                        else
                        {
                            // Cloth rule found in a group that shouldn't have cloth rules, add for removal.
                            rulesToRemove.push_back(index);
                            rulesUpdated = true;
                        }
                    }
                }

                // Remove in reversed order, as otherwise the indices will be wrong. For example if we remove index 3, then index 6 would really be 5 afterwards.
                // By doing this in reversed order we remove items at the end of the list first so it won't impact the indices of previous ones.
                for (AZStd::vector<size_t>::reverse_iterator it = rulesToRemove.rbegin(); it != rulesToRemove.rend(); ++it)
                { 
                    groupRules.RemoveRule(*it);
                }
            }

            return rulesUpdated;
        }

        bool ClothRuleBehavior::UpdateClothRule(const AZ::SceneAPI::Containers::Scene& scene, const AZ::SceneAPI::DataTypes::ISceneNodeGroup& group, ClothRule& clothRule)
        {
            bool ruleUpdated = false;

            if (clothRule.GetMeshNodeName() != ClothRule::m_defaultChooseNodeName)
            {
                bool foundMeshNode = false;
                const AZStd::string& meshNodemName = clothRule.GetMeshNodeName();

                const auto& selectedNodesList = group.GetSceneNodeSelectionList();
                for (size_t i = 0; i < selectedNodesList.GetSelectedNodeCount(); ++i)
                {
                    if (meshNodemName == selectedNodesList.GetSelectedNode(i))
                    {
                        foundMeshNode = true;
                        break;
                    }
                }

                // Mesh node selected in the cloth rule is not part of the list of selected nodes anymore, set the default value.
                if (!foundMeshNode)
                {
                    clothRule.SetMeshNodeName(ClothRule::m_defaultChooseNodeName);
                    ruleUpdated = true;
                }
            }

            if (clothRule.GetVertexColorStreamName() != ClothRule::m_defaultInverseMassString)
            {
                const AZStd::string& vertexColorStreamName = clothRule.GetVertexColorStreamName();

                const auto& graph = scene.GetGraph();
                auto graphNames = graph.GetNameStorage();

                auto found = AZStd::find_if(graphNames.cbegin(), graphNames.cend(), 
                    [&vertexColorStreamName](const AZ::SceneAPI::Containers::SceneGraph::NameStorageType& graphName)
                    {
                        return vertexColorStreamName == graphName.GetName();
                    });

                // Vertex Color Stream selected in the cloth rule doesn't exist anymore, set the default value.
                if (found == graphNames.cend())
                {
                    clothRule.SetVertexColorStreamName(ClothRule::m_defaultInverseMassString);
                    ruleUpdated = true;
                }
            }

            return ruleUpdated;
        }
        
    } // namespace Pipeline
} // namespace NvCloth
