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
//  File name:   ShapeTool.h
//  Created:     May/5/2013 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "Tools/BaseTool.h"
#include "Util/SpotManager.h"

class ShapeTool
    : public BaseTool
    , public SpotManager
{
public:

    ShapeTool(CD::EDesignerTool tool)
        : BaseTool(tool)
        , SpotManager()
        , m_EditMode(eEditMode_None)
        , m_pIntermediatePolygon(NULL)
    {
    }

    virtual ~ShapeTool(){}

    virtual void Enter() override;

    virtual void OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& point) override;
    virtual void OnLButtonUp(CViewport* view, UINT nFlags, const QPoint& point) override;
    virtual void OnMouseMove(CViewport* view, UINT nFlags, const QPoint& point) override;
    virtual bool OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags) override;
    virtual void Display(DisplayContext& dc) override;

    virtual bool IsPhaseFirstStepOnPrimitiveCreation() const override { return m_EditMode == eEditMode_Beginning; }

    void DisplayCurrentSpot(DisplayContext& dc);

    bool EnabledSeamlessSelection() const override { return m_EditMode == eEditMode_Beginning && !IsModelEmpty() ? true : false; }

protected:

    bool UpdateCurrentSpotPosition(CViewport* view, UINT nFlags, const QPoint& point, bool bKeepInitialPlane, bool bSearchAllShelves = false);
    void UpdateDrawnPolygon(const BrushVec2& p0, const BrushVec2& p1);

protected:

    enum EEditMode
    {
        eEditMode_Beginning,
        eEditMode_Editing,
        eEditMode_Done,
        eEditMode_None
    };

    enum ELineState
    {
        eLineState_Diagonal,
        eLineState_ParallelToAxis,
        eLineState_Cross
    };

protected:

    CD::STexInfo GetTexInfo() const;
    int GetMatID() const;

    void SetIntermediatePolygon(CD::PolygonPtr pPolygon){m_pIntermediatePolygon = pPolygon; }
    CD::PolygonPtr GetIntermediatePolygon() const{return m_pIntermediatePolygon; }
    void DrawIntermediatePolygon(DisplayContext& dc);

    void SetTempPolygon(CD::PolygonPtr pPolygon) { m_pTempPolygon = pPolygon; }
    CD::PolygonPtr GetTempPolygon() const { return m_pTempPolygon; }

    EEditMode GetEditMode() const{return m_EditMode; }
    void SetEditMode(EEditMode editMode){m_EditMode = editMode; }

    virtual void FreezeModel();
    void Freeze2DModel();
    void CancelCreation();

    bool IsSeparateStatus() const;
    virtual void StoreSeparateStatus() { m_bSeparatedNewShape = GetPickedPolygon() && !GetAsyncKeyState(VK_SHIFT) ? false : true; }
    void Separate1stStep();
    void Separate2ndStep();

    bool m_bSeparatedNewShape;
private:

    EEditMode m_EditMode;
    CD::PolygonPtr m_pIntermediatePolygon;
    CD::PolygonPtr m_pTempPolygon;
};