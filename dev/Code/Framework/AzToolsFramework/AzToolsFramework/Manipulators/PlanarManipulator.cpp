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

#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

#include "PlanarManipulator.h"

namespace AzToolsFramework
{
    PlanarManipulator::PlanarManipulator(AZ::EntityId entityId)
        : BaseManipulator(entityId)
    {

    }

    void PlanarManipulator::InstallMouseDownCallback(MouseActionCallback onMouseDownCallback)
    {
        m_onMouseDownCallback = onMouseDownCallback;
    }

    void PlanarManipulator::InstallMouseMoveCallback(MouseActionCallback onMouseMoveCallback)
    {
        m_onMouseMoveCallback = onMouseMoveCallback;
    }

    void PlanarManipulator::InstallMouseUpCallback(MouseActionCallback onMouseUpCallback)
    {
        m_onMouseUpCallback = onMouseUpCallback;
    }

    void PlanarManipulator::OnMouseDown(const ViewportInteraction::MouseInteraction& interaction, float /*t*/)
    {
        if (!IsRegistered())
        {
            return;
        }

        if (m_isPerformingOperation)
        {
            AZ_Warning("Manipulators", false, "MouseDown action received, but this manipulator is still performing!");
            return;
        }
        m_isPerformingOperation = true;
        AzToolsFramework::ManipulatorManagerRequestBus::Event(m_manipulatorManagerId,
            &AzToolsFramework::ManipulatorManagerRequestBus::Events::SetActiveManipulator, this);

        ViewportInteraction::CameraState cameraState;
        ViewportInteractionRequestBus::EventResult(cameraState, interaction.m_viewportID, &ViewportInteractionRequestBus::Events::GetCameraState);
        m_cameraFarClip = cameraState.m_farClip;

        m_manipulatorPlaneNormalWorld = m_axis1World.Cross(m_axis2World);
        Internal::CalcRayPlaneIntersectingPoint(interaction.m_rayOrigin, interaction.m_rayDirection, m_cameraFarClip,
            m_originWorld, m_manipulatorPlaneNormalWorld, m_currentHitWorldPosition);
        m_startHitWorldPosition = m_currentHitWorldPosition;

        if (m_onMouseDownCallback)
        {
            PlanarManipulationData data;
            data.m_manipulatorId = GetManipulatorId();
            data.m_startHitWorldPosition = m_startHitWorldPosition;
            data.m_currentHitWorldPosition = m_currentHitWorldPosition;
            data.m_manipulationWorldDirection1 = m_axis1World;
            data.m_manipulationWorldDirection2 = m_axis2World;
            data.m_axesDelta.fill(0.0f);

            m_onMouseDownCallback(data);
        }
    }

    void PlanarManipulator::OnMouseMove(const ViewportInteraction::MouseInteraction& interaction)
    {
        if (!IsRegistered())
        {
            return;
        }

        if (!m_isPerformingOperation)
        {
            AZ_Warning("Manipulators", false, "MouseMove action received, but this manipulator is not performing operations!");
            return;
        }

        if (m_onMouseMoveCallback)
        {
            PlanarManipulationData data = GetManipulationData(interaction.m_rayOrigin, interaction.m_rayDirection);
            m_onMouseMoveCallback(data);
        }
    }


    void PlanarManipulator::OnMouseUp(const ViewportInteraction::MouseInteraction& interaction)
    {
        if (!IsRegistered())
        {
            return;
        }

        if (!m_isPerformingOperation)
        {
            AZ_Warning("Manipulators", false, "MouseUp action received, but this manipulator didn't receive MouseDown action before!");
            return;
        }
        m_isPerformingOperation = false;
        AzToolsFramework::ManipulatorManagerRequestBus::Event(m_manipulatorManagerId,
            &AzToolsFramework::ManipulatorManagerRequestBus::Events::SetActiveManipulator, nullptr);

        if (m_onMouseUpCallback)
        {
            PlanarManipulationData data = GetManipulationData(interaction.m_rayOrigin, interaction.m_rayDirection);
            m_onMouseUpCallback(data);
        }
    }

    void PlanarManipulator::OnMouseOver(ManipulatorId manipulatorId)
    {
        if (!IsRegistered())
        {
            return;
        }
        BaseManipulator::OnMouseOver(manipulatorId);
    }

    void PlanarManipulator::Invalidate()
    {
        ManipulatorManagerRequestBus::Event(m_manipulatorManagerId, &ManipulatorManagerRequestBus::Events::DeleteManipulatorBound, m_quadBoundId);
        m_quadBoundId = Picking::InvalidBoundId;

        BaseManipulator::Invalidate();
    }

    void PlanarManipulator::SetBoundsDirty()
    {
        ManipulatorManagerRequestBus::Event(m_manipulatorManagerId, &ManipulatorManagerRequestBus::Events::SetBoundDirty, m_quadBoundId);
        BaseManipulator::SetBoundsDirty();
    }

    void PlanarManipulator::Draw(AzFramework::EntityDebugDisplayRequests& display, const ViewportInteraction::CameraState& cameraState)
    {
        if (!IsRegistered())
        {
            return;
        }

        AZ::Transform entityWorldTM;
        AZ::TransformBus::EventResult(entityWorldTM, m_entityId, &AZ::TransformBus::Events::GetWorldTM);
        m_originWorld = entityWorldTM * m_origin;

        float projectedDistanceToCamera = cameraState.m_forward.Dot(m_originWorld - cameraState.m_location);
        if (projectedDistanceToCamera < cameraState.m_nearClip)
        {
            SetBoundsDirty();
            return;
        }

        ManipulatorReferenceSpace referenceSpace;
        ManipulatorManagerRequestBus::EventResult(referenceSpace, m_manipulatorManagerId, &ManipulatorManagerRequestBus::Events::GetManipulatorReferenceSpace);
        Internal::TransformDirectionBasedOnReferenceSpace(referenceSpace, entityWorldTM, m_axis1, m_axis1World);
        Internal::TransformDirectionBasedOnReferenceSpace(referenceSpace, entityWorldTM, m_axis2, m_axis2World);

        // Ignore scaling when m_isSizeFixedInScreen is false.
        float lengthAxis1_world = m_lengthAxis1;
        float lengthAxis2_world = m_lengthAxis2;

        if (m_isSizeFixedInScreen)
        {
            m_screenToWorldMultiplier = Internal::CalcScreenToWorldMultiplier(projectedDistanceToCamera, cameraState);
        }
        else
        {
            m_screenToWorldMultiplier = 1.0f;
        }

        lengthAxis1_world = m_lengthAxis1 * m_screenToWorldMultiplier;
        lengthAxis2_world = m_lengthAxis2 * m_screenToWorldMultiplier;

        /* draw manipulators */

        if (m_isMouseOver)
        {
            display.SetColor(m_mouseOverColor.GetAsVector4());
        }
        else
        {
            display.SetColor(m_color.GetAsVector4());
        }

        AZ::Vector3 endAxis1_world = m_originWorld + lengthAxis1_world * m_axis1World;
        AZ::Vector3 endAxis2_world = m_originWorld + lengthAxis2_world * m_axis2World;
        AZ::Vector3 fourthPoint = m_originWorld + lengthAxis1_world * m_axis1World + lengthAxis2_world * m_axis2World;
        display.CullOff();
        display.DrawQuad(m_originWorld, endAxis1_world, fourthPoint, endAxis2_world);
        display.CullOn();

        /* Update the manipulator's bounds if necessary. */

        // If m_isSizeFixedInScreen is true, any camera movement can potentially change the size of the 
        // manipulator. So we update bounds every frame regardless until we have performance issue.
        if (m_isSizeFixedInScreen || m_isBoundsDirty)
        {
            Picking::BoundShapeQuad quadBoundData;
            quadBoundData.m_corner1 = m_originWorld;
            quadBoundData.m_corner2 = endAxis1_world;
            quadBoundData.m_corner3 = fourthPoint;
            quadBoundData.m_corner4 = endAxis2_world;

            ManipulatorManagerRequestBus::EventResult(m_quadBoundId, m_manipulatorManagerId, &ManipulatorManagerRequestBus::Events::UpdateManipulatorBound,
                m_manipulatorId, m_quadBoundId, quadBoundData);

            m_isBoundsDirty = false;
        }
    }

    void PlanarManipulator::SetAxes(const AZ::Vector3& axis1, const AZ::Vector3& axis2)
    {
        m_axis1 = axis1;
        m_axis2 = axis2;
    }

    void PlanarManipulator::SetAxesLength(float length1, float length2)
    {
        m_lengthAxis1 = length1;
        m_lengthAxis2 = length2;
    }

    PlanarManipulationData PlanarManipulator::GetManipulationData(const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection)
    {
        PlanarManipulationData data;
        data.m_manipulatorId = GetManipulatorId();
        data.m_startHitWorldPosition = m_startHitWorldPosition;
        Internal::CalcRayPlaneIntersectingPoint(rayOrigin, rayDirection, m_cameraFarClip, m_originWorld, m_manipulatorPlaneNormalWorld, m_currentHitWorldPosition);
        data.m_currentHitWorldPosition = m_currentHitWorldPosition;
        data.m_manipulationWorldDirection1 = m_axis1World;
        data.m_manipulationWorldDirection2 = m_axis2World;
        AZ::Vector3 worldHitDelta = m_currentHitWorldPosition - m_startHitWorldPosition;
        data.m_axesDelta[0] = m_axis1World.Dot(worldHitDelta);
        data.m_axesDelta[1] = m_axis2World.Dot(worldHitDelta);
        
        return data;
    }
}