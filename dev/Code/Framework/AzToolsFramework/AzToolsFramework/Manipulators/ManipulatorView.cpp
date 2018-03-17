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

#include "ManipulatorView.h"

#include <AzCore/Math/VectorConversions.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/std/containers/array.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/Manipulators/AngularManipulator.h>
#include <AzToolsFramework/Manipulators/LinearManipulator.h>
#include <AzToolsFramework/Manipulators/LineSegmentSelectionManipulator.h>
#include <AzToolsFramework/Manipulators/PlanarManipulator.h>
#include <AzToolsFramework/Manipulators/SplineSelectionManipulator.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

namespace AzToolsFramework
{
    const float g_defaultManipulatorSphereRadius = 0.1f;

    /**
     * Calculate quad bound in world space.
     */
    static Picking::BoundShapeQuad CalculateQuadBound(
        const AZ::Vector3& localPosition, const AZ::Transform& worldFromLocal,
        const AZ::Vector3& axis1, const AZ::Vector3& axis2, float size)
    {
        const AZ::Vector3 worldPosition = worldFromLocal * localPosition;
        const AZ::Vector3 endAxis1World = worldFromLocal.Multiply3x3(axis1).GetNormalized() * size;
        const AZ::Vector3 endAxis2World = worldFromLocal.Multiply3x3(axis2).GetNormalized() * size;

        Picking::BoundShapeQuad quadBound;
        quadBound.m_corner1 = worldPosition;
        quadBound.m_corner2 = worldPosition + endAxis1World;
        quadBound.m_corner3 = worldPosition + endAxis1World + endAxis2World;
        quadBound.m_corner4 = worldPosition + endAxis2World;
        return quadBound;
    }

    static Picking::BoundShapeQuad CalculateQuadBoundBillboard(
        const AZ::Vector3& localPosition, const AZ::Transform& worldFromLocal,
        float size, const ViewportInteraction::CameraState& cameraState)
    {
        const AZ::Vector3 worldPosition = worldFromLocal * localPosition;

        Picking::BoundShapeQuad quadBound;
        quadBound.m_corner1 = worldPosition - size * cameraState.m_up - size * cameraState.m_side;
        quadBound.m_corner2 = worldPosition - size * cameraState.m_up + size * cameraState.m_side;
        quadBound.m_corner3 = worldPosition + size * cameraState.m_up + size * cameraState.m_side;
        quadBound.m_corner4 = worldPosition + size * cameraState.m_up - size * cameraState.m_side;
        return quadBound;
    }

    /**
     * Calculate line bound in world space (axis and length).
     */
    static Picking::BoundShapeLineSegment CalculateLineBound(
        const AZ::Vector3& localPosition, const AZ::Transform& worldFromLocal,
        const AZ::Vector3& axis, float length, float width)
    {
        Picking::BoundShapeLineSegment lineBound;
        lineBound.m_start = worldFromLocal * localPosition;
        lineBound.m_end = lineBound.m_start + worldFromLocal.Multiply3x3(axis).GetNormalized() * length;
        lineBound.m_width = width;
        return lineBound;
    }

    /**
     * Calculate line bound in world space (start and end point).
     */
    static Picking::BoundShapeLineSegment CalculateLineBound(
        const AZ::Vector3& localStartPosition, const AZ::Vector3& localEndPosition,
        const AZ::Transform& worldFromLocal, float width)
    {
        Picking::BoundShapeLineSegment lineBound;
        lineBound.m_start = worldFromLocal * localStartPosition;
        lineBound.m_end = worldFromLocal * localEndPosition;
        lineBound.m_width = width;
        return lineBound;
    }

    /**
     * Calculate cone bound in world space.
     */
    static Picking::BoundShapeCone CalculateConeBound(
        const AZ::Vector3& localPosition, const AZ::Transform& worldFromLocal,
        const AZ::Vector3& axis, const AZ::Vector3& offset, float length, float radius)
    {
        Picking::BoundShapeCone coneBound;
        coneBound.m_radius = radius;
        coneBound.m_height = length;
        coneBound.m_axis = worldFromLocal.Multiply3x3(axis).GetNormalized();
        coneBound.m_base = worldFromLocal * (localPosition + offset);
        return coneBound;
    }

    /**
     * Calculate box bound in world space.
     */
    static Picking::BoundShapeBox CalculateBoxBound(
        const AZ::Vector3& localPosition, const AZ::Transform& worldFromLocal,
        const AZ::Quaternion& orientation, const AZ::Vector3& offset, const AZ::Vector3& halfExtents)
    {
        Picking::BoundShapeBox boxBound;
        boxBound.m_halfExtents = halfExtents;
        boxBound.m_orientation = (AZ::Quaternion::CreateFromTransform(worldFromLocal) * orientation).GetNormalized();
        boxBound.m_center = worldFromLocal * localPosition + offset;
        return boxBound;
    }

    /**
     * Calculate cylinder bound in world space.
     */
    static Picking::BoundShapeCylinder CalculateCylinderBound(
        const AZ::Vector3& localPosition, const AZ::Transform& worldFromLocal,
        const AZ::Vector3& axis, float length, float radius)
    {
        Picking::BoundShapeCylinder boxBound;
        boxBound.m_axis = worldFromLocal.Multiply3x3(axis).GetNormalized();
        boxBound.m_base = worldFromLocal * localPosition;
        boxBound.m_height = length;
        boxBound.m_radius = radius;
        return boxBound;
    }

    /**
     * Calculate sphere bound in world space.
     */
    static Picking::BoundShapeSphere CalculateSphereBound(
        const AZ::Vector3& localPosition, const AZ::Transform& worldFromLocal, float radius)
    {
        Picking::BoundShapeSphere sphereBound;
        sphereBound.m_center = worldFromLocal * localPosition;
        sphereBound.m_radius = radius;
        return sphereBound;
    }

    /**
     * Calculate torus bound in world space.
     */
    static Picking::BoundShapeTorus CalculateTorusBound(
        const AZ::Vector3& localPosition, const AZ::Transform& worldFromLocal,
        const AZ::Vector3& axis, float radius, float width)
    {
        Picking::BoundShapeTorus torusBound;
        torusBound.m_center = worldFromLocal * localPosition;
        torusBound.m_minorRadius = width;
        torusBound.m_majorRadius = radius;
        torusBound.m_axis = worldFromLocal.Multiply3x3(axis).GetNormalized();
        return torusBound;
    }

    /**
     * Calculate spline bound in world space.
     */
    static Picking::BoundShapeSpline CalculateSplineBound(
        const AZStd::weak_ptr<const AZ::Spline> spline, const AZ::Transform& worldFromLocal, float width)
    {
        Picking::BoundShapeSpline splineBound;
        splineBound.m_spline = spline;
        splineBound.m_width = width;
        splineBound.m_transform = worldFromLocal;
        return splineBound;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    static float LineWidth(bool mouseOver, float defaultWidth, float mouseOverWidth)
    {
        const AZStd::array<float, 2> lineWidth = { { defaultWidth, mouseOverWidth } };
        return lineWidth[mouseOver];
    }

    static AZ::Color ViewColor(
        bool mouseOver, const AZ::Color& defaultColor, const AZ::Color& mouseOverColor)
    {
        const AZStd::array<AZ::Color, 2> viewColor = { { defaultColor, mouseOverColor } };
        return viewColor[mouseOver].GetAsVector4();
    }

    auto defaultLineWidth = [](bool mouseOver)
    {
        return LineWidth(mouseOver, 0.0f, 4.0f);
    };

    ManipulatorView::ManipulatorView() {}

    ManipulatorView::~ManipulatorView() {}

    void ManipulatorView::SetBoundDirty(ManipulatorManagerId managerId)
    {
        ManipulatorManagerRequestBus::Event(managerId,
            &ManipulatorManagerRequestBus::Events::SetBoundDirty,
            m_boundId);

        m_boundDirty = true;
    }

    void ManipulatorView::RefreshBound(
        ManipulatorManagerId managerId, ManipulatorId manipulatorId,
        const Picking::BoundRequestShapeBase& shape)
    {
        ManipulatorManagerRequestBus::EventResult(m_boundId, managerId,
            &ManipulatorManagerRequestBus::Events::UpdateBound,
            manipulatorId, m_boundId, shape);

        m_boundDirty = false;
    }

    void ManipulatorView::Invalidate(ManipulatorManagerId managerId)
    {
        ManipulatorManagerRequestBus::Event(managerId,
            &ManipulatorManagerRequestBus::Events::DeleteManipulatorBound, m_boundId);
        m_boundId = Picking::InvalidBoundId;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    void ManipulatorViewQuad::Draw(
        ManipulatorManagerId managerId, ManipulatorId manipulatorId,
        bool mouseOver, const AZ::Vector3& localPosition, const AZ::Transform& worldFromLocal,
        AzFramework::EntityDebugDisplayRequests& display, const ViewportInteraction::CameraState& cameraState,
        const ViewportInteraction::MouseInteraction& /*mouseInteraction*/, ManipulatorSpace manipulatorSpace)
    {
        const AZ::Transform localFromWorld = worldFromLocal.GetInverseFast();
        const AZ::Vector3 axis1 = Internal::TransformAxisForSpace(
            manipulatorSpace, localFromWorld, m_axis1).GetNormalized();
        const AZ::Vector3 axis2 = Internal::TransformAxisForSpace(
            manipulatorSpace, localFromWorld, m_axis2).GetNormalized();

        const Picking::BoundShapeQuad quadBound =
            CalculateQuadBound(localPosition, worldFromLocal, axis1, axis2,
                m_size * CalculateScreenToWorldMultiplier(
                    m_screenSizeFixed, worldFromLocal * localPosition, cameraState));

        display.SetLineWidth(defaultLineWidth(mouseOver));

        display.SetColor(ViewColor(mouseOver, m_axis1Color, m_mouseOverColor).GetAsVector4());
        display.DrawLine(quadBound.m_corner4, quadBound.m_corner3);

        display.SetColor(ViewColor(mouseOver, m_axis2Color, m_mouseOverColor).GetAsVector4());
        display.DrawLine(quadBound.m_corner2, quadBound.m_corner3);

        if (mouseOver)
        {
            display.SetColor(Vector3ToVector4(m_mouseOverColor.GetAsVector3(), 0.5f));

            display.CullOff();
            display.DrawQuad(
                quadBound.m_corner1, quadBound.m_corner2,
                quadBound.m_corner3, quadBound.m_corner4);
            display.CullOn();
        }

        // update the manipulator's bounds if necessary
        // if m_screenSizeFixed is true, any camera movement can potentially change the size
        // of the manipulator, so we update bounds every frame regardless until we have performance issue
        if (m_screenSizeFixed || m_boundDirty)
        {
            RefreshBound(managerId, manipulatorId, quadBound);
        }
    }

    void ManipulatorViewQuadBillboard::Draw(
        ManipulatorManagerId managerId, ManipulatorId manipulatorId,
        bool mouseOver, const AZ::Vector3& localPosition, const AZ::Transform& worldFromLocal,
        AzFramework::EntityDebugDisplayRequests& display, const ViewportInteraction::CameraState& cameraState,
        const ViewportInteraction::MouseInteraction& /*mouseInteraction*/, ManipulatorSpace /*manipulatorSpace*/)
    {
        const Picking::BoundShapeQuad quadBound =
            CalculateQuadBoundBillboard(localPosition, worldFromLocal,
                m_size * CalculateScreenToWorldMultiplier(
                    m_screenSizeFixed, worldFromLocal * localPosition, cameraState), cameraState);

        display.SetColor(ViewColor(mouseOver, m_color, m_mouseOverColor).GetAsVector4());
        display.DrawQuad(
            quadBound.m_corner1, quadBound.m_corner2,
            quadBound.m_corner3, quadBound.m_corner4);

        if (m_screenSizeFixed || m_boundDirty)
        {
            RefreshBound(managerId, manipulatorId, quadBound);
        }
    }

    void ManipulatorViewLine::Draw(
        ManipulatorManagerId managerId, ManipulatorId manipulatorId,
        bool mouseOver, const AZ::Vector3& localPosition, const AZ::Transform& worldFromLocal,
        AzFramework::EntityDebugDisplayRequests& display, const ViewportInteraction::CameraState& cameraState,
        const ViewportInteraction::MouseInteraction& /*mouseInteraction*/, ManipulatorSpace manipulatorSpace)
    {
        const float viewScale = CalculateScreenToWorldMultiplier(
            m_screenSizeFixed, worldFromLocal * localPosition, cameraState);

        const AZ::Transform localFromWorld = worldFromLocal.GetInverseFast();
        const AZ::Vector3 axis = Internal::TransformAxisForSpace(
            manipulatorSpace, localFromWorld, m_axis).GetNormalized();

        const Picking::BoundShapeLineSegment lineBound =
            CalculateLineBound(localPosition, worldFromLocal, axis,
                m_length * viewScale,
                m_width * viewScale);

        display.SetColor(ViewColor(mouseOver, m_color, m_mouseOverColor).GetAsVector4());
        display.SetLineWidth(defaultLineWidth(mouseOver));
        display.DrawLine(lineBound.m_start, lineBound.m_end);

        if (m_screenSizeFixed || m_boundDirty)
        {
            RefreshBound(managerId, manipulatorId, lineBound);
        }
    }

    void ManipulatorViewLineSelect::Draw(
        ManipulatorManagerId managerId, ManipulatorId manipulatorId,
        bool mouseOver, const AZ::Vector3& localPosition, const AZ::Transform& worldFromLocal,
        AzFramework::EntityDebugDisplayRequests& display, const ViewportInteraction::CameraState& cameraState,
        const ViewportInteraction::MouseInteraction& mouseInteraction, ManipulatorSpace /*manipulatorSpace*/)
    {
        const float viewScale = CalculateScreenToWorldMultiplier(
            /*@param screenSizeFixed=*/true, worldFromLocal * localPosition, cameraState);

        const Picking::BoundShapeLineSegment lineBound =
            CalculateLineBound(m_localStart, m_localEnd, worldFromLocal, m_width * viewScale);

        if (m_boundDirty)
        {
            RefreshBound(managerId, manipulatorId, lineBound);
        }
            
        if (mouseOver)
        {
            const LineSegmentSelectionManipulator::Action action = CalculateManipulationDataAction(
                worldFromLocal, mouseInteraction.m_mousePick.m_rayOrigin, 
                mouseInteraction.m_mousePick.m_rayDirection,
                cameraState.m_farClip, m_localStart, m_localEnd);
            
            const AZ::Vector3 worldLineHitPosition = worldFromLocal * action.m_localLineHitPosition;
            display.SetColor(AZ::Vector4(0.0f, 1.0f, 0.0f, 1.0f));
            display.DrawBall(
                worldLineHitPosition,
                CalculateScreenToWorldMultiplier(
                    true, worldLineHitPosition, cameraState) * g_defaultManipulatorSphereRadius);
        }
    }

    void ManipulatorViewCone::Draw(
        ManipulatorManagerId managerId, ManipulatorId manipulatorId,
        bool mouseOver, const AZ::Vector3& localPosition, const AZ::Transform& worldFromLocal,
        AzFramework::EntityDebugDisplayRequests& display, const ViewportInteraction::CameraState& cameraState,
        const ViewportInteraction::MouseInteraction& /*mouseInteraction*/, ManipulatorSpace manipulatorSpace)
    {
        const float viewScale = CalculateScreenToWorldMultiplier(
            m_screenSizeFixed, worldFromLocal * localPosition, cameraState);

        const AZ::Vector3 scale = worldFromLocal.RetrieveScale();
        const AZ::Transform localFromWorld = worldFromLocal.GetInverseFast();
        const AZ::Vector3 axis = Internal::TransformAxisForSpace(
            manipulatorSpace, localFromWorld, m_axis).GetNormalized();
        const AZ::Vector3 offset = Internal::TransformAxisForSpace(
            manipulatorSpace, localFromWorld, m_offset);

        const Picking::BoundShapeCone coneBound =
            CalculateConeBound(localPosition, worldFromLocal, axis,
                offset * scale.GetReciprocal() * viewScale,
                m_length * viewScale,
                m_radius * viewScale);

        display.SetColor(ViewColor(mouseOver, m_color, m_mouseOverColor).GetAsVector4());
        display.DrawCone(
            coneBound.m_base, coneBound.m_axis, coneBound.m_radius, coneBound.m_height);

        if (m_screenSizeFixed || m_boundDirty)
        {
            RefreshBound(managerId, manipulatorId, coneBound);
        }
    }

    void ManipulatorViewBox::Draw(
        ManipulatorManagerId managerId, ManipulatorId manipulatorId,
        bool mouseOver, const AZ::Vector3& localPosition, const AZ::Transform& worldFromLocal,
        AzFramework::EntityDebugDisplayRequests& display, const ViewportInteraction::CameraState& cameraState,
        const ViewportInteraction::MouseInteraction& /*mouseInteraction*/, ManipulatorSpace manipulatorSpace)
    {
        const float viewScale = CalculateScreenToWorldMultiplier(
            m_screenSizeFixed, worldFromLocal * localPosition, cameraState);

        const AZ::Transform localFromWorld = worldFromLocal.GetInverseFast();
        const AZ::Quaternion orientation = [this, manipulatorSpace, &localFromWorld]() -> AZ::Quaternion {
            switch (manipulatorSpace)
            {
            case ManipulatorSpace::World:
                return AZ::Quaternion::CreateFromTransform(localFromWorld) * m_orientation;
            case ManipulatorSpace::Local:
                // fallthrough
            default:
                return m_orientation;
            }
        }();

        const AZ::Vector3 offset = Internal::TransformAxisForSpace(
            manipulatorSpace, localFromWorld, m_offset);

        const Picking::BoundShapeBox boxBound =
            CalculateBoxBound(localPosition, worldFromLocal, orientation,
                offset * viewScale,
                m_halfExtents * viewScale);

        const AZ::Vector3 xAxis = boxBound.m_orientation * AZ::Vector3::CreateAxisX();
        const AZ::Vector3 yAxis = boxBound.m_orientation * AZ::Vector3::CreateAxisY();
        const AZ::Vector3 zAxis = boxBound.m_orientation * AZ::Vector3::CreateAxisZ();

        display.SetColor(ViewColor(mouseOver, m_color, m_mouseOverColor).GetAsVector4());
        display.DrawSolidOBB(boxBound.m_center,
            xAxis, yAxis, zAxis, boxBound.m_halfExtents);

        if (m_screenSizeFixed || m_boundDirty)
        {
            RefreshBound(managerId, manipulatorId, boxBound);
        }
    }

    void ManipulatorViewCylinder::Draw(
        ManipulatorManagerId managerId, ManipulatorId manipulatorId,
        bool mouseOver, const AZ::Vector3& localPosition, const AZ::Transform& worldFromLocal,
        AzFramework::EntityDebugDisplayRequests& display, const ViewportInteraction::CameraState& cameraState,
        const ViewportInteraction::MouseInteraction& /*mouseInteraction*/, ManipulatorSpace manipulatorSpace)
    {
        const float viewScale = CalculateScreenToWorldMultiplier(
            m_screenSizeFixed, worldFromLocal * localPosition, cameraState);

        const AZ::Transform localFromWorld = worldFromLocal.GetInverseFast();
        const AZ::Vector3 axis = Internal::TransformAxisForSpace(
            manipulatorSpace, localFromWorld, m_axis);

        const Picking::BoundShapeCylinder cylinderBound =
            CalculateCylinderBound(
                localPosition, worldFromLocal, axis,
                m_length * viewScale,
                m_radius * viewScale);

        display.SetColor(ViewColor(mouseOver, m_color, m_mouseOverColor).GetAsVector4());
        display.DrawSolidCylinder(cylinderBound.m_base + cylinderBound.m_axis * cylinderBound.m_height * 0.5f,
            cylinderBound.m_axis, cylinderBound.m_radius, cylinderBound.m_height);

        if (m_screenSizeFixed || m_boundDirty)
        {
            RefreshBound(managerId, manipulatorId, cylinderBound);
        }
    }

    void ManipulatorViewSphere::Draw(
        ManipulatorManagerId managerId, ManipulatorId manipulatorId,
        bool mouseOver, const AZ::Vector3& localPosition, const AZ::Transform& worldFromLocal,
        AzFramework::EntityDebugDisplayRequests& display, const ViewportInteraction::CameraState& cameraState,
        const ViewportInteraction::MouseInteraction& mouseInteraction, ManipulatorSpace /*manipulatorSpace*/)
    {
        const Picking::BoundShapeSphere sphereBound =
            CalculateSphereBound(localPosition, worldFromLocal,
                m_radius * CalculateScreenToWorldMultiplier(
                    m_screenSizeFixed, worldFromLocal * localPosition, cameraState));

        display.SetColor(m_decideColorFn(mouseInteraction, mouseOver, m_color).GetAsVector4());
        display.DrawBall(sphereBound.m_center, sphereBound.m_radius);

        if (m_screenSizeFixed || m_boundDirty)
        {
            RefreshBound(managerId, manipulatorId, sphereBound);
        }
    }

    void ManipulatorViewCircle::Draw(
        ManipulatorManagerId managerId, ManipulatorId manipulatorId,
        bool mouseOver, const AZ::Vector3& localPosition, const AZ::Transform& worldFromLocal,
        AzFramework::EntityDebugDisplayRequests& display, const ViewportInteraction::CameraState& cameraState,
        const ViewportInteraction::MouseInteraction& /*mouseInteraction*/, ManipulatorSpace manipulatorSpace)
    {
        const float viewScale = CalculateScreenToWorldMultiplier(
            m_screenSizeFixed, worldFromLocal * localPosition, cameraState);

        const AZ::Transform localFromWorld = worldFromLocal.GetInverseFast();
        const AZ::Vector3 axis = Internal::TransformAxisForSpace(
            manipulatorSpace, localFromWorld, m_axis);

        const Picking::BoundShapeTorus torusBound =
            CalculateTorusBound(
                localPosition, worldFromLocal, axis,
                m_radius * viewScale,
                m_width * viewScale);

        // transform circle based on delta between default z up axis and other axes
        const AZ::Transform worldFromLocalWithOrientation = worldFromLocal *
            AZ::Transform::CreateFromQuaternion(AZ::Quaternion::CreateShortestArc(AZ::Vector3::CreateAxisZ(), axis));

        display.PushMatrix(worldFromLocalWithOrientation);
        display.SetColor(ViewColor(mouseOver, m_color, m_mouseOverColor).GetAsVector4());
        display.DrawHalfDottedCircle(localPosition, torusBound.m_majorRadius, worldFromLocalWithOrientation.GetInverseFast() * cameraState.m_location);
        display.PopMatrix();

        if (m_screenSizeFixed || m_boundDirty)
        {
            RefreshBound(managerId, manipulatorId, torusBound);
        }
    }

    void ManipulatorViewSplineSelect::Draw(
        ManipulatorManagerId managerId, ManipulatorId manipulatorId,
        bool mouseOver, const AZ::Vector3& localPosition, const AZ::Transform& worldFromLocal,
        AzFramework::EntityDebugDisplayRequests& display, const ViewportInteraction::CameraState& cameraState,
        const ViewportInteraction::MouseInteraction& mouseInteraction, ManipulatorSpace /*manipulatorSpace*/)
    {
        const float viewScale = CalculateScreenToWorldMultiplier(
            /*@param screenSizeFixed=*/true, worldFromLocal * localPosition, cameraState);

        const Picking::BoundShapeSpline splineBound =
            CalculateSplineBound(m_spline, worldFromLocal, m_width * viewScale);

        if (m_boundDirty)
        {
            RefreshBound(managerId, manipulatorId, splineBound);
        }

        if (mouseOver)
        {
            const SplineSelectionManipulator::Action action = CalculateManipulationDataAction(
                worldFromLocal, mouseInteraction.m_mousePick.m_rayOrigin,
                mouseInteraction.m_mousePick.m_rayDirection, m_spline);

            const AZ::Vector3 worldSplineHitPosition = worldFromLocal * action.m_localSplineHitPosition;
                display.SetColor(m_color.GetAsVector4());
                display.DrawBall(worldSplineHitPosition,
                    CalculateScreenToWorldMultiplier(true, worldSplineHitPosition, cameraState) * g_defaultManipulatorSphereRadius);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    AZStd::unique_ptr<ManipulatorView> CreateManipulatorViewQuad(
        const PlanarManipulator& planarManipulator, const AZ::Color& axis1Color,
        const AZ::Color& axis2Color, float size)
    {
        AZStd::unique_ptr<ManipulatorViewQuad> viewQuad = AZStd::make_unique<ManipulatorViewQuad>();
        viewQuad->m_axis1 = planarManipulator.GetAxis1();
        viewQuad->m_axis2 = planarManipulator.GetAxis2();
        viewQuad->m_size = size;
        viewQuad->m_axis1Color = axis1Color;
        viewQuad->m_axis2Color = axis2Color;
        viewQuad->m_mouseOverColor = BaseManipulator::s_defaultMouseOverColor;
        return AZStd::move(viewQuad);
    }

    AZStd::unique_ptr<ManipulatorView> CreateManipulatorViewQuadBillboard(
        const AZ::Color& color, float size)
    {
        AZStd::unique_ptr<ManipulatorViewQuadBillboard> viewQuad = AZStd::make_unique<ManipulatorViewQuadBillboard>();
        viewQuad->m_size = size;
        viewQuad->m_color = color;
        viewQuad->m_mouseOverColor = BaseManipulator::s_defaultMouseOverColor;
        return AZStd::move(viewQuad);
    }

    AZStd::unique_ptr<ManipulatorView> CreateManipulatorViewLine(
        const LinearManipulator& linearManipulator, const AZ::Color& color, float length, float width)
    {
        AZStd::unique_ptr<ManipulatorViewLine> viewLine = AZStd::make_unique<ManipulatorViewLine>();
        viewLine->m_axis = linearManipulator.GetAxis();
        viewLine->m_length = length;
        viewLine->m_width = width;
        viewLine->m_color = color;
        viewLine->m_mouseOverColor = BaseManipulator::s_defaultMouseOverColor;
        return AZStd::move(viewLine);
    }

    AZStd::unique_ptr<ManipulatorView> CreateManipulatorViewLineSelect(
        const LineSegmentSelectionManipulator& lineSegmentManipulator, const AZ::Color& color,
        float width)
    {
        AZStd::unique_ptr<ManipulatorViewLineSelect> viewLineSelect = AZStd::make_unique<ManipulatorViewLineSelect>();
        viewLineSelect->m_localStart = lineSegmentManipulator.GetStart();
        viewLineSelect->m_localEnd = lineSegmentManipulator.GetEnd();
        viewLineSelect->m_width = width;
        viewLineSelect->m_color - color;
        viewLineSelect->m_mouseOverColor = BaseManipulator::s_defaultMouseOverColor;
        return AZStd::move(viewLineSelect);
    }

    AZStd::unique_ptr<ManipulatorView> CreateManipulatorViewCone(
        const LinearManipulator& linearManipulator, const AZ::Color& color,
        const AZ::Vector3& offset, float length, float radius)
    {
        AZStd::unique_ptr<ManipulatorViewCone> viewCone = AZStd::make_unique<ManipulatorViewCone>();
        viewCone->m_axis = linearManipulator.GetAxis();
        viewCone->m_length = length;
        viewCone->m_radius = radius;
        viewCone->m_offset = offset;
        viewCone->m_color = color;
        viewCone->m_mouseOverColor = BaseManipulator::s_defaultMouseOverColor;
        return AZStd::move(viewCone);
    }

    AZStd::unique_ptr<ManipulatorView> CreateManipulatorViewBox(
        const AZ::Transform& transform, const AZ::Color& color,
        const AZ::Vector3& offset, const AZ::Vector3& halfExtents)
    {
        AZStd::unique_ptr<ManipulatorViewBox> viewBox = AZStd::make_unique<ManipulatorViewBox>();
        viewBox->m_orientation = AZ::Quaternion::CreateFromTransform(transform);
        viewBox->m_halfExtents = halfExtents;
        viewBox->m_offset = offset;
        viewBox->m_color = color;
        viewBox->m_mouseOverColor = BaseManipulator::s_defaultMouseOverColor;
        return AZStd::move(viewBox);
    }

    AZStd::unique_ptr<ManipulatorView> CreateManipulatorViewCylinder(
        const LinearManipulator& linearManipulator,
        const AZ::Color& color, float length, float radius)
    {
        AZStd::unique_ptr<ManipulatorViewCylinder> viewClinder = AZStd::make_unique<ManipulatorViewCylinder>();
        viewClinder->m_axis = linearManipulator.GetAxis();
        viewClinder->m_radius = radius;
        viewClinder->m_length = length;
        viewClinder->m_color = color;
        viewClinder->m_mouseOverColor = BaseManipulator::s_defaultMouseOverColor;
        return AZStd::move(viewClinder);
    }

    AZStd::unique_ptr<ManipulatorView> CreateManipulatorViewSphere(
        const AZ::Color& color, float radius, DecideColorFn decideColor)
    {
        AZStd::unique_ptr<ManipulatorViewSphere> viewSphere = AZStd::make_unique<ManipulatorViewSphere>();
        viewSphere->m_radius = radius;
        viewSphere->m_color = color;
        viewSphere->m_mouseOverColor = BaseManipulator::s_defaultMouseOverColor;
        viewSphere->m_decideColorFn = decideColor;
        return AZStd::move(viewSphere);
    }

    AZStd::unique_ptr<ManipulatorView> CreateManipulatorViewCircle(
        const AngularManipulator& angularManipulator, const AZ::Color& color, float radius, float width)
    {
        AZStd::unique_ptr<ManipulatorViewCircle> viewCircle = AZStd::make_unique<ManipulatorViewCircle>();
        viewCircle->m_axis = angularManipulator.GetAxis();
        viewCircle->m_color = color;
        viewCircle->m_radius = radius;
        viewCircle->m_width = width;
        return AZStd::move(viewCircle);
    }

    AZStd::unique_ptr<ManipulatorView> CreateManipulatorViewSplineSelect(
        const SplineSelectionManipulator& splineManipulator, const AZ::Color& color, float width)
    {
        AZStd::unique_ptr<ManipulatorViewSplineSelect> viewSplineSelect = AZStd::make_unique<ManipulatorViewSplineSelect>();
        viewSplineSelect->m_spline = splineManipulator.GetSpline();
        viewSplineSelect->m_color = color;
        viewSplineSelect->m_width = width;
        return AZStd::move(viewSplineSelect);
    }
}