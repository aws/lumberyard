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
//  File name:   BrushTree2D.h
//  Created:     8/23/2011 by Jaesik.
////////////////////////////////////////////////////////////////////////////

namespace CD
{
    class BSPTree2DNode;

    class BSPTree2D
        : public CRefCountBase
    {
    public:

        BSPTree2D()
            : m_pRootNode(NULL)
        {
        }
        ~BSPTree2D();

        CD::EPointPosEnum IsVertexIn(const BrushVec3& vertex) const;
        CD::EIntersectionType HasIntersection(const BrushEdge3D& edge) const;
        bool IsInside(const BrushEdge3D& edge, bool bCheckCoDiff) const;
        bool IsOnEdge(const BrushEdge3D& edge) const;

        struct SOutputEdges
        {
            BrushEdge3D::List posList;
            BrushEdge3D::List negList;
            BrushEdge3D::List coSameList;
            BrushEdge3D::List coDiffList;
        };

        void GetPartitions(const BrushEdge3D& inEdge, SOutputEdges& outEdges) const;

        void BuildTree(const BrushPlane& plane, const std::vector<BrushEdge3D>& edgeList);
        void GetEdgeList(BrushEdge3D::List& outEdgeList) const    { GetEdgeList(m_pRootNode, outEdgeList); }

        bool HasNegativeNode() const;

    private:

        struct SIntersection
        {
            BrushVec2 point;
            int lineIndex[2];
        };
        typedef std::vector<SIntersection> IntersectionList;

        static void GetEdgeList(BSPTree2DNode* pTree, BrushEdge3D::List& outEdgeList);
        static BSPTree2DNode* ConstructTree(const BrushPlane& plane, const std::vector<BrushEdge3D>& edgeList);

    private:
        BSPTree2DNode* m_pRootNode;
    };

    typedef _smart_ptr<BSPTree2D> BSPTree2DPtr;
};