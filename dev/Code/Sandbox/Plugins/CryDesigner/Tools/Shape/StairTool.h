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
//  File name:   StairTool.h
//  Created:     April/25/2013 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "ShapeTool.h"
#include "Util/HeightManipulator.h"

namespace CD
{
    struct SStairParameter
    {
        float m_StepRise;
        bool m_bMirror;
        bool m_bRotation90Degree;
        float m_Width;
        float m_Height;
        float m_Depth;

        SStairParameter()
            : m_Width(0)
            , m_Height(0)
            , m_Depth(0)
            , m_bRotation90Degree(false)
            , m_bMirror(false)
            , m_StepRise(CD::kDefaultStepRise)
        {
        }

        void Serialize(Serialization::IArchive& ar)
        {
            ar(STEPRISE_RANGE(m_StepRise), "StepRise", "Step Rise");
            ar(m_bMirror, "Mirror", "Mirror");
            ar(m_bRotation90Degree, "RotationBy90Degrees", "Rotation by 90 Degrees");
            if (ar.IsEdit())
            {
                ar(LENGTH_RANGE(m_Width), "Width", "Width");
                ar(LENGTH_RANGE(m_Height), "Height", "Height");
                ar(LENGTH_RANGE(m_Depth), "Depth", "Depth");
            }
        }
    };
}

class StairTool
    : public ShapeTool
{
public:

    StairTool(CD::EDesignerTool tool)
        : ShapeTool(tool)
        , m_StairMode(eStairMode_PlaceFirstPoint)
        , m_fBoxWidth(0)
        , m_fBoxDepth(0)
        , m_bIsOverOpposite(false)
    {
    }

    void Enter() override;
    void Leave() override;

    void OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& point) override;
    void OnLButtonUp(CViewport* view, UINT nFlags, const QPoint& point) override;
    void OnMouseMove(CViewport* view, UINT nFlags, const QPoint& point) override;

    bool OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags) override;
    void Display(DisplayContext& dc) override;
    void OnEditorNotifyEvent(EEditorNotifyEvent event) override;

    bool IsPhaseFirstStepOnPrimitiveCreation() const override { return m_StairMode == eStairMode_PlaceFirstPoint; }

    void UpdateStair();
    void UpdateStair(BrushFloat fWidth, BrushFloat fHeight, BrushFloat fDepth);

    void Serialize(Serialization::IArchive& ar)
    {
        m_StairParameter.Serialize(ar);
    }
    void OnChangeParameter(bool continuous) override { UpdateStair(m_StairParameter.m_Width, m_StairParameter.m_Height, m_StairParameter.m_Depth); }

    struct SOutputParameterForStair
    {
        SOutputParameterForStair()
            : pCapPolygon(NULL)
        {
        }
        std::vector<CD::PolygonPtr> polygons;
        std::vector<CD::PolygonPtr> polygonsNeedPostProcess;
        CD::PolygonPtr pCapPolygon;
    };

    static void CreateStair(
        const BrushVec3& vStartPos,
        const BrushVec3& vEndPos,
        BrushFloat fBoxWidth,
        BrushFloat fBoxDepth,
        BrushFloat fBoxHeight,
        const BrushPlane& floorPlane,
        float fStepRise,
        bool bXDirection,
        bool bMirrored,
        bool bRotationBy90Degree,
        CD::PolygonPtr pBasePolygon,
        SOutputParameterForStair& out);

private:

    enum EStairMode
    {
        eStairMode_PlaceFirstPoint,
        eStairMode_CreateRectangle,
        eStairMode_CreateBox,
        eStairMode_Done
    };

    EStairMode m_StairMode;

    void PlaceFirstPoint(CViewport* view, UINT nFlags, const QPoint& point);
    void CreateRectangle(CViewport* view, UINT nFlags, const QPoint& point);
    void CreateBox(CViewport* view, UINT nFlags, const QPoint& point);

    void GetRectangleVertices(BrushVec3& outV0, BrushVec3& outV1, BrushVec3& outV2, BrushVec3& outV3);
    static CD::PolygonPtr CreatePolygon(const std::vector<BrushVec3>& vList, bool bFlip, CD::PolygonPtr pBasePolygon);
    void AcceptUndo();
    void FreezeModel() override;

    BrushVec3 m_BottomVertices[4];
    BrushVec3 m_TopVertices[4];
    BrushFloat m_fBoxHeight;
    BrushFloat m_fBoxWidth;
    BrushFloat m_fBoxDepth;
    BrushFloat m_fPrevBoxHegith;
    bool m_bXDirection;
    BrushPlane m_FloorPlane;
    CD::PolygonPtr m_pFloorPolygon;
    CD::PolygonPtr m_pCapPolygon;
    bool m_bIsOverOpposite;

    BrushVec3 m_vStartPos;
    BrushVec3 m_vEndPos;

    std::vector<CD::PolygonPtr> m_PolygonsNeedPostProcess;
    _smart_ptr<CD::Model> m_pUndoModel;

    CD::SStairParameter m_StairParameter;
};