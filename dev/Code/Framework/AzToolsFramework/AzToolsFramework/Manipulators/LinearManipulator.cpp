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

#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/Picking/ContextBoundAPI.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

#include "LinearManipulator.h"

namespace AzToolsFramework
{
    LinearManipulator::LinearManipulator(AZ::EntityId entityId)
        : BaseManipulator(entityId)
    {

    }

    LinearManipulator::~LinearManipulator()
    {

    }

    void LinearManipulator::InstallMouseDownCallback(MouseActionCallback onMouseDownCallback)
    {
        m_onMouseDownCallback = onMouseDownCallback;
    }

    void LinearManipulator::InstallMouseMoveCallback(MouseActionCallback onMouseMoveCallback)
    {
        m_onMouseMoveCallback = onMouseMoveCallback;
    }

    void LinearManipulator::InstallMouseUpCallback(MouseActionCallback onMouseUpCallback)
    {
        m_onMouseUpCallback = onMouseUpCallback;
    }

    void LinearManipulator::OnMouseDown(const ViewportInteraction::MouseInteraction& interaction, float /*t*/)
    {
        if (!IsRegistered())
        {
            return;
        }

        if (m_isPerformingOperation)
        {
            AZ_Warning("Manipulators", false, "MouseDown action received, but the manipulator (id: %d) is still performing!", GetManipulatorId());
            return;
        }
        m_isPerformingOperation = true;

        if (m_onMouseDownCallback)
        {   
            AzToolsFramework::ManipulatorManagerRequestBus::Event(m_manipulatorManagerId,
                &AzToolsFramework::ManipulatorManagerRequestBus::Events::SetActiveManipulator, this);

            ViewportInteraction::CameraState cameraState;
            ViewportInteractionRequestBus::EventResult(cameraState, interaction.m_viewportID, &ViewportInteractionRequestBus::Events::GetCameraState);
            m_cameraFarClip = cameraState.m_farClip;

            const AZ::VectorFloat EPSILON(0.00001f);

            // Compute the normal of the plane on which the manipulator lies. Try to find the normal
            // that's as much parallel to m_rayDirection as possible, so that following ray cast in 
            // OnMouseMove would have intersection on the plane.
            m_manipulatorPlaneNormalWorld = m_manipulatorDirectionWorld.Cross(interaction.m_rayDirection);
            if (!m_manipulatorPlaneNormalWorld.IsZero(EPSILON))
            {
                m_manipulatorPlaneNormalWorld = m_manipulatorPlaneNormalWorld.Cross(m_manipulatorDirectionWorld);
                m_manipulatorPlaneNormalWorld.NormalizeExact();
            }
            else
            {
                m_manipulatorPlaneNormalWorld = AZ::Vector3::CreateZero();
            }

            Internal::CalcRayPlaneIntersectingPoint(interaction.m_rayOrigin, interaction.m_rayDirection, m_cameraFarClip,
                m_manipulatorOriginWorld, m_manipulatorPlaneNormalWorld, m_currentHitWorldPosition);
            m_startHitWorldPosition = m_currentHitWorldPosition;

            if (m_onMouseDownCallback)
            {
                LinearManipulationData data;
                data.m_manipulatorId = GetManipulatorId();
                data.m_localManipulatorDirection = m_direction;
                data.m_manipulationWorldDirection = m_manipulatorDirectionWorld;
                data.m_screenToWorldMultiplier = m_screenToWorldMultiplier;
                data.m_totalTranslationWorldDelta = 0.0f;

                m_onMouseDownCallback(data);
            }
        }
    }

    void LinearManipulator::OnMouseMove(const ViewportInteraction::MouseInteraction& interaction)
    {
        if (!IsRegistered())
        {
            return;
        }
        
        if (!m_isPerformingOperation)
        {
            AZ_Warning("Manipulators", false, "MouseMove action received, but this manipulator (id: %d) is not performing operations!", GetManipulatorId());
            return;
        }

        if (m_onMouseMoveCallback)
        {
            LinearManipulationData data = GetManipulationData(interaction.m_rayOrigin, interaction.m_rayDirection);
            m_onMouseMoveCallback(data);
        }
    }


    void LinearManipulator::OnMouseUp(const ViewportInteraction::MouseInteraction& interaction)
    {
        if (!IsRegistered())
        {
            return;
        }

        if (!m_isPerformingOperation)
        {
            AZ_Warning("Manipulators", false, "MouseUp action received, but this manipulator (id: %d) didn't receive MouseDown action before!", GetManipulatorId());
            return;
        }
        m_isPerformingOperation = false;
        AzToolsFramework::ManipulatorManagerRequestBus::Event(m_manipulatorManagerId,
            &AzToolsFramework::ManipulatorManagerRequestBus::Events::SetActiveManipulator, nullptr);
        
        if (m_onMouseUpCallback)
        {
            LinearManipulationData data = GetManipulationData(interaction.m_rayOrigin, interaction.m_rayDirection);
            m_onMouseUpCallback(data);
        }
    }

    void LinearManipulator::OnMouseOver(ManipulatorId manipulatorId)
    {   
        if (!IsRegistered())
        {
            return;
        }
        BaseManipulator::OnMouseOver(manipulatorId);
    }

    void LinearManipulator::Invalidate()
    {
        ManipulatorManagerRequestBus::Event(m_manipulatorManagerId, &ManipulatorManagerRequestBus::Events::DeleteManipulatorBound, m_cylinderBoundId);
        m_cylinderBoundId = Picking::InvalidBoundId;
        ManipulatorManagerRequestBus::Event(m_manipulatorManagerId, &ManipulatorManagerRequestBus::Events::DeleteManipulatorBound, m_coneBoundId);
        m_coneBoundId = Picking::InvalidBoundId;

        BaseManipulator::Invalidate();
    }

    void LinearManipulator::SetBoundsDirty()
    {
        ManipulatorManagerRequestBus::Event(m_manipulatorManagerId, &ManipulatorManagerRequestBus::Events::SetBoundDirty, m_cylinderBoundId);
        ManipulatorManagerRequestBus::Event(m_manipulatorManagerId, &ManipulatorManagerRequestBus::Events::SetBoundDirty, m_coneBoundId);
        BaseManipulator::SetBoundsDirty();
    }

    void LinearManipulator::Draw(AzFramework::EntityDebugDisplayRequests& display, const ViewportInteraction::CameraState& cameraState)
    {
        if (!IsRegistered())
        {
            return;
        }

        AZ::Transform entityWorldTM;
        AZ::TransformBus::EventResult(entityWorldTM, m_entityId, &AZ::TransformBus::Events::GetWorldTM);
        m_manipulatorOriginWorld = entityWorldTM * m_origin;

        float projectedDistanceToCamera = cameraState.m_forward.Dot(m_manipulatorOriginWorld - cameraState.m_location);
        if (projectedDistanceToCamera < cameraState.m_nearClip)
        {
            SetBoundsDirty();
            return;
        }

        ManipulatorReferenceSpace referenceSpace = ManipulatorReferenceSpace::Local;
        ManipulatorManagerRequestBus::EventResult(referenceSpace, m_manipulatorManagerId, &ManipulatorManagerRequestBus::Events::GetManipulatorReferenceSpace);

        if (m_displayType == DisplayType::SquarePoint)
        {
            // Always define the manipulation direction in local space, if the LinearManipulator is displayed as SquarePoint.
            m_manipulatorDirectionWorld = entityWorldTM * m_direction - entityWorldTM.GetPosition();
            m_manipulatorDirectionWorld.NormalizeExact();
        }
        else
        {   
            Internal::TransformDirectionBasedOnReferenceSpace(referenceSpace, entityWorldTM, m_direction, m_manipulatorDirectionWorld);
        }

        if (m_isSizeFixedInScreen)
        {
            m_screenToWorldMultiplier = Internal::CalcScreenToWorldMultiplier(projectedDistanceToCamera, cameraState);
        }
        else
        {
            m_screenToWorldMultiplier = 1.0f;
        }

        /* draw manipulators and update their bounds */

        if (m_isMouseOver)
        {
            display.SetColor(m_mouseOverColor.GetAsVector4());
        }
        else
        {
            display.SetColor(m_color.GetAsVector4());
        }

        switch (m_displayType)
        {
            case DisplayType::Arrow:
            {
                float cylinderLength_world = m_length * m_screenToWorldMultiplier;
                float cylinderRadius_world = m_width * m_screenToWorldMultiplier * 0.5f;
                float coneHeight_world = cylinderLength_world * 0.5f;
                float coneRadius_world = cylinderRadius_world * 2.7f;

                AZ::Vector3 cylinderEnd_world = m_manipulatorOriginWorld + cylinderLength_world * m_manipulatorDirectionWorld;
                AZ::Vector3 cylinderCenter_world = m_manipulatorOriginWorld + cylinderLength_world * 0.5f * m_manipulatorDirectionWorld;
                display.DrawSolidCylinder(cylinderCenter_world, m_manipulatorDirectionWorld, cylinderRadius_world, cylinderLength_world);
                display.DrawCone(cylinderEnd_world, m_manipulatorDirectionWorld, coneRadius_world, coneHeight_world);

                /* Update the manipulator's bounds if necessary. */

                // If m_isSizeFixedInScreen is true, any camera movement can potentially change the size of the 
                // manipulator. So we update bounds every frame regardless until we notice performance issue.
                if (m_isSizeFixedInScreen || m_isBoundsDirty)
                {
                    Picking::BoundShapeCylinder cylinderBoundData;
                    cylinderBoundData.m_height = cylinderLength_world;
                    cylinderBoundData.m_radius = cylinderRadius_world;
                    cylinderBoundData.m_transform = AZ::Transform::Identity();
                    cylinderBoundData.m_transform.SetPosition(m_manipulatorOriginWorld);
                    cylinderBoundData.m_transform.SetBasisZ(m_manipulatorDirectionWorld);
                    ManipulatorManagerRequestBus::EventResult(m_cylinderBoundId, m_manipulatorManagerId, &ManipulatorManagerRequestBus::Events::UpdateManipulatorBound,
                        m_manipulatorId, m_cylinderBoundId, cylinderBoundData);

                    Picking::BoundShapeCone coneBoundData;
                    coneBoundData.m_height = coneHeight_world;
                    coneBoundData.m_radius = coneRadius_world;
                    coneBoundData.m_transform = AZ::Transform::Identity();
                    coneBoundData.m_transform.SetPosition(m_manipulatorOriginWorld + (cylinderLength_world + coneHeight_world) * m_manipulatorDirectionWorld);
                    coneBoundData.m_transform.SetBasisZ(-m_manipulatorDirectionWorld);
                    ManipulatorManagerRequestBus::EventResult(m_coneBoundId, m_manipulatorManagerId, &ManipulatorManagerRequestBus::Events::UpdateManipulatorBound,
                        m_manipulatorId, m_coneBoundId, coneBoundData);

                    m_isBoundsDirty = false;
                }
            } break;

            case DisplayType::Cube:
            {
                float cylinderLength_world = m_length * m_screenToWorldMultiplier;
                float cylinderRadius_world = m_width * m_screenToWorldMultiplier * 0.5f;
                float cubeHalfSize_world = cylinderRadius_world * 3.0f;

                AZ::Vector3 cylinderEnd_world = m_manipulatorOriginWorld + cylinderLength_world * m_manipulatorDirectionWorld;
                AZ::Vector3 cylinderCenter_world = m_manipulatorOriginWorld + cylinderLength_world * 0.5f * m_manipulatorDirectionWorld;
                display.DrawSolidCylinder(cylinderCenter_world, m_manipulatorDirectionWorld, cylinderRadius_world, cylinderLength_world);

                AZ::Vector3 cubeUpAxisWorld;
                Internal::TransformDirectionBasedOnReferenceSpace(referenceSpace, entityWorldTM, m_upDir, cubeUpAxisWorld);
                AZ::Vector3 cubeSideAxisWorld;
                Internal::TransformDirectionBasedOnReferenceSpace(referenceSpace, entityWorldTM, m_sideDir, cubeSideAxisWorld);

                AZ::Vector3 cubeHalfExtents(cubeHalfSize_world);
                AZ::Vector3 cubeCenter = cylinderEnd_world + cubeHalfSize_world * m_manipulatorDirectionWorld;
                display.DrawSolidOBB(cubeCenter, cubeSideAxisWorld, m_manipulatorDirectionWorld, cubeUpAxisWorld, cubeHalfExtents);

                if (m_isSizeFixedInScreen || m_isBoundsDirty)
                {
                    Picking::BoundShapeCylinder cylinderBoundData;
                    cylinderBoundData.m_height = cylinderLength_world;
                    cylinderBoundData.m_radius = cylinderRadius_world;
                    cylinderBoundData.m_transform = AZ::Transform::Identity();
                    cylinderBoundData.m_transform.SetPosition(m_manipulatorOriginWorld);
                    cylinderBoundData.m_transform.SetBasisZ(m_manipulatorDirectionWorld);
                    ManipulatorManagerRequestBus::EventResult(m_cylinderBoundId, m_manipulatorManagerId, &ManipulatorManagerRequestBus::Events::UpdateManipulatorBound,
                        m_manipulatorId, m_cylinderBoundId, cylinderBoundData);

                    Picking::BoundShapeBox cubeBoundData;
                    cubeBoundData.m_halfExtents = cubeHalfExtents;
                    cubeBoundData.m_transform = AZ::Transform::Identity();
                    cubeBoundData.m_transform.SetPosition(cubeCenter);
                    cubeBoundData.m_transform.SetBasisX(cubeSideAxisWorld);
                    cubeBoundData.m_transform.SetBasisY(m_manipulatorDirectionWorld);
                    cubeBoundData.m_transform.SetBasisZ(cubeUpAxisWorld);
                    ManipulatorManagerRequestBus::EventResult(m_coneBoundId, m_manipulatorManagerId, &ManipulatorManagerRequestBus::Events::UpdateManipulatorBound,
                        m_manipulatorId, m_coneBoundId, cubeBoundData);

                    m_isBoundsDirty = false;
                }
            } break;

            case DisplayType::SquarePoint:
            {
                float squarePointSize_world = m_squarePointSize * m_screenToWorldMultiplier;
                AZ::Vector3 point1 = m_manipulatorOriginWorld - squarePointSize_world * cameraState.m_up - squarePointSize_world * cameraState.m_side;
                AZ::Vector3 point2 = m_manipulatorOriginWorld - squarePointSize_world * cameraState.m_up + squarePointSize_world * cameraState.m_side;
                AZ::Vector3 point3 = m_manipulatorOriginWorld + squarePointSize_world * cameraState.m_up + squarePointSize_world * cameraState.m_side;
                AZ::Vector3 point4 = m_manipulatorOriginWorld + squarePointSize_world * cameraState.m_up - squarePointSize_world * cameraState.m_side;
                display.CullOff();
                display.DrawQuad(point1, point2, point3, point4);
                display.CullOn();

                if (m_isSizeFixedInScreen || m_isBoundsDirty)
                {
                    Picking::BoundShapeQuad quadBoundData;
                    quadBoundData.m_corner1 = point1;
                    quadBoundData.m_corner2 = point2;
                    quadBoundData.m_corner3 = point3;
                    quadBoundData.m_corner4 = point4;
                    ManipulatorManagerRequestBus::EventResult(m_cylinderBoundId, m_manipulatorManagerId, &ManipulatorManagerRequestBus::Events::UpdateManipulatorBound,
                        m_manipulatorId, m_squarePointBoundId, quadBoundData);

                    m_isBoundsDirty = false;
                }
            } break;
        }
    }

    LinearManipulationData LinearManipulator::GetManipulationData(const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection)
    {
        LinearManipulationData data;
        data.m_manipulatorId = GetManipulatorId();
        data.m_localManipulatorDirection = m_direction;
        data.m_manipulationWorldDirection = m_manipulatorDirectionWorld;
        data.m_screenToWorldMultiplier = m_screenToWorldMultiplier;
        Internal::CalcRayPlaneIntersectingPoint(rayOrigin, rayDirection, m_cameraFarClip, m_manipulatorOriginWorld, m_manipulatorPlaneNormalWorld, m_currentHitWorldPosition);
        data.m_totalTranslationWorldDelta = m_manipulatorDirectionWorld.Dot(m_currentHitWorldPosition - m_startHitWorldPosition);
        return data;
    }
}