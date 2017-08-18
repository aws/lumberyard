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
//  (c) 2001 - 2012 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   ExtrudeTool.h
//  Created:     Sep/1/2012 by Jaesik.
////////////////////////////////////////////////////////////////////////////
#include "Tools/BaseTool.h"
#include "Util/ArgumentModel.h"
#include "Core/Model.h"

class ExtrudeTool
    : public BaseTool
{
public:

    enum EResizeStatus
    {
        eRS_None,
        eRS_Resizing
    };

    ExtrudeTool(CD::EDesignerTool tool)
        : BaseTool(tool)
        , m_ResizeStatus(eRS_None)
        , m_pScaledPolygon(new CD::Polygon)
    {
    }

    void Enter() override;
    void Leave() override;

    void OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& point) override;
    void OnLButtonUp(CViewport* view, UINT nFlags, const QPoint& point) override;
    void OnLButtonDblClk(CViewport* view, UINT nFlags, const QPoint& point) override;
    void OnMouseMove(CViewport* view, UINT nFlags, const QPoint& point) override;

    void Display(DisplayContext& dc) override;
    void OnEditorNotifyEvent(EEditorNotifyEvent event) override;

    static void Extrude(CD::SMainContext& mc, CD::PolygonPtr pPolygon, float fHeight, float fScale);

protected:

    void RaiseHeight(const QPoint& point, CViewport* view, int nFlags);
    bool AlignHeight(CD::PolygonPtr pCapPolygon, CViewport* view, const QPoint& point);

    bool StartPushPull(CViewport* view, UINT nFlags, const QPoint& point);

    struct SExtrusionContext
        : public CD::SMainContext
    {
        SExtrusionContext()
            : pPolygon(NULL)
            , pArgumentModel(NULL)
            , bFirstUpdate(false)
            , bTouchedMirrorPlane(false)
            , pushPull(CD::ePP_None)
            , initPushPull(CD::ePP_None)
            , fScale(0)
            , bUpdateBrush(true)
        {
        }

        CD::PolygonPtr pPolygon;
        CD::EPushPull pushPull;
        CD::EPushPull initPushPull;
        bool bUpdateBrush;
        CD::BrushArgumentPtr pArgumentModel;
        bool bFirstUpdate;
        bool bTouchedMirrorPlane;
        bool bIsLocatedAtOpposite;
        BrushFloat fScale;
        std::vector<CD::PolygonPtr> backupPolygons;
        std::vector<CD::PolygonPtr> mirroredBackupPolygons;
    };
    static bool PrepareExtrusion(SExtrusionContext& ec);
    static void MakeArgumentBrush(SExtrusionContext& ec);
    static void FinishPushPull(SExtrusionContext& ec);
    static void CheckBoundary(SExtrusionContext& ec);
    static void UpdateDesigner(SExtrusionContext& ec);

    void RaiseLowerPolygon(CViewport* view, UINT nFlags, const QPoint& point);
    void ResizePolygon(CViewport* view, UINT nFlags, const QPoint& point);
    void SelectPolygon(CViewport* view, UINT nFlags, const QPoint& point);

protected:

    SExtrusionContext m_ec;

    struct SActionInfo
    {
        SActionInfo()
            : m_Type(CD::ePP_None)
            , m_Distance(0) {}
        CD::EPushPull m_Type;
        BrushFloat m_Distance;
    };
    SActionInfo m_PrevAction;

    CD::PolygonPtr m_pScaledPolygon;
    EResizeStatus m_ResizeStatus;
    BrushVec2 m_ResizeStartScreenPos;
    CD::SLButtonInfo m_LButtonInfo;
};
