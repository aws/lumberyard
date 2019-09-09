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

#include <AzToolsFramework/API/ComponentEntitySelectionBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>

#include "EditorUtils.h"
#include "River.h"

namespace RoadsAndRivers
{
    /**
     * In-editor river component. Handles debug drawing of the river geometry, listens to callbacks of underlying spline  
     * to regenerate the river
     */
    class EditorRiverComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , private AzFramework::EntityDebugDisplayEventBus::Handler
        , private AzToolsFramework::EditorComponentSelectionRequestsBus::Handler
        , private AzToolsFramework::EditorComponentSelectionNotificationsBus::Handler
        , private LmbrCentral::SplineAttributeNotificationBus::Handler
        , private AzToolsFramework::EditorEvents::Bus::Handler
    {
    private:
        using Base = AzToolsFramework::Components::EditorComponentBase;

    public:
        AZ_EDITOR_COMPONENT(EditorRiverComponent, "{62FF15A5-3B03-4B3D-958A-5F40E94D1577}");

        ~EditorRiverComponent() override = default;

        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        void BuildGameEntity(AZ::Entity* gameEntity) override;

    private:
        // EntityDebugDisplayEventBus
        void DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;

        // EditorComponentSelectionRequestsBus::Handler
        AZ::Aabb GetEditorSelectionBoundsViewport(
            const AzFramework::ViewportInfo& viewportInfo) override;
        bool EditorSelectionIntersectRayViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            const AZ::Vector3& src, const AZ::Vector3& dir, AZ::VectorFloat& distance) override;
        bool SupportsEditorRayIntersect() override { return true; };
        AZ::u32 GetBoundingBoxDisplayType() override { return NoBoundingBox; }

        // EditorComponentSelectionNotificationsBus::Handler
        void OnAccentTypeChanged(AzToolsFramework::EntityAccentType accent) override;

        // SplineAttributeNotificationBus::Handler
        void OnAttributeAdded(size_t index) override;
        void OnAttributeRemoved(size_t index) override;
        void OnAttributesSet(size_t size) override;
        void OnAttributesCleared() override;

        // AzToolsFramework::EditorEvents::Handler
        void OnEditorSpecChange() override;

        // Terrain heightmap properties
        float m_terrainBorderWidth = 5.0f;
        float m_embankment = 1.0f;
        float m_riverBedDepth = 5.0f;
        float m_riverBedWidthOffset = -1.0f;

        bool m_heightMapButton = false;

        // Vegetation properties
        bool m_eraseVegetationButton = true;
        float m_eraseVegetationWidth = 0.0f;
        float m_eraseVegetationVariance = 0.0f;

        void HeightmapButtonClicked();
        void EraseVegetationButtonClicked();
        float GetMinBorderWidthForTools();

        AzToolsFramework::EntityAccentType m_accentType = AzToolsFramework::EntityAccentType::None;
        River m_river;
    };
} // namespace RoadsAndRivers