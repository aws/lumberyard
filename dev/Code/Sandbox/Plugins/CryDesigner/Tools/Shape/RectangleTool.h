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
//  File name:   RectangleTool.h
//  Created:     May/7/2013 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "ShapeTool.h"

namespace CD
{
    struct SRectangleParameter
    {
        float m_Width;
        float m_Depth;

        SRectangleParameter()
            : m_Width(0)
            , m_Depth(0)
        {
        }

        void Serialize(Serialization::IArchive& ar)
        {
            if (ar.IsEdit())
            {
                ar(LENGTH_RANGE(m_Width), "Width", "Width");
                ar(LENGTH_RANGE(m_Depth), "Depth", "Depth");
            }
        }
    };
}

class RectangleTool
    : public ShapeTool
{
public:

    RectangleTool(CD::EDesignerTool tool)
        : ShapeTool(tool)
    {
    }

    void Enter() override;
    void Leave() override;

    void OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& point) override;
    void OnLButtonUp(CViewport* view, UINT nFlags, const QPoint& point) override;
    void OnMouseMove(CViewport* view, UINT nFlags, const QPoint& point) override;
    bool OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags) override;
    void OnEditorNotifyEvent(EEditorNotifyEvent event) override;

    void Display(DisplayContext& dc) override;

    bool IsPhaseFirstStepOnPrimitiveCreation() const override { return m_Phase == eRectanglePhase_PlaceFirstPoint; }
    bool EnabledSeamlessSelection() const override { return m_Phase == eRectanglePhase_PlaceFirstPoint && !IsModelEmpty() ? true : false; }

    void StoreSeparateStatus() override { m_bSeparatedNewShape = !GetPickedPolygon() && GetAsyncKeyState(VK_SHIFT); }
    void UpdateRectangle(const BrushVec3& v0, const BrushVec3& v1, bool bRenderFace, bool bUpdateUIs);

    void Serialize(Serialization::IArchive& ar)
    {
        m_RectangleParameter.Serialize(ar);
    }
    void OnChangeParameter(bool continuous) override;

private:

    enum ERectanglePhase
    {
        eRectanglePhase_PlaceFirstPoint,
        eRectanglePhase_DrawRectangle,
        eRectanglePhase_Done,
    };

    ERectanglePhase m_Phase;
    BrushVec3 m_v[2];

    CD::SRectangleParameter m_RectangleParameter;
};