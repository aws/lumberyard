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
#include <AzToolsFramework/Viewport/ViewportMessages.h>

namespace AzToolsFramework
{
    AZ::Quaternion LocalRotationForSpace(
        const ManipulatorSpace manipulatorSpace, const AZ::Transform& localTransform)
    {
        // local rotation will depend on if the manipulator space is local or world,
        // if space is world then disregard local rotation
        switch (manipulatorSpace)
        {
        case ManipulatorSpace::Local:
            return AZ::Quaternion::CreateFromTransform(
                TransformNormalizedScale(localTransform));
        case ManipulatorSpace::World:
            // fallthrough
        default:
            return AZ::Quaternion::CreateIdentity();
        }
    }

    LinearManipulator::StartInternal LinearManipulator::CalculateManipulationDataStart(
        const Fixed& fixed, const AZ::Transform& worldFromLocal, const AZ::Transform& localTransform,
        const bool snapping, const float gridSize, const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection,
        const ManipulatorSpace manipulatorSpace)
    {
        AZ::Transform worldFromLocalNormalized = worldFromLocal;
        const AZ::VectorFloat scale = worldFromLocalNormalized.ExtractScale().GetMaxElement();
        const AZ::VectorFloat scaleRecip = Round3(scale.GetReciprocal());

        const AZ::Transform localFromWorldNormalized = worldFromLocalNormalized.GetInverseFast();
        const AZ::Vector3 localRayOrigin = localFromWorldNormalized * rayOrigin;
        const AZ::Vector3 localRayDirection = localFromWorldNormalized.Multiply3x3(rayDirection);
        const AZ::Quaternion localRotation = LocalRotationForSpace(manipulatorSpace, localTransform);

        const AZ::Vector3 axis = Internal::TransformAxisForSpace(
            manipulatorSpace, localFromWorldNormalized, localRotation, fixed.m_axis);
        const AZ::Vector3 rayCrossAxis = localRayDirection.Cross(axis);

        StartInternal startInternal;
        startInternal.m_localNormal = rayCrossAxis.Cross(axis).GetNormalizedSafeExact();

        Internal::CalculateRayPlaneIntersectingPoint(
            localRayOrigin, localRayDirection, localTransform.GetTranslation(),
            startInternal.m_localNormal, startInternal.m_localHitPosition);

        // calculate position amount to snap to align with grid
        const AZ::Vector3 positionSnapOffset = snapping
            ? CalculateSnappedOffset(localTransform.GetTranslation(), axis, gridSize * scaleRecip)
            : AZ::Vector3::CreateZero();

        // calculate scale amount to snap to align to round scale value
        const AZ::Vector3 scaleSnapOffset = snapping
            ? localRotation.GetInverseFast() * CalculateSnappedOffset(
                localRotation * localTransform.RetrieveScale(), axis, gridSize * scaleRecip)
            : AZ::Vector3::CreateZero();

        startInternal.m_positionSnapOffset = positionSnapOffset;
        startInternal.m_scaleSnapOffset = scaleSnapOffset;
        startInternal.m_localPosition = localTransform.GetTranslation() + positionSnapOffset;
        startInternal.m_localScale = localTransform.RetrieveScale() + scaleSnapOffset;

        return startInternal;
    }

    LinearManipulator::Action LinearManipulator::CalculateManipulationDataAction(
        const Fixed& fixed, const StartInternal& startInternal, const AZ::Transform& worldFromLocal,
        const AZ::Transform& localTransform, const bool snapping, const float gridSize,
        const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection, const ManipulatorSpace manipulatorSpace)
    {
        AZ::Transform worldFromLocalNormalized = worldFromLocal;
        const AZ::VectorFloat scale = worldFromLocalNormalized.ExtractScale().GetMaxElement();
        const AZ::VectorFloat scaleRecip = Round3(scale.GetReciprocal());

        const AZ::Transform localFromWorldNormalized = worldFromLocalNormalized.GetInverseFast();
        const AZ::Vector3 localRayOrigin = localFromWorldNormalized * rayOrigin;
        const AZ::Vector3 localRayDirection = localFromWorldNormalized.Multiply3x3(rayDirection);
        const AZ::Quaternion localRotation = LocalRotationForSpace(manipulatorSpace, localTransform);

        AZ::Vector3 localHitPosition;
        Internal::CalculateRayPlaneIntersectingPoint(localRayOrigin, localRayDirection,
            startInternal.m_localPosition, startInternal.m_localNormal, localHitPosition);

        const AZ::Vector3 axis = Internal::TransformAxisForSpace(
            manipulatorSpace, localFromWorldNormalized, localRotation, fixed.m_axis);
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

        // how much to adjust the scale based on movement
        const AZ::Quaternion invLocalRotation = localRotation.GetInverseFast();
        action.m_current.m_localScaleOffset = snapping
            ? invLocalRotation * (unsnappedOffset + CalculateSnappedOffset(unsnappedOffset, axis, gridSize * scaleRecip))
            : invLocalRotation * unsnappedOffset;

        // in world space, no matter which axis you use, the shape will always scale uniformily
        if (manipulatorSpace == ManipulatorSpace::World)
        {
            // it is only possible to modify one axis at a time - here we add them all up
            // (will be two zero values and one non-zero value) - set all components
            // with that value to update scale uniformily
            AZ::VectorFloat scaleOffset = AZ::VectorFloat::CreateZero();
            for (size_t i = 0; i < 3; ++i)
            {
                scaleOffset += action.m_current.m_localScaleOffset.GetElement(static_cast<int>(i));
            }

            action.m_current.m_localScaleOffset = AZ::Vector3(scaleOffset);
        }

        return action;
    }

    LinearManipulator::LinearManipulator(const AZ::EntityId entityId, const AZ::Transform& worldFromLocal)
        : BaseManipulator(entityId)
        , m_worldFromLocal(worldFromLocal)
    {
        AttachLeftMouseDownImpl();

        // default to normal manipulator manager space
        m_interactiveManipulatorSpace = [this]()
        {
            return GetManipulatorSpace(GetManipulatorManagerId());
        };

        // later we may want to override the visual manipulator space (e.g. have the
        // manipulator drawn in local space, but affect world space)
        m_visualManipulatorSpace = m_interactiveManipulatorSpace;
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
        const ManipulatorSpace manipulatorSpace = m_interactiveManipulatorSpace();

        m_startInternal = CalculateManipulationDataStart(
            m_fixed, worldFromLocalUniformScale, m_localTransform, snapping, gridSize,
            interaction.m_mousePick.m_rayOrigin, interaction.m_mousePick.m_rayDirection,
            manipulatorSpace);

        if (m_onLeftMouseDownCallback)
        {
            m_onLeftMouseDownCallback(CalculateManipulationDataAction(
                m_fixed, m_startInternal, worldFromLocalUniformScale, m_localTransform, snapping, gridSize,
                interaction.m_mousePick.m_rayOrigin, interaction.m_mousePick.m_rayDirection,
                manipulatorSpace));
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
                m_interactiveManipulatorSpace()));
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
                m_interactiveManipulatorSpace()));
        }
    }

    void LinearManipulator::Draw(
        const ManipulatorManagerState& managerState,
        AzFramework::EntityDebugDisplayRequests& display,
        const ViewportInteraction::CameraState& cameraState,
        const ViewportInteraction::MouseInteraction& mouseInteraction)
    {
        const ManipulatorSpace manipulatorSpace = m_visualManipulatorSpace();

        for (auto& view : m_manipulatorViews)
        {
            view->Draw(
                GetManipulatorManagerId(), managerState,
                GetManipulatorId(), {
                    TransformUniformScale(m_worldFromLocal) * TransformNormalizedScale(m_localTransform),
                    AZ::Vector3::CreateZero(), MouseOver()
                },
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

    void LinearManipulator::OverrideInteractiveManipulatorSpace(
        const AZStd::function<ManipulatorSpace()>& interactiveManipulatorFunction)
    {
        m_interactiveManipulatorSpace = interactiveManipulatorFunction;
    }

    void LinearManipulator::OverrideVisualManipulatorSpace(
        const AZStd::function<ManipulatorSpace()>& visualManipulatorFunction)
    {
        m_visualManipulatorSpace = visualManipulatorFunction;
    }
}