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

#include "LinearManipulator.h"

#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <AzToolsFramework/Manipulators/ManipulatorSnapping.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

namespace AzToolsFramework
{
    LinearManipulator::StartInternal LinearManipulator::CalculateManipulationDataStart(
        const Fixed& fixed, const AZ::Transform& worldFromLocal, bool snapping, float gridSize,
        const AZ::Vector3 localStartPosition, const AZ::Vector3& rayOrigin,
        const AZ::Vector3& rayDirection, ManipulatorSpace manipulatorSpace)
    {
        AZ::Transform worldFromLocalNormalized = worldFromLocal;
        const AZ::Vector3 scale = worldFromLocalNormalized.ExtractScale();

        const AZ::Transform localFromWorldNormalized = worldFromLocalNormalized.GetInverseFast();
        const AZ::Vector3 localRayOrigin = localFromWorldNormalized * rayOrigin;
        const AZ::Vector3 localRayDirection = localFromWorldNormalized.Multiply3x3(rayDirection);

        const AZ::Vector3 axis = Internal::TransformAxisForSpace(manipulatorSpace, localFromWorldNormalized, fixed.m_axis);
        const AZ::Vector3 rayCrossAxis = localRayDirection.Cross(axis);

        StartInternal startInternal;
        startInternal.m_localNormal = rayCrossAxis.Cross(axis).GetNormalizedSafeExact();

        Internal::CalculateRayPlaneIntersectingPoint(localRayOrigin, localRayDirection,
            localStartPosition, startInternal.m_localNormal, startInternal.m_localHitPosition);

        const AZ::Vector3 snapOffset = snapping
            ? CalculateSnappedOffset(localStartPosition, axis, gridSize * scale.GetReciprocal().GetMaxElement())
            : AZ::Vector3::CreateZero();

        startInternal.m_snapOffset = snapOffset;
        startInternal.m_localPosition = localStartPosition + snapOffset;

        return startInternal;
    }

    LinearManipulator::Action LinearManipulator::CalculateManipulationDataAction(
        const Fixed& fixed, const StartInternal& startInternal, const AZ::Transform& worldFromLocal,
        bool snapping, float gridSize, const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection,
        ManipulatorSpace manipulatorSpace)
    {
        AZ::Transform worldFromLocalNormalized = worldFromLocal;
        const AZ::Vector3 scale = worldFromLocalNormalized.ExtractScale();

        const AZ::Transform localFromWorldNormalized = worldFromLocalNormalized.GetInverseFast();
        const AZ::Vector3 localRayOrigin = localFromWorldNormalized * rayOrigin;
        const AZ::Vector3 localRayDirection = localFromWorldNormalized.Multiply3x3(rayDirection);

        AZ::Vector3 localHitPosition;
        Internal::CalculateRayPlaneIntersectingPoint(localRayOrigin, localRayDirection,
            startInternal.m_localPosition, startInternal.m_localNormal, localHitPosition);

        const AZ::Vector3 scaleRecip = scale.GetReciprocal();
        const AZ::Vector3 axis = Internal::TransformAxisForSpace(manipulatorSpace, localFromWorldNormalized, fixed.m_axis);
        const AZ::Vector3 hitDelta = (localHitPosition - startInternal.m_localHitPosition) * scaleRecip;
        const AZ::Vector3 unsnappedOffset = axis * axis.Dot(hitDelta);

        Action action;
        action.m_start.m_localPosition = startInternal.m_localPosition;
        action.m_start.m_snapOffset = startInternal.m_snapOffset;
        action.m_current.m_localOffset = snapping
            ? unsnappedOffset + CalculateSnappedOffset(unsnappedOffset, axis, gridSize * scaleRecip.GetMaxElement())
            : unsnappedOffset;
        return action;
    }

    LinearManipulator::LinearManipulator(AZ::EntityId entityId)
        : BaseManipulator(entityId)
    {
        AttachLeftMouseDownImpl();
    }

    LinearManipulator::~LinearManipulator() {}

    void LinearManipulator::InstallLeftMouseDownCallback(MouseActionCallback onMouseDownCallback)
    {
        m_onLeftMouseDownCallback = onMouseDownCallback;
    }

    void LinearManipulator::InstallLeftMouseUpCallback(MouseActionCallback onMouseUpCallback)
    {
        m_onLeftMouseUpCallback = onMouseUpCallback;
    }

    void LinearManipulator::InstallMouseMoveCallback(MouseActionCallback onMouseMoveCallback)
    {
        m_onMouseMoveCallback = onMouseMoveCallback;
    }

    void LinearManipulator::OnLeftMouseDownImpl(
        const ViewportInteraction::MouseInteraction& interaction, float /*rayIntersectionDistance*/)
    {
        const AZ::Transform worldFromLocalUniformScale = WorldFromLocalWithUniformScale(GetEntityId());

        const bool snapping =
            GridSnapping(interaction.m_interactionId.m_viewportId);
        const float gridSize =
            GridSize(interaction.m_interactionId.m_viewportId);
        const ManipulatorSpace manipulatorSpace =
            GetManipulatorSpace(GetManipulatorManagerId());

        m_startInternal = CalculateManipulationDataStart(
            m_fixed, worldFromLocalUniformScale, snapping, gridSize, m_position,
            interaction.m_mousePick.m_rayOrigin, interaction.m_mousePick.m_rayDirection,
            manipulatorSpace);

        if (m_onLeftMouseDownCallback)
        {
            m_onLeftMouseDownCallback(CalculateManipulationDataAction(
                m_fixed, m_startInternal, worldFromLocalUniformScale, snapping, gridSize,
                interaction.m_mousePick.m_rayOrigin, interaction.m_mousePick.m_rayDirection,
                manipulatorSpace));
        }
    }

    void LinearManipulator::OnMouseMoveImpl(const ViewportInteraction::MouseInteraction& interaction)
    {
        if (m_onMouseMoveCallback)
        {
            m_onMouseMoveCallback(CalculateManipulationDataAction(
                m_fixed, m_startInternal, WorldFromLocalWithUniformScale(GetEntityId()), GridSnapping(interaction.m_interactionId.m_viewportId),
                GridSize(interaction.m_interactionId.m_viewportId), interaction.m_mousePick.m_rayOrigin,
                interaction.m_mousePick.m_rayDirection, GetManipulatorSpace(GetManipulatorManagerId())));
        }
    }

    void LinearManipulator::OnLeftMouseUpImpl(const ViewportInteraction::MouseInteraction& interaction)
    {
        if (m_onLeftMouseUpCallback)
        {
            m_onLeftMouseUpCallback(CalculateManipulationDataAction(
                m_fixed, m_startInternal, WorldFromLocalWithUniformScale(GetEntityId()), GridSnapping(interaction.m_interactionId.m_viewportId),
                GridSize(interaction.m_interactionId.m_viewportId), interaction.m_mousePick.m_rayOrigin,
                interaction.m_mousePick.m_rayDirection, GetManipulatorSpace(GetManipulatorManagerId())));
        }
    }

    void LinearManipulator::Draw(
        const ManipulatorManagerState& managerState,
        AzFramework::EntityDebugDisplayRequests& display,
        const ViewportInteraction::CameraState& cameraState,
        const ViewportInteraction::MouseInteraction& mouseInteraction)
    {
        const ManipulatorSpace manipulatorSpace = GetManipulatorSpace(GetManipulatorManagerId());

        for (auto& view : m_manipulatorViews)
        {
            view->Draw(
                GetManipulatorManagerId(), managerState,
                GetManipulatorId(), { WorldFromLocalWithUniformScale(GetEntityId()), m_position, MouseOver() },
                display, cameraState, mouseInteraction, manipulatorSpace);
        }
    }

    void LinearManipulator::SetViews(ManipulatorViews&& views)
    {
        m_manipulatorViews = AZStd::move(views);
    }

    void LinearManipulator::InvalidateImpl()
    {
        for (auto& view : m_manipulatorViews)
        {
            view->Invalidate(GetManipulatorManagerId());
        }
    }

    void LinearManipulator::SetBoundsDirtyImpl()
    {
        for (auto& view : m_manipulatorViews)
        {
            view->SetBoundDirty(GetManipulatorManagerId());
        }
    }
}