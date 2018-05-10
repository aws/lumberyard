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

#include "PolygonPrismShape.h"
#include "EditorBaseShapeComponent.h"
#include "PolygonPrismShapeComponent.h"

#include <AzCore/Math/VertexContainerInterface.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Manipulators/EditorVertexSelection.h>
#include <AzToolsFramework/Manipulators/TranslationManipulator.h>
#include <AzToolsFramework/Manipulators/LinearManipulator.h>
#include <LmbrCentral/Shape/PolygonPrismShapeComponentBus.h>

namespace LmbrCentral
{
    /**
     * Editor representation for the PolygonPrismShapeComponent.
     * Visualizes the polygon prism in the editor - an extruded polygon.
     * Provide ability to create/edit polygon prism component from the editor.
     */
    class EditorPolygonPrismShapeComponent
        : public EditorBaseShapeComponent
        , private AzToolsFramework::EntitySelectionEvents::Bus::Handler
        , private AzToolsFramework::ToolsApplicationEvents::Bus::Handler
        , private AzFramework::EntityDebugDisplayEventBus::Handler
        , private EditorPolygonPrismShapeComponentRequestsBus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(EditorPolygonPrismShapeComponent, "{5368F204-FE6D-45C0-9A4F-0F933D90A785}", EditorBaseShapeComponent);
        static void Reflect(AZ::ReflectContext* context);

        EditorPolygonPrismShapeComponent() = default;

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

    private:
        AZ_DISABLE_COPY_MOVE(EditorPolygonPrismShapeComponent)

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            EditorBaseShapeComponent::GetProvidedServices(provided);
            provided.push_back(AZ_CRC("PolygonPrismShapeService", 0x1cbc4ed4));
            provided.push_back(AZ_CRC("VertexContainerService", 0x22cf8e10));
        }

        // EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;

        // AZ::TransformNotificationBus::Handler
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;
        
        // AzToolsFramework::EntitySelectionEvents::Bus::Handler
        void OnSelected() override;
        void OnDeselected() override;

        // AzFramework::EntityDebugDisplayEventBus
        void DisplayEntity(bool& handled) override;

        // Manipulator handling
        void CreateManipulators();
        void DestroyManipulators();
        void RefreshManipulators();

        // AzToolsFramework::ToolsApplicationEvents
        void AfterUndoRedo() override;

        void GenerateVertices() override;

        PolygonPrismShape m_polygonPrismShape; ///< Stores configuration data of a polygon prism for this component.
        AzToolsFramework::EditorVertexSelectionVariable<AZ::Vector2> m_vertexSelection; ///< Handles all manipulator interactions with vertices (inserting and translating).
        AZStd::unique_ptr<AzToolsFramework::LinearManipulator> m_heightManipulator; ///< Manipulator to control the height of the polygon prism.
        PolygonPrismMesh m_polygonPrismMesh; ///< Buffer to store triangles of top and bottom of Polygon Prism.
    };
} // namespace LmbrCentral