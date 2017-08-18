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
//  File name:   DiscTool.h
//  Created:     May/7/2013 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "ShapeTool.h"

namespace CD
{
    struct SDiscParameter
    {
        SDiscParameter()
            : m_NumOfSubdivision(CD::kDefaultSubdivisionNum)
            , m_Radius(0)
        {
        }

        void Serialize(Serialization::IArchive& ar)
        {
            ar(SUBDIVISION_RANGE(m_NumOfSubdivision), "SubdivisionCount", "Subdivision Count");
            if (ar.IsEdit())
            {
                ar(LENGTH_RANGE(m_Radius), "Radius", "Radius");
            }
        }

        int m_NumOfSubdivision;
        float m_Radius;
    };
}

class DiscTool
    : public ShapeTool
{
public:

    DiscTool(CD::EDesignerTool tool)
        : ShapeTool(tool)
        , m_fAngle(0)
    {
    }

    void Enter() override;
    void Leave() override;

    void OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& point)  override;
    void OnLButtonUp(CViewport* view, UINT nFlags, const QPoint& point) override;
    void OnMouseMove(CViewport* view, UINT nFlags, const QPoint& point) override;
    bool OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags) override;
    void OnEditorNotifyEvent(EEditorNotifyEvent event) override;
    void Update();
    void Display(DisplayContext& dc) override;

    void RegisterDrawnPolygonToDesigner();

    void StoreSeparateStatus() override { m_bSeparatedNewShape = !GetPickedPolygon() && GetAsyncKeyState(VK_SHIFT); }
    void OnChangeParameter(bool continuous) override { Update(); }
    void Serialize(Serialization::IArchive& ar)
    {
        m_DiscParameter.Serialize(ar);
    }

private:

    BrushVec2 m_vCenterOnPlane;
    float m_fAngle;

    CD::SDiscParameter m_DiscParameter;
};