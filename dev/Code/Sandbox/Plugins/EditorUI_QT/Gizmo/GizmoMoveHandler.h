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
#pragma once

 
#include "IGizmoMouseDragHandler.h"
#include "GizmoTransformConstraint.h"

#include "../../EditorCommon/EditorCommonAPI.h"
class EDITOR_COMMON_API QViewport;

struct GizmoMoveHandler
    : IGizmoMouseDragHandler
{
    explicit GizmoMoveHandler(const GizmoTransformConstraint& constraint);

    bool Begin(const SMouseEvent& ev, Matrix34 matStart) override;
    void Update(const SMouseEvent& ev) override;
    void End(const SMouseEvent& ev) override;
    Matrix34 GetMatrix() const override;
private:
    bool ReprojectStartPoint(QViewport* viewport);
    void Move(Vec3 delta);

    Vec3 m_ManipulatedPoint;
    Matrix34 m_ManipulatedMatrix;
    Vec2i m_LastMouse;
    GizmoTransformConstraint m_constraint;
};
