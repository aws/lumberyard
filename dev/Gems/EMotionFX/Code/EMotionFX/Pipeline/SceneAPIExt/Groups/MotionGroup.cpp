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

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBoneData.h>
#include <SceneAPI/SceneData/EMotionFX/Rules/MetaDataRule.h>
#include <SceneAPI/SceneData/Groups/EFXMotionGroup.h>


namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneData
        {
            AZ_CLASS_ALLOCATOR_IMPL(EFXMotionGroup, SystemAllocator, 0)

            EFXMotionGroup::EFXMotionGroup()
                : m_startFrame(0)
                , m_endFrame(0)
            {
            }

            void EFXMotionGroup::SetName(const AZStd::string& name)
            {
                m_name = name;
            }

            void EFXMotionGroup::SetName(AZStd::string&& name)
            {
                m_name = AZStd::move(name);
            }

            Containers::RuleContainer& EFXMotionGroup::GetRuleContainer()
            {
                return m_rules;
            }

            const Containers::RuleContainer& EFXMotionGroup::GetRuleContainerConst() const
            {
                return m_rules;
            }

            const AZStd::string& EFXMotionGroup::GetSelectedRootBone() const
            {
                return m_selectedRootBone;
            }

            uint32_t EFXMotionGroup::GetStartFrame() const
            {
                return m_startFrame;
            }

            uint32_t EFXMotionGroup::GetEndFrame() const
            {
                return m_endFrame;
            }

            void EFXMotionGroup::SetSelectedRootBone(const AZStd::string& selectedRootBone)
            {
                m_selectedRootBone = selectedRootBone;
            }

            void EFXMotionGroup::SetStartFrame(uint32_t frame)
            {
                m_startFrame = frame;
            }

            void EFXMotionGroup::SetEndFrame(uint32_t frame)
            {
                m_endFrame = frame;
            }

            const AZStd::string& EFXMotionGroup::GetName() const
            {
                return m_name;
            }
            
            void EFXMotionGroup::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (!serializeContext || serializeContext->FindClassData(EFXMotionGroup::TYPEINFO_Uuid()))
                {
                    return;
                }

                serializeContext->Class<EFXMotionGroup, DataTypes::IEFXMotionGroup>()->Version(1)
                    ->Field("name", &EFXMotionGroup::m_name)
                    ->Field("selectedRootBone", &EFXMotionGroup::m_selectedRootBone)
                    ->Field("startFrame", &EFXMotionGroup::m_startFrame)
                    ->Field("endFrame", &EFXMotionGroup::m_endFrame)
                    ->Field("rules", &EFXMotionGroup::m_rules);

                EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<EFXMotionGroup>("Motion", "Configure animation data for exporting.")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                            ->Attribute("AutoExpand", true)
                            ->Attribute(Edit::Attributes::NameLabelOverride, "")
                        ->DataElement(AZ_CRC("ManifestName", 0x5215b349), &EFXMotionGroup::m_name, "Name motion",
                            "Name for the group. This name will also be used as the name for the generated file.")
                            ->Attribute("FilterType", DataTypes::IEFXMotionGroup::TYPEINFO_Uuid())
                        ->DataElement("NodeListSelection", &EFXMotionGroup::m_selectedRootBone, "Select root bone", "The root bone of the animation that will be exported.")
                            ->Attribute("ClassTypeIdFilter", AZ::SceneAPI::DataTypes::IBoneData::TYPEINFO_Uuid())
                        ->DataElement(Edit::UIHandlers::Default, &EFXMotionGroup::m_startFrame, "Start frame", "The start frame of the animation that will be exported.")
                        ->DataElement(Edit::UIHandlers::Default, &EFXMotionGroup::m_endFrame, "End frame", "The end frame of the animation that will be exported.")
                        ->DataElement(Edit::UIHandlers::Default, &EFXMotionGroup::m_rules, "", "Add or remove rules to fine-tune the export process.")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20));
                }
            }

        }
    }
}
#endif //MOTIONCANVAS_GEM_ENABLED