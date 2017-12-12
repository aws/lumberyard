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
//  File name:   CurveTool.h
//  Created:     May/7/2013 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "PolylineTool.h"

namespace CD
{
    struct SCurveParameter
    {
        SCurveParameter()
            : m_NumOfSubdivision(CD::kDefaultSubdivisionNum)
        {
        }

        void Serialize(Serialization::IArchive& ar)
        {
            ar(SUBDIVISION_RANGE(m_NumOfSubdivision), "SubdivisionCount", "Subdivision Count");
        }

        int m_NumOfSubdivision;
    };
}

class CurveTool
    : public PolylineTool
{
public:

    CurveTool(CD::EDesignerTool tool)
        : PolylineTool(tool)
        , m_ArcState(eArcState_ChooseFirstPoint)
    {
    }

    virtual ~CurveTool(){}

    void Leave();
    void OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& point){}
    void OnLButtonUp(CViewport* view, UINT nFlags, const QPoint& point);
    void OnMouseMove(CViewport* view, UINT nFlags, const QPoint& point);
    void Display(DisplayContext& dc);
    bool OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags);

    bool IsPhaseFirstStepOnPrimitiveCreation() const override;
    void Serialize(Serialization::IArchive& ar)
    {
        m_CurveParameter.Serialize(ar);
    }

protected:

    void PrepareArcSpots(CViewport* view, UINT nFlags, const QPoint& point);
    void PrepareBeizerSpots(CViewport* view, UINT nFlags, const QPoint& point);

protected:

    enum EDrawingArcState
    {
        eArcState_ChooseFirstPoint,
        eArcState_ChooseLastPoint,
        eArcState_ControlMiddlePoint
    };

    ELineState m_LineState;
    EDrawingArcState m_ArcState;
    SSpot m_LastSpot;

    CD::SCurveParameter m_CurveParameter;
};