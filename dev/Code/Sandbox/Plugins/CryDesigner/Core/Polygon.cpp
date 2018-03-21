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
#include "Polygon.h"
#include "Util/GeometryUtil.h"
#include "IDisplayViewport.h"
#include "Base64.h"
#include "Convexes.h"
#include "BSPTree2D.h"
#include "PolygonDecomposer.h"
#include "BrushHelper.h"

#include <QUuid>

namespace CD
{
    Polygon::Polygon()
    {
        InitForCreation();
    }

    Polygon::Polygon(const Polygon& polygon)
    {
        InitForCreation();
        operator =(polygon);
    }

    Polygon::Polygon(const std::vector<BrushVec3>& vertices, const std::vector<SEdge>& edges)
    {
        InitForCreation();
        Reset(vertices, edges);
        UpdateUVs();
    }

    Polygon::Polygon(const std::vector<BrushVec3>& vertices)
    {
        InitForCreation();
        Reset(vertices);
        UpdateUVs();
    }

    Polygon::Polygon(const std::vector<SVertex>& vertices)
    {
        InitForCreation();
        Reset(vertices);
        UpdateUVs();
    }

    Polygon::Polygon(const std::vector<BrushVec3>& vertices, const BrushPlane& plane, int matID, const STexInfo* pTexInfo, bool bClosed)
    {
        InitForCreation(plane, matID);
        SetVertexList_Basic(ToSVertexList(vertices));
        InitializeEdgesAndUpdate(bClosed);
        if (pTexInfo)
        {
            SetTexInfo(*pTexInfo);
        }
        UpdateUVs();
        Optimize();
    }

    Polygon::Polygon(const std::vector<SVertex>& vertices, const BrushPlane& plane, int matID, const STexInfo* pTexInfo, bool bClosed)
    {
        InitForCreation(plane, matID);
        SetVertexList_Basic(vertices);
        InitializeEdgesAndUpdate(bClosed);
        if (pTexInfo)
        {
            SetTexInfo(*pTexInfo);
        }
        Optimize();
    }

    Polygon::Polygon(const std::vector<BrushVec3>& vertices, const std::vector<SEdge>& edges, const BrushPlane& plane, int matID, const STexInfo* pTexInfo, bool bOptimizePolygon)
    {
        InitForCreation(plane, matID);
        SetVertexList_Basic(ToSVertexList(vertices));
        SetEdgeList_Basic(edges);
        if (pTexInfo)
        {
            SetTexInfo(*pTexInfo);
        }
        if (bOptimizePolygon)
        {
            Optimize();
        }
        UpdateUVs();
    }

    Polygon::Polygon(const std::vector<SVertex>& vertices, const std::vector<SEdge>& edges, const BrushPlane& plane, int matID, const STexInfo* pTexInfo, bool bOptimizePolygon)
    {
        InitForCreation(plane, matID);
        SetVertexList_Basic(vertices);
        SetEdgeList_Basic(edges);
        if (pTexInfo)
        {
            SetTexInfo(*pTexInfo);
        }
        if (bOptimizePolygon)
        {
            Optimize();
        }
    }

    Polygon::Polygon(const std::vector<BrushVec2>& points, const BrushPlane& plane, int matID, const STexInfo* pTexInfo, bool bClosed)
    {
        InitForCreation(plane, matID);
        Transform2Vertices(points, plane);
        InitializeEdgesAndUpdate(bClosed);
        if (pTexInfo)
        {
            SetTexInfo(*pTexInfo);
        }
        Optimize();
        UpdateUVs();
    }

    void Polygon::Init()
    {
        InvalidateCacheData();
        m_MaterialID = 0;
        m_Flag = 0;
        m_Plane.Set(BrushVec3(0, 0, 0), 0);
        m_Vertices.clear();
        m_Edges.clear();
        m_GUID = QUuid::createUuid();
    }

    void Polygon::InitForCreation(const BrushPlane& plane, int matID)
    {
        m_pBSPTree = NULL;
        m_pConvexes = NULL;
        m_pTriangles = NULL;
        m_pRepresentativePos = NULL;
        m_pBoundBox = NULL;

        m_MaterialID = matID;
        m_Plane = plane;
        m_Flag = 0;
        m_Vertices.clear();
        m_Edges.clear();
        m_PrivateFlag = eRPF_Invalid;

        m_GUID = QUuid::createUuid();
    }

    void Polygon::Reset(const std::vector<SVertex>& vertices, const std::vector<SEdge>& edgeList)
    {
        SetVertexList_Basic(vertices);
        SetEdgeList_Basic(edgeList);

        std::vector<SVertex> vList;
        GetLinkedVertices(vList);
        ComputePlane(vList, m_Plane);

        UpdateBoundBox();
        Optimize();
    }

    void Polygon::Reset(const std::vector<SVertex>& vertices)
    {
        std::vector<SEdge> edgeList;
        for (int i = 0, iVertexCount(vertices.size()); i < iVertexCount; ++i)
        {
            edgeList.push_back(SEdge(i, (i + 1) % iVertexCount));
        }
        Reset(vertices, edgeList);
    }

    Polygon::~Polygon()
    {
        InvalidateCacheData();
    }

    void Polygon::InitializeEdgesAndUpdate(bool bClosed)
    {
        DeleteAllEdges_Basic();
        m_Edges.reserve(m_Vertices.size());
        for (int i = 0, iVertexSize(m_Vertices.size()); i < iVertexSize; ++i)
        {
            if (!bClosed)
            {
                if (i == iVertexSize - 1)
                {
                    break;
                }
            }
            AddEdge_Basic(SEdge(i, (i + 1) % iVertexSize));
        }
        UpdateBoundBox();
    }

    Polygon& Polygon::operator = (const Polygon& polygon)
    {
        SetVertexList_Basic(polygon.m_Vertices);
        m_Plane = polygon.m_Plane;
        m_MaterialID = polygon.m_MaterialID;
        m_Flag = polygon.m_Flag;
        m_TexInfo = polygon.m_TexInfo;
        m_GUID = QUuid::createUuid();
        SetEdgeList_Basic(polygon.m_Edges);
        m_PrivateFlag = polygon.m_PrivateFlag;
        return *this;
    }

    PolygonPtr Polygon::Clone() const
    {
        return new Polygon(*this);
    }

    void Polygon::Clear()
    {
        DeleteAllVertices_Basic();
        DeleteAllEdges_Basic();
    }

    void Polygon::CopyEdges(const std::vector<SEdge>& sourceEdges, std::vector<SEdge>& destincationEdges)
    {
        destincationEdges.clear();
        destincationEdges.reserve(sourceEdges.size());
        for (int i = 0, iEdgeSize(sourceEdges.size()); i < iEdgeSize; ++i)
        {
            destincationEdges.push_back(sourceEdges[i]);
        }
    }

    void Polygon::Display(DisplayContext& dc) const
    {
        if (!IsValid())
        {
            return;
        }

        for (int i = 0, iEdgeSize(m_Edges.size()); i < iEdgeSize; ++i)
        {
            dc.DrawLine(GetPos(m_Edges[i].m_i[0]), GetPos(m_Edges[i].m_i[1]));
        }
    }

    bool Polygon::IsPassed(const BrushVec3& raySrc, const BrushVec3& rayDir, BrushFloat& outT)
    {
        BrushVec3 vIntersection;

        if (CheckFlags(ePolyFlag_NonplanarQuad))
        {
            if (GetTriangles(false)->IsPassed(raySrc, rayDir, outT))
            {
                return true;
            }
        }
        else if (GetPlane().HitTest(raySrc, raySrc + rayDir, &outT, &vIntersection))
        {
            if (outT > 0 && GetTriangles())
            {
                FlexibleMesh* pTriangle = GetTriangles();
                return pTriangle->IsPassed(raySrc, rayDir, outT);
            }
        }

        return false;
    }

    bool Polygon::IsIdentical(PolygonPtr pPolygon) const
    {
        if (pPolygon == NULL)
        {
            return false;
        }

        if (m_Edges.size() != pPolygon->m_Edges.size())
        {
            return false;
        }

        if (!IsPlaneEquivalent(pPolygon))
        {
            return false;
        }

        bool bTestOnceMoreTime = false;

        for (int i = 0, iEdgeSize(m_Edges.size()); i < iEdgeSize; ++i)
        {
            BrushEdge3D edge1(GetPos(m_Edges[i].m_i[0]), GetPos(m_Edges[i].m_i[1]));
            BrushEdge3D edge1Inverse(GetPos(m_Edges[i].m_i[1]), GetPos(m_Edges[i].m_i[0]));
            bool bSameExist(false);
            for (int k = 0, kEdgeSize(pPolygon->m_Edges.size()); k < kEdgeSize; ++k)
            {
                BrushEdge3D edge2(pPolygon->GetPos(pPolygon->m_Edges[k].m_i[0]), pPolygon->GetPos(pPolygon->m_Edges[k].m_i[1]));
                if (edge1.IsEquivalent(edge2) || edge1Inverse.IsEquivalent(edge2))
                {
                    bSameExist = true;
                    break;
                }
            }
            if (!bSameExist)
            {
                bTestOnceMoreTime = true;
                break;
            }
        }

        if (!bTestOnceMoreTime)
        {
            return true;
        }

        for (int i = 0, iEdgeSize(m_Edges.size()); i < iEdgeSize; ++i)
        {
            BrushEdge edge1(m_Plane.W2P(GetPos(m_Edges[i].m_i[0])), m_Plane.W2P(GetPos(m_Edges[i].m_i[1])));
            BrushEdge edge1Inverse(edge1.m_v[1], edge1.m_v[0]);
            bool bSameExist(false);
            for (int k = 0, kEdgeSize(pPolygon->m_Edges.size()); k < kEdgeSize; ++k)
            {
                BrushEdge edge2(m_Plane.W2P(pPolygon->GetPos(pPolygon->m_Edges[k].m_i[0])), m_Plane.W2P(pPolygon->GetPos(pPolygon->m_Edges[k].m_i[1])));
                if (edge1.IsEquivalent(edge2) || edge1Inverse.IsEquivalent(edge2))
                {
                    bSameExist = true;
                    break;
                }
            }
            if (bSameExist == false)
            {
                return false;
            }
        }

        return true;
    }

    EIntersectionType Polygon::HasIntersection(PolygonPtr pPolygon)
    {
        if (!IsValid() || !pPolygon)
        {
            return eIT_None;
        }

        if (!GetBSPTree())
        {
            return eIT_None;
        }

        if (!IsPlaneEquivalent(pPolygon))
        {
            return eIT_None;
        }

        if (IsIdentical(pPolygon))
        {
            return eIT_Intersection;
        }

        if (IsOpen(m_Vertices, m_Edges))
        {
            return eIT_None;
        }

        bool bHaveTouched = false;

        for (int i = 0, iSize(pPolygon->m_Vertices.size()); i < iSize; ++i)
        {
            EPointPosEnum checkVertexIn = GetBSPTree()->IsVertexIn(pPolygon->GetPos(i));
            if (checkVertexIn == ePP_INSIDE)
            {
                return eIT_Intersection;
            }
            else if (checkVertexIn == ePP_BORDER)
            {
                bHaveTouched = true;
            }
        }

        for (int i = 0, iSize(pPolygon->GetEdgeCount()); i < iSize; ++i)
        {
            BrushEdge3D edge = pPolygon->GetEdge(i);
            EIntersectionType intersectionType(GetBSPTree()->HasIntersection(edge));
            if (intersectionType == eIT_Intersection)
            {
                return intersectionType;
            }
            else if (intersectionType == eIT_JustTouch)
            {
                bHaveTouched = true;
            }
        }

        if (bHaveTouched)
        {
            std::vector< std::pair<int, int> > vertexIndices;
            for (int i = 0, iVertexCount(m_Vertices.size()); i < iVertexCount; ++i)
            {
                int nVertexIndex = -1;
                if (!pPolygon->GetVertexIndex(m_Vertices[i].pos, nVertexIndex))
                {
                    continue;
                }
                vertexIndices.push_back(std::pair<int, int>(i, nVertexIndex));
            }
            if (vertexIndices.size() == 1)
            {
                std::vector<int> edgeIndices[2];
                GetEdgesByVertexIndex(vertexIndices[0].first, edgeIndices[0]);
                pPolygon->GetEdgesByVertexIndex(vertexIndices[0].second, edgeIndices[1]);

                for (int i = 0, iEdgeIndexCount0(edgeIndices[0].size()); i < iEdgeIndexCount0; ++i)
                {
                    BrushEdge3D e0 = GetEdge(edgeIndices[0][i]);
                    for (int k = 0, iEdgeIndexCount1(edgeIndices[1].size()); k < iEdgeIndexCount1; ++k)
                    {
                        BrushEdge3D e1 = pPolygon->GetEdge(edgeIndices[1][k]);
                        if (e0.ContainVertex(e1.m_v[0]) && e0.ContainVertex(e1.m_v[1]))
                        {
                            return eIT_JustTouch;
                        }
                    }
                }
                return eIT_None;
            }
        }

        return bHaveTouched ? eIT_JustTouch : eIT_None;
    }

    bool Polygon::IsEdgeOnCrust(const BrushEdge3D& edge, int* pOutEdgeIndex, BrushEdge3D* pOutIntersectedEdge) const
    {
        BrushLine3D edgeline(edge.m_v[0], edge.m_v[1]);
        BrushLine3D invEdgeline(edge.m_v[1], edge.m_v[0]);

        for (int i = 0, iEdgeSize(m_Edges.size()); i < iEdgeSize; ++i)
        {
            BrushEdge3D edgeOnPolygon = GetEdge(i);

            if (!edgeOnPolygon.IsEquivalent(edge) && !edgeOnPolygon.GetInverted().IsEquivalent(edge))
            {
                BrushLine3D edgeLineOnPolygon(edgeOnPolygon.m_v[0], edgeOnPolygon.m_v[1]);
                if (!CD::IsEquivalent(edgeline.m_Dir, edgeLineOnPolygon.m_Dir) && !CD::IsEquivalent(invEdgeline.m_Dir, edgeLineOnPolygon.m_Dir))
                {
                    continue;
                }

                BrushVec3 vProjectedPos;
                if (!edgeOnPolygon.GetProjectedPos(edge.m_v[0], vProjectedPos))
                {
                    continue;
                }

                if ((vProjectedPos - edge.m_v[0]).GetLength() >= kDesignerEpsilon)
                {
                    continue;
                }
            }

            BrushEdge3D intersectionEdge;
            EOperationResult intersectionResult(IntersectEdge3D(edge, edgeOnPolygon, intersectionEdge));
            if (intersectionResult == eOR_One)
            {
                BrushFloat fLength = (intersectionEdge.m_v[1] - intersectionEdge.m_v[0]).GetLength();
                if (fLength < kDesignerEpsilon)
                {
                    continue;
                }
                if (pOutEdgeIndex)
                {
                    *pOutEdgeIndex = i;
                }
                if (pOutIntersectedEdge)
                {
                    *pOutIntersectedEdge = intersectionEdge;
                }
                return true;
            }
        }

        return false;
    }

    bool Polygon::IsEquivalent(const PolygonPtr& pPolygon) const
    {
        if (this == pPolygon)
        {
            return true;
        }

        if (GetEdgeCount() != pPolygon->GetEdgeCount() || GetVertexCount() != pPolygon->GetVertexCount())
        {
            return false;
        }

        if (GetFlag() != pPolygon->GetFlag())
        {
            return false;
        }

        if (!GetPlane().IsEquivalent(pPolygon->GetPlane()))
        {
            return false;
        }

        for (int i = 0, iEdgeSize(GetEdgeCount()); i < iEdgeSize; ++i)
        {
            if (!pPolygon->HasEdge(GetEdge(i), true))
            {
                return false;
            }
        }

        return true;
    }

    bool Polygon::SubtractEdge(const BrushEdge3D& edge, std::vector<BrushEdge3D>& outSubtractedEdges) const
    {
        for (int i = 0, iEdgeSize(m_Edges.size()); i < iEdgeSize; ++i)
        {
            BrushEdge3D edgeOnPolygon = GetEdge(i);
            if (!ToEdge2D(m_Plane, edgeOnPolygon).IsIdenticalLine(ToEdge2D(m_Plane, edge)))
            {
                continue;
            }
            BrushEdge3D subtractedEdges[2];
            EOperationResult opResult = SubtractEdge3D(edge, edgeOnPolygon, subtractedEdges);
            if (opResult != eOR_Invalid)
            {
                if (opResult == eOR_One || opResult == eOR_Two)
                {
                    outSubtractedEdges.push_back(subtractedEdges[0]);
                    if (opResult == eOR_Two)
                    {
                        outSubtractedEdges.push_back(subtractedEdges[1]);
                    }
                }
                return true;
            }
        }
        return false;
    }

    bool Polygon::HasEdge(const BrushEdge3D& edge, bool bApplyDir, int* pOutEdgeIndex) const
    {
        BrushEdge3D invertedEdge(edge.GetInverted());

        for (int i = 0, iEdgeSize(m_Edges.size()); i < iEdgeSize; ++i)
        {
            BrushEdge3D edgeOnPolygon = GetEdge(i);
            if (bApplyDir)
            {
                if (edge.IsEquivalent(edgeOnPolygon))
                {
                    if (pOutEdgeIndex)
                    {
                        *pOutEdgeIndex = i;
                    }
                    return true;
                }
            }
            else
            {
                if (edge.IsEquivalent(edgeOnPolygon) || invertedEdge.IsEquivalent(edgeOnPolygon))
                {
                    if (pOutEdgeIndex)
                    {
                        *pOutEdgeIndex = i;
                    }
                    return true;
                }
            }
        }
        return false;
    }

    bool Polygon::HasOverlappedEdges(PolygonPtr pPolygon) const
    {
        for (int i = 0; i < GetEdgeCount(); ++i)
        {
            BrushEdge3D edge = GetEdge(i);
            if (pPolygon->IsEdgeOnCrust(edge))
            {
                return true;
            }
        }
        return false;
    }

    EIntersectionType Polygon::HasIntersection(PolygonPtr pPolygon0, PolygonPtr pPolygon1)
    {
        if (!pPolygon0 || !pPolygon1)
        {
            return eIT_None;
        }

        EIntersectionType intersetionType0(pPolygon0->HasIntersection(pPolygon1));
        EIntersectionType intersetionType1(pPolygon1->HasIntersection(pPolygon0));

        if (intersetionType0 == eIT_Intersection || intersetionType1 == eIT_Intersection)
        {
            return eIT_Intersection;
        }

        if (intersetionType0 == eIT_JustTouch || intersetionType1 == eIT_JustTouch)
        {
            return eIT_JustTouch;
        }

        return eIT_None;
    }

    bool Polygon::IncludeAllEdges(PolygonPtr pPolygon) const
    {
        if (!IsValid() || !pPolygon || !GetBSPTree() || !IsPlaneEquivalent(pPolygon))
        {
            return false;
        }

        if (IsIdentical(pPolygon))
        {
            return true;
        }

        for (int i = 0, iEdgeCount(pPolygon->GetEdgeCount()); i < iEdgeCount; ++i)
        {
            BrushEdge3D edge = pPolygon->GetEdge(i);
            if (!GetBSPTree()->IsInside(edge, false))
            {
                return false;
            }
        }

        return true;
    }

    bool Polygon::Include(PolygonPtr pPolygon) const
    {
        if (!IsValid() || !pPolygon || !GetBSPTree() || !IsPlaneEquivalent(pPolygon))
        {
            return false;
        }

        if (IsIdentical(pPolygon))
        {
            return true;
        }

        if (!IncludeAllEdges(pPolygon))
        {
            return false;
        }

        for (int i = 0, iEdgeCount(GetEdgeCount()); i < iEdgeCount; ++i)
        {
            BrushEdge3D edge = GetEdge(i);
            if (pPolygon->GetBSPTree()->IsInside(edge, false))
            {
                return false;
            }
        }

        return true;
    }

    bool Polygon::IntersectedBetweenAABBs(const AABB& aabb) const
    {
        return aabb.IsIntersectBox(GetBoundBox());
    }

    bool Polygon::Include(const BrushVec3& vertex) const
    {
        if (!IsValid() || !GetBSPTree())
        {
            return false;
        }

        return GetBSPTree()->IsVertexIn(vertex) != ePP_OUTSIDE;
    }

    bool Polygon::IsOpen(const std::vector<SVertex>& vertices, const std::vector<SEdge>& edges) const
    {
        for (int i = 0, iEdgeSize(edges.size()); i < iEdgeSize; ++i)
        {
            const SEdge& edge(edges[i]);
            int prevEdge = -1;
            int nextEdge = -1;
            GetAdjacentEdgeIndexWithEdgeIndex(i, prevEdge, nextEdge, vertices, edges);
            if (prevEdge == -1 || nextEdge == -1)
            {
                return true;
            }
        }
        return false;
    }

    bool Polygon::QueryIntersections(const BrushEdge3D& edge, std::map<BrushFloat, BrushVec3>& outIntersections) const
    {
        if (std::abs(m_Plane.Distance(edge.m_v[0])) >= kDesignerEpsilon || std::abs(m_Plane.Distance(edge.m_v[1])) >= kDesignerEpsilon)
        {
            return false;
        }

        BrushEdge edge2D(m_Plane.W2P(edge.m_v[0]), m_Plane.W2P(edge.m_v[1]));

        for (int i = 0, iEdgeCount(m_Edges.size()); i < iEdgeCount; ++i)
        {
            BrushEdge polygonEdge2D(m_Plane.W2P(m_Vertices[m_Edges[i].m_i[0]].pos), m_Plane.W2P(m_Vertices[m_Edges[i].m_i[1]].pos));

            BrushVec2 vIntersection;
            if (!edge2D.GetIntersect(polygonEdge2D, vIntersection))
            {
                continue;
            }

            BrushFloat fLength = (edge2D.m_v[0] - vIntersection).GetLength();
            outIntersections[fLength] = m_Plane.P2W(vIntersection);
        }

        return outIntersections.empty() ? false : true;
    }

    bool Polygon::QueryIntersections(const BrushPlane& plane, const BrushLine3D& crossLine3D, std::vector<BrushEdge3D>& outSortedIntersections) const
    {
        std::vector<BrushVec3> intersections;

        for (int i = 0, iEdgeCount(m_Edges.size()); i < iEdgeCount; ++i)
        {
            const BrushVec3& v0 = m_Vertices[m_Edges[i].m_i[0]].pos;
            const BrushVec3& v1 = m_Vertices[m_Edges[i].m_i[1]].pos;

            BrushFloat d0 = plane.Distance(v0);
            BrushFloat d1 = plane.Distance(v1);

            if (d0 * d1 < 0)
            {
                BrushVec3 intersection;
                if (plane.HitTest(v0, v1, NULL, &intersection))
                {
                    intersections.push_back(intersection);
                }
            }
        }

        return SortVerticesAlongCrossLine(intersections, crossLine3D, outSortedIntersections);
    }

    bool Polygon::QueryNearestEdge(const BrushVec3& vertex, BrushEdge3D& outNearestEdge, BrushVec3& outNearestPos) const
    {
        BrushFloat fLastDistance = 3e10f;
        for (int i = 0, iEdgeSize(m_Edges.size()); i < iEdgeSize; ++i)
        {
            const SEdge& edgeIndex(m_Edges[i]);
            BrushEdge3D edge(GetPos(edgeIndex.m_i[0]), GetPos(edgeIndex.m_i[1]));

            bool bInEdge;
            BrushVec3 outPos;
            if (!edge.GetNearestVertex(vertex, outPos, bInEdge))
            {
                continue;
            }

            BrushFloat fDistance((vertex - outPos).GetLength());
            if (fDistance < fLastDistance)
            {
                outNearestPos = outPos;
                fLastDistance = fDistance;
                outNearestEdge = edge;
            }
        }

        return fLastDistance < 3e10f;
    }

    bool Polygon::QueryNearestPosFromBoundary(const BrushVec3& vertex, BrushVec3& outNearestPos) const
    {
        if (!GetBSPTree())
        {
            return false;
        }

        if (GetBSPTree()->IsVertexIn(vertex) != ePP_OUTSIDE)
        {
            outNearestPos = GetPlane().P2W(GetPlane().W2P(vertex));
            return true;
        }

        BrushFloat fNearestDistance(kEnoughBigNumber);
        for (int i = 0, iEdgeSize(m_Edges.size()); i < iEdgeSize; ++i)
        {
            BrushEdge3D edge = GetEdge(i);
            BrushFloat fDistance;
            EResultDistance resultDistance(BrushEdge3D::GetSquaredDistance(edge, vertex, fDistance));
            if (fDistance < fNearestDistance)
            {
                fNearestDistance = fDistance;

                if (resultDistance == eResultDistance_EdgeP0)
                {
                    outNearestPos = edge.m_v[0];
                }
                else if (resultDistance == eResultDistance_EdgeP1)
                {
                    outNearestPos = edge.m_v[1];
                }
                else if (resultDistance == eResultDistance_Middle)
                {
                    BrushVec3 edgeVector(edge.m_v[1] - edge.m_v[0]);
                    outNearestPos = edge.m_v[0] + (edgeVector.Dot(vertex - edge.m_v[0]) / edgeVector.Dot(edgeVector)) * edgeVector;
                }
            }
        }

        return fNearestDistance < kEnoughBigNumber;
    }

    bool Polygon::QueryEdgesContainingVertex(const BrushVec3& vertex, std::vector<int>& outEdgeIndices) const
    {
        bool bAdded = false;
        BrushVec2 ptPos = m_Plane.W2P(vertex);
        for (int i = 0, iEdgeListCount(m_Edges.size()); i < iEdgeListCount; ++i)
        {
            BrushEdge edge(m_Plane.W2P(m_Vertices[m_Edges[i].m_i[0]].pos), m_Plane.W2P(m_Vertices[m_Edges[i].m_i[1]].pos));
            if (edge.IsInside(ptPos))
            {
                outEdgeIndices.push_back(i);
                bAdded = true;
            }
        }

        return bAdded;
    }

    bool Polygon::QueryEdgesHavingVertex(const BrushVec3& vertex, std::vector<int>& outEdgeIndices) const
    {
        bool bAdded = false;

        for (int i = 0, iEdgeListCount(m_Edges.size()); i < iEdgeListCount; ++i)
        {
            BrushEdge3D e = GetEdge(i);
            if (CD::IsEquivalent(e.m_v[0], vertex))
            {
                outEdgeIndices.push_back(i);
                bAdded = true;
            }
        }

        for (int i = 0, iEdgeListCount(m_Edges.size()); i < iEdgeListCount; ++i)
        {
            BrushEdge3D e = GetEdge(i);
            if (CD::IsEquivalent(e.m_v[1], vertex))
            {
                outEdgeIndices.push_back(i);
                bAdded = true;
            }
        }

        return bAdded;
    }

    bool Polygon::QueryEdges(const BrushVec3& vertex, int vIndexInEdge, std::set<int>* pOutEdgeIndices) const
    {
        DESIGNER_ASSERT(vIndexInEdge == 0 || vIndexInEdge == 1);

        for (int i = 0, iEdgeSize(m_Edges.size()); i < iEdgeSize; ++i)
        {
            if (CD::IsEquivalent(m_Vertices[m_Edges[i].m_i[vIndexInEdge]].pos, vertex))
            {
                if (pOutEdgeIndices)
                {
                    pOutEdgeIndices->insert(i);
                }
                else
                {
                    return true;
                }
            }
        }

        return !pOutEdgeIndices || pOutEdgeIndices->empty() ? false : true;
    }

    bool Polygon::QueryAxisAlignedLines(std::vector<BrushLine>& outLines)
    {
        BrushVec2 vAxises[] = { BrushVec2(1, 0), BrushVec2(-1, 0), BrushVec2(0, 1), BrushVec2(0, -1) };
        bool bAdded = false;

        for (int i = 0, iEdgeSize(GetEdgeCount()); i < iEdgeSize; ++i)
        {
            BrushEdge3D edge = GetEdge(i);
            BrushVec2 v0_2D = GetPlane().W2P(edge.m_v[0]);
            BrushVec2 v1_2D = GetPlane().W2P(edge.m_v[1]);

            BrushVec2 vDir = (v1_2D - v0_2D).GetNormalized();

            for (int k = 0; k < sizeof(vAxises) / sizeof(*vAxises); ++k)
            {
                if (vDir.Dot(vAxises[k]) >= 1 - kDesignerEpsilon)
                {
                    outLines.push_back(BrushLine(v0_2D, v1_2D));
                    bAdded = true;
                    break;
                }
            }
        }

        return bAdded;
    }

    bool Polygon::ShouldOrderReverse(PolygonPtr BPolygon) const
    {
        if (!BPolygon)
        {
            return false;
        }

        if (!BPolygon->IsOpen())
        {
            return false;
        }

        bool bReverseOrderingEdge = false;
        for (int i = 0, iEdgeSize(BPolygon->GetEdgeCount()); i < iEdgeSize; ++i)
        {
            BrushEdge3D edge = BPolygon->GetEdge(i);
            for (int a = 0; a < 2; ++a)
            {
                int nExistedIndex;
                if (Exist(edge.m_v[a], &nExistedIndex))
                {
                    for (int k = 0, nAEdgeSize(m_Edges.size()); k < nAEdgeSize; ++k)
                    {
                        if (m_Edges[k].m_i[a] == nExistedIndex)
                        {
                            bReverseOrderingEdge = true;
                            break;
                        }
                        if (bReverseOrderingEdge)
                        {
                            break;
                        }
                    }
                }
                if (bReverseOrderingEdge)
                {
                    break;
                }
            }
        }

        return bReverseOrderingEdge;
    }

    bool Polygon::AddOpenPolygon(PolygonPtr BPolygon)
    {
        if (!BPolygon || !BPolygon->IsOpen())
        {
            return false;
        }

        std::vector<SVertex> linkedVertices;
        if (!BPolygon->GetLinkedVertices(linkedVertices))
        {
            return false;
        }

        BrushVec3 eitherVertices[2] = { linkedVertices[0].pos, linkedVertices[linkedVertices.size() - 1].pos };
        std::vector<int> edgeIndices[2];
        int newVertexIndices[2] = { -1, -1 };

        for (int k = 0; k < 2; ++k)
        {
            if (!QueryEdgesContainingVertex(eitherVertices[k], edgeIndices[k]))
            {
                return false;
            }

            int nPrevVertexListCount = m_Vertices.size();
            newVertexIndices[k] = AddVertex(m_Vertices, SVertex(eitherVertices[k]));
            if (nPrevVertexListCount == m_Vertices.size())
            {
                continue;
            }

            for (int i = 0, iCount(edgeIndices[k].size()); i < iCount; ++i)
            {
                SEdge newEdge(m_Edges[edgeIndices[k][i]].m_i[0], newVertexIndices[k]);
                m_Edges[edgeIndices[k][i]].m_i[0] = newVertexIndices[k];
                m_Edges.push_back(newEdge);
            }
        }

        int nLinkedVertexCount(linkedVertices.size());
        for (int i = 0; i < nLinkedVertexCount - 1; ++i)
        {
            int nVertexIndex = AddVertex(m_Vertices, SVertex(linkedVertices[i]));
            int nNextVertexIndex = AddVertex(m_Vertices, SVertex(linkedVertices[i + 1]));
            m_Edges.push_back(SEdge(nVertexIndex, nNextVertexIndex));
            m_Edges.push_back(SEdge(nNextVertexIndex, nVertexIndex));
        }

        return true;
    }

    void Polygon::ConnectNearVertices(std::vector<SVertex>& vertices, std::vector<SEdge>& edges) const
    {
        std::vector<int> edgeListFirstDisconnect;
        for (int i = 0, iEdgeCount(edges.size()); i < iEdgeCount; ++i)
        {
            bool bFound = false;
            for (int k = 0; k < iEdgeCount; ++k)
            {
                int nIndex = (i + 1 + k) % iEdgeCount;
                if (edges[i].m_i[0] == edges[nIndex].m_i[1])
                {
                    bFound = true;
                    break;
                }
            }
            if (bFound == false)
            {
                edgeListFirstDisconnect.push_back(i);
            }
        }
        std::vector<int> edgeListSecondDisconnect;
        for (int i = 0, iEdgeCount(edges.size()); i < iEdgeCount; ++i)
        {
            bool bFound = false;
            for (int k = 0; k < iEdgeCount; ++k)
            {
                int nIndex = (i + 1 + k) % iEdgeCount;
                if (edges[i].m_i[1] == edges[nIndex].m_i[0])
                {
                    bFound = true;
                    break;
                }
            }
            if (bFound == false)
            {
                edgeListSecondDisconnect.push_back(i);
            }
        }

        if (!edgeListFirstDisconnect.empty() && !edgeListSecondDisconnect.empty())
        {
            std::set<int> usedIndices;
            for (int i = 0, iFirstCount(edgeListFirstDisconnect.size()); i < iFirstCount; ++i)
            {
                if (usedIndices.find(edgeListFirstDisconnect[i]) != usedIndices.end())
                {
                    continue;
                }
                BrushFloat theLeastDistance = (BrushFloat)3e10;
                int theLeastIndex = -1;
                for (int k = 0, iSecondCount(edgeListSecondDisconnect.size()); k < iSecondCount; ++k)
                {
                    if (usedIndices.find(edgeListSecondDisconnect[k]) != usedIndices.end())
                    {
                        continue;
                    }
                    BrushFloat distance = vertices[edges[edgeListFirstDisconnect[i]].m_i[0]].pos.GetDistance(vertices[edges[edgeListSecondDisconnect[k]].m_i[1]].pos);
                    if (distance < theLeastDistance)
                    {
                        theLeastDistance = distance;
                        theLeastIndex = k;
                    }
                }
                if (theLeastIndex != -1)
                {
                    edges[edgeListFirstDisconnect[i]].m_i[0] = edges[edgeListSecondDisconnect[theLeastIndex]].m_i[1];
                    usedIndices.insert(edgeListFirstDisconnect[i]);
                    usedIndices.insert(edgeListSecondDisconnect[theLeastIndex]);
                }
            }
        }
    }

    void Polygon::RemoveUnconnectedEdges(std::vector<SEdge>& edges) const
    {
        bool bKeepProcessing = true;
        while (bKeepProcessing)
        {
            std::set<SEdge> removedEdges;
            for (int i = 0, iEdgeCount(edges.size()); i < iEdgeCount; ++i)
            {
                int nCountWithConnectionFromBeginningToEnd = 0;
                int nCountWithConnectionFromEndToBeginning = 0;
                for (int k = 0; k < iEdgeCount; ++k)
                {
                    if (i == k || edges[i].m_i[0] == edges[k].m_i[1] && edges[i].m_i[1] == edges[k].m_i[0])
                    {
                        continue;
                    }
                    if (edges[i].m_i[0] == edges[k].m_i[1])
                    {
                        ++nCountWithConnectionFromBeginningToEnd;
                    }
                    if (edges[i].m_i[1] == edges[k].m_i[0])
                    {
                        ++nCountWithConnectionFromEndToBeginning;
                    }
                }
                if (nCountWithConnectionFromBeginningToEnd == 0 || nCountWithConnectionFromEndToBeginning == 0)
                {
                    removedEdges.insert(edges[i]);
                }
            }
            std::vector<SEdge>::iterator iEdge = edges.begin();
            for (; iEdge != edges.end(); )
            {
                if (removedEdges.find(*iEdge) != removedEdges.end())
                {
                    iEdge  = edges.erase(iEdge);
                }
                else
                {
                    ++iEdge;
                }
            }
            bKeepProcessing = removedEdges.empty() ? false : true;
        }
    }

    bool Polygon::Union(PolygonPtr BPolygon)
    {
        if (!BPolygon)
        {
            return false;
        }

        if (IsOpen() && BPolygon->IsOpen())
        {
            bool bReverseOrderingEdge = ShouldOrderReverse(BPolygon);
            for (int i = 0, iEdgeSize(BPolygon->GetEdgeCount()); i < iEdgeSize; ++i)
            {
                BrushEdge3D edge = BPolygon->GetEdge(i);
                if (bReverseOrderingEdge)
                {
                    edge.Invert();
                }
                AddEdge(edge);
            }

            Optimize();

            return true;
        }
        else if (IsOpen() || BPolygon->IsOpen())
        {
            return false;
        }

        if (!IsValid())
        {
            SetVertexList_Basic(BPolygon->m_Vertices);
            SetEdgeList_Basic(BPolygon->m_Edges);
            m_Plane = BPolygon->GetPlane();
            UpdateBoundBox();
        }
        else
        {
            if (!IsPlaneEquivalent(BPolygon))
            {
                return false;
            }

            if (!GetBSPTree() || !BPolygon->GetBSPTree())
            {
                return false;
            }

            std::vector<SVertex> vertices;
            std::vector<SEdge> edges;

            Clip(BPolygon->GetBSPTree(), eCT_Negative, vertices, edges, eCO_Union0, 0);
            BPolygon->Clip(GetBSPTree(), eCT_Negative, vertices, edges, eCO_Union1, 0);

            ConnectNearVertices(vertices, edges);
            RemoveUnconnectedEdges(edges);

            if (Optimize(vertices, edges) == false)
            {
                return false;
            }
        }
        return true;
    }

    bool Polygon::Intersect(PolygonPtr BPolygon, uint8 includeCoEdgeFlags)
    {
        if (!IsValid() || !BPolygon)
        {
            return false;
        }

        if (IsOpen() || BPolygon->IsOpen())
        {
            return false;
        }

        if (!GetBSPTree() || !BPolygon->GetBSPTree())
        {
            return false;
        }

        if (!IsPlaneEquivalent(BPolygon))
        {
            return false;
        }

        std::vector<SVertex> vertices;
        std::vector<SEdge> edges;

        if (includeCoEdgeFlags & eICEII_IncludeCoSame)
        {
            Clip(BPolygon->GetBSPTree(), eCT_Positive, vertices, edges, eCO_Intersection0IncludingCoSame, 0);
        }
        else
        {
            Clip(BPolygon->GetBSPTree(), eCT_Positive, vertices, edges, eCO_Intersection0, 0);
        }

        if (includeCoEdgeFlags & eICEII_IncludeCoDiff)
        {
            BPolygon->Clip(GetBSPTree(), eCT_Positive, vertices, edges, eCO_Intersection1IncludingCoDiff, 0);
        }
        else
        {
            BPolygon->Clip(GetBSPTree(), eCT_Positive, vertices, edges, eCO_Intersection1, 0);
        }

        if (IsOpen(vertices, edges))
        {
            SetEdgeList_Basic(edges);
            DeleteAllVertices_Basic();
            for (int i = 0, iVertexSize(vertices.size()); i < iVertexSize; ++i)
            {
                AddVertex_Basic(vertices[i]);
            }
        }

        if (!Optimize(vertices, edges))
        {
            return false;
        }

        return true;
    }

    bool Polygon::Subtract(PolygonPtr BPolygon)
    {
        if (!IsValid() || !BPolygon)
        {
            return false;
        }

        if (IsOpen() || BPolygon->IsOpen())
        {
            return false;
        }

        if (!GetBSPTree() || !BPolygon->GetBSPTree())
        {
            return false;
        }

        if (!IsPlaneEquivalent(BPolygon))
        {
            return false;
        }

        if (BPolygon->Include(this))
        {
            DeleteAllVertices_Basic();
            DeleteAllEdges_Basic();
            return true;
        }

        std::vector<SVertex> vertices;
        std::vector<SEdge> edges;

        Clip(BPolygon->GetBSPTree(), eCT_Negative, vertices, edges, eCO_Subtract, 0);
        BPolygon->Clip(GetBSPTree(), eCT_Positive, vertices, edges, eCO_Subtract, 0);

        ConnectNearVertices(vertices, edges);
        RemoveUnconnectedEdges(edges);

        if (!Optimize(vertices, edges))
        {
            return false;
        }

        return true;
    }

    bool Polygon::ExclusiveOR(PolygonPtr BPolygon)
    {
        if (!IsValid() || !BPolygon)
        {
            return false;
        }

        if (IsOpen() || BPolygon->IsOpen())
        {
            return false;
        }

        if (!GetBSPTree() || !BPolygon->GetBSPTree())
        {
            return false;
        }

        if (!IsPlaneEquivalent(BPolygon))
        {
            return false;
        }

        std::vector<SVertex> vertices;
        std::vector<SEdge> edges;

        //A-B
        Clip(BPolygon->GetBSPTree(), eCT_Negative, vertices, edges, eCO_Subtract, 0);
        BPolygon->Clip(GetBSPTree(), eCT_Positive, vertices, edges, eCO_Subtract, 0);

        //B-A
        BPolygon->Clip(GetBSPTree(), eCT_Negative, vertices, edges, eCO_Subtract, 1);
        Clip(BPolygon->GetBSPTree(), eCT_Positive, vertices, edges, eCO_Subtract, 1);

        ConnectNearVertices(vertices, edges);
        RemoveUnconnectedEdges(edges);

        if (!Optimize(vertices, edges))
        {
            return false;
        }

        return true;
    }

    bool Polygon::ClipInside(PolygonPtr BPolygon)
    {
        if (!IsValid() || !BPolygon)
        {
            return false;
        }

        if (IsOpen() || BPolygon->IsOpen())
        {
            return false;
        }

        if (!BPolygon->GetBSPTree())
        {
            return false;
        }

        if (!IsPlaneEquivalent(BPolygon))
        {
            return false;
        }

        std::vector<SVertex> vertices;
        std::vector<SEdge> edges;
        Clip(BPolygon->GetBSPTree(), eCT_Negative, vertices, edges, eCO_JustClip, 0);

        return Optimize(vertices, edges);
    }

    bool Polygon::ClipOutside(PolygonPtr BPolygon)
    {
        if (!IsValid() || !BPolygon)
        {
            return false;
        }

        if (IsOpen() || BPolygon->IsOpen())
        {
            return false;
        }

        if (!BPolygon->GetBSPTree())
        {
            return false;
        }

        if (!IsPlaneEquivalent(BPolygon))
        {
            return false;
        }

        std::vector<SVertex> vertices;
        std::vector<SEdge> edges;
        Clip(BPolygon->GetBSPTree(), eCT_Positive, vertices, edges, eCO_JustClip, 0);

        return Optimize(vertices, edges);
    }

    void Polygon::ReverseEdges()
    {
        std::vector<SEdge> reverseOrder;
        reverseOrder.resize(m_Edges.size());
        for (int i = 0, iEdgeSize(m_Edges.size()); i < iEdgeSize; ++i)
        {
            SwapEdgeIndex_Basic(i);
            reverseOrder[iEdgeSize - i - 1] = m_Edges[i];
        }
        SetEdgeList_Basic(reverseOrder);
    }

    void Polygon::UpdateBoundBox() const
    {
        if (m_pBoundBox)
        {
            return;
        }

        m_pBoundBox = new SPolygonBound;
        m_pBoundBox->aabb.Reset();

        for (int i = 0, iVertexSize(m_Vertices.size()); i < iVertexSize; ++i)
        {
            m_pBoundBox->aabb.Add(GetPos(i));
        }

        m_pBoundBox->raidus = 0;
        Vec3 vCenter = m_pBoundBox->aabb.GetCenter();
        for (int i = 0, iVertexSize(m_Vertices.size()); i < iVertexSize; ++i)
        {
            float fDistance = vCenter.GetDistance(GetPos(i));
            if (m_pBoundBox->raidus < fDistance)
            {
                m_pBoundBox->raidus = fDistance;
            }
        }
    }

    const AABB& Polygon::GetBoundBox() const
    {
        if (!m_pBoundBox)
        {
            UpdateBoundBox();
        }
        return m_pBoundBox->aabb;
    }

    PolygonPtr Polygon::Flip()
    {
        m_Plane.Invert();
        ReverseEdges();
        return this;
    }

    bool Polygon::GetSeparatedPolygons(std::vector<PolygonPtr>& outSeparatedPolygons, int nSpratePolygonFlag, bool bOptimizePolygon) const
    {
        if (m_Edges.empty())
        {
            return false;
        }

        std::vector<PolygonPtr> outerPolygons;
        std::vector<PolygonPtr> innerPolygons;
        bool bErrorHappen(false);

        std::vector<EdgeList> outLoops;
        FindLoops(outLoops);

        for (int i = 0, nLoopCount(outLoops.size()); i < nLoopCount; ++i)
        {
            PolygonPtr pPolygon;
            if (CreateNewPolygonFromEdges(outLoops[i], pPolygon, bOptimizePolygon))
            {
                pPolygon->SetFlag(GetFlag());
                if (pPolygon->IsCCW())
                {
                    outerPolygons.push_back(pPolygon);
                }
                else
                {
                    innerPolygons.push_back(pPolygon);
                }
            }
        }

        if (nSpratePolygonFlag & eSP_Together)
        {
            for (int k = 0; k < outerPolygons.size(); k++)
            {
                Polygon oldOuterPolygon(*outerPolygons[k]);
                for (int i = 0; i < innerPolygons.size(); ++i)
                {
                    if (oldOuterPolygon.IncludeAllEdges(innerPolygons[i]))
                    {
                        outerPolygons[k]->Attach(innerPolygons[i]);
                    }
                }
            }
        }
        else if (nSpratePolygonFlag & eSP_InnerHull)
        {
            outerPolygons = innerPolygons;
        }

        outSeparatedPolygons = outerPolygons;

        return !outSeparatedPolygons.empty();
    }

    bool Polygon::Attach(PolygonPtr pPolygon)
    {
        if (!pPolygon)
        {
            return false;
        }

        int nBaseIndex = m_Vertices.size();

        std::map<int, int> vertexIndexMapper;
        for (int i = 0; i < pPolygon->m_Vertices.size(); ++i)
        {
            AddVertex_Basic(SVertex(pPolygon->GetPos(i)));
        }

        for (int i = 0; i < pPolygon->m_Edges.size(); ++i)
        {
            AddEdge_Basic(SEdge(pPolygon->m_Edges[i].m_i[0] + nBaseIndex, pPolygon->m_Edges[i].m_i[1] + nBaseIndex));
        }

        return true;
    }

    bool Polygon::Optimize(const std::vector<BrushEdge3D>& edges)
    {
        std::vector<SVertex> vertices;
        std::vector<SEdge> edgeIndices;
        for (int i = 0, iEdgeSize(edges.size()); i < iEdgeSize; ++i)
        {
            edgeIndices.push_back(SEdge(AddVertex(vertices, SVertex(edges[i].m_v[0], edges[i].m_data[0])), AddVertex(vertices, SVertex(edges[i].m_v[1], edges[i].m_data[1]))));
        }
        return Optimize(vertices, edgeIndices);
    }

    void Polygon::Optimize()
    {
        std::vector<SEdge> replicatedEdges;
        CopyEdges(m_Edges, replicatedEdges);
        Optimize(m_Vertices, replicatedEdges);
    }

    bool Polygon::Optimize(std::vector<SVertex>& vertices, std::vector<SEdge>& edges)
    {
        std::vector<BrushFloat> xyzList[3];
        for (int i = 0, iVertexCount(vertices.size()); i < iVertexCount; ++i)
        {
            for (int a = 0; a < 3; ++a)
            {
                bool bAdjusted = false;
                for (int k = 0, iXYZListCount(xyzList[a].size()); k < iXYZListCount; ++k)
                {
                    if (vertices[i].pos[a] != xyzList[a][k] && std::abs(vertices[i].pos[a] - xyzList[a][k]) < kDesignerEpsilon)
                    {
                        vertices[i].pos[a] = xyzList[a][k];
                        bAdjusted = true;
                        break;
                    }
                }
                if (!bAdjusted)
                {
                    xyzList[a].push_back(vertices[i].pos[a]);
                }
            }
        }

        bool bSuccessOptimizeEdge = OptimizeEdges(vertices, edges);
        if (bSuccessOptimizeEdge)
        {
            OptimizeVertices(vertices, edges);
        }
        else
        {
            DESIGNER_ASSERT(bSuccessOptimizeEdge || edges.size() > 0);
            return false;
        }

        for (int i = 0, iCount(vertices.size()); i < iCount; ++i)
        {
            vertices[i].id = 0;
        }

        SetEdgeList_Basic(edges);
        SetVertexList_Basic(vertices);

        if (!m_Edges.empty() || !m_Vertices.empty())
        {
            UpdateBoundBox();
        }

        if (m_Vertices.size() != 4 || m_Edges.size() != 4)
        {
            RemoveFlags(ePolyFlag_NonplanarQuad);
        }

        return true;
    }

    void Polygon::UpdatePrivateFlags() const
    {
        if (m_PrivateFlag != eRPF_Invalid)
        {
            return;
        }

        m_PrivateFlag = 0;

        if (m_Vertices.empty() || m_Edges.empty())
        {
            return;
        }

        if (!IsOpen(m_Vertices, m_Edges))
        {
            std::vector<PolygonPtr> innerPolygons;
            GetSeparatedPolygons(innerPolygons, eSP_InnerHull);
            bool bHasHoles = !innerPolygons.empty();
            if (bHasHoles)
            {
                AddPrivateFlags(eRPF_HasHoles);
            }

            if (GetEdgeCount() == 3)
            {
                AddPrivateFlags(eRPF_Convex);
            }
            else if (!HasHoles())
            {
                std::vector<SVertex> polygon;
                if (GetLinkedVertices(polygon))
                {
                    AddPrivateFlags(eRPF_Convex);
                    for (int i = 0, iPolygonSize(polygon.size()); i < iPolygonSize; ++i)
                    {
                        int nNextI = (i + 1) % iPolygonSize;
                        int nDoubleNextI = (i + 2) % iPolygonSize;
                        const BrushVec3& v0 = polygon[i].pos;
                        const BrushVec3& v1 = polygon[nNextI].pos;
                        const BrushVec3& v2 = polygon[nDoubleNextI].pos;
                        BrushVec3 vNormal = (v2 - v1) ^ (v0 - v1);
                        if (!m_Plane.IsSameFacing(vNormal))
                        {
                            RemovePrivateFlags(eRPF_Convex);
                            break;
                        }
                    }
                }
            }
        }
        else
        {
            AddPrivateFlags(eRPF_Open);
        }
    }

    bool Polygon::ExtractEdge3DList(std::vector<BrushEdge3D>& outList) const
    {
        for (int i = 0, iSize(GetEdgeCount()); i < iSize; ++i)
        {
            BrushEdge3D edge = GetEdge(i);
            if (edge.IsPoint())
            {
                return false;
            }

            for (int a = 0; a < 3; ++a)
            {
                if (edge.m_v[0][a] != edge.m_v[1][a] && std::abs(edge.m_v[0][a] - edge.m_v[1][a]) < kDesignerEpsilon)
                {
                    edge.m_v[1][a] = edge.m_v[0][a];
                }
            }

            outList.push_back(edge);
        }
        return true;
    }

    bool Polygon::BuildBSP() const
    {
        DESIGNER_ASSERT(!m_pBSPTree);
        if (m_pBSPTree)
        {
            return false;
        }
        if (!IsValid())
        {
            return false;
        }
        if (IsOpen(m_Vertices, m_Edges))
        {
            return false;
        }

        std::vector<BrushEdge3D> edgeList;
        if (!ExtractEdge3DList(edgeList))
        {
            return false;
        }

        m_pBSPTree = new BSPTree2D();
        m_pBSPTree->AddRef();

        m_pBSPTree->BuildTree(GetPlane(), edgeList);

        return true;
    }

    void Polygon::ModifyOrientation()
    {
        if (!GetBSPTree())
        {
            return;
        }

        if (!IsCCW())
        {
            for (int i = 0, iEdgeSize(m_Edges.size()); i < iEdgeSize; ++i)
            {
                SwapEdgeIndex_Basic(i);
            }
        }
    }

    void Polygon::Clip(const BSPTree2D* pTree, EClipType cliptype, std::vector<SVertex>& vertices, std::vector<SEdge>& edges, EClipObjective clipObjective, unsigned char vertexID) const
    {
        if (!IsValid() || pTree == NULL)
        {
            return;
        }

        for (int i = 0, iEdgeSize(m_Edges.size()); i < iEdgeSize; ++i)
        {
            BrushEdge3D edge(GetPos(m_Edges[i].m_i[0]), GetPos(m_Edges[i].m_i[1]));
            edge.m_data[0] = GetUV(m_Edges[i].m_i[0]);
            edge.m_data[1] = GetUV(m_Edges[i].m_i[1]);

            BSPTree2D::SOutputEdges outEdges;
            pTree->GetPartitions(edge, outEdges);

            std::vector<BrushEdge3D::List> validEdges;

            if (clipObjective == eCO_JustClip)
            {
                if (cliptype == eCT_Negative)
                {
                    validEdges.push_back(outEdges.posList);
                    validEdges.push_back(outEdges.coSameList);
                }
                else
                {
                    validEdges.push_back(outEdges.negList);
                    validEdges.push_back(outEdges.coDiffList);
                }
            }
            if (clipObjective == eCO_Union0)
            {
                if (cliptype == eCT_Negative)
                {
                    validEdges.push_back(outEdges.posList);
                    validEdges.push_back(outEdges.coSameList);
                }
            }
            else if (clipObjective == eCO_Union1)
            {
                if (cliptype == eCT_Negative)
                {
                    validEdges.push_back(outEdges.posList);
                }
            }
            else if (clipObjective == eCO_Intersection0 || clipObjective == eCO_Intersection0IncludingCoSame)
            {
                if (cliptype == eCT_Positive)
                {
                    validEdges.push_back(outEdges.negList);
                    if (clipObjective == eCO_Intersection0IncludingCoSame)
                    {
                        validEdges.push_back(outEdges.coSameList);
                    }
                }
            }
            else if (clipObjective == eCO_Intersection1 || clipObjective == eCO_Intersection1IncludingCoDiff)
            {
                if (cliptype == eCT_Positive)
                {
                    validEdges.push_back(outEdges.negList);
                    if (clipObjective == eCO_Intersection1IncludingCoDiff)
                    {
                        validEdges.push_back(outEdges.coDiffList);
                    }
                }
            }
            else if (clipObjective == eCO_Subtract)
            {
                if (cliptype == eCT_Positive)
                {
                    for (int a = 0, iCount(outEdges.negList.size()); a < iCount; ++a)
                    {
                        outEdges.negList[a].Invert();
                    }
                    validEdges.push_back(outEdges.negList);
                }
                else if (cliptype == eCT_Negative)
                {
                    validEdges.push_back(outEdges.posList);
                    validEdges.push_back(outEdges.coDiffList);
                }
            }

            for (int a = 0; a < validEdges.size(); ++a)
            {
                for (int k = 0, iPosEdgeSize(validEdges[a].size()); k < iPosEdgeSize; k++)
                {
                    int index0(AddVertex(vertices, SVertex(validEdges[a][k].m_v[0], validEdges[a][k].m_data[0], vertexID)));
                    int index1(AddVertex(vertices, SVertex(validEdges[a][k].m_v[1], validEdges[a][k].m_data[1], vertexID)));

                    if (index0 == index1)
                    {
                        continue;
                    }

                    SEdge e(index0, index1);
                    if (DoesIdenticalEdgeExist(edges, e))
                    {
                        continue;
                    }

                    edges.push_back(e);
                }
            }
        }
    }

    int Polygon::AddVertex(std::vector<SVertex>& vertices, const SVertex& newVertex) const
    {
        for (int i = 0, vSize(vertices.size()); i < vSize; ++i)
        {
            if (CD::IsEquivalent(newVertex.pos, vertices[i].pos) && newVertex.id == vertices[i].id)
            {
                return i;
            }
        }
        vertices.push_back(newVertex);
        return vertices.size() - 1;
    }

    void Polygon::Transform2Vertices(const std::vector<BrushVec2>& points, const BrushPlane& plane)
    {
        int iPtSize(points.size());
        DeleteAllVertices_Basic();
        m_Vertices.reserve(iPtSize);
        for (int i = 0; i < iPtSize; ++i)
        {
            AddVertex_Basic(SVertex(plane.P2W(points[i])));
        }
    }

    bool Polygon::GetAdjacentPrevEdgeIndicesWithVertexIndex(int vertexIndex, std::vector<int>& outEdgeIndices, const std::vector<SEdge>& edges) const
    {
        for (int i = 0, iEdgeSize(edges.size()); i < iEdgeSize; ++i)
        {
            const SEdge& edge(edges[i]);
            if (edge.m_i[1] == vertexIndex)
            {
                outEdgeIndices.push_back(i);
            }
        }

        return !outEdgeIndices.empty();
    }

    bool Polygon::GetAdjacentNextEdgeIndicesWithVertexIndex(int vertexIndex, std::vector<int>& outEdgeIndices, const std::vector<SEdge>& edges) const
    {
        for (int i = 0, iEdgeSize(edges.size()); i < iEdgeSize; ++i)
        {
            const SEdge& edge(edges[i]);
            if (edge.m_i[0] == vertexIndex)
            {
                outEdgeIndices.push_back(i);
            }
        }

        return !outEdgeIndices.empty();
    }

    bool Polygon::GetAdjacentEdgeIndexWithEdgeIndex(int edgeIndex, int& outPrevEdgeIndex, int& outNextEdgeIndex, const std::vector<SVertex>& vertices, const std::vector<SEdge>& edges) const
    {
        outPrevEdgeIndex = -1;
        outNextEdgeIndex = -1;

        const SEdge& edge(edges[edgeIndex]);

        std::vector<int> edgesPreviousFirstVertex;
        if (GetAdjacentPrevEdgeIndicesWithVertexIndex(edge.m_i[0], edgesPreviousFirstVertex, edges))
        {
            std::vector<int>::iterator ii = edgesPreviousFirstVertex.begin();
            for (; ii != edgesPreviousFirstVertex.end(); ++ii)
            {
                if (edge.m_i[1] == edges[*ii].m_i[0] && edge.m_i[0] == edges[*ii].m_i[1])
                {
                    edgesPreviousFirstVertex.erase(ii);
                    break;
                }
            }
            if (edgesPreviousFirstVertex.size() == 1)
            {
                outPrevEdgeIndex = *edgesPreviousFirstVertex.begin();
            }
            else if (edgesPreviousFirstVertex.size() > 1)
            {
                EdgeIndexSet candidateSecondIndices;
                for (int i = 0, iEdgeCount(edgesPreviousFirstVertex.size()); i < iEdgeCount; ++i)
                {
                    candidateSecondIndices.insert(edges[edgesPreviousFirstVertex[i]].m_i[0]);
                }
                SEdge outEdge;
                ChoosePrevEdge(vertices, edge, candidateSecondIndices, outEdge);
                GetEdgeIndex(edges, outEdge.m_i[0], outEdge.m_i[1], outPrevEdgeIndex);
            }
        }

        std::vector<int> edgesNextSecondVertex;
        if (GetAdjacentNextEdgeIndicesWithVertexIndex(edge.m_i[1], edgesNextSecondVertex, edges))
        {
            std::vector<int>::iterator ii = edgesNextSecondVertex.begin();
            for (; ii != edgesNextSecondVertex.end(); ++ii)
            {
                if (edge.m_i[1] == edges[*ii].m_i[0] && edge.m_i[0] == edges[*ii].m_i[1])
                {
                    edgesNextSecondVertex.erase(ii);
                    break;
                }
            }

            if (edgesNextSecondVertex.size() == 1)
            {
                outNextEdgeIndex = *edgesNextSecondVertex.begin();
            }
            else if (edgesNextSecondVertex.size() > 1)
            {
                EdgeIndexSet candidateSecondIndices;
                for (int i = 0, iEdgeCount(edgesNextSecondVertex.size()); i < iEdgeCount; ++i)
                {
                    candidateSecondIndices.insert(edges[edgesNextSecondVertex[i]].m_i[1]);
                }
                SEdge outEdge;
                ChooseNextEdge(vertices, edge, candidateSecondIndices, outEdge);
                GetEdgeIndex(edges, outEdge.m_i[0], outEdge.m_i[1], outNextEdgeIndex);
            }
        }

        return outPrevEdgeIndex != -1 || outNextEdgeIndex != -1;
    }

    bool Polygon::GetLinkedVertices(std::vector<SVertex>& outVertexList, const std::vector<SVertex>& vertices, const std::vector<SEdge>& edges) const
    {
        SEdge edge(edges[0]);
        outVertexList.clear();
        int counter(0);

        if (IsOpen(vertices, edges))
        {
            // In a case of edges not being closed,
            // the starting edge which first index is connected to nothing should be found.
            for (int i = 0, nEdgeSize(edges.size()); i < nEdgeSize; ++i)
            {
                bool bExist(false);
                for (int k = 0; k < nEdgeSize; ++k)
                {
                    if (i == k)
                    {
                        continue;
                    }
                    if (edges[i].m_i[0] == edges[k].m_i[1])
                    {
                        bExist = true;
                        break;
                    }
                }
                if (!bExist)
                {
                    edge = edges[i];
                    break;
                }
            }
        }
        else if (!m_Plane.Normal().IsEquivalent(BrushVec3(0, 0, 0), 0))
        {
            BrushFloat smallestX = 3e10;
            int nSmallestVIndex = -1;
            for (int i = 0, iVertexCount(m_Vertices.size()); i < iVertexCount; ++i)
            {
                BrushVec2 v2D = m_Plane.W2P(m_Vertices[i].pos);
                if (v2D.x < smallestX)
                {
                    smallestX = v2D.x;
                    nSmallestVIndex = i;
                }
            }
            DESIGNER_ASSERT(nSmallestVIndex != -1);
            for (int i = 0, iEdgeCount(m_Edges.size()); i < iEdgeCount; ++i)
            {
                if (m_Edges[i].m_i[0] == nSmallestVIndex || m_Edges[i].m_i[1] == nSmallestVIndex)
                {
                    edge = m_Edges[i];
                    break;
                }
            }
        }

        SEdge entryEdge(edge);

        while (outVertexList.size() < edges.size() && (counter++) < edges.size())
        {
            std::vector<int> linkedNextEdges;
            GetAdjacentNextEdgeIndicesWithVertexIndex(edge.m_i[1], linkedNextEdges, edges);
            if (linkedNextEdges.empty())
            {
                if (edge.m_i[0] == edge.m_i[1])
                {
                    return false;
                }
                outVertexList.push_back(vertices[edge.m_i[0]]);
                outVertexList.push_back(vertices[edge.m_i[1]]);
                return true;
            }
            outVertexList.push_back(vertices[edge.m_i[0]]);

            if (linkedNextEdges.size() == 1)
            {
                edge = edges[*linkedNextEdges.begin()];
            }
            else if (linkedNextEdges.size() > 1)
            {
                EdgeIndexSet candidateSecondIndices;
                for (int i = 0, iEdgeCount(linkedNextEdges.size()); i < iEdgeCount; ++i)
                {
                    candidateSecondIndices.insert(edges[linkedNextEdges[i]].m_i[1]);
                }
                ChooseNextEdge(vertices, edge, candidateSecondIndices, edge);
            }

            if (edge == entryEdge)
            {
                return true;
            }
        }

        return outVertexList.size() == edges.size();
    }

    bool Polygon::OptimizeEdges(std::vector<SVertex>& vertices, std::vector<SEdge>& edges) const
    {
        RemoveEdgesHavingSameIndices(edges);
        RemoveEdgesRegardedAsVertex(vertices, edges);
        return FlattenEdges(vertices, edges);
    }

    bool Polygon::FlattenEdges(std::vector<SVertex>& vertices, std::vector<SEdge>& edges) const
    {
        std::set<int> usedEdges;
        std::vector<SEdge> newEdges;
        newEdges.reserve(edges.size());
        for (int i = 0, iEdgeSize(edges.size()); i < iEdgeSize; ++i)
        {
            if (usedEdges.find(i) != usedEdges.end())
            {
                continue;
            }

            std::vector<int> edgeLists[2];
            SearchLinkedColinearEdges(i, eSD_Previous, vertices, edges, edgeLists[eSD_Previous]);
            SearchLinkedColinearEdges(i, eSD_Next, vertices, edges, edgeLists[eSD_Next]);

            int edgeIndexInEnds[2] = {i, i};
            for (int direction = 0; direction < 2; ++direction)
            {
                if (edgeLists[direction].empty())
                {
                    continue;
                }
                int iEdgeListCount = edgeLists[direction].size();
                for (int k = 0; k < iEdgeListCount; ++k)
                {
                    int edgeIndex(edgeLists[direction][k]);
                    if (usedEdges.find(edgeIndex) != usedEdges.end())
                    {
                        DESIGNER_ASSERT(0);
                        return false;
                    }
                    usedEdges.insert(edgeIndex);
                }
                edgeIndexInEnds[direction] = edgeLists[direction][iEdgeListCount - 1];
            }
            SEdge edge;
            if (edgeIndexInEnds[eSD_Previous] == i && edgeIndexInEnds[eSD_Next] == i)
            {
                edge = edges[i];
            }
            else
            {
                edge = SEdge(
                        edges[edgeIndexInEnds[eSD_Previous]].m_i[0],
                        edges[edgeIndexInEnds[eSD_Next]].m_i[1]
                        );
            }
            newEdges.push_back(edge);
        }
        edges = newEdges;
        return true;
    }

    void Polygon::RemoveEdgesRegardedAsVertex(std::vector<SVertex>& vertices, std::vector<SEdge>& edges) const
    {
        int iVertexSize(vertices.size());
        std::vector<SEdge>::iterator ie = edges.begin();
        for (; ie != edges.end(); )
        {
            SEdge edge = *ie;
            if (edge.m_i[0] >= iVertexSize || edge.m_i[1] >= iVertexSize)
            {
                ++ie;
                continue;
            }
            const SVertex& p0(vertices[edge.m_i[0]]);
            const SVertex& p1(vertices[edge.m_i[1]]);
            if (CD::IsEquivalent(p0.pos, p1.pos))
            {
                ie = edges.erase(ie);
                int nNewIndex = vertices.size();
                vertices.push_back(p0);
                for (int i = 0, iEdgeSize(edges.size()); i < iEdgeSize; ++i)
                {
                    if (edges[i].m_i[0] == edge.m_i[1])
                    {
                        edges[i].m_i[0] = nNewIndex;
                    }
                    else if (edges[i].m_i[1] == edge.m_i[0])
                    {
                        edges[i].m_i[1] = nNewIndex;
                    }
                }
            }
            else
            {
                ++ie;
            }
        }
    }


    void Polygon::RemoveEdgesHavingSameIndices(std::vector<SEdge>& edges)
    {
        std::set<int> removedEdgeIndices;
        for (int i = 0, iEdgeSize(edges.size()); i < iEdgeSize; ++i)
        {
            if (removedEdgeIndices.find(i) != removedEdgeIndices.end())
            {
                continue;
            }

            const SEdge& edge0(edges[i]);
            if (edge0.m_i[0] == edge0.m_i[1])
            {
                removedEdgeIndices.insert(i);
            }

            for (int k = i + 1; k < iEdgeSize; ++k)
            {
                if (removedEdgeIndices.find(k) != removedEdgeIndices.end())
                {
                    continue;
                }
                const SEdge& edge1(edges[k]);
                if (edge0.m_i[0] == edge1.m_i[0] && edge0.m_i[1] == edge1.m_i[1])
                {
                    removedEdgeIndices.insert(k);
                }
            }
        }
        std::vector<SEdge>::iterator ii = edges.begin();
        for (int counter = 0; ii != edges.end(); ++counter)
        {
            if (removedEdgeIndices.find(counter) != removedEdgeIndices.end())
            {
                ii = edges.erase(ii);
            }
            else
            {
                ++ii;
            }
        }
    }

    void Polygon::SearchLinkedColinearEdges(int edgeIndex, ESearchDirection direction, const std::vector<SVertex>& vertices, const std::vector<SEdge>& edges, std::vector<int>& outLinkedEdges) const
    {
        int outEdgeIndices[2];
        BrushLine edgeLine(GetLineFromEdge(edgeIndex, vertices, edges));
        int counter(0);
        int edgeSize(edges.size());

        while ((counter++) < edgeSize)
        {
            if (!GetAdjacentEdgeIndexWithEdgeIndex(edgeIndex, outEdgeIndices[eSD_Previous], outEdgeIndices[eSD_Next], vertices, edges))
            {
                break;
            }
            if (outEdgeIndices[direction] == -1)
            {
                break;
            }
            BrushLine adjacentEdgeLine(GetLineFromEdge(outEdgeIndices[direction], vertices, edges));
            if (!edgeLine.IsEquivalent(adjacentEdgeLine))
            {
                break;
            }
            edgeIndex = outEdgeIndices[direction];
            outLinkedEdges.push_back(edgeIndex);
        }
    }

    void Polygon::SearchLinkedEdges(int edgeIndex, ESearchDirection direction, const std::vector<SVertex>& vertices, const std::vector<SEdge>& edges, std::vector<int>& outLinkedEdges) const
    {
        int edgeIndices[2];
        int counter(0);
        int edgeSize(edges.size());

        while ((counter++) < edgeSize)
        {
            if (!GetAdjacentEdgeIndexWithEdgeIndex(edgeIndex, edgeIndices[eSD_Previous], edgeIndices[eSD_Next], vertices, edges))
            {
                break;
            }
            if (edgeIndices[direction] == -1 || edgeIndices[direction] == edgeIndex)
            {
                break;
            }
            edgeIndex = edgeIndices[direction];
            outLinkedEdges.push_back(edgeIndex);
        }
    }

    void Polygon::OptimizeVertices(std::vector<SVertex>& vertices, std::vector<SEdge>& edges) const
    {
        std::vector<SVertex> newVertices;
        std::map<int, int> vertexMapper;
        newVertices.reserve(vertices.size());
        for (int k = 0, vertexSize(vertices.size()); k < vertexSize; ++k)
        {
            for (int i = 0, edgeSize(edges.size()); i < edgeSize; ++i)
            {
                for (int a = 0; a < 2; ++a)
                {
                    if (edges[i].m_i[a] == k)
                    {
                        if (vertexMapper.find(k) == vertexMapper.end())
                        {
                            int newIndex = newVertices.size();
                            vertexMapper[k] = newIndex;
                            newVertices.push_back(vertices[k]);
                            edges[i].m_i[a] = newIndex;
                        }
                        else
                        {
                            edges[i].m_i[a] = vertexMapper[edges[i].m_i[a]];
                        }
                    }
                }
            }
        }
        vertices = newVertices;
    }

    bool Polygon::DoesIdenticalEdgeExist(const std::vector<SEdge>& edges, const SEdge& e, int* pOutIndex)
    {
        for (int i = 0, iEdgeSize(edges.size()); i < iEdgeSize; ++i)
        {
            if (edges[i].m_i[0] == e.m_i[0] && edges[i].m_i[1] == e.m_i[1])
            {
                if (pOutIndex)
                {
                    *pOutIndex = i;
                }
                return true;
            }
        }
        return false;
    }

    bool Polygon::DoesReverseEdgeExist(const std::vector<SEdge>& edges, const SEdge& e, int* pOutIndex)
    {
        for (int i = 0, iEdgeSize(edges.size()); i < iEdgeSize; ++i)
        {
            if (edges[i].m_i[1] == e.m_i[0] && edges[i].m_i[0] == e.m_i[1])
            {
                if (pOutIndex)
                {
                    *pOutIndex = i;
                }
                return true;
            }
        }
        return false;
    }

    BrushFloat Polygon::Cosine(int i0, int i1, int i2, const std::vector<SVertex>& vertices) const
    {
        BrushVec2 p0 = m_Plane.W2P(vertices[i0].pos);
        BrushVec2 p1 = m_Plane.W2P(vertices[i1].pos);
        BrushVec2 p2 = m_Plane.W2P(vertices[i2].pos);

        const BrushVec2 p10 = (p0 - p1).GetNormalized();
        const BrushVec2 p12 = (p2 - p1).GetNormalized();

        return p10.Dot(p12);
    }

    bool Polygon::IsCCW(int i0, int i1, int i2, const std::vector<SVertex>& vertices) const
    {
        const BrushVec3& v0 = m_Plane.W2P(vertices[i0].pos);
        const BrushVec3& v1 = m_Plane.W2P(vertices[i1].pos);
        const BrushVec3& v2 = m_Plane.W2P(vertices[i2].pos);

        BrushVec3 v10 = (v0 - v1).GetNormalized();
        BrushVec3 v12 = (v2 - v1).GetNormalized();

        BrushVec3 v10_x_v12 = v10.Cross(v12);

        return v10_x_v12.z > 0;
    }

    void Polygon::FindLoops(std::vector<EdgeList>& outLoopList) const
    {
        EdgeSet handledEdgeSet;
        EdgeSet edgeSet;
        EdgeMap edgeMapFrom1stTo2nd;
        EdgeMap edgeMapFrom2ndTo1st;

        for (int i = 0, iEdgeCount(m_Edges.size()); i < iEdgeCount; ++i)
        {
            edgeSet.insert(m_Edges[i]);

            EdgeMap::iterator iEdge = edgeMapFrom1stTo2nd.find(m_Edges[i].m_i[0]);
            edgeMapFrom1stTo2nd[m_Edges[i].m_i[0]].insert(m_Edges[i].m_i[1]);
            edgeMapFrom2ndTo1st[m_Edges[i].m_i[1]].insert(m_Edges[i].m_i[0]);
        }

        int nCounter(0);
        EdgeSet::iterator iEdgeSet = edgeSet.begin();
        for (iEdgeSet = edgeSet.begin(); iEdgeSet != edgeSet.end(); ++iEdgeSet)
        {
            if (edgeMapFrom1stTo2nd[(*iEdgeSet).m_i[0]].size() > 1 || edgeMapFrom2ndTo1st[(*iEdgeSet).m_i[1]].size() > 1)
            {
                continue;
            }

            if (handledEdgeSet.find(*iEdgeSet) != handledEdgeSet.end())
            {
                continue;
            }

            std::vector<int> subPiece;
            SEdge edge = *iEdgeSet;
            handledEdgeSet.insert(edge);

            do
            {
                if (edgeMapFrom1stTo2nd.find(edge.m_i[1]) != edgeMapFrom1stTo2nd.end())
                {
                    subPiece.push_back(edge.m_i[0]);
                }
                else
                {
                    subPiece.clear();
                    break;
                }

                const EdgeIndexSet& secondIndexSet = edgeMapFrom1stTo2nd[edge.m_i[1]];

                if (secondIndexSet.size() == 1)
                {
                    edge = SEdge(edge.m_i[1], *secondIndexSet.begin());
                }
                else if (secondIndexSet.size() > 1)
                {
                    ChooseNextEdge(m_Vertices, SEdge(edge.m_i[0], edge.m_i[1]), secondIndexSet, edge);
                }
                if (handledEdgeSet.find(edge) != handledEdgeSet.end())
                {
                    subPiece.clear();
                    break;
                }
                if (++nCounter > 10000)
                {
                    DESIGNER_ASSERT(0 && "Searching connected an edge doesn't seem possible.");
                    return;
                }
            } while (edge.m_i[1] != (*iEdgeSet).m_i[0]);

            if (!subPiece.empty())
            {
                EdgeList subLoop;
                subPiece.push_back(edge.m_i[0]);
                for (int i = 0, subPieceCount(subPiece.size()); i < subPieceCount; ++i)
                {
                    subLoop.push_back(SEdge(subPiece[i], subPiece[(i + 1) % subPieceCount]));
                    handledEdgeSet.insert(SEdge(subPiece[i], subPiece[(i + 1) % subPieceCount]));
                }
                outLoopList.push_back(subLoop);
            }
        }
    }

    void Polygon::ChoosePrevEdge(const std::vector<SVertex>& vertices, const SEdge& edge, const EdgeIndexSet& candidateSecondIndices, SEdge& outEdge) const
    {
        BrushFloat cwCosMax = -1.5f;
        BrushFloat ccwCosMin = 1.5f;
        int ccwIndex = -1;
        int cwIndex = -1;
        EdgeIndexSet::iterator iSecondIndexSet = candidateSecondIndices.begin();
        for (; iSecondIndexSet != candidateSecondIndices.end(); ++iSecondIndexSet)
        {
            if (*iSecondIndexSet == edge.m_i[1])
            {
                continue;
            }
            BrushFloat cosine = Cosine(edge.m_i[1], edge.m_i[0], *iSecondIndexSet, vertices);
            if (IsCW(edge.m_i[1], edge.m_i[0], *iSecondIndexSet, vertices))
            {
                if (cosine > cwCosMax)
                {
                    cwIndex = *iSecondIndexSet;
                    cwCosMax = cosine;
                }
            }
            else if (cosine < ccwCosMin)
            {
                ccwIndex = *iSecondIndexSet;
                ccwCosMin = cosine;
            }
        }

        if (cwIndex != -1)
        {
            outEdge = SEdge(cwIndex, edge.m_i[0]);
        }
        else
        {
            outEdge = SEdge(ccwIndex, edge.m_i[0]);
        }
    }

    void Polygon::ChooseNextEdge(const std::vector<SVertex>& vertices, const SEdge& edge, const EdgeIndexSet& candidateSecondIndices, SEdge& outEdge) const
    {
        BrushFloat ccwCosMax = -1.5f;
        BrushFloat cwCosMin = 1.5f;
        int ccwIndex = -1;
        int cwIndex = -1;
        EdgeIndexSet::iterator iSecondIndexSet = candidateSecondIndices.begin();
        for (; iSecondIndexSet != candidateSecondIndices.end(); ++iSecondIndexSet)
        {
            if (*iSecondIndexSet == edge.m_i[0])
            {
                continue;
            }
            BrushFloat cosine = Cosine(edge.m_i[0], edge.m_i[1], *iSecondIndexSet, vertices);
            if (IsCCW(edge.m_i[0], edge.m_i[1], *iSecondIndexSet, vertices))
            {
                if (cosine > ccwCosMax)
                {
                    ccwIndex = *iSecondIndexSet;
                    ccwCosMax = cosine;
                }
            }
            else if (cosine < cwCosMin)
            {
                cwIndex = *iSecondIndexSet;
                cwCosMin = cosine;
            }
        }

        if (ccwIndex != -1)
        {
            outEdge = SEdge(edge.m_i[1], ccwIndex);
        }
        else
        {
            outEdge = SEdge(edge.m_i[1], cwIndex);
        }
    }

    bool Polygon::CreateNewPolygonFromEdges(const std::vector<SEdge>& inputEdges, PolygonPtr& outPolygon, bool bOptimizePolygon) const
    {
        std::vector<SVertex> vertices;
        std::vector<SEdge> edges;

        for (int i = 0, iSize(inputEdges.size()); i < iSize; ++i)
        {
            const SEdge& edge(inputEdges[i]);
            int edgeIndex0(AddVertex(vertices, GetVertex(edge.m_i[0])));
            int edgeIndex1(AddVertex(vertices, GetVertex(edge.m_i[1])));

            if (edgeIndex0 != edgeIndex1)
            {
                edges.push_back(SEdge(edgeIndex0, edgeIndex1));
            }
        }

        if (vertices.size() >= 3 && edges.size() >= 3)
        {
            outPolygon = new Polygon(vertices, edges, GetPlane(), m_MaterialID, &m_TexInfo, bOptimizePolygon);
            return true;
        }

        outPolygon = NULL;
        return false;
    }

    void Polygon::SaveBinary(std::vector<char>& buffer)
    {
        int nVertexCount = m_Vertices.size();
        int nEdgeCount = m_Edges.size();

        buffer.reserve(nVertexCount * sizeof(BrushVec3) + nEdgeCount * sizeof(nEdgeCount) + 100);

        Write2Buffer(buffer, &nVertexCount, sizeof(int));
        if (nVertexCount > 0)
        {
            std::vector<BrushVec3> positions(nVertexCount);
            for (int i = 0; i < nVertexCount; ++i)
            {
                positions[i] = m_Vertices[i].pos;
            }
            Write2Buffer(buffer, &positions[0], sizeof(BrushVec3) * nVertexCount);
        }
        Write2Buffer(buffer, &nEdgeCount, sizeof(int));
        if (nEdgeCount > 0)
        {
            Write2Buffer(buffer, &m_Edges[0], sizeof(SEdge) * nEdgeCount);
        }
        Write2Buffer(buffer, &m_Flag, sizeof(unsigned int));
        Write2Buffer(buffer, &m_MaterialID, sizeof(int));
        Write2Buffer(buffer, &m_TexInfo, sizeof(STexInfo));
        BrushFloat distance = GetPlane().Distance();
        Write2Buffer(buffer, &GetPlane().Normal(), sizeof(BrushVec3));
        Write2Buffer(buffer, &distance, sizeof(BrushFloat));
    }

    void Polygon::SaveUVBinary(std::vector<char>& buffer)
    {
        if (m_Vertices.empty())
        {
            return;
        }

        int nVertexCount = m_Vertices.size();
        std::vector<Vec2> uvs(nVertexCount);
        for (int i = 0; i < nVertexCount; ++i)
        {
            uvs[i] = m_Vertices[i].uv;
        }
        Write2Buffer(buffer, &uvs[0], sizeof(Vec2) * nVertexCount);
    }

    int Polygon::LoadBinary(std::vector<char>& buffer)
    {
        int nBufferPos = 0;
        int nVertexCount = 0;
        nBufferPos = ReadFromBuffer(buffer, nBufferPos, &nVertexCount, sizeof(int));
        m_Vertices.clear();
        if (nVertexCount > 0)
        {
            std::vector<BrushVec3> positions(nVertexCount);
            nBufferPos = ReadFromBuffer(buffer, nBufferPos, &positions[0], sizeof(BrushVec3) * nVertexCount);
            m_Vertices.resize(nVertexCount);
            for (int i = 0; i < nVertexCount; ++i)
            {
                m_Vertices[i].pos = positions[i];
            }
        }
        int nEdgeCount = 0;
        m_Edges.clear();
        nBufferPos = ReadFromBuffer(buffer, nBufferPos, &nEdgeCount, sizeof(int));
        if (nEdgeCount > 0)
        {
            m_Edges.resize(nEdgeCount);
            nBufferPos = ReadFromBuffer(buffer, nBufferPos, &m_Edges[0], sizeof(SEdge) * nEdgeCount);
        }
        nBufferPos = ReadFromBuffer(buffer, nBufferPos, &m_Flag, sizeof(unsigned int));
        nBufferPos = ReadFromBuffer(buffer, nBufferPos, &m_MaterialID, sizeof(int));
        nBufferPos = ReadFromBuffer(buffer, nBufferPos, &m_TexInfo, sizeof(STexInfo));
        BrushVec3 planeNormal = GetPlane().Normal();
        nBufferPos = ReadFromBuffer(buffer, nBufferPos, &planeNormal, sizeof(BrushVec3));
        BrushFloat planeDist = GetPlane().Distance();
        nBufferPos = ReadFromBuffer(buffer, nBufferPos, &planeDist, sizeof(BrushFloat));
        SetPlane(BrushPlane(planeNormal, planeDist));

        return nBufferPos;
    }

    void Polygon::LoadUVBinary(std::vector<char>& buffer, int offset)
    {
        if (m_Vertices.empty())
        {
            return;
        }
        int nVertexCount = m_Vertices.size();
        std::vector<Vec2> uvs(nVertexCount);
        ReadFromBuffer(buffer, offset, &uvs[0], nVertexCount * sizeof(Vec2));
        for (int i = 0; i < nVertexCount; ++i)
        {
            m_Vertices[i].uv = uvs[i];
        }
    }

    void Polygon::Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bUndo)
    {
        if (bLoading)
        {
            const char* srcString = NULL;
            if (xmlNode->getAttr("BinaryData", &srcString))
            {
                int nLength = strlen(srcString);
                if (nLength > 0)
                {
                    std::vector<char> buffer;
                    int nDestBufferLen = Base64::decodedsize_base64(nLength);
                    buffer.resize(nDestBufferLen);
                    Base64::decode_base64(&buffer[0], srcString, nLength, false);
                    if (!buffer.empty())
                    {
                        int offset = LoadBinary(buffer);
                        int decoded_offset = Base64::encodedsize_base64(offset + 1);
                        if (decoded_offset < nLength)
                        {
                            LoadUVBinary(buffer, offset);
                        }
                    }
                }
            }
            else
            {
                Load(xmlNode, bUndo);
            }

            xmlNode->getAttr("GUID", m_GUID);
            UpdateBoundBox();
            Optimize();
        }
        else
        {
            std::vector<char> buffer;
            SaveBinary(buffer);
            SaveUVBinary(buffer);

            if (!buffer.empty())
            {
                std::vector<char> encodedStr(Base64::encodedsize_base64(buffer.size()) + 1);
                Base64::encode_base64(&encodedStr[0], &buffer[0], buffer.size(), true);
                xmlNode->setAttr("GUID", m_GUID);
                xmlNode->setAttr("BinaryData", &encodedStr[0]);
            }
        }
    }

    void Polygon::Load(XmlNodeRef& xmlNode, bool bUndo)
    {
        BrushVec3 p_normal(0, 0, 0);
        BrushFloat p_distance(0);

        xmlNode->getAttr("matID", m_MaterialID);
        m_TexInfo.Load(xmlNode);

        DeleteAllVertices_Basic();
        int nVer(0);
        xmlNode->getAttr("Ver", nVer);
        for (int i = 0;; ++i)
        {
            if (nVer == 0)
            {
                BrushVec2 position;
                QString attribute;
                attribute = QStringLiteral("p%1").arg(i);
                if (!xmlNode->getAttr(attribute.toUtf8().data(), position))
                {
                    break;
                }
                AddVertex_Basic(SVertex(m_Plane.P2W(position)));
            }
            else if (nVer == 1)
            {
                BrushVec3 position;
                QString attribute;
                attribute = QStringLiteral("v%1").arg(i);
                if (!xmlNode->getAttr(attribute.toUtf8().data(), position))
                {
                    break;
                }
                AddVertex_Basic(SVertex(position));
            }
        }

        DeleteAllEdges_Basic();
        for (int i = 0;; ++i)
        {
            QString attribute;
            attribute = QStringLiteral("e%1").arg(i);
            int edgeindices(0);
            if (!xmlNode->getAttr(attribute.toUtf8().data(), edgeindices))
            {
                break;
            }
            int e[2];
            e[0] = (edgeindices & 0xFFFF0000) >> 16;
            e[1] = edgeindices & 0x0000FFFF;
            if (e[0] >= m_Vertices.size() || e[1] >= m_Vertices.size())
            {
                DESIGNER_ASSERT(0);
                continue;
            }
            AddEdge_Basic(SEdge(e[0], e[1]));
        }

        xmlNode->getAttr("Flags", m_Flag);

        bool bReadPlaneNormal = xmlNode->getAttr("planeNormal", p_normal);
        bool bReadPlaneDist = xmlNode->getAttr("planeDistance", p_distance);

        if (IsValid() && (!bReadPlaneNormal || !bReadPlaneDist || !IsOpen()))
        {
            BrushPlane plane;
            if (GetComputedPlane(plane))
            {
                SetPlane(plane);
            }
            else
            {
                Clear();
            }
        }
        else
        {
            m_Plane.Set(p_normal, p_distance);
        }
    }

    void Polygon::Save(XmlNodeRef& xmlNode, bool bUndo)
    {
        xmlNode->setAttr("Ver", "1");

        for (int i = 0, iSize(m_Vertices.size()); i < iSize; ++i)
        {
            QString attribute;
            attribute = QStringLiteral("v%1").arg(i);
            xmlNode->setAttr(attribute.toUtf8().data(), m_Vertices[i].pos);
        }

        for (int i = 0, iSize(m_Edges.size()); i < iSize; ++i)
        {
            QString attribute;
            attribute = QStringLiteral("e%1").arg(i);
            int edgeindices((m_Edges[i].m_i[0] << 16) | (m_Edges[i].m_i[1]));
            xmlNode->setAttr(attribute.toUtf8().data(), edgeindices);
        }

        xmlNode->setAttr("matID", m_MaterialID);
        m_TexInfo.Save(xmlNode);

        if (IsOpen())
        {
            xmlNode->setAttr("planeNormal", GetPlane().Normal());
            xmlNode->setAttr("planeDistance", GetPlane().Distance());
        }

        xmlNode->setAttr("Flags", m_Flag);
    }

    void Polygon::AddEdges(const std::vector<BrushVec2>& positivePoints, const std::vector<BrushVec2>& negativePoints, std::vector<BrushVec2>& outPoints, std::vector<SEdge>& outEdges)
    {
        if (positivePoints.empty() || negativePoints.empty())
        {
            DESIGNER_ASSERT(0);
            return;
        }

        int basePointIndex = outPoints.size();

        for (int i = 0, iSize(positivePoints.size()); i < iSize; ++i)
        {
            outPoints.push_back(positivePoints[i]);
        }

        for (int i = 0, iSize(negativePoints.size()); i < iSize; ++i)
        {
            outPoints.push_back(negativePoints[iSize - i - 1]);
        }

        for (int i = basePointIndex, iPointSize(outPoints.size()); i < iPointSize; ++i)
        {
            int nexti = (i + 1 == iPointSize) ? basePointIndex : i + 1;
            outEdges.push_back(SEdge(i, nexti));
        }
    }

    bool Polygon::UpdatePlane(const BrushPlane& plane, const BrushVec3& directionForHitTest)
    {
        if (plane.IsEquivalent(GetPlane()))
        {
            return false;
        }

        std::vector<SVertex> newVertices;
        std::vector<SEdge> newEdges;

        int numberOfVertices = (int)m_Vertices.size();
        int numberOfEdges = (int)m_Edges.size();

        newVertices.reserve(numberOfVertices);
        newEdges.reserve(numberOfEdges);

        std::map<int, int> indexMap;

        for (int i = 0; i < numberOfVertices; ++i)
        {
            BrushVec3 worldPT(GetPos(i));
            BrushVec3 vOut;
            if (plane.HitTest(worldPT, worldPT + directionForHitTest, NULL, &vOut))
            {
                indexMap[i] = AddVertex(newVertices, SVertex(vOut));
            }
        }

        if (newVertices.size() < 3)
        {
            return false;
        }

        for (int i = 0; i < numberOfEdges; ++i)
        {
            std::map<int, int>::iterator iIndex0 = indexMap.find(m_Edges[i].m_i[0]);
            if (iIndex0 == indexMap.end())
            {
                DESIGNER_ASSERT(0);
                continue;
            }
            std::map<int, int>::iterator iIndex1 = indexMap.find(m_Edges[i].m_i[1]);
            if (iIndex1 == indexMap.end())
            {
                DESIGNER_ASSERT(0);
                continue;
            }
            int newIndex0 = iIndex0->second;
            int newIndex1 = iIndex1->second;
            if (newIndex0 == newIndex1)
            {
                continue;
            }
            newEdges.push_back(SEdge(newIndex0, newIndex1));
        }

        if (newEdges.size() < 3)
        {
            return false;
        }

        SetVertexList_Basic(newVertices);
        SetEdgeList_Basic(newEdges);

        if (plane.Normal().Dot(GetPlane().Normal()) < 0)
        {
            ReverseEdges();
        }

        SetPlane(plane);

        return true;
    }

    BrushVec3 Polygon::GetCenterPosition() const
    {
        BrushVec3 minVertex(3e10f, 3e10f, 3e10f);
        BrushVec3 maxVertex(-3e10f, -3e10f, -3e10f);
        BrushVec3 centerPos(0, 0, 0);
        int iVertexSize(m_Vertices.size());
        for (int i = 0; i < iVertexSize; ++i)
        {
            if (GetPos(i).x < minVertex.x)
            {
                minVertex.x = GetPos(i).x;
            }
            if (GetPos(i).x > maxVertex.x)
            {
                maxVertex.x = GetPos(i).x;
            }
            if (GetPos(i).y < minVertex.y)
            {
                minVertex.y = GetPos(i).y;
            }
            if (GetPos(i).y > maxVertex.y)
            {
                maxVertex.y = GetPos(i).y;
            }
            if (GetPos(i).z < minVertex.z)
            {
                minVertex.z = GetPos(i).z;
            }
            if (GetPos(i).z > maxVertex.z)
            {
                maxVertex.z = GetPos(i).z;
            }
        }
        centerPos = (minVertex + maxVertex) * 0.5f;
        return centerPos;
    }

    BrushVec3 Polygon::GetAveragePosition() const
    {
        BrushVec3 averagePos(0, 0, 0);
        int iVertexSize(m_Vertices.size());
        for (int i = 0; i < iVertexSize; ++i)
        {
            averagePos += GetPos(i);
        }
        averagePos /= iVertexSize;
        return averagePos;
    }

    BrushVec3 Polygon::GetRepresentativePosition() const
    {
        if (m_pRepresentativePos)
        {
            return *m_pRepresentativePos;
        }

        m_pRepresentativePos = new BrushVec3;

#if 1
        if (IsOpen())
        {
            std::vector<SVertex> linkedVertices;
            GetLinkedVertices(linkedVertices);
            if (!linkedVertices.empty())
            {
                int nSize = linkedVertices.size();
                if ((nSize % 2) == 1)
                {
                    *m_pRepresentativePos = linkedVertices[nSize / 2].pos;
                }
                else
                {
                    *m_pRepresentativePos = (BrushFloat)0.5 * (linkedVertices[nSize / 2].pos + linkedVertices[(nSize / 2 - 1)].pos);
                }
            }
        }
        else
        {
            *m_pRepresentativePos = GetAveragePosition();
        }
#else
        if (IsConvex())
        {
            *m_pRepresentativePos = GetAveragePosition();
        }
        else
        {
            std::map< BrushFloat, int, std::greater<BrushFloat> > verticesSortedByDistance;
            for (int i = 1, iVertexCount(m_Vertices.size()); i < iVertexCount; ++i)
            {
                BrushVec3 vDir = m_Vertices[i] - m_Vertices[0];
                verticesSortedByDistance[vDir.GetLength()] = i;
            }

            bool bFound = false;

            std::map< BrushFloat, int, std::greater<BrushFloat> >::iterator iter = verticesSortedByDistance.begin();
            for (; iter != verticesSortedByDistance.end(); ++iter)
            {
                int nTargetIndex = iter->second;
                if (HasEdge(BrushEdge3D(m_Vertices[0], m_Vertices[nTargetIndex])))
                {
                    continue;
                }
                BrushVec3 vDir = m_Vertices[nTargetIndex] - m_Vertices[0];
                BrushVec3 v0 = m_Vertices[0] + vDir * (BrushFloat)0.001f;
                BrushVec3 v1 = m_Vertices[nTargetIndex] - vDir * (BrushFloat)0.001f;
                if (GetBSPTree()->IsInside(BrushEdge3D(v0, v1), false))
                {
                    *m_pRepresentativePos = (v0 + v1) * (BrushFloat)0.5f;
                    bFound = true;
                    break;
                }
            }

            if (!bFound)
            {
                *m_pRepresentativePos = m_Vertices[0];
            }
        }
#endif

        return *m_pRepresentativePos;
    }

    bool Polygon::Scale(const BrushFloat& kScale, bool bCheckBoundary, std::vector<BrushEdge3D>* pOutEdgesBeforeOptimization)
    {
        if (std::abs(kScale) < BrushFloat(0.001))
        {
            return true;
        }

        if (IsOpen())
        {
            return false;
        }

        if (bCheckBoundary)
        {
            std::vector<PolygonPtr> outPolygon;
            std::vector<PolygonPtr> innerPolygons;
            GetSeparatedPolygons(outPolygon, eSP_OuterHull);
            GetSeparatedPolygons(innerPolygons, eSP_InnerHull);

            if (outPolygon.size() == 1 && !innerPolygons.empty())
            {
                if (!outPolygon[0]->Scale(kScale, false))
                {
                    return false;
                }

                int innerPolygonSize(innerPolygons.size());
                for (int i = 0; i < innerPolygonSize; ++i)
                {
                    if (!innerPolygons[i]->Scale(kScale, false))
                    {
                        return false;
                    }
                    innerPolygons[i]->ReverseEdges();
                }

                if (!outPolygon[0]->IncludeAllEdges(innerPolygons[0]))
                {
                    return false;
                }

                for (int i = 0; i < innerPolygonSize; ++i)
                {
                    if (!outPolygon[0]->IncludeAllEdges(innerPolygons[i]))
                    {
                        return false;
                    }
                    for (int k = i + 1; k < innerPolygonSize; ++k)
                    {
                        EIntersectionType interestionType = Polygon::HasIntersection(innerPolygons[i], innerPolygons[k]);
                        if (interestionType != eIT_None)
                        {
                            return false;
                        }
                    }
                }
            }
        }

        std::vector<BrushEdge> edges;
        std::vector<BrushLine> lines;

        int iEdgeSize(m_Edges.size());

        edges.resize(iEdgeSize);
        lines.resize(iEdgeSize);

        for (int i = 0; i < iEdgeSize; ++i)
        {
            BrushEdge3D edge3D = GetEdge(i);
            edges[i] = BrushEdge(m_Plane.W2P(edge3D.m_v[0]), m_Plane.W2P(edge3D.m_v[1]));

            BrushLine line(edges[i].m_v[0], edges[i].m_v[1]);
            line.m_Distance += kScale;
            lines[i] = line;
        }

        for (int i = 0; i < iEdgeSize; ++i)
        {
            int edgeIndices[2] = {-1, -1};
            if (!GetAdjacentEdgeIndexWithEdgeIndex(i, edgeIndices[eSD_Previous], edgeIndices[eSD_Next], m_Vertices, m_Edges))
            {
                DESIGNER_ASSERT(0);
                return false;
            }

            if (edgeIndices[eSD_Previous] != -1 && !lines[i].Intersect(lines[edgeIndices[eSD_Previous]], edges[i].m_v[0], kDesignerEpsilon * kDesignerEpsilon))
            {
                if (!lines[i].HitTest(edges[i].m_v[0], edges[i].m_v[0] + lines[i].m_Normal, NULL, &edges[i].m_v[0]))
                {
                    DESIGNER_ASSERT(0);
                    return false;
                }
            }

            if (edgeIndices[eSD_Next] != -1 && !lines[i].Intersect(lines[edgeIndices[eSD_Next]], edges[i].m_v[1], kDesignerEpsilon * kDesignerEpsilon))
            {
                if (!lines[i].HitTest(edges[i].m_v[1], edges[i].m_v[1] + lines[i].m_Normal, NULL, &edges[i].m_v[1]))
                {
                    DESIGNER_ASSERT(0);
                    return false;
                }
            }
        }

        std::vector<BrushEdge3D> edges3D;
        edges3D.reserve(iEdgeSize);
        for (int i = 0; i < iEdgeSize; ++i)
        {
            BrushEdge3D scaledEdge3D = BrushEdge3D(m_Plane.P2W(edges[i].m_v[0]), m_Plane.P2W(edges[i].m_v[1]));
            BrushEdge3D edge3D = GetEdge(i);

            scaledEdge3D.m_data[0] = edge3D.m_data[0];
            scaledEdge3D.m_data[1] = edge3D.m_data[1];

            Vec3_tpl<BrushFloat> vScaledDir = scaledEdge3D.m_v[1] - scaledEdge3D.m_v[0];
            Vec3_tpl<BrushFloat> vOriginalDir = GetPos(m_Edges[i].m_i[1]) - GetPos(m_Edges[i].m_i[0]);

            if (bCheckBoundary)
            {
                if (vScaledDir.Dot(vOriginalDir) <= 0)
                {
                    return false;
                }

                int nNextI = (i + 1) % iEdgeSize;
                int nPrevI = (i - 1) < 0 ? iEdgeSize - 1 : (i - 1);

                for (int k = 0; k < iEdgeSize; ++k)
                {
                    if (k == i || k == nNextI || k == nPrevI)
                    {
                        continue;
                    }
                    if (edges[i].IsIntersect(edges[k]))
                    {
                        return false;
                    }
                }
            }

            edges3D.push_back(scaledEdge3D);
        }

        if (pOutEdgesBeforeOptimization)
        {
            for (int i = 0, iEdgeSize(edges3D.size()); i < iEdgeSize; ++i)
            {
                *pOutEdgesBeforeOptimization = edges3D;
            }
        }

        if (!Optimize(edges3D))
        {
            return false;
        }

        if (IsOpen())
        {
            return false;
        }

        return true;
    }

    bool Polygon::BroadenVertex(const BrushFloat& kScale, int nVertexIndex, const BrushEdge3D* pBaseEdge)
    {
        int nPrevEdgeIndex = -1;
        int nNextEdgeIndex = -1;
        if (!GetAdjacentEdgesByVertexIndex(nVertexIndex, &nPrevEdgeIndex, &nNextEdgeIndex))
        {
            return false;
        }

        BrushEdge3D nextEdge = GetEdge(nNextEdgeIndex);
        BrushEdge3D prevEdge = GetEdge(nPrevEdgeIndex);
        BrushFloat fDeltaToNextEdge = kScale;
        BrushFloat fDeltaToPrevEdge = kScale;
        if (pBaseEdge)
        {
            BrushEdge3D baseEdge(*pBaseEdge);
            if (!CD::IsEquivalent(baseEdge.m_v[0], nextEdge.m_v[0]))
            {
                baseEdge.Invert();
            }

            BrushVec3 vBaseEdgeDir = (baseEdge.m_v[1] - baseEdge.m_v[0]).GetNormalized();
            BrushVec3 vNextEdgeDir = (nextEdge.m_v[1] - nextEdge.m_v[0]).GetNormalized();
            BrushVec3 vPrevEdgeDir = (prevEdge.m_v[1] - prevEdge.m_v[0]).GetNormalized();

            if (CD::IsEquivalent(nextEdge.m_v[0], baseEdge.m_v[0]))
            {
                fDeltaToNextEdge = fDeltaToNextEdge / std::sin(std::acos(vNextEdgeDir.Dot(-vBaseEdgeDir)));
            }
            else if (CD::IsEquivalent(nextEdge.m_v[1], baseEdge.m_v[0]))
            {
                fDeltaToNextEdge = fDeltaToNextEdge / std::sin(std::acos(vNextEdgeDir.Dot(vBaseEdgeDir)));
            }

            if (CD::IsEquivalent(prevEdge.m_v[0], baseEdge.m_v[0]))
            {
                fDeltaToPrevEdge = fDeltaToPrevEdge / std::sin(std::acos(vPrevEdgeDir.Dot(-vBaseEdgeDir)));
            }
            else if (CD::IsEquivalent(prevEdge.m_v[1], baseEdge.m_v[0]))
            {
                fDeltaToPrevEdge = fDeltaToPrevEdge / std::sin(std::acos(vPrevEdgeDir.Dot(vBaseEdgeDir)));
            }
        }

        m_Vertices[nVertexIndex] = SVertex(nextEdge.m_v[0] + fDeltaToNextEdge * (nextEdge.m_v[1] - nextEdge.m_v[0]).GetNormalized());
        BrushVec3 newVertex = prevEdge.m_v[1] + fDeltaToPrevEdge * (prevEdge.m_v[0] - prevEdge.m_v[1]).GetNormalized();
        AddVertex(newVertex);

        return true;
    }

    bool Polygon::GetMaximumScale(BrushFloat& fOutShortestScale) const
    {
        int iEdgeSize(m_Edges.size());
        int nShortestEdgeIndex(-1);
        BrushFloat fShortestLength = 3e10f;

        for (int i = 0; i < iEdgeSize; ++i)
        {
            BrushEdge3D edge3D = GetEdge(i);
            BrushFloat fEdgeLength = edge3D.GetLength();
            if (fEdgeLength < fShortestLength)
            {
                fShortestLength = fEdgeLength;
                nShortestEdgeIndex = i;
            }
        }

        if (nShortestEdgeIndex == -1)
        {
            return false;
        }

        int nNextEdge = (nShortestEdgeIndex + 1) % iEdgeSize;
        BrushEdge3D nextEdge3D = GetEdge(nNextEdge);
        int nPrevEdge = (nShortestEdgeIndex - 1) < 0 ? iEdgeSize - 1 : (nShortestEdgeIndex - 1);
        BrushEdge3D prevEdge3D = GetEdge(nPrevEdge);

        BrushEdge prevEdge = BrushEdge(m_Plane.W2P(prevEdge3D.m_v[0]), m_Plane.W2P(prevEdge3D.m_v[1]));
        BrushEdge nextEdge = BrushEdge(m_Plane.W2P(nextEdge3D.m_v[0]), m_Plane.W2P(nextEdge3D.m_v[1]));

        Vec2 prevDir = (prevEdge.m_v[1] - prevEdge.m_v[0]).GetNormalized();
        Vec2 nextDir = (nextEdge.m_v[1] - nextEdge.m_v[0]).GetNormalized();

        BrushLine prevLine(prevEdge.m_v[0], prevEdge.m_v[1]);
        BrushLine nextLine(nextEdge.m_v[0], nextEdge.m_v[1]);

        BrushEdge3D shortestEdge3D = GetEdge(nShortestEdgeIndex);
        BrushEdge shortestEdge(m_Plane.W2P(shortestEdge3D.m_v[0]), m_Plane.W2P(shortestEdge3D.m_v[1]));
        BrushLine shortestLine(shortestEdge.m_v[0], shortestEdge.m_v[1]);
        BrushVec2 shortestEdgeDir = (shortestEdge.m_v[1] - shortestEdge.m_v[0]).GetNormalized();

        float shortestEdgeSpeed0 = std::tan(std::acos(prevDir.Dot(shortestLine.m_Normal)));
        float shortestEdgeSpeed1 = std::tan(std::acos(nextDir.Dot(-shortestLine.m_Normal)));

        float prevSpeed = 1 / shortestEdgeDir.Dot(-prevLine.m_Normal);
        float nextSpeed = 1 / shortestEdgeDir.Dot(nextLine.m_Normal);

        fOutShortestScale = -(fShortestLength) / (prevSpeed + nextSpeed - shortestEdgeSpeed0 - shortestEdgeSpeed1) + kDesignerEpsilon * 100.0f;

        return true;
    }

    PolygonPtr Polygon::RemoveInside()
    {
        std::vector<PolygonPtr> polygonList;
        if (GetSeparatedPolygons(polygonList, eSP_OuterHull) && polygonList.size() == 1)
        {
            *this = *polygonList[0];
        }
        return this;
    }

    BrushFloat Polygon::GetNearestDistance(PolygonPtr pPolygon, const BrushVec3& direction) const
    {
        BrushFloat fShortestDistance(3e10f);
        for (int i = 0, iVertexSize(pPolygon->m_Vertices.size()); i < iVertexSize; ++i)
        {
            BrushVec3 targetVertex(pPolygon->GetPos(i));
            BrushFloat distance(3e10f);
            if (!GetPlane().HitTest(targetVertex, targetVertex + direction, &distance))
            {
                continue;
            }
            if (distance < fShortestDistance)
            {
                fShortestDistance = distance;
            }
        }
        if (fabs(fShortestDistance) < kDesignerEpsilon)
        {
            fShortestDistance = 0;
        }
        return fShortestDistance;
    }

    void Polygon::MakeThisConvex()
    {
        std::vector<SVertex> face;
        if (!GetLinkedVertices(face, m_Vertices, m_Edges))
        {
            return;
        }

        std::vector<Vec3> polygonVec3;
        std::vector<Vec3> convexHullPolygonVec3;

        polygonVec3.resize(face.size());
        for (int i = 0, iSize(face.size()); i < iSize; ++i)
        {
            BrushVec2 pt = m_Plane.W2P(face[i].pos);
            polygonVec3[i] = Vec3((float)pt.x, (float)pt.y, 0);
        }

        ConvexHull2D(convexHullPolygonVec3, polygonVec3);

        int iConvexHullSize = convexHullPolygonVec3.size();
        if (iConvexHullSize < 3)
        {
            return;
        }

        DeleteAllVertices_Basic();
        DeleteAllEdges_Basic();
        m_Vertices.reserve(iConvexHullSize);
        m_Edges.reserve(iConvexHullSize);

        for (int i = 0; i < iConvexHullSize; ++i)
        {
            AddVertex_Basic(SVertex(m_Plane.P2W(BrushVec2(convexHullPolygonVec3[i].x, convexHullPolygonVec3[i].y))));
        }

        for (int i = 0; i < iConvexHullSize; ++i)
        {
            int nexti = (i + 1) % iConvexHullSize;
            BrushEdge3D edge(GetPos(i), GetPos(nexti));
            if (edge.IsPoint())
            {
                continue;
            }
            AddEdge_Basic(SEdge(i, nexti));
        }

        BrushVec3 v0 = convexHullPolygonVec3[0] - convexHullPolygonVec3[1];
        BrushVec3 v1 = convexHullPolygonVec3[2] - convexHullPolygonVec3[1];
        BrushVec3 vCrossV0V1 = v0.Cross(v1);

        if (vCrossV0V1.z < 0)
        {
            ReverseEdges();
        }
    }

    bool Polygon::GetEdgeIndex(const std::vector<SEdge>& edgeList, int nVertexIndex0, int nVertexIndex1, int& nEdgeIndex)
    {
        for (int i = 0, iEdgeSize(edgeList.size()); i < iEdgeSize; ++i)
        {
            if (edgeList[i].m_i[0] == nVertexIndex0 && edgeList[i].m_i[1] == nVertexIndex1)
            {
                nEdgeIndex = i;
                return true;
            }
        }
        return false;
    }

    int Polygon::GetEdgeIndex(const BrushEdge3D& edge3D) const
    {
        for (int i = 0, iEdgeSize(m_Edges.size()); i < iEdgeSize; ++i)
        {
            BrushEdge3D edgeFromList = GetEdge(i);
            if (edgeFromList.IsEquivalent(edge3D))
            {
                return i;
            }
        }
        return -1;
    }

    bool Polygon::GetEdge(int nVertexIndex0, int nVertexIndex1, SEdge& outEdge) const
    {
        int nEdgeIndex(-1);
        if (!GetEdgeIndex(nVertexIndex0, nVertexIndex1, nEdgeIndex))
        {
            return false;
        }
        outEdge = m_Edges[nEdgeIndex];
        return true;
    }

    bool Polygon::GetEdgesByVertexIndex(int nVertexIndex, std::vector<int>& outEdgeIndices) const
    {
        bool bAdded = false;
        for (int i = 0, iEdgeCount(m_Edges.size()); i < iEdgeCount; ++i)
        {
            if (m_Edges[i].m_i[0] == nVertexIndex || m_Edges[i].m_i[1] == nVertexIndex)
            {
                bAdded = true;
                outEdgeIndices.push_back(i);
            }
        }
        return bAdded;
    }

    bool Polygon::GetNextVertex(int nVertexIndex, BrushVec3& outVertex) const
    {
        for (int i = 0, iEdgeSize(m_Edges.size()); i < iEdgeSize; ++i)
        {
            if (m_Edges[i].m_i[0] == nVertexIndex)
            {
                outVertex = GetPos(m_Edges[i].m_i[1]);
                return true;
            }
        }
        return false;
    }

    bool Polygon::GetPrevVertex(int nVertexIndex, BrushVec3& outVertex) const
    {
        for (int i = 0, iEdgeSize(m_Edges.size()); i < iEdgeSize; ++i)
        {
            if (m_Edges[i].m_i[1] == nVertexIndex)
            {
                outVertex = GetPos(m_Edges[i].m_i[0]);
                return true;
            }
        }
        return false;
    }

    bool Polygon::AddVertex(const BrushVec3& vertex, int* pOutNewVertexIndex, EdgeIndexSet* pOutNewEdgeIndices)
    {
        bool bEquivalentExist = false;
        for (int i = 0, iEdgeSize(m_Edges.size()); i < iEdgeSize; ++i)
        {
            BrushEdge3D edge = GetEdge(i);
            if (CD::IsEquivalent(edge.m_v[0], vertex))
            {
                if (pOutNewVertexIndex)
                {
                    *pOutNewVertexIndex = m_Edges[i].m_i[0];
                }
                if (pOutNewEdgeIndices)
                {
                    pOutNewEdgeIndices->insert(i);
                }
                bEquivalentExist = true;
            }
        }

        if (bEquivalentExist)
        {
            return true;
        }

        BrushFloat fNearestDistance(3e10f);
        int nNearestEdgeIndex(-1);

        int iEdgeSize(GetEdgeCount());

        for (int i = 0; i < iEdgeSize; ++i)
        {
            BrushEdge3D edge = GetEdge(i);
            BrushFloat fDistance(0);
            if (eResultDistance_Middle != BrushEdge3D::GetSquaredDistance(edge, vertex, fDistance))
            {
                continue;
            }
            if (fDistance < fNearestDistance)
            {
                fNearestDistance = fDistance;
                nNearestEdgeIndex = i;
            }
        }

        if (nNearestEdgeIndex == -1)
        {
            return false;
        }

        int nNewVertexIndex(m_Vertices.size());
        AddVertex_Basic(SVertex(vertex));

        AddEdge_Basic(SEdge(nNewVertexIndex, m_Edges[nNearestEdgeIndex].m_i[1]));
        m_Edges[nNearestEdgeIndex].m_i[1] = nNewVertexIndex;

        if (pOutNewVertexIndex)
        {
            *pOutNewVertexIndex = nNewVertexIndex;
        }

        if (pOutNewEdgeIndices)
        {
            pOutNewEdgeIndices->insert(m_Edges.size() - 1);
        }

        return true;
    }

    bool Polygon::Exist(const BrushVec3& vertex, const BrushFloat& kEpsilon, int* pOutIndex) const
    {
        for (int i = 0, iVertexSize(m_Vertices.size()); i < iVertexSize; ++i)
        {
            if (CD::IsEquivalent(m_Vertices[i].pos, vertex))
            {
                if (pOutIndex)
                {
                    *pOutIndex = i;
                }
                return true;
            }
        }
        return false;
    }

    bool Polygon::Exist(const BrushEdge3D& edge3D, bool bAllowReverse, int* pOutIndex) const
    {
        for (int i = 0, iEdgeSize(m_Edges.size()); i < iEdgeSize; ++i)
        {
            if (CD::IsEquivalent(m_Vertices[m_Edges[i].m_i[0]].pos, edge3D.m_v[0]) && CD::IsEquivalent(m_Vertices[m_Edges[i].m_i[1]].pos, edge3D.m_v[1]))
            {
                if (pOutIndex)
                {
                    *pOutIndex = i;
                }
                return true;
            }
            if (bAllowReverse)
            {
                if (CD::IsEquivalent(m_Vertices[m_Edges[i].m_i[1]].pos, edge3D.m_v[0]) && CD::IsEquivalent(m_Vertices[m_Edges[i].m_i[0]].pos, edge3D.m_v[1]))
                {
                    if (pOutIndex)
                    {
                        *pOutIndex = i;
                    }
                    return true;
                }
            }
        }
        return false;
    }

    bool Polygon::SwapEdge(int nEdgeIndex0, int nEdgeIndex1)
    {
        if (nEdgeIndex0 < 0 || nEdgeIndex0 >= GetEdgeCount() || nEdgeIndex1 < 0 || nEdgeIndex1 >= GetEdgeCount())
        {
            return false;
        }
        if (nEdgeIndex0 == nEdgeIndex1)
        {
            return true;
        }

        std::swap(m_Edges[nEdgeIndex0], m_Edges[nEdgeIndex0]);

        return true;
    }

    void Polygon::Move(const BrushVec3& offset)
    {
        if (!IsValid())
        {
            return;
        }
        for (int i = 0, iVertexCount(m_Vertices.size()); i < iVertexCount; ++i)
        {
            m_Vertices[i].pos += offset;
        }
        const BrushVec3& planeNormal = m_Plane.Normal();
        m_Plane.Set(planeNormal, -planeNormal.Dot(m_Vertices[0].pos));
        InvalidateCacheData();
    }

    void Polygon::Transform(const Matrix34& tm)
    {
        if (!IsValid())
        {
            return;
        }

        int iVertexSize(m_Vertices.size());

        std::vector<SVertex> transformedVertices;
        transformedVertices.resize(iVertexSize);
        for (int i = 0; i < iVertexSize; ++i)
        {
            transformedVertices[i] = SVertex(ToBrushVec3(tm.TransformPoint(GetPos(i))));
        }

        if (transformedVertices.size() >= 3)
        {
            std::vector<SVertex> linkedTransformedVertices;
            GetLinkedVertices(linkedTransformedVertices, transformedVertices, m_Edges);
            ComputePlane(linkedTransformedVertices, m_Plane);
        }
        else
        {
            BrushVec4 vPlane(m_Plane.Normal().x, m_Plane.Normal().y, m_Plane.Normal().z, m_Plane.Distance());
            Matrix44 tm44(tm);
            tm44.Invert();
            tm44.Transpose();
            vPlane = tm44 * vPlane;
            m_Plane.Set(BrushVec3(vPlane.x, vPlane.y, vPlane.z), vPlane.w);
        }

        SetVertexList_Basic(transformedVertices);
    }

    bool Polygon::IsEndPoint(const BrushVec3& position, bool* bOutFirst) const
    {
        if (!IsOpen())
        {
            return false;
        }

        for (int i = 0, iEdgeSize(m_Edges.size()); i < iEdgeSize; ++i)
        {
            for (int k = 0; k < 2; ++k)
            {
                int index = m_Edges[i].m_i[k];
                const BrushVec3& vertex = GetPos(index);
                if (!CD::IsEquivalent(vertex, position))
                {
                    continue;
                }

                std::vector<int> linkedPrevEdgeIndices;
                std::vector<int> linkedNextEdgeIndices;

                GetAdjacentPrevEdgeIndicesWithVertexIndex(index, linkedPrevEdgeIndices, m_Edges);
                GetAdjacentNextEdgeIndicesWithVertexIndex(index, linkedNextEdgeIndices, m_Edges);

                if (bOutFirst)
                {
                    *bOutFirst = linkedPrevEdgeIndices.empty() && !linkedNextEdgeIndices.empty();
                }

                return true;
            }
        }

        return false;
    }

    int Polygon::AddEdge(const BrushEdge3D& edge)
    {
        SEdge newEdge(-1, -1);
        for (int i = 0; i < 2; ++i)
        {
            if (!Exist(edge.m_v[i], &newEdge.m_i[i]))
            {
                newEdge.m_i[i] = m_Vertices.size();
                AddVertex_Basic(SVertex(edge.m_v[i]));
            }
        }
        if (newEdge.m_i[0] == -1 && newEdge.m_i[1] == -1)
        {
            return -1;
        }
        int nNewIndex = m_Edges.size();
        AddEdge_Basic(SEdge(newEdge));
        return nNewIndex;
    }

    bool Polygon::Concatenate(PolygonPtr pPolygon)
    {
        if (!pPolygon)
        {
            return false;
        }

        if (!IsOpen() || !pPolygon->IsOpen())
        {
            return false;
        }

        for (int i = 0, nVertexSize(pPolygon->m_Vertices.size()); i < nVertexSize; ++i)
        {
            bool bThisFirst(false);
            if (IsEndPoint(pPolygon->GetPos(i), &bThisFirst))
            {
                bool bTargetFirst(false);
                if (!pPolygon->IsEndPoint(pPolygon->GetPos(i), &bTargetFirst))
                {
                    continue;
                }
                for (int k = 0, nEdgeSize(pPolygon->m_Edges.size()); k < nEdgeSize; ++k)
                {
                    if (bThisFirst == bTargetFirst)
                    {
                        AddEdge(BrushEdge3D(pPolygon->GetPos(pPolygon->m_Edges[k].m_i[1]), pPolygon->GetPos(pPolygon->m_Edges[k].m_i[0])));
                    }
                    else
                    {
                        AddEdge(BrushEdge3D(pPolygon->GetPos(pPolygon->m_Edges[k].m_i[0]), pPolygon->GetPos(pPolygon->m_Edges[k].m_i[1])));
                    }
                }
                return true;
            }
        }

        return false;
    }

    bool Polygon::GetFirstVertex(BrushVec3& outVertex) const
    {
        if (!IsOpen())
        {
            return false;
        }

        for (int i = 0, iVertexSize(m_Vertices.size()); i < iVertexSize; ++i)
        {
            std::vector<int> adjacentPrevEdgeIndices;
            std::vector<int> adjacentNextEdgeIndices;

            GetAdjacentPrevEdgeIndicesWithVertexIndex(i, adjacentPrevEdgeIndices, m_Edges);
            GetAdjacentNextEdgeIndicesWithVertexIndex(i, adjacentNextEdgeIndices, m_Edges);

            if (!adjacentNextEdgeIndices.empty() && adjacentPrevEdgeIndices.empty())
            {
                outVertex = GetPos(i);
                return true;
            }
        }

        return false;
    }

    bool Polygon::GetLastVertex(BrushVec3& outVertex) const
    {
        if (!IsOpen())
        {
            return false;
        }

        for (int i = 0, iVertexSize(m_Vertices.size()); i < iVertexSize; ++i)
        {
            std::vector<int> adjacentPrevEdgeIndices;
            std::vector<int> adjacentNextEdgeIndices;

            GetAdjacentPrevEdgeIndicesWithVertexIndex(i, adjacentPrevEdgeIndices, m_Edges);
            GetAdjacentNextEdgeIndicesWithVertexIndex(i, adjacentNextEdgeIndices, m_Edges);

            if (adjacentNextEdgeIndices.empty() && !adjacentPrevEdgeIndices.empty())
            {
                outVertex = GetPos(i);
                return true;
            }
        }

        return false;
    }

    void Polygon::Rearrange()
    {
        std::vector<SVertex> linkedVertices;
        if (!GetLinkedVertices(linkedVertices))
        {
            return;
        }

        int iVertexSize(linkedVertices.size());
        std::vector<SEdge> newEdges;
        for (int i = 0; i < iVertexSize - 1; ++i)
        {
            newEdges.push_back(SEdge(i, i + 1));
        }

        SetVertexList_Basic(linkedVertices);
        SetEdgeList_Basic(newEdges);
    }

    void Polygon::ClipByEdge(int nEdgeIndex, std::vector<PolygonPtr>& outSplittedPolygons) const
    {
        std::vector<SVertex> exVertices;
        int nVertexSize = m_Vertices.size();
        exVertices.reserve(nVertexSize);
        for (int i = 0; i < nVertexSize; ++i)
        {
            exVertices.push_back(m_Vertices[i]);
        }

        ESearchDirection directions[2] = { eSD_Previous, eSD_Next };

        for (int i = 0; i < 2; ++i)
        {
            std::vector<int> oneDirectionEdges;
            std::vector<BrushVec3> oneDirectionVertices;

            SearchLinkedEdges(nEdgeIndex, directions[i], exVertices, m_Edges, oneDirectionEdges);

            int nEdgeSize(oneDirectionEdges.size());
            if (nEdgeSize > 0)
            {
                if (directions[i] == eSD_Next)
                {
                    for (int i = 0; i < nEdgeSize; ++i)
                    {
                        oneDirectionVertices.push_back(GetPos(m_Edges[oneDirectionEdges[i]].m_i[0]));
                    }
                    oneDirectionVertices.push_back(GetPos(m_Edges[oneDirectionEdges[nEdgeSize - 1]].m_i[1]));
                }
                else
                {
                    for (int i = 0; i < nEdgeSize; ++i)
                    {
                        oneDirectionVertices.push_back(GetPos(m_Edges[oneDirectionEdges[i]].m_i[1]));
                    }
                    oneDirectionVertices.push_back(GetPos(m_Edges[oneDirectionEdges[nEdgeSize - 1]].m_i[0]));
                }

                Polygon* pPolygon = new Polygon(oneDirectionVertices, m_Plane, m_MaterialID, &m_TexInfo, false);
                pPolygon->SetFlag(GetFlag());
                outSplittedPolygons.push_back(pPolygon);
            }
        }
    }

    void Polygon::MakePolygonsFromEdgeList(const std::vector<BrushEdge3D>& edgeList, const BrushPlane& plane, std::vector<PolygonPtr>& outPolygons)
    {
        std::set<int> usedEdgeIndexSet;

        for (int i = 0, iEdgeSize(edgeList.size()); i < iEdgeSize; ++i)
        {
            if (usedEdgeIndexSet.find(i) != usedEdgeIndexSet.end())
            {
                continue;
            }

            int nLeftConnectionIndex = -1;
            int nRightConnectionIndex = -1;

            for (int a = 0; a < iEdgeSize; ++a)
            {
                if (a == i || usedEdgeIndexSet.find(a) != usedEdgeIndexSet.end())
                {
                    continue;
                }

                if (nLeftConnectionIndex == -1 && CD::IsEquivalent(edgeList[i].m_v[0], edgeList[a].m_v[1]))
                {
                    nLeftConnectionIndex = a;
                }

                if (nRightConnectionIndex == -1  && CD::IsEquivalent(edgeList[i].m_v[1], edgeList[a].m_v[0]))
                {
                    nRightConnectionIndex = a;
                }

                if (nLeftConnectionIndex != -1 && nRightConnectionIndex != -1)
                {
                    break;
                }
            }

            if (nLeftConnectionIndex != -1 && nRightConnectionIndex != -1)
            {
                continue;
            }

            usedEdgeIndexSet.insert(i);

            PolygonPtr pNewPolygon = new Polygon;
            pNewPolygon->SetPlane(plane);
            pNewPolygon->AddEdge(edgeList[i]);

            BrushEdge3D currentEdge(edgeList[i]);
            int bExistConnection = true;

            while (bExistConnection)
            {
                bExistConnection = false;

                for (int a = 0; a < iEdgeSize; ++a)
                {
                    if (usedEdgeIndexSet.find(a) != usedEdgeIndexSet.end())
                    {
                        continue;
                    }
                    if (nLeftConnectionIndex == -1 && nRightConnectionIndex != -1)
                    {
                        if (CD::IsEquivalent(currentEdge.m_v[1], edgeList[a].m_v[0]))
                        {
                            bExistConnection = true;
                        }
                    }
                    else if (nLeftConnectionIndex != -1 && nRightConnectionIndex == -1)
                    {
                        if (CD::IsEquivalent(currentEdge.m_v[0], edgeList[a].m_v[1]))
                        {
                            bExistConnection = true;
                        }
                    }
                    if (bExistConnection)
                    {
                        pNewPolygon->AddEdge(edgeList[a]);
                        usedEdgeIndexSet.insert(a);
                        currentEdge = edgeList[a];
                        break;
                    }
                }
            }

            outPolygons.push_back(pNewPolygon);
        }
    }

    void Polygon::SetVertex(int nIndex, const SVertex& vertex)
    {
        if (nIndex < 0 || nIndex >= m_Vertices.size())
        {
            return;
        }
        SetVertex_Basic(nIndex, vertex);
    }

    void Polygon::SetPos(int nIndex, const BrushVec3& pos)
    {
        if (nIndex < 0 || nIndex >= m_Vertices.size())
        {
            return;
        }
        SetPos_Basic(nIndex, pos);
    }

    bool Polygon::GetVertexIndex(const BrushVec3& vertex, int& nOutIndex) const
    {
        for (int i = 0, nVertexSize(m_Vertices.size()); i < nVertexSize; ++i)
        {
            if (CD::IsEquivalent(m_Vertices[i].pos, vertex))
            {
                nOutIndex = i;
                return true;
            }
        }
        return false;
    }

    bool Polygon::GetNearestVertexIndex(const BrushVec3& vertex, int& nOutIndex) const
    {
        BrushFloat fMinDistance = (BrushFloat)3e10;
        int nIndex = -1;
        for (int i = 0, nVertexSize(m_Vertices.size()); i < nVertexSize; ++i)
        {
            BrushFloat fDistance = m_Vertices[i].pos.GetDistance(vertex);
            if (fDistance < fMinDistance)
            {
                fMinDistance = fDistance;
                nIndex = i;
            }
        }

        if (nIndex == -1)
        {
            return false;
        }

        nOutIndex = nIndex;

        return true;
    }

    BrushEdge Polygon::GetEdge2D(int nEdgeIndex) const
    {
        return BrushEdge(m_Plane.W2P(GetPos(m_Edges[nEdgeIndex].m_i[0])), m_Plane.W2P(GetPos(m_Edges[nEdgeIndex].m_i[1])));
    }

    BrushEdge3D Polygon::GetEdge(int nEdgeIndex) const
    {
        return BrushEdge3D(
            GetPos(m_Edges[nEdgeIndex].m_i[0]),
            GetPos(m_Edges[nEdgeIndex].m_i[1]),
            GetUV(m_Edges[nEdgeIndex].m_i[0]),
            GetUV(m_Edges[nEdgeIndex].m_i[1]));
    }

    int Polygon::GetEdgeIndex(int nVertexIndex0, int nVertexIndex1) const
    {
        for (int i = 0, iEdgeCount(m_Edges.size()); i < iEdgeCount; ++i)
        {
            if (m_Edges[i].m_i[0] == nVertexIndex0 && m_Edges[i].m_i[1] == nVertexIndex1)
            {
                return i;
            }
        }
        return -1;
    }

    bool Polygon::GetAdjacentEdgesByEdgeIndex(int nEdgeIndex, int* pOutPrevEdgeIndex, int* pOutNextEdgeIndex) const
    {
        int nPrevEdgeIndex = -1;
        int nNextEdgeIndex = -1;

        if (!GetAdjacentEdgeIndexWithEdgeIndex(nEdgeIndex, nPrevEdgeIndex, nNextEdgeIndex, m_Vertices, m_Edges))
        {
            return false;
        }

        if (pOutPrevEdgeIndex)
        {
            *pOutPrevEdgeIndex = nPrevEdgeIndex;
        }
        if (pOutNextEdgeIndex)
        {
            *pOutNextEdgeIndex = nNextEdgeIndex;
        }

        return true;
    }

    bool Polygon::GetAdjacentEdgesByVertexIndex(int nVertexIndex, int* pOutPrevEdgeIndex, int* pOutNextEdgeIndex) const
    {
        std::vector<int> prevEdgeIndices;
        GetAdjacentPrevEdgeIndicesWithVertexIndex(nVertexIndex, prevEdgeIndices, m_Edges);
        if (!prevEdgeIndices.empty() && pOutPrevEdgeIndex)
        {
            *pOutPrevEdgeIndex = prevEdgeIndices[0];
        }

        std::vector<int> nextEdgeIndices;
        GetAdjacentNextEdgeIndicesWithVertexIndex(nVertexIndex, nextEdgeIndices, m_Edges);
        if (!nextEdgeIndices.empty() && pOutNextEdgeIndex)
        {
            *pOutNextEdgeIndex = nextEdgeIndices[0];
        }

        return prevEdgeIndices.empty() || nextEdgeIndices.empty() ? false : true;
    }

    BrushFloat Polygon::GetRadius() const
    {
        if (!m_pBoundBox)
        {
            UpdateBoundBox();
        }
        return m_pBoundBox->raidus;
    }

    bool Polygon::IsPlaneEquivalent(PolygonPtr pPolygon) const
    {
        if (!pPolygon)
        {
            return false;
        }
        return GetPlane().IsEquivalent(pPolygon->GetPlane());
    }

    bool Polygon::IsCCW() const
    {
        std::vector<SVertex> vertices;
        if (!GetLinkedVertices(vertices))
        {
            return false;
        }
        int nVertexCount = vertices.size();
        if (nVertexCount < m_Vertices.size())
        {
            return false;
        }

        std::map< std::pair<BrushFloat, BrushFloat>, int > sortedPoints;
        for (int i = 0; i < nVertexCount; ++i)
        {
            BrushVec2 point = m_Plane.W2P(vertices[i].pos);
            sortedPoints[std::pair < BrushFloat, BrushFloat > (point.y, point.x)] = i;
        }

        int nLeftTopIndex = sortedPoints.begin()->second;
        int nPrev = ((nLeftTopIndex - 1) + nVertexCount) % nVertexCount;
        int nNext = (nLeftTopIndex + 1) % nVertexCount;

        BrushVec3 v0 = (vertices[nPrev].pos - vertices[nLeftTopIndex].pos).GetNormalized();
        BrushVec3 v1 = (vertices[nNext].pos - vertices[nLeftTopIndex].pos).GetNormalized();
        BrushVec3 vCross = v0.Cross(v1);

        return vCross.Dot(m_Plane.Normal()) < 0;
    }

    bool Polygon::IsVertexOnCrust(const BrushVec3& vertex) const
    {
        BrushVec2 point = m_Plane.W2P(vertex);

        for (int i = 0, iEdgeCount(m_Edges.size()); i < iEdgeCount; ++i)
        {
            BrushEdge edge = GetEdge2D(i);
            if (edge.IsInside(point))
            {
                return true;
            }
        }
        return false;
    }

    bool Polygon::HasVertex(const BrushVec3& vertex, int* pOutVertexIndex) const
    {
        for (int i = 0, iVertexCount(m_Vertices.size()); i < iVertexCount; ++i)
        {
            if (CD::IsEquivalent(m_Vertices[i].pos, vertex))
            {
                if (pOutVertexIndex)
                {
                    *pOutVertexIndex = i;
                }
                return true;
            }
        }

        return false;
    }

    bool Polygon::IsBridgeEdgeRelation(const SEdge& pEdge0, const SEdge& pEdge1) const
    {
        return pEdge0.m_i[0] == pEdge1.m_i[1] && pEdge0.m_i[1] == pEdge1.m_i[0];
    }

    bool Polygon::HasBridgeEdges() const
    {
        for (int i = 0, iEdgeCount(m_Edges.size()); i < iEdgeCount; ++i)
        {
            const SEdge& edge0 = m_Edges[i];
            for (int k = i + 1; k < iEdgeCount; ++k)
            {
                const SEdge& edge1 = m_Edges[k];
                if (IsBridgeEdgeRelation(edge0, edge1))
                {
                    return true;
                }
            }
        }
        return false;
    }

    void Polygon::GetBridgeEdgeSet(std::set<SEdge>& outBridgeEdgeSet, bool bOutputBothDirection) const
    {
        for (int i = 0, iEdgeCount(m_Edges.size()); i < iEdgeCount; ++i)
        {
            const SEdge& edge0 = m_Edges[i];

            if (outBridgeEdgeSet.find(edge0) != outBridgeEdgeSet.end())
            {
                continue;
            }

            for (int k = i + 1; k < iEdgeCount; ++k)
            {
                const SEdge& edge1 = m_Edges[k];
                if (IsBridgeEdgeRelation(edge0, edge1))
                {
                    outBridgeEdgeSet.insert(edge0);
                    if (bOutputBothDirection)
                    {
                        outBridgeEdgeSet.insert(edge1);
                    }
                    break;
                }
            }
        }
    }

    void Polygon::RemoveBridgeEdges()
    {
        std::set<SEdge> bridgeEdgeSet;
        GetBridgeEdgeSet(bridgeEdgeSet, true);

        if (bridgeEdgeSet.empty())
        {
            return;
        }

        std::vector<SEdge> newEdgeList;
        for (int i = 0, iEdgeCount(m_Edges.size()); i < iEdgeCount; ++i)
        {
            const SEdge& edge = m_Edges[i];
            if (bridgeEdgeSet.find(edge) != bridgeEdgeSet.end())
            {
                continue;
            }
            newEdgeList.push_back(edge);
        }

        std::vector<SVertex> vList(m_Vertices);
        Optimize(vList, newEdgeList);
    }

    void Polygon::GetBridgeEdges(std::vector<BrushEdge3D>& outBridgeEdges) const
    {
        std::set<SEdge> bridgeEdgeSet;
        GetBridgeEdgeSet(bridgeEdgeSet, false);

        if (bridgeEdgeSet.empty())
        {
            return;
        }

        std::set<SEdge>::iterator ii = bridgeEdgeSet.begin();
        for (; ii != bridgeEdgeSet.end(); ++ii)
        {
            outBridgeEdges.push_back(BrushEdge3D(m_Vertices[(*ii).m_i[0]].pos, m_Vertices[(*ii).m_i[1]].pos));
        }
    }

    void Polygon::RemoveEdge(const BrushEdge3D& edge)
    {
        BrushEdge3D invEdge(edge.GetInverted());
        std::vector<BrushEdge3D> subtractedEdges;

        std::vector<SEdge>::iterator ie = m_Edges.begin();
        for (; ie != m_Edges.end(); )
        {
            BrushEdge3D e(m_Vertices[(*ie).m_i[0]].pos, m_Vertices[(*ie).m_i[1]].pos);
            if (edge.IsEquivalent(e) || invEdge.IsEquivalent(e) || e.GetSubtractedEdges(edge, subtractedEdges) || edge.Include(e))
            {
                ie = m_Edges.erase(ie);
            }
            else
            {
                ++ie;
            }
        }

        for (int i = 0, iSize(subtractedEdges.size()); i < iSize; ++i)
        {
            int nEdgeIdx0 = AddVertex(m_Vertices, SVertex(subtractedEdges[i].m_v[0]));
            int nEdgeIdx1 = AddVertex(m_Vertices, SVertex(subtractedEdges[i].m_v[1]));
            m_Edges.push_back(SEdge(nEdgeIdx0, nEdgeIdx1));
        }

        Optimize();
    }

    void Polygon::RemoveEdge(int nEdgeIndex)
    {
        if (nEdgeIndex < 0 || nEdgeIndex >= m_Edges.size() - 1)
        {
            DESIGNER_ASSERT(0);
            return;
        }
        m_Edges.erase(m_Edges.begin() + nEdgeIndex);
        Optimize();
    }

    bool Polygon::FindFirstEdgeIndex(const std::set<int>& edgeSet, int& outEdgeIndex) const
    {
        int iEdgeSize(m_Edges.size());
        std::set<int>::iterator ii = edgeSet.begin();

        for (; ii != edgeSet.end(); ++ii)
        {
            bool bFirstEdge = true;
            for (int k = 0; k < iEdgeSize; ++k)
            {
                if (*ii == k)
                {
                    continue;
                }
                if (m_Edges[*ii].m_i[0] == m_Edges[k].m_i[1])
                {
                    bFirstEdge = false;
                    break;
                }
            }
            if (bFirstEdge)
            {
                outEdgeIndex = *ii;
                return true;
            }
        }

        return false;
    }

    void Polygon::GetUnconnectedPolygons(std::vector<PolygonPtr>& outPolygons)
    {
        if (!IsOpen())
        {
            outPolygons.push_back(this);
            return;
        }

        int iEdgeSize(m_Edges.size());
        int nEdgeIndex = 0;

        std::set<int> leftovers;
        for (int i = 0; i < iEdgeSize; ++i)
        {
            leftovers.insert(i);
        }

        if (!FindFirstEdgeIndex(leftovers, nEdgeIndex))
        {
            DESIGNER_ASSERT(0);
            return;
        }

        int nStartEdgeIndex = nEdgeIndex;
        std::vector<BrushVec3> vList;
        vList.push_back(m_Vertices[m_Edges[nEdgeIndex].m_i[0]].pos);
        vList.push_back(m_Vertices[m_Edges[nEdgeIndex].m_i[1]].pos);

        DESIGNER_ASSERT(leftovers.find(nEdgeIndex) != leftovers.end());
        while (!leftovers.empty())
        {
            std::vector<int> nextEdgeIndices;
            leftovers.erase(nEdgeIndex);
            if (!GetAdjacentNextEdgeIndicesWithVertexIndex(m_Edges[nEdgeIndex].m_i[1], nextEdgeIndices, m_Edges) || nStartEdgeIndex == nextEdgeIndices[0])
            {
                bool bClosed = !nextEdgeIndices.empty() && nStartEdgeIndex == m_Edges[nextEdgeIndices[0]].m_i[0];
                PolygonPtr pPolygon = new Polygon(vList, GetPlane(), m_MaterialID, &m_TexInfo, bClosed);
                pPolygon->SetFlag(GetFlag());
                outPolygons.push_back(pPolygon);
                if (!leftovers.empty())
                {
                    if (!FindFirstEdgeIndex(leftovers, nEdgeIndex))
                    {
                        DESIGNER_ASSERT(0);
                        break;
                    }
                    nStartEdgeIndex = nEdgeIndex;
                    vList.clear();
                    vList.push_back(m_Vertices[m_Edges[nEdgeIndex].m_i[0]].pos);
                    vList.push_back(m_Vertices[m_Edges[nEdgeIndex].m_i[1]].pos);
                }
            }
            else
            {
                nEdgeIndex = nextEdgeIndices[0];
                DESIGNER_ASSERT(leftovers.find(nEdgeIndex) != leftovers.end());
                vList.push_back(m_Vertices[m_Edges[nEdgeIndex].m_i[1]].pos);
            }
        }
    }

    EResultExtract Polygon::ExtractVertexList(int nStartEdgeIdx, int nEndEdgeIdx, std::vector<BrushVec3>& outVertexList, std::vector<int>* pOutVertexIndices) const
    {
        int nCounter = 0;
        int outEdgeIndices[2];
        int nCurrentEdgeIdx = nStartEdgeIdx;
        outVertexList.push_back(m_Vertices[m_Edges[nCurrentEdgeIdx].m_i[0]].pos);
        if (pOutVertexIndices)
        {
            pOutVertexIndices->push_back(m_Edges[nCurrentEdgeIdx].m_i[0]);
        }

        while (GetAdjacentEdgeIndexWithEdgeIndex(nCurrentEdgeIdx, outEdgeIndices[eSD_Previous], outEdgeIndices[eSD_Next], m_Vertices, m_Edges) && outEdgeIndices[eSD_Next] != -1)
        {
            nCurrentEdgeIdx = outEdgeIndices[eSD_Next];

            outVertexList.push_back(m_Vertices[m_Edges[nCurrentEdgeIdx].m_i[0]].pos);
            if (pOutVertexIndices)
            {
                (*pOutVertexIndices).push_back(m_Edges[nCurrentEdgeIdx].m_i[0]);
            }

            if (nCurrentEdgeIdx == nEndEdgeIdx)
            {
                return eRE_EndAtEndVtx;
            }
            else if (nCurrentEdgeIdx == nStartEdgeIdx)
            {
                return eRE_EndAtStartVtx;
            }

            if (++nCounter > m_Edges.size())
            {
                DESIGNER_ASSERT(0 && "Can't find the next edge");
                break;
            }
        }

        return eRE_Fail;
    }

    bool Polygon::ClipByPlane(const BrushPlane& clipPlane, std::vector<PolygonPtr>& pOutFrontPolygons, std::vector<PolygonPtr>& pOutBackPolygons, std::vector<BrushEdge3D>* pOutBoundaryEdges) const
    {
        if (GetPlane().IsEquivalent(clipPlane))
        {
            pOutFrontPolygons.push_back(Clone());
            return false;
        }

        if (GetPlane().GetInverted().IsEquivalent(clipPlane))
        {
            pOutBackPolygons.push_back(Clone());
            return false;
        }

        PolygonPtr pFrontPart = new Polygon;
        PolygonPtr pBackPart = new Polygon;

        if (IsOpen())
        {
            pFrontPart->SetPlane(GetPlane());
            pBackPart->SetPlane(GetPlane());

            for (int i = 0, iEdgeCount(m_Edges.size()); i < iEdgeCount; ++i)
            {
                BrushEdge3D e = GetEdge(i);
                BrushFloat d0 = clipPlane.Distance(e.m_v[0]);
                BrushFloat d1 = clipPlane.Distance(e.m_v[1]);

                if (d0 > kDesignerEpsilon && d1 > kDesignerEpsilon)
                {
                    pFrontPart->AddEdge(e);
                }
                else if (d0 < -kDesignerEpsilon && d1 < -kDesignerEpsilon)
                {
                    pBackPart->AddEdge(e);
                }
                else if ((d0 > kDesignerEpsilon && d1 < -kDesignerEpsilon) || (d1 > kDesignerEpsilon && d0 < -kDesignerEpsilon))
                {
                    BrushVec3 vDir = e.m_v[1] - e.m_v[0];
                    BrushVec3 vHitPos;
                    if (clipPlane.HitTest(e.m_v[0], vDir, NULL, &vHitPos))
                    {
                        if (d0 > kDesignerEpsilon)
                        {
                            pFrontPart->AddEdge(BrushEdge3D(e.m_v[0], vHitPos));
                            pBackPart->AddEdge(BrushEdge3D(vHitPos, e.m_v[1]));
                        }
                        else
                        {
                            pBackPart->AddEdge(BrushEdge3D(e.m_v[0], vHitPos));
                            pFrontPart->AddEdge(BrushEdge3D(vHitPos, e.m_v[1]));
                        }
                    }
                }
            }

            if (pFrontPart->IsValid())
            {
                pOutFrontPolygons.push_back(pFrontPart);
            }
            if (pBackPart->IsValid())
            {
                pOutBackPolygons.push_back(pBackPart);
            }

            return true;
        }

        PolygonPtr pPolygons[] = { pFrontPart, pBackPart };
        std::vector<PolygonPtr>* pOutPolygons[] = { &pOutFrontPolygons, &pOutBackPolygons };

        pFrontPart->m_Plane = pBackPart->m_Plane = m_Plane;
        pFrontPart->m_Flag = pBackPart->m_Flag = m_Flag;
        pFrontPart->m_TexInfo = pBackPart->m_TexInfo = m_TexInfo;
        pFrontPart->m_MaterialID = pBackPart->m_MaterialID = m_MaterialID;

        std::vector<BrushVec3> boundaryVertices;
        std::vector<BrushEdge3D> boundaryEdges;

        for (int i = 0, iEdgeSize(m_Edges.size()); i < iEdgeSize; ++i)
        {
            BrushEdge3D edge = GetEdge(i);
            BrushFloat d0 = clipPlane.Distance(edge.m_v[0]);
            BrushFloat d1 = clipPlane.Distance(edge.m_v[1]);
            if (std::abs(d0) < kDesignerEpsilon)
            {
                d0 = 0;
            }
            if (std::abs(d1) < kDesignerEpsilon)
            {
                d1 = 0;
            }

            if (d0 < 0 && d1 > 0 || d0 > 0 && d1 < 0)
            {
                BrushVec3 vHitPos;

                int nEdgeIndex = 0;
                if (pOutBoundaryEdges && DoesEquivalentExist(*pOutBoundaryEdges, edge, &nEdgeIndex))
                {
                    if ((*pOutBoundaryEdges)[nEdgeIndex].m_v[0].IsEquivalent(edge.m_v[1]))
                    {
                        edge = (*pOutBoundaryEdges)[nEdgeIndex].GetInverted();
                    }
                    else
                    {
                        edge = (*pOutBoundaryEdges)[nEdgeIndex];
                    }
                }

                if (clipPlane.HitTest(edge.m_v[0], edge.m_v[1], NULL, &vHitPos))
                {
                    if (d0 > 0)
                    {
                        pFrontPart->AddEdge(BrushEdge3D(edge.m_v[0], vHitPos));
                        pBackPart->AddEdge(BrushEdge3D(vHitPos, edge.m_v[1]));
                    }
                    else
                    {
                        pFrontPart->AddEdge(BrushEdge3D(vHitPos, edge.m_v[1]));
                        pBackPart->AddEdge(BrushEdge3D(edge.m_v[0], vHitPos));
                    }

                    if (!DoesEquivalentExist(boundaryVertices, vHitPos))
                    {
                        boundaryVertices.push_back(vHitPos);
                    }
                }
            }
            else if (d0 > 0 && d1 >= 0 || d0 >= 0 && d1 > 0)
            {
                pFrontPart->AddEdge(edge);
            }
            else if (d0 < 0 && d1 <= 0 || d0 <= 0 && d1 < 0)
            {
                pBackPart->AddEdge(edge);
            }
        }

        if (pFrontPart->IsOpen() || pBackPart->IsOpen())
        {
            for (int i = 0, iEdgeSize(m_Edges.size()); i < iEdgeSize; ++i)
            {
                BrushEdge3D edge = GetEdge(i);
                BrushFloat d0 = clipPlane.Distance(edge.m_v[0]);
                BrushFloat d1 = clipPlane.Distance(edge.m_v[1]);
                if (std::abs(d0) < kDesignerEpsilon)
                {
                    d0 = 0;
                }
                if (std::abs(d1) < kDesignerEpsilon)
                {
                    d1 = 0;
                }
                if (d0 == 0 && d1 == 0)
                {
                    if (!DoesEquivalentExist(boundaryEdges, edge))
                    {
                        boundaryEdges.push_back(edge);
                    }
                    for (int k = 0; k < 2; ++k)
                    {
                        if (!pPolygons[k]->IsValid() || !pPolygons[k]->IsOpen())
                        {
                            continue;
                        }

                        bool bConnect0to0 = pPolygons[k]->QueryEdges(edge.m_v[0], 0);
                        bool bConnect1to1 = pPolygons[k]->QueryEdges(edge.m_v[1], 1);

                        if (bConnect0to0 || bConnect1to1)
                        {
                            pPolygons[k]->AddEdge(edge.GetInverted());
                        }
                        else
                        {
                            pPolygons[k]->AddEdge(edge);
                        }
                    }
                    if (pOutBoundaryEdges)
                    {
                        (*pOutBoundaryEdges).push_back(edge);
                    }
                }
                else if (d0 == 0)
                {
                    if (!DoesEquivalentExist(boundaryVertices, edge.m_v[0]))
                    {
                        boundaryVertices.push_back(edge.m_v[0]);
                    }
                }
                else if (d1 == 0)
                {
                    if (!DoesEquivalentExist(boundaryVertices, edge.m_v[1]))
                    {
                        boundaryVertices.push_back(edge.m_v[1]);
                    }
                }
            }
        }

        if (pFrontPart->IsOpen() || pBackPart->IsOpen())
        {
            for (int i = 0, iBoundaryVertexCount(boundaryVertices.size()); i < iBoundaryVertexCount; ++i)
            {
                BrushEdge3D vHitEdge(boundaryVertices[i], boundaryVertices[i]);
                if (!DoesEquivalentExist(boundaryEdges, vHitEdge))
                {
                    boundaryEdges.push_back(vHitEdge);
                }
            }

            if (boundaryEdges.size() == 1 && !CD::IsEquivalent(boundaryEdges[0].m_v[0], boundaryEdges[0].m_v[1]))
            {
                boundaryEdges.push_back(BrushEdge3D(boundaryEdges[0].m_v[1], boundaryEdges[0].m_v[1]));
                boundaryEdges[0].m_v[1] = boundaryEdges[0].m_v[0];
            }

            BrushLine3D intersectionLine;
            if (GetPlane().IntersectionLine(clipPlane, intersectionLine))
            {
                std::vector<BrushEdge3D> sortedGapEdges;
                if (SortEdgesAlongCrossLine(boundaryEdges, intersectionLine, sortedGapEdges))
                {
                    for (int i = 0, iSortedGapEdgeCount(sortedGapEdges.size()); i < iSortedGapEdgeCount; ++i)
                    {
                        for (int k = 0; k < 2; ++k)
                        {
                            if (!pPolygons[k]->IsOpen())
                            {
                                continue;
                            }

                            bool bConnect0to0 = pPolygons[k]->QueryEdges(sortedGapEdges[i].m_v[0], 0);
                            bool bConnect1to1 = pPolygons[k]->QueryEdges(sortedGapEdges[i].m_v[1], 1);
                            bool bConnect0to1 = pPolygons[k]->QueryEdges(sortedGapEdges[i].m_v[0], 1);
                            bool bConnect1to0 = pPolygons[k]->QueryEdges(sortedGapEdges[i].m_v[1], 0);

                            if (bConnect0to0 && bConnect1to1 && bConnect0to1 && bConnect1to0)
                            {
                                for (int a = 0; a < 2; ++a)
                                {
                                    BrushEdge3D candidateEdge = a == 0 ? sortedGapEdges[i] : sortedGapEdges[i].GetInverted();
                                    int nEdgeIndex = pPolygons[k]->AddEdge(candidateEdge);
                                    int nNextIndex = -1;
                                    bool bLoopExist = false;
                                    int nCount = 0;
                                    while (pPolygons[k]->GetAdjacentEdgesByEdgeIndex(nEdgeIndex, NULL, &nNextIndex) && nNextIndex != -1)
                                    {
                                        BrushEdge3D nextEdge = pPolygons[k]->GetEdge(nNextIndex);
                                        if (CD::IsEquivalent(nextEdge.m_v[1], sortedGapEdges[i].m_v[0]))
                                        {
                                            bLoopExist = true;
                                            break;
                                        }
                                        if (++nCount > 10000)
                                        {
                                            DESIGNER_ASSERT(0 && "Falled Into an infinite loop");
                                            break;
                                        }
                                        nEdgeIndex = nNextIndex;
                                    }
                                    if (bLoopExist)
                                    {
                                        break;
                                    }
                                    else
                                    {
                                        pPolygons[k]->RemoveEdge(candidateEdge);
                                    }
                                }
                            }
                            else if (bConnect0to0 && bConnect1to1)
                            {
                                pPolygons[k]->AddEdge(sortedGapEdges[i].GetInverted());
                            }
                            else if (bConnect0to1 && bConnect1to0)
                            {
                                pPolygons[k]->AddEdge(sortedGapEdges[i]);
                            }
                            if (pOutBoundaryEdges && !DoesEquivalentExist(*pOutBoundaryEdges, sortedGapEdges[i]))
                            {
                                pOutBoundaryEdges->push_back(sortedGapEdges[i]);
                            }
                        }
                    }
                }
            }
        }

        if (!pFrontPart->IsValid() && !pBackPart->IsValid())
        {
            if (GetPlane().IsSameFacing(clipPlane))
            {
                pOutFrontPolygons.push_back(Clone());
            }
            else
            {
                pOutBackPolygons.push_back(Clone());
            }
            return true;
        }

        for (int k = 0; k < 2; ++k)
        {
            pOutPolygons[k]->clear();

            if (pPolygons[k]->IsValid() && pPolygons[k]->GetEdgeCount() > 2 && pPolygons[k]->GetVertexCount() > 2)
            {
                pPolygons[k]->Optimize();
                if (pPolygons[k]->IsOpen() || !pPolygons[k]->HasHoles())
                {
                    pPolygons[k]->GetSeparatedPolygons(*pOutPolygons[k], eSP_OuterHull);
                }
                else
                {
                    pOutPolygons[k]->push_back(pPolygons[k]);
                }
                DESIGNER_ASSERT(!pOutPolygons[k]->empty());
                if (pOutPolygons[k]->empty())
                {
                    return false;
                }
            }
        }

        return !pOutFrontPolygons.empty() || !pOutBackPolygons.empty() ? true : false;
    }

    bool Polygon::SortVerticesAlongCrossLine(std::vector<BrushVec3>& vList, const BrushLine3D& crossLine3D, std::vector<BrushEdge3D>& outGapEdges) const
    {
        int nIndex = 0;
        if (std::abs(crossLine3D.m_Dir.y) > std::abs(crossLine3D.m_Dir.x) && std::abs(crossLine3D.m_Dir.y) > std::abs(crossLine3D.m_Dir.z))
        {
            nIndex = 1;
        }
        else if (std::abs(crossLine3D.m_Dir.z) > std::abs(crossLine3D.m_Dir.x) && std::abs(crossLine3D.m_Dir.z) > std::abs(crossLine3D.m_Dir.y))
        {
            nIndex = 2;
        }

        if (crossLine3D.m_Dir[nIndex] == 0)
        {
            return false;
        }

        std::map<BrushFloat, BrushVec3> sortedIntersections;

        for (int k = 0, iIntersectionSize(vList.size()); k < iIntersectionSize; ++k)
        {
            BrushFloat t = (vList[k][nIndex] - crossLine3D.m_Pivot[nIndex]) / crossLine3D.m_Dir[nIndex];
            sortedIntersections[t] = vList[k];
        }

        bool bAdded = false;

        std::map<BrushFloat, BrushVec3>::iterator ii = sortedIntersections.begin();
        for (; ii != sortedIntersections.end(); ++ii)
        {
            BrushVec3 vBegin = ii->second;
            ++ii;
            if (ii == sortedIntersections.end())
            {
                break;
            }

            BrushVec3 vEnd = ii->second;
            if (Include((vBegin + vEnd) * 0.5f))
            {
                bAdded = true;
                outGapEdges.push_back(BrushEdge3D(vBegin, vEnd));
            }
        }

        return bAdded;
    }

    bool Polygon::SortEdgesAlongCrossLine(std::vector<BrushEdge3D>& edgeList, const BrushLine3D& crossLine3D, std::vector<BrushEdge3D>& outGapEdges) const
    {
        int nIndex = 0;
        if (std::abs(crossLine3D.m_Dir.y) > std::abs(crossLine3D.m_Dir.x) && std::abs(crossLine3D.m_Dir.y) > std::abs(crossLine3D.m_Dir.z))
        {
            nIndex = 1;
        }
        else if (std::abs(crossLine3D.m_Dir.z) > std::abs(crossLine3D.m_Dir.x) && std::abs(crossLine3D.m_Dir.z) > std::abs(crossLine3D.m_Dir.y))
        {
            nIndex = 2;
        }

        if (crossLine3D.m_Dir[nIndex] == 0)
        {
            return false;
        }

        std::map<BrushFloat, BrushEdge3D> sortedIntersections;

        for (int k = 0, iIntersectionSize(edgeList.size()); k < iIntersectionSize; ++k)
        {
            const BrushVec3& v0 = edgeList[k].m_v[0];
            const BrushVec3& v1 = edgeList[k].m_v[1];
            BrushFloat t0 = (v0[nIndex] - crossLine3D.m_Pivot[nIndex]) / crossLine3D.m_Dir[nIndex];
            BrushFloat t1 = (v1[nIndex] - crossLine3D.m_Pivot[nIndex]) / crossLine3D.m_Dir[nIndex];

            if (t0 <= t1)
            {
                sortedIntersections[t0] = BrushEdge3D(v0, v1);
            }
            else
            {
                sortedIntersections[t1] = BrushEdge3D(v1, v0);
            }
        }

        bool bAdded = false;

        std::map<BrushFloat, BrushEdge3D>::iterator ii = sortedIntersections.begin();
        for (; ii != sortedIntersections.end(); )
        {
            const BrushEdge3D& vBegin = ii->second;
            ++ii;
            if (ii == sortedIntersections.end())
            {
                break;
            }
            const BrushEdge3D& vEnd = ii->second;

            if (Include((vBegin.m_v[1] + vEnd.m_v[0]) * 0.5f))
            {
                bAdded = true;
                outGapEdges.push_back(BrushEdge3D(vBegin.m_v[1], vEnd.m_v[0]));
            }
        }

        return bAdded;
    }

    PolygonPtr Polygon::Mirror(const BrushPlane& mirrorPlane)
    {
        if (m_Vertices.empty())
        {
            return NULL;
        }

        for (int i = 0, iVertexSize(m_Vertices.size()); i < iVertexSize; ++i)
        {
            m_Vertices[i] = SVertex(mirrorPlane.MirrorVertex(m_Vertices[i].pos), m_Vertices[i].uv);
        }

        ReverseEdges();

        AddFlags(ePolyFlag_Mirrored);
        m_Plane = mirrorPlane.MirrorPlane(m_Plane);

        return this;
    }

    bool Polygon::GetComputedPlane(BrushPlane& outPlane) const
    {
        std::vector<SVertex> vLinkedVertices;
        if (m_Vertices.empty() || !GetLinkedVertices(vLinkedVertices))
        {
            return false;
        }

        if (CheckFlags(ePolyFlag_NonplanarQuad))
        {
            DESIGNER_ASSERT(vLinkedVertices.size() == 4);
            if (vLinkedVertices.size() != 4)
            {
                return false;
            }

            BrushPlane p0, p1;

            std::vector<SVertex> face0;
            std::vector<SVertex> face1;

            face0.push_back(vLinkedVertices[0]);
            face0.push_back(vLinkedVertices[1]);
            face0.push_back(vLinkedVertices[2]);

            face1.push_back(vLinkedVertices[0]);
            face1.push_back(vLinkedVertices[2]);
            face1.push_back(vLinkedVertices[3]);

            if (ComputePlane(face0, p0) && ComputePlane(face1, p1))
            {
                BrushVec3 center = (vLinkedVertices[0].pos + vLinkedVertices[1].pos + vLinkedVertices[2].pos + vLinkedVertices[3].pos) / 4;
                BrushVec3 normal = (p0.Normal() + p1.Normal()).GetNormalized();
                BrushFloat distance = -normal.Dot(center);
                outPlane = BrushPlane(normal, distance);
                return true;
            }
            else
            {
                return false;
            }
        }

        return ComputePlane(vLinkedVertices, outPlane);
    }

    bool Polygon::InRectangle(IDisplayViewport* pView, const BrushMatrix34& worldTM, PolygonPtr pRectPolygon, bool bExcludeBackFace)
    {
        PolygonPtr pViewPolygon = Convert2ViewPolygon(pView, worldTM, this, NULL);
        if (!pViewPolygon)
        {
            return false;
        }

        if (bExcludeBackFace && pViewPolygon->GetPlane().Normal().z > 0)
        {
            return false;
        }

        if (!pViewPolygon->GetBoundBox().IsIntersectBox(pRectPolygon->GetBoundBox()))
        {
            return false;
        }

        if (!pViewPolygon->GetPlane().IsEquivalent(pRectPolygon->GetPlane()))
        {
            pRectPolygon->Flip();
        }

        if (Polygon::HasIntersection(pViewPolygon, pRectPolygon) == eIT_Intersection)
        {
            return true;
        }

        return false;
    }

    void Polygon::QueryIntersectionEdgesWith2DRect(IDisplayViewport* pView, const BrushMatrix34& worldTM, PolygonPtr pRectPolygon, bool bExcludeBackFace, EdgeQueryResult& outIntersectionEdges)
    {
        PolygonPtr pPolygon = NULL;
        PolygonPtr pViewPolygon = Convert2ViewPolygon(pView, worldTM, this, &pPolygon);
        if (!pViewPolygon)
        {
            return;
        }

        if (bExcludeBackFace && pViewPolygon->GetPlane().Normal().z > 0)
        {
            return;
        }

        const AABB& viewPolygonAABB = pViewPolygon->GetBoundBox();
        const AABB& rectAABB = pRectPolygon->GetBoundBox();
        if (!viewPolygonAABB.IsIntersectBox(rectAABB))
        {
            return;
        }

        for (int i = 0, iEdgeCount(pViewPolygon->GetEdgeCount()); i < iEdgeCount; ++i)
        {
            BrushEdge3D ve = pViewPolygon->GetEdge(i);
            BSPTree2D::SOutputEdges partitions;
            pRectPolygon->GetBSPTree()->GetPartitions(ve, partitions);

            if (partitions.negList.size() == 1)
            {
                BrushVec3 vCenter = (partitions.negList[0].m_v[0] + partitions.negList[0].m_v[1]) * (BrushFloat)0.5;
                BrushFloat edgeDistance = ve.m_v[1].GetDistance(ve.m_v[0]);
                if (edgeDistance > 0)
                {
                    BrushFloat t = vCenter.GetDistance(ve.m_v[0]) / edgeDistance;
                    BrushEdge3D e;
                    if (pPolygon)
                    {
                        e = pPolygon->GetEdge(i);
                        int nEdgeIndex = 0;
                        if (IsEdgeOnCrust(e, &nEdgeIndex))
                        {
                            e = GetEdge(nEdgeIndex);
                        }
                    }
                    else
                    {
                        e = GetEdge(i);
                    }
                    outIntersectionEdges.push_back(std::pair<BrushEdge3D, BrushVec3>(e, e.m_v[0] + (e.m_v[1] - e.m_v[0]) * t));
                }
            }
        }
    }

    void Polygon::InvalidateBspTree() const
    {
        if (m_pBSPTree)
        {
            m_pBSPTree->Release();
        }
        m_pBSPTree = NULL;
    }

    void Polygon::InvalidateConvexes() const
    {
        if (m_pConvexes)
        {
            m_pConvexes->Release();
        }
        m_pConvexes = NULL;
    }

    void Polygon::InvalidateTriangles() const
    {
        if (m_pTriangles)
        {
            m_pTriangles->Release();
        }
        m_pTriangles = NULL;
    }

    void Polygon::InvalidateRepresentativePos() const
    {
        if (!m_pRepresentativePos)
        {
            return;
        }
        delete m_pRepresentativePos;
        m_pRepresentativePos = NULL;
    }

    void Polygon::InvalidateBoundBox() const
    {
        if (!m_pBoundBox)
        {
            return;
        }
        delete m_pBoundBox;
        m_pBoundBox = NULL;
    }

    Convexes* Polygon::GetConvexes()
    {
        if (!m_pConvexes)
        {
            m_pConvexes = new Convexes;
            m_pConvexes->AddRef();
            PolygonDecomposer decomposer;
            decomposer.DecomposeToConvexes(this, *m_pConvexes);
        }
        return m_pConvexes;
    }

    FlexibleMesh* Polygon::GetTriangles(bool bGenerateBackFaces) const
    {
        if (m_pTriangles && CheckPrivateFlags(eRPF_EnabledBackFaces) != bGenerateBackFaces)
        {
            InvalidateTriangles();
        }

        if (!m_pTriangles)
        {
            m_pTriangles = new FlexibleMesh;
            m_pTriangles->AddRef();
            if (bGenerateBackFaces)
            {
                AddPrivateFlags(eRPF_EnabledBackFaces);
            }
            else
            {
                RemovePrivateFlags(eRPF_EnabledBackFaces);
            }
            CreateMeshFacesFromPolygon(Clone(), *m_pTriangles, bGenerateBackFaces);
        }

        return m_pTriangles;
    }

    bool Polygon::GetQuadDiagonal(BrushEdge3D& outEdge) const
    {
        if (!IsQuad())
        {
            return false;
        }

        FlexibleMesh* pTriangles = GetTriangles(false);
        DESIGNER_ASSERT(pTriangles->faceList.size() == 2);

        for (int i = 0; i < 3; ++i)
        {
            for (int k = 0; k < 3; ++k)
            {
                if (pTriangles->faceList[0].v[i] == pTriangles->faceList[1].v[(k + 1) % 3] && pTriangles->faceList[0].v[(i + 1) % 3] == pTriangles->faceList[1].v[k])
                {
                    outEdge = BrushEdge3D(pTriangles->vertexList[pTriangles->faceList[0].v[i]].pos, pTriangles->vertexList[pTriangles->faceList[0].v[(i + 1) % 3]].pos);
                    return true;
                }
            }
        }

        return false;
    }

    bool Polygon::FindSharingEdge(PolygonPtr pPolygon, BrushEdge3D& outEdge) const
    {
        for (int i = 0, iEdgeCount0(GetEdgeCount()); i < iEdgeCount0; ++i)
        {
            BrushEdge3D e0 = GetEdge(i).GetInverted();

            for (int k = 0, iEdgeCount1(pPolygon->GetEdgeCount()); k < iEdgeCount1; ++k)
            {
                BrushEdge3D e1 = pPolygon->GetEdge(k);
                if (e0.IsEquivalent(e1))
                {
                    outEdge = e0;
                    return true;
                }
            }
        }
        return false;
    }

    bool Polygon::FindOppositeEdge(const BrushEdge3D& edge, BrushEdge3D& outEdge) const
    {
        int nEdgeCount = GetEdgeCount();
        if (nEdgeCount == 4)
        {
            for (int a = 0; a < nEdgeCount; ++a)
            {
                BrushEdge3D e3D(GetEdge(a));
                if (!CD::IsEquivalent(e3D.m_v[0], edge.m_v[0]) && !CD::IsEquivalent(e3D.m_v[0], edge.m_v[1]) &&
                    !CD::IsEquivalent(e3D.m_v[1], edge.m_v[0]) && !CD::IsEquivalent(e3D.m_v[1], edge.m_v[1]))
                {
                    BrushVec3 vDir0 = edge.m_v[1] - edge.m_v[0];
                    BrushVec3 vDir00 = e3D.m_v[0] - edge.m_v[0];
                    BrushVec3 vDir01 = e3D.m_v[1] - edge.m_v[0];
                    if (vDir0.Dot(vDir00) > vDir0.Dot(vDir01))
                    {
                        outEdge = e3D.GetInverted();
                    }
                    else
                    {
                        outEdge = e3D;
                    }
                    return true;
                }
            }
        }
        else if (nEdgeCount == 3)
        {
            for (int a = 0, iVertexCount(GetVertexCount()); a < iVertexCount; ++a)
            {
                const BrushVec3& v = GetPos(a);
                if (!CD::IsEquivalent(v, edge.m_v[0]) && !CD::IsEquivalent(v, edge.m_v[1]))
                {
                    outEdge = BrushEdge3D(v, v);
                    return true;
                }
            }
        }

        return false;
    }

    void Polygon::UpdateUVs()
    {
        for (int i = 0, iCount(m_Vertices.size()); i < iCount; ++i)
        {
            CD::CalcTexCoords(SBrushPlane<float>(ToVec3(GetPlane().Normal()), 0), GetTexInfo(), ToVec3(m_Vertices[i].pos), m_Vertices[i].uv.x, m_Vertices[i].uv.y);
        }
    }
}