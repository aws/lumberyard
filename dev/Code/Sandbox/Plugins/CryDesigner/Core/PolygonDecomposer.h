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
//  File name:   PolygonDecomposer.h
//  Created:     Feb/26/2013 by Jaesik.
////////////////////////////////////////////////////////////////////////////
#include "Polygon.h"

namespace CD
{
    class BSPTree2D;
    class Convexes;

    enum EDecomposerFlag
    {
        eDF_SkipOptimizationOfPolygonResults = BIT(0)
    };
}

class PolygonDecomposer
{
public:
    PolygonDecomposer(int nFlag = 0)
        : m_nFlag(nFlag)
        , m_bGenerateConvexes(false)
        , m_pOutData(NULL)
    {
    }

    void DecomposeToConvexes(CD::PolygonPtr pPolygon, CD::Convexes& outConvexes);

    bool TriangulatePolygon(CD::PolygonPtr pPolygon, CD::FlexibleMesh& outMesh, int vertexOffset = 0, int faceOffset = 0);
    bool TriangulatePolygon(CD::PolygonPtr pPolygon, std::vector<CD::PolygonPtr>& outTriangulePolygons);

private:
    enum EMarkSide
    {
        eMarkSide_Invalid = 0,
        eMarkSide_Left  = 1,
        eMarkSide_Right = 2
    };

    enum EMarkType
    {
        eMarkType_Invalid,
        eMarkType_Start,
        eMarkType_End,
        eMarkType_Regular,
        eMarkType_Split,
        eMarkType_Merge
    };

    enum EInteriorDir
    {
        eInteriorDir_Invalid,
        eInteriorDir_Right,
        eInteriorDir_Left
    };

    struct SPointInfo
    {
        SPointInfo(const BrushVec2& pos, int nPrevIndex, int nNextIndex)
            : m_Pos(pos)
            , m_nPrevIndex(nPrevIndex)
            , m_nNextIndex(nNextIndex)
        {
        }
        BrushVec2 m_Pos;
        int m_nPrevIndex;
        int m_nNextIndex;
    };

    typedef std::vector<int> IndexList;

private:
    static const BrushFloat kComparisonEpsilon;

    struct SFloat
    {
        SFloat(BrushFloat rhs)
            : v(rhs)
        {
        }
        BrushFloat v;
        bool operator < (const SFloat& value) const
        {
            if (v - value.v < -kComparisonEpsilon)
            {
                return true;
            }
            return false;
        }
    };

    struct SMark
    {
        SMark(const BrushFloat yPos, int xPriority)
        {
            m_yPos = yPos;
            m_XPriority = xPriority;
        }

        BrushFloat m_yPos;
        int m_XPriority;

        bool operator < (const SMark& mark) const
        {
            if (m_yPos - mark.m_yPos < -kComparisonEpsilon)
            {
                return true;
            }
            if (fabs(m_yPos - mark.m_yPos) < kComparisonEpsilon && m_XPriority < mark.m_XPriority)
            {
                return true;
            }
            return false;
        }

        bool operator > (const SMark& mark) const
        {
            if (m_yPos - mark.m_yPos > kComparisonEpsilon)
            {
                return true;
            }
            if (fabs(m_yPos - mark.m_yPos) < kComparisonEpsilon && m_XPriority > mark.m_XPriority)
            {
                return true;
            }
            return false;
        }
    };

private:
    typedef bool (PolygonDecomposer::* DecomposeRoutine)(IndexList* pIndexList, bool bHasInnerHull);
    bool Decompose(DecomposeRoutine pDecomposeRoutine);
    bool DecomposeToTriangules(IndexList* pIndexList, bool bHasInnerHull);
    bool DecomposeNonplanarQuad();

    bool TriangulateConvex(const IndexList& indexList, std::vector<SMeshFace>& outFaceList) const;
    bool TriangulateConcave(const IndexList& indexList, std::vector<SMeshFace>& outFaceList);
    bool TriangulateMonotonePolygon(const IndexList& indexList, std::vector<SMeshFace>& outFaceList) const;
    bool SplitIntoMonotonePieces(const IndexList& indexList, std::vector<IndexList>& outMonatonePieces);

    void BuildEdgeListHavingSameY(const IndexList& indexList, std::map<SFloat, IndexList>& outEdgeList) const;
    void BuildMarkSideList(const IndexList& indexList, const std::map<SFloat, IndexList>& edgeListHavingSameY, std::vector<EMarkSide>& outMarkSideList, std::map<SMark, std::pair<int, int> >& outSortedMarksMap) const;

    void AddTriangle(int i0, int i1, int i2, std::vector<SMeshFace>& outFaceList) const;
    void AddFace(int i0, int i1, int i2, const IndexList& indices, std::vector<SMeshFace>& outFaceList) const;
    bool IsInside(CD::BSPTree2D* pBSPTree, int i0, int i1) const;
    bool IsOnEdge(CD::BSPTree2D* pBSPTree, int i0, int i1) const;
    bool IsCCW(int i0, int i1, int i2) const;
    bool IsCW(int i0, int i1, int i2) const{ return !IsCCW(i0, i1, i2); }
    bool IsCCW(const IndexList& indexList, int nCurr) const;
    bool IsCW(const IndexList& indexList, int nCurr) const { return !IsCCW(indexList, nCurr); }
    BrushFloat IsCCW(const BrushVec2& prev, const BrushVec2& current, const BrushVec2& next) const;
    BrushFloat IsCW(const BrushVec2& prev, const BrushVec2& current, const BrushVec2& next) const;
    bool IsConvex(const IndexList& indexList, bool bGetPrevNextFromPointList) const;
    BrushFloat Cosine(int i0, int i1, int i2) const;
    bool HasAlreadyAdded(int i0, int i1, int i2, const std::vector<SMeshFace>& faceList) const;
    bool IsColinear(const BrushVec2& p0, const BrushVec2& p1, const BrushVec2& p2) const;
    bool IsDifferenceOne(int i0, int i1, const IndexList& indexList) const;
    bool IsInsideEdge(int i0, int i1, int i2) const;
    bool GetNextIndex(int nCurr, const IndexList& indices, int& nOutNextIndex) const;
    bool HasArea(int i0, int i1, int i2) const;

    int FindLeftTopVertexIndex(const IndexList& indexList) const;
    int FindRightBottomVertexIndex(const IndexList& indexList) const;
    template<class _Pr>
    int FindExtreamVertexIndex(const IndexList& indexList) const;

    EMarkSide QueryMarkSide(int nIndex, const IndexList& indexList, int nLeftTopIndex, int nRightBottomIndex) const;
    EMarkType QueryMarkType(int nIndex, const IndexList& indexList) const;
    EInteriorDir QueryInteriorDirection(int nIndex, const IndexList& indexList) const;

    int FindDirectlyLeftEdge(int nBeginIndex, const IndexList& edgeSearchList, const IndexList& indexList) const;
    void EraseElement(int nIndex, IndexList& edgeSearchList) const;

    void SearchMonotoneLoops(CD::EdgeSet& diagonalSet, const IndexList& indexList, std::vector<IndexList>& monotonePieces) const;
    void AddDiagonalEdge(int i0, int i1, CD::EdgeSet& diagonalList) const;
    CD::BSPTree2D* GenerateBSPTree(const IndexList& indexList) const;

    void RemoveIndexWithSameAdjacentPoint(IndexList& indexList) const;
    static void FillVertexListFromPolygon(CD::PolygonPtr pPolygon, std::vector<CD::SVertex>& outVertexList);

    void CreateConvexes();
    int CheckFlag(int nFlag) const { return m_nFlag & nFlag; }

private: // Related to Triangulation
    int m_nFlag;
    CD::FlexibleMesh* m_pOutData;
    int m_VertexOffset;
    int m_FaceOffset;
    std::vector<CD::SVertex> m_VertexList;
    std::vector<SPointInfo> m_PointList;
    BrushPlane m_Plane;
    short m_MaterialID;
    CD::PolygonPtr m_pPolygon;
    int m_nBasedVertexIndex;
    bool m_bGenerateConvexes;

private: // Related to Decomposition into convexes
    std::pair<int, int> GetSortedEdgePair(int i0, int i1) const
    {
        if (i1 < i0)
        {
            std::swap(i0, i1);
        }
        return std::pair<int, int>(i0, i1);
    }
    void FindMatchedConnectedVertexIndices(int iV0, int iV1, const IndexList& indexList, int& nOutIndex0, int& nOutIndex1) const;
    bool MergeTwoConvexes(int iV0, int iV1, int iConvex0, int iConvex1, IndexList& outMergedPolygon);
    void RemoveAllConvexData()
    {
        m_InitialEdgesSortedByEdge.clear();
        m_ConvexesSortedByEdge.clear();
        m_EdgesSortedByConvex.clear();
        m_Convexes.clear();
    }
    mutable std::set<std::pair<int, int> > m_InitialEdgesSortedByEdge;
    mutable std::map<std::pair<int, int>, std::vector<int> > m_ConvexesSortedByEdge;
    mutable std::map<int, std::set<std::pair<int, int> > > m_EdgesSortedByConvex;
    mutable std::vector<IndexList> m_Convexes;
    mutable CD::Convexes* m_pBrushConvexes;
};
