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
//  Copyright (C), Crytek Studios, 2011
// -------------------------------------------------------------------------
//  File name:   ModelDB.h
//  Created:     March/5/2012 by Jaesik
////////////////////////////////////////////////////////////////////////////
#include "Polygon.h"

namespace CD
{
    class Model;
    class CPlaneDB;

    class ModelDB
        : public CRefCountBase
    {
    public:

        static const BrushFloat& kDBEpsilon;

        struct Mark
        {
            Mark()
                : m_pPolygon(NULL)
                , m_VertexIndex(-1)
            {
            }

            CD::PolygonPtr m_pPolygon;
            int m_VertexIndex;
        };
        typedef std::vector<Mark> MarkList;

        struct Vertex
        {
            BrushVec3 m_Pos;
            MarkList m_MarkList;

            void Merge(const Vertex& v)
            {
                m_MarkList.insert(m_MarkList.end(), v.m_MarkList.begin(), v.m_MarkList.end());
            }
        };

        typedef std::vector<Vertex> QueryResult;

    public:
        ModelDB();
        ModelDB(const ModelDB& db);
        ~ModelDB();
        ModelDB& operator =(const ModelDB& db);

        void Reset(CD::Model* pModel, int nFlag, int nValidShelfID = -1);
        bool QueryAsVertex(const BrushVec3& pos, QueryResult& qResult) const;
        bool QueryAsRectangle(CViewport* pView, const BrushMatrix34& worldTM, const QRect& rect, QueryResult& qResult) const;
        bool QueryAsRay(BrushVec3& vRaySrc, BrushVec3& vRayDir, QueryResult& qResult) const;
        void AddMarkToVertex(const BrushVec3& vPos, const Mark& mark);
        bool UpdatePolygonVertices(CD::PolygonPtr pPolygon);
        BrushVec3 Snap(const BrushVec3& pos);

        BrushPlane AddPlane(const BrushPlane& plane);
        bool FindPlane(const BrushPlane& inPlane, BrushPlane& outPlane) const;

        void GetVertexList(std::vector<BrushVec3>& outVertexList) const;

    private:
        void AddVertex(const BrushVec3& vertex, int nVertexIndex, CD::PolygonPtr pPolygon);
        void UpdateAllPolygonVertices();

    private:

        std::vector<Vertex> m_VertexDB;
        std::unique_ptr<CPlaneDB> m_pPlaneDB;
    };
};