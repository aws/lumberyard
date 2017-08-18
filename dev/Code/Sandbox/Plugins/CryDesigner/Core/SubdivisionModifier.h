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
//  File name:   SubdivisionModifier.h
//  Created:     Sep/4/2014 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "HalfEdgeMesh.h"

namespace CD
{
    struct SSubdivisionContext
    {
        _smart_ptr<HalfEdgeMesh> fullPatches;
        _smart_ptr<HalfEdgeMesh> transitionPatches;
    };
};

class SubdivisionModifier
    : public CRefCountBase
{
public:

    CD::SSubdivisionContext CreateSubdividedMesh(CD::Model* pModel, int nSubdivisionLevel, int nTessFactor);

private:

    CD::SSubdivisionContext CreateSubdividedMesh(CD::SSubdivisionContext& sc);

    struct SPos
    {
        enum EVertexType
        {
            eVT_Valid = BIT(0),
            eVT_Corner = BIT(1)
        };
        SPos(int _pos_index, char _flag = eVT_Valid)
            : pos_index(_pos_index)
            , flag(_flag) {}
        int pos_index;
        char flag;
        bool IsValid() const { return flag & eVT_Valid; }
        bool IsCorner() const { return flag & eVT_Corner; }
    };

    void CalculateNextLocations(
        CD::HalfEdgeMesh* pHalfEdgeMesh,
        std::vector<CD::HE_Position>& outNextPosList,
        std::vector<SPos>& outNextFaceLocations,
        std::vector<SPos>& outNextEdgeLocations,
        std::vector<SPos>& outNextVertexLocations);
};