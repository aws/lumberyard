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
#ifdef MOTIONCANVAS_GEM_ENABLED

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBoneData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneData/EMotionFX/Rules/MetaDataRule.h>
#include <SceneAPI/SceneData/Groups/ActorGroup.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneData
        {
            AZ_CLASS_ALLOCATOR_IMPL(ActorGroup, SystemAllocator, 0)

            ActorGroup::ActorGroup()
                : m_loadMorphTargets(false)
                , m_autoCreateTrajectoryNode(true)
            {
            }

            const AZStd::string& ActorGroup::GetName() const
            {
                return m_name;
            }

            void ActorGroup::SetName(const AZStd::string& name)
            {
                m_name = name;
            }

            void ActorGroup::SetName(AZStd::string&& name)
            {
                m_name = AZStd::move(name);
            }

            Containers::RuleContainer& ActorGroup::GetRuleContainer()
            {
                return m_rules;
            }

            const Containers::RuleContainer& ActorGroup::GetRuleContainerConst() const
            {
                return m_rules;
            }
            
            DataTypes::ISceneNodeSelectionList& ActorGroup::GetSceneNodeSelectionList()
            {
                return m_nodeSelectionList;
            }

            const DataTypes::ISceneNodeSelectionList& ActorGroup::GetSceneNodeSelectionList() const
            {
                return m_nodeSelectionList;
            }

            const AZStd::string & ActorGroup::GetSelectedRootBone() const
            {
                return m_selectedRootBone;
            }

            bool ActorGroup::GetLoadMorphTargets() const
            {
                return m_loadMorphTargets;
            }

            bool ActorGroup::GetAutoCreateTrajectoryNode() const
            {
                return m_autoCreateTrajectoryNode;
            }

            void ActorGroup::SetSelectedRootBone(const AZStd::string & selectedRootBone)
            {
                m_selectedRootBone = selectedRootBone;
            }

            void ActorGroup::SetLoadMorphTargets(bool loadMorphTargets)
            {
                m_loadMorphTargets = loadMorphTargets;
            }

            void ActorGroup::SetAutoCreateTrajectoryNode(bool autoCreateTrajectoryNode)
            {
                m_autoCreateTrajectoryNode = autoCreateTrajectoryNode;
            }

            void ActorGroup::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (!serializeContext || serializeContext->FindClassData(ActorGroup::TYPEINFO_Uuid()))
                {
                    return;
                }

                serializeContext->Class<ActorGroup, DataTypes::IActorGroup>()->Version(1)
                    ->Field("name", &ActorGroup::m_name)
                    ->Field("selectedRootBone", &ActorGroup::m_selectedRootBone)
                    ->Field("nodeSelectionList", &ActorGroup::m_nodeSelectionList)
                    ->Field("loadMorphTargets", &ActorGroup::m_loadMorphTargets)
                    ->Field("autoCreateTrajectoryNode", &ActorGroup::m_autoCreateTrajectoryNode)
                    ->Field("rules", &ActorGroup::m_rules);

                EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<ActorGroup>("Actor group", "Configure actor data exporting.")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                            ->Attribute("AutoExpand", true)
                            ->Attribute(Edit::Attributes::NameLabelOverride, "")
                        ->DataElement(AZ_CRC("ManifestName", 0x5215b349), &ActorGroup::m_name, "Name actor",
                            "Name for the group. This name will also be used as the name for the generated file.")
                            ->Attribute("FilterType", DataTypes::IActorGroup::TYPEINFO_Uuid())
                        ->DataElement("NodeListSelection", &ActorGroup::m_selectedRootBone, "Select root bone", "The root bone of the animation that will be exported.")
                            ->Attribute("ClassTypeIdFilter", AZ::SceneAPI::DataTypes::IBoneData::TYPEINFO_Uuid())
                        ->DataElement(Edit::UIHandlers::Default, &ActorGroup::m_nodeSelectionList, "Select meshes", "Select the meshes to be included in the actor.")
                            ->Attribute("FilterName", "meshes")
                            ->Attribute("FilterType", DataTypes::IMeshData::TYPEINFO_Uuid())
                        ->DataElement(Edit::UIHandlers::Default, &ActorGroup::m_loadMorphTargets, "Load morph targets", "")
                        ->DataElement(Edit::UIHandlers::Default, &ActorGroup::m_autoCreateTrajectoryNode, "Auto create trajectory node", "")
                        ->DataElement(Edit::UIHandlers::Default, &ActorGroup::m_rules, "", "Add or remove rules to fine-tune the export process.")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20));
                }
            }

        }
    }
}
#endif //MOTIONCANVAS_GEM_ENABLED