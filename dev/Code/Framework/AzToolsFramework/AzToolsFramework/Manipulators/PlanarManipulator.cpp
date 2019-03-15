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

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <AzToolsFramework/Manipulators/ManipulatorSnapping.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

namespace AzToolsFramework
{
    PlanarManipulator::StartInternal PlanarManipulator::CalculateManipulationDataStart(
        const Fixed& fixed, const AZ::Transform& worldFromLocal, const AZ::Transform& localTransform,
        const bool snapping, const float gridSize, const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection,
        const ManipulatorSpace manipulatorSpace)
    {
        AZ::Transform worldFromLocalNormalized = worldFromLocal;
        const AZ::VectorFloat scale = worldFromLocalNormalized.ExtractScale().GetMaxElement();
        const AZ::VectorFloat scaleRecip = Round3(scale.GetReciprocal());
        const AZ::Quaternion localOrientation = AZ::Quaternion::CreateFromTransform(localTransform);

        const AZ::Transform localFromWorldNormalized = worldFromLocalNormalized.GetInverseFast();
        const AZ::Vector3 normal = Internal::TransformAxisForSpace(
            manipulatorSpace, localFromWorldNormalized, localOrientation, fixed.m_normal);
        const AZ::Vector3 axis1 = Internal::TransformAxisForSpace(
            manipulatorSpace, localFromWorldNormalized, localOrientation, fixed.m_axis1);
        const AZ::Vector3 axis2 = Internal::TransformAxisForSpace(
            manipulatorSpace, localFromWorldNormalized, localOrientation, fixed.m_axis2);

        const AZ::Vector3 localRayOrigin = localFromWorldNormalized * rayOrigin;
        const AZ::Vector3 localRayDirection = localFromWorldNormalized.Multiply3x3(rayDirection);

        StartInternal startInternal;
        Internal::CalculateRayPlaneIntersectingPoint(
            localRayOrigin, localRayDirection, localTransform.GetTranslation(),
            normal, startInternal.m_localHitPosition);

        // calculate amount to snap to align with grid
        const AZ::Vector3 snapOffset = snapping
            ? CalculateSnappedOffset(localTransform.GetTranslation(), axis1, gridSize * scaleRecip) +
              CalculateSnappedOffset(localTransform.GetTranslation(), axis2, gridSize * scaleRecip)
            : AZ::Vector3::CreateZero();

        startInternal.m_snapOffset = snapOffset;
        startInternal.m_localPosition = localTransform.GetTranslation() + snapOffset;

        return startInternal;
    }

    PlanarManipulator::Action PlanarManipulator::CalculateManipulationDataAction(
        const Fixed& fixed, const StartInternal& startInternal, const AZ::Transform& worldFromLocal,
        const AZ::Transform& localTransform, const bool snapping, const float gridSize,
        const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection, const ManipulatorSpace manipulatorSpace)
    {
        AZ::Transform worldFromLocalNormalized = worldFromLocal;
        const AZ::VectorFloat scale = worldFromLocalNormalized.ExtractScale().GetMaxElement();
        const AZ::VectorFloat scaleRecip = Round3(scale.GetReciprocal());
        const AZ::Quaternion localOrientation = AZ::Quaternion::CreateFromTransform(localTransform);

        const AZ::Transform localFromWorldNormalized = worldFromLocalNormalized.GetInverseFast();
        const AZ::Vector3 normal = Internal::TransformAxisForSpace(
            manipulatorSpace, localFromWorldNormalized, localOrientation, fixed.m_normal);

        const AZ::Vector3 localRayOrigin = localFromWorldNormalized * rayOrigin;
        const AZ::Vector3 localRayDirection = localFromWorldNormalized.Multiply3x3(rayDirection);

        AZ::Vector3 localHitPosition;
        Internal::CalculateRayPlaneIntersectingPoint(
            localRayOrigin, localRayDirection, startInternal.m_localPosition,
            normal, localHitPosition);

        const AZ::Vector3 axis1 = Internal::TransformAxisForSpace(
            manipulatorSpace, localFromWorldNormalized, localOrientation, fixed.m_axis1);
        const AZ::Vector3 axis2 = Internal::TransformAxisForSpace(
            manipulatorSpace, localFromWorldNormalized, localOrientation, fixed.m_axis2);

        const AZ::Vector3 hitDelta = (localHitPosition - startInternal.m_localHitPosition) * scaleRecip;
        const AZ::Vector3 unsnappedOffset =
            axis1.Dot(hitDelta) * axis1 +
            axis2.Dot(hitDelta) * axis2;

        Action action;
        action.m_start.m_localPosition = startInternal.m_localPosition;
        action.m_start.m_snapOffset = startInternal.m_snapOffset;
        action.m_current.m_localOffset = snapping
            ? unsnappedOffset +
                CalculateSnappedOffset(unsnappedOffset, axis1, gridSize * scaleRecip) +
                CalculateSnappedOffset(unsnappedOffset, axis2, gridSize * scaleRecip)
            : unsnappedOffset;

        return action;
    }

    PlanarManipulator::PlanarManipulator(const AZ::EntityId entityId, const AZ::Transform& worldFromLocal)
        : BaseManipulator(entityId)
        , m_worldFromLocal(worldFromLocal)
    {
        AttachLeftMouseDownImpl();
    }

    void PlanarManipulator::InstallLeftMouseDownCallback(const MouseActionCallback& onMouseDownCallback)
    {
        m_onLeftMouseDownCallback = onMouseDownCallback;
    }

    void PlanarManipulator::InstallLeftMouseUpCallback(const MouseActionCallback& onMouseUpCallback)
    {
        m_onLeftMouseUpCallback = onMouseUpCallback;
    }

    void PlanarManipulator::InstallMouseMoveCallback(const MouseActionCallback& onMouseMoveCallback)
    {
        m_onMouseMoveCallback = onMouseMoveCallback;
    }

    void PlanarManipulator::OnLeftMouseDownImpl(
        const ViewportInteraction::MouseInteraction& interaction, float /*rayIntersectionDistance*/)
    {
        const AZ::Transform worldFromLocalUniformScale = TransformUniformScale(m_worldFromLocal);

        const bool snapping =
            GridSnapping(interaction.m_interactionId.m_viewportId);
        const float gridSize =
            GridSize(interaction.m_interactionId.m_viewportId);
        const ManipulatorSpace manipulatorSpace =
            GetManipulatorSpace(GetManipulatorManagerId());

        m_startInternal = CalculateManipulationDataStart(
            m_fixed, worldFromLocalUniformScale, TransformNormalizedScale(m_localTransform),
            snapping, gridSize, interaction.m_mousePick.m_rayOrigin, interaction.m_mousePick.m_rayDirection,
            manipulatorSpace);

        if (m_onLeftMouseDownCallback)
        {
            m_onLeftMouseDownCallback(CalculateManipulationDataAction(
                m_fixed, m_startInternal, worldFromLocalUniformScale, TransformNormalizedScale(m_localTransform),
                snapping, gridSize, interaction.m_mousePick.m_rayOrigin, interaction.m_mousePick.m_rayDirection,
                manipulatorSpace));
        }
    }

    void PlanarManipulator::OnMouseMoveImpl(const ViewportInteraction::MouseInteraction& interaction)
    {
        if (m_onMouseMoveCallback)
        {
            m_onMouseMoveCallback(CalculateManipulationDataAction(
                m_fixed, m_startInternal, TransformUniformScale(m_worldFromLocal),
                TransformNormalizedScale(m_localTransform),
                GridSnapping(interaction.m_interactionId.m_viewportId),
                GridSize(interaction.m_interactionId.m_viewportId), interaction.m_mousePick.m_rayOrigin,
                interaction.m_mousePick.m_rayDirection, GetManipulatorSpace(GetManipulatorManagerId())));
        }
    }

    void PlanarManipulator::OnLeftMouseUpImpl(const ViewportInteraction::MouseInteraction& interaction)
    {
        if (m_onLeftMouseUpCallback)
        {
            m_onLeftMouseUpCallback(CalculateManipulationDataAction(
                m_fixed, m_startInternal, TransformUniformScale(m_worldFromLocal),
                TransformNormalizedScale(m_localTransform),
                GridSnapping(interaction.m_interactionId.m_viewportId),
                GridSize(interaction.m_interactionId.m_viewportId), interaction.m_mousePick.m_rayOrigin,
                interaction.m_mousePick.m_rayDirection, GetManipulatorSpace(GetManipulatorManagerId())));
        }
    }

    void PlanarManipulator::Draw(
        const ManipulatorManagerState& managerState,
        AzFramework::EntityDebugDisplayRequests& display,
        const ViewportInteraction::CameraState& cameraState,
        const ViewportInteraction::MouseInteraction& mouseInteraction)
    {
        m_manipulatorView->Draw(
            GetManipulatorManagerId(), managerState,
            GetManipulatorId(), {
                TransformUniformScale(m_worldFromLocal) * TransformNormalizedScale(m_localTransform),
                AZ::Vector3::CreateZero(), MouseOver()
            },
            display, cameraState, mouseInteraction,
            GetManipulatorSpace(GetManipulatorManagerId()));
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