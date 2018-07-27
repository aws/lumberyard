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

#include "StdAfx.h"
#include "ModelCompiler.h"
#include "Model.h"
#include "Util/Undo.h"
#include "Util/EdgesSharpnessManager.h"
#include "SmoothingGroupManager.h"
#include "HalfEdgeMesh.h"
#include "ModelDB.h"

namespace CD
{
    void Model::Init()
    {
        m_nModeFlag = 0;
        m_ShelfID = 0;
        m_MirrorPlane = BrushPlane(BrushVec3(0, 0, 0), 0);
        m_SubdivisionLevel = 0;
        m_nTessFactor = 0;
        m_pDB = new ModelDB;
        m_pDB->AddRef();
        m_pSmoothingGroupMgr = new SmoothingGroupManager;
        m_pSmoothingGroupMgr->AddRef();
        m_pEdgeSharpnessMgr = new EdgeSharpnessManager;
        m_pEdgeSharpnessMgr->AddRef();
        m_pSubdividionResult = NULL;
        m_bSmoothingGroupForSubdividedSurfaces = false;
    }

    Model::Model()
    {
        Init();
    }

    Model::~Model()
    {
        if (m_pDB)
        {
            m_pDB->Release();
        }
        if (m_pSmoothingGroupMgr)
        {
            m_pSmoothingGroupMgr->Release();
        }
        if (m_pEdgeSharpnessMgr)
        {
            m_pEdgeSharpnessMgr->Release();
        }
        if (m_pSubdividionResult)
        {
            m_pSubdividionResult->Release();
        }
    }

    Model::Model(const std::vector<PolygonPtr>& polygonList)
    {
        Init();
        m_Polygons[0] = polygonList;
    }

    Model::Model(const Model& model)
        : CRefCountBase()
    {
        Init();
        operator =(model);
    }

    Model& Model::operator = (const Model& model)
    {
        for (ShelfID shelfID = 0; shelfID < kMaxShelfCount; ++shelfID)
        {
            SetShelf(shelfID);
            Clear();
        }

        for (ShelfID shelfID = 0; shelfID < kMaxShelfCount; ++shelfID)
        {
            int nPolygonSize(model.m_Polygons[shelfID].size());
            m_Polygons[shelfID].reserve(nPolygonSize);
            for (int i = 0; i < nPolygonSize; ++i)
            {
                m_Polygons[shelfID].push_back(model.m_Polygons[shelfID][i]->Clone());
            }
        }

        m_MirrorPlane = model.m_MirrorPlane;
        m_nModeFlag = model.m_nModeFlag;
        m_SubdivisionLevel = model.m_SubdivisionLevel;
        m_nTessFactor = model.m_nTessFactor;

        *m_pDB = *model.m_pDB;
        m_pSmoothingGroupMgr->CopyFromModel(this, &model);
        m_pEdgeSharpnessMgr->CopyFromModel(this, &model);

        m_ShelfID = 0;

        return *this;
    }

    bool Model::QueryPosition(const BrushPlane& plane, const BrushVec3& localRayOrigin, const BrushVec3& localRayDir, BrushVec3& outPosition, BrushFloat* outDist, PolygonPtr* outPolygon) const
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);

        BrushFloat dist;
        if (plane.HitTest(localRayOrigin, localRayOrigin + localRayDir, &dist, &outPosition) == false)
        {
            return false;
        }
        int nPolygon(-1);
        if (outPolygon && QueryPolygon(plane, localRayOrigin, localRayDir, nPolygon))
        {
            *outPolygon = m_Polygons[m_ShelfID][nPolygon];
        }
        if (outDist)
        {
            *outDist = dist;
        }
        return true;
    }

    bool Model::QueryPosition(const BrushVec3& localRayOrigin, const BrushVec3& localRayDir, BrushVec3& outPosition, BrushPlane* outPlane, BrushFloat* outDist, PolygonPtr* outPolygon) const
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);

        int nPolygon(-1);
        if (QueryPolygon(localRayOrigin, localRayDir, nPolygon) == false)
        {
            return false;
        }

        PolygonPtr pPolygon(m_Polygons[m_ShelfID][nPolygon]);
        BrushFloat dist;
        if (pPolygon->GetPlane().HitTest(localRayOrigin, localRayOrigin + localRayDir, &dist, &outPosition) == false)
        {
            return false;
        }

        if (outPolygon)
        {
            *outPolygon = pPolygon;
        }
        if (outPlane)
        {
            *outPlane = pPolygon->GetPlane();
        }
        if (outDist)
        {
            *outDist = dist;
        }

        return true;
    }

    bool Model::QueryEdgesHavingVertex(const BrushVec3& vertex, std::vector<BrushEdge3D>& outEdges) const
    {
        bool bFound = false;

        for (int i = 0, iPolygonCount(GetPolygonCount()); i < iPolygonCount; ++i)
        {
            PolygonPtr pPolygon = GetPolygon(i);

            std::vector<PolygonPtr> polygonsOutside;
            if (!pPolygon->IsOpen())
            {
                if (!pPolygon->GetSeparatedPolygons(polygonsOutside, eSP_OuterHull) || polygonsOutside.empty())
                {
                    continue;
                }
            }
            else
            {
                polygonsOutside.push_back(pPolygon);
            }

            for (int k = 0, iPolygonsOutsideCount(polygonsOutside.size()); k < iPolygonsOutsideCount; ++k)
            {
                pPolygon = polygonsOutside[k];
                std::vector<int> edgeIndices;
                if (!pPolygon->QueryEdgesHavingVertex(vertex, edgeIndices))
                {
                    continue;
                }
                bFound = true;
                for (int k = 0, iEdgeCount(edgeIndices.size()); k < iEdgeCount; ++k)
                {
                    outEdges.push_back(pPolygon->GetEdge(edgeIndices[k]));
                }
            }
        }

        return bFound;
    }

    void Model::QueryOpenPolygons(const BrushVec3& raySrc, const BrushVec3& rayDir, std::vector<PolygonPtr>& outPolygons) const
    {
        for (int i = 0, iPolygonCount(m_Polygons[m_ShelfID].size()); i < iPolygonCount; ++i)
        {
            if (!m_Polygons[m_ShelfID][i]->IsOpen())
            {
                continue;
            }
            Vec3 vHitPos;
            AABB aabb = m_Polygons[m_ShelfID][i]->GetBoundBox();
            aabb.Expand(Vec3(0.01f));
            if (Intersect::Ray_AABB(ToVec3(raySrc), ToVec3(rayDir), aabb, vHitPos))
            {
                outPolygons.push_back(m_Polygons[m_ShelfID][i]);
            }
        }
    }

    bool Model::AddPolygon(PolygonPtr pPolygon, EOperationType opType)
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);

        if (!pPolygon)
        {
            return false;
        }

        bool bSuccess(false);
        if (opType == eOpType_Add || pPolygon->IsOpen())
        {
            bSuccess = true;
            AddPolygon(pPolygon);
        }
        else if (opType == eOpType_Union)
        {
            bSuccess = UnionPolygon(pPolygon);
        }
        else if (opType == eOpType_SubtractAB)
        {
            bSuccess = SubtractPolygonAB(pPolygon);
        }
        else if (opType == eOpType_SubtractBA)
        {
            bSuccess = SubtractPolygonBA(pPolygon);
        }
        else if (opType == eOpType_Intersection)
        {
            bSuccess = IntersectPolygon(pPolygon);
        }
        else if (opType == eOpType_Split)
        {
            bSuccess = SplitPolygon(pPolygon);
        }
        else if (opType == eOpType_ExclusiveOR)
        {
            bSuccess = ExclusiveORPolygon(pPolygon);
        }

        return bSuccess;
    }

    void Model::AddPolygonUnconditionally(PolygonPtr pPolygon)
    {
        pPolygon->UpdateUVs();
        m_Polygons[m_ShelfID].push_back(pPolygon);
    }

    bool Model::AddOpenPolygon(PolygonPtr pPolygon, bool bOnlyAdd)
    {
        DESIGNER_ASSERT(pPolygon->IsOpen());

        if (!pPolygon || !pPolygon->IsOpen())
        {
            return false;
        }

        if (bOnlyAdd)
        {
            AddPolygon(pPolygon->Clone());
            return true;
        }

        return SplitPolygonsByOpenPolygon(pPolygon);
    }

    bool Model::UnionPolygon(PolygonPtr pPolygon)
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);

        if (!pPolygon || pPolygon->IsOpen())
        {
            return false;
        }

        AABB aabb = pPolygon->GetBoundBox();
        aabb.Expand(Vec3(0.01f, 0.01f, 0.01f));

        PolygonPtr pNewPolygon = pPolygon->Clone();
        std::set<PolygonPtr> deletedPolygons;
        for (int i = 0, iPolygonSize(m_Polygons[m_ShelfID].size()); i < iPolygonSize; ++i)
        {
            if (m_Polygons[m_ShelfID][i]->IsOpen())
            {
                continue;
            }

            if (pPolygon->CheckFlags(ePolyFlag_Mirrored) != m_Polygons[m_ShelfID][i]->CheckFlags(ePolyFlag_Mirrored))
            {
                continue;
            }

            if (!aabb.IsIntersectBox(m_Polygons[m_ShelfID][i]->GetBoundBox()))
            {
                continue;
            }

            if (Polygon::HasIntersection(m_Polygons[m_ShelfID][i], pPolygon) != eIT_None)
            {
                PolygonPtr pBackupPolygon = pNewPolygon->Clone();

                if (pNewPolygon->Union(m_Polygons[m_ShelfID][i]))
                {
                    deletedPolygons.insert(m_Polygons[m_ShelfID][i]);
                }
                else
                {
                    return false;
                }
            }
        }

        if (!pNewPolygon->IsValid() || pNewPolygon->IsOpen())
        {
            return false;
        }

        DeletePolygons(deletedPolygons);
        AddPolygonSeparately(pNewPolygon);
        InvalidateAABB(m_ShelfID);

        return true;
    }

    bool Model::SubtractPolygonAB(PolygonPtr pPolygon)
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);

        if (!pPolygon || pPolygon->IsOpen())
        {
            return false;
        }

        AABB aabb = pPolygon->GetBoundBox();
        aabb.Expand(Vec3(0.01f, 0.01f, 0.01f));

        PolygonPtr pNewPolygon = pPolygon->Clone();
        bool bIntersetion(false);

        std::vector<PolygonPtr> copiedPolygons(m_Polygons[m_ShelfID]);
        std::vector<PolygonPtr>::iterator ii = copiedPolygons.begin();

        for (; ii != copiedPolygons.end(); ++ii)
        {
            if ((*ii)->IsOpen())
            {
                continue;
            }
            if (pPolygon->CheckFlags(ePolyFlag_Mirrored) != (*ii)->CheckFlags(ePolyFlag_Mirrored))
            {
                continue;
            }
            if (!aabb.IsIntersectBox((*ii)->GetBoundBox()))
            {
                continue;
            }
            if ((*ii)->Subtract(pNewPolygon))
            {
                bIntersetion = true;
                std::vector<PolygonPtr>::iterator ioriginal = m_Polygons[m_ShelfID].begin();
                for (; ioriginal != m_Polygons[m_ShelfID].end(); ++ioriginal)
                {
                    if (*ioriginal == *ii)
                    {
                        ioriginal = RemovePolygon(ioriginal);
                        break;
                    }
                }
                if ((*ii)->IsValid())
                {
                    AddPolygonSeparately(*ii);
                }
            }
        }

        if (bIntersetion == false)
        {
            AddPolygonSeparately(pNewPolygon);
        }

        InvalidateAABB(m_ShelfID);

        return true;
    }

    bool Model::SubtractPolygonBA(PolygonPtr pPolygon)
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);

        if (!pPolygon || pPolygon->IsOpen())
        {
            return false;
        }

        AABB aabb = pPolygon->GetBoundBox();
        aabb.Expand(Vec3(0.01f, 0.01f, 0.01f));

        PolygonPtr pNewPolygon = pPolygon->Clone();
        std::vector<PolygonPtr>::iterator ii = m_Polygons[m_ShelfID].begin();

        for (; ii !=  m_Polygons[m_ShelfID].end(); ++ii)
        {
            if (pPolygon->IsOpen())
            {
                continue;
            }
            if (pPolygon->CheckFlags(ePolyFlag_Mirrored) != (*ii)->CheckFlags(ePolyFlag_Mirrored))
            {
                continue;
            }
            if (!aabb.IsIntersectBox((*ii)->GetBoundBox()))
            {
                continue;
            }
            pNewPolygon->Subtract(*ii);
        }

        if (pNewPolygon->IsValid())
        {
            AddPolygonSeparately(pNewPolygon);
        }

        InvalidateAABB(m_ShelfID);

        return true;
    }

    bool Model::IntersectPolygon(PolygonPtr pPolygon)
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);

        if (!pPolygon || pPolygon->IsOpen())
        {
            return false;
        }

        AABB aabb = pPolygon->GetBoundBox();
        aabb.Expand(Vec3(0.01f, 0.01f, 0.01f));

        PolygonPtr pNewPolygon = pPolygon->Clone();
        std::vector<PolygonPtr> intersectedPolygons;
        for (int i = 0, iPolygonSize(m_Polygons[m_ShelfID].size()); i < iPolygonSize; ++i)
        {
            if (m_Polygons[m_ShelfID][i]->IsOpen())
            {
                continue;
            }
            if (pPolygon->CheckFlags(ePolyFlag_Mirrored) != m_Polygons[m_ShelfID][i]->CheckFlags(ePolyFlag_Mirrored))
            {
                continue;
            }
            if (!aabb.IsIntersectBox(m_Polygons[m_ShelfID][i]->GetBoundBox()))
            {
                continue;
            }
            if (!m_Polygons[m_ShelfID][i]->Intersect(pNewPolygon, eICEII_IncludeCoSame))
            {
                return false;
            }
            intersectedPolygons.push_back(m_Polygons[m_ShelfID][i]);
        }

        if (intersectedPolygons.empty())
        {
            AddPolygon(pNewPolygon);
        }
        else
        {
            for (int i = 0, iPolygonCount(intersectedPolygons.size()); i < iPolygonCount; ++i)
            {
                if (!intersectedPolygons[i]->IsValid())
                {
                    RemovePolygon(intersectedPolygons[i]);
                }
                else
                {
                    AddPolygonSeparately(intersectedPolygons[i], true);
                }
            }
        }

        InvalidateAABB(m_ShelfID);

        return true;
    }

    bool Model::ExclusiveORPolygon(PolygonPtr pPolygon)
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);

        if (!pPolygon || pPolygon->IsOpen())
        {
            return false;
        }

        AABB aabb = pPolygon->GetBoundBox();
        aabb.Expand(Vec3(0.01f, 0.01f, 0.01f));

        std::vector<PolygonPtr> overlappedPolygons;
        std::vector<PolygonPtr> overlappedReplicatedPolygons;
        std::vector<PolygonPtr> touchedPolygons;

        for (int i = 0, iPolygonSize(m_Polygons[m_ShelfID].size()); i < iPolygonSize; ++i)
        {
            if (m_Polygons[m_ShelfID][i]->IsOpen())
            {
                continue;
            }
            if (pPolygon->CheckFlags(ePolyFlag_Mirrored) != m_Polygons[m_ShelfID][i]->CheckFlags(ePolyFlag_Mirrored))
            {
                continue;
            }
            if (!aabb.IsIntersectBox(m_Polygons[m_ShelfID][i]->GetBoundBox()))
            {
                continue;
            }
            if (!pPolygon->GetPlane().IsEquivalent(m_Polygons[m_ShelfID][i]->GetPlane()))
            {
                continue;
            }

            EIntersectionType it = Polygon::HasIntersection(m_Polygons[m_ShelfID][i], pPolygon);
            if (it == eIT_Intersection || it == eIT_JustTouch && m_Polygons[m_ShelfID][i]->HasBridgeEdges())
            {
                overlappedPolygons.push_back(m_Polygons[m_ShelfID][i]);
                overlappedReplicatedPolygons.push_back(m_Polygons[m_ShelfID][i]->Clone());
            }
            else if (it == eIT_JustTouch)
            {
                touchedPolygons.push_back(m_Polygons[m_ShelfID][i]);
            }
        }

        int iOverlappedPolygonCount = overlappedPolygons.size();
        if (iOverlappedPolygonCount == 0)
        {
            if (!touchedPolygons.empty())
            {
                touchedPolygons[0]->Union(pPolygon);
                for (int i = 1, iTouchedPolygonCount(touchedPolygons.size()); i < iTouchedPolygonCount; ++i)
                {
                    touchedPolygons[0]->Union(touchedPolygons[i]);
                    RemovePolygon(touchedPolygons[i]);
                }
            }
            else
            {
                AddPolygon(pPolygon->Clone());
            }
            InvalidateAABB(m_ShelfID);
            return true;
        }
        else if (iOverlappedPolygonCount == 1 && pPolygon->IncludeAllEdges(overlappedPolygons[0]))
        {
            PolygonPtr pInputPolygon = pPolygon->Clone();
            pInputPolygon->Subtract(overlappedPolygons[0]);
            if (pInputPolygon->IsValid())
            {
                pInputPolygon->Flip();
                *overlappedPolygons[0] = *pInputPolygon;
                AddPolygonSeparately(overlappedPolygons[0], true);
            }
            else
            {
                std::vector<PolygonPtr>::iterator ii = m_Polygons[m_ShelfID].begin();
                for (; ii != m_Polygons[m_ShelfID].end(); ++ii)
                {
                    if (*ii == overlappedPolygons[0])
                    {
                        RemovePolygon(ii);
                        break;
                    }
                }
            }
            InvalidateAABB(m_ShelfID);
            return true;
        }
        else
        {
            for (int i = 0; i < iOverlappedPolygonCount; ++i)
            {
                if (pPolygon->IncludeAllEdges(overlappedPolygons[i]))
                {
                    std::vector<PolygonPtr>::iterator ii = m_Polygons[m_ShelfID].begin();
                    for (; ii != m_Polygons[m_ShelfID].end(); ++ii)
                    {
                        if (*ii == overlappedPolygons[i])
                        {
                            RemovePolygon(ii);
                            InvalidateAABB(m_ShelfID);
                            break;
                        }
                    }
                }
                else
                {
                    PolygonPtr pInputPolygon(pPolygon->Clone());
                    for (int k = 0; k < iOverlappedPolygonCount; ++k)
                    {
                        if (i == k)
                        {
                            continue;
                        }
                        pInputPolygon->Subtract(overlappedReplicatedPolygons[k]);
                    }

                    overlappedPolygons[i]->Subtract(pInputPolygon);
                    AddPolygonSeparately(overlappedPolygons[i], true);

                    pInputPolygon->Subtract(overlappedReplicatedPolygons[i]);
                    if (pInputPolygon->IsValid())
                    {
                        pInputPolygon->Flip();
                        AddPolygonSeparately(pInputPolygon);
                    }

                    InvalidateAABB(m_ShelfID);
                }
            }
        }

        return true;
    }

    void Model::Replace(int nIndex, PolygonPtr pPolygon)
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);

        if (!pPolygon)
        {
            return;
        }
        if (nIndex < 0 || nIndex >= m_Polygons[m_ShelfID].size())
        {
            return;
        }

        *(m_Polygons[m_ShelfID][nIndex]) = *pPolygon;
        InvalidateAABB(m_ShelfID);
    }

    bool Model::SplitPolygon(PolygonPtr pPolygon)
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);

        if (!pPolygon || pPolygon->IsOpen())
        {
            return false;
        }

        AABB aabb = pPolygon->GetBoundBox();
        aabb.Expand(Vec3(0.01f, 0.01f, 0.01f));

        PolygonPtr pEnteredPolygon = pPolygon->Clone();

        std::set<PolygonPtr> deletedPolygons;
        std::vector<PolygonPtr> spannedPolygons;
        for (int i = 0, iPolygonSize(m_Polygons[m_ShelfID].size()); i < iPolygonSize; ++i)
        {
            if (m_Polygons[m_ShelfID][i]->IsOpen())
            {
                continue;
            }
            if (pPolygon->CheckFlags(ePolyFlag_Mirrored) != m_Polygons[m_ShelfID][i]->CheckFlags(ePolyFlag_Mirrored))
            {
                continue;
            }
            if (!aabb.IsIntersectBox(m_Polygons[m_ShelfID][i]->GetBoundBox()))
            {
                continue;
            }
            if (!pPolygon->GetPlane().IsEquivalent(m_Polygons[m_ShelfID][i]->GetPlane()))
            {
                continue;
            }
            if (Polygon::HasIntersection(m_Polygons[m_ShelfID][i], pEnteredPolygon) == eIT_Intersection)
            {
                spannedPolygons.push_back(m_Polygons[m_ShelfID][i]->Clone());
                deletedPolygons.insert(m_Polygons[m_ShelfID][i]);
            }
        }

        if (spannedPolygons.empty())
        {
            if (pEnteredPolygon)
            {
                AddPolygon(pEnteredPolygon);
            }
            return true;
        }

        for (int i = 0, iSize(spannedPolygons.size()); i < iSize; ++i)
        {
            if (!pEnteredPolygon->Subtract(spannedPolygons[i]))
            {
                break;
            }
        }

        std::vector<PolygonPtr> intersectedPolygons;
        for (int i = 0, iSize(spannedPolygons.size()); i < iSize; ++i)
        {
            PolygonPtr intersectedPolygon = spannedPolygons[i]->Clone();
            if (intersectedPolygon->Intersect(pPolygon, eICEII_IncludeCoSame))
            {
                intersectedPolygons.push_back(intersectedPolygon);
            }
        }

        std::vector<PolygonPtr> subtractedPolygons(spannedPolygons);
        for (int i = 0, iSize(subtractedPolygons.size()); i < iSize; ++i)
        {
            if (!subtractedPolygons[i]->Subtract(pPolygon))
            {
                return false;
            }
        }

        DeletePolygons(deletedPolygons);

        for (int i = 0, iSize(subtractedPolygons.size()); i < iSize; ++i)
        {
            AddPolygonSeparately(subtractedPolygons[i]);
        }

        for (int i = 0, iSize(intersectedPolygons.size()); i < iSize; ++i)
        {
            AddPolygonSeparately(intersectedPolygons[i]);
        }

        if (pEnteredPolygon && pEnteredPolygon->IsValid())
        {
            AddPolygonSeparately(pEnteredPolygon);
        }

        return true;
    }

    void Model::AddPolygonSeparately(PolygonPtr pPolygon, bool bAddedOnlyAsSeparated)
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);

        if (!pPolygon->IsValid())
        {
            return;
        }

        std::vector<PolygonPtr> separatedPolygons;
        if (pPolygon->GetSeparatedPolygons(separatedPolygons) && separatedPolygons.size() >= 2)
        {
            for (int k = 0, iSeparatedPolygonSize(separatedPolygons.size()); k < iSeparatedPolygonSize; ++k)
            {
                DESIGNER_ASSERT(separatedPolygons[k]->IsValid());
                AddPolygon(separatedPolygons[k]);
                RemovePolygon(pPolygon);
            }
        }
        else if (!bAddedOnlyAsSeparated)
        {
            AddPolygon(pPolygon);
        }

        pPolygon->UpdateUVs();
    }

    bool Model::DrillPolygon(int nPolygonIndex, bool bRemainFrame)
    {
        PolygonPtr pPolygon(GetPolygonPtr(nPolygonIndex));
        if (pPolygon == NULL)
        {
            return false;
        }

        bool bDrillAfterOffsetMode = m_nModeFlag & eDesignerMode_FrameRemainAfterDrill;
        if (bRemainFrame && bDrillAfterOffsetMode)
        {
            ESurroundType surroundType = QuerySurroundType(nPolygonIndex);
            bool bSurrounding = surroundType == eSurroundType_None || surroundType == eSurroundType_Surrounding || surroundType == eSurroundType_Partly;
            if (bSurrounding)
            {
                PolygonPtr pScaledPolygon = pPolygon->Clone();
                pScaledPolygon->RemoveInside();
                pScaledPolygon->Scale((BrushFloat)0.1);
                AddPolygon(pScaledPolygon, eOpType_SubtractAB);
                return true;
            }
        }

        RemovePolygon(nPolygonIndex);

        return true;
    }

    bool Model::DrillPolygon(PolygonPtr pPolygon, bool bRemainFrame)
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);

        for (int i = 0, iPolygonCount(m_Polygons[m_ShelfID].size()); i < iPolygonCount; ++i)
        {
            if (m_Polygons[m_ShelfID][i] == pPolygon)
            {
                return DrillPolygon(i, bRemainFrame);
            }
        }
        return false;
    }

    bool Model::DrillPolygon(Model* pModel)
    {
        for (int i = 0, iPolygonCount(GetPolygonCount()); i < iPolygonCount; ++i)
        {
            std::vector<PolygonPtr> pPolygons;
            GetPolygon(i)->GetSeparatedPolygons(pPolygons, eSP_OuterHull);
            if (pPolygons.size() != 1)
            {
                continue;
            }
            std::vector<PolygonPtr> intersections = pModel->GetIntersectedParts(pPolygons[0]);
            if (intersections.empty())
            {
                continue;
            }

            for (int k = 0, iIntersectionCount(intersections.size()); k < iIntersectionCount; ++k)
            {
                AddPolygon(intersections[k], eOpType_SubtractAB);
            }
        }

        return true;
    }

    void Model::RecordUndo(const char* sUndoDescription, CBaseObject* pObject) const
    {
        if (pObject == NULL)
        {
            return;
        }
        if (CUndo::IsRecording())
        {
            CUndo::Record(new DesignerUndo(pObject, this, sUndoDescription));
        }
    }

    bool Model::EraseEdge(const BrushEdge3D& edge)
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);

        std::vector<PolygonPtr> neighbourPolygons;

        QueryNeighbourPolygonsByEdge(edge, neighbourPolygons);
        if (neighbourPolygons.empty())
        {
            return false;
        }

        if (neighbourPolygons.size() == 1 && neighbourPolygons[0]->IsOpen())
        {
            int edgeIndex;
            if (!neighbourPolygons[0]->IsEdgeOnCrust(edge, &edgeIndex))
            {
                return false;
            }

            std::vector<PolygonPtr> splittedPolygons;
            neighbourPolygons[0]->ClipByEdge(edgeIndex, splittedPolygons);
            RemovePolygon(neighbourPolygons[0]);
            if (splittedPolygons.empty())
            {
                return true;
            }
            for (int i = 0, iPolygonSize(splittedPolygons.size()); i < iPolygonSize; ++i)
            {
                AddOpenPolygon(splittedPolygons[i], true);
            }
        }
        else if (neighbourPolygons.size() == 1 && neighbourPolygons[0]->HasEdge(edge, true) && neighbourPolygons[0]->HasEdge(edge.GetInverted(), true))
        {
            neighbourPolygons[0]->RemoveEdge(edge);
            neighbourPolygons[0]->RemoveEdge(edge.GetInverted());
        }
        else if (neighbourPolygons.size() == 2)
        {
            if (neighbourPolygons[0]->IncludeAllEdges(neighbourPolygons[1]))
            {
                neighbourPolygons[0]->Union(neighbourPolygons[1]);
                neighbourPolygons[1]->RemoveEdge(edge);
            }
            else if (neighbourPolygons[1]->IncludeAllEdges(neighbourPolygons[0]))
            {
                neighbourPolygons[1]->Union(neighbourPolygons[0]);
                neighbourPolygons[0]->RemoveEdge(edge);
            }
            else if (!neighbourPolygons[0]->GetPlane().IsEquivalent(neighbourPolygons[1]->GetPlane()))
            {
                RemovePolygon(neighbourPolygons[0]);
                RemovePolygon(neighbourPolygons[1]);
            }
            else
            {
                PolygonPtr pClone0 = neighbourPolygons[0]->Clone();
                neighbourPolygons[0]->Union(neighbourPolygons[1]);

                RemovePolygon(neighbourPolygons[1]);

                neighbourPolygons[1]->Intersect(pClone0, eICEII_IncludeCoDiff);

                std::vector<PolygonPtr> unconnectedPolygons;
                neighbourPolygons[1]->GetUnconnectedPolygons(unconnectedPolygons);

                for (int k = 0, iUnconnectedSize(unconnectedPolygons.size()); k < iUnconnectedSize; ++k)
                {
                    int edgeIndex;

                    if (unconnectedPolygons[k]->IsEdgeOnCrust(edge, &edgeIndex))
                    {
                        std::vector<PolygonPtr> splittedPolygons;
                        unconnectedPolygons[k]->ClipByEdge(edgeIndex, splittedPolygons);
                        for (int i = 0, iSize(splittedPolygons.size()); i < iSize; ++i)
                        {
                            AddOpenPolygon(splittedPolygons[i], true);
                        }
                    }
                    else
                    {
                        AddOpenPolygon(unconnectedPolygons[k], true);
                    }
                }
            }
        }
        else
        {
            return false;
        }

        return true;
    }

    bool Model::EraseVertex(const BrushVec3& vertex)
    {
        ModelDB::QueryResult qlist;
        GetDB()->QueryAsVertex(vertex, qlist);
        if (qlist.empty())
        {
            return false;
        }

        for (int k = 0, iQListCount(qlist.size()); k < iQListCount; ++k)
        {
            for (int a = 0, iMarkCount(qlist[k].m_MarkList.size()); a < iMarkCount; ++a)
            {
                RemovePolygon(qlist[k].m_MarkList[a].m_pPolygon);
            }
        }

        return true;
    }

    void Model::QueryNeighbourPolygonsByEdge(const BrushEdge3D& edge, PolygonList& neighbourPolygons) const
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);

        BrushVec3 edgeDir = (edge.m_v[1] - edge.m_v[0]).GetNormalized();

        for (int i = 0, iPolygonSize(m_Polygons[m_ShelfID].size()); i < iPolygonSize; ++i)
        {
            const PolygonPtr& pPolygon(m_Polygons[m_ShelfID][i]);
            if (pPolygon == NULL)
            {
                continue;
            }
            if (!pPolygon->IsOpen() && std::abs(pPolygon->GetPlane().Normal().Dot(edgeDir)) > kDesignerLooseEpsilon)
            {
                continue;
            }
            if (!pPolygon->IsEdgeOnCrust(edge))
            {
                continue;
            }
            neighbourPolygons.push_back(pPolygon);
        }
    }

    void Model::QueryPolygonsSharingEdge(const BrushEdge3D& edge, PolygonList& outPolygons) const
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);

        AABB edgeAABB;
        edgeAABB.Reset();
        edgeAABB.Add(ToVec3(edge.m_v[0]));
        edgeAABB.Add(ToVec3(edge.m_v[1]));
        edgeAABB.Expand(Vec3(0.001f, 0.001f, 0.001f));

        for (int i = 0, iPolygonSize(m_Polygons[m_ShelfID].size()); i < iPolygonSize; ++i)
        {
            const PolygonPtr& pPolygon(m_Polygons[m_ShelfID][i]);
            if (pPolygon == NULL)
            {
                continue;
            }
            if (!pPolygon->IntersectedBetweenAABBs(edgeAABB))
            {
                continue;
            }
            if (pPolygon->HasEdge(edge))
            {
                outPolygons.push_back(pPolygon);
            }
        }
    }

    bool Model::HasIntersection(PolygonPtr pPolygon, bool bStrongCheck) const
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);

        if (!pPolygon)
        {
            return false;
        }

        for (int i = 0, iPolygonSize(m_Polygons[m_ShelfID].size()); i < iPolygonSize; ++i)
        {
            if (pPolygon == m_Polygons[m_ShelfID][i])
            {
                continue;
            }
            if (bStrongCheck)
            {
                if (Polygon::HasIntersection(m_Polygons[m_ShelfID][i], pPolygon) == eIT_Intersection)
                {
                    return true;
                }
            }
            else
            {
                if (Polygon::HasIntersection(m_Polygons[m_ShelfID][i], pPolygon) != eIT_None)
                {
                    return true;
                }
            }
        }
        return false;
    }

    bool Model::HasTouched(PolygonPtr pPolygon) const
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);

        if (!pPolygon)
        {
            return false;
        }

        for (int i = 0, iPolygonSize(m_Polygons[m_ShelfID].size()); i < iPolygonSize; ++i)
        {
            if (pPolygon == m_Polygons[m_ShelfID][i])
            {
                continue;
            }
            if (Polygon::HasIntersection(m_Polygons[m_ShelfID][i], pPolygon) == eIT_JustTouch)
            {
                return true;
            }
        }
        return false;
    }

    bool Model::HasEdge(const BrushEdge3D& edge) const
    {
        for (int i = 0, iPolygonCount(GetPolygonCount()); i < iPolygonCount; ++i)
        {
            PolygonPtr pPolygon = GetPolygon(i);
            if (pPolygon->HasEdge(edge))
            {
                return true;
            }
        }
        return false;
    }

    bool Model::RemovePolygon(int nPolygonIndex)
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);

        if (nPolygonIndex < 0 || nPolygonIndex >= m_Polygons[m_ShelfID].size())
        {
            return false;
        }

        RemovePolygon(m_Polygons[m_ShelfID].begin() + nPolygonIndex);
        InvalidateAABB(m_ShelfID);

        return true;
    }

    bool Model::RemovePolygon(PolygonPtr pPolygon)
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);

        for (int i = 0, iPolygonSize(m_Polygons[m_ShelfID].size()); i < iPolygonSize; ++i)
        {
            if (m_Polygons[m_ShelfID][i] == pPolygon)
            {
                return RemovePolygon(i);
            }
        }

        return false;
    }

    PolygonList::iterator Model::RemovePolygon(const PolygonList::iterator& iPolygon)
    {
        if (*iPolygon)
        {
            GetSmoothingGroupMgr()->RemovePolygon(*iPolygon);
        }
        std::vector<PolygonPtr>::iterator iNext = m_Polygons[m_ShelfID].erase(iPolygon);
        return iNext;
    }

    void Model::RemovePolygonsWithSpecificFlagsPlane(int nFlags, const BrushPlane* pPlane)
    {
        std::vector<PolygonPtr>::iterator ii = m_Polygons[m_ShelfID].begin();
        for (; ii != m_Polygons[m_ShelfID].end(); )
        {
            if (pPlane && !pPlane->IsEquivalent((*ii)->GetPlane()))
            {
                ++ii;
                continue;
            }

            if ((*ii)->CheckFlags(nFlags))
            {
                ii = RemovePolygon(ii);
            }
            else
            {
                ++ii;
            }
        }
        InvalidateAABB(m_ShelfID);
    }

    PolygonPtr Model::QueryPolygon(REFGUID guid) const
    {
        for (int i = 0; i < kMaxShelfCount; ++i)
        {
            for (int k = 0, iPolygonCount(m_Polygons[i].size()); k < iPolygonCount; ++k)
            {
                if (m_Polygons[i][k]->GetGUID() == guid)
                {
                    return m_Polygons[i][k];
                }
            }
        }
        return NULL;
    }

    PolygonPtr Model::GetPolygonPtr(int nIndex) const
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);

        if (nIndex >= m_Polygons[m_ShelfID].size() || nIndex < 0)
        {
            return NULL;
        }
        return m_Polygons[m_ShelfID][nIndex];
    }

    bool Model::QueryPolygon(const BrushPlane& plane, const BrushVec3& raySrc, const BrushVec3& rayDir, int& nOutIndex) const
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);

        BrushPlane invPlane = plane.GetInverted();

        BrushFloat fShortestDist(1000000.0f);
        nOutIndex = -1;
        for (int i = 0, polygonSize(m_Polygons[m_ShelfID].size()); i < polygonSize; ++i)
        {
            PolygonPtr polygon = m_Polygons[m_ShelfID][i];
            BrushFloat t = 0;
            if (!polygon->GetPlane().IsEquivalent(plane) && !polygon->GetPlane().IsEquivalent(invPlane))
            {
                continue;
            }
            if (polygon->CheckFlags(ePolyFlag_Hidden))
            {
                continue;
            }
            if (polygon->IsPassed(raySrc, rayDir, t))
            {
                if (t < fShortestDist || std::abs(t - fShortestDist) < kDesignerEpsilon && polygon->IsOpen())
                {
                    fShortestDist = t;
                    nOutIndex = i;
                }
            }
        }
        return nOutIndex != -1;
    }

    bool Model::QueryPolygons(const BrushPlane& plane, std::vector<PolygonPtr>& outPolygons) const
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);
        bool bAdded = false;
        for (int i = 0, iPolygonCount(m_Polygons[m_ShelfID].size()); i < iPolygonCount; ++i)
        {
            if (m_Polygons[m_ShelfID][i]->GetPlane().IsEquivalent(plane))
            {
                bAdded = true;
                outPolygons.push_back(m_Polygons[m_ShelfID][i]);
            }
        }
        return bAdded;
    }

    bool Model::QueryIntersectedPolygonsByAABB(const AABB& aabb, PolygonList& outPolygons) const
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);
        bool bAdded = false;
        for (int i = 0, iPolygonCount(m_Polygons[m_ShelfID].size()); i < iPolygonCount; ++i)
        {
            if (m_Polygons[m_ShelfID][i]->IntersectedBetweenAABBs(aabb))
            {
                bAdded = true;
                outPolygons.push_back(m_Polygons[m_ShelfID][i]);
            }
        }
        return bAdded;
    }

    bool Model::QueryPolygon(const BrushVec3& raySrc, const BrushVec3& rayDir, int& nOutIndex) const
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);

        BrushFloat fShortestDist(1000000.0f);
        nOutIndex = -1;
        for (int i = 0, polygonSize(m_Polygons[m_ShelfID].size()); i < polygonSize; ++i)
        {
            PolygonPtr pPolygon = m_Polygons[m_ShelfID][i];

            if (!pPolygon->IsValid() || pPolygon->CheckFlags(ePolyFlag_Hidden))
            {
                continue;
            }

            BrushFloat t = 0;
            if (pPolygon->IsPassed(raySrc, rayDir, t))
            {
                BrushFloat fDistanceFromPlane = pPolygon->GetPlane().Distance(raySrc);
                BrushFloat fAbsShortestDistance = std::abs(fShortestDist);
                if (std::abs(t - fAbsShortestDistance) < kDesignerEpsilon && fDistanceFromPlane >= 0 || t < fAbsShortestDistance)
                {
                    fShortestDist = t;
                    if (fDistanceFromPlane < 0)
                    {
                        fShortestDist = -fShortestDist;
                    }
                    nOutIndex = i;
                }
            }
        }
        return nOutIndex != -1;
    }

    bool Model::QueryCenterOfPolygon(const BrushVec3& raySrc, const BrushVec3& rayDir, BrushVec3& outCenterOfPos) const
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);

        int nPolygonIndex(0);
        if (!QueryPolygon(raySrc, rayDir, nPolygonIndex))
        {
            return false;
        }

        outCenterOfPos = m_Polygons[m_ShelfID][nPolygonIndex]->GetBoundBox().GetCenter();
        return true;
    }

    bool Model::QueryNearestPosFromBoundary(const BrushVec3& pos, BrushVec3& outNearestPos) const
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);

        BrushFloat fNearestDistance(kEnoughBigNumber);
        BrushVec3 NearestPos;
        for (int i = 0, iPolygonSize(m_Polygons[m_ShelfID].size()); i < iPolygonSize; ++i)
        {
            PolygonPtr polygon(m_Polygons[m_ShelfID][i]);
            BrushVec3 NearestPosOnWorld;
            if (!m_Polygons[m_ShelfID][i]->QueryNearestPosFromBoundary(pos, NearestPosOnWorld))
            {
                continue;
            }
            BrushFloat fSqDistance((NearestPosOnWorld - pos).GetLengthSquared());
            if (fSqDistance < fNearestDistance)
            {
                fNearestDistance = fSqDistance;
                outNearestPos = NearestPosOnWorld;
            }
        }
        return fNearestDistance < kEnoughBigNumber;
    }

    ESurroundType Model::QuerySurroundType(int nPolygonIndex) const
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);

        if (nPolygonIndex < 0 || nPolygonIndex >= m_Polygons[m_ShelfID].size())
        {
            return eSurroundType_WrongInput;
        }

        PolygonPtr pUnionPolygon = new CD::Polygon;

        for (int i = 0, iPolygonSize(m_Polygons[m_ShelfID].size()); i < iPolygonSize; ++i)
        {
            if (i == nPolygonIndex)
            {
                continue;
            }
            if (!m_Polygons[m_ShelfID][nPolygonIndex]->IsPlaneEquivalent(m_Polygons[m_ShelfID][i]))
            {
                continue;
            }
            if (Polygon::HasIntersection(m_Polygons[m_ShelfID][nPolygonIndex], m_Polygons[m_ShelfID][i]) != eIT_JustTouch)
            {
                continue;
            }
            pUnionPolygon->Union(m_Polygons[m_ShelfID][i]);
        }

        if (!pUnionPolygon->IsValid())
        {
            return eSurroundType_None;
        }

        pUnionPolygon->RemoveInside();

        if (pUnionPolygon->IncludeAllEdges(m_Polygons[m_ShelfID][nPolygonIndex]))
        {
            return eSurroundType_Surrounded;
        }

        PolygonPtr pInputPolygon = m_Polygons[m_ShelfID][nPolygonIndex]->Clone();
        pInputPolygon->RemoveInside();
        if (pInputPolygon->IncludeAllEdges(pUnionPolygon))
        {
            return eSurroundType_Surrounding;
        }

        return eSurroundType_Partly;
    }

    bool Model::QueryNearestEdges(const BrushPlane& plane, const BrushVec3& raySrc, const BrushVec3& rayDir, BrushVec3& outPos, BrushVec3& outPosOnEdge, std::vector<SQueryEdgeResult>& outEdges) const
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);

        float fShorestDist = 3e10f;
        BrushVec3 posOnEdge;

        for (int i = 0, iPolygonSize(m_Polygons[m_ShelfID].size()); i < iPolygonSize; ++i)
        {
            const PolygonPtr pPolygon(m_Polygons[m_ShelfID][i]);
            if (pPolygon == NULL)
            {
                continue;
            }
            if (!pPolygon->GetPlane().IsEquivalent(plane))
            {
                continue;
            }
            BrushPlane p;
            BrushEdge3D edge;
            if (!QueryNearestEdge(i, raySrc, rayDir, outPos, posOnEdge, p, edge))
            {
                continue;
            }
            BrushFloat fDistance = outPos.GetDistance(posOnEdge);
            if (std::abs(fDistance - fShorestDist) < kDesignerEpsilon)
            {
                outEdges.push_back(SQueryEdgeResult(pPolygon, edge));
            }
            else if (fDistance < fShorestDist)
            {
                fShorestDist = fDistance;
                outPosOnEdge = posOnEdge;
                outEdges.clear();
                outEdges.push_back(SQueryEdgeResult(pPolygon, edge));
            }
        }
        return fShorestDist < 3e9f;
    }

    bool Model::QueryNearestEdges(const BrushPlane& plane, const BrushVec3& position, BrushVec3& outPosOnEdge, std::vector<SQueryEdgeResult>& outEdges) const
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);

        float fShorestDist = 3e10f;
        BrushVec3 posOnEdge;

        for (int i = 0, iPolygonSize(m_Polygons[m_ShelfID].size()); i < iPolygonSize; ++i)
        {
            const PolygonPtr pPolygon(m_Polygons[m_ShelfID][i]);
            if (pPolygon == NULL)
            {
                continue;
            }
            if (!pPolygon->GetPlane().IsEquivalent(plane))
            {
                continue;
            }
            BrushPlane p;
            BrushEdge3D edge;
            if (!QueryNearestEdge(i, position, posOnEdge, p, edge))
            {
                continue;
            }
            BrushFloat fDistance = position.GetDistance(posOnEdge);
            if (std::abs(fDistance - fShorestDist) < kDesignerEpsilon)
            {
                outEdges.push_back(SQueryEdgeResult(pPolygon, edge));
            }
            else if (fDistance < fShorestDist)
            {
                fShorestDist = fDistance;
                outPosOnEdge = posOnEdge;
                outEdges.clear();
                outEdges.push_back(SQueryEdgeResult(pPolygon, edge));
            }
        }
        return fShorestDist < 3e9f;
    }

    bool Model::QueryNearestEdges(const BrushVec3& raySrc, const BrushVec3& rayDir, BrushVec3& outPos, BrushVec3& outPosOnEdge, BrushPlane& outPlane, std::vector<SQueryEdgeResult>& outEdges) const
    {
        int nPolygonIndex(-1);
        if (QueryPolygon(raySrc, rayDir, nPolygonIndex) == false)
        {
            return false;
        }
        outPlane = GetPolygon(nPolygonIndex)->GetPlane();
        return QueryNearestEdges(outPlane, raySrc, rayDir, outPos, outPosOnEdge, outEdges);
    }

    bool Model::QueryNearestEdge(int nPolygonIndex, const BrushVec3& raySrc, const BrushVec3& rayDir, BrushVec3& outPos, BrushVec3& outPosOnEdge, BrushPlane& outPlane, BrushEdge3D& outEdge) const
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);

        PolygonPtr pPolygon(m_Polygons[m_ShelfID][nPolygonIndex]);
        outPlane = pPolygon->GetPlane();

        if (!outPlane.HitTest(raySrc, raySrc + rayDir, NULL, &outPos))
        {
            return false;
        }

        BrushVec3 posOnEdge;
        BrushEdge3D edge;
        if (pPolygon->QueryNearestEdge(outPos, edge, posOnEdge) == false)
        {
            return false;
        }

        outEdge = edge;
        outPosOnEdge = posOnEdge;

        return true;
    }

    bool Model::QueryNearestEdge(int nPolygonIndex, const BrushVec3& position, BrushVec3& outPosOnEdge, BrushPlane& outPlane, BrushEdge3D& outEdge) const
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);

        PolygonPtr pPolygon(m_Polygons[m_ShelfID][nPolygonIndex]);
        outPlane = pPolygon->GetPlane();

        BrushVec3 posOnEdge;
        BrushEdge3D edge;
        if (pPolygon->QueryNearestEdge(position, edge, posOnEdge) == false)
        {
            return false;
        }

        outEdge = edge;
        outPosOnEdge = posOnEdge;

        return true;
    }

    void Model::QueryAdjacentPerpendicularPolygons(PolygonPtr pPolygon, std::vector<PolygonPtr>& outPolygons) const
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);

        if (!pPolygon)
        {
            return;
        }

        for (int i = 0, iPolygonCount(m_Polygons[m_ShelfID].size()); i < iPolygonCount; ++i)
        {
            PolygonPtr pCandidatePolygon = m_Polygons[m_ShelfID][i];
            if (std::abs(pPolygon->GetPlane().Normal().Dot(pCandidatePolygon->GetPlane().Normal())) > kDesignerEpsilon)
            {
                continue;
            }
            for (int a = 0, iCandidateEdgeCount(pCandidatePolygon->GetEdgeCount()); a < iCandidateEdgeCount; ++a)
            {
                BrushEdge3D candidateEdge3D = pCandidatePolygon->GetEdge(a);
                if (pPolygon->IsEdgeOnCrust(candidateEdge3D))
                {
                    outPolygons.push_back(pCandidatePolygon);
                    break;
                }
            }
        }
    }

    void Model::QueryPerpendicularPolygons(PolygonPtr pPolygon, std::vector<PolygonPtr>& outPolygons) const
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);

        if (!pPolygon)
        {
            return;
        }

        AABB aabb = pPolygon->GetBoundBox();
        aabb.Expand(Vec3(0.01f, 0.01f, 0.01f));

        for (int i = 0, iPolygonCount(m_Polygons[m_ShelfID].size()); i < iPolygonCount; ++i)
        {
            PolygonPtr pCandidatePolygon = m_Polygons[m_ShelfID][i];
            if (pCandidatePolygon == pPolygon || pCandidatePolygon->IsOpen())
            {
                continue;
            }

            BrushFloat normalDot = std::abs(pPolygon->GetPlane().Normal().Dot(pCandidatePolygon->GetPlane().Normal()));
            if (normalDot > kDesignerEpsilon)
            {
                continue;
            }

            for (int a = 0, iVertexCount(pPolygon->GetVertexCount()); a < iVertexCount; ++a)
            {
                const BrushVec3& v = pPolygon->GetPos(a);
                BrushFloat fAbsDist = std::abs(pCandidatePolygon->GetPlane().Distance(v));
                if (fAbsDist < kDistanceLimitation)
                {
                    outPolygons.push_back(pCandidatePolygon);
                    break;
                }
            }
        }
    }

    void Model::Clear()
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);

        PolygonList::iterator ii = m_Polygons[m_ShelfID].begin();
        for (; ii != m_Polygons[m_ShelfID].end(); )
        {
            ii = RemovePolygon(ii);
        }

        InvalidateAABB();
    }

    void Model::Display(DisplayContext& dc, const int nLineThickness, const ColorB& lineColor)
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);
        int numOfPolygons(m_Polygons[m_ShelfID].size());
        DisplayPolygons(dc, nLineThickness, lineColor);
        DisplaySubdividedMesh(dc);
    }

    void Model::DisplayPolygons(DisplayContext& dc, const int nLineThickness, const ColorB& lineColor)
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);
        MODEL_SHELF_RECONSTRUCTOR(this);

        int oldThickness = dc.GetLineWidth();
        dc.SetLineWidth(nLineThickness);
        dc.SetColor(lineColor);

        for (ShelfID shelfID = 0; shelfID < kMaxShelfCount; ++shelfID)
        {
            SetShelf(shelfID);

            int iPolygonSize(m_Polygons[shelfID].size());

            for (int i = 0; i < iPolygonSize; ++i)
            {
                PolygonPtr pPolygon = m_Polygons[m_ShelfID][i];

                if (!pPolygon->IsValid() || pPolygon->CheckFlags(ePolyFlag_Hidden | ePolyFlag_Mirrored))
                {
                    continue;
                }

                for (int k = 0, iEdgeSize(pPolygon->GetEdgeCount()); k < iEdgeSize; ++k)
                {
                    const SEdge& rEdge = pPolygon->GetEdgeIndexPair(k);
                    BrushEdge3D edge(pPolygon->GetPos(rEdge.m_i[0]), pPolygon->GetPos(rEdge.m_i[1]));
                    std::vector<BrushEdge3D> visibleParts;
                    if (!GetVisibleEdge(edge, pPolygon->GetPlane(), visibleParts))
                    {
                        continue;
                    }
                    if (!visibleParts.empty())
                    {
                        for (int i = 0, iVisibleSize(visibleParts.size()); i < iVisibleSize; ++i)
                        {
                            dc.DrawLine(visibleParts[i].m_v[0], visibleParts[i].m_v[1]);
                        }
                    }
                    else
                    {
                        dc.DrawLine(edge.m_v[0], edge.m_v[1]);
                    }
                }
            }
        }
        dc.SetLineWidth(oldThickness);
    }

    void Model::DisplaySubdividedMesh(DisplayContext& dc)
    {
        if (!m_pSubdividionResult || gDesignerSettings.bDisplayTriangulation || !gDesignerSettings.bDisplaySubdividedResult)
        {
            return;
        }

        dc.SetColor(ColorB(150, 150, 150));
        dc.SetLineWidth(2);

        for (int i = 0, iFaceCount(m_pSubdividionResult->GetFaceCount()); i < iFaceCount; ++i)
        {
            const HE_Face& f = m_pSubdividionResult->GetFace(i);
            std::vector<BrushVec3> vertices;
            m_pSubdividionResult->GetFaceVertices(f, vertices);
            for (int k = 0, iVertexCount(vertices.size()); k < iVertexCount; ++k)
            {
                const BrushVec3& v0 = vertices[k];
                const BrushVec3& v1 = vertices[(k + 1) % iVertexCount];

                dc.DrawLine(ToVec3(v0), ToVec3(v1));
            }
        }
    }

    bool Model::GetVisibleEdge(const BrushEdge3D& edge, const BrushPlane& plane, std::vector<BrushEdge3D>& outVisibleEdges) const
    {
        BrushLine line(plane.W2P(edge.m_v[0]), plane.W2P(edge.m_v[1]));
        BrushLine invLine(line.GetInverted());

        std::vector<int> indicesOnCoLine;

        for (int i = 0, iSize(m_ExcludedEdgesInDrawing.size()); i < iSize; ++i)
        {
            BrushLine lineFromRejected(plane.W2P(m_ExcludedEdgesInDrawing[i].m_v[0]), plane.W2P(m_ExcludedEdgesInDrawing[i].m_v[1]));
            if (!line.IsEquivalent(lineFromRejected) && !invLine.IsEquivalent(lineFromRejected))
            {
                continue;
            }

            bool bEquivalent = edge.IsEquivalent(m_ExcludedEdgesInDrawing[i]);
            bool bInverseEquivalent = !bEquivalent ? IsEquivalent(edge.m_v[0], m_ExcludedEdgesInDrawing[i].m_v[1]) && IsEquivalent(edge.m_v[1], m_ExcludedEdgesInDrawing[i].m_v[0]) : true;
            bool bInside = !bInverseEquivalent ? m_ExcludedEdgesInDrawing[i].ContainVertex(edge.m_v[0]) && m_ExcludedEdgesInDrawing[i].ContainVertex(edge.m_v[1]) : true;
            if (bEquivalent || bInverseEquivalent || bInside)
            {
                return false;
            }

            indicesOnCoLine.push_back(i);
        }

        for (int i = 0, iSize(indicesOnCoLine.size()); i < iSize; ++i)
        {
            if (edge.GetSubtractedEdges(m_ExcludedEdgesInDrawing[indicesOnCoLine[i]], outVisibleEdges))
            {
                break;
            }
        }

        return true;
    }

    void Model::DeletePolygons(std::set<PolygonPtr>& deletedPolygons)
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);

        if (!deletedPolygons.empty())
        {
            std::vector<PolygonPtr>::iterator ii = m_Polygons[m_ShelfID].begin();
            for (; ii != m_Polygons[m_ShelfID].end(); )
            {
                if (deletedPolygons.find(*ii) != deletedPolygons.end())
                {
                    ii = RemovePolygon(ii);
                }
                else
                {
                    ++ii;
                }
            }
            InvalidateAABB(m_ShelfID);
        }
    }

    bool Model::GetPolygonPlane(int nPolygonIndex, BrushPlane& outPlane) const
    {
        PolygonPtr pPolygon(GetPolygonPtr(nPolygonIndex));
        if (pPolygon == NULL)
        {
            return false;
        }

        outPlane = pPolygon->GetPlane();
        return true;
    }

    void Model::Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bUndo)
    {
        if (bLoading)
        {
            int childNum = xmlNode->getChildCount();

            m_Polygons[0].clear();
            m_Polygons[1].clear();
            m_Polygons[0].reserve(childNum);

            for (int i = 0; i < childNum; ++i)
            {
                XmlNodeRef pChildNode = xmlNode->getChild(i);
                DESIGNER_ASSERT(pChildNode);
                if (!pChildNode)
                {
                    continue;
                }
                if (!strcmp(pChildNode->getTag(), "Polygon") || !strcmp(pChildNode->getTag(), "Region"))
                {
                    unsigned int nPolygonFlag = 0;
                    pChildNode->getAttr("Flags", nPolygonFlag);
                    static const int kBackFaceFlag = BIT(2);
                    if (!(nPolygonFlag & kBackFaceFlag))
                    {
                        PolygonPtr pPolygon = new CD::Polygon;
                        pPolygon->Serialize(pChildNode, bLoading, bUndo);
                        AddPolygon(0, pPolygon);
                    }
                }
                else if (!strcmp(pChildNode->getTag(), "SmoothingGroups"))
                {
                    GetSmoothingGroupMgr()->Serialize(pChildNode, bLoading, bUndo, this);
                }
                else if (!strcmp(pChildNode->getTag(), "SemiSharpCrease"))
                {
                    GetEdgeSharpnessMgr()->Serialize(pChildNode, bLoading, bUndo, this);
                }
            }

            xmlNode->getAttr("DesignerModeFlags", m_nModeFlag);
            xmlNode->getAttr("SubdivisionLevel", m_SubdivisionLevel);
            xmlNode->getAttr("SubdivisionSmoothingGroup", m_bSmoothingGroupForSubdividedSurfaces);
            xmlNode->getAttr("TessFactor", m_nTessFactor);

            BrushVec3 mirrorPlaneNormal;
            BrushFloat mirrorPlaneDistance;
            if (xmlNode->getAttr("MirrorPlaneNormal", mirrorPlaneNormal) && xmlNode->getAttr("MirrorPlaneDistance", mirrorPlaneDistance))
            {
                m_MirrorPlane = BrushPlane(mirrorPlaneNormal, mirrorPlaneDistance);
            }

            InvalidateAABB();
        }
        else
        {
            for (int i = 0, iSize(m_Polygons[0].size()); i < iSize; ++i)
            {
                if (m_Polygons[0][i])
                {
                    XmlNodeRef polygonNode(xmlNode->newChild("Polygon"));
                    m_Polygons[0][i]->Serialize(polygonNode, bLoading, bUndo);
                }
                else
                {
                    DESIGNER_ASSERT(0);
                }
            }

            XmlNodeRef smoothingGroupsNode(xmlNode->newChild("SmoothingGroups"));
            GetSmoothingGroupMgr()->Serialize(smoothingGroupsNode, bLoading, bUndo, this);

            XmlNodeRef semiSharpCreaseNode(xmlNode->newChild("SemiSharpCrease"));
            GetEdgeSharpnessMgr()->Serialize(semiSharpCreaseNode, bLoading, bUndo, this);

            xmlNode->setAttr("DesignerModeFlags", m_nModeFlag);
            xmlNode->setAttr("SubdivisionLevel", m_SubdivisionLevel);
            xmlNode->setAttr("SubdivisionSmoothingGroup", m_bSmoothingGroupForSubdividedSurfaces);
            xmlNode->setAttr("TessFactor", m_nTessFactor);
            xmlNode->setAttr("MirrorPlaneNormal", m_MirrorPlane.Normal());
            xmlNode->setAttr("MirrorPlaneDistance", m_MirrorPlane.Distance());
        }
    }

    PolygonPtr Model::GetPolygon(int nPolygonIndex) const
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);

        if (nPolygonIndex < 0 || nPolygonIndex >= m_Polygons[m_ShelfID].size())
        {
            return NULL;
        }
        return m_Polygons[m_ShelfID][nPolygonIndex];
    }

    int Model::GetPolygonIndex(PolygonPtr pPolygon) const
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);

        for (int i = 0, iPolygonCount(m_Polygons[m_ShelfID].size()); i < iPolygonCount; ++i)
        {
            if (m_Polygons[m_ShelfID][i] == pPolygon)
            {
                return i;
            }
        }

        return -1;
    }

    void Model::GetPolygons(const BrushPlane& plane, std::vector<int>& outPolygonIndices) const
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);

        for (int i = 0, nPolygonSize(m_Polygons[m_ShelfID].size()); i < nPolygonSize; ++i)
        {
            if (plane.IsEquivalent(m_Polygons[m_ShelfID][i]->GetPlane()))
            {
                outPolygonIndices.push_back(i);
            }
        }
    }

    EPolygonRelation Model::QueryOppositePolygon(PolygonPtr pPolygon, EFindOppositeFlag nFlag, BrushFloat fScale, PolygonPtr& outPolygon, BrushFloat& outDistance) const
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);

        if (!pPolygon)
        {
            return eER_None;
        }

        BrushPlane plane(pPolygon->GetPlane());
        BrushPlane invertedPlane(plane);
        invertedPlane.Invert();

        BrushVec3 centerOfPolygon(pPolygon->GetCenterPosition());

        BrushFloat nearestDistance(3e10f);
        PolygonPtr pNearestPolygon;

        for (int i = 0, nPolygonSize(m_Polygons[m_ShelfID].size()); i < nPolygonSize; ++i)
        {
            const BrushPlane& oppositePlane(m_Polygons[m_ShelfID][i]->GetPlane());

            if (oppositePlane.IsEquivalent(plane) || oppositePlane.IsEquivalent(invertedPlane))
            {
                continue;
            }

            if (!m_Polygons[m_ShelfID][i]->IsValid() || m_Polygons[m_ShelfID][i]->IsOpen())
            {
                continue;
            }

            BrushVec3 v(m_Polygons[m_ShelfID][i]->GetPos(0));
            BrushFloat dist(0);
            if (!plane.HitTest(v, v + oppositePlane.Normal(), &dist))
            {
                continue;
            }

            if (std::abs(fScale) < kDesignerEpsilon)
            {
                if (oppositePlane.Normal().Dot(plane.Normal()) > -kDesignerEpsilon)
                {
                    continue;
                }

                if (nFlag == eFOF_PushDirection)
                {
                    if (dist > 0)
                    {
                        continue;
                    }
                }
                else
                {
                    if (dist < 0)
                    {
                        continue;
                    }
                }
            }
            else
            {
                if (oppositePlane.Normal().Dot(plane.Normal()) < 1 - kDesignerEpsilon)
                {
                    continue;
                }

                if (nFlag == eFOF_PushDirection)
                {
                    if (dist < 0)
                    {
                        continue;
                    }
                }
                else
                {
                    if (dist > 0)
                    {
                        continue;
                    }
                }
            }

            PolygonPtr pIntersectionPolygon = m_Polygons[m_ShelfID][i]->Clone();

            if (!pIntersectionPolygon->UpdatePlane(pPolygon->GetPlane()))
            {
                continue;
            }

            if (std::abs(fScale) > kDesignerEpsilon)
            {
                if (!pIntersectionPolygon->Scale(-fScale))
                {
                    continue;
                }
            }

            pIntersectionPolygon->Intersect(pPolygon, eICEII_IncludeCoSame);
            if (pIntersectionPolygon->IsValid() == false)
            {
                continue;
            }

            BrushVec3 direction = (nFlag == eFOF_PushDirection) ? pPolygon->GetPlane().Normal() : -pPolygon->GetPlane().Normal();
            BrushFloat distance(0);
            if (!pIntersectionPolygon->UpdatePlane(m_Polygons[m_ShelfID][i]->GetPlane(), direction))
            {
                continue;
            }
            distance = pPolygon->GetNearestDistance(pIntersectionPolygon, direction);

            if (distance >= 0 && distance < nearestDistance)
            {
                nearestDistance = distance;
                pNearestPolygon = pIntersectionPolygon;
            }
        }

        if (nearestDistance == 3e10f)
        {
            outPolygon = NULL;
            return eER_None;
        }

        outDistance = nearestDistance;
        outPolygon = pNearestPolygon;

        if (std::abs(outDistance) < kDesignerEpsilon * 10.0f)
        {
            return eER_ZeroDistance;
        }

        if (std::abs(plane.Normal().Dot(outPolygon->GetPlane().Normal())) < 1 - kDesignerEpsilon)
        {
            outDistance = nearestDistance - 0.01f;
        }

        return eER_Intersection;
    }

    void Model::QueryIntersectionByPolygon(PolygonPtr pPolygon, std::vector<PolygonPtr>& outIntersetionPolygons) const
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);
        if (!pPolygon)
        {
            return;
        }

        AABB bb = pPolygon->GetBoundBox();
        bb.Expand(Vec3(0.01f, 0.01f, 0.01f));

        for (int i = 0, iPolygonSize(m_Polygons[m_ShelfID].size()); i < iPolygonSize; ++i)
        {
            if (m_Polygons[m_ShelfID][i]->CheckFlags(ePolyFlag_Mirrored))
            {
                continue;
            }
            if (!m_Polygons[m_ShelfID][i]->GetBoundBox().IsIntersectBox(bb))
            {
                continue;
            }
            if (!m_Polygons[m_ShelfID][i]->GetPlane().IsEquivalent(pPolygon->GetPlane()))
            {
                continue;
            }
            if (Polygon::HasIntersection(m_Polygons[m_ShelfID][i], pPolygon) == eIT_Intersection)
            {
                outIntersetionPolygons.push_back(m_Polygons[m_ShelfID][i]);
            }
        }
    }

    void Model::QueryIntersectionPolygonsWith2DRect(IDisplayViewport* pView, const BrushMatrix34& worldTM, PolygonPtr pRectPolygon, bool bExcludeBackFace, PolygonList& outIntersectionPolygons) const
    {
        for (int i = 0, iPolygonCount(m_Polygons[m_ShelfID].size()); i < iPolygonCount; ++i)
        {
            PolygonPtr pPolygon = m_Polygons[m_ShelfID][i];
            if (pPolygon->CheckFlags(ePolyFlag_Mirrored | ePolyFlag_Hidden))
            {
                continue;
            }
            if (pPolygon->InRectangle(pView, worldTM, pRectPolygon, bExcludeBackFace))
            {
                outIntersectionPolygons.push_back(pPolygon);
            }
        }
    }

    void Model::QueryIntersectionEdgesWith2DRect(IDisplayViewport* pView, const BrushMatrix34& worldTM, PolygonPtr pRectPolygon, bool bExcludeBackFace, EdgeQueryResult& outIntersectionEdges) const
    {
        for (int i = 0, iPolygonCount(m_Polygons[m_ShelfID].size()); i < iPolygonCount; ++i)
        {
            PolygonPtr pPolygon = m_Polygons[m_ShelfID][i];
            if (pPolygon->CheckFlags(ePolyFlag_Mirrored | ePolyFlag_Hidden))
            {
                continue;
            }
            pPolygon->QueryIntersectionEdgesWith2DRect(pView, worldTM, pRectPolygon, bExcludeBackFace, outIntersectionEdges);
        }
    }

    void Model::QueryIntersectionByEdge(const BrushEdge3D& edge, std::vector<IntersectionPair>& outIntersections) const
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);
        std::map<BrushFloat, IntersectionPair> sortedIntersections;

        for (int i = 0, iPolygonSize(m_Polygons[m_ShelfID].size()); i < iPolygonSize; ++i)
        {
            if (m_Polygons[m_ShelfID][i]->CheckFlags(ePolyFlag_Mirrored))
            {
                continue;
            }
            std::map<BrushFloat, BrushVec3> polygonIntersections;
            m_Polygons[m_ShelfID][i]->QueryIntersections(edge, polygonIntersections);

            std::map<BrushFloat, BrushVec3>::iterator ii = polygonIntersections.begin();
            for (; ii != polygonIntersections.end(); ++ii)
            {
                std::map<BrushFloat, IntersectionPair>::iterator iSorted = sortedIntersections.begin();
                bool bIdenticalExist = false;
                for (; iSorted != sortedIntersections.end(); ++iSorted)
                {
                    const IntersectionPair& intersection = iSorted->second;
                    if (IsEquivalent(intersection.second, ii->second))
                    {
                        bIdenticalExist = true;
                        break;
                    }
                }
                if (!bIdenticalExist)
                {
                    sortedIntersections[ii->first] = IntersectionPair(m_Polygons[m_ShelfID][i], ii->second);
                }
            }
        }

        std::map<BrushFloat, IntersectionPair>::iterator iter = sortedIntersections.begin();
        for (; iter != sortedIntersections.end(); ++iter)
        {
            outIntersections.push_back(iter->second);
        }
    }

    void Model::Optimize()
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);

        std::vector<PolygonPtr>::iterator ii = m_Polygons[m_ShelfID].begin();
        for (; ii != m_Polygons[m_ShelfID].end(); )
        {
            if (!(*ii)->IsValid())
            {
                ii = RemovePolygon(ii);
            }
            else
            {
                ++ii;
            }
        }

        std::vector<PolygonPtr> polygons = m_Polygons[m_ShelfID];
        for (int i = 0, iPolygonSize(polygons.size()); i < iPolygonSize; ++i)
        {
            if (polygons[i]->IsOpen())
            {
                continue;
            }
            AddPolygonSeparately(polygons[i], true);
        }
    }

    void Model::SeparatePolygons(const BrushPlane& plane)
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);

        std::vector<PolygonPtr> polygons = m_Polygons[m_ShelfID];
        for (int i = 0, iPolygonSize(polygons.size()); i < iPolygonSize; ++i)
        {
            if (polygons[i]->IsOpen())
            {
                continue;
            }
            if (!plane.IsEquivalent(polygons[i]->GetPlane()))
            {
                continue;
            }
            AddPolygonSeparately(polygons[i], true);
        }
    }

    void Model::Move(const BrushVec3& offset)
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);

        for (int i = 0, iPolygonSize(m_Polygons[m_ShelfID].size()); i < iPolygonSize; ++i)
        {
            m_Polygons[m_ShelfID][i]->Move(offset);
            GetSmoothingGroupMgr()->InvalidateSmoothingGroup(m_Polygons[m_ShelfID][i]);
        }

        InvalidateAABB(m_ShelfID);
    }

    void Model::Transform(const BrushMatrix34& tm)
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);

        for (int i = 0, iPolygonSize(m_Polygons[m_ShelfID].size()); i < iPolygonSize; ++i)
        {
            m_Polygons[m_ShelfID][i]->Transform(tm);
            GetSmoothingGroupMgr()->InvalidateSmoothingGroup(m_Polygons[m_ShelfID][i]);
        }

        InvalidateAABB(m_ShelfID);
    }

    bool Model::IsVertexOnEdge(const BrushPlane& plane, const BrushVec3& vertex, PolygonPtr pExcludedPolygon) const
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);

        for (int i = 0, iPolygonSize(m_Polygons[m_ShelfID].size()); i < iPolygonSize; ++i)
        {
            PolygonPtr pPolygon = m_Polygons[m_ShelfID][i];
            if (pPolygon == NULL)
            {
                continue;
            }
            if (pExcludedPolygon && pExcludedPolygon == pPolygon)
            {
                continue;
            }
            const BrushPlane& polygonPlane = pPolygon->GetPlane();
            if (!plane.IsEquivalent(polygonPlane))
            {
                continue;
            }
            BrushEdge3D nearestEdge;
            BrushVec3 hitPos;
            if (!pPolygon->QueryNearestEdge(vertex, nearestEdge, hitPos))
            {
                continue;
            }
            if ((vertex - hitPos).GetLength() < kDesignerEpsilon)
            {
                return true;
            }
        }
        return false;
    }

    void Model::GetPolygonList(std::vector<PolygonPtr>& outExportedPolygonList) const
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);
        for (int i = 0, iPolygonSize(m_Polygons[m_ShelfID].size()); i < iPolygonSize; ++i)
        {
            if (m_Polygons[m_ShelfID][i]->IsOpen())
            {
                continue;
            }
            outExportedPolygonList.push_back(m_Polygons[m_ShelfID][i]);
        }
    }

    void Model::MoveShelf(ShelfID sourceShelfID, ShelfID destShelfID)
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);

        MODEL_SHELF_RECONSTRUCTOR(this);

        for (int i = 0, iPolygonCount(m_Polygons[sourceShelfID].size()); i < iPolygonCount; ++i)
        {
            m_Polygons[destShelfID].push_back(m_Polygons[sourceShelfID][i]);
        }

        SetShelf(sourceShelfID);
        Clear();
        InvalidateAABB();
    }

    bool Model::SplitPolygonsByOpenPolygon(PolygonPtr pOpenPolygon)
    {
        DESIGNER_ASSERT(m_ShelfID >= 0 && m_ShelfID < kMaxShelfCount);

        std::set<PolygonPtr> spannedPolygons;
        for (int i = 0, iSize(m_Polygons[m_ShelfID].size()); i < iSize; ++i)
        {
            if (m_Polygons[m_ShelfID][i]->IsOpen())
            {
                continue;
            }
            EIntersectionType intersectionType = Polygon::HasIntersection(m_Polygons[m_ShelfID][i], pOpenPolygon);
            if (intersectionType == eIT_Intersection)
            {
                spannedPolygons.insert(m_Polygons[m_ShelfID][i]);
            }
        }

        if (spannedPolygons.empty())
        {
            AddPolygon(pOpenPolygon->Clone());
            return true;
        }

        std::vector<PolygonPtr>::iterator ii;
        for (ii = m_Polygons[m_ShelfID].begin(); ii != m_Polygons[m_ShelfID].end(); )
        {
            if (spannedPolygons.find(*ii) != spannedPolygons.end())
            {
                ii = RemovePolygon(ii);
            }
            else
            {
                ++ii;
            }
        }

        PolygonPtr pRearrangedOpenPolygon(pOpenPolygon->Clone());
        pRearrangedOpenPolygon->Rearrange();

        std::set<PolygonPtr>::iterator iPolygon = spannedPolygons.begin();
        for (; iPolygon != spannedPolygons.end(); ++iPolygon)
        {
            PolygonPtr pSpannedPolygon = (*iPolygon);
            PolygonPtr pSpannedPolygonWithoutBridgeEdges = (*iPolygon)->Clone();
            pSpannedPolygonWithoutBridgeEdges->RemoveBridgeEdges();

            bool hHasHoles = pSpannedPolygonWithoutBridgeEdges->HasHoles();

            PolygonPtr pCulledOpenPolygon(pRearrangedOpenPolygon->Clone());
            pCulledOpenPolygon->ClipOutside(pSpannedPolygon);

            if (!pCulledOpenPolygon->IsValid())
            {
                continue;
            }

            std::vector<PolygonPtr> unconnectedPolygons;
            pCulledOpenPolygon->GetUnconnectedPolygons(unconnectedPolygons);

            for (int i = 0, iUnconnectedPolygonSize(unconnectedPolygons.size()); i < iUnconnectedPolygonSize; ++i)
            {
                unconnectedPolygons[i]->Rearrange();

                const BrushVec3& vFirstVertex = unconnectedPolygons[i]->GetPos(0);
                const BrushVec3& vLastVertex = unconnectedPolygons[i]->GetPos(unconnectedPolygons[i]->GetVertexCount() - 1);

                DESIGNER_ASSERT(pSpannedPolygon->IsVertexOnCrust(vFirstVertex));
                DESIGNER_ASSERT(pSpannedPolygon->IsVertexOnCrust(vLastVertex));

                int nNewVtxIdx[2] = { -1, -1 };
                int nNewEdgeIdx[2] = { -1, -1 };

                std::set<int> newEdgeIndexCandidates[2];
                pSpannedPolygon->AddVertex(vFirstVertex, &nNewVtxIdx[0], &newEdgeIndexCandidates[0]);
                pSpannedPolygon->AddVertex(vLastVertex, &nNewVtxIdx[1], &newEdgeIndexCandidates[1]);

                for (int k = 0; k < 2; ++k)
                {
                    if (newEdgeIndexCandidates[k].size() > 1)
                    {
                        EdgeIndexSet secondIndices;
                        std::set<int>::iterator iter = newEdgeIndexCandidates[k].begin();
                        for (; iter != newEdgeIndexCandidates[k].end(); ++iter)
                        {
                            secondIndices.insert(pSpannedPolygon->GetEdgeIndexPair(*iter).m_i[1]);
                        }
                        nNewEdgeIdx[k] = pSpannedPolygon->ChooseNextEdge(SEdge(nNewVtxIdx[1 - k], nNewVtxIdx[k]), secondIndices);
                    }
                    else if (newEdgeIndexCandidates[k].size() == 1)
                    {
                        nNewEdgeIdx[k] = *newEdgeIndexCandidates[k].begin();
                    }
                    else
                    {
                        DESIGNER_ASSERT(0);
                        return false;
                    }
                }

                int iVertexSize = unconnectedPolygons[i]->GetVertexCount();

                std::vector<BrushVec3> vList;
                EResultExtract result = pSpannedPolygon->ExtractVertexList(nNewEdgeIdx[0], nNewEdgeIdx[1], vList);
                if (result == eRE_EndAtEndVtx)
                {
                    for (int k = iVertexSize - 2; k >= 1; --k)
                    {
                        vList.push_back(unconnectedPolygons[i]->GetPos(k));
                    }

                    PolygonPtr pNewPolygon = new CD::Polygon(vList, pSpannedPolygon->GetPlane(), pSpannedPolygon->GetMaterialID(), &pSpannedPolygon->GetTexInfo(), true);
                    pNewPolygon->SetFlag(pSpannedPolygon->GetFlag());

                    if (pNewPolygon->IsValid())
                    {
                        if (hHasHoles)
                        {
                            pNewPolygon->Intersect(pSpannedPolygonWithoutBridgeEdges);
                        }
                        if (pNewPolygon->IsValid() && !pNewPolygon->IsOpen())
                        {
                            AddPolygon(pNewPolygon, eOpType_Add);
                        }
                    }

                    vList.clear();
                    result = pSpannedPolygon->ExtractVertexList(nNewEdgeIdx[1], nNewEdgeIdx[0], vList);
                    DESIGNER_ASSERT(result == eRE_EndAtEndVtx);
                    for (int k = 1; k < iVertexSize - 1; ++k)
                    {
                        vList.push_back(unconnectedPolygons[i]->GetPos(k));
                    }

                    pNewPolygon = new CD::Polygon(vList, pSpannedPolygon->GetPlane(), pSpannedPolygon->GetMaterialID(), &(pSpannedPolygon->GetTexInfo()), true);
                    pNewPolygon->SetFlag(pSpannedPolygon->GetFlag());

                    if (pNewPolygon->IsValid())
                    {
                        if (hHasHoles)
                        {
                            pNewPolygon->Intersect(pSpannedPolygonWithoutBridgeEdges);
                        }
                        if (pNewPolygon->IsValid() && !pNewPolygon->IsOpen())
                        {
                            AddPolygon(pNewPolygon, eOpType_Add);
                        }
                    }
                }
                else if (result == eRE_EndAtStartVtx)
                {
                    for (int k = 0; k < iVertexSize - 1; ++k)
                    {
                        const BrushVec3& vertex = unconnectedPolygons[i]->GetPos(k);
                        const BrushVec3& nextVertex = unconnectedPolygons[i]->GetPos(k + 1);
                        pSpannedPolygon->AddEdge(BrushEdge3D(vertex, nextVertex));
                        pSpannedPolygon->AddEdge(BrushEdge3D(nextVertex, vertex));
                    }
                    if (i == iUnconnectedPolygonSize - 1)
                    {
                        AddPolygon(pSpannedPolygon->Clone(), eOpType_Add);
                    }
                }
            }
        }

        InvalidateAABB();
        return true;
    }

    PolygonPtr Model::QueryEquivalentPolygon(PolygonPtr pPolygon, int* pOutPolygonIndex) const
    {
        return QueryEquivalentPolygon(m_ShelfID, pPolygon, pOutPolygonIndex);
    }

    PolygonPtr Model::QueryEquivalentPolygon(int nShelfID, PolygonPtr pPolygon, int* pOutPolygonIndex) const
    {
        int iPolygonSize(m_Polygons[nShelfID].size());
        for (int i = 0; i < iPolygonSize; ++i)
        {
            if (m_Polygons[nShelfID][i] && m_Polygons[nShelfID][i] == pPolygon)
            {
                if (pOutPolygonIndex)
                {
                    *pOutPolygonIndex = i;
                }
                return pPolygon;
            }
        }

        for (int i = 0; i < iPolygonSize; ++i)
        {
            if (m_Polygons[nShelfID][i] && m_Polygons[nShelfID][i]->IsEquivalent(pPolygon))
            {
                if (pOutPolygonIndex)
                {
                    *pOutPolygonIndex = i;
                }
                return m_Polygons[nShelfID][i];
            }
        }
        return NULL;
    }

    void Model::AddPolygon(PolygonPtr pPolygon)
    {
        AddPolygon(m_ShelfID, pPolygon);
    }

    void Model::AddPolygon(int nShelf, PolygonPtr pPolygon)
    {
        bool bEquivalentExists = QueryEquivalentPolygon(nShelf, pPolygon) ? true : false;
        DESIGNER_ASSERT(!bEquivalentExists);
        DESIGNER_ASSERT(pPolygon->IsValid());
        if (!bEquivalentExists && pPolygon->IsValid())
        {
            pPolygon->UpdateUVs();
            m_Polygons[nShelf].push_back(pPolygon);
            InvalidateAABB();
        }
    }

    EClipPolygonResult Model::Clip(const BrushPlane& clipPlane, _smart_ptr<Model>& pOutFrontPart, _smart_ptr<Model>& pOutBackPart, bool bFillFacet) const
    {
        EClipPolygonResult clipResult = eCPR_SUCCESSED;

        _smart_ptr<Model> pFrontPart = new Model;
        _smart_ptr<Model> pBackPart = new Model;

        pFrontPart->m_nModeFlag = pBackPart->m_nModeFlag = m_nModeFlag;
        std::vector<BrushEdge3D> facetEdges;

        for (int i = 0, iPolygonSize(m_Polygons[m_ShelfID].size()); i < iPolygonSize; ++i)
        {
            PolygonPtr pPolygonWithoutBridges;
            if (m_Polygons[m_ShelfID][i]->HasBridgeEdges())
            {
                pPolygonWithoutBridges = m_Polygons[m_ShelfID][i]->Clone();
                pPolygonWithoutBridges->RemoveBridgeEdges();
            }
            else
            {
                pPolygonWithoutBridges = m_Polygons[m_ShelfID][i];
            }

            std::vector<PolygonPtr> pFrontPolygons;
            std::vector<PolygonPtr> pBackPolygons;
            if (!m_Polygons[m_ShelfID][i]->ClipByPlane(clipPlane, pFrontPolygons, pBackPolygons, &facetEdges))
            {
                continue;
            }

            for (int k = 0; k < pFrontPolygons.size(); ++k)
            {
                pFrontPart->AddPolygon(pFrontPolygons[k]->Clone(), eOpType_Add);
            }

            for (int k = 0; k < pBackPolygons.size(); ++k)
            {
                pBackPart->AddPolygon(pBackPolygons[k]->Clone(), eOpType_Add);
            }
        }

        if (bFillFacet)
        {
            std::vector<PolygonPtr> facetPolygons;
            if (GeneratePolygonsFromEdgeList(facetEdges, clipPlane, facetPolygons))
            {
                for (int i = 0, iPolygonSize(facetPolygons.size()); i < iPolygonSize; ++i)
                {
                    PolygonPtr pFacetPolygon = facetPolygons[i];

                    DESIGNER_ASSERT(!pFacetPolygon->IsOpen());
                    if (pFacetPolygon->IsOpen())
                    {
                        continue;
                    }

                    pBackPart->AddPolygon(pFacetPolygon->Clone(), eOpType_Add);
                    pFrontPart->AddPolygon(pFacetPolygon->Clone()->Flip(), eOpType_Add);
                }
            }
            else
            {
                clipResult = eCPR_CLIPSUCCESSEDBUTFILLFAILED;
            }
        }

        pOutFrontPart = NULL;
        pOutBackPart = NULL;

        if (pFrontPart->GetPolygonCount() > 0)
        {
            pOutFrontPart = pFrontPart;
        }

        if (pBackPart->GetPolygonCount() > 0)
        {
            pOutBackPart = pBackPart;
        }

        return pOutFrontPart || pOutBackPart ? clipResult : eCPR_CLIPFAILED;
    }

    bool Model::GeneratePolygonsFromEdgeList(std::vector<BrushEdge3D>& edgeList, const BrushPlane& plane, std::vector<PolygonPtr>& outPolygons)
    {
        if (edgeList.empty())
        {
            return false;
        }

        std::set<int> usedIndices;
        int nEdgeCount(edgeList.size());
        BrushEdge3D currentEdge(edgeList[0]);

        std::vector<PolygonPtr> facetPolygons;
        PolygonPtr pFacetPolygon = new CD::Polygon;
        pFacetPolygon->AddEdge(currentEdge);
        usedIndices.insert(0);
        facetPolygons.push_back(pFacetPolygon);

        while (usedIndices.size() < nEdgeCount)
        {
            bool bFoundNextEdge = false;
            int k = 0;

            for (; k < nEdgeCount; ++k)
            {
                if (usedIndices.find(k) != usedIndices.end())
                {
                    continue;
                }
                if (IsEquivalent(currentEdge.m_v[1], edgeList[k].m_v[0]))
                {
                    currentEdge = edgeList[k];
                    bFoundNextEdge = true;
                    break;
                }
                else if (IsEquivalent(currentEdge.m_v[1], edgeList[k].m_v[1]))
                {
                    currentEdge = edgeList[k].GetInverted();
                    bFoundNextEdge = true;
                    break;
                }
            }

            if (bFoundNextEdge)
            {
                pFacetPolygon->AddEdge(currentEdge);
                usedIndices.insert(k);
            }
            else if (usedIndices.size() < nEdgeCount)
            {
                DESIGNER_ASSERT(!pFacetPolygon->IsOpen());
                if (pFacetPolygon->IsOpen())
                {
                    return false;
                }

                pFacetPolygon = new CD::Polygon;
                facetPolygons.push_back(pFacetPolygon);
                for (int i = 0; i < nEdgeCount; ++i)
                {
                    if (usedIndices.find(i) == usedIndices.end())
                    {
                        currentEdge = edgeList[i];
                        pFacetPolygon->AddEdge(currentEdge);
                        usedIndices.insert(i);
                        break;
                    }
                }
            }
            else
            {
                DESIGNER_ASSERT(0);
                break;
            }
        }

        int ifacetPolygonsize(facetPolygons.size());

        for (int i = 0; i < ifacetPolygonsize; ++i)
        {
            pFacetPolygon = facetPolygons[i];

            std::vector<SVertex> polygon;
            pFacetPolygon->GetLinkedVertices(polygon);

            BrushFloat fSmallestY = 3e10f;
            int nSmallestIndex = 0;
            int iPtSize(polygon.size());
            for (int k = 0; k < iPtSize; ++k)
            {
                BrushVec2 pos2D = plane.W2P(polygon[k].pos);
                if (fSmallestY > pos2D.y)
                {
                    nSmallestIndex = k;
                    fSmallestY = pos2D.y;
                }
            }

            pFacetPolygon->SetPlane(BrushPlane(polygon[((nSmallestIndex - 1) + iPtSize) % iPtSize].pos, polygon[nSmallestIndex].pos, polygon[(nSmallestIndex + 1) % iPtSize].pos));

            if (!pFacetPolygon->GetPlane().IsSameFacing(plane))
            {
                pFacetPolygon->Flip();
            }

            pFacetPolygon->SetPlane(plane);
        }

        usedIndices.clear();
        for (int i = 0; i < ifacetPolygonsize; ++i)
        {
            if (usedIndices.find(i) != usedIndices.end())
            {
                continue;
            }
            bool bInclude = false;
            for (int k = 0; k < ifacetPolygonsize; ++k)
            {
                if (i == k)
                {
                    continue;
                }
                if (usedIndices.find(k) != usedIndices.end())
                {
                    continue;
                }
                if (facetPolygons[i]->IncludeAllEdges(facetPolygons[k]))
                {
                    usedIndices.insert(k);
                    facetPolygons[i]->Subtract(facetPolygons[k]);
                    facetPolygons[k] = NULL;
                    bInclude = true;
                }
            }
            if (bInclude)
            {
                usedIndices.insert(i);
            }
        }

        for (int i = 0; i < ifacetPolygonsize; ++i)
        {
            if (facetPolygons[i] == NULL)
            {
                continue;
            }
            outPolygons.push_back(facetPolygons[i]);
        }

        return true;
    }

    void Model::ResetFromList(const std::vector<PolygonPtr>& polygonList)
    {
        Clear();

        for (int i = 0, iPolygonCount(polygonList.size()); i < iPolygonCount; ++i)
        {
            AddPolygon(polygonList[i], eOpType_Union);
        }
    }

    AABB Model::GetBoundBox(int nShelf)
    {
        MODEL_SHELF_RECONSTRUCTOR(this);

        for (int i = 0; i < kMaxShelfCount; ++i)
        {
            if (i == nShelf || nShelf == -1)
            {
                if (m_BoundBox[i].bValid)
                {
                    if (i == nShelf)
                    {
                        return m_BoundBox[i].aabb;
                    }
                    continue;
                }

                m_BoundBox[i].bValid = true;
                m_BoundBox[i].aabb.Reset();

                SetShelf(i);
                for (int k = 0, iPolygonCount(GetPolygonCount()); k < iPolygonCount; ++k)
                {
                    m_BoundBox[i].aabb.Add(GetPolygon(k)->GetBoundBox());
                }

                if (i == nShelf)
                {
                    return m_BoundBox[i].aabb;
                }
            }
        }

        AABB aabb;
        aabb.Reset();
        if (nShelf == -1)
        {
            aabb.Add(m_BoundBox[0].aabb);
            aabb.Add(m_BoundBox[1].aabb);
            return aabb;
        }

        DESIGNER_ASSERT(0 && "Invalid calling Model::GetBoundBox()");
        return aabb;
    }

    bool Model::IsEmpty(int nShelf) const
    {
        if (nShelf == -1)
        {
            return m_Polygons[0].empty() && m_Polygons[1].empty();
        }

        DESIGNER_ASSERT(nShelf == 0 || nShelf == 1);
        if (nShelf != 0 && nShelf != 1)
        {
            return false;
        }

        return m_Polygons[nShelf].empty();
    }

    bool Model::HasClosedPolygon(int nShelf) const
    {
        for (int i = 0; i < 2; ++i)
        {
            if (i != nShelf && nShelf != -1)
            {
                continue;
            }
            for (int k = 0, iPolygonCount(m_Polygons[i].size()); k < iPolygonCount; ++k)
            {
                if (!m_Polygons[i][k]->IsOpen())
                {
                    return true;
                }
            }
        }
        return false;
    }

    void Model::ResetDB(int nFlag, int nValidShelfID)
    {
        m_pDB->Reset(this, nFlag, nValidShelfID);
    }

    void Model::SetSubdivisionResult(HalfEdgeMesh* pSubdividedHalfMesh)
    {
        if (m_pSubdividionResult)
        {
            m_pSubdividionResult->Release();
            m_pSubdividionResult = NULL;
        }

        m_pSubdividionResult = pSubdividedHalfMesh;
        if (m_pSubdividionResult)
        {
            m_pSubdividionResult->AddRef();
        }
    }
}