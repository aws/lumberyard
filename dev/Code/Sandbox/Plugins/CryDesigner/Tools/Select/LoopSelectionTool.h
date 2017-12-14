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

    static void LoopSelection(const CD::SMainContext& mc);
    static bool SelectFaceLoop(const CD::SMainContext& mc, CD::PolygonPtr pFirstPolygon, CD::PolygonPtr pSecondPolygon);
    static bool GetLoopPolygons(const CD::SMainContext& mc, CD::PolygonPtr pFirstPolygon, CD::PolygonPtr pSecondPolygon, std::vector<CD::PolygonPtr>& outPolygons);
    static bool GetLoopPolygonsInBothWays(const CD::SMainContext& mc, CD::PolygonPtr pFirstPolygon, CD::PolygonPtr pSecondPolygon, std::vector<CD::PolygonPtr>& outPolygons);

private:

    static CD::PolygonPtr FindAdjacentNextPolygon(const CD::SMainContext& mc, CD::PolygonPtr pFristPolygon, CD::PolygonPtr pSecondPolygon);
    static bool SelectLoop(const CD::SMainContext& mc, const BrushEdge3D& initialEdge);
    static bool SelectBorderInOnePolygon(const CD::SMainContext& mc, const BrushEdge3D& edge);
    static bool SelectBorder(const CD::SMainContext& mc, const BrushEdge3D& edge, ElementManager& outElementInfos);
    static int GetPolygonCountSharingEdge(const CD::SMainContext& mc, const BrushEdge3D& edge, const BrushPlane* pPlane = NULL);
    static int CountAllEdges(const CD::SMainContext& mc);
};
