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

#include "Polygon.h"

namespace CD
{
    class Model;

    struct HE_Edge
    {
        HE_Edge()
            : vertex(-1)
            , pair_edge(-1)
            , face(-1)
            , next_edge(-1)
            , sharpness(0)
            , irregular(false)
        {
        }
        HE_Edge(int _vertex, int _pair_edge, int _face, int _next_edge, float _sharpness, bool _irregular)
            : vertex(_vertex)
            , pair_edge(_pair_edge)
            , face(_face)
            , next_edge(_next_edge)
            , sharpness(_sharpness)
            , irregular(_irregular)
        {
        }
        int vertex;
        int pair_edge;
        int face;
        int next_edge;
        float sharpness;
        bool irregular;
    };

    struct HE_Vertex
    {
        HE_Vertex()
            : pos_index(-1)
            , edge(-1)
        {
        }
        HE_Vertex(const int& _pos_index, int _edge)
            : pos_index(_pos_index)
            , edge(_edge)
        {
        }
        int pos_index;
        int edge;
    };

    struct HE_Position
    {
        HE_Position()
        {
        }
        HE_Position(const BrushVec3& _pos)
            : pos(_pos)
        {
        }
        BrushVec3 pos;
        std::vector<int> edges;
        float GetSharpness(const std::vector<HE_Edge>& he_edges) const
        {
            float sharpnessSum = 0;
            int iEdgeCount = edges.size();
            for (int i = 0; i < iEdgeCount; ++i)
            {
                sharpnessSum += he_edges[edges[i]].sharpness;
            }
            return sharpnessSum / (float)iEdgeCount;
        }
    };

    struct HE_Face
    {
        HE_Face()
            : edge(-1)
        {
        }
        HE_Face(int _edge, CD::PolygonPtr _pOriginPolygon)
            : edge(_edge)
            , pOriginPolygon(_pOriginPolygon)
        {
        }
        int edge;
        CD::PolygonPtr pOriginPolygon;
    };

    class HalfEdgeMesh
        : public CRefCountBase
    {
    public:

        void ConstructMesh(CD::Model* pModel);

        int GetVertexCount() const { return m_Vertices.size(); }
        int GetPosCount() const { return m_Positions.size(); }
        int GetEdgeCount() const { return m_Edges.size(); }
        int GetFaceCount() const { return m_Faces.size(); }

        bool IsIrregularFace(const HE_Face& f) const;

        const HE_Vertex& GetVertex(int nIndex) const { return m_Vertices[nIndex]; }
        const HE_Vertex& GetVertex(const HE_Face& f) const { return m_Vertices[m_Edges[f.edge].vertex]; }
        const HE_Vertex& GetVertex(const HE_Edge& e) const { return m_Vertices[e.vertex]; }

        const BrushVec3& GetPos(int vertex_index) const { return m_Positions[m_Vertices[vertex_index].pos_index].pos; }
        const BrushVec3& GetPos(const HE_Vertex& v) const { return m_Positions[v.pos_index].pos; }
        const BrushVec3& GetPos(const HE_Face& f) const { return m_Positions[m_Vertices[m_Edges[f.edge].vertex].pos_index].pos; }
        const BrushVec3& GetPos(const HE_Edge& e) const { return m_Positions[m_Vertices[e.vertex].pos_index].pos; }
        const HE_Position& GetPosition(const HE_Vertex& v) const { return m_Positions[v.pos_index]; }

        const HE_Edge& GetEdge(int nIndex) const { return m_Edges[nIndex]; }
        const HE_Edge& GetEdge(const HE_Face& f) const { return m_Edges[f.edge]; }
        const HE_Edge& GetEdge(const HE_Vertex& v) const { return m_Edges[v.edge]; }
        BrushEdge3D GetRealEdge(const HE_Edge& e) const { return BrushEdge3D(GetPos(e), GetPos(e.next_edge)); }

        const HE_Edge& GetNextEdge(const HE_Edge& e) const { return m_Edges[e.next_edge]; }
        const HE_Edge& GetPrevEdge(const HE_Edge& e) const;
        const HE_Edge* GetPairEdge(const HE_Edge& e) const
        {
            if (e.pair_edge == -1)
            {
                return NULL;
            }
            return &m_Edges[e.pair_edge];
        }

        const HE_Face& GetFace(int nIndex) const { return m_Faces[nIndex]; }
        const HE_Face& GetFace(const HE_Edge& e) const { return m_Faces[e.face]; }

        bool GetValenceCount(const HE_Vertex& v, int& nOutValenceCount) const;

        BrushVec3 GetFaceAveragePos(const HE_Face& f) const;
        void GetFaceVertices(const CD::HE_Face& f, std::vector<BrushVec3>& outVertices) const;
        void GetFacePosIndices(const CD::HE_Face& f, std::vector<int>& outPosIndices) const;

        std::vector<HE_Vertex>& GetVertices() { return m_Vertices; }
        std::vector<HE_Position>& GetPositions() { return m_Positions; }
        std::vector<HE_Edge>& GetEdges() { return m_Edges; }
        std::vector<HE_Face>& GetFaces() { return m_Faces; }

        const HE_Edge* FindNextEdgeClockwiseAroundVertex(const HE_Edge& edge) const;
        const HE_Edge& FindEndEdgeCounterClockwiseAroundVertex(const HE_Edge& edge) const;

        void CreateMeshFaces(std::vector<CD::FlexibleMeshPtr>& outMeshes, bool bGenerateBackFaces, bool bEnableSmoothingSurface);
        CD::Model* CreateModel();

        int AddPos(const BrushVec3& vPos);

    private:

        void CalculateFaceSmoothingNormals(std::vector<BrushVec3>& outNormals);
        void SolveTJunction(CD::Model* pModel, CD::PolygonPtr pPolygon, Convexes* pConvexes);
        void FindEachPairEdge();
        void ConstructEdgeSharpnessTable(CD::Model* pModel);
        void AddConvex(CD::PolygonPtr pPolygon, const std::vector<SVertex>& vConvex);
        void Clear()
        {
            m_Vertices.clear();
            m_Positions.clear();
            m_Edges.clear();
            m_Faces.clear();
        }

        std::vector<HE_Vertex> m_Vertices;
        std::vector<HE_Position> m_Positions;
        std::vector<HE_Edge> m_Edges;
        std::vector<HE_Face> m_Faces;
    };
}