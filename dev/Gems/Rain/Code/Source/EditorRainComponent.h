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
        , public AZ::TickBus::Handler
        , public Rain::RainComponentRequestBus::Handler
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

        // TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

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

        // RainComponentRequestBus::Handler
        void Enable() override;
        void Disable() override;
        void Toggle() override;

        bool IsEnabled() override;
        void SetEnabled(bool enabled) override;

        /*
            Note: If a user needs to call many setters it's best to not use the
            individual setters. Instead call GetRainOptions, modify the object
            and then call SetRainOptions. That way you're not calling UpdateRain
            more than once.
        */

        bool GetUseVisAreas() override;
        void SetUseVisAreas(bool useVisAreas) override;

        bool GetDisableOcclusion() override;
        void SetDisableOcclusion(bool disableOcclusion) override;

        float GetRadius() override;
        void SetRadius(float radius) override;

        float GetAmount() override;
        void SetAmount(float amount) override;

        float GetDiffuseDarkening() override;
        void SetDiffuseDarkening(float diffuseDarkening) override;

        float GetRainDropsAmount() override;
        void SetRainDropsAmount(float rainDropsAmount) override;

        float GetRainDropsSpeed() override;
        void SetRainDropsSpeed(float rainDropsSpeed) override;

        float GetRainDropsLighting() override;
        void SetRainDropsLighting(float rainDropsLighting) override;

        float GetPuddlesAmount() override;
        void SetPuddlesAmount(float puddlesAmount) override;

        float GetPuddlesMaskAmount() override;
        void SetPuddlesMaskAmount(float puddlesMaskAmount) override;

        float GetPuddlesRippleAmount() override;
        void SetPuddlesRippleAmount(float puddlesRippleAmount) override;

        float GetSplashesAmount() override;
        void SetSplashesAmount(float splashesAmount) override;

        RainOptions GetRainOptions() override;
        void SetRainOptions(RainOptions rainOptions) override;

        /**
         * Sends this component's rain parameters to the engine
         *
         * If the component is disabled or the "amount" of this rain object is 0
         * the engine will attempt to disable rain. Rain can be re-enabled if another
         * rain component updates and overrides that setting.
         */
        void UpdateRain() override;

    private:
        //Reflected Data
        bool m_enabled = true;
        bool m_renderInEditMode = false;
        RainOptions m_rainOptions;

        //Unreflected Data
        AZ::Vector3 m_currentWorldPos;
        AZ::Transform m_currentWorldTransform;
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
