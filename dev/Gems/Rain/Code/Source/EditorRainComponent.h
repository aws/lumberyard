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

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>

#include <AzFramework/Entity/EntityDebugDisplayBus.h>

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/TickBus.h>

#include <LegacyEntityConversion/LegacyEntityConversionBus.h>

#include "RainComponent.h"

namespace Rain
{
    /**
     * Editor-side Rain component
     *
     * One of the main differences between the EditorRainComponent and the 
     * RainComponent is that the EditorRainComponent will send rain updates
     * every tick on the Tick bus. This is because EditorRainComponent instances
     * will come in and out and we want to make sure that the latest info is always
     * up to date. The RainComponent will only send new RainOptions to the 3DEngine
     * when it's created, when it changes, and when it's destroyed. 
     */
    class EditorRainComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , public AZ::TransformNotificationBus::Handler
        , public AzFramework::EntityDebugDisplayEventBus::Handler
    {
    public:
        friend class RainConverter;

        AZ_COMPONENT(EditorRainComponent, "{635A02E2-5DD8-4E09-8729-6EC4F674BC91}", AzToolsFramework::Components::EditorComponentBase);
    
        static void Reflect(AZ::ReflectContext* context);

        ~EditorRainComponent() override = default;

        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;

        // TransformBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;

        // AzFrameowrk::EntityDebugDisplayEventBus
        void DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("RainService"));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("TransformService"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("RainService"));
        }

    private:
        //Reflected Data
        bool m_enabled = true;
        RainOptions m_rainOptions;

        //Unreflected Data
        AZ::Vector3 m_currentWorldPos;
    };

    class RainConverter : public AZ::LegacyConversion::LegacyConversionEventBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(RainConverter, AZ::SystemAllocator, 0);

        RainConverter() {}

        // ------------------- LegacyConversionEventBus::Handler -----------------------------
        AZ::LegacyConversion::LegacyConversionResult ConvertEntity(CBaseObject* pEntityToConvert) override;
        bool BeforeConversionBegins() override;
        bool AfterConversionEnds() override;
        // END ----------------LegacyConversionEventBus::Handler ------------------------------
    };
}