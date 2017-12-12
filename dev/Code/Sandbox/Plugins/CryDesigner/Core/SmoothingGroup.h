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
//  File name:   SmoothingGroup.h
//  Created:     July/4/2014 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "Polygon.h"

namespace CD
{
    class Model;
};

class SmoothingGroup
    : public CRefCountBase
{
public:

    SmoothingGroup(const std::vector<CD::PolygonPtr>& polygons);
    ~SmoothingGroup();

    void SetPolygons(const std::vector<CD::PolygonPtr>& polygons);
    void AddPolygon(CD::PolygonPtr pPolygon);
    bool HasPolygon(CD::PolygonPtr pPolygon) const;

    int GetPolygonCount() const;
    CD::PolygonPtr GetPolygon(int nIndex) const;
    std::vector<CD::PolygonPtr> GetAll() const;
    void RemovePolygon(CD::PolygonPtr pPolygon);
    void Invalidate() { m_bValidmesh[0] = m_bValidmesh [1] = false; }

    const CD::FlexibleMesh& GetFlexibleMesh(bool bGenerateBackFaces = false);

private:

    bool CalculateNormal(const BrushVec3& vPos, BrushVec3& vOutNormal) const;
    void Updatemesh(bool bGenerateBackFaces = false);

private:

    CD::Model* m_pStorage;
    std::set<CD::PolygonPtr> m_PolygonSet;
    CD::FlexibleMesh m_FlexibleMesh;
    bool m_bValidmesh[2]; // 0 - Front Faces, 1 - Back Faces
};

typedef _smart_ptr<SmoothingGroup> DesignerSmoothingGroupPtr;