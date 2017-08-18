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
//  File name:   StairProfileTool.h
//  Created:     May/29/2013 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "ShapeTool.h"

namespace CD
{
    enum EStairHeightCalculationWayMode
    {
        eStairHeightCalculationWay_StepRise,
        eStairHeightCalculationWay_StepNumber
    };

    struct SStairProfileParameter
    {
        float m_StepRise;

        SStairProfileParameter()
            : m_StepRise(CD::kDefaultStepRise)
        {
        }

        void Serialize(Serialization::IArchive& ar)
        {
            ar(STEPRISE_RANGE(m_StepRise), "StepRise", "Step Rise");
        }
    };
}

class StairProfileTool
    : public ShapeTool
{
public:

    StairProfileTool(CD::EDesignerTool tool)
        : ShapeTool(tool)
        , m_SideStairMode(eSideStairMode_PlaceFirstPoint)
        , m_nSelectedCandidate(0)
    {
    }

    void Enter() override;

    void OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& point) override;
    void OnMouseMove(CViewport* view, UINT nFlags, const QPoint& point) override;

    bool OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags) override;
    void Display(DisplayContext& dc) override;

    void Serialize(Serialization::IArchive& ar)
    {
        m_StairProfileParameter.Serialize(ar);
    }

protected:

    void CreateCandidates();
    void DrawCandidateStair(DisplayContext& dc, int nIndex, const ColorB& color);

    enum ESideStairMode
    {
        eSideStairMode_PlaceFirstPoint,
        eSideStairMode_DrawDiagonal,
        eSideStairMode_SelectDirection,
    };

    ESideStairMode m_SideStairMode;
    std::vector<SSpot> m_CandidateStairs[2];
    BrushLine m_BorderLine;
    int m_nSelectedCandidate;
    SSpot m_LastSpot;
    int m_nSelectedPolygonIndex;

    CD::SStairProfileParameter m_StairProfileParameter;
};