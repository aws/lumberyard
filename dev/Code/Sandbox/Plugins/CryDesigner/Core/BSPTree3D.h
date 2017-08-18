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
//  Copyright (C), Crytek Studios, 2012.
// -------------------------------------------------------------------------
//  File name:   BSPTree3D.h
//  Created:     8/17/2011 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "Model.h"

namespace CD
{
    class BSPTree3DNode;

    struct SOutputPolygons
    {
        PolygonList posList;
        PolygonList negList;
        PolygonList coSameList;
        PolygonList coDiffList;
    };

    class BSPTree3D
        : public CRefCountBase
    {
    public:

        BSPTree3D(PolygonList& polygonList);
        ~BSPTree3D();

        void GetPartitions(PolygonPtr& pPolygon, SOutputPolygons& outPolygons) const;
        bool IsInside(const BrushVec3& vPos) const;
        bool IsValidTree() const { return m_bValidTree; }

    private:

        BSPTree3DNode* m_pRootNode;
        bool m_bValidTree;
        BSPTree3DNode* BuildBSP(PolygonList& polygonList);
    };
};