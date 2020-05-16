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
#include <EditorUI_QT_Precompiled.h>

//Cry
#include <Cry_GeoIntersect.h>

//Editor
#include <Util/Math.h>
#include <Util/EditorUtils.h>

//EditorCommon
#include <QViewportEvents.h>
#include <QViewport.h>

//Local
#include "GizmoMoveHandler.h"

GizmoMoveHandler::GizmoMoveHandler(const GizmoTransformConstraint& constraint)
    : m_constraint(constraint)
    , m_LastMouse(ZERO)
    , m_ManipulatedPoint(ZERO)
    , m_ManipulatedMatrix(IDENTITY)
{
}

bool GizmoMoveHandler::Begin(const SMouseEvent& ev, Matrix34 matStart)
{
    m_LastMouse = Vec2i(ev.x, ev.y);
    m_ManipulatedMatrix = matStart;
    m_ManipulatedPoint = m_ManipulatedMatrix.GetTranslation();

    if (!ReprojectStartPoint(ev.viewport))
    {
        return false;
    }
    return true;
}

void GizmoMoveHandler::Update(const SMouseEvent& ev)
{
    if (m_LastMouse != Vec2i(ev.x, ev.y))
    {
        ReprojectStartPoint(ev.viewport);

        Ray ray;
        MAKE_SURE(ev.viewport->ScreenToWorldRay(&ray, ev.x, ev.y), return );

        if (m_constraint.type == GizmoTransformConstraint::PLANE)
        {
            Vec3 point(0.0f, 0.0f, 0.0f);
            Intersect::Ray_Plane(ray, m_constraint.plane, point, false);

            Vec3 delta = point - m_ManipulatedPoint;
            Move(delta);
        }
        else if (m_constraint.type == GizmoTransformConstraint::AXIS)
        {
            Vec3 point = m_ManipulatedPoint;
            Vec3 axis = m_constraint.GetAxis();
            RayToLineDistance(ray.origin, ray.origin + ray.direction, m_ManipulatedPoint - axis * 10000.0f, m_ManipulatedPoint + axis * 10000.0f, point);
            Vec3 newPoint = m_ManipulatedPoint + axis * axis.dot(point - m_ManipulatedPoint);
            Vec3 delta = newPoint - m_ManipulatedPoint;
            Move(delta);
        }

        m_LastMouse.set(ev.x, ev.y);
    }
}

void GizmoMoveHandler::End(const SMouseEvent& ev)
{
}

Matrix34 GizmoMoveHandler::GetMatrix() const
{
    return m_ManipulatedMatrix;
}

bool GizmoMoveHandler::ReprojectStartPoint(QViewport* viewport)
{
    Ray ray;
    MAKE_SURE(viewport->ScreenToWorldRay(&ray, m_LastMouse.x, m_LastMouse.y), return false);

    if (m_constraint.type == GizmoTransformConstraint::PLANE)
    {
        Intersect::Ray_Plane(ray, m_constraint.plane, m_ManipulatedPoint, false);
    }
    else if (m_constraint.type == GizmoTransformConstraint::AXIS)
    {
        Vec3 point;
        Vec3 axis = m_constraint.GetAxis();
        RayToLineDistance(ray.origin, ray.origin + ray.direction, m_ManipulatedPoint - axis * 10000.0f, m_ManipulatedPoint + axis * 10000.0f, point);
        m_ManipulatedPoint = m_ManipulatedPoint + axis * axis.dot(point - m_ManipulatedPoint);
    }
    return true;
}

void GizmoMoveHandler::Move(Vec3 delta)
{
    Vec3 pos = m_ManipulatedMatrix.GetTranslation();
    pos += delta;
    m_ManipulatedMatrix.SetTranslation(pos);
}
