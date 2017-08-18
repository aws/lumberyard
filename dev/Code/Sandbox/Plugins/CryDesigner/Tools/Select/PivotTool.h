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
//  (c) 2001 - 2014 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   PivotTool.h
//  Created:     Feb/4/2014 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "Tools/BaseTool.h"

class PivotTool
    : public BaseTool
{
public:

    PivotTool(CD::EDesignerTool tool)
        : BaseTool(tool)
        , m_PivotMoved(false)
    {
    }

    void Enter() override;
    void Leave() override;

    void Display(DisplayContext& dc) override;

    bool OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags) override;
    void OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& point) override;
    void OnMouseMove(CViewport* view, UINT nFlags, const QPoint& point) override;

    enum EPivotSelectionType
    {
        ePST_BoundBox,
        ePST_Designer,
    };
    void SetSelectionType(EPivotSelectionType selectionType, bool bForce = false);

    void OnManipulatorDrag(CViewport* pView, ITransformManipulator* pManipulator, QPoint& p0, QPoint& p1, const BrushVec3& value) override;
    void OnManipulatorMouseEvent(CViewport* pView, ITransformManipulator* pManipulator, EMouseEvent event, QPoint& point, int flags, bool bHitGizmo) override;

private:

    std::vector<BrushVec3> m_CandidateVertices;
    int m_nSelectedCandidate;
    int m_nPivotIndex;
    BrushVec3 m_PivotPos;
    BrushVec3 m_StartingDragManipulatorPos;
    bool      m_PivotMoved;
};
