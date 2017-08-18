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
//  (c) 2001 - 2013 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   PolylineTool.h
//  Created:     May/7/2013 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "ShapeTool.h"

class PolylineTool
    : public ShapeTool
{
public:

    PolylineTool(CD::EDesignerTool tool)
        : ShapeTool(tool)
        , m_bAlignedToRecentSpotLine(false)
    {
    }

    virtual ~PolylineTool(){}
    virtual void Enter() override;
    virtual void Leave() override;

    void OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& point) override;
    void OnLButtonUp(CViewport* view, UINT nFlags, const QPoint& point) override;
    void OnMouseMove(CViewport* view, UINT nFlags, const QPoint& point) override;
    virtual void Display(DisplayContext& dc) override;
    virtual bool OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags) override;

    bool IsPhaseFirstStepOnPrimitiveCreation() const override;

protected:

    virtual void CreatePolygonFromSpots(bool bClosePolygon, const SpotList& spotList);
    bool IntersectExisintingLines(const BrushVec3& v0, const BrushVec3& v1, int* pOutSpotIndex = NULL) const;

    ELineState GetLineState() const{ return m_LineState; }
    void SetLineState(ELineState lineState){ m_LineState = lineState; }

    ELineState GetAlienedPointWithAxis(const BrushVec3& v0, const BrushVec3& v1, const BrushPlane& plane, IDisplayViewport* view, std::vector<BrushEdge3D>* pAxisList, BrushVec3& outPos) const;
    void AddPolygonWithCurrentSameAsFirst();

    void RegisterEitherEndSpotList();
    void RegisterSpotListAfterBreaking();

    void AlignEdgeWithPrincipleAxises(IDisplayViewport* view, bool bAlign);
    void PutCurrentSpot();

    void Complete();

    void OnEditorNotifyEvent(EEditorNotifyEvent event) override;

private:

    ELineState m_LineState;

    SSpot m_RecentSpotOnEdge;
    BrushEdge3D m_RecentEdge;
    bool m_bHasValidRecentEdge;
    bool m_bAlignedToRecentSpotLine;
    bool m_bAlignedToAnotherEdge;
};