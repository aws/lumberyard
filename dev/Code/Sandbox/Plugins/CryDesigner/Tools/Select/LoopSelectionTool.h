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
//  File name:   LoopSelectionTool.h
//  Created:     Feb/13/2014 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "Tools/BaseTool.h"

class LoopSelectionTool
    : public BaseTool
{
public:

    LoopSelectionTool(CD::EDesignerTool tool)
        : BaseTool(tool)
    {
    }

    void Enter() override;

    static void LoopSelection(CD::SMainContext& mc);
    static bool SelectFaceLoop(CD::SMainContext& mc, CD::PolygonPtr pFirstPolygon, CD::PolygonPtr pSecondPolygon);
    static bool GetLoopPolygons(CD::SMainContext& mc, CD::PolygonPtr pFirstPolygon, CD::PolygonPtr pSecondPolygon, std::vector<CD::PolygonPtr>& outPolygons);
    static bool GetLoopPolygonsInBothWays(CD::SMainContext& mc, CD::PolygonPtr pFirstPolygon, CD::PolygonPtr pSecondPolygon, std::vector<CD::PolygonPtr>& outPolygons);

private:

    static CD::PolygonPtr FindAdjacentNextPolygon(CD::SMainContext& mc, CD::PolygonPtr pFristPolygon, CD::PolygonPtr pSecondPolygon);
    static bool SelectLoop(CD::SMainContext& mc, const BrushEdge3D& initialEdge);
    static bool SelectBorderInOnePolygon(CD::SMainContext& mc, const BrushEdge3D& edge);
    static bool SelectBorder(CD::SMainContext& mc, const BrushEdge3D& edge, ElementManager& outElementInfos);
    static int GetPolygonCountSharingEdge(CD::SMainContext& mc, const BrushEdge3D& edge, const BrushPlane* pPlane = NULL);
    static int CountAllEdges(CD::SMainContext& mc);
};