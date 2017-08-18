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
//  File name:   Model.h
//  Created:     August/12/2011 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "Polygon.h"

class SmoothingGroupManager;
class EdgeSharpnessManager;

namespace CD
{
    class ModelDB;
    class BSPTree3D;
    class HalfEdgeMesh;

    enum EOperationType
    {
        eOpType_Split,
        eOpType_Union,
        eOpType_SubtractAB,
        eOpType_SubtractBA,
        eOpType_Intersection,
        eOpType_ExclusiveOR,
        eOpType_Add,
    };

    enum ESurroundType
    {
        eSurroundType_Surrounded,
        eSurroundType_Surrounding,
        eSurroundType_Partly,
        eSurroundType_None,
        eSurroundType_WrongInput,
    };

    enum EDesignerEditMode
    {
        eDesignerMode_FrameRemainAfterDrill = BIT(1),
        eDesignerMode_Mirror = BIT(3),
        eDesignerMode_DisplayBackFace = BIT(4)
    };

    enum EPolygonRelation
    {
        eER_None,
        eER_Intersection,
        eER_ZeroDistance
    };

    enum EFindOppositeFlag
    {
        eFOF_PushDirection,
        eFOF_PullDirection
    };

    enum EClipPolygonResult
    {
        eCPR_CLIPFAILED,
        eCPR_SUCCESSED,
        eCPR_CLIPSUCCESSEDBUTFILLFAILED
    };

    struct SQueryEdgeResult
    {
        SQueryEdgeResult(PolygonPtr pPolygon, const BrushEdge3D& edge)
        {
            m_pPolygon = pPolygon;
            m_Edge = edge;
        }
        PolygonPtr m_pPolygon;
        BrushEdge3D m_Edge;
    };
    typedef std::pair<PolygonPtr, BrushVec3> IntersectionPair;

    class Model
        : public CRefCountBase
    {
    public:

        Model();
        Model(const std::vector<PolygonPtr>& polygonList);
        Model(const Model& model);
        ~Model();

        Model& operator = (const Model& model);

        void Clear();
        void Display(DisplayContext& dc, const int nLineThickness = 2, const ColorB& lineColor = ColorB(0, 0, 2));
        void DisplaySubdividedMesh(DisplayContext& dc);
        bool AddPolygon(PolygonPtr pPolygon, EOperationType opType);
        void AddPolygonSeparately(PolygonPtr pPolygon, bool bAddedOnlyAsSeparated = false);
        void AddPolygonUnconditionally(PolygonPtr pPolygon);
        bool AddOpenPolygon(PolygonPtr pPolygon, bool bOnlyAdd);
        bool DrillPolygon(int nPolygonIndex, bool bRemainFrame = false);
        bool DrillPolygon(PolygonPtr pPolygon, bool bRemainFrame = false);
        bool DrillPolygon(Model* pModel);

        PolygonPtr QueryPolygon(REFGUID guid) const;
        bool QueryPolygon(const BrushPlane& plane, const BrushVec3& raySrc, const BrushVec3& rayDir, int& nOutIndex) const;
        bool QueryPolygons(const BrushPlane& plane, PolygonList& outPolygons) const;
        bool QueryIntersectedPolygonsByAABB(const AABB& aabb, PolygonList& outPolygons) const;
        bool QueryPolygon(const BrushVec3& raySrc, const BrushVec3& rayDir, int& nOutIndex) const;
        PolygonPtr QueryEquivalentPolygon(PolygonPtr pPolygon, int* pOutPolygonIndex = NULL) const;
        bool QueryNearestEdges(const BrushPlane& plane, const BrushVec3& raySrc, const BrushVec3& rayDir, BrushVec3& outPos, BrushVec3& outPosOnEdge, std::vector<SQueryEdgeResult>& outEdges) const;
        bool QueryNearestEdges(const BrushPlane& plane, const BrushVec3& position, BrushVec3& outPosOnEdge, std::vector<SQueryEdgeResult>& outEdges) const;
        bool QueryNearestEdges(const BrushVec3& raySrc, const BrushVec3& rayDir, BrushVec3& outPos, BrushVec3& outPosOnEdge, BrushPlane& outPlane, std::vector<SQueryEdgeResult>& outEdges) const;
        bool QueryPosition(const BrushPlane& plane, const BrushVec3& localRayOrigin, const BrushVec3& localRayDir, BrushVec3& outPosition, BrushFloat* outDist = NULL, PolygonPtr* outPolygon = NULL) const;
        bool QueryPosition(const BrushVec3& localRayOrigin, const BrushVec3& localRayDir, BrushVec3& outPosition, BrushPlane* outPlane = NULL, BrushFloat* outDist = NULL, PolygonPtr* outPolygon = NULL) const;
        bool QueryEdgesHavingVertex(const BrushVec3& vertex, std::vector<BrushEdge3D>& outEdges) const;
        bool QueryCenterOfPolygon(const BrushVec3& raySrc, const BrushVec3& rayDir, BrushVec3& outCenterOfPos) const;
        bool QueryNearestPosFromBoundary(const BrushVec3& pos, BrushVec3& outNearestPos) const;
        void QueryAdjacentPerpendicularPolygons(PolygonPtr pPolygon, PolygonList& outPolygons) const;
        void QueryPerpendicularPolygons(PolygonPtr pPolygon, PolygonList& outPolygons) const;
        EPolygonRelation QueryOppositePolygon(PolygonPtr pPolygon, EFindOppositeFlag nFlag, BrushFloat fScale, PolygonPtr& outPolygon, BrushFloat& outDistance) const;
        ESurroundType QuerySurroundType(int nPolygonIndex) const;
        void QueryIntersectionByPolygon(PolygonPtr pPolygon, PolygonList& outIntersetionPolygons) const;
        void QueryIntersectionByEdge(const BrushEdge3D& edge, std::vector<IntersectionPair>& outIntersections) const;
        void QueryIntersectionPolygonsWith2DRect(IDisplayViewport* pView, const BrushMatrix34& worldTM, PolygonPtr pRectPolygon, bool bExcludeBackFace, PolygonList& outIntersectionPolygons) const;
        void QueryIntersectionEdgesWith2DRect(IDisplayViewport* pView, const BrushMatrix34& worldTM, PolygonPtr pRectPolygon, bool bExcludeBackFace, EdgeQueryResult& outIntersectionEdges) const;
        void QueryOpenPolygons(const BrushVec3& raySrc, const BrushVec3& rayDir, std::vector<PolygonPtr>& outPolygons) const;
        void QueryNeighbourPolygonsByEdge(const BrushEdge3D& edge, PolygonList& neighbourPolygons) const;
        void QueryPolygonsSharingEdge(const BrushEdge3D& edge, PolygonList& outPolygons) const;

        void Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bUndo);
        void Replace(int nIndex, PolygonPtr pPolygon);
        void Optimize();
        void SeparatePolygons(const BrushPlane& plane);
        bool EraseEdge(const BrushEdge3D& edge);
        bool EraseVertex(const BrushVec3& vertex);
        bool HasIntersection(PolygonPtr pPolygon, bool bStrongCheck = false) const;
        bool HasTouched(PolygonPtr pPolygon) const;
        bool HasEdge(const BrushEdge3D& edge) const;
        void RecordUndo(const char* sUndoDescription, CBaseObject* pObject) const;
        bool RemovePolygon(int nPolygonIndex);
        bool RemovePolygon(PolygonPtr pPolygon);
        void RemovePolygonsWithSpecificFlagsPlane(int nFlags, const BrushPlane* pPlane = NULL);
        bool IsVertexOnEdge(const BrushPlane& plane, const BrushVec3& vertex, PolygonPtr pExcludedPolygon = NULL) const;
        void Move(const BrushVec3& offset);
        void MoveShelf(ShelfID sourceShelfID, ShelfID destShelfID);
        void Transform(const BrushMatrix34& tm);
        void GetPolygonList(PolygonList& outExportedPolygonList) const;
        void GetPolygons(const BrushPlane& plane, std::vector<int>& outClonedPolygons) const;
        PolygonPtr GetPolygon(int nPolygonIndex) const;
        int GetPolygonIndex(PolygonPtr pPolygon) const;
        void SetShelf(ShelfID shelfID) const
        {
            assert(shelfID >= 0 && shelfID < kMaxShelfCount);
            m_ShelfID = shelfID;
        }
        ShelfID GetShelf() const { return m_ShelfID; }
        int GetPolygonCount() const { return m_Polygons[m_ShelfID].size(); }
        void SetModeFlag(int nFlag) { m_nModeFlag = nFlag; }
        void AddModeFlag(int nFlag) { m_nModeFlag |= nFlag; }
        bool CheckModeFlag(int nFlags) { return m_nModeFlag & nFlags ? true : false; }
        int GetModeFlag() const { return m_nModeFlag; }
        void AddExcludedEdgeInDrawing(const BrushEdge3D& edge) { m_ExcludedEdgesInDrawing.push_back(edge); }
        void ClearExcludedEdgesInDrawing() { m_ExcludedEdgesInDrawing.clear(); }

        void SetSubdivisionLevel(unsigned char nLevel) { m_SubdivisionLevel = nLevel; }
        unsigned char GetSubdivisionLevel() const { return m_SubdivisionLevel; }

        void SetTessFactor(unsigned char nTessFactor) { m_nTessFactor = nTessFactor; }
        unsigned char GetTessFactor() const { return m_nTessFactor; }

        ModelDB* GetDB() const{ return m_pDB; }
        void ResetDB(int nFlag, int nValidShelfID = -1);

        SmoothingGroupManager* GetSmoothingGroupMgr() const { return m_pSmoothingGroupMgr; }
        EdgeSharpnessManager* GetEdgeSharpnessMgr() const { return m_pEdgeSharpnessMgr; }

        void SetSubdivisionResult(HalfEdgeMesh* pSubdividedHalfMesh);
        HalfEdgeMesh* GetSubdivisionResult() const { return m_pSubdividionResult; }

        void EnableSmoothingSurfaceForSubdividedMesh(bool bEnabled) { m_bSmoothingGroupForSubdividedSurfaces = bEnabled; }
        bool IsSmoothingSurfaceForSubdividedMeshEnabled() const { return m_bSmoothingGroupForSubdividedSurfaces; }

        EClipPolygonResult Clip(const BrushPlane& clipPlane, _smart_ptr<Model>& pOutFrontPart, _smart_ptr<Model>& pOutBackPart, bool bFillFacet) const;
        void SetMirrorPlane(const BrushPlane& mirrorPlane){ m_MirrorPlane = mirrorPlane; }
        const BrushPlane& GetMirrorPlane() const { return m_MirrorPlane; }
        void ResetFromList(const std::vector<PolygonPtr>& polygonList);

        bool IsEmpty(int nShelf = -1) const;
        bool HasClosedPolygon(int nShelf = -1) const;

        void Union(Model* BModel);
        void Subtract(Model* BModel);
        void Intersect(Model* BModel);
        void ClipOutside(Model* BModel);
        void ClipInside(Model* BModel);
        bool IsInside(const BrushVec3& vPos) const;

        AABB GetBoundBox(int nShelf = -1);
        void InvalidateAABB(int nShelf = -1) const
        {
            if (nShelf == 0 || nShelf == 1)
            {
                m_BoundBox[nShelf].bValid = false;
            }
            if (nShelf == -1)
            {
                m_BoundBox[0].bValid = m_BoundBox[1].bValid = false;
            }
        }

    private:

        struct SInterectedPolygon
        {
            PolygonPtr m_pPolygon;
            int m_nEdgeIndex;
            Vec2d m_IntersectionPt;
        };

        struct SControlPointInfo
        {
            SControlPointInfo(const Vec2d& point, bool bSkip)
            {
                m_Point = point;
                m_bSkip = bSkip;
            }
            Vec2d m_Point;
            bool m_bSkip;
        };

        typedef std::vector<SInterectedPolygon> IntersectedPolygonList;
        typedef std::map< Polygon*, IntersectedPolygonList > IntersectedPolygonMap;

    private:

        void Init();

        bool QueryNearestEdge(int nPolygonIndex, const BrushVec3& raySrc, const BrushVec3& rayDir, BrushVec3& outPos, BrushVec3& outPosOnEdge, BrushPlane& outPlane, BrushEdge3D& outEdge) const;
        bool QueryNearestEdge(int nPolygonIndex, const BrushVec3& position, BrushVec3& outPosOnEdge, BrushPlane& outPlane, BrushEdge3D& outEdge) const;
        bool UnionPolygon(PolygonPtr pPolygon);
        bool SubtractPolygonAB(PolygonPtr pPolygon);
        bool SubtractPolygonBA(PolygonPtr pPolygon);
        bool IntersectPolygon(PolygonPtr pPolygon);
        bool ExclusiveORPolygon(PolygonPtr pPolygon);
        bool SplitPolygon(PolygonPtr pPolygon);
        bool SplitPolygonsByOpenPolygon(PolygonPtr pOpenPolygon);
        bool GetPolygonPlane(int nPolygonIndex, BrushPlane& outPlane) const;
        void DeletePolygons(std::set<PolygonPtr>& deletedPolygons);
        PolygonList::iterator RemovePolygon(const PolygonList::iterator& iPolygon);
        PolygonPtr GetPolygonPtr(int nIndex) const;
        void DisplayPolygons(DisplayContext& dc, const int nLineThickness = 2, const ColorB& lineColor = ColorB(0, 0, 2));
        bool GetVisibleEdge(const BrushEdge3D& edge, const BrushPlane& plane, std::vector<BrushEdge3D>& outVisibleEdges) const;
        void AddPolygon(PolygonPtr pPolygon);
        void AddPolygon(int nShelf, PolygonPtr pPolygon);
        void Clip(const BSPTree3D* pTree, PolygonList& polygonList, EClipType cliptype, PolygonList& outPolygons, EClipObjective clipObjective);
        std::vector<PolygonPtr> GetIntersectedParts(PolygonPtr pPolygon) const;
        PolygonPtr QueryEquivalentPolygon(int nShelfID, PolygonPtr pPolygon, int* pOutPolygonIndex = NULL) const;
        static bool GeneratePolygonsFromEdgeList(std::vector<BrushEdge3D>& edgeList, const BrushPlane& plane, PolygonList& outPolygons);

    private:

        struct ModelBound
        {
            ModelBound()
                : bValid(false)
            {
                aabb.Reset();
            }
            AABB aabb;
            bool bValid;
        };

        mutable ShelfID m_ShelfID;
        std::vector<BrushEdge3D> m_ExcludedEdgesInDrawing;
        PolygonList m_Polygons[kMaxShelfCount];
        mutable ModelBound m_BoundBox[kMaxShelfCount];
        BrushPlane m_MirrorPlane;
        unsigned char m_SubdivisionLevel;
        unsigned char m_nTessFactor;
        bool m_bSmoothingGroupForSubdividedSurfaces;
        int m_nModeFlag;

        ModelDB* m_pDB;
        SmoothingGroupManager* m_pSmoothingGroupMgr;
        EdgeSharpnessManager* m_pEdgeSharpnessMgr;
        HalfEdgeMesh* m_pSubdividionResult;
    };

    class CShelfIDReconstructor
    {
    public:

        CShelfIDReconstructor(Model* pModel)
            : m_pModel(pModel)
        {
            if (m_pModel)
            {
                m_ShelfID = m_pModel->GetShelf();
            }
        }

        CShelfIDReconstructor(Model* pModel, ShelfID shelfID)
            : m_pModel(pModel)
            , m_ShelfID(shelfID)
        {
        }

        ~CShelfIDReconstructor()
        {
            if (m_pModel)
            {
                m_pModel->SetShelf(m_ShelfID);
            }
        }

    private:
        _smart_ptr<Model> m_pModel;
        ShelfID m_ShelfID;
    };
};

#define MODEL_SHELF_RECONSTRUCTOR_POSTFIX(pModel, nPostFix) CD::CShelfIDReconstructor shelfIDReconstructor##nPostFix(pModel);
#define MODEL_SHELF_RECONSTRUCTOR(pModel) MODEL_SHELF_RECONSTRUCTOR_POSTFIX(pModel, 0);
