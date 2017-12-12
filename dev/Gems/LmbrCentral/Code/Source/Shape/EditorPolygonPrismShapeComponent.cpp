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

#include "StdAfx.h"
#include "EditorPolygonPrismShapeComponent.h"

#include <AzCore/Math/VertexContainerInterface.h>
#include <AzCore/Math/VectorConversions.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <AzToolsFramework/Manipulators/ManipulatorBus.h>
#include <AzToolsFramework/Manipulators/PlanarManipulator.h>

namespace LmbrCentral
{
    static const float s_polygonPrismVertexSize = 0.2f;
    static const AZ::Vector4 s_aabbColor = AZ::Vector4(0.69f, 1.0f, 0.78f, 1.0f);

    /**
     * Util to calculate central position of prism (to draw the height manipulator)
     */
    static AZ::Vector3 CalculateHeightManipulatorPosition(const AZ::PolygonPrism& polygonPrism)
    {
        const AZ::VertexContainer<AZ::Vector2>& vertexContainer = polygonPrism.m_vertexContainer;

        AZ::Vector2 averageCentrePosition = AZ::Vector2::CreateZero();
        for (const AZ::Vector2& vertex : vertexContainer.GetVertices())
        {
            averageCentrePosition += vertex;
        }

        return Vector2ToVector3(averageCentrePosition / vertexContainer.Size(), polygonPrism.GetHeight());
    }

    void EditorPolygonPrismShapeComponent::Activate()
    {
        EditorBaseShapeComponent::Activate();
        PolygonPrismShapeComponentRequestsBus::Handler::BusConnect(GetEntityId());

        AZ::EntityId entityId = GetEntityId();
        PolygonPrismShapeComponentRequestsBus::Handler::BusConnect(entityId);
        EntitySelectionEvents::Bus::Handler::BusConnect(entityId);

        m_polygonPrismCommon.m_polygonPrism->SetCallbacks(
            [this]() { ShapeChangedNotification(GetEntityId()); RefreshManipulators(); },
            [this]()
            {
                // destroy and recreate manipulators when container is modified (vertices are added or removed)
                UnregisterManipulators();
                RegisterManipulators();
                ShapeChangedNotification(GetEntityId());
            });        
        ShapeComponentRequestsBus::Handler::BusConnect(GetEntityId());
    }

    void EditorPolygonPrismShapeComponent::Deactivate()
    {
        UnregisterManipulators();

        EditorBaseShapeComponent::Deactivate();

        PolygonPrismShapeComponentRequestsBus::Handler::BusDisconnect();
        EntitySelectionEvents::Bus::Handler::BusDisconnect();
        ShapeComponentRequestsBus::Handler::BusDisconnect();
    }

    void EditorPolygonPrismShapeComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorPolygonPrismShapeComponent, EditorComponentBase>()
                ->Version(1)
                ->Field("Configuration", &EditorPolygonPrismShapeComponent::m_polygonPrismCommon);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorPolygonPrismShapeComponent>(
                    "Polygon Prism Shape", "Provides polygon prism shape")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Shape")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/PolygonPrism.png")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/PolygonPrism.png")
                        //->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c)) Disabled for v1.11
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorPolygonPrismShapeComponent::m_polygonPrismCommon, "Configuration", "PolygonPrism Shape Configuration")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
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

        AzFramework::EntityDebugDisplayRequests* displayContext = AzFramework::EntityDebugDisplayRequestBus::FindFirstHandler();
        AZ_Assert(displayContext, "Invalid display context.");

        // draw the polygon prism aabb (aligned to world axis)
        AZ::Aabb aabb = AZ::PolygonPrismUtil::CalculateAabb(*m_polygonPrismCommon.m_polygonPrism, m_currentEntityTransform);
        displayContext->SetColor(s_aabbColor);
        displayContext->DrawWireBox(aabb.GetMin(), aabb.GetMax());
    }

    void EditorPolygonPrismShapeComponent::DrawShape(AzFramework::EntityDebugDisplayRequests* displayContext) const
    {
        const AZStd::vector<AZ::Vector2>& vertices = m_polygonPrismCommon.m_polygonPrism->m_vertexContainer.GetVertices();
        const float height = m_polygonPrismCommon.m_polygonPrism->GetHeight();

        displayContext->SetColor(s_shapeColor);
        displayContext->CullOff();

        // draw walls
        const size_t vertexCount = vertices.size();
        for (size_t i = 0; i < vertexCount; ++i)
        {
            const AZ::Vector3& p1 = AZ::Vector2ToVector3(vertices[i]);
            const AZ::Vector3& p2 = AZ::Vector2ToVector3(vertices[(i + 1) % vertexCount]);
            const AZ::Vector3 p3 = AZ::Vector2ToVector3(AZ::Vector3ToVector2(p1), height);
            const AZ::Vector3 p4 = AZ::Vector2ToVector3(AZ::Vector3ToVector2(p2), height);

            displayContext->DrawQuad(p2, p1, p3, p4);
        }

        displayContext->CullOn();

        // draw vertices
        for (const AZ::Vector2& vertex : vertices)
        {
            displayContext->DrawBall(AZ::Vector2ToVector3(vertex), s_polygonPrismVertexSize);
        }

        // draw lines
        displayContext->SetColor(s_shapeWireColor);
        for (size_t i = 0; i < vertexCount; ++i)
        {
            // vertical line
            const AZ::Vector3& p1 = AZ::Vector2ToVector3(vertices[i]);
            const AZ::Vector3 p2 = AZ::Vector2ToVector3(AZ::Vector3ToVector2(p1), height);
            displayContext->DrawLine(p1, p2);

            // bottom line
            const AZ::Vector3& p1n = AZ::Vector2ToVector3(vertices[(i + 1) % vertexCount]);
            displayContext->DrawLine(p1, p1n);

            // top line
            const AZ::Vector3 p2n = AZ::Vector2ToVector3(AZ::Vector3ToVector2(p1n), height);
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

        // refresh manipulators
        for (auto& manipulator : m_vertexManipulators)
        {
            manipulator->SetBoundsDirty();
        }

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

    void EditorPolygonPrismShapeComponent::AddVertex(const AZ::Vector2& vertex)
    {
        m_polygonPrismCommon.m_polygonPrism->m_vertexContainer.AddVertex(vertex);
    }

    void EditorPolygonPrismShapeComponent::UpdateVertex(size_t index, const AZ::Vector2& vertex)
    {
        m_polygonPrismCommon.m_polygonPrism->m_vertexContainer.UpdateVertex(index, vertex);
    }

    void EditorPolygonPrismShapeComponent::InsertVertex(size_t index, const AZ::Vector2& vertex)
    {
        m_polygonPrismCommon.m_polygonPrism->m_vertexContainer.InsertVertex(index, vertex);
    }

    void EditorPolygonPrismShapeComponent::RemoveVertex(size_t index)
    {
        m_polygonPrismCommon.m_polygonPrism->m_vertexContainer.RemoveVertex(index);
    }

    void EditorPolygonPrismShapeComponent::SetVertices(const AZStd::vector<AZ::Vector2>& vertices)
    {
        m_polygonPrismCommon.m_polygonPrism->m_vertexContainer.SetVertices(vertices);
    }

    void EditorPolygonPrismShapeComponent::ClearVertices()
    {
        m_polygonPrismCommon.m_polygonPrism->m_vertexContainer.Clear();
    }

    void EditorPolygonPrismShapeComponent::OnSelected()
    {
        RegisterManipulators();
    }

    void EditorPolygonPrismShapeComponent::OnDeselected()
    {
        UnregisterManipulators();
    }

    void EditorPolygonPrismShapeComponent::RegisterManipulators()
    {
        const AZ::VertexContainer<AZ::Vector2>& vertexContainer = m_polygonPrismCommon.m_polygonPrism->m_vertexContainer;
        size_t vertexCount = vertexContainer.Size();
        m_vertexManipulators.reserve(vertexCount);

        // initialize manipulators for all prism vertices
        AzToolsFramework::ManipulatorManagerId managerId = 1;
        for (size_t i = 0; i < vertexCount; ++i)
        {
            AZ::Vector2 vertex;
            vertexContainer.GetVertex(i, vertex);

            m_vertexManipulators.push_back(AZStd::make_shared<AzToolsFramework::TranslationManipulator>(GetEntityId(), AzToolsFramework::TranslationManipulator::Dimensions::Two));
            AzToolsFramework::TranslationManipulator* vertexManipulator = m_vertexManipulators.back().get();

            // initialize vertex manipulator
            vertexManipulator->SetPosition(Vector2ToVector3(vertex));
            vertexManipulator->SetAxes(AZ::Vector3::CreateAxisX(), AZ::Vector3::CreateAxisY());
            vertexManipulator->SetPlanesColor(AZ::Color(0.75f, 0.87f, 0.93f, 1.0f));
            vertexManipulator->SetAxesColor(AZ::Color(0.87f, 0.32f, 0.34f, 1.0f), AZ::Color(0.35f, 0.82f, 0.34f, 1.0f));

            // planar manipulator callbacks
            vertexManipulator->InstallPlanarManipulatorMouseDownCallback([this, vertexManipulator](const AzToolsFramework::PlanarManipulationData& manipulationData)
            {
                m_initialManipulatorPositionLocal = vertexManipulator->GetPosition();
            });

            vertexManipulator->InstallPlanarManipulatorMouseMoveCallback([this, vertexManipulator, i](const AzToolsFramework::PlanarManipulationData& manipulationData)
            {
                UpdateManipulatorAndVertexPositions(vertexManipulator, i, CalculateLocalOffsetFromOrigin(manipulationData, m_currentEntityTransform.GetInverseFast()));
            });

            // linear manipulator callbacks
            vertexManipulator->InstallLinearManipulatorMouseDownCallback([this, vertexManipulator](const AzToolsFramework::LinearManipulationData& manipulationData)
            {
                m_initialManipulatorPositionLocal = vertexManipulator->GetPosition();
            });

            vertexManipulator->InstallLinearManipulatorMouseMoveCallback([this, vertexManipulator, i](const AzToolsFramework::LinearManipulationData& manipulationData)
            {
                UpdateManipulatorAndVertexPositions(vertexManipulator, i, CalculateLocalOffsetFromOrigin(manipulationData, m_currentEntityTransform.GetInverseFast()));
            });

            vertexManipulator->Register(managerId);
        }

        // initialize height manipulator 
        m_heightManipulator = AZStd::make_shared<AzToolsFramework::LinearManipulator>(GetEntityId());
        m_heightManipulator->SetPosition(CalculateHeightManipulatorPosition(*m_polygonPrismCommon.m_polygonPrism));
        m_heightManipulator->SetDirection(AZ::Vector3::CreateAxisZ());
        m_heightManipulator->SetColor(AZ::Color(0.35f, 0.82f, 0.34f, 1.0f));
        m_heightManipulator->SetDisplayType(AzToolsFramework::LinearManipulator::DisplayType::SquarePoint);
        m_heightManipulator->SetSquarePointSize(5.0f);

        // height manipulator callbacks
        m_heightManipulator->InstallMouseDownCallback([this](const AzToolsFramework::LinearManipulationData& manipulationData)
        {
            m_initialHeight = m_polygonPrismCommon.m_polygonPrism->GetHeight();
            m_initialManipulatorPositionLocal = m_heightManipulator->GetPosition();
        });

        m_heightManipulator->InstallMouseMoveCallback([this](const AzToolsFramework::LinearManipulationData& manipulationData)
        {
            m_polygonPrismCommon.m_polygonPrism->SetHeight(AZ::VectorFloat(m_initialHeight + manipulationData.m_totalTranslationWorldDelta).GetMax(AZ::VectorFloat::CreateZero()));
            
            AZ::Vector3 localPosition = m_initialManipulatorPositionLocal + CalculateLocalOffsetFromOrigin(manipulationData, m_currentEntityTransform.GetInverseFast());
            m_heightManipulator->SetPosition(Vector2ToVector3(Vector3ToVector2(localPosition), localPosition.GetZ().GetMax(AZ::VectorFloat::CreateZero())));
            m_heightManipulator->SetBoundsDirty();

            // ensure property grid values are refreshed
            AzToolsFramework::ToolsApplicationNotificationBus::Broadcast(&AzToolsFramework::ToolsApplicationNotificationBus::Events::InvalidatePropertyDisplay, AzToolsFramework::Refresh_Values);
        });

        m_heightManipulator->Register(managerId);
    }

    void EditorPolygonPrismShapeComponent::UnregisterManipulators()
    {
        if (m_heightManipulator)
        {
            m_heightManipulator->Unregister();
            m_heightManipulator.reset();
        }

        // clear all manipulators when deselected
        for (auto& manipulator : m_vertexManipulators)
        {
            manipulator->Unregister();
        }

        m_vertexManipulators.clear();
    }

    void EditorPolygonPrismShapeComponent::RefreshManipulators()
    {
        const AZ::VertexContainer<AZ::Vector2>& vertexContainer = m_polygonPrismCommon.m_polygonPrism->m_vertexContainer;
        for (size_t i = 0; i < m_vertexManipulators.size(); ++i)
        {
            AZ::Vector2 vertex;
            if (vertexContainer.GetVertex(i, vertex))
            {
                m_vertexManipulators[i]->SetPosition(Vector2ToVector3(vertex));
                m_vertexManipulators[i]->SetBoundsDirty();
            }
        }

        if (m_heightManipulator)
        {
            m_heightManipulator->SetPosition(CalculateHeightManipulatorPosition(*m_polygonPrismCommon.m_polygonPrism));
            m_heightManipulator->SetBoundsDirty();
        }
    }

    void EditorPolygonPrismShapeComponent::UpdateManipulatorAndVertexPositions(AzToolsFramework::TranslationManipulator* vertexManipulator, size_t vertexIndex, const AZ::Vector3& localOffset)
    {
        AZ::Vector3 localPosition = m_initialManipulatorPositionLocal + localOffset;

        UpdateVertex(vertexIndex, Vector3ToVector2(localPosition));
        vertexManipulator->SetPosition(localPosition);
        vertexManipulator->SetBoundsDirty();

        // ensure we refresh the height manipulator as vertices are moved to ensure it stays central to the prism
        m_heightManipulator->SetPosition(CalculateHeightManipulatorPosition(*m_polygonPrismCommon.m_polygonPrism));
        m_heightManipulator->SetBoundsDirty();

        // ensure property grid values are refreshed
        AzToolsFramework::ToolsApplicationNotificationBus::Broadcast(&AzToolsFramework::ToolsApplicationNotificationBus::Events::InvalidatePropertyDisplay, AzToolsFramework::Refresh_Values);
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