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
//  (c) 2001 - 2014 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   MagnetTool.h
//  Created:     May/5/2014 by Jaesik.
////////////////////////////////////////////////////////////////////////////
#include "Tools/Select/SelectTool.h"

class MagnetTool
    : public SelectTool
{
public:

    MagnetTool(CD::EDesignerTool tool)
        : SelectTool(tool)
    {
    }

    void Enter() override;
    void Leave() override;

    void OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& point) override;
    void OnMouseMove(CViewport* view, UINT nFlags, const QPoint& point) override;
    bool OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags) override;

    void Display(DisplayContext& dc) override;

private:

    enum EMagnetToolPhase
    {
        eMTP_ChooseFirstPoint,
        eMTP_ChooseUpPoint,
        eMTP_ChooseMoveToTargetPoint,
    };

    struct SSourceVertex
    {
        SSourceVertex(const BrushVec3& v)
            : position(v)
            , color(CD::kElementBoxColor){}
        SSourceVertex(const BrushVec3& v, const ColorB c)
            : position(v)
            , color(c){}
        BrushVec3 position;
        ColorB color;
    };

    void PrepareChooseFirstPointStep();
    void InitializeSelectedPolygonBeforeTransform();
    void AddVertexToList(const BrushVec3& vertex, ColorB color, std::vector<SSourceVertex>& vertices);
    void SwitchSides();
    void AlignSelectedPolygon();

    EMagnetToolPhase m_Phase;
    std::vector<SSourceVertex> m_SourceVertices;
    int m_nSelectedSourceVertex;
    int m_nSelectedUpVertex;
    CD::PolygonPtr m_pInitPolygon;
    BrushVec3 m_TargetPos;
    BrushVec3 m_PickedPos;
    BrushVec3 m_vTargetUpDir;
    bool m_bPickedTargetPos;
    bool m_bSwitchedSides;
};