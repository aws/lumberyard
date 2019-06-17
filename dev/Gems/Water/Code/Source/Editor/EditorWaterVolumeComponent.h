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

#include <LegacyEntityConversion/LegacyEntityConversionBus.h>

#include "WaterVolumeComponent.h"

namespace Water
{
    class EditorWaterVolumeComponent;

    class EditorWaterVolumeCommon
        : public WaterVolumeCommon
        , public EditorWaterVolumeComponentRequestBus::Handler
        , public AzToolsFramework::EditorEvents::Bus::Handler
    {
        friend class WaterVolumeConverter; //So that it can access m_displayFilled

    public:
        AZ_TYPE_INFO(EditorWaterVolumeCommon, "{6033CAE2-9FCD-44EF-9D06-DAA86417182B}", WaterVolumeCommon);
        AZ_CLASS_ALLOCATOR(EditorWaterVolumeCommon, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* context);

        void DrawWaterVolume(AzFramework::EntityDebugDisplayRequests* dc);

        //Overriding WaterVolumeCommon to connect/disconnect to editor specific bus
        void Activate() override;
        void Deactivate() override;

        // EditorWaterVolumeComponentRequestBus::Handler
        void SetMinSpec(EngineSpec minSpec) override;
        EngineSpec GetEngineSpec() override { return m_minSpec; }

        void SetDisplayFilled(bool displayFilled) override { m_displayFilled = displayFilled; }
        bool GetDisplayFilled() override { return m_displayFilled; }

        void SetViewDistanceMultiplier(float viewDistanceMultiplier);
        float GetViewDistanceMultiplier() { return m_viewDistanceMultiplier; }

    private:
        // AzToolsFramework::EditorEvents interface implementation
        void OnEditorSpecChange() override;

        bool m_displayFilled = false;

        static AZ::Color s_waterAreaColor;
    };

    class EditorWaterVolumeComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , public AzFramework::EntityDebugDisplayEventBus::Handler
    {
    public:
        friend class EditorWaterVolumeCommon;
        friend class WaterVolumeConverter; //So that it can access m_common

        AZ_COMPONENT(EditorWaterVolumeComponent, "{41EE10CD-11CF-40D3-BDF4-B70FC23EB44C}", AzToolsFramework::Components::EditorComponentBase);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provides);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& requires);
        static void Reflect(AZ::ReflectContext* context);
        
        ~EditorWaterVolumeComponent() = default;

        // EditorComponentBase
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        void BuildGameEntity(AZ::Entity* gameEntity);

        // EntityDebugDisplayEventBus
        void DisplayEntity(bool& handled) override;
        
    private:
        //Reflected members
        EditorWaterVolumeCommon m_common;
    };

    class WaterConverter
        : public AZ::LegacyConversion::LegacyConversionEventBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(WaterConverter, AZ::SystemAllocator, 0);

        WaterConverter() = default;

        // ------------------- LegacyConversionEventBus::Handler -----------------------------
        AZ::LegacyConversion::LegacyConversionResult ConvertEntity(CBaseObject* pEntityToConvert) override;
        bool BeforeConversionBegins() override;
        bool AfterConversionEnds() override;
        // END ----------------LegacyConversionEventBus::Handler ------------------------------
    };

} // namespace Water