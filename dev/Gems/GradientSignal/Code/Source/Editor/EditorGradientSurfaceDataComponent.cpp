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

#include "GradientSignal_precompiled.h"
#include "EditorGradientSurfaceDataComponent.h"
#include <AzCore/Serialization/Utils.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>

namespace GradientSignal
{
    void EditorGradientSurfaceDataComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClassType::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<EditorGradientSurfaceDataComponent, BaseClassType>()
                ->Version(1, &LmbrCentral::EditorWrappedComponentBaseVersionConverter<typename BaseClassType::WrappedComponentType, typename BaseClassType::WrappedConfigType,1>)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<EditorGradientSurfaceDataComponent>(
                    s_componentName, s_componentDescription)
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Icon, s_icon)
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, s_viewportIcon)
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, s_helpUrl)
                    ->Attribute(AZ::Edit::Attributes::Category, s_categoryName)
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Preview")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->UIElement("GradientPreviewer", "Previewer")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                    ->Attribute(AZ_CRC("GradientEntity", 0xe8531817), &EditorGradientSurfaceDataComponent::GetGradientEntityId)
                    ->Attribute(AZ_CRC("GradientFilter", 0x99bf0362), &EditorGradientSurfaceDataComponent::GetFilterFunc)
                    ->ClassElement(AZ::Edit::ClassElements::Group, "")
                    ;
            }
        }
    }

    void EditorGradientSurfaceDataComponent::Activate()
    {
        m_gradientEntityId = GetEntityId();
        
        BaseClassType::Activate();

        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
    }

    void EditorGradientSurfaceDataComponent::Deactivate()
    {
        // If the preview shouldn't be active, use an invalid entityId
        m_gradientEntityId = AZ::EntityId();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        
        BaseClassType::Deactivate();
    }

    AZ::u32 EditorGradientSurfaceDataComponent::ConfigurationChanged()
    {
        auto result = BaseClassType::ConfigurationChanged();

        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);

        return result;
    }

    AZ::EntityId EditorGradientSurfaceDataComponent::GetGradientEntityId() const
    {
        return m_gradientEntityId;
    }

    AZStd::function<float(float)> EditorGradientSurfaceDataComponent::GetFilterFunc()
    {
        return [this](float sampleValue)
        {
            return ((sampleValue >= m_configuration.m_thresholdMin) && (sampleValue <= m_configuration.m_thresholdMax)) ? sampleValue : 0.0f;
        };
    }
}