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

#include "PlanarManipulator.h"

#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <AzToolsFramework/Manipulators/ManipulatorSnapping.h>

namespace AzToolsFramework
{
    PlanarManipulator::StartInternal PlanarManipulator::CalculateManipulationDataStart(
        const Fixed& fixed, const AZ::Transform& worldFromLocal, bool snapping, float gridSize,
        const AZ::Vector3 localStartPosition, const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection,
        ManipulatorSpace manipulatorSpace)
    {
        AZ::Transform worldFromLocalNormalized = worldFromLocal;
        const AZ::Vector3 scale = worldFromLocalNormalized.ExtractScale();

        const AZ::Transform localFromWorldNormalized = worldFromLocalNormalized.GetInverseFast();
        const AZ::Vector3 normal = Internal::TransformAxisForSpace(manipulatorSpace, localFromWorldNormalized, fixed.m_normal);
        const AZ::Vector3 axis1 = Internal::TransformAxisForSpace(manipulatorSpace, localFromWorldNormalized, fixed.m_axis1);
        const AZ::Vector3 axis2 = Internal::TransformAxisForSpace(manipulatorSpace, localFromWorldNormalized, fixed.m_axis2);

        const AZ::Vector3 localRayOrigin = localFromWorldNormalized * rayOrigin;
        const AZ::Vector3 localRayDirection = localFromWorldNormalized.Multiply3x3(rayDirection);

        StartInternal startInternal;
        Internal::CalculateRayPlaneIntersectingPoint(
            localRayOrigin, localRayDirection, localStartPosition,
            normal, startInternal.m_localHitPosition);

        const AZ::Vector3 scaleRecip = scale.GetReciprocal();
        const AZ::Vector3 snapOffset = snapping
            ? CalculateSnappedOffset(localStartPosition, axis1, gridSize * MaxElement(scaleRecip)) +
              CalculateSnappedOffset(localStartPosition, axis2, gridSize * MaxElement(scaleRecip))
            : AZ::Vector3::CreateZero();

        startInternal.m_snapOffset = snapOffset;
        startInternal.m_localPosition = localStartPosition + snapOffset;

        return startInternal;
    }

    PlanarManipulator::Action PlanarManipulator::CalculateManipulationDataAction(
        const Fixed& fixed, const StartInternal& startInternal, const AZ::Transform& worldFromLocal,
        bool snapping, float gridSize, const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection,
        ManipulatorSpace manipulatorSpace)
    {
        AZ::Transform worldFromLocalNormalized = worldFromLocal;
        const AZ::Vector3 scale = worldFromLocalNormalized.ExtractScale();

        const AZ::Transform localFromWorldNormalized = worldFromLocalNormalized.GetInverseFast();
        const AZ::Vector3 normal = Internal::TransformAxisForSpace(manipulatorSpace, localFromWorldNormalized, fixed.m_normal);

        const AZ::Vector3 localRayOrigin = localFromWorldNormalized * rayOrigin;
        const AZ::Vector3 localRayDirection = localFromWorldNormalized.Multiply3x3(rayDirection);

        AZ::Vector3 localHitPosition;
        Internal::CalculateRayPlaneIntersectingPoint(
            localRayOrigin, localRayDirection, startInternal.m_localPosition,
            normal, localHitPosition);

        const AZ::Vector3 scaleRecip = scale.GetReciprocal();
        const AZ::Vector3 axis1 = Internal::TransformAxisForSpace(manipulatorSpace, localFromWorldNormalized, fixed.m_axis1);
        const AZ::Vector3 axis2 = Internal::TransformAxisForSpace(manipulatorSpace, localFromWorldNormalized, fixed.m_axis2);
        const AZ::Vector3 hitDelta = (localHitPosition - startInternal.m_localHitPosition) * scaleRecip;

        const AZ::Vector3 unsnappedOffset =
            axis1.Dot(hitDelta) * axis1 +
            axis2.Dot(hitDelta) * axis2;

        Action action;
        action.m_start.m_localPosition = startInternal.m_localPosition;
        action.m_start.m_snapOffset = startInternal.m_snapOffset;
        action.m_current.m_localOffset = snapping
            ? unsnappedOffset +
                CalculateSnappedOffset(unsnappedOffset, axis1, gridSize * MaxElement(scaleRecip)) +
                CalculateSnappedOffset(unsnappedOffset, axis2, gridSize * MaxElement(scaleRecip))
            : unsnappedOffset;

        return action;
    }

    PlanarManipulator::PlanarManipulator(AZ::EntityId entityId)
        : BaseManipulator(entityId)
    {
        AttachLeftMouseDownImpl();
    }

    PlanarManipulator::~PlanarManipulator() {}

    void PlanarManipulator::InstallLeftMouseDownCallback(MouseActionCallback onMouseDownCallback)
    {
        m_onLeftMouseDownCallback = onMouseDownCallback;
    }

    void PlanarManipulator::InstallLeftMouseUpCallback(MouseActionCallback onMouseUpCallback)
    {
        m_onLeftMouseUpCallback = onMouseUpCallback;
    }

    void PlanarManipulator::InstallMouseMoveCallback(MouseActionCallback onMouseMoveCallback)
    {
        m_onMouseMoveCallback = onMouseMoveCallback;
    }

    void PlanarManipulator::OnLeftMouseDownImpl(
        const ViewportInteraction::MouseInteraction& interaction, float /*rayIntersectionDistance*/)
    {
        AZ::Transform worldFromLocal;
        AZ::TransformBus::EventResult(worldFromLocal, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

        const bool snapping =
            GridSnapping(interaction.m_interactionId.m_viewportId);
        const float gridSize =
            GridSize(interaction.m_interactionId.m_viewportId);
        const ManipulatorSpace manipulatorSpace =
            GetManipulatorSpace(GetManipulatorManagerId());

        m_startInternal = CalculateManipulationDataStart(
            m_fixed, worldFromLocal, snapping, gridSize, m_position,
            interaction.m_mousePick.m_rayOrigin, interaction.m_mousePick.m_rayDirection,
            manipulatorSpace);

        if (m_onLeftMouseDownCallback)
        {
            m_onLeftMouseDownCallback(CalculateManipulationDataAction(
                m_fixed, m_startInternal, worldFromLocal, snapping, gridSize,
                interaction.m_mousePick.m_rayOrigin, interaction.m_mousePick.m_rayDirection,
                manipulatorSpace));
        }
    }

    void PlanarManipulator::OnMouseMoveImpl(const ViewportInteraction::MouseInteraction& interaction)
    {
        if (m_onMouseMoveCallback)
        {
            AZ::Transform worldFromLocal;
            AZ::TransformBus::EventResult(worldFromLocal, GetEntityId(),
                &AZ::TransformBus::Events::GetWorldTM);

            m_onMouseMoveCallback(CalculateManipulationDataAction(
                m_fixed, m_startInternal, worldFromLocal, GridSnapping(interaction.m_interactionId.m_viewportId),
                GridSize(interaction.m_interactionId.m_viewportId), interaction.m_mousePick.m_rayOrigin,
                interaction.m_mousePick.m_rayDirection, GetManipulatorSpace(GetManipulatorManagerId())));
        }
    }

    void PlanarManipulator::OnLeftMouseUpImpl(const ViewportInteraction::MouseInteraction& interaction)
    {
        if (m_onLeftMouseUpCallback)
        {
            AZ::Transform worldFromLocal;
            AZ::TransformBus::EventResult(worldFromLocal, GetEntityId(),
                &AZ::TransformBus::Events::GetWorldTM);

            m_onLeftMouseUpCallback(CalculateManipulationDataAction(
                m_fixed, m_startInternal, worldFromLocal, GridSnapping(interaction.m_interactionId.m_viewportId),
                GridSize(interaction.m_interactionId.m_viewportId), interaction.m_mousePick.m_rayOrigin,
                interaction.m_mousePick.m_rayDirection, GetManipulatorSpace(GetManipulatorManagerId())));
        }
    }

    void PlanarManipulator::Draw(
        AzFramework::EntityDebugDisplayRequests& display,
        const ViewportInteraction::CameraState& cameraState,
        const ViewportInteraction::MouseInteraction& mouseInteraction)
    {
        AZ::Transform worldFromLocal;
        AZ::TransformBus::EventResult(
            worldFromLocal, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

        m_manipulatorView->Draw(
            GetManipulatorManagerId(), GetManipulatorId(),
            MouseOver(), m_position, worldFromLocal,
            display, cameraState, mouseInteraction, GetManipulatorSpace(GetManipulatorManagerId()));
    }

    void PlanarManipulator::SetAxes(const AZ::Vector3& axis1, const AZ::Vector3& axis2)
    {
        m_fixed.m_axis1 = axis1;
        m_fixed.m_axis2 = axis2;
        m_fixed.m_normal = axis1.Cross(axis2);
    }

    void PlanarManipulator::SetView(AZStd::unique_ptr<ManipulatorView>&& view)
    {
        m_manipulatorView = AZStd::move(view);
    }

    void PlanarManipulator::InvalidateImpl()
    {
        m_manipulatorView->Invalidate(GetManipulatorManagerId());
    }

    void PlanarManipulator::SetBoundsDirtyImpl()
    {
        m_manipulatorView->SetBoundDirty(GetManipulatorManagerId());
    }
}