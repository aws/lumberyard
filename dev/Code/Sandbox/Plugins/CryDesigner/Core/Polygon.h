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
//  (c) 2001 - 2012 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   Polygon.h
//  Created:     8/26/2011 by Jaesik.
////////////////////////////////////////////////////////////////////////////

namespace CD
{
    class BSPTree2D;
    class Convexes;

    enum EPolygonFlag
    {
        ePolyFlag_Mirrored = BIT(1),
        ePolyFlag_Hidden   = BIT(4),
        ePolyFlag_NonplanarQuad = BIT(5),
        ePolyFlag_All      = 0xFFFFFFFF
    };

    enum EIncludeCoEdgeInIntersecting
    {
        eICEII_IncludeCoSame = BIT(0),
        eICEII_IncludeCoDiff = BIT(1),
        eICEII_IncludeBothBoundary = eICEII_IncludeCoSame | eICEII_IncludeCoDiff
    };

    enum ESeparatePolygons
    {
        eSP_OuterHull = 0x01,
        eSP_InnerHull = 0x02,
        eSP_Together = 0x04,
    };

    enum EResultExtract
    {
        eRE_Fail,
        eRE_EndAtEndVtx,
        eRE_EndAtStartVtx
    };

    inline std::vector<SVertex> ToSVertexList(const std::vector<BrushVec3>& vList)
    {
        int count = vList.size();
        std::vector<SVertex> vertices;
        vertices.reserve(count);
        for (int i = 0; i < count; ++i)
        {
            vertices.push_back(SVertex(vList[i]));
        }
        return vertices;
    }

    class Polygon
        : public CRefCountBase
    {
    public:

        typedef _smart_ptr<Polygon> PolygonPtr;

    public:

        Polygon();
        Polygon(const Polygon& polygon);
        Polygon(const std::vector<BrushVec3>& vertices, const std::vector<SEdge>& edges);
        Polygon(const std::vector<BrushVec3>& vertices);
        Polygon(const std::vector<SVertex>&   vertices);
        Polygon(const std::vector<BrushVec2>& points, const BrushPlane& plane, int matID, const STexInfo* pTexInfo, bool bClosed);
        Polygon(const std::vector<BrushVec3>& vertices, const std::vector<SEdge>& edges, const BrushPlane& plane, int matID, const STexInfo* pTexInfo, bool bOptimizePolygon = true);
        Polygon(const std::vector<SVertex>&   vertices, const std::vector<SEdge>& edges, const BrushPlane& plane, int matID, const STexInfo* pTexInfo, bool bOptimizePolygon = true);
        Polygon(const std::vector<BrushVec3>& vertices, const BrushPlane& plane, int matID, const STexInfo* pTexInfo, bool bClose);
        Polygon(const std::vector<SVertex>&   vertices, const BrushPlane& plane, int matID, const STexInfo* pTexInfo, bool bClose);
        ~Polygon();

        Polygon& operator = (const Polygon& polygon);

        void Init();
        PolygonPtr Clone() const;
        void Clear();
        void Display(DisplayContext& dc) const;
        bool IsPassed(const BrushVec3& raySrc, const BrushVec3& rayDir, BrushFloat& outT);
        bool IncludeAllEdges(PolygonPtr pPolygon) const;
        bool Include(PolygonPtr pPolygon) const;
        bool Include(const BrushVec3& vertex) const;
        bool IntersectedBetweenAABBs(const AABB& aabb) const;
        bool IsOpen(const std::vector<SVertex>& vertices, const std::vector<SEdge>& edges) const;
        bool IsOpen() const { return CheckPrivateFlags(eRPF_Open); }
        bool IsIdentical(PolygonPtr pPolygon) const;
        bool IsEdgeOnCrust(const BrushEdge3D& edge, int* pOutEdgeIndex = NULL, BrushEdge3D* pOutIntersectedEdge = NULL) const;
        bool IsEquivalent(const PolygonPtr& pPolygon) const;
        bool SubtractEdge(const BrushEdge3D& edge, std::vector<BrushEdge3D>& outSubtractedEdges) const;
        bool HasOverlappedEdges(PolygonPtr pPolygon) const;
        bool HasEdge(const BrushEdge3D& edge, bool bApplyDir = false, int* pOutEdgeIndex = NULL) const;
        bool QueryIntersections(const BrushEdge3D& edge, std::map<BrushFloat, BrushVec3>& outIntersections) const;
        bool QueryIntersections(const BrushPlane& plane, const BrushLine3D& crossLine3D, std::vector<BrushEdge3D>& outSortedIntersections) const;
        bool QueryNearestEdge(const BrushVec3& vertex, BrushEdge3D& outNearestEdge, BrushVec3& outNearestPos) const;
        bool QueryNearestPosFromBoundary(const BrushVec3& vertex, BrushVec3& outNearestPos) const;
        bool QueryEdgesHavingVertex(const BrushVec3& vertex, std::vector<int>& outEdgeIndices) const;
        bool QueryAxisAlignedLines(std::vector<BrushLine>& outLines);
        void QueryIntersectionEdgesWith2DRect(IDisplayViewport* pView, const BrushMatrix34& worldTM, PolygonPtr pRectPolygon, bool bExcludeBackFace, EdgeQueryResult& outIntersectionEdges);
        void Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bUndo);
        void SaveBinary(std::vector<char>& buffer);
        int  LoadBinary(std::vector<char>& buffer);
        void SaveUVBinary(std::vector<char>& buffer);
        void LoadUVBinary(std::vector<char>& buffer, int offset);
        bool AddOpenPolygon(PolygonPtr BPolygon);
        bool Union(PolygonPtr BPolygon);
        bool Subtract(PolygonPtr BPolygon);
        bool ExclusiveOR(PolygonPtr BPolygon);
        bool Intersect(PolygonPtr BPolygon, uint8 includeCoEdgeFlags = eICEII_IncludeBothBoundary);
        bool ClipInside(PolygonPtr BPolygon);
        bool ClipOutside(PolygonPtr BPolygon);
        PolygonPtr RemoveInside();
        void MakeThisConvex();
        PolygonPtr Flip();
        void ReverseEdges();
        bool Attach(PolygonPtr pPolygon);
        BrushFloat GetNearestDistance(PolygonPtr pPolygon, const BrushVec3& direction) const;
        void ClipByEdge(int nEdgeIndex, std::vector<PolygonPtr>& outSplittedPolygons) const;
        const AABB& GetBoundBox() const;
        BrushFloat GetRadius() const;
        bool GetSeparatedPolygons(std::vector<PolygonPtr>& outSeparatedPolygons, int nSpratePolygonFlag = eSP_Together, bool bOptimizePolygon = true) const;
        void ModifyOrientation();
        bool IsValid() const {  return !m_Vertices.empty() && !m_Edges.empty(); }
        const BrushPlane& GetPlane() const { return m_Plane; }
        void SetPlane(const BrushPlane& plane){ m_Plane = plane; }
        bool UpdatePlane(const BrushPlane& plane)
        {
            return UpdatePlane(plane, -plane.Normal());
        }
        bool UpdatePlane(const BrushPlane& plane, const BrushVec3& directionForHitTest);
        void SetMaterialID(int nMatID)
        {
            InvalidateCacheData();
            m_MaterialID = nMatID;
        }
        int GetMaterialID() const { return m_MaterialID; }
        const STexInfo& GetTexInfo() const { return m_TexInfo; }
        void SetTexInfo(const STexInfo& texInfo)
        {
            InvalidateCacheData();
            m_TexInfo = texInfo;
            UpdateUVs();
        }
        bool IsConvex() const { return CheckPrivateFlags(eRPF_Convex); }
        int GetEdgeCount() const{ return m_Edges.size(); }
        BrushEdge3D GetEdge(int nEdgeIndex) const;
        BrushEdge GetEdge2D(int nEdgeIndex) const;
        bool GetEdgesByVertexIndex(int nVertexIndex, std::vector<int>& outEdgeIndices) const;
        int GetEdgeIndex(int nVertexIndex0, int nVertexIndex1) const;
        int GetEdgeIndex(const BrushEdge3D& edge3D) const;
        bool GetEdge(int nVertexIndex0, int nVertexIndex1, SEdge& outEdge) const;
        const SEdge& GetEdgeIndexPair(int nEdgeIndex) const { return m_Edges[nEdgeIndex]; }
        bool GetAdjacentEdgesByEdgeIndex(int nEdgeIndex, int* pOutPrevEdgeIndex, int* pOutNextEdgeIndex) const;
        bool GetAdjacentEdgesByVertexIndex(int nVertexIndex, int* pOutPrevEdgeIndex, int* pOutNextEdgeIndex) const;
        BrushVec3 GetCenterPosition() const;
        BrushVec3 GetAveragePosition() const;
        BrushVec3 GetRepresentativePosition() const;
        bool Scale(const BrushFloat& kScale, bool bCheckBoundary = false, std::vector<BrushEdge3D>* pOutEdgesBeforeOptimization = NULL);
        bool BroadenVertex(const BrushFloat& kScale, int nVertexIndex, const BrushEdge3D* pBaseEdge = NULL);
        bool GetMaximumScale(BrushFloat& fOutShortestScale) const;
        bool IsPlaneEquivalent(PolygonPtr pPolygon) const;
        bool CheckFlags(int nFlags) const
        {
            if (m_Flag == 0 && nFlags == ePolyFlag_All)
            {
                return true;
            }
            return m_Flag & nFlags ? true : false;
        }
        int AddFlags(int nFlags){return m_Flag |= nFlags; }
        void SetFlag(int nFlag){m_Flag = nFlag; }
        int RemoveFlags(int nFlags){ return m_Flag &= ~nFlags; }
        int GetFlag() const {return m_Flag; }
        bool GetVertexIndex(const BrushVec3& vertex, int& nOutIndex) const;
        bool GetNearestVertexIndex(const BrushVec3& vertex, int& nOutIndex) const;
        const BrushVec3& GetPos(int nIndex) const { return m_Vertices[nIndex].pos; }
        const Vec2& GetUV(int nIndex) const { return m_Vertices[nIndex].uv; }
        const SVertex& GetVertex(int nIndex) const { return m_Vertices[nIndex]; }
        int GetVertexCount() const{return m_Vertices.size(); }
        void SetVertex(int nIndex, const SVertex& vertex);
        void SetPos(int nIndex, const BrushVec3& pos);
        bool AddVertex(const BrushVec3& vertex, int* pOutNewVertexIndex = NULL, EdgeIndexSet* pOutNewEdgeIndices = NULL);
        bool SwapEdge(int nEdgeIndex0, int nEdgeIndex1);
        int AddEdge(const BrushEdge3D& edge);
        bool GetEdgeIndex(int nVertexIndex0, int nVertexIndex1, int& nOutEdgeIndex) const
        {
            return GetEdgeIndex(m_Edges, nVertexIndex0, nVertexIndex1, nOutEdgeIndex);
        }
        static bool GetEdgeIndex(const std::vector<SEdge>& edgeList, int nVertexIndex0, int nVertexIndex1, int& nOutEdgeIndex);
        bool GetNextVertex(int nVertexIndex, BrushVec3& outVertex) const;
        bool GetPrevVertex(int nVertexIndex, BrushVec3& outVertex) const;
        bool GetLinkedVertices(std::vector<SVertex>& outVertexList) const
        {
            return GetLinkedVertices(outVertexList, m_Vertices, m_Edges);
        }
        void Optimize();
        bool Exist(const BrushVec3& vertex, const BrushFloat& kEpsilon, int* pOutIndex = NULL) const;
        bool Exist(const BrushVec3& vertex, int* pOutIndex = NULL) const { return Exist(vertex, kDesignerEpsilon, pOutIndex); }
        bool Exist(const BrushEdge3D& edge3D, bool bAllowReverse, int* pOutIndex = NULL) const;
        bool IsEndPoint(const BrushVec3& position, bool* bOutFirst = NULL) const;
        void Move(const BrushVec3& offset);
        void Transform(const Matrix34& tm);
        bool Concatenate(PolygonPtr pPolygon);
        // These functions are only valid in the case that the polygon is not close.
        // If the polygon is not close, the methods will return false.
        bool GetFirstVertex(BrushVec3& outVertex) const;
        bool GetLastVertex(BrushVec3& outVertex) const;
        // Rearrange vertices and edges so that they can have the sequential orders
        void Rearrange();
        BSPTree2D* GetBSPTree() const
        {
            if (!m_pBSPTree)
            {
                BuildBSP();
            }
            return m_pBSPTree;
        }
        bool IsCCW() const;
        bool HasHoles() const { return CheckPrivateFlags(eRPF_HasHoles); }
        bool IsVertexOnCrust(const BrushVec3& vertex) const;
        bool HasVertex(const BrushVec3& vertex, int* pOutVertexIndex = NULL) const;
        bool IsBridgeEdgeRelation(const SEdge& pEdge0, const SEdge& pEdge1) const;
        bool HasBridgeEdges() const;
        void RemoveBridgeEdges();
        void GetBridgeEdges(std::vector<BrushEdge3D>& outBridgeEdges) const;
        void RemoveEdge(const BrushEdge3D& edge);
        void RemoveEdge(int nEdgeIndex);
        void GetUnconnectedPolygons(std::vector<PolygonPtr>& outPolygons);
        EResultExtract ExtractVertexList(int nStartEdgeIdx, int nEndEdgeIdx, std::vector<BrushVec3>& outVertexList, std::vector<int>* pOutVertexIndices = NULL) const;
        bool InRectangle(IDisplayViewport* pView, const BrushMatrix34& worldTM, PolygonPtr pRectPolygon, bool bExcludeBackFace);

        static EIntersectionType HasIntersection(PolygonPtr pPolygon0, PolygonPtr pPolygon1);
        static void MakePolygonsFromEdgeList(const std::vector<BrushEdge3D>& edgeList, const BrushPlane& plane, std::vector<PolygonPtr>& outPolygons);

        REFGUID GetGUID() const{ return m_GUID; }
        bool ClipByPlane(const BrushPlane& clipPlane, std::vector<PolygonPtr>& pOutFrontPolygons, std::vector<PolygonPtr>& pOutBackPolygons, std::vector<BrushEdge3D>* pOutBoundaryEdges = NULL) const;
        PolygonPtr Mirror(const BrushPlane& mirrorPlane);
        bool GetComputedPlane(BrushPlane& outPlane) const;

        int ChoosePrevEdge(const SEdge& edge, const EdgeIndexSet& candidateSecondIndices) const
        {
            SEdge outEdge;
            ChoosePrevEdge(m_Vertices, edge, candidateSecondIndices, outEdge);
            return GetEdgeIndex(outEdge.m_i[0], outEdge.m_i[1]);
        }
        int ChooseNextEdge(const SEdge& edge, const EdgeIndexSet& candidateSecondIndices) const
        {
            SEdge outEdge;
            ChooseNextEdge(m_Vertices, edge, candidateSecondIndices, outEdge);
            return GetEdgeIndex(outEdge.m_i[0], outEdge.m_i[1]);
        }

        Convexes* GetConvexes();
        FlexibleMesh* GetTriangles(bool bGenerateBackFaces = false) const;

        bool GetQuadDiagonal(BrushEdge3D& outEdge) const;
        bool IsQuad() const { return GetEdgeCount() == 4 && GetVertexCount() == 4 && !IsOpen(); }
        bool IsTriangle() const { return GetEdgeCount() == 3 && GetVertexCount() == 3 && !IsOpen(); }

        bool FindOppositeEdge(const BrushEdge3D& edge, BrushEdge3D& outEdge) const;
        bool FindSharingEdge(PolygonPtr pPolygon, BrushEdge3D& outEdge) const;
        void UpdateUVs();

    private:

        enum ESearchDirection
        {
            eSD_Previous,
            eSD_Next,
        };

        enum EPolygonPrivateFlag
        {
            eRPF_Open      = BIT(0),
            eRPF_Convex    = BIT(1),
            eRPF_HasHoles  = BIT(2),
            eRPF_EnabledBackFaces = BIT(3),
            eRPF_Invalid   = BIT(31)
        };

        struct SPolygonBound
        {
            AABB aabb;
            BrushFloat raidus;
        };

    private:

        void Reset(const std::vector<BrushVec3>& vertices, const std::vector<SEdge>& edgeList)
        {
            Reset(ToSVertexList(vertices), edgeList);
        }
        void Reset(const std::vector<SVertex>&   vertices, const std::vector<SEdge>& edgeList);
        void Reset(const std::vector<BrushVec3>& vertices)
        {
            Reset(ToSVertexList(vertices));
        }
        void Reset(const std::vector<SVertex>&   vertices);
        bool QueryEdgesContainingVertex(const BrushVec3& vertex, std::vector<int>& outEdgeIndices) const;
        bool BuildBSP() const;
        bool OptimizeEdges(std::vector<SVertex>& vertices, std::vector<SEdge>& edges) const;
        void OptimizeVertices(std::vector<SVertex>& vertices, std::vector<SEdge>& edges) const;
        void Transform2Vertices(const std::vector<BrushVec2>& points, const BrushPlane& plane);
        EIntersectionType HasIntersection(PolygonPtr pPolygon);
        int AddVertex(std::vector<SVertex>& vertices, const SVertex& newVertex) const;
        void Clip(const BSPTree2D* pTree, EClipType cliptype, std::vector<SVertex>& vertices, std::vector<SEdge>& edges, EClipObjective clipObjective, unsigned char vertexID) const;
        bool Optimize(std::vector<SVertex>& vertices, std::vector<SEdge>& edges);
        bool Optimize(const std::vector<BrushEdge3D>& edges);
        bool GetAdjacentPrevEdgeIndicesWithVertexIndex(int vertexIndex, std::vector<int>& outEdgeIndices, const std::vector<SEdge>& edges) const;
        bool GetAdjacentNextEdgeIndicesWithVertexIndex(int vertexIndex, std::vector<int>& outEdgeIndices, const std::vector<SEdge>& edges) const;
        bool GetAdjacentEdgeIndexWithEdgeIndex(int edgeIndex, int& outPrevIndex, int& outNextIndex, const std::vector<SVertex>& vertices, const std::vector<SEdge>& edges) const;
        BrushLine GetLineFromEdge(int edgeIndex, const std::vector<SVertex>& vertices, const std::vector<SEdge>& edges) const
        {
            return BrushLine(m_Plane.W2P(vertices[edges[edgeIndex].m_i[0]].pos), m_Plane.W2P(vertices[edges[edgeIndex].m_i[1]].pos));
        }
        bool GetLinkedVertices(std::vector<SVertex>& outVertexList, const std::vector<SVertex>& vertices, const std::vector<SEdge>& edges) const;
        void FindLoops(std::vector<EdgeList>& outLoopList) const;
        void SearchLinkedColinearEdges(int edgeIndex, ESearchDirection direction, const std::vector<SVertex>& vertices, const std::vector<SEdge>& edges, std::vector<int>& outLinkedEdges) const;
        void SearchLinkedEdges(int edgeIndex, ESearchDirection direction, const std::vector<SVertex>& vertices, const std::vector<SEdge>& edges, std::vector<int>& outLinkedEdges) const;
        static bool DoesIdenticalEdgeExist(const std::vector<SEdge>& edges, const SEdge& e, int* pOutIndex = NULL);
        static bool DoesReverseEdgeExist(const std::vector<SEdge>& edges, const SEdge& e, int* pOutIndex = NULL);
        bool CreateNewPolygonFromEdges(const std::vector<SEdge>& inputEdges, PolygonPtr& outPolygon, bool bOptimizePolygon = true) const;
        void Load(XmlNodeRef& xmlNode, bool bUndo);
        void Save(XmlNodeRef& xmlNode, bool bUndo);
        static void CopyEdges(const std::vector<SEdge>& sourceEdges, std::vector<SEdge>& destincationEdges);
        static void AddEdges(const std::vector<BrushVec2>& positivePoints, const std::vector<BrushVec2>& negativePoints, std::vector<BrushVec2>& outPoints, std::vector<SEdge>& outEdges);
        void InitializeEdgesAndUpdate(bool bClosed);
        //! Remove colinear edges which have a same normal vector.
        bool FlattenEdges(std::vector<SVertex>& vertices, std::vector<SEdge>& edges) const;
        //! Remove edges whose two indices are same or which exist already.
        static void RemoveEdgesHavingSameIndices(std::vector<SEdge>& edges);
        //! enough short edges should be regarded as a vertex
        void RemoveEdgesRegardedAsVertex(std::vector<SVertex>& vertices, std::vector<SEdge>& edges) const;
        static BrushEdge ToEdge2D(const BrushPlane& plane, const BrushEdge3D& edge3D)
        {
            return BrushEdge(plane.W2P(edge3D.m_v[0]), plane.W2P(edge3D.m_v[1]));
        }
        bool ShouldOrderReverse(PolygonPtr BPolygon) const;
        void InvalidateCacheData() const
        {
            InvalidateBspTree();
            InvalidateConvexes();
            InvalidateTriangles();
            InvalidateRepresentativePos();
            InvalidateBoundBox();
            m_PrivateFlag = eRPF_Invalid;
        }
        void InitForCreation(const BrushPlane& plane = BrushPlane(BrushVec3(0, 0, 0), 0), int matID = 0);

        BrushFloat Cosine(int i0, int i1, int i2, const std::vector<SVertex>& vertices) const;
        bool IsCCW(int i0, int i1, int i2, const std::vector<SVertex>& vertices) const;
        bool IsCW(int i0, int i1, int i2, const std::vector<SVertex>& vertices) const{ return !IsCCW(i0, i1, i2, vertices); }
        void ChoosePrevEdge(const std::vector<SVertex>& vertices, const SEdge& edge, const EdgeIndexSet& candidateSecondIndices, SEdge& outEdge) const;
        void ChooseNextEdge(const std::vector<SVertex>& vertices, const SEdge& edge, const EdgeIndexSet& candidateSecondIndices, SEdge& outEdge) const;

        bool FindFirstEdgeIndex(const std::set<int>& edgeSet, int& outEdgeIndex) const;
        bool ExtractEdge3DList(std::vector<BrushEdge3D>& outList) const;
        bool SortVerticesAlongCrossLine(std::vector<BrushVec3>& vList, const BrushLine3D& crossLine, std::vector<BrushEdge3D>& outGapEdges) const;
        bool SortEdgesAlongCrossLine(std::vector<BrushEdge3D>& edgeList, const BrushLine3D& crossLine, std::vector<BrushEdge3D>& outGapEdges) const;
        bool QueryEdges(const BrushVec3& vertex, int vIndexInEdge, std::set<int>* pOutEdgeIndices = NULL) const;
        void GetBridgeEdgeSet(std::set<SEdge>& outBridgeEdgeSet, bool bOutputBothDirection) const;
        void UpdateBoundBox() const;
        void ConnectNearVertices(std::vector<SVertex>& vertices, std::vector<SEdge>& edges) const;
        void RemoveUnconnectedEdges(std::vector<SEdge>& edges) const;
        void UpdatePrivateFlags() const;

    private:

        void AddVertex_Basic(const SVertex& vertex)
        {
            InvalidateCacheData();
            m_Vertices.push_back(vertex);
        }
        void SetVertex_Basic(int nIndex, const SVertex& vertex)
        {
            InvalidateCacheData();
            m_Vertices[nIndex] = vertex;
        }
        void SetPos_Basic(int nIndex, const BrushVec3& pos)
        {
            InvalidateCacheData();
            m_Vertices[nIndex].pos = pos;
        }
        void DeleteAllVertices_Basic()
        {
            InvalidateCacheData();
            m_Vertices.clear();
        }
        void SetVertexList_Basic(const std::vector<SVertex>& vertexList)
        {
            InvalidateCacheData();
            if (&vertexList == &m_Vertices)
            {
                return;
            }
            m_Vertices = vertexList;
        }
        void AddEdge_Basic(const SEdge& edge)
        {
            InvalidateCacheData();
            m_Edges.push_back(edge);
        }
        void DeleteAllEdges_Basic()
        {
            InvalidateCacheData();
            m_Edges.clear();
        }
        void SetEdgeList_Basic(const std::vector<SEdge>& edgeList)
        {
            InvalidateCacheData();
            CopyEdges(edgeList, m_Edges);
        }
        void SwapEdgeIndex_Basic(int nIndex)
        {
            InvalidateCacheData();
            std::swap(m_Edges[nIndex].m_i[0], m_Edges[nIndex].m_i[1]);
        }

        bool CheckPrivateFlags(int nFlags) const
        {
            DESIGNER_ASSERT(nFlags != eRPF_Invalid);
            if (nFlags == eRPF_Invalid)
            {
                return false;
            }
            if (m_PrivateFlag == eRPF_Invalid)
            {
                UpdatePrivateFlags();
            }
            return m_PrivateFlag & nFlags ? true : false;
        }
        int AddPrivateFlags(int nFlags) const   { return m_PrivateFlag |=  nFlags; }
        int RemovePrivateFlags(int nFlags) const { return m_PrivateFlag &= ~nFlags; }

        void InvalidateBspTree() const;
        void InvalidateConvexes() const;
        void InvalidateTriangles() const;
        void InvalidateRepresentativePos() const;
        void InvalidateBoundBox() const;

    private:

        GUID m_GUID;
        std::vector<SVertex> m_Vertices;
        std::vector<SEdge> m_Edges;
        BrushPlane m_Plane;
        int m_MaterialID;
        STexInfo m_TexInfo;
        unsigned int m_Flag;

        // Cache Data
        mutable BSPTree2D* m_pBSPTree;
        mutable Convexes* m_pConvexes;
        mutable FlexibleMesh* m_pTriangles;
        mutable BrushVec3* m_pRepresentativePos;
        mutable SPolygonBound* m_pBoundBox;
        mutable unsigned int m_PrivateFlag;
    };

    typedef Polygon::PolygonPtr PolygonPtr;
    typedef std::vector<PolygonPtr> PolygonList;
};