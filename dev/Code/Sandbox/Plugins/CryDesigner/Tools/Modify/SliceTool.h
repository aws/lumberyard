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
//  File name:   SliceTool.h
//  Created:     Aug/8/2013 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "Tools/BaseTool.h"

class SliceTool
    : public BaseTool
{
public:

    SliceTool(CD::EDesignerTool tool)
        : BaseTool(tool)
        , m_PrevGizmoPos(BrushVec3(0, 0, 0))
        , m_GizmoPos(BrushVec3(0, 0, 0))
        , m_CursorPos(BrushVec3(0, 0, 0))
    {
    }

    virtual void Display(DisplayContext& dc) override;

    void Enter() override;
    void Leave() override;

    void CenterPivot();

    void SliceFrontPart();
    void SliceBackPart();
    void Clip();
    void Divide();
    void AlignSlicePlane(const BrushVec3& normalDir);
    void InvertSlicePlane();

    void SetNumberSlicePlane(int numberSlicePlane);
    int GetNumberSlicePlane() const {   return m_NumberSlicePlane;  }

    void OnManipulatorDrag(CViewport* pView, ITransformManipulator* pManipulator, QPoint& p0, QPoint& p1, const BrushVec3& value) override;
    void OnManipulatorMouseEvent(CViewport* pView, ITransformManipulator* pManipulator, EMouseEvent event, QPoint& point, int flags, bool bHitGizmo) override;

protected:

    struct ETraverseLineInfo
    {
        ETraverseLineInfo(CD::PolygonPtr pPolygon, const BrushEdge3D& edge, const BrushPlane& slicePlane)
            : m_pPolygon(pPolygon)
            , m_Edge(edge)
            , m_SlicePlane(slicePlane)
        {
        }
        CD::PolygonPtr m_pPolygon;
        BrushEdge3D m_Edge;
        BrushPlane m_SlicePlane;
    };

    typedef std::vector<ETraverseLineInfo> TraverseLineList;
    typedef std::vector<TraverseLineList> TraverseLineLists;

    void UpdateRestTraverseLineSet();
    void UpdateSlicePlane();
    virtual bool UpdateManipulatorInMirrorMode(const BrushMatrix34& offsetTM) {   return false;   }

    void GenerateLoop(const BrushPlane& slicePlane, TraverseLineList& outLineList) const;
    BrushVec3 GetLoopPivotPoint() const;

    void DrawOutlines(DisplayContext& dc);
    void DrawOutline(DisplayContext& dc, TraverseLineList& lineList);
    virtual void UpdateGizmo();

    TraverseLineList m_MainTraverseLines;
    TraverseLineLists m_RestTraverseLineSet;

    int m_NumberSlicePlane;

    BrushVec3 m_PrevGizmoPos;
    BrushVec3 m_GizmoPos;

    BrushPlane m_SlicePlane;
    BrushVec3 m_CursorPos;
};