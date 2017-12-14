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

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include <AzCore/Component/TransformBus.h>

#include <LegacyEntityConversion/LegacyEntityConversionBus.h>

#include "SkyHighlightComponent.h"

namespace Lightning
{
    class SkyHighlightConverter;
    class EditorSkyHighlightComponent;

    /**
    * Configuration for the EditorSkyHighlightComponent
    */
    class EditorSkyHighlightConfiguration :
        public SkyHighlightConfiguration
    {
    public:
        AZ_TYPE_INFO(EditorSkyHighlightConfiguration, "{CB98F2F5-5E14-4E19-A238-F92BCBA876F8}");
        AZ_CLASS_ALLOCATOR(EditorSkyHighlightConfiguration, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* context);

        EditorSkyHighlightComponent* m_component;
    };

    /**
    * Editor version of the SkyHighlightComponent
    */
    class EditorSkyHighlightComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , public SkyHighlightComponentRequestBus::Handler
    {
    public:
        AZ_COMPONENT(EditorSkyHighlightComponent, "{5901B51A-8C81-4BE8-B45B-FA83ED564B84}", AzToolsFramework::Components::EditorComponentBase);
        
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provides);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& requires);
        static void Reflect(AZ::ReflectContext* context);

        EditorSkyHighlightComponent();
        ~EditorSkyHighlightComponent() = default;

        // SkyHighlightComponentRequestBus
        AZ_INLINE void Enable() override { m_config.m_enabled = true; }
        AZ_INLINE void Disable() override { m_config.m_enabled = false; }
        AZ_INLINE void Toggle() override { m_config.m_enabled = !m_config.m_enabled; }
        AZ_INLINE bool IsEnabled() override { return m_config.m_enabled; }

        AZ_INLINE const AZ::Color& GetColor() override { return m_config.m_color; }
        AZ_INLINE void SetColor(const AZ::Color& color) override { m_config.m_color = color; }

        AZ_INLINE float GetColorMultiplier() override { return m_config.m_colorMultiplier; }
        AZ_INLINE void SetColorMultiplier(float colorMultiplier) override { m_config.m_colorMultiplier = colorMultiplier; }

        AZ_INLINE float GetVerticalOffset() override { return m_config.m_verticalOffset; }
        AZ_INLINE void SetVerticalOffset(float verticalOffset) override { m_config.m_verticalOffset = verticalOffset; }

        AZ_INLINE float GetSize() override { return m_config.m_size; }
        AZ_INLINE void SetSize(float size) override { m_config.m_size = size; }

        void BuildGameEntity(AZ::Entity* gameEntity);

    private:
        EditorSkyHighlightConfiguration m_config;
    };

} //namespace Lightning