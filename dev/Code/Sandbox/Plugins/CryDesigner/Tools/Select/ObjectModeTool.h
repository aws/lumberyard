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
//  File name:   ObjectModeTool.h
//  Created:     Feb/4/2014 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "Tools/BaseTool.h"

class CObjectMode;

class ObjectModeTool
    : public BaseTool
{
public:

    ObjectModeTool(CD::EDesignerTool tool)
        : BaseTool(tool)
        , m_pObjectMode(NULL)
        , m_bSelectedAnother(false)
    {
    }

    void Enter() override;
    void Leave() override;

    void OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& point) override;
    void OnLButtonUp(CViewport* view, UINT nFlags, const QPoint& point) override;
    void OnLButtonDblClk(CViewport* view, UINT nFlags, const QPoint& point) override;
    void OnRButtonDown(CViewport* view, UINT nFlags, const QPoint& point) override;
    void OnRButtonUp(CViewport* view, UINT nFlags, const QPoint& point) override;
    void OnMButtonDown(CViewport* view, UINT nFlags, const QPoint& point) override;
    void OnMouseMove(CViewport* view, UINT nFlags, const QPoint& point) override;
    bool OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags) override;
    void Display(DisplayContext& dc) override;
    void OnManipulatorDrag(CViewport* view, ITransformManipulator* pManipulator, QPoint& p0, QPoint& p1, const BrushVec3& value) override;

    void OnEditorNotifyEvent(EEditorNotifyEvent event);

    bool EnabledSeamlessSelection() const override { return false; }

private:

    void SetEditMode();

    CObjectMode* m_pObjectMode;
    bool m_bSelectedAnother;
};