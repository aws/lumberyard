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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2012 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   MoveTool.h
//  Created:     Sep/1/2012 by Jaesik.
////////////////////////////////////////////////////////////////////////////
#include "Tools/BaseTool.h"
#include "Core/Model.h"
#include "Tools/Select/SelectTool.h"

class MovePipeline;

namespace CD
{
    class Model;
};

#define GENERATE_MOVETOOL_CLASS(type)            \
    class Move##type##Tool                       \
        : public MoveTool                        \
    {                                            \
    public:                                      \
        Move##type##Tool(CD::EDesignerTool tool) \
            : MoveTool(tool){}                   \
    };

class MoveTool
    : public SelectTool
{
public:
    MoveTool(CD::EDesignerTool tool);
    ~MoveTool();

    void Enter() override;
    void OnLButtonDown(CViewport* pView, UINT nFlags, const QPoint& point) override;
    void OnLButtonUp(CViewport* pView, UINT nFlags, const QPoint& point) override;
    void OnMouseMove(CViewport* pView, UINT nFlags, const QPoint& point) override;
    void OnManipulatorDrag(CViewport* pView, ITransformManipulator* pManipulator, QPoint& p0, QPoint& p1, const BrushVec3& value) override;
    void OnManipulatorMouseEvent(CViewport* pView, ITransformManipulator* pManipulator, EMouseEvent event, QPoint& point, int flags, bool bHitGizmo) override;
    void OnEditorNotifyEvent(EEditorNotifyEvent event) override;

    static void Transform(CD::SMainContext& mc, const BrushMatrix34& tm, bool bMoveTogether);

private:

    void StartTransformation(bool bSeparate);
    void TransformSelections(const BrushMatrix34& offsetTM);
    void EndTransformation();

    static void TransformSelections(CD::SMainContext& mc, MovePipeline& pipeline, const BrushMatrix34& offsetTM);

    BrushMatrix34 GetOffsetTMOnAlignedPlane(CViewport* pView, const BrushPlane& planeAlighedWithView, const QPoint& prevPos, const QPoint& currentPos);
    void InitializeMovementOnViewport(CViewport* pView, UINT nMouseFlags);

    BrushPlane m_PlaneAlignedWithView;
    std::unique_ptr<MovePipeline> m_Pipeline;
    bool m_bManipulatingGizmo;
    BrushVec3 m_SelectedElementNormal;
};

GENERATE_MOVETOOL_CLASS(Vertex)
GENERATE_MOVETOOL_CLASS(Edge)
GENERATE_MOVETOOL_CLASS(Face)
GENERATE_MOVETOOL_CLASS(VertexEdge)
GENERATE_MOVETOOL_CLASS(VertexFace)
GENERATE_MOVETOOL_CLASS(EdgeFace)
GENERATE_MOVETOOL_CLASS(VertexEdgeFace)
