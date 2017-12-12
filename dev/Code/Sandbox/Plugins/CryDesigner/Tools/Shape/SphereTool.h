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
//  File name:   SphereTool.h
//  Created:     Feb/1/2014 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "ShapeTool.h"

namespace CD
{
    struct SSphereParameter
    {
        int m_NumOfSubdivision;
        float m_Radius;

        SSphereParameter()
            : m_NumOfSubdivision(CD::kDefaultSubdivisionNum)
            , m_Radius(0)
        {
        }

        void Serialize(Serialization::IArchive& ar)
        {
            ar(SUBDIVISION_RANGE(m_NumOfSubdivision), "SubdivisionCount", "SubdivisionCount");
            if (ar.IsEdit())
            {
                ar(LENGTH_RANGE(m_Radius), "Radius", "Radius");
            }
        }
    };
}

class SphereTool
    : public ShapeTool
{
public:

    SphereTool(CD::EDesignerTool tool)
        : ShapeTool(tool)
        , m_fAngle(0)
        , m_vCenterOnPlane(BrushVec2(0, 0))
        , m_MatTo001(BrushMatrix34::CreateIdentity())
    {
    }

    ~SphereTool(){}

    void Enter() override;
    void Leave() override;

    void OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& point) override;
    void OnLButtonUp(CViewport* view, UINT nFlags, const QPoint& point) override;
    void OnMouseMove(CViewport* view, UINT nFlags, const QPoint& point) override;
    bool OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags) override;
    void OnEditorNotifyEvent(EEditorNotifyEvent event) override;

    void UpdateSphere();
    void UpdateHelperDisc(float fRadius, int nSubdivisionNum);
    void UpdateDesignerBasedOnSpherePolygons(const BrushMatrix34& tm);

    void Display(DisplayContext& dc) override;

    void FreezeModel() override;

    void Serialize(Serialization::IArchive& ar)
    {
        m_SphereParameter.Serialize(ar);
    }
    void OnChangeParameter(bool continuous) override { UpdateSphere(); }

private:

    BrushMatrix34 m_MatTo001;
    std::vector<CD::PolygonPtr> m_SpherePolygons;
    BrushVec2 m_vCenterOnPlane;
    float m_fAngle;

    CD::SSphereParameter m_SphereParameter;
};