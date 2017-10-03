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
#include "EditorSplineComponent.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzToolsFramework/Manipulators/PlanarManipulator.h>
#include <AzToolsFramework/Manipulators/LinearManipulator.h>

namespace LmbrCentral
{
    static const float s_splineVertexSize = 0.2f;
    static const float s_controlPointSize = 0.1f;
    static const AZ::Vector4 s_vertexColor = AZ::Vector4(1.00f, 1.00f, 0.78f, 0.4f);
    static const AZ::Vector4 s_splineColor = AZ::Vector4(1.00f, 1.00f, 0.78f, 0.5f);

    void EditorSplineComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorSplineComponent, EditorComponentBase>()
                ->Version(1)
                ->Field("Configuration", &EditorSplineComponent::m_splineCommon);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorSplineComponent>(
                    "Spline", "Defines a sequence of points that can be interpolated.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Shape")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/Attachment.png")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/Attachment.png")
                        //->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c)) Disabled for v1.11
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorSplineComponent::m_splineCommon, "Configuration", "Spline Configuration")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
            }
        }
    }

    void EditorSplineComponent::Activate()
    {        
        EditorComponentBase::Activate();
        
        AZ::EntityId entityId = GetEntityId();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(entityId);
        SplineComponentRequestBus::Handler::BusConnect(entityId);
        EntitySelectionEvents::Bus::Handler::BusConnect(entityId);

        m_splineCommon.SetCallbacks(
            [this]() { SplineComponentNotificationBus::Event(GetEntityId(), &SplineComponentNotificationBus::Events::OnSplineChanged); RefreshManipulators(); },
            [this]()
            {
                // destroy and recreate manipulators when container is modified (vertices are added or removed)
                UnregisterManipulators();
                RegisterManipulators();
                SplineComponentNotificationBus::Event(GetEntityId(), &SplineComponentNotificationBus::Events::OnSplineChanged);
            });
    }

    void EditorSplineComponent::Deactivate()
    {
        UnregisterManipulators();

        EditorComponentBase::Deactivate();

        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        EntitySelectionEvents::Bus::Handler::BusDisconnect();
        SplineComponentRequestBus::Handler::BusDisconnect();
    }

    void EditorSplineComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        if (auto component = gameEntity->CreateComponent<SplineComponent>())
        {
            component->m_splineCommon = m_splineCommon;
        }
    }

    void EditorSplineComponent::OnSplineChanged()
    {
        m_splineCommon.GetSpline()->OnSplineChanged();
        SplineComponentNotificationBus::Event(GetEntityId(), &SplineComponentNotificationBus::Events::OnSplineChanged);
    }

    static void DrawSpline(const AZ::Spline& spline, size_t begin, size_t end, AzFramework::EntityDebugDisplayRequests* displayContext)
    {
        size_t granularity = spline.GetSegmentGranularity();

        for (size_t i = begin; i < end; ++i)
        {
            AZ::Vector3 p1 = spline.GetVertex(i - 1);
            for (size_t j = 1; j <= granularity; ++j)
            {
                AZ::Vector3 p2 = spline.GetPosition(AZ::SplineAddress(i - 1, j / static_cast<float>(granularity)));
                displayContext->DrawLine(p1, p2);
                p1 = p2;
            }
        }
    }

    void EditorSplineComponent::DisplayEntity(bool& handled)
    {
        if (!IsSelected())
        {
            return;
        }

        handled = true;

        AzFramework::EntityDebugDisplayRequests* displayContext = AzFramework::EntityDebugDisplayRequestBus::FindFirstHandler();
        AZ_Assert(displayContext, "Invalid display context.");

        const AZ::Spline* spline = m_splineCommon.GetSpline().get();
        const size_t vertexCount = spline->GetVertexCount();
        if (vertexCount == 0)
        {
            return;
        }

        displayContext->PushMatrix(GetWorldTM());
        displayContext->SetColor(s_vertexColor);

        // render vertices
        for (const auto& vertex : spline->GetVertices())
        {
            displayContext->DrawBall(vertex, s_splineVertexSize);
        }
        
        displayContext->SetColor(s_splineColor);

        // render spline
        if (spline->RTTI_IsTypeOf(AZ::LinearSpline::RTTI_Type()) || spline->RTTI_IsTypeOf(AZ::BezierSpline::RTTI_Type()))
        {
            DrawSpline(*spline, 1, spline->IsClosed() ? vertexCount + 1 : vertexCount, displayContext);

            // draw bezier control points
            if (const AZ::BezierSpline* bezierSpline = azrtti_cast<const AZ::BezierSpline*>(spline))
            {
                for (const auto& bd : bezierSpline->GetBezierData())
                {
                    displayContext->DrawBall(bd.m_back, s_controlPointSize);
                    displayContext->DrawBall(bd.m_forward, s_controlPointSize);
                }
            }
        }
        else if (spline->RTTI_IsTypeOf(AZ::CatmullRomSpline::RTTI_Type()))
        {
            // catmull-rom splines use the first and last points as control points only, omit those for display
            DrawSpline(*spline, spline->IsClosed() ? 1 : 2, spline->IsClosed() ? vertexCount + 1 : vertexCount - 1, displayContext);
        }

        displayContext->PopMatrix();
    }

    void EditorSplineComponent::OnSelected()
    {
        RegisterManipulators();
    }
    
    void EditorSplineComponent::OnDeselected()
    {
        UnregisterManipulators();
    }

    void EditorSplineComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& /*world*/)
    {
        // refresh manipulators
        for (auto& manipulator : m_vertexManipulators)
        {
            manipulator->SetBoundsDirty();
        }
    }
    
    void EditorSplineComponent::RegisterManipulators()
    {
        const AZ::VertexContainer<AZ::Vector3>& vertexContainer = m_splineCommon.GetSpline()->m_vertexContainer;
        size_t vertexCount = vertexContainer.Size();
        m_vertexManipulators.reserve(vertexCount);

        // initialize manipulators for all spline vertices
        AzToolsFramework::ManipulatorManagerId managerId = 1;
        for (size_t i = 0; i < vertexCount; ++i)
        {
            AZ::Vector3 vertex;
            vertexContainer.GetVertex(i, vertex);

            m_vertexManipulators.push_back(AZStd::make_shared<AzToolsFramework::TranslationManipulator>(GetEntityId(), AzToolsFramework::TranslationManipulator::Dimensions::Three));
            AzToolsFramework::TranslationManipulator* vertexManipulator = m_vertexManipulators.back().get();

            // initialize vertex manipulator
            vertexManipulator->SetPosition(vertex);
            vertexManipulator->SetAxes(AZ::Vector3::CreateAxisX(), AZ::Vector3::CreateAxisY(), AZ::Vector3::CreateAxisZ());
            vertexManipulator->SetPlanesColor(AZ::Color(0.87f, 0.32f, 0.34f, 1.0f), AZ::Color(0.35f, 0.82f, 0.34f, 1.0f), AZ::Color(0.48f, 0.73f, 0.93f, 1.0f));
            vertexManipulator->SetAxesColor(AZ::Color(0.87f, 0.32f, 0.34f, 1.0f), AZ::Color(0.35f, 0.82f, 0.34f, 1.0f), AZ::Color(0.48f, 0.73f, 0.93f, 1.0f));

            // planar manipulator callbacks
            vertexManipulator->InstallPlanarManipulatorMouseDownCallback([this, vertexManipulator](const AzToolsFramework::PlanarManipulationData& manipulationData)
            {
                m_initialManipulatorPositionLocal = vertexManipulator->GetPosition();
            });

            vertexManipulator->InstallPlanarManipulatorMouseMoveCallback([this, vertexManipulator, i](const AzToolsFramework::PlanarManipulationData& manipulationData)
            {
                UpdateManipulatorAndVertexPositions(vertexManipulator, i, CalculateLocalOffsetFromOrigin(manipulationData, GetWorldTM().GetInverseFast()));
            });

            // linear manipulator callbacks
            vertexManipulator->InstallLinearManipulatorMouseDownCallback([this, vertexManipulator](const AzToolsFramework::LinearManipulationData& manipulationData)
            {
                m_initialManipulatorPositionLocal = vertexManipulator->GetPosition();
            });

            vertexManipulator->InstallLinearManipulatorMouseMoveCallback([this, vertexManipulator, i](const AzToolsFramework::LinearManipulationData& manipulationData)
            {
                UpdateManipulatorAndVertexPositions(vertexManipulator, i, CalculateLocalOffsetFromOrigin(manipulationData, GetWorldTM().GetInverseFast()));
            });

            vertexManipulator->Register(managerId);
        }
    }
    
    void EditorSplineComponent::UnregisterManipulators()
    {
        // clear all manipulators when deselected
        for (auto& manipulator : m_vertexManipulators)
        {
            manipulator->Unregister();
        }

        m_vertexManipulators.clear();
    }
    
    void EditorSplineComponent::RefreshManipulators()
    {
        const AZ::VertexContainer<AZ::Vector3>& vertexContainer = m_splineCommon.GetSpline()->m_vertexContainer;
        for (size_t i = 0; i < m_vertexManipulators.size(); ++i)
        {
            AZ::Vector3 vertex;
            if (vertexContainer.GetVertex(i, vertex))
            {
                m_vertexManipulators[i]->SetPosition(vertex);
                m_vertexManipulators[i]->SetBoundsDirty();
            }
        }
    }

    AZ::ConstSplinePtr EditorSplineComponent::GetSpline()
    {
        return m_splineCommon.GetSpline();
    }

    void EditorSplineComponent::ChangeSplineType(AZ::u64 splineType)
    {
        m_splineCommon.ChangeSplineType(splineType);
    }

    void EditorSplineComponent::SetClosed(bool closed)
    {
        m_splineCommon.GetSpline()->SetClosed(closed);
        OnSplineChanged();
    }

    void EditorSplineComponent::UpdateVertex(size_t index, const AZ::Vector3& vertex)
    {
        m_splineCommon.GetSpline()->m_vertexContainer.UpdateVertex(index, vertex);
        SplineComponentNotificationBus::Event(GetEntityId(), &SplineComponentNotificationBus::Events::OnSplineChanged);
    }

    void EditorSplineComponent::AddVertex(const AZ::Vector3& vertex)
    {
        m_splineCommon.GetSpline()->m_vertexContainer.AddVertex(vertex);
        SplineComponentNotificationBus::Event(GetEntityId(), &SplineComponentNotificationBus::Events::OnSplineChanged);
    }

    void EditorSplineComponent::InsertVertex(size_t index, const AZ::Vector3& vertex)
    {
        m_splineCommon.GetSpline()->m_vertexContainer.InsertVertex(index, vertex);
        SplineComponentNotificationBus::Event(GetEntityId(), &SplineComponentNotificationBus::Events::OnSplineChanged);
    }

    void EditorSplineComponent::RemoveVertex(size_t index)
    {
        m_splineCommon.GetSpline()->m_vertexContainer.RemoveVertex(index);
        SplineComponentNotificationBus::Event(GetEntityId(), &SplineComponentNotificationBus::Events::OnSplineChanged);
    }

    void EditorSplineComponent::SetVertices(const AZStd::vector<AZ::Vector3>& vertices)
    {
        m_splineCommon.GetSpline()->m_vertexContainer.SetVertices(vertices);
        SplineComponentNotificationBus::Event(GetEntityId(), &SplineComponentNotificationBus::Events::OnSplineChanged);
    }

    void EditorSplineComponent::ClearVertices()
    {
        m_splineCommon.GetSpline()->m_vertexContainer.Clear();
        SplineComponentNotificationBus::Event(GetEntityId(), &SplineComponentNotificationBus::Events::OnSplineChanged);
    }
    
    void EditorSplineComponent::UpdateManipulatorAndVertexPositions(AzToolsFramework::TranslationManipulator* vertexManipulator, size_t vertexIndex, const AZ::Vector3& localOffset)
    {
        AZ::Vector3 localPosition = m_initialManipulatorPositionLocal + localOffset;

        UpdateVertex(vertexIndex, localPosition);
        vertexManipulator->SetPosition(localPosition);
        vertexManipulator->SetBoundsDirty();

        // ensure property grid values are refreshed
        AzToolsFramework::ToolsApplicationNotificationBus::Broadcast(&AzToolsFramework::ToolsApplicationNotificationBus::Events::InvalidatePropertyDisplay, AzToolsFramework::Refresh_Values);
    }

} // namespace LmbrCentral