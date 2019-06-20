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

#include <AzCore/Math/Transform.h>
#include <AzCore/Component/TransformBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/API/ComponentEntitySelectionBus.h>
#include <LmbrCentral/Shape/EditorShapeComponentBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <Shape/ShapeDisplay.h>

namespace LmbrCentral
{
    /// Common functionality for Editor Component Shapes.
    class EditorBaseShapeComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , public EditorShapeComponentRequestsBus::Handler
        , public AzToolsFramework::EditorComponentSelectionRequestsBus::Handler
        , public AzToolsFramework::EditorComponentSelectionNotificationsBus::Handler
        , private AZ::TransformNotificationBus::Handler
    {
    public:
        AZ_RTTI(EditorBaseShapeComponent, "{32B9D7E9-6743-427B-BAFD-1C42CFBE4879}", AzToolsFramework::Components::EditorComponentBase);
        static void Reflect(AZ::SerializeContext& context);

        EditorBaseShapeComponent() = default;

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // AZ::TransformNotificationBus
        void OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& /*world*/) override {}

        // EditorShapeComponentRequestsBus
        void SetShapeColor(const AZ::Color& solidColor) override;
        void SetShapeWireframeColor(const AZ::Color& wireColor) override;
        void SetVisibleInEditor(bool visible) override;

        /// Should shape be rendered all the time, even when not selected.
        bool CanDraw() const;

        void SetShapeComponentConfig(ShapeComponentConfig* shapeConfig);

    protected:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        // EditorComponentSelectionRequestsBus
        AZ::Aabb GetEditorSelectionBoundsViewport(
            const AzFramework::ViewportInfo& viewportInfo) override;
        bool EditorSelectionIntersectRayViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            const AZ::Vector3& src, const AZ::Vector3& dir, AZ::VectorFloat& distance) override;
        bool SupportsEditorRayIntersect() override { return true; }
        AZ::u32 GetBoundingBoxDisplayType() override { return AzToolsFramework::EditorComponentSelectionRequests::NoBoundingBox; }

        // EditorComponentSelectionNotificationsBus
        void OnAccentTypeChanged(AzToolsFramework::EntityAccentType accent) override;

        AZ::Color m_shapeColor = AzFramework::ViewportColors::DeselectedColor; ///< Shaded color used for debug visualizations.
        AZ::Color m_shapeWireColor = AzFramework::ViewportColors::WireColor; ///< Wireframe color used for debug visualizations.

        bool m_visibleInEditor = true; ///< Visible in the editor viewport.
        bool m_visibleInGameView = false; ///< Visible in Game View.
        bool m_displayFilled = true; ///< Should shape be displayed filled.

        ShapeComponentConfig* m_shapeConfig = nullptr;

    private:

        void OnShapeColorChanged();
        void OnDisplayFilledChanged();
    };
} // namespace LmbrCentral