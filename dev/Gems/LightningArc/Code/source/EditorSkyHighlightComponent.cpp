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

#include "LightningArc_precompiled.h"

#include <AzCore/RTTI/BehaviorContext.h>

#include "EditorSkyHighlightComponent.h"

namespace Lightning
{
    void EditorSkyHighlightConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorSkyHighlightConfiguration, SkyHighlightConfiguration>()
                ->Version(1);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorSkyHighlightConfiguration>("Sky Highlight Configuration", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editContext->Class<SkyHighlightConfiguration>("Sky Highlight Configuration", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &SkyHighlightConfiguration::m_enabled, "Enabled", "Sets if the sky highlight is enabled.")

                    ->DataElement(AZ::Edit::UIHandlers::Color, &SkyHighlightConfiguration::m_color, "Color", "Color of the sky highlight.")

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &SkyHighlightConfiguration::m_colorMultiplier, "Color Multiplier", "Modifier for the color intensity.")
                        ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &SkyHighlightConfiguration::m_verticalOffset, "Vertical Offset", "An offset of the sky highlight on the global Z axis.")
                        ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &SkyHighlightConfiguration::m_size, "Size", "Size of the sky highlight.")
                        ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                        ->Attribute(AZ::Edit::Attributes::Min, SkyHighlightComponent::MinSkyHighlightSize)
                    ;
            }
        }
    }

    void EditorSkyHighlightComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType & provides)
    {
        provides.push_back(AZ_CRC("SkyHighlightService"));
    }

    void EditorSkyHighlightComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType & requires)
    {
        requires.push_back(AZ_CRC("TransformService"));
    }

    void EditorSkyHighlightComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorSkyHighlightComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(1)
                ->Field("m_config", &EditorSkyHighlightComponent::m_config)
                ;
            
            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorSkyHighlightComponent>("SkyHighlight", "Produces a bright sky highlight effect.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Environment")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/SkyHighlight.png")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/SkyHighlight.png")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, "http://docs.aws.amazon.com/console/lumberyard/userguide/sky-highlight-component")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorSkyHighlightComponent::m_config, "m_config", "No Description")
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<EditorSkyHighlightComponent>()->RequestBus("SkyHighlightRequestBus");
        }

        EditorSkyHighlightConfiguration::Reflect(context);
    }

    EditorSkyHighlightComponent::EditorSkyHighlightComponent()
    {
        m_config.m_component = this;
    }

    void EditorSkyHighlightComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        SkyHighlightComponent* component = gameEntity->CreateComponent<SkyHighlightComponent>(&m_config);
    }

} //namespace Lightning