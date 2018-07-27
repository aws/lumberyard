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

#include "LmbrCentral_precompiled.h"
#include "EditorPolygonPrismShapeComponent.h"

#include <AzCore/Math/IntersectSegment.h>
#include <AzCore/Math/VectorConversions.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/Manipulators/ManipulatorBus.h>
#include <AzToolsFramework/Manipulators/HoverSelection.h>

#include "EditorShapeComponentConverters.h"
#include "ShapeDisplay.h"
#include "ShapeGeometryUtil.h"

namespace LmbrCentral
{
    /**
     * Util to calculate central position of prism (to draw the height manipulator)
     */
    static AZ::Vector3 CalculateHeightManipulatorPosition(const AZ::PolygonPrism& polygonPrism)
    {
        const AZ::VertexContainer<AZ::Vector2>& vertexContainer = polygonPrism.m_vertexContainer;

        AZ::Vector2 averageCenterPosition = AZ::Vector2::CreateZero();
        for (const AZ::Vector2& vertex : vertexContainer.GetVertices())
        {
            averageCenterPosition += vertex;
        }

        return Vector2ToVector3(averageCenterPosition / vertexContainer.Size(), polygonPrism.GetHeight());
    }

    void EditorPolygonPrismShapeComponent::Activate()
    {
        EditorBaseShapeComponent::Activate();

        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
        EntitySelectionEvents::Bus::Handler::BusConnect(GetEntityId());
        EditorPolygonPrismShapeComponentRequestsBus::Handler::BusConnect(GetEntityId());
        ToolsApplicationEvents::Bus::Handler::BusConnect();

        m_polygonPrismShape.Activate(GetEntityId());

        bool selected = false;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
            selected, GetEntityId(), &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsSelected);

        // placeholder - create initial polygon prism shape if empty
        AZ::VertexContainer<AZ::Vector2>& vertexContainer = m_polygonPrismShape.GetPolygonPrism()->m_vertexContainer;
        if (selected && vertexContainer.Empty())
        {
            vertexContainer.AddVertex(AZ::Vector2(-2.0f, -2.0f));
            vertexContainer.AddVertex(AZ::Vector2(2.0f, -2.0f));
            vertexContainer.AddVertex(AZ::Vector2(2.0f, 2.0f));
            vertexContainer.AddVertex(AZ::Vector2(-2.0f, 2.0f));
            CreateManipulators();
        }

        const auto containerChanged = [this]()
        {
            GenerateVertices();

            // destroy and recreate manipulators when container is modified (vertices are added or removed)
            DestroyManipulators();
            CreateManipulators();

            m_polygonPrismShape.ShapeChanged();
        };

        const auto shapeModified = [this]()
        {
            GenerateVertices();
            m_polygonPrismShape.ShapeChanged();
            RefreshManipulators();
        };

        const auto vertexAdded = [this, containerChanged](size_t index)
        {
            containerChanged();

            AzToolsFramework::ManipulatorManagerId managerId = AzToolsFramework::ManipulatorManagerId(1);
            m_vertexSelection.CreateTranslationManipulator(GetEntityId(), managerId,
                AzToolsFramework::TranslationManipulator::Dimensions::Two,
                m_polygonPrismShape.GetPolygonPrism()->m_vertexContainer.GetVertices()[index], index,
                AzToolsFramework::ConfigureTranslationManipulatorAppearance2d);
        };

        m_polygonPrismShape.GetPolygonPrism()->SetCallbacks(
            vertexAdded,
            [containerChanged](size_t) { containerChanged(); },
            shapeModified,
            containerChanged,
            containerChanged,
            shapeModified);

        // callback after vertices in the selection have moved
        m_vertexSelection.m_onVertexPositionsUpdated = [this]()
        {
            // ensure we refresh the height manipulator after vertices are moved to ensure it stays central to the prism
            m_heightManipulator->SetPosition(CalculateHeightManipulatorPosition(*m_polygonPrismShape.GetPolygonPrism()));
            m_heightManipulator->SetBoundsDirty();
        };

        GenerateVertices();
    }

    void EditorPolygonPrismShapeComponent::Deactivate()
    {
        EditorBaseShapeComponent::Deactivate();

        DestroyManipulators();

        m_polygonPrismShape.Deactivate();
        ToolsApplicationEvents::Bus::Handler::BusDisconnect();
        EditorPolygonPrismShapeComponentRequestsBus::Handler::BusDisconnect();
        EntitySelectionEvents::Bus::Handler::BusDisconnect();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
    }

    void EditorPolygonPrismShapeComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorPolygonPrismShapeComponent, EditorBaseShapeComponent>()
                ->Version(2, &ClassConverters::UpgradeEditorPolygonPrismShapeComponent)
                ->Field("Configuration", &EditorPolygonPrismShapeComponent::m_polygonPrismShape)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorPolygonPrismShapeComponent>(
                    "Polygon Prism Shape", "Provides polygon prism shape")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Shape")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/PolygonPrism.png")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/PolygonPrism.png")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "http://docs.aws.amazon.com/console/lumberyard/userguide/polygon-prism-component")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorPolygonPrismShapeComponent::m_polygonPrismShape, "Configuration", "PolygonPrism Shape Configuration")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;
            }
        }
    }

    void EditorPolygonPrismShapeComponent::DisplayEntity(bool& handled)
    {
        DisplayShape(handled,
            [this]() { return CanDraw(); },
            [this](AzFramework::EntityDebugDisplayRequests* displayContext)
            {
                DrawPolygonPrismShape(
                    { m_shapeColor, m_shapeWireColor, m_displayFilled },
                    m_polygonPrismMesh, *displayContext);

                displayContext->SetColor(m_shapeWireColor);
                AzToolsFramework::EditorVertexSelectionUtil::DisplayVertexContainerIndices(
                    *displayContext, m_vertexSelection,
                    AzToolsFramework::TransformUniformScale(
                        m_polygonPrismShape.GetCurrentTransform()), IsSelected());
            },
            m_polygonPrismShape.GetCurrentTransform());
    }

    void EditorPolygonPrismShapeComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        if (auto component = gameEntity->CreateComponent<PolygonPrismShapeComponent>())
        {
            component->m_polygonPrismShape = m_polygonPrismShape;
        }

        if (m_visibleInGameView)
        {
            gameEntity->CreateComponent<PolygonPrismShapeDebugDisplayComponent>(
                *m_polygonPrismShape.GetPolygonPrism());
        }
    }

    void EditorPolygonPrismShapeComponent::OnTransformChanged(
        const AZ::Transform& /*local*/, const AZ::Transform& /*world*/)
    {
        // refresh bounds in all manipulators after the entity has moved
        m_vertexSelection.SetBoundsDirty();

        if (m_heightManipulator)
        {
            m_heightManipulator->SetBoundsDirty();
        }
    }

    void EditorPolygonPrismShapeComponent::OnSelected()
    {
        // ensure any maniulators are destroyed before recreated - (for undo/redo)
        DestroyManipulators();
        CreateManipulators();
    }

    void EditorPolygonPrismShapeComponent::OnDeselected()
    {
        DestroyManipulators();
    }

    void EditorPolygonPrismShapeComponent::CreateManipulators()
    {
        bool selected = false;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
            selected, &AzToolsFramework::ToolsApplicationRequests::IsSelected, GetEntityId());

        if (!selected)
        {
            return;
        }

        // if we have no vertices, do not attempt to create any manipulators
        if (m_polygonPrismShape.GetPolygonPrism()->m_vertexContainer.Empty())
        {
            return;
        }

        AZStd::unique_ptr<AzToolsFramework::LineSegmentHoverSelection<AZ::Vector2>> polygonPrismHover =
            AZStd::make_unique<AzToolsFramework::LineSegmentHoverSelection<AZ::Vector2>>();
        polygonPrismHover->m_vertices = AZStd::make_unique<AzToolsFramework::VariableVerticesVertexContainer<AZ::Vector2>>(
            m_polygonPrismShape.GetPolygonPrism()->m_vertexContainer);
        m_vertexSelection.m_hoverSelection = AZStd::move(polygonPrismHover);

        // create interface wrapping internal vertex container for use by vertex selection
        m_vertexSelection.m_vertices =
            AZStd::make_unique<AzToolsFramework::VariableVerticesVertexContainer<AZ::Vector2>>(
                m_polygonPrismShape.GetPolygonPrism()->m_vertexContainer);

        const AzToolsFramework::ManipulatorManagerId managerId = AzToolsFramework::ManipulatorManagerId(1);
        m_vertexSelection.Create(GetEntityId(), managerId,
            AzToolsFramework::TranslationManipulator::Dimensions::Two,
            AzToolsFramework::ConfigureTranslationManipulatorAppearance2d);

        // initialize height manipulator
        m_heightManipulator = AZStd::make_unique<AzToolsFramework::LinearManipulator>(GetEntityId());
        m_heightManipulator->SetPosition(CalculateHeightManipulatorPosition(*m_polygonPrismShape.GetPolygonPrism()));
        m_heightManipulator->SetAxis(AZ::Vector3::CreateAxisZ());

        const float lineLength = 0.5f;
        const float lineWidth = 0.05f;
        const float coneLength = 0.28f;
        const float coneRadius = 0.07f;
        AzToolsFramework::ManipulatorViews views;
        views.emplace_back(CreateManipulatorViewLine(*m_heightManipulator, AZ::Color(0.0f, 0.0f, 1.0f, 1.0f),
            lineLength, lineWidth));
        views.emplace_back(CreateManipulatorViewCone(*m_heightManipulator,
            AZ::Color(0.0f, 0.0f, 1.0f, 1.0f), m_heightManipulator->GetAxis() *
            (lineLength - coneLength), coneLength, coneRadius));
        m_heightManipulator->SetViews(AZStd::move(views));

        // height manipulator callbacks
        m_heightManipulator->InstallMouseMoveCallback([this](
            const AzToolsFramework::LinearManipulator::Action& action)
        {
            m_polygonPrismShape.GetPolygonPrism()->SetHeight(AZ::VectorFloat(
                action.LocalPosition().GetZ()).GetMax(AZ::VectorFloat::CreateZero()));
            m_heightManipulator->SetPosition(Vector2ToVector3(Vector3ToVector2(
                action.LocalPosition()), action.LocalPosition().GetZ().GetMax(AZ::VectorFloat::CreateZero())));
            m_heightManipulator->SetBoundsDirty();

            // ensure property grid values are refreshed
            AzToolsFramework::ToolsApplicationNotificationBus::Broadcast(
                &AzToolsFramework::ToolsApplicationNotificationBus::Events::InvalidatePropertyDisplay,
                AzToolsFramework::Refresh_Values);
        });

        m_heightManipulator->Register(managerId);
    }

    void EditorPolygonPrismShapeComponent::DestroyManipulators()
    {
        // clear all manipulators when deselected
        if (m_heightManipulator)
        {
            m_heightManipulator->Unregister();
            m_heightManipulator.reset();
        }

        m_vertexSelection.Destroy();
    }

    void EditorPolygonPrismShapeComponent::AfterUndoRedo()
    {
        bool selected;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
            selected, &AzToolsFramework::ToolsApplicationRequests::IsSelected, GetEntityId());

        if (selected)
        {
            DestroyManipulators();
            CreateManipulators();
        }
    }

    void EditorPolygonPrismShapeComponent::RefreshManipulators()
    {
        m_vertexSelection.Refresh();

        if (m_heightManipulator)
        {
            m_heightManipulator->SetPosition(CalculateHeightManipulatorPosition(*m_polygonPrismShape.GetPolygonPrism()));
            m_heightManipulator->SetBoundsDirty();
        }
    }

    void EditorPolygonPrismShapeComponent::GenerateVertices()
    {
        GeneratePolygonPrismMesh(
            m_polygonPrismShape.GetPolygonPrism()->m_vertexContainer.GetVertices(),
            m_polygonPrismShape.GetPolygonPrism()->GetHeight(), m_polygonPrismMesh);
    }
} // namespace LmbrCentral