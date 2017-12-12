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
//  File name:   FillSpaceTool.h
//  Created:     July/28/2013 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "Tools/BaseTool.h"

class FillSpaceTool
    : public BaseTool
{
public:

    FillSpaceTool(CD::EDesignerTool tool)
        : BaseTool(tool)
    {
    }

    void Enter() override;
    void Leave() override;

    void OnMouseMove(CViewport* view, UINT nFlags, const QPoint& point) override;
    void OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& point) override;

    void OnEditorNotifyEvent(EEditorNotifyEvent event) override;

private:

    void CompileHoles();

    bool FillHoleBasedOnSelectedElements();

    static bool ContainPolygon(CD::PolygonPtr pPolygon, std::vector<CD::PolygonPtr>& polygonList);
    static CD::PolygonPtr QueryAdjacentPolygon(CD::PolygonPtr pPolygon, std::vector<CD::PolygonPtr>& polygonList);

    _smart_ptr<CD::Model> m_pHoleContainer;
    CD::PolygonPtr m_PickedHolePolygon;
};