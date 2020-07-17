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

#include "WindVolumeComponent.h"
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>

namespace LmbrCentral
{
    /**
     * In-editor WindVolume component for a wind volume.
     */
    class EditorWindVolumeComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , private AzFramework::EntityDebugDisplayEventBus::Handler
        , public WindVolume
    {
    public:
        AZ_EDITOR_COMPONENT(EditorWindVolumeComponent, "{61E5864D-F553-4A37-9A03-B9F836F1D3DC}", EditorComponentBase);
        static void Reflect(AZ::ReflectContext* context);

        EditorWindVolumeComponent() = default;
        ~EditorWindVolumeComponent() override = default;

        // EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        WindVolumeConfiguration& GetConfiguration() override { return m_configuration; }

    protected:
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("ShapeService", 0xe86aa5fe));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("CapsuleShapeService", 0x9bc1122c));
            incompatible.push_back(AZ_CRC("CylinderShapeService", 0x507c688e));
            incompatible.push_back(AZ_CRC("CompoundShapeService", 0x4f7c640a));
            incompatible.push_back(AZ_CRC("PolygonPrismShapeService", 0x1cbc4ed4));
        }

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("LegacyCryPhysicsService", 0xbb370351));
        }

    private:
        // AzFramework::EntityDebugDisplayEventBus
        void DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;

        void DrawBox(AzFramework::DebugDisplayRequests& debugDisplay);
        void DrawSphere(AzFramework::DebugDisplayRequests& debugDisplay);
        void DrawArrow(AzFramework::DebugDisplayRequests& debugDisplay, const AZ::Vector3& point);
        void OnConfigurationChanged();
        AZ::Vector3 GetLocalWindDirection(const AZ::Vector3& point) const;

        bool m_visibleInEditor = true;              ///< Visible in the editor viewport
        WindVolumeConfiguration m_configuration;    ///< Configuration of the wind volume
    };
} // namespace LmbrCentral
