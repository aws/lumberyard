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

struct GizmoRotationHandler
    : public IGizmoMouseDragHandler
{
    explicit GizmoRotationHandler(const GizmoTransformConstraint& constraint);

    bool Begin(const SMouseEvent& ev, Matrix34 matStart) override;
    void Update(const SMouseEvent& ev) override;
    void End(const SMouseEvent& ev) override;
    Matrix34 GetMatrix() const override;

private:
    GizmoTransformConstraint m_constraint;
    Matrix33 m_ManipulatedRot;
    Matrix34 m_ManipulatedMatrix;
    Vec2i m_MouseStart;
};