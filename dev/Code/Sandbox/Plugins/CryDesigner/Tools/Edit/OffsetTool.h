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
//  File name:   BrushDesignerEditOffsetTool.h
//  Created:     May/7/2013 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "Tools/BaseTool.h"

class OffsetTool
    : public BaseTool
{
public:

    OffsetTool(CD::EDesignerTool tool)
        : BaseTool(tool)
        , m_fScale(0)
        , m_fPrevScale(0)
    {
    }

    void Leave() override;
    void OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& point) override;
    void OnLButtonUp(CViewport* view, UINT nFlags, const QPoint& point) override;
    void OnMouseMove(CViewport* view, UINT nFlags, const QPoint& point) override;
    void OnLButtonDblClk(CViewport* view, UINT nFlags, const QPoint& point) override;
    void Display(DisplayContext& dc) override;

    static void ApplyOffset(CD::Model* pModel, CD::PolygonPtr pScaledPolygon, CD::PolygonPtr pOriginalPolygon, std::vector<CD::PolygonPtr>& holes, bool bCreateBridgeEdges);

private:

    CD::PolygonPtr QueryOffsetPolygon(CViewport* view, const QPoint& point) const;
    void AddScaledPolygon(bool bCreateBridgeEdges);
    BrushFloat ApplyScaleToSelectedPolygon(CD::PolygonPtr pPolygon, BrushFloat fScale);
    void CalculateOffset(CViewport* view, const QPoint& point);

    struct SOffsetContext
    {
        BrushPlane plane;
        BrushVec3 origin;
        CD::PolygonPtr polygon;
        std::vector<CD::PolygonPtr> holes;
    };

    SOffsetContext m_Context;
    CD::PolygonPtr m_pOffsetedPolygon;
    CD::SLButtonInfo m_LButtonInfo;
    BrushFloat m_fScale;
    BrushFloat m_fPrevScale;
    BrushVec3 m_HitPos;
};
