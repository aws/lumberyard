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
///////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2012 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   PrimitiveShape.h
//  Created:     5/5/2010 by Jaesik.
////////////////////////////////////////////////////////////////////////////
#include "Tools/Shape/PolylineTool.h"
#include "ArgumentModel.h"
#include "Core/Polygon.h"

class PrimitiveShape
{
public:

    void CreateBox(const BrushVec3& mins, const BrushVec3& maxs, std::vector<CD::PolygonPtr>* pOutPolygonList = NULL) const;
    void CreateSphere(const BrushVec3& mins, const BrushVec3& maxs, int numSides, std::vector<CD::PolygonPtr>* pOutPolygonList = NULL) const;
    void CreateSphere(const BrushVec3& vCenter, float radius, int numSides, std::vector<CD::PolygonPtr>* pOutPolygonList = NULL) const;
    void CreateCylinder(const BrushVec3& mins, const BrushVec3& maxs, int numSides, std::vector<CD::PolygonPtr>* pOutPolygonList = NULL) const;
    void CreateCylinder(CD::PolygonPtr pBaseDiscPolygon, float fHeight, std::vector<CD::PolygonPtr>* pOutPolygonList = NULL) const;
    void CreateCone(const BrushVec3& mins, const BrushVec3& maxs, int numSides, std::vector<CD::PolygonPtr>* pOutPolygonList = NULL) const;
    void CreateCone(CD::PolygonPtr pBaseDiscPolygon, float fHeight, std::vector<CD::PolygonPtr>* pOutPolygonList = NULL) const;
    void CreateRectangle(const BrushVec3& mins, const BrushVec3& maxs, std::vector<CD::PolygonPtr>* pOutPolygonList = NULL) const;
    void CreateDisc(const BrushVec3& mins, const BrushVec3& maxs, int numSides, std::vector<CD::PolygonPtr>* pOutPolygonList = NULL) const;

private:

    void CreateCircle(const BrushVec3& mins, const BrushVec3& maxs, int numSides, std::vector<BrushVec3>& outVertexList) const;
};