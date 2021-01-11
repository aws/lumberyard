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

#include "SnowComponent.h"

namespace Snow
{
    /**
     * Editor-side Snow component
     *
     * One of the main differences between the EditorSnowComponent and the 
     * SnowComponent is that the EditorSnowComponent will send snow updates
     * every tick on the Tick bus. This is because EditorSnowComponent instances
     * will come in and out and we want to make sure that the latest info is always
     * up to date. The SnowComponent will only send new SnowOptions to the 3DEngine
     * when it's created, when it changes, and when it's destroyed. 
     */
    class EditorSnowComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , private AZ::TransformNotificationBus::Handler
        , public AzFramework::EntityDebugDisplayEventBus::Handler
        , private AZ::TickBus::Handler
        , private Snow::SnowComponentRequestBus::Handler
    {
    public:
        friend class SnowConverter;

        AZ_COMPONENT(EditorSnowComponent, "{46166250-F004-4049-AAD2-35104721965F}", AzToolsFramework::Components::EditorComponentBase);
    
        static void Reflect(AZ::ReflectContext* context);

        ~EditorSnowComponent() override = default;

        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;

        // TransformBus::Handler
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;

        // EntityDebugDisplayEventBus
        void DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;

        // TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("SnowService"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("SnowService"));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("TransformService"));
        }

        // SnowComponentRequestBus::Handler
        void Enable() override;
        void Disable() override;
        void Toggle() override;

        bool IsEnabled() override;
        void SetEnabled(bool enabled) override;

        /*
            Note: If a user needs to call many setters it's best to not use the
            individual setters. Instead call GetSnowOptions, modify the object
            and then call SetSnowOptions. That way you're not calling UpdateSnow
            more than once.
        */

        float GetRadius();
        void SetRadius(float disableOcclusion) override;

        float GetSnowAmount();
        void SetSnowAmount(float snowAmount) override;

        float GetFrostAmount();
        void SetFrostAmount(float frostAmount) override;

        float GetSurfaceFreezing();
        void SetSurfaceFreezing(float surfaceFreezing) override;

        AZ::u32 GetSnowFlakeCount();
        void SetSnowFlakeCount(AZ::u32 snowFlakeCount) override;

        float GetSnowFlakeSize() override;
        void SetSnowFlakeSize(float snowFlakeSize) override;

        float GetSnowFallBrightness() override;
        void SetSnowFallBrightness(float snowFallBrightness) override;

        float GetSnowFallGravityScale() override;
        void SetSnowFallGravityScale(float snowFallGravityScale) override;

        float GetSnowFallWindScale() override;
        void SetSnowFallWindScale(float snowFallWindScale) override;

        float GetSnowFallTurbulence() override;
        void SetSnowFallTurbulence(float snowFallTurbulence) override;

        float GetSnowFallTurbulenceFreq() override;
        void SetSnowFallTurbulenceFreq(float snowFallTurbulenceFreq) override;

        SnowOptions GetSnowOptions() override;
        void SetSnowOptions(SnowOptions snowOptions) override;

        /**
         * Sends this component's snow parameters to the engine
         *
         * If the component is disabled or the "amount" of this snow object is 0
         * the engine will attempt to disable snow. Snow can be re-enabled if another
         * snow component updates and overrides that setting.
         */
        void UpdateSnow() override;

    private:
        //Reflected members
        bool m_enabled;
        bool m_renderInEditMode = false;
        SnowOptions m_snowOptions;

        //Unreflected members
        AZ::Transform m_currentWorldTransform;
    };

    class SnowConverter : public AZ::LegacyConversion::LegacyConversionEventBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(SnowConverter, AZ::SystemAllocator, 0);

        SnowConverter() {}

        // ------------------- LegacyConversionEventBus::Handler -----------------------------
        AZ::LegacyConversion::LegacyConversionResult ConvertEntity(CBaseObject* pEntityToConvert) override;
        bool BeforeConversionBegins() override;
        bool AfterConversionEnds() override;
        // END ----------------LegacyConversionEventBus::Handler ------------------------------
    };
}
