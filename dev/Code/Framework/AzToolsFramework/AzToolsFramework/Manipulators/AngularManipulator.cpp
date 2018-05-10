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

#include "AngularManipulator.h"

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include "ManipulatorSnapping.h"

namespace AzToolsFramework
{
    /**
     * Transform local axis of object to orientation when in world space
     */
    AZ::Vector3 LocalAxisToWorld(ManipulatorSpace manipulatorSpace, const AZ::Transform& worldFromLocal, const AZ::Vector3& axis)
    {
        switch (manipulatorSpace)
        {
        case ManipulatorSpace::Local:
            return worldFromLocal.Multiply3x3(axis);
        case ManipulatorSpace::World:
        default:
            return axis;
        }
    }

    AngularManipulator::ActionInternal AngularManipulator::CalculateManipulationDataStart(
        const Fixed& fixed, const AZ::Transform& worldFromLocal, const AZ::Transform& localTransform,
        const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection, ManipulatorSpace manipulatorSpace)
    {
        AZ::Transform worldFromLocalWithTransform = worldFromLocal * localTransform;

        const AZ::Vector3 worldAxis = LocalAxisToWorld(
            manipulatorSpace, worldFromLocalWithTransform, fixed.m_axis).GetNormalized();

        // store initial world hit position
        ActionInternal actionInternal;
        Internal::CalculateRayPlaneIntersectingPoint(
            rayOrigin, rayDirection, worldFromLocalWithTransform.GetTranslation(),
            worldAxis, actionInternal.m_current.m_worldHitPosition);

        // store entity transform (to go from local to world space)
        // and store our own starting local transform
        actionInternal.m_start.m_worldFromLocal = worldFromLocal;
        actionInternal.m_start.m_localTransform = localTransform;
        actionInternal.m_current.m_radians = 0.0f;

        return actionInternal;
    }

    AngularManipulator::Action AngularManipulator::CalculateManipulationDataAction(
        const Fixed& fixed, ActionInternal& actionInternal, const AZ::Transform& worldFromLocal,
        const AZ::Transform& localTransform, const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection,
        ManipulatorSpace manipulatorSpace)
    {
        const AZ::Transform worldFromLocalWithTransform = worldFromLocal * localTransform;

        const AZ::Vector3 worldAxis = LocalAxisToWorld(
            manipulatorSpace, worldFromLocalWithTransform, fixed.m_axis).GetNormalized();
        
        const AZ::Transform localFromWorldWithTransform = worldFromLocalWithTransform.GetInverseFast();
        const AZ::Vector3 localAxis = Internal::TransformAxisForSpace(
            manipulatorSpace, localFromWorldWithTransform, fixed.m_axis).GetNormalized();

        const AZ::Vector3 worldStartPosition = worldFromLocalWithTransform.GetTranslation();
        AZ::Vector3 worldHitPosition = AZ::Vector3::CreateZero();
        Internal::CalculateRayPlaneIntersectingPoint(rayOrigin, rayDirection,
            worldStartPosition, worldAxis, worldHitPosition);

        // get vector from center of rotation for this and previous frame
        const AZ::Vector3 currentWorldHitVector = (worldHitPosition - worldStartPosition).GetNormalizedSafeExact();
        const AZ::Vector3 previousWorldHitVector = (actionInternal.m_current.m_worldHitPosition - worldStartPosition).GetNormalizedSafeExact();

        // calculate which direction we rotated
        const AZ::Vector3 worldAxisRight = worldAxis.Cross(previousWorldHitVector);
        const float rotateSign = Sign(currentWorldHitVector.Dot(worldAxisRight));

        const float rotationAngleRad = acosf(GetMin(AZ::VectorFloat::CreateOne(), currentWorldHitVector.Dot(previousWorldHitVector)));
        actionInternal.m_current.m_radians += rotationAngleRad * rotateSign;
        actionInternal.m_current.m_worldHitPosition = worldHitPosition;

        Action action;
        action.m_start.m_localOrientation = AZ::Quaternion::CreateFromTransform(actionInternal.m_start.m_worldFromLocal).GetNormalizedExact();
        action.m_current.m_localRotation = (AZ::Quaternion::CreateFromTransform(actionInternal.m_start.m_localTransform) *
            AZ::Quaternion::CreateFromAxisAngle(localAxis, actionInternal.m_current.m_radians)).GetNormalizedExact();
            
        return action;
    }

    AngularManipulator::AngularManipulator(AZ::EntityId entityId)
        : BaseManipulator(entityId)
    {
        AttachLeftMouseDownImpl();
    }

    AngularManipulator::~AngularManipulator() {}

    void AngularManipulator::InstallLeftMouseDownCallback(MouseActionCallback onMouseDownCallback)
    {
        m_onLeftMouseDownCallback = onMouseDownCallback;
    }

    void AngularManipulator::InstallLeftMouseUpCallback(MouseActionCallback onMouseUpCallback)
    {
        m_onLeftMouseUpCallback = onMouseUpCallback;
    }

    void AngularManipulator::InstallMouseMoveCallback(MouseActionCallback onMouseMoveCallback)
    {
        m_onMouseMoveCallback = onMouseMoveCallback;
    }

    void AngularManipulator::OnLeftMouseDownImpl(
        const ViewportInteraction::MouseInteraction& interaction, float /*rayIntersectionDistance*/)
    {
        const ManipulatorSpace manipulatorSpace =
            GetManipulatorSpace(GetManipulatorManagerId());

        // calculate initial state when mouse first pressed
        m_actionInternal = CalculateManipulationDataStart(
            m_fixed, WorldFromLocalWithUniformScale(GetEntityId()), m_localTransform, interaction.m_mousePick.m_rayOrigin,
            interaction.m_mousePick.m_rayDirection, manipulatorSpace);

        if (m_onLeftMouseDownCallback)
        {
            m_onLeftMouseDownCallback(CalculateManipulationDataAction(
                m_fixed, m_actionInternal, m_actionInternal.m_start.m_worldFromLocal,
                m_actionInternal.m_start.m_localTransform, interaction.m_mousePick.m_rayOrigin,
                interaction.m_mousePick.m_rayDirection, manipulatorSpace));
        }
    }

    void AngularManipulator::OnMouseMoveImpl(const ViewportInteraction::MouseInteraction& interaction)
    {
        if (m_onMouseMoveCallback)
        {
            // calculate delta rotation
            m_onMouseMoveCallback(CalculateManipulationDataAction(
                m_fixed, m_actionInternal, m_actionInternal.m_start.m_worldFromLocal,
                m_actionInternal.m_start.m_localTransform, interaction.m_mousePick.m_rayOrigin,
                interaction.m_mousePick.m_rayDirection, GetManipulatorSpace(GetManipulatorManagerId())));
        }
    }

    void AngularManipulator::OnLeftMouseUpImpl(const ViewportInteraction::MouseInteraction& interaction)
    {
        if (m_onLeftMouseUpCallback)
        {
            m_onLeftMouseUpCallback(CalculateManipulationDataAction(
                m_fixed, m_actionInternal, m_actionInternal.m_start.m_worldFromLocal,
                m_actionInternal.m_start.m_localTransform, interaction.m_mousePick.m_rayOrigin,
                interaction.m_mousePick.m_rayDirection, GetManipulatorSpace(GetManipulatorManagerId())));
        }
    }

    void AngularManipulator::Draw(
        const ManipulatorManagerState& managerState,
        AzFramework::EntityDebugDisplayRequests& display,
        const ViewportInteraction::CameraState& cameraState,
        const ViewportInteraction::MouseInteraction& mouseInteraction)
    {
        AZ::Transform worldFromLocalWithLocalTransformNormalized = 
            WorldFromLocalWithUniformScale(GetEntityId()) * m_localTransform;
        worldFromLocalWithLocalTransformNormalized.ExtractScale();

        m_manipulatorView->Draw(
            GetManipulatorManagerId(), managerState,
            GetManipulatorId(), { worldFromLocalWithLocalTransformNormalized, AZ::Vector3::CreateZero(), MouseOver() },
            display, cameraState, mouseInteraction, GetManipulatorSpace(GetManipulatorManagerId()));
    }

    void AngularManipulator::SetView(AZStd::unique_ptr<ManipulatorView>&& view)
    {
        m_manipulatorView = AZStd::move(view);
    }

    void AngularManipulator::SetBoundsDirtyImpl()
    {
        m_manipulatorView->SetBoundDirty(GetManipulatorManagerId());
    }

    void AngularManipulator::InvalidateImpl()
    {
        m_manipulatorView->Invalidate(GetManipulatorManagerId());
    }
}