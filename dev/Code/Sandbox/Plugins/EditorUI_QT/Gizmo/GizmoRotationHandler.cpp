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

//Local
#include "GizmoRotationHandler.h"

GizmoRotationHandler::GizmoRotationHandler(const GizmoTransformConstraint& constraint)
    : m_constraint(constraint)
    , m_ManipulatedMatrix(IDENTITY)
    , m_ManipulatedRot(IDENTITY)
    , m_MouseStart(ZERO)
{
}

bool GizmoRotationHandler::Begin(const SMouseEvent& ev, Matrix34 matStart)
{
    m_MouseStart = Vec2i(ev.x, ev.y);
    m_ManipulatedMatrix = matStart;
    m_ManipulatedMatrix.GetRotation33(m_ManipulatedRot);
    return true;
}

void GizmoRotationHandler::Update(const SMouseEvent& ev)
{
    if (ev.y != m_MouseStart.y)
    {
        float angle = ((ev.y - m_MouseStart.y) * 0.01f) * 3.1415926f;
        Matrix33 rotation;
        float cos, sin;
        sin = sinf(angle);
        cos = cosf(angle);
        Vec3 axis = m_constraint.GetAxis();
        rotation.SetRotationAA(cos, sin, axis);

        m_ManipulatedRot *= rotation;

        m_MouseStart.y = ev.y;
    }
}

void GizmoRotationHandler::End(const SMouseEvent& ev)
{
}

Matrix34 GizmoRotationHandler::GetMatrix() const
{
    Matrix34 retval(m_ManipulatedRot, m_ManipulatedMatrix.GetTranslation());
    return retval;
}
