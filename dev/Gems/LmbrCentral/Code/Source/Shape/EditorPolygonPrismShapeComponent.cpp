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

#include <AzCore/Math/VectorConversions.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/Manipulators/ManipulatorBus.h>
#include <AzToolsFramework/Manipulators/HoverSelection.h>
#include "ShapeComponentConverters.h"

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

        const AZ::EntityId entityId = GetEntityId();
        EntitySelectionEvents::Bus::Handler::BusConnect(entityId);
        ShapeComponentRequestsBus::Handler::BusConnect(entityId);
        PolygonPrismShapeComponentRequestBus::Handler::BusConnect(entityId);
        ToolsApplicationEvents::Bus::Handler::BusConnect();

        bool selected = false;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
            selected, GetEntityId(), &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsSelected);

        // placeholder - create initial polygon prism shape if empty
        AZ::VertexContainer<AZ::Vector2>& vertexContainer = m_polygonPrismCommon.m_polygonPrism->m_vertexContainer;
        if (selected && vertexContainer.Empty())
        {
            vertexContainer.AddVertex(AZ::Vector2(-2.0f, -2.0f));
            vertexContainer.AddVertex(AZ::Vector2(2.0f, -2.0f));
            vertexContainer.AddVertex(AZ::Vector2(2.0f, 2.0f));
            vertexContainer.AddVertex(AZ::Vector2(-2.0f, 2.0f));
            CreateManipulators();
        }

        auto containerChanged = [this]()
        {
            // destroy and recreate manipulators when container is modified (vertices are added or removed)
            DestroyManipulators();
            CreateManipulators();
            ShapeChangedNotification(GetEntityId());
        };

        auto shapeModified = [this]()
        {
            ShapeChangedNotification(GetEntityId());
            RefreshManipulators();
        };

        auto vertexAdded = [this, containerChanged](size_t index)
        {
            containerChanged();

            AzToolsFramework::ManipulatorManagerId managerId = AzToolsFramework::ManipulatorManagerId(1);
            m_vertexSelection.CreateTranslationManipulator(GetEntityId(), managerId,
                AzToolsFramework::TranslationManipulator::Dimensions::Two,
                m_polygonPrismCommon.m_polygonPrism->m_vertexContainer.GetVertices()[index], index,
                AzToolsFramework::ConfigureTranslationManipulatorAppearance2d);
        };

        m_polygonPrismCommon.m_polygonPrism->SetCallbacks(
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
            m_heightManipulator->SetPosition(CalculateHeightManipulatorPosition(*m_polygonPrismCommon.m_polygonPrism));
            m_heightManipulator->SetBoundsDirty();
        };
    }

    void EditorPolygonPrismShapeComponent::Deactivate()
    {
        DestroyManipulators();

        EditorBaseShapeComponent::Deactivate();

        PolygonPrismShapeComponentRequestBus::Handler::BusDisconnect();
        EntitySelectionEvents::Bus::Handler::BusDisconnect();
        ShapeComponentRequestsBus::Handler::BusDisconnect();
        ToolsApplicationEvents::Bus::Handler::BusDisconnect();
    }

    void EditorPolygonPrismShapeComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorPolygonPrismShapeComponent, EditorBaseShapeComponent>()
                ->Version(2, &ClassConverters::UpgradeEditorPolygonPrismShapeComponent)
                ->Field("Configuration", &EditorPolygonPrismShapeComponent::m_polygonPrismCommon);

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
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorPolygonPrismShapeComponent::m_polygonPrismCommon, "Configuration", "PolygonPrism Shape Configuration")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                ;
            }
        }
    }

    void EditorPolygonPrismShapeComponent::DisplayEntity(bool& handled)
    {
        // ensure base shape display is called (DrawShape)
        EditorBaseShapeComponent::DisplayEntity(handled);

        if (!IsSelected())
        {
            return;
        }

        handled = true;

        // disable Aabb rendering
        //AzFramework::EntityDebugDisplayRequests* displayContext = AzFramework::EntityDebugDisplayRequestBus::FindFirstHandler();
        //AZ_Assert(displayContext, "Invalid display context.");

        // draw the polygon prism aabb (aligned to world axis)
        //AZ::Aabb aabb = AZ::PolygonPrismUtil::CalculateAabb(*m_polygonPrismCommon.m_polygonPrism, m_currentEntityTransform);
        //displayContext->SetColor(AZ::Vector4(0.69f, 1.0f, 0.78f, 1.0f));
        //displayContext->DrawWireBox(aabb.GetMin(), aabb.GetMax());
    }

    void EditorPolygonPrismShapeComponent::DrawShape(AzFramework::EntityDebugDisplayRequests* displayContext) const
    {
        const AZStd::vector<AZ::Vector2>& vertices = m_polygonPrismCommon.m_polygonPrism->m_vertexContainer.GetVertices();
        const float height = m_polygonPrismCommon.m_polygonPrism->GetHeight();

        // draw lines
        displayContext->SetColor(s_shapeWireColor);
        const size_t vertexCount = vertices.size();

        for (size_t i = 0; i < vertexCount; ++i)
        {
            // vertical line
            const AZ::Vector3 p1 = Vector2ToVector3(vertices[i]);
            const AZ::Vector3 p2 = Vector2ToVector3(vertices[i], height);
            displayContext->DrawLine(p1, p2);
        }

        // figure out how many lines to draw
        const size_t lineCount = vertexCount > 2
            ? vertexCount
            : vertexCount > 1
            ? 1
            : 0;

        for (size_t i = 0; i < lineCount; ++i)
        {
            // bottom line
            const AZ::Vector3 p1 = Vector2ToVector3(vertices[i]);
            const AZ::Vector3 p1n = Vector2ToVector3(vertices[(i + 1) % vertexCount]);
            displayContext->DrawLine(p1, p1n);

            // top line
            const AZ::Vector3 p2 = Vector2ToVector3(vertices[i], height);
            const AZ::Vector3 p2n = Vector2ToVector3(Vector3ToVector2(p1n), height);
            displayContext->DrawLine(p2, p2n);
        }
    }

    void EditorPolygonPrismShapeComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        if (auto component = gameEntity->CreateComponent<PolygonPrismShapeComponent>())
        {
            component->m_polygonPrismCommon = m_polygonPrismCommon;
        }
    }

    void EditorPolygonPrismShapeComponent::OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world)
    {
        EditorBaseShapeComponent::OnTransformChanged(local, world);

        // refresh bounds in all manipulators after the entity has moved
        m_vertexSelection.SetBoundsDirty();

        if (m_heightManipulator)
        {
            m_heightManipulator->SetBoundsDirty();
        }
    }

    AZ::ConstPolygonPrismPtr EditorPolygonPrismShapeComponent::GetPolygonPrism()
    {
        return m_polygonPrismCommon.m_polygonPrism;
    }

    void EditorPolygonPrismShapeComponent::SetHeight(float height)
    {
        m_polygonPrismCommon.m_polygonPrism->SetHeight(height);
    }

    bool EditorPolygonPrismShapeComponent::GetVertex(size_t index, AZ::Vector2& vertex) const
    {
        return m_polygonPrismCommon.m_polygonPrism->m_vertexContainer.GetVertex(index, vertex);
    }

    void EditorPolygonPrismShapeComponent::AddVertex(const AZ::Vector2& vertex)
    {
        m_polygonPrismCommon.m_polygonPrism->m_vertexContainer.AddVertex(vertex);
    }

    bool EditorPolygonPrismShapeComponent::UpdateVertex(size_t index, const AZ::Vector2& vertex)
    {
        return m_polygonPrismCommon.m_polygonPrism->m_vertexContainer.UpdateVertex(index, vertex);
    }

    bool EditorPolygonPrismShapeComponent::InsertVertex(size_t index, const AZ::Vector2& vertex)
    {
        return m_polygonPrismCommon.m_polygonPrism->m_vertexContainer.InsertVertex(index, vertex);
    }

    bool EditorPolygonPrismShapeComponent::RemoveVertex(size_t index)
    {
        return m_polygonPrismCommon.m_polygonPrism->m_vertexContainer.RemoveVertex(index);
    }

    void EditorPolygonPrismShapeComponent::SetVertices(const AZStd::vector<AZ::Vector2>& vertices)
    {
        m_polygonPrismCommon.m_polygonPrism->m_vertexContainer.SetVertices(vertices);
    }

    void EditorPolygonPrismShapeComponent::ClearVertices()
    {
        m_polygonPrismCommon.m_polygonPrism->m_vertexContainer.Clear();
    }

    size_t EditorPolygonPrismShapeComponent::Size() const
    {
        return m_polygonPrismCommon.m_polygonPrism->m_vertexContainer.Size();
    }

    bool EditorPolygonPrismShapeComponent::Empty() const
    {
        return m_polygonPrismCommon.m_polygonPrism->m_vertexContainer.Empty();
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
        // if we have no vertices, do not attempt to create any manipulators
        if (m_polygonPrismCommon.m_polygonPrism->m_vertexContainer.Empty())
        {
            return;
        }

        AZStd::unique_ptr<AzToolsFramework::LineSegmentHoverSelection<AZ::Vector2>> polygonPrismHover =
            AZStd::make_unique<AzToolsFramework::LineSegmentHoverSelection<AZ::Vector2>>();
        polygonPrismHover->m_vertices = AZStd::make_unique<AzToolsFramework::VariableVerticesVertexContainer<AZ::Vector2>>(
            m_polygonPrismCommon.m_polygonPrism->m_vertexContainer);
        m_vertexSelection.m_hoverSelection = AZStd::move(polygonPrismHover);

        // create interface wrapping internal vertex container for use by vertex selection
        m_vertexSelection.m_vertices =
            AZStd::make_unique<AzToolsFramework::VariableVerticesVertexContainer<AZ::Vector2>>(
                m_polygonPrismCommon.m_polygonPrism->m_vertexContainer);

        const AzToolsFramework::ManipulatorManagerId managerId = AzToolsFramework::ManipulatorManagerId(1);
        m_vertexSelection.Create(GetEntityId(), managerId,
            AzToolsFramework::TranslationManipulator::Dimensions::Two,
            AzToolsFramework::ConfigureTranslationManipulatorAppearance2d);

        // initialize height manipulator
        m_heightManipulator = AZStd::make_unique<AzToolsFramework::LinearManipulator>(GetEntityId());
        m_heightManipulator->SetPosition(CalculateHeightManipulatorPosition(*m_polygonPrismCommon.m_polygonPrism));
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
            m_polygonPrismCommon.m_polygonPrism->SetHeight(AZ::VectorFloat(
                action.LocalPosition().GetZ()).GetMax(AZ::VectorFloat::CreateZero()));
            m_heightManipulator->SetPosition(Vector2ToVector3(Vector3ToVector2(
                action.LocalPosition()), action.LocalPosition().GetZ().GetMax(AZ::VectorFloat::CreateZero())));
            m_heightManipulator->SetBoundsDirty();

            // ensure property grid values are refreshed
            AzToolsFramework::ToolsApplicationNotificationBus::Broadcast(&AzToolsFramework::ToolsApplicationNotificationBus::Events::InvalidatePropertyDisplay, AzToolsFramework::Refresh_Values);
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
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(selected, &AzToolsFramework::ToolsApplicationRequests::IsSelected, GetEntityId());
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
            m_heightManipulator->SetPosition(CalculateHeightManipulatorPosition(*m_polygonPrismCommon.m_polygonPrism));
            m_heightManipulator->SetBoundsDirty();
        }
    }

    AZ::Aabb EditorPolygonPrismShapeComponent::GetEncompassingAabb()
    {
        return AZ::PolygonPrismUtil::CalculateAabb(*m_polygonPrismCommon.m_polygonPrism, m_currentEntityTransform);
    }

    bool EditorPolygonPrismShapeComponent::IsPointInside(const AZ::Vector3& point)
    {
        // initial early aabb rejection test
        // note: will implicitly do height test too
        if (!GetEncompassingAabb().Contains(point))
        {
            return false;
        }

        return AZ::PolygonPrismUtil::IsPointInside(*m_polygonPrismCommon.m_polygonPrism, point, m_currentEntityTransform);
    }

    float EditorPolygonPrismShapeComponent::DistanceSquaredFromPoint(const AZ::Vector3& point)
    {
        return AZ::PolygonPrismUtil::DistanceSquaredFromPoint(*m_polygonPrismCommon.m_polygonPrism, point, m_currentEntityTransform);;
    }
} // namespace LmbrCentral