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
//  File name:   LatheTool.h
//  Created:     Feb/4/2014 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "Tools/BaseTool.h"

class LatheTool
    : public BaseTool
{
public:

    LatheTool(CD::EDesignerTool tool)
        : BaseTool(tool)
    {
    }

    void Enter() override;
    void OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& point) override;

private:

    enum ELatheErrorCode
    {
        eLEC_Success,
        eLEC_NoPath,
        eLEC_InappropriateProfileShape,
        eLEC_ProfileShapeTooBig,
    };

    ELatheErrorCode CreateShapeAlongPath(CD::PolygonPtr pInitProfilePolygon);

    std::vector<BrushPlane> CreatePlanesAtEachPointOfPath(const std::vector<CD::SVertex>& vPath, bool bPathClosed);
    std::vector<CD::SVertex> ExtractPathFromSelectedElements(bool& bOutClosed);
    bool GluePolygons(const std::vector<CD::PolygonPtr>& polygons);

    void AddPolygonToDesigner(CD::Model* pModel, const std::vector<BrushVec3>& vList, CD::PolygonPtr pInitPolygon, bool bFlip);

private:

    CD::PolygonPtr m_pPathPolygon;
};