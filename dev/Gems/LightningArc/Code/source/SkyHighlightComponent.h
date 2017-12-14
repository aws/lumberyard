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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>

#include "Lightning/SkyHighlightComponentBus.h"

namespace Lightning
{
    /**
    * Configuration for the SkyHighlightComponent
    */
    class SkyHighlightConfiguration
    {
    public:
        AZ_TYPE_INFO(SkyHighlightConfiguration, "{C9AC26D5-9602-40F4-9C18-1815F01CAFFD}");
        AZ_CLASS_ALLOCATOR(SkyHighlightConfiguration, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* context);

        bool m_enabled = true;

        AZ::Color m_color = AZ::Color(0.8f, 0.8f, 1.0f, 1.0f); //A very pale blue
        float m_colorMultiplier = 1.0f;
        float m_verticalOffset = 0.0f;
        float m_size = 10.0f;
    };

    /**
    * A component for creating and controlling sky highlight effects
    */
    class SkyHighlightComponent
        : public AZ::Component
        , private SkyHighlightComponentRequestBus::Handler
        , public AZ::TransformNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(SkyHighlightComponent, "{F52A3699-A195-40B5-B39B-B0F9A3C5C568}", AZ::Component);
        
		const static float MinSkyHighlightSize;

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provides);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& requires);
        static void Reflect(AZ::ReflectContext* context);

        SkyHighlightComponent() {};
        explicit SkyHighlightComponent(SkyHighlightConfiguration* config)
        {
            m_config = *config;
        }
        ~SkyHighlightComponent() = default;

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // TransformNotificationBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // SkyHighlightComponentRequestBus
        void Enable() override;
        void Disable() override;
        void Toggle() override;
        bool IsEnabled() override { return m_config.m_enabled; }

        const AZ::Color& GetColor() override { return m_config.m_color; }
        void SetColor(const AZ::Color& color) override;

        float GetColorMultiplier() override { return m_config.m_colorMultiplier; }
        void SetColorMultiplier(float colorMultiplier) override;

        float GetVerticalOffset() override { return m_config.m_verticalOffset; }
        void SetVerticalOffset(float verticalOffset) override;

        float GetSize() override { return m_config.m_size; }
        void SetSize(float lightAttenuation) override;

    private:
        // Reflected Data
        SkyHighlightConfiguration m_config;

        //Unreflected but stored data
        AZ::Vector3 m_worldPos; 

        /**
        * Send SkyHighlight parameters to the 3D engine so that it renders
        */
        void UpdateSkyHighlight();

        /**
        * Turns off any SkyHighlight effect
        *
        * This sends zeros to the 3D engine's SkyHighlight params to turn off the effect
        */
        void TurnOffHighlight();
    };
} //namespace Lightning