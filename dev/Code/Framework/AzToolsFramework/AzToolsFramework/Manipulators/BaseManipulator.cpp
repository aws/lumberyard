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

#include "BaseManipulator.h"

#include <AzCore/Math/IntersectSegment.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

namespace AzToolsFramework
{
    const AZ::Color BaseManipulator::s_defaultMouseOverColor = AZ::Color(1.0f, 1.0f, 0.0f, 1.0f); // yellow

    AZ_CLASS_ALLOCATOR_IMPL(BaseManipulator, AZ::SystemAllocator, 0)

    BaseManipulator::~BaseManipulator()
    {
        AZ_Assert(!Registered(), "Manipulator must be unregistered before it is destroyed");
        EndUndoBatch();
    }

    bool BaseManipulator::OnLeftMouseDown(
        const ViewportInteraction::MouseInteraction& interaction, const float rayIntersectionDistance)
    {
        if (m_onLeftMouseDownImpl)
        {
            BeginAction();
            ToolsApplicationRequests::Bus::BroadcastResult(
                m_undoBatch, &ToolsApplicationRequests::Bus::Events::BeginUndoBatch, "ManipulatorLeftMouseDown");

            for (const AZ::EntityId entityId : m_entityIds)
            {
                ToolsApplicationRequests::Bus::Broadcast(
                    &ToolsApplicationRequests::Bus::Events::AddDirtyEntity, entityId);
            }

            (*this.*m_onLeftMouseDownImpl)(interaction, rayIntersectionDistance);

            return true;
        }

        return false;
    }

    bool BaseManipulator::OnRightMouseDown(
        const ViewportInteraction::MouseInteraction& interaction, const float rayIntersectionDistance)
    {
        if (m_onRightMouseDownImpl)
        {
            BeginAction();
            ToolsApplicationRequests::Bus::BroadcastResult(
                m_undoBatch, &ToolsApplicationRequests::Bus::Events::BeginUndoBatch, "ManipulatorRightMouseDown");

            for (const AZ::EntityId entityId : m_entityIds)
            {
                ToolsApplicationRequests::Bus::Broadcast(
                    &ToolsApplicationRequests::Bus::Events::AddDirtyEntity, entityId);
            }

            (*this.*m_onRightMouseDownImpl)(interaction, rayIntersectionDistance);

            return true;
        }

        return false;
    }

    // note: OnLeft/RightMouseUp will not be called if OnLeft/RightMouseDownImpl have not been
    // attached as no active manipulator will have been set in ManipulatorManager.
    void BaseManipulator::OnLeftMouseUp(const ViewportInteraction::MouseInteraction& interaction)
    {
        EndAction();
        OnLeftMouseUpImpl(interaction);
        EndUndoBatch();
    }

    void BaseManipulator::OnRightMouseUp(const ViewportInteraction::MouseInteraction& interaction)
    {
        EndAction();
        OnRightMouseUpImpl(interaction);
        EndUndoBatch();
    }

    bool BaseManipulator::OnMouseOver(
        const ManipulatorId manipulatorId, const ViewportInteraction::MouseInteraction& interaction)
    {
        UpdateMouseOver(manipulatorId);
        OnMouseOverImpl(manipulatorId, interaction);
        return m_mouseOver;
    }

    void BaseManipulator::OnMouseWheel(const ViewportInteraction::MouseInteraction& interaction)
    {
        OnMouseWheelImpl(interaction);
    }

    void BaseManipulator::OnMouseMove(const ViewportInteraction::MouseInteraction& interaction)
    {
        if (!m_performingAction)
        {
            AZ_Warning("Manipulators", false, "MouseMove action received, but this manipulator is not performing actions!");
            return;
        }

        OnMouseMoveImpl(interaction);
    }

    void BaseManipulator::SetBoundsDirty()
    {
        SetBoundsDirtyImpl();
    }

    void BaseManipulator::Register(const ManipulatorManagerId managerId)
    {
        if (Registered())
        {
            Unregister();
        }

        ManipulatorManagerRequestBus::Event(managerId,
            &ManipulatorManagerRequestBus::Events::RegisterManipulator, shared_from_this());
    }

    void BaseManipulator::Unregister()
    {
        // if the manipulator has already been unregistered, the m_manipulatorManagerId
        // should be invalid which makes the call below a no-op.
        ManipulatorManagerRequestBus::Event(m_manipulatorManagerId,
            &ManipulatorManagerRequestBus::Events::UnregisterManipulator, this);
    }

    void BaseManipulator::Invalidate()
    {
        SetBoundsDirty();
        InvalidateImpl();
        m_mouseOver = false;
        m_performingAction = false;
        m_manipulatorId = InvalidManipulatorId;
        m_manipulatorManagerId = InvalidManipulatorManagerId;
    }

    void BaseManipulator::BeginAction()
    {
        if (m_performingAction)
        {
            AZ_Warning(
                "Manipulators", false,
                "MouseDown action received, but the manipulator (id: %d) is still performing an action",
                GetManipulatorId());

            return;
        }

        m_performingAction = true;
    }

    void BaseManipulator::EndAction()
    {
        if (!m_performingAction)
        {
            AZ_Warning(
                "Manipulators", false,
                "MouseUp action received, but this manipulator (id: %d) didn't receive MouseDown action before",
                GetManipulatorId());
            return;
        }

        m_performingAction = false;
    }

    void BaseManipulator::EndUndoBatch()
    {
        if (m_undoBatch != nullptr)
        {
            ToolsApplicationRequests::Bus::Broadcast(&ToolsApplicationRequests::Bus::Events::EndUndoBatch);
            m_undoBatch = nullptr;
        }
    }

    void BaseManipulator::AddEntityId(const AZ::EntityId entityId)
    {
        m_entityIds.insert(entityId);
    }

    AZStd::set<AZ::EntityId>::iterator BaseManipulator::RemoveEntityId(const AZ::EntityId entityId)
    {
        const auto entityIdIt = m_entityIds.find(entityId);
        if (entityIdIt != m_entityIds.end())
        {
            return m_entityIds.erase(entityIdIt);
        }

        return entityIdIt;
    }

    void Manipulators::Register(const ManipulatorManagerId manipulatorManagerId)
    {
        ProcessManipulators([manipulatorManagerId](BaseManipulator* manipulator)
        {
            manipulator->Register(manipulatorManagerId);
        });
    }

    void Manipulators::Unregister()
    {
        ProcessManipulators([](BaseManipulator* manipulator)
        {
            if (manipulator->Registered())
            {
                manipulator->Unregister();
            }
        });
    }

    void Manipulators::SetBoundsDirty()
    {
        ProcessManipulators([](BaseManipulator* manipulator)
        {
            manipulator->SetBoundsDirty();
        });
    }

    void Manipulators::AddEntityId(const AZ::EntityId entityId)
    {
        ProcessManipulators([entityId](BaseManipulator* manipulator)
        {
            manipulator->AddEntityId(entityId);
        });
    }

    void Manipulators::RemoveEntityId(const AZ::EntityId entityId)
    {
        ProcessManipulators([entityId](BaseManipulator* manipulator)
        {
            manipulator->RemoveEntityId(entityId);
        });
    }

    bool Manipulators::PerformingAction()
    {
        bool performingAction = false;
        ProcessManipulators([&performingAction](BaseManipulator* manipulator)
        {
            if (manipulator->PerformingAction())
            {
                performingAction = true;
            }
        });

        return performingAction;
    }

    bool Manipulators::Registered()
    {
        bool registered = false;
        ProcessManipulators([&registered](BaseManipulator* manipulator)
        {
            if (manipulator->Registered())
            {
                registered = true;
            }
        });

        return registered;
    }

    namespace Internal
    {
        bool CalculateRayPlaneIntersectingPoint(const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection,
            const AZ::Vector3& pointOnPlane, const AZ::Vector3& planeNormal, AZ::Vector3& resultIntersectingPoint)
        {
            float t = 0.0f;
            if (AZ::Intersect::IntersectRayPlane(rayOrigin, rayDirection, pointOnPlane, planeNormal, t) > 0)
            {
                resultIntersectingPoint = rayOrigin + t * rayDirection;
                return true;
            }

            return false;
        }
    } // namespace Internal
} // namespace AzToolsFramework