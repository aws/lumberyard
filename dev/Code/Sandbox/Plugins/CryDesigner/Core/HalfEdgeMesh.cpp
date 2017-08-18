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
#include "HalfEdgeMesh.h"
#include "Convexes.h"
#include "Model.h"
#include "Util/EdgesSharpnessManager.h"

namespace CD
{
    const HE_Edge& HalfEdgeMesh::GetPrevEdge(const HE_Edge& e) const
    {
        const HE_Edge* e_variable = &e;
        while (m_Edges[e_variable->next_edge].vertex != e.vertex)
        {
            e_variable = &m_Edges[e_variable->next_edge];
        }
        return *e_variable;
    }

    void HalfEdgeMesh::ConstructMesh(CD::Model* pModel)
    {
        Clear();

        EdgeSharpnessManager* pEdgeSharpnessMgr = pModel->GetEdgeSharpnessMgr();
        MODEL_SHELF_RECONSTRUCTOR(pModel);

        for (CD::ShelfID id = 0; id < CD::kMaxShelfCount; ++id)
        {
            pModel->SetShelf(id);
            for (int i = 0, iPolygonCount(pModel->GetPolygonCount()); i < iPolygonCount; ++i)
            {
                CD::PolygonPtr pPolygon = pModel->GetPolygon(i);
                if (pPolygon->IsOpen())
                {
                    continue;
                }
                Convexes* pConvexList = pPolygon->GetConvexes();
                SolveTJunction(pModel, pPolygon, pConvexList);
                for (int k = 0, iConvexCount(pConvexList->GetConvexCount()); k < iConvexCount; ++k)
                {
                    AddConvex(pPolygon, pConvexList->GetConvex(k));
                }
            }
        }

        for (int i = 0, iEdgeCount(m_Edges.size()); i < iEdgeCount; ++i)
        {
            BrushEdge3D e(GetPos(m_Edges[i]), GetPos(m_Edges[m_Edges[i].next_edge]));
            m_Edges[i].sharpness = pEdgeSharpnessMgr->FindSharpness(e);
            m_Edges[i].irregular = m_Edges[i].sharpness > kDesignerEpsilon;
        }

        FindEachPairEdge();
        ConstructEdgeSharpnessTable(pModel);
    }

    void HalfEdgeMesh::AddConvex(CD::PolygonPtr pPolygon, const std::vector<SVertex>& vConvex)
    {
        int nStartEdgeIndex = (int)m_Edges.size();
        int nFaceIndex = (int)m_Faces.size();

        for (int i = 0, iVListCount(vConvex.size()); i < iVListCount; ++i)
        {
            int nVertexIndex = (int)m_Vertices.size();
            int nEdgeIndex = (int)m_Edges.size();

            HE_Vertex v;
            v.pos_index = AddPos(vConvex[i].pos);
            v.edge = nEdgeIndex;
            m_Vertices.push_back(v);

            HE_Edge e;
            e.next_edge = (i == iVListCount - 1) ? nStartEdgeIndex : (nEdgeIndex + 1);
            e.face = nFaceIndex;
            e.vertex = nVertexIndex;
            m_Edges.push_back(e);
        }

        CD::HE_Face f;
        f.edge = nStartEdgeIndex;
        f.pOriginPolygon = pPolygon;
        m_Faces.push_back(f);
    }

    int HalfEdgeMesh::AddPos(const BrushVec3& vPos)
    {
        for (int i = 0, iCount(m_Positions.size()); i < iCount; ++i)
        {
            if (CD::IsEquivalent(m_Positions[i].pos, vPos))
            {
                return i;
            }
        }
        m_Positions.push_back(vPos);
        return m_Positions.size() - 1;
    }

    void HalfEdgeMesh::FindEachPairEdge()
    {
        int nEdgeCount(m_Edges.size());

        for (int i = 0; i < nEdgeCount; ++i)
        {
            if (m_Edges[i].pair_edge != -1)
            {
                continue;
            }
            std::pair<int, int> e0(GetVertex(m_Edges[i].vertex).pos_index, GetVertex(m_Edges[m_Edges[i].next_edge].vertex).pos_index);
            for (int k = 0; k < nEdgeCount; ++k)
            {
                if (i == k || m_Edges[k].pair_edge != -1 || m_Edges[i].face == m_Edges[k].face)
                {
                    continue;
                }
                std::pair<int, int> e1(GetVertex(m_Edges[k].vertex).pos_index, GetVertex(m_Edges[m_Edges[k].next_edge].vertex).pos_index);
                if (e0.first == e1.second && e0.second == e1.first)
                {
                    m_Edges[i].pair_edge = k;
                    m_Edges[k].pair_edge = i;
                    break;
                }
            }
        }
    }

    void HalfEdgeMesh::ConstructEdgeSharpnessTable(CD::Model* pModel)
    {
        EdgeSharpnessManager* pSharpnessMgr = pModel->GetEdgeSharpnessMgr();
        for (int i = 0, iEdgeCount(m_Edges.size()); i < iEdgeCount; ++i)
        {
            BrushEdge3D e = GetRealEdge(m_Edges[i]);
            m_Edges[i].sharpness = pSharpnessMgr->FindSharpness(e);
        }

        for (int i = 0, iVertexCount(m_Vertices.size()); i < iVertexCount; ++i)
        {
            HE_Vertex& v = m_Vertices[i];
            HE_Edge& e = m_Edges[m_Vertices[i].edge];
            if (e.sharpness > 0)
            {
                m_Positions[v.pos_index].edges.push_back(m_Vertices[i].edge);
            }
        }
    }

    BrushVec3 HalfEdgeMesh::GetFaceAveragePos(const CD::HE_Face& f) const
    {
        BrushVec3 vSum(0, 0, 0);
        int nStartingEdge = f.edge;
        int nEdge = nStartingEdge;
        int nCount = 0;
        do
        {
            const HE_Edge& e = GetEdge(nEdge);
            vSum += GetPos(e);
            nEdge = e.next_edge;
            ++nCount;
        } while (nEdge != nStartingEdge);
        return vSum / (BrushFloat)nCount;
    }

    void HalfEdgeMesh::GetFaceVertices(const CD::HE_Face& f, std::vector<BrushVec3>& outVertices) const
    {
        int nStartingEdge = f.edge;
        int nEdge = nStartingEdge;
        outVertices.reserve(4);
        do
        {
            const HE_Edge& e = GetEdge(nEdge);
            outVertices.push_back(GetPos(e));
            nEdge = e.next_edge;
        } while (nEdge != nStartingEdge);
    }

    void HalfEdgeMesh::GetFacePosIndices(const CD::HE_Face& f, std::vector<int>& outPosIndices) const
    {
        int nStartingEdge = f.edge;
        int nEdge = nStartingEdge;
        outPosIndices.reserve(4);
        do
        {
            const HE_Edge& e = GetEdge(nEdge);
            outPosIndices.push_back(m_Vertices[e.vertex].pos_index);
            nEdge = e.next_edge;
        } while (nEdge != nStartingEdge);
    }

    void HalfEdgeMesh::CreateMeshFaces(std::vector<CD::FlexibleMeshPtr>& outMeshes, bool bGenerateBackFaces, bool bEnableSmoothingSurface)
    {
        int nFaceCount = m_Faces.size();
        std::vector<BrushVec3> vNormals;

        if (bEnableSmoothingSurface)
        {
            CalculateFaceSmoothingNormals(vNormals);
        }

        CD::FlexibleMesh* m = NULL;
        for (int i = 0; i < nFaceCount; ++i)
        {
            std::vector<int> posIndices;
            const HE_Face& f = m_Faces[i];
            GetFacePosIndices(f, posIndices);

            int estimatedVertexCount = 0;
            if (m != NULL)
            {
                estimatedVertexCount = m->vertexList.size() + posIndices.size();
                if (bGenerateBackFaces)
                {
                    estimatedVertexCount += posIndices.size();
                }
            }

            if (m == NULL || estimatedVertexCount > 0xffff)
            {
                outMeshes.push_back(new CD::FlexibleMesh());
                m = outMeshes[outMeshes.size() - 1];
            }

            BrushVec3 vFaceNormal = (m_Positions[posIndices[0]].pos - m_Positions[posIndices[1]].pos).Cross(m_Positions[posIndices[2]].pos - m_Positions[posIndices[1]].pos).GetNormalized();
            for (int a = 0; a < 2; ++a)
            {
                if (a == 1 && !bGenerateBackFaces)
                {
                    break;
                }

                if (a == 1)
                {
                    std::reverse(posIndices.begin(), posIndices.end());
                }

                int nVertexOffset = m->vertexList.size();
                int iPosIndicesCount(posIndices.size());

                for (int k = 0; k < iPosIndicesCount; ++k)
                {
                    const BrushVec3& vNormal = bEnableSmoothingSurface ? vNormals[posIndices[k]] : vFaceNormal;
                    const BrushVec3& vPos = m_Positions[posIndices[k]].pos;

                    CD::SVertex v(vPos);
                    CD::CalcTexCoords(SBrushPlane<float>(ToVec3(vFaceNormal), 0), f.pOriginPolygon ? f.pOriginPolygon->GetTexInfo() : STexInfo(), ToVec3(vPos), v.uv.x, v.uv.y);
                    m->vertexList.push_back(v);
                    m->normalList.push_back(vNormal);
                }

                SMeshFace mf;
                mf.nSubset = m->AddMatID(f.pOriginPolygon->GetMaterialID());
                for (int k = 0; k < iPosIndicesCount - 2; ++k)
                {
                    mf.v[0] = nVertexOffset;
                    mf.v[1] = nVertexOffset + k + 1;
                    mf.v[2] = nVertexOffset + k + 2;
                    m->faceList.push_back(mf);
                }
            }
        }
    }

    void HalfEdgeMesh::CalculateFaceSmoothingNormals(std::vector<BrushVec3>& outNormals)
    {
        int nPosCount = m_Positions.size();
        int nFaceCount = m_Faces.size();

        outNormals.resize(nPosCount);
        for (int i = 0; i < nPosCount; ++i)
        {
            outNormals[i] = BrushVec3(0, 0, 0);
        }

        for (int i = 0; i < nFaceCount; ++i)
        {
            const HE_Face& f = m_Faces[i];
            std::vector<BrushVec3> vList;
            GetFaceVertices(f, vList);
            BrushVec3 vNormal = (vList[0] - vList[1]).Cross(vList[2] - vList[1]).GetNormalized();
            int e = f.edge;
            do
            {
                const HE_Edge& edge = GetEdge(e);
                const HE_Vertex& vertex = GetVertex(edge);
                outNormals[vertex.pos_index] += vNormal;
                e = edge.next_edge;
            } while (e != f.edge);
        }
        for (int i = 0; i < nPosCount; ++i)
        {
            outNormals[i].Normalize();
        }
    }

    CD::Model* HalfEdgeMesh::CreateModel()
    {
        int nFaceCount = m_Faces.size();
        if (nFaceCount == 0)
        {
            return NULL;
        }

        CD::Model* pModel = new CD::Model;

        pModel->SetShelf(0);
        for (int i = 0; i < nFaceCount; ++i)
        {
            const HE_Face& f = m_Faces[i];
            std::vector<BrushVec3> vList;
            GetFaceVertices(f, vList);

            CD::PolygonPtr pPolygon = new CD::Polygon(vList);
            pPolygon->SetTexInfo(f.pOriginPolygon->GetTexInfo());
            pPolygon->SetMaterialID(f.pOriginPolygon->GetMaterialID());
            pModel->AddPolygon(pPolygon, CD::eOpType_Add);
        }

        return pModel;
    }

    void FindPointBetweenEdge(const BrushEdge3D& edge, CD::PolygonPtr pPolygon, std::vector<CD::PolygonPtr>& neighbourPolygons, std::map<BrushFloat, BrushVec3>& outPoints)
    {
        AABB edgeAABB;
        edgeAABB.Reset();
        edgeAABB.Add(ToVec3(edge.m_v[0]));
        edgeAABB.Add(ToVec3(edge.m_v[1]));
        edgeAABB.Expand(Vec3(0.01f, 0.01f, 0.01f));

        BrushLine3D edgeLine(edge.m_v[0], edge.m_v[1]);

        for (int i = 0, iNeighbourPolygonCount(neighbourPolygons.size()); i < iNeighbourPolygonCount; ++i)
        {
            CD::PolygonPtr pNeighbourPolygon = neighbourPolygons[i];
            if (pPolygon == pNeighbourPolygon || pNeighbourPolygon->IsOpen())
            {
                continue;
            }

            for (int k = 0, iEdgeCount(neighbourPolygons[i]->GetEdgeCount()); k < iEdgeCount; ++k)
            {
                BrushEdge3D neighbourEdge = pNeighbourPolygon->GetEdge(k);
                if (!edgeAABB.IsContainPoint(neighbourEdge.m_v[0]) && !edgeAABB.IsContainPoint(neighbourEdge.m_v[1]))
                {
                    continue;
                }

                BrushFloat d0 = edgeLine.GetDistance(neighbourEdge.m_v[0]);
                BrushFloat d1 = edgeLine.GetDistance(neighbourEdge.m_v[1]);

                if (d0 > kDesignerEpsilon || d1 > kDesignerEpsilon)
                {
                    continue;
                }

                BrushEdge3D intersectedEdge;
                if (CD::IntersectEdge3D(edge, neighbourEdge, intersectedEdge) == eOR_One)
                {
                    for (int a = 0; a < 2; ++a)
                    {
                        if (CD::IsEquivalent(edge.m_v[0], intersectedEdge.m_v[a]) || CD::IsEquivalent(edge.m_v[1], intersectedEdge.m_v[a]))
                        {
                            continue;
                        }
                        BrushFloat dist = edge.m_v[0].GetDistance(intersectedEdge.m_v[a]);
                        outPoints[dist] = intersectedEdge.m_v[a];
                    }
                }
            }
        }
    }

    void HalfEdgeMesh::SolveTJunction(CD::Model* pModel, CD::PolygonPtr pPolygon, Convexes* pConvexes)
    {
        AABB bbox = pPolygon->GetBoundBox();
        bbox.Expand(Vec3(0.01f, 0.01f, 0.01f));
        std::vector<CD::PolygonPtr> neighbourPolygons;

        {
            MODEL_SHELF_RECONSTRUCTOR(pModel);
            for (int i = 0; i < CD::kMaxShelfCount; ++i)
            {
                pModel->SetShelf(i);
                pModel->QueryIntersectedPolygonsByAABB(bbox, neighbourPolygons);
            }
            if (neighbourPolygons.empty())
            {
                return;
            }
        }

        for (int i = 0, iConvexCount(pConvexes->GetConvexCount()); i < iConvexCount; ++i)
        {
            CD::Convex updatedConvex;
            CD::Convex& convex = pConvexes->GetConvex(i);
            int iVertexCount(convex.size());
            updatedConvex.reserve(iVertexCount);
            for (int k = 0; k < iVertexCount; ++k)
            {
                updatedConvex.push_back(convex[k]);

                BrushEdge3D e(convex[k].pos, convex[(k + 1) % iVertexCount].pos);
                std::map<BrushFloat, BrushVec3> pointsOnEdge;
                FindPointBetweenEdge(e, pPolygon, neighbourPolygons, pointsOnEdge);
                if (pointsOnEdge.empty())
                {
                    continue;
                }

                std::map<BrushFloat, BrushVec3>::iterator iiForPoionts = pointsOnEdge.begin();
                for (; iiForPoionts != pointsOnEdge.end(); ++iiForPoionts)
                {
                    updatedConvex.push_back(CD::SVertex(iiForPoionts->second));
                }
            }
            if (updatedConvex.size() > convex.size())
            {
                convex = updatedConvex;
            }
        }
    }

    const HE_Edge* HalfEdgeMesh::FindNextEdgeClockwiseAroundVertex(const HE_Edge& edge) const
    {
        const HE_Edge* pPairEdge = GetPairEdge(edge);
        if (pPairEdge == NULL)
        {
            return NULL;
        }
        return &m_Edges[pPairEdge->next_edge];
    }

    const HE_Edge& HalfEdgeMesh::FindEndEdgeCounterClockwiseAroundVertex(const HE_Edge& edge) const
    {
        const HE_Edge* pEdge = &edge;
        const HE_Edge* pPrevEdge = NULL;
        do
        {
            pPrevEdge = &(GetPrevEdge(*pEdge));
            const HE_Edge* pPairOfPrevEdge = GetPairEdge(*pPrevEdge);
            if (pPairOfPrevEdge == NULL)
            {
                break;
            }
            pEdge = pPairOfPrevEdge;
        } while (pEdge != &edge);
        return *pPrevEdge;
    }

    bool HalfEdgeMesh::IsIrregularFace(const HE_Face& f) const
    {
        int eindex = -1;

        while (eindex != f.edge)
        {
            if (eindex == -1)
            {
                eindex = f.edge;
            }

            if (GetEdge(eindex).irregular)
            {
                return true;
            }

            const HE_Vertex& v = GetVertex(GetEdge(eindex));
            int nValenceCount = 0;
            if (GetValenceCount(v, nValenceCount) && nValenceCount != 4)
            {
                return true;
            }

            eindex = GetEdge(eindex).next_edge;
        }

        return false;
    }

    bool HalfEdgeMesh::GetValenceCount(const HE_Vertex& v, int& nOutValenceCount) const
    {
        const HE_Edge* pFirstEdge = &GetEdge(v);
        const HE_Edge* pEdge = NULL;
        int nValenceCount = 0;

        while (pEdge != pFirstEdge)
        {
            if (pEdge == NULL)
            {
                pEdge = pFirstEdge;
            }

            ++nValenceCount;

            pEdge = FindNextEdgeClockwiseAroundVertex(*pEdge);
            if (pEdge == NULL)
            {
                return false;
            }
        }

        nOutValenceCount = nValenceCount;
        return true;
    }
};