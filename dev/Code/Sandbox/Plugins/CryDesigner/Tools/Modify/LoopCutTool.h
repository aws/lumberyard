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

#include "Tools/BaseTool.h"

class LoopCutTool
    : public BaseTool
{
public:

    LoopCutTool(CD::EDesignerTool tool)
        : BaseTool(tool)
    {
    }

    void Enter() override;

    void OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& point) override;
    void OnMouseMove(CViewport* view, UINT nFlags, const QPoint& point) override;
    void OnMouseWheel(CViewport* view, UINT nFlags, const QPoint& point) override;
    bool OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags) override;
    void Display(DisplayContext& dc) override;

private:

    void UpdateLoops(CD::PolygonPtr pPolygon, const BrushVec3& posOnSurface);
    void UpdateLoops();
    void FreezeLoops();

    enum ELoopCutMode
    {
        eLoopCutMode_Divide,
        eLoopCutMode_Slide,
    };

    struct SLoopSection
    {
        SLoopSection(CD::PolygonPtr _pPolygon = NULL)
            : pPolygon(_pPolygon) {}
        std::vector<BrushEdge3D> edges;
        CD::PolygonPtr pPolygon;
    };

    CD::PolygonPtr m_pSelectedPolygon;
    std::vector<SLoopSection> m_LoopPolygons;
    BrushVec3 m_vPosOnSurface;
    int m_SegmentNum;
    BrushFloat m_SlideOffset;
    BrushFloat m_NormalizedSlideOffset;
    BrushVec3 m_LoopCutDir;
    ELoopCutMode m_LoopCutMode;
};