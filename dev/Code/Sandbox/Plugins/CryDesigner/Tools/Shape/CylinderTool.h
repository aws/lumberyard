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
//  File name:   CylinderTool.h
//  Created:     Feb/1/2014 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "ShapeTool.h"

namespace CD
{
    struct SCylinderParameter
    {
        int m_NumOfSubdivision;
        float m_Height;
        float m_Radius;

        SCylinderParameter()
            : m_NumOfSubdivision(CD::kDefaultSubdivisionNum)
            , m_Height(0)
            , m_Radius(0)
        {
        }

        void Serialize(Serialization::IArchive& ar)
        {
            ar(SUBDIVISION_RANGE(m_NumOfSubdivision), "SubdivisionCount", "Subdivision Count");
            if (ar.IsEdit())
            {
                ar(LENGTH_RANGE(m_Height), "Height", "Height");
                ar(LENGTH_RANGE(m_Radius), "Radius", "Radius");
            }
        }
    };
}

class CylinderTool
    : public ShapeTool
{
public:

    CylinderTool(CD::EDesignerTool tool)
        : ShapeTool(tool)
        , m_fAngle(0)
        , m_pBasePolygon(NULL)
        , m_vCenterOnPlane(BrushVec2(0, 0))
        , m_CylinderPhase(eCylinderPhase_PlaceFirstPoint)
    {
    }

    ~CylinderTool(){}

    void Enter() override;
    void Leave() override;

    void OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& point) override;
    void OnLButtonUp(CViewport* view, UINT nFlags, const QPoint& point) override;
    void OnMouseMove(CViewport* view, UINT nFlags, const QPoint& point) override;
    bool OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags) override;
    void Display(DisplayContext& dc) override;
    void OnEditorNotifyEvent(EEditorNotifyEvent event) override;

    void UpdateHeightWithBoundaryCheck(BrushFloat fHeight);
    void Update();

    bool IsPhaseFirstStepOnPrimitiveCreation() const override { return m_CylinderPhase == eCylinderPhase_PlaceFirstPoint; }
    bool EnabledSeamlessSelection() const override { return m_CylinderPhase == eCylinderPhase_PlaceFirstPoint && !IsModelEmpty() ? true : false; }

    void Serialize(Serialization::IArchive& ar)
    {
        m_CylinderParameter.Serialize(ar);
    }
    void OnChangeParameter(bool continuous) override { Update(); }

private:

    void FreezeModel() override;
    void UpdateHeight(float fHeight);
    void UpdateBasePolygon(float fRadius, int nNumOfSubdivision);

    enum ECylinderPhase
    {
        eCylinderPhase_PlaceFirstPoint,
        eCylinderPhase_Radius,
        eCylinderPhase_RaiseHeight,
        eCylinderPhase_Done,
    };

    ECylinderPhase m_CylinderPhase;
    BrushVec2 m_vCenterOnPlane;
    float m_fAngle;
    bool m_bIsOverOpposite;
    CD::PolygonPtr m_pBasePolygon;
    CD::PolygonPtr m_pCapPolygon;

    CD::SCylinderParameter m_CylinderParameter;
};