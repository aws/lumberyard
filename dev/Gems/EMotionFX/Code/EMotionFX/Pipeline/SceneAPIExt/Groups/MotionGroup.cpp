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

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBoneData.h>

#include <SceneAPIExt/Rules/MetaDataRule.h>
#include <SceneAPIExt/Groups/MotionGroup.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Group
        {
            AZ_CLASS_ALLOCATOR_IMPL(MotionGroup, AZ::SystemAllocator, 0)

            MotionGroup::MotionGroup()
                : m_id(AZ::Uuid::CreateRandom())
                , m_startFrame(0)
                , m_endFrame(0)
            {
            }

            void MotionGroup::SetName(const AZStd::string& name)
            {
                m_name = name;
            }

            void MotionGroup::SetName(AZStd::string&& name)
            {
                m_name = AZStd::move(name);
            }

            const AZ::Uuid& MotionGroup::GetId() const
            {
                return m_id;
            }

            void MotionGroup::OverrideId(const AZ::Uuid& id)
            {
                m_id = id;
            }

            AZ::SceneAPI::Containers::RuleContainer& MotionGroup::GetRuleContainer()
            {
                return m_rules;
            }

            const AZ::SceneAPI::Containers::RuleContainer& MotionGroup::GetRuleContainerConst() const
            {
                return m_rules;
            }

            const AZStd::string& MotionGroup::GetSelectedRootBone() const
            {
                return m_selectedRootBone;
            }

            AZ::u32 MotionGroup::GetStartFrame() const
            {
                return m_startFrame;
            }

            AZ::u32 MotionGroup::GetEndFrame() const
            {
                return m_endFrame;
            }

            void MotionGroup::SetSelectedRootBone(const AZStd::string& selectedRootBone)
            {
                m_selectedRootBone = selectedRootBone;
            }

            void MotionGroup::SetStartFrame(AZ::u32 frame)
            {
                m_startFrame = frame;
            }

            void MotionGroup::SetEndFrame(AZ::u32 frame)
            {
                m_endFrame = frame;
            }

            const AZStd::string& MotionGroup::GetName() const
            {
                return m_name;
            }
            
            void MotionGroup::Reflect(AZ::ReflectContext* context)
            {
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                if (!serializeContext)
                {
                    return;
                }

                serializeContext->Class<IMotionGroup, AZ::SceneAPI::DataTypes::IGroup>()->Version(1);

                serializeContext->Class<MotionGroup, IMotionGroup>()->Version(2, VersionConverter)
                    ->Field("name", &MotionGroup::m_name)
                    ->Field("selectedRootBone", &MotionGroup::m_selectedRootBone)
                    ->Field("id", &MotionGroup::m_id)
                    ->Field("startFrame", &MotionGroup::m_startFrame)
                    ->Field("endFrame", &MotionGroup::m_endFrame)
                    ->Field("rules", &MotionGroup::m_rules);

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<MotionGroup>("Motion", "Configure animation data for exporting.")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute("AutoExpand", true)
                            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                        ->DataElement(AZ_CRC("ManifestName", 0x5215b349), &MotionGroup::m_name, "Name motion",
                            "Name for the group. This name will also be used as the name for the generated file.")
                            ->Attribute("FilterType", IMotionGroup::TYPEINFO_Uuid())
                        ->DataElement("NodeListSelection", &MotionGroup::m_selectedRootBone, "Select root bone", "The root bone of the animation that will be exported.")
                            ->Attribute("ClassTypeIdFilter", AZ::SceneAPI::DataTypes::IBoneData::TYPEINFO_Uuid())
                        ->DataElement(AZ::Edit::UIHandlers::Default, &MotionGroup::m_startFrame, "Start frame", "The start frame of the animation that will be exported.")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &MotionGroup::m_endFrame, "End frame", "The end frame of the animation that will be exported.")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &MotionGroup::m_rules, "", "Add or remove rules to fine-tune the export process.")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20));
                }
            }

            bool MotionGroup::VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
            {
                const unsigned int version = classElement.GetVersion();

                bool result = true;
                // Added a uuid "id" as the unique identifier to replace the file name.
                // Setting it to null by default and expecting a behavior to patch this when additional information is available.
                if (version < 2)
                {
                    result = result && classElement.AddElementWithData<AZ::Uuid>(context, "id", AZ::Uuid::CreateNull()) != -1;
                }

                return result;
            }
        }
    }
}