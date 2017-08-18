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

#include "ShapeTool.h"

namespace CD
{
    struct SBoxParameter
    {
        float m_Width;
        float m_Height;
        float m_Depth;

        SBoxParameter()
            : m_Width(0)
            , m_Height(0)
            , m_Depth(0)
        {
        }

        void Serialize(Serialization::IArchive& ar)
        {
            if (ar.IsEdit())
            {
                ar(LENGTH_RANGE(m_Width), "Width", "Width");
                ar(LENGTH_RANGE(m_Height), "Height", "Height");
                ar(LENGTH_RANGE(m_Depth), "Depth", "Depth");
            }
        }
    };
}

class BoxTool
    : public ShapeTool
{
public:

    BoxTool(CD::EDesignerTool tool)
        : ShapeTool(tool)
    {
    }

    void Enter() override;
    void Leave() override;

    void OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& point) override;
    void OnLButtonUp(CViewport* view, UINT nFlags, const QPoint& point) override;
    void OnMouseMove(CViewport* view, UINT nFlags, const QPoint& point) override;
    bool OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags) override;

    void UpdateBoxWithBoundaryCheck(const BrushVec3& v0, const BrushVec3& v1, BrushFloat fHeight, bool bUpdateUI);

    void Display(DisplayContext& dc) override;

    bool IsPhaseFirstStepOnPrimitiveCreation() const override { return m_Phase == eBoxPhase_PlaceFirstPoint; }

    void OnEditorNotifyEvent(EEditorNotifyEvent event) override;
    bool EnabledSeamlessSelection() const override { return m_Phase == eBoxPhase_PlaceFirstPoint && !IsModelEmpty() ? true : false; }

    void Serialize(Serialization::IArchive& ar)
    {
        m_BoxParameter.Serialize(ar);
    }
    void OnChangeParameter(bool continuous) override;

private:

    void FreezeModel() override;
    void UpdateBox(const BrushVec3& v0, const BrushVec3& v1, BrushFloat fHeight, bool bUpdateUI);

    enum EBoxPhase
    {
        eBoxPhase_PlaceFirstPoint,
        eBoxPhase_DrawRectangle,
        eBoxPhase_RaiseHeight,
        eBoxPhase_Done,
    };

    EBoxPhase m_Phase;
    bool m_bIsOverOpposite;
    CD::PolygonPtr m_pCapPolygon;
    bool m_bStartedUndo;
    BrushVec3 m_v[2];

    CD::SBoxParameter m_BoxParameter;
};