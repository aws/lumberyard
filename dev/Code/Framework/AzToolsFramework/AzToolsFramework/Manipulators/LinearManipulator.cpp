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

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/Manipulators/ManipulatorSnapping.h>
#include <AzToolsFramework/Maths/TransformUtils.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

namespace AzToolsFramework
{
    LinearManipulator::StartInternal LinearManipulator::CalculateManipulationDataStart(
        const Fixed& fixed, const AZ::Transform& worldFromLocal, const AZ::Transform& localTransform,
        const bool snapping, const float gridSize, const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection,
        const AzFramework::CameraState& cameraState)
    {
        AZ::Transform worldFromLocalNormalized = worldFromLocal;
        const AZ::VectorFloat scale = worldFromLocalNormalized.ExtractScale().GetMaxElement();
        const AZ::VectorFloat scaleRecip = Round3(scale.GetReciprocal());

        const AZ::Transform localFromWorldNormalized = worldFromLocalNormalized.GetInverseFull();
        const AZ::Vector3 localRayOrigin = localFromWorldNormalized * rayOrigin;
        const AZ::Vector3 localRayDirection = localFromWorldNormalized.Multiply3x3(rayDirection);
        const AZ::Quaternion localRotation = QuaternionFromTransformNoScaling(localTransform);

        const AZ::Vector3 axis = localRotation * fixed.m_axis;
        const AZ::Vector3 rayCrossAxis = localRayDirection.Cross(axis);

        StartInternal startInternal;
        startInternal.m_localNormal = rayCrossAxis.Cross(axis).GetNormalizedSafeExact();

        Internal::CalculateRayPlaneIntersectingPoint(
            localRayOrigin, localRayDirection, localTransform.GetTranslation(),
            startInternal.m_localNormal, startInternal.m_localHitPosition);

        // calculate position amount to snap, to align with grid
        const AZ::Vector3 positionSnapOffset = snapping
            ? CalculateSnappedOffset(localTransform.GetTranslation(), axis, gridSize * scaleRecip)
            : AZ::Vector3::CreateZero();

        // calculate scale amount to snap, to align to round scale value
        const AZ::Vector3 scaleSnapOffset = snapping
            ? localRotation.GetInverseFull() * CalculateSnappedOffset(
                localRotation * localTransform.RetrieveScale(), axis, gridSize * scaleRecip)
            : AZ::Vector3::CreateZero();

        startInternal.m_positionSnapOffset = positionSnapOffset;
        startInternal.m_scaleSnapOffset = scaleSnapOffset;
        startInternal.m_localPosition = localTransform.GetTranslation() + positionSnapOffset;
        startInternal.m_localScale = localTransform.RetrieveScale() + scaleSnapOffset;
        startInternal.m_screenToWorldScale =
            AZ::VectorFloat::CreateOne() /
                CalculateScreenToWorldMultiplier((worldFromLocal * localTransform).GetTranslation(), cameraState);

        return startInternal;
    }

    LinearManipulator::Action LinearManipulator::CalculateManipulationDataAction(
        const Fixed& fixed, const StartInternal& startInternal, const AZ::Transform& worldFromLocal,
        const AZ::Transform& localTransform, const bool snapping, const float gridSize,
        const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection,
        const ViewportInteraction::KeyboardModifiers keyboardModifiers)
    {
        AZ::Transform worldFromLocalNormalized = worldFromLocal;
        const AZ::VectorFloat scale = worldFromLocalNormalized.ExtractScale().GetMaxElement();
        const AZ::VectorFloat scaleRecip = Round3(scale.GetReciprocal());

        const AZ::Transform localFromWorldNormalized = worldFromLocalNormalized.GetInverseFull();
        const AZ::Vector3 localRayOrigin = localFromWorldNormalized * rayOrigin;
        const AZ::Vector3 localRayDirection = localFromWorldNormalized.Multiply3x3(rayDirection);
        const AZ::Quaternion localRotation = QuaternionFromTransformNoScaling(localTransform);

        // as CalculateRayPlaneIntersectingPoint may fail, ensure localHitPosition is initialized with 
        // the starting hit position so the manipulator returns to the original location it was pressed
        // if an invalid ray intersection is attempted
        AZ::Vector3 localHitPosition = startInternal.m_localHitPosition;
        Internal::CalculateRayPlaneIntersectingPoint(localRayOrigin, localRayDirection,
            startInternal.m_localPosition, startInternal.m_localNormal, localHitPosition);

        const AZ::Vector3 axis = localRotation * fixed.m_axis;
        const AZ::Vector3 hitDelta = (localHitPosition - startInternal.m_localHitPosition) * scaleRecip;
        const AZ::Vector3 unsnappedOffset = axis * axis.Dot(hitDelta);

        Action action;
        action.m_fixed = fixed;
        action.m_start.m_localPosition = startInternal.m_localPosition;
        action.m_start.m_localScale = startInternal.m_localScale;
        action.m_start.m_localHitPosition = startInternal.m_localHitPosition;
        action.m_start.m_positionSnapOffset = startInternal.m_positionSnapOffset;
        action.m_start.m_scaleSnapOffset = startInternal.m_scaleSnapOffset;
        action.m_current.m_localPositionOffset = snapping
            ? unsnappedOffset + CalculateSnappedOffset(unsnappedOffset, axis, gridSize * scaleRecip)
            : unsnappedOffset;
        // sign to determine which side of the linear axis we pressed
        // (useful to know when the visual axis flips to face the camera)
        action.m_start.m_sign =
            (startInternal.m_localHitPosition - localTransform.GetTranslation()).Dot(axis).GetSign();

        const AZ::Vector3 scaledUnsnappedOffset = unsnappedOffset * startInternal.m_screenToWorldScale;
        // how much to adjust the scale based on movement
        const AZ::Quaternion invLocalRotation = localRotation.GetInverseFull();
        action.m_current.m_localScaleOffset = snapping
            ? invLocalRotation * (scaledUnsnappedOffset + CalculateSnappedOffset(scaledUnsnappedOffset, axis, gridSize * scaleRecip))
            : invLocalRotation * scaledUnsnappedOffset;

        // record what modifier keys are held during this action
        action.m_modifiers = keyboardModifiers;

        return action;
    }

    LinearManipulator::LinearManipulator(const AZ::Transform& worldFromLocal)
        : m_worldFromLocal(worldFromLocal)
    {
        AttachLeftMouseDownImpl();
    }

    void LinearManipulator::InstallLeftMouseDownCallback(const MouseActionCallback& onMouseDownCallback)
    {
        m_onLeftMouseDownCallback = onMouseDownCallback;
    }

    void LinearManipulator::InstallLeftMouseUpCallback(const MouseActionCallback& onMouseUpCallback)
    {
        m_onLeftMouseUpCallback = onMouseUpCallback;
    }

    void LinearManipulator::InstallMouseMoveCallback(const MouseActionCallback& onMouseMoveCallback)
    {
        m_onMouseMoveCallback = onMouseMoveCallback;
    }

    void LinearManipulator::OnLeftMouseDownImpl(
        const ViewportInteraction::MouseInteraction& interaction, float /*rayIntersectionDistance*/)
    {
        const AZ::Transform worldFromLocalUniformScale = TransformUniformScale(m_worldFromLocal);

        const bool snapping =
            GridSnapping(interaction.m_interactionId.m_viewportId);
        const float gridSize =
            GridSize(interaction.m_interactionId.m_viewportId);

        AzFramework::CameraState cameraState;
        ViewportInteraction::ViewportInteractionRequestBus::EventResult(
            cameraState, interaction.m_interactionId.m_viewportId,
            &ViewportInteraction::ViewportInteractionRequestBus::Events::GetCameraState);

        m_startInternal = CalculateManipulationDataStart(
            m_fixed, worldFromLocalUniformScale, m_localTransform, snapping, gridSize,
            interaction.m_mousePick.m_rayOrigin, interaction.m_mousePick.m_rayDirection,
            cameraState);

        if (m_onLeftMouseDownCallback)
        {
            m_onLeftMouseDownCallback(CalculateManipulationDataAction(
                m_fixed, m_startInternal, worldFromLocalUniformScale, m_localTransform, snapping, gridSize,
                interaction.m_mousePick.m_rayOrigin, interaction.m_mousePick.m_rayDirection,
                interaction.m_keyboardModifiers));
        }
    }

    void LinearManipulator::OnMouseMoveImpl(const ViewportInteraction::MouseInteraction& interaction)
    {
        if (m_onMouseMoveCallback)
        {
            m_onMouseMoveCallback(CalculateManipulationDataAction(
                m_fixed, m_startInternal, TransformUniformScale(m_worldFromLocal), m_localTransform,
                GridSnapping(interaction.m_interactionId.m_viewportId),
                GridSize(interaction.m_interactionId.m_viewportId),
                interaction.m_mousePick.m_rayOrigin, interaction.m_mousePick.m_rayDirection,
                interaction.m_keyboardModifiers));
        }
    }

    void LinearManipulator::OnLeftMouseUpImpl(const ViewportInteraction::MouseInteraction& interaction)
    {
        if (m_onLeftMouseUpCallback)
        {
            m_onLeftMouseUpCallback(CalculateManipulationDataAction(
                m_fixed, m_startInternal, TransformUniformScale(m_worldFromLocal), m_localTransform,
                GridSnapping(interaction.m_interactionId.m_viewportId),
                GridSize(interaction.m_interactionId.m_viewportId),
                interaction.m_mousePick.m_rayOrigin, interaction.m_mousePick.m_rayDirection,
                interaction.m_keyboardModifiers));
        }
    }

    void LinearManipulator::Draw(
        const ManipulatorManagerState& managerState,
        AzFramework::DebugDisplayRequests& debugDisplay,
        const AzFramework::CameraState& cameraState,
        const ViewportInteraction::MouseInteraction& mouseInteraction)
    {
        const AZ::Transform localTransform = m_useVisualsOverride
            ? AZ::Transform::CreateFromQuaternionAndTranslation(
                m_visualOrientationOverride, m_localTransform.GetTranslation())
            : m_localTransform;

        for (auto& view : m_manipulatorViews)
        {
            view->Draw(
                GetManipulatorManagerId(), managerState,
                GetManipulatorId(), {
                    m_worldFromLocal * localTransform,
                    AZ::Vector3::CreateZero(), MouseOver()
                },
                debugDisplay, cameraState, mouseInteraction);
        }
    }

    void LinearManipulator::SetAxis(const AZ::Vector3& axis)
    {
        m_fixed.m_axis = axis;
    }

    void LinearManipulator::SetSpace(const AZ::Transform& worldFromLocal)
    {
        m_worldFromLocal = worldFromLocal;
    }

    void LinearManipulator::SetLocalTransform(const AZ::Transform& localTransform)
    {
        m_localTransform = localTransform;
    }

    void LinearManipulator::SetLocalPosition(const AZ::Vector3& localPosition)
    {
        m_localTransform.SetPosition(localPosition);
    }

    void LinearManipulator::SetLocalOrientation(const AZ::Quaternion& localOrientation)
    {
        m_localTransform = AZ::Transform::CreateFromQuaternionAndTranslation(
            localOrientation, m_localTransform.GetTranslation());
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

} // namespace AzToolsFramework