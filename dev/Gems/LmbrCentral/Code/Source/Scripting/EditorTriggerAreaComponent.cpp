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
#include "LmbrCentral_precompiled.h"
#include "EditorTriggerAreaComponent.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>

namespace LmbrCentral
{ //=========================================================================
    namespace ClassConverters
    {
        static bool ConvertOldTriggerAreaComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& node);
    } // namespace ClassConverters

    //=========================================================================
    void EditorTriggerAreaComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<EditorTriggerAreaComponent, EditorComponentBase>()
                ->Version(2, ClassConverters::ConvertOldTriggerAreaComponent)
                ->Field("TriggerArea", &EditorTriggerAreaComponent::m_gameComponent)
                ->Field("RequiredTags", &EditorTriggerAreaComponent::m_requiredTags)
                ->Field("ExcludedTags", &EditorTriggerAreaComponent::m_excludedTags)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<TriggerAreaComponent>("Trigger Area", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Activation")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TriggerAreaComponent::m_triggerOnce, "Trigger once", "If set, the trigger will deactivate after the first trigger event.")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &TriggerAreaComponent::m_activationEntityType, "Activated by", "The types of entities capable of interacting with the area trigger.")
                        ->EnumAttribute(TriggerAreaComponent::ActivationEntityType::AllEntities, "All entities")
                        ->EnumAttribute(TriggerAreaComponent::ActivationEntityType::SpecificEntities, "Specific entities")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &TriggerAreaComponent::OnActivatedByComboBoxChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TriggerAreaComponent::m_specificInteractEntities, "Specific entities", "List of entities that can interact with the trigger.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &TriggerAreaComponent::UseSpecificEntityList)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                edit->Class<EditorTriggerAreaComponent>("Trigger Area", "The Trigger Area component provides generic triggering services by using Shape components as its bounds")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Category, "Scripting")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/TriggerArea.png")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/Trigger.png")
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.aws.amazon.com/lumberyard/latest/userguide/component-triggerarea.html")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorTriggerAreaComponent::m_gameComponent, "Trigger Area", "Activate/deactivate external entities when trigger is entered/exited.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20))
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Tag Filters")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorTriggerAreaComponent::m_requiredTags, "Required tags", "These tags are required on an entity for it to activate this Trigger")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorTriggerAreaComponent::m_excludedTags, "Excluded tags", "The tags exclude an entity from activating this Trigger");
                }
        }
    }

    void EditorTriggerAreaComponent::Activate()
    {
        EditorComponentBase::Activate();
    }

    void EditorTriggerAreaComponent::Deactivate()
    {
        EditorComponentBase::Deactivate();
    }

    void EditorTriggerAreaComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        m_gameComponent.m_requiredTags.clear();
        for (const auto& requiredTag : m_requiredTags)
        {
            m_gameComponent.AddRequiredTagInternal(AZ::Crc32(requiredTag.c_str()));
        }

        m_gameComponent.m_excludedTags.clear();
        for (const auto& excludedTag : m_excludedTags)
        {
            m_gameComponent.AddExcludedTagInternal(AZ::Crc32(excludedTag.c_str()));
        }

        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!context)
        {
            AZ_Error("EditorTriggerAreaComponent", false, "Can't get serialize context from component application.");
            return;
        }

        gameEntity->AddComponent(context->CloneObject(&m_gameComponent));
    }

    static bool ClassConverters::ConvertOldTriggerAreaComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& node)
    {
        if (node.GetVersion() < 2)
        {
            int nodeIdx = node.FindElement(AZ_CRC("TriggerArea", 0x9ec4e63c));

            if (nodeIdx != -1)
            {
                auto& subNode = node.GetSubElement(nodeIdx);
                int nodeIdx = subNode.FindElement(AZ_CRC("ActivatedBy", 0xdf90b5bf));

                if (nodeIdx != -1)
                {
                    auto& activatedByNode = subNode.GetSubElement(nodeIdx);
                    int oldValue = 0;
                    activatedByNode.GetData(oldValue);
                    // Checking against 1 because before version 2 there used to be 4 values in enum
                    // in version2 , there are only 2 valid choices, so for any option greater than 1
                    // we reset the value to 0
                    if (oldValue > 1)
                    {
                        activatedByNode.SetData<int>(context, 0);
                    }
                }
            }
        }
        return true;
    }
} // namespace LmbrCentral
