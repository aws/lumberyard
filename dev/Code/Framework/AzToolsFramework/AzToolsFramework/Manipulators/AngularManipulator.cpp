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
#include <AzCore/Math/IntersectSegment.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

#include "AngularManipulator.h"

namespace AzToolsFramework
{
    namespace Internal
    {
        AZ::Vector2 TransformWorldPositionToScreen(AZ::Vector3 worldPosition, const ViewportInteraction::CameraState& cameraState)
        {
            AZ::Transform worldToCamera = AZ::Transform::CreateFromColumns(cameraState.m_side, cameraState.m_forward, cameraState.m_up, AZ::Vector3::CreateZero());
            worldToCamera.Transpose3x3();
            worldToCamera = worldToCamera * AZ::Transform::CreateTranslation(-cameraState.m_location);

            float viewportWidth = cameraState.m_viewportSize.GetX();
            float viewportHeight = cameraState.m_viewportSize.GetY();

            float nearPlaneHalfHeight = cameraState.m_nearClip * tanf(cameraState.m_verticalFovRadian * 0.5f);
            float viewportHeightOverNearPlaneHeight = (viewportHeight * 0.5f) / nearPlaneHalfHeight;

            AZ::Vector3 cylinderStart_camera = worldToCamera * worldPosition;

            AZ::Vector2 screenPosition;
            if (cameraState.m_isOrthographic)
            {
                float cameraToScreenMultiplier = viewportHeightOverNearPlaneHeight * cameraState.m_zoom;

                screenPosition = AZ::Vector2(cameraToScreenMultiplier * cylinderStart_camera.GetX() + viewportWidth * 0.5f,
                    viewportHeight * 0.5f - cameraToScreenMultiplier * cylinderStart_camera.GetZ());
            }
            else
            {
                float cylinderStartDistanceY = cylinderStart_camera.GetY();
                float cameraToScreenMultiplier = viewportHeightOverNearPlaneHeight * cameraState.m_nearClip / cylinderStartDistanceY;
                screenPosition = AZ::Vector2(cameraToScreenMultiplier * cylinderStart_camera.GetX() + viewportWidth * 0.5f,
                    viewportHeight * 0.5f - cameraToScreenMultiplier * cylinderStart_camera.GetZ());
            }

            return screenPosition;
        }

        // Calculate the angle between the vector from center to mousePosition and baseRadiusVec. The returned
        // angle is in the range [0, 2 * Pi). If the mouse goes counter clock-wise the angle increases.
        float CalcAngleBetweenVectorsOnScreen(const AZ::Vector2& center, const AZ::Vector2& baseRadiusVec, const AZ::Vector2& mousePosition)
        {
            AZ::Vector2 radiusScreenDir = mousePosition - center;
            radiusScreenDir.NormalizeSafe();
            // because in screen space y-axis is pointing down, flip the sign
            AZ::Vector3 refRadiusDir3 = AZ::Vector3(baseRadiusVec.GetX(), -baseRadiusVec.GetY(), 0.0f);
            AZ::Vector3 currentRadiusDir3 = AZ::Vector3(radiusScreenDir.GetX(), -radiusScreenDir.GetY(), 0.0f);
            AZ::Vector3 vectorOutOfScreen = AZ::Vector3(0.0f, 0.0f, 1.0f);

            float cosAngle = refRadiusDir3.Dot(currentRadiusDir3);
            cosAngle = AZ::GetClamp(cosAngle, -1.0f, 1.0f);
            float angleRadian = acosf(cosAngle);
            AZ::Vector3 crossVec = refRadiusDir3.Cross(currentRadiusDir3);
            AZ::VectorFloat side = vectorOutOfScreen.Dot(crossVec);
            if (side.IsLessThan(AZ::VectorFloat::CreateZero()))
            {
                angleRadian = AZ::Constants::TwoPi - angleRadian;
            }
            return angleRadian;
        }

        
        AZ::Transform CaclCircleDrawTransform(ManipulatorReferenceSpace referenceSpace, const AZ::Transform& entityWorldTM, const AZ::Vector3& rotationAxis, const AZ::Vector3& rotationCenterPosition)
        {
            // Compute a basis from a unit vector. 
            // Reference: http://box2d.org/2014/02/computing-a-basis/
            AZ::Vector3 axis2, axis3;
            if (rotationAxis.GetX() >= 0.57735f)
            {
                axis2.Set(rotationAxis.GetY(), -rotationAxis.GetX(), 0.0f);
            }
            else
            {
                axis2.Set(0.0f, rotationAxis.GetZ(), -rotationAxis.GetY());
            }
            axis2.NormalizeExact();
            axis3 = rotationAxis.Cross(axis2);
            axis3.NormalizeExact();

            // Form a matrix that transforms the origin to m_centerPosition and the z-axis to m_rotationAxis.
            AZ::Transform tm = AZ::Transform::CreateFromColumns(axis2, axis3, rotationAxis, rotationCenterPosition);

            if (referenceSpace == ManipulatorReferenceSpace::World)
            {
                tm.SetPosition(tm.GetPosition() + entityWorldTM.GetPosition());
                return tm;
            }
            else // default to (referenceSpace == ManipulatorReferenceSpace::Local)
            {
                AZ::Transform entityWorldNoScale = entityWorldTM;
                entityWorldNoScale.ExtractScaleExact();
                AZ::Transform result = entityWorldNoScale * tm;
                return result;
            }
        }
    }

    AngularManipulator::AngularManipulator(AZ::EntityId entityId)
        : BaseManipulator(entityId)
    {

    }

    AngularManipulator::~AngularManipulator()
    {

    }

    void AngularManipulator::InstallMouseDownCallback(MouseActionCallback onMouseDownCallback)
    {
        m_onMouseDownCallback = onMouseDownCallback;
    }

    void AngularManipulator::InstallMouseMoveCallback(MouseActionCallback onMouseMoveCallback)
    {
        m_onMouseMoveCallback = onMouseMoveCallback;
    }

    void AngularManipulator::InstallMouseUpCallback(MouseActionCallback onMouseUpCallback)
    {
        m_onMouseUpCallback = onMouseUpCallback;
    }

    void AngularManipulator::OnMouseDown(const ViewportInteraction::MouseInteraction& interaction, float t)
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

        m_rotationCenterScreenPosition = Internal::TransformWorldPositionToScreen(m_centerPositionWorld, cameraState);
        m_startMouseScreenPosition = interaction.m_screenCoordinates;
        m_lastMouseScreenPosition = m_startMouseScreenPosition;
        m_currentMouseScreenPosition = m_startMouseScreenPosition;
        m_lastTotalScreenRotationDelta = 0.0f;
        m_totalScreenRotationDelta = 0.0f;
        m_lastTotalWorldRotationDelta = 0.0f;
        m_totalWorldRotationDelta = 0.0f;
        m_startRadiusVector = (m_currentMouseScreenPosition - m_rotationCenterScreenPosition).GetNormalizedSafe();
        m_isRotationAxisWorldTowardsCamera = (cameraState.m_forward.Dot(m_rotationAxisWorld) < 0.0f ? true : false);

        /* Calculate the initial radius vector from the center to the circumference. */
        AZ::Vector3 intersection = interaction.m_rayOrigin + t * interaction.m_rayDirection;
        m_startRadiusVectorWorld = intersection - m_centerPositionWorld;
        // In case the intersection is not exactly on the circumference, we calculate raidus vector by decomposing the rotation axis.
        AZ::VectorFloat perpendicularOffset = m_rotationAxisWorld.Dot(m_startRadiusVectorWorld);
        m_startRadiusVectorWorld -= perpendicularOffset * m_rotationAxisWorld;
        m_startRadiusVectorWorld.NormalizeExact();
        m_startRadiusSideVectorWorld = m_rotationAxisWorld.Cross(m_startRadiusVectorWorld);
        m_startRadiusSideVectorWorld.NormalizeExact();

        if (m_onMouseDownCallback)
        {
            AngularManipulationData data = GetManipulationData();
            m_onMouseDownCallback(data);
        }
    }

    void AngularManipulator::OnMouseMove(const ViewportInteraction::MouseInteraction& interaction)
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

        CalcManipulationData(interaction.m_screenCoordinates);
        if (m_onMouseMoveCallback)
        {
            AngularManipulationData data = GetManipulationData();
            m_onMouseMoveCallback(data);
        }
    }


    void AngularManipulator::OnMouseUp(const ViewportInteraction::MouseInteraction& interaction)
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

        CalcManipulationData(interaction.m_screenCoordinates);
        if (m_onMouseUpCallback)
        {
            AngularManipulationData data = GetManipulationData();
            m_onMouseUpCallback(data);
        }
    }

    void AngularManipulator::OnMouseOver(ManipulatorId manipulatorId)
    {
        if (!IsRegistered())
        {
            return;
        }
        BaseManipulator::OnMouseOver(manipulatorId);
    }

    void AngularManipulator::SetRotationAxis(const AZ::Vector3& axis)
    {
        if (fabsf(axis.GetLengthSq() - 1.0f) < 0.00001f)
        {
            m_rotationAxis = axis;
        }
        else
        {
            m_rotationAxis = axis.GetNormalizedExact();
        }
    }

    void AngularManipulator::Invalidate()
    {
        ManipulatorManagerRequestBus::Event(m_manipulatorManagerId, &ManipulatorManagerRequestBus::Events::DeleteManipulatorBound, m_torusBoundId);
        m_torusBoundId = Picking::InvalidBoundId;

        BaseManipulator::Invalidate();
    }

    void AngularManipulator::SetBoundsDirty()
    {
        ManipulatorManagerRequestBus::Event(m_manipulatorManagerId, &ManipulatorManagerRequestBus::Events::SetBoundDirty, m_torusBoundId);
        BaseManipulator::SetBoundsDirty();
    }

    void AngularManipulator::Draw(AzFramework::EntityDebugDisplayRequests& display, const ViewportInteraction::CameraState& cameraState)
    {
        if (!IsRegistered())
        {
            return;
        }

        AZ::Transform entityWorldTM;
        AZ::TransformBus::EventResult(entityWorldTM, m_entityId, &AZ::TransformBus::Events::GetWorldTM);
        m_centerPositionWorld = entityWorldTM * m_centerPosition;

        float projectedDistanceToCamera = cameraState.m_forward.Dot(m_centerPositionWorld - cameraState.m_location);
        if (projectedDistanceToCamera < cameraState.m_nearClip)
        {
            SetBoundsDirty();
            return;
        }

        ManipulatorReferenceSpace referenceSpace;
        ManipulatorManagerRequestBus::EventResult(referenceSpace, m_manipulatorManagerId, &ManipulatorManagerRequestBus::Events::GetManipulatorReferenceSpace);
        Internal::TransformDirectionBasedOnReferenceSpace(referenceSpace, entityWorldTM, m_rotationAxis, m_rotationAxisWorld);

        // Ignore scaling when m_isSizeFixedInScreen is false.
        float radius_world = m_radius;

        if (m_isSizeFixedInScreen)
        {
            m_screenToWorldMultiplier = Internal::CalcScreenToWorldMultiplier(projectedDistanceToCamera, cameraState);
        }
        else
        {
            m_screenToWorldMultiplier = 1.0f;
        }

        radius_world = m_radius * m_screenToWorldMultiplier;

        /* draw manipulators */

        if (m_isMouseOver)
        {
            display.SetColor(m_mouseOverColor.GetAsVector4());
        }
        else
        {
            display.SetColor(m_color.GetAsVector4());
        }

        const float torusWidth = 7.0f;

        AZ::Transform circleDrawTM = Internal::CaclCircleDrawTransform(referenceSpace, entityWorldTM, m_rotationAxis, m_centerPosition);
        display.PushMatrix(circleDrawTM);
        display.SetLineWidth(torusWidth);
        display.DrawCircle(AZ::Vector3::CreateZero(), radius_world);
        display.PopMatrix();

        float cylinderBoundHeight = torusWidth * m_screenToWorldMultiplier;
        float cylinderBoundRadius = radius_world + cylinderBoundHeight;
        AZ::Vector3 cylinderCenter = m_centerPositionWorld;

        /* Update the manipulator's bounds if necessary. */

        // If m_isSizeFixedInScreen is true, any camera movement can potentially change the size of the 
        // manipulator. So we update bounds every frame regardless until we have performance issue.
        if (m_isSizeFixedInScreen || m_isBoundsDirty)
        {
            Picking::BoundShapeTorus torusBoundData;
            torusBoundData.m_height = cylinderBoundHeight;
            torusBoundData.m_radius = cylinderBoundRadius;
            torusBoundData.m_transform = AZ::Transform::Identity();
            torusBoundData.m_transform.SetPosition(cylinderCenter - 0.5f * cylinderBoundHeight * m_rotationAxisWorld);
            torusBoundData.m_transform.SetBasisZ(m_rotationAxisWorld);

            ManipulatorManagerRequestBus::EventResult(m_torusBoundId, m_manipulatorManagerId, &ManipulatorManagerRequestBus::Events::UpdateManipulatorBound,
                m_manipulatorId, m_torusBoundId, torusBoundData);

            m_isBoundsDirty = false;
        }

        if (IsPerformingOperation())
        {
            /* Draw an arrow at the radiusPoint on circle circumference and line going through the rotation center. */

            AZ::VectorFloat sin, cos;
            AZ::GetSinCos(AZ::VectorFloat(m_totalWorldRotationDelta), sin, cos);
            AZ::Vector3 currentRadiusVector = sin * m_startRadiusSideVectorWorld + cos * m_startRadiusVectorWorld;
            AZ::Vector3 radiusPoint = m_centerPositionWorld + radius_world * currentRadiusVector;
            AZ::Vector3 currentTangentVector = m_rotationAxisWorld.Cross(currentRadiusVector);
            currentTangentVector.NormalizeExact();

            display.SetColor(m_color.GetAsVector4());
            display.DrawCone(radiusPoint, currentTangentVector, cylinderBoundHeight * 1.5f, cylinderBoundHeight * 2.0f);
        }
    }

    void AngularManipulator::CalcManipulationData(const AZ::Vector2& mouseScreenPosition)
    {
        m_lastMouseScreenPosition = m_currentMouseScreenPosition;
        m_currentMouseScreenPosition = mouseScreenPosition;
        m_lastTotalScreenRotationDelta = m_totalScreenRotationDelta;
        m_totalScreenRotationDelta = Internal::CalcAngleBetweenVectorsOnScreen(m_rotationCenterScreenPosition, m_startRadiusVector, m_currentMouseScreenPosition);
        m_lastTotalWorldRotationDelta = m_totalWorldRotationDelta;
        m_totalWorldRotationDelta = (m_isRotationAxisWorldTowardsCamera ? m_totalScreenRotationDelta : AZ::Constants::TwoPi - m_totalScreenRotationDelta);
    }

    AngularManipulationData AngularManipulator::GetManipulationData()
    {
        AngularManipulationData data;
        data.m_manipulatorId = GetManipulatorId();
        data.m_localRotationAxis = m_rotationAxis;
        data.m_startMouseScreenPosition = m_startMouseScreenPosition;
        data.m_lastMouseScreenPosition = m_lastMouseScreenPosition;
        data.m_currentMouseScreenPosition = m_currentMouseScreenPosition;
        data.m_lastTotalScreenRotationDelta = m_lastTotalScreenRotationDelta;
        data.m_totalScreenRotationDelta = m_totalScreenRotationDelta;
        data.m_lastTotalWorldRotationDelta = m_lastTotalWorldRotationDelta;
        data.m_totalWorldRotationDelta = m_totalWorldRotationDelta;
        return data;
    }
}