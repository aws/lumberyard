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

#ifndef CRYINCLUDE_CRYAISYSTEM_CTRIANGULATOR_H
#define CRYINCLUDE_CRYAISYSTEM_CTRIANGULATOR_H
#pragma once

#include <vector>
#include <list>

struct Vtx
{
    real x, y, z;
    std::vector<int> m_lstTris; // triangles that contain this point
    bool bCollidable;
    bool bHideable;

    Vtx(real _x = 0.0f, real _y = 0.0f, real _z = 0.0f)
        : x(_x)
        , y(_y)
        , z(_z) {bCollidable = true; bHideable = false; }
    Vtx(const Vec3r& v)
        : x(v.x)
        , y(v.y)
        , z(v.z) {bCollidable = true; bHideable = false; }

    bool operator==(const Vtx& other)
    {
        if ((fabs(x - other.x) < 0.001) && (fabs(y - other.y) < 0.001))
        {
            return true;
        }
        return false;
    }
};


struct Tri
{
    int v[3];
    Vtx center;
    real radiusSq;

    unsigned graphNodeIndex;

    Tri(int v0 = 0, int v1 = 0, int v2 = 0)
    {
        v[0] = v0;
        v[1] = v1;
        v[2] = v2;
        radiusSq = 0.0f;
        center.x = center.y = center.z = 0.0f;
        graphNodeIndex = 0;
    }
};

struct SPOINT
{
    int x, y;
};

struct MYPOINT
    : public SPOINT
{
    bool operator==(const MYPOINT& other) const
    {
        return ((this->x == other.x) && (this->y == other.y));
    }
};

typedef std::list<Tri*> TARRAY;
typedef std::vector<Vtx> VARRAY;

class CTriangulator
{
    // line segment
    struct SSegment
    {
        SSegment(const Vec3r& p1, const Vec3r& p2)
            : m_p1(p1)
            , m_p2(p2){}
        Vec3r   m_p1;
        Vec3r   m_p2;
    };
    typedef std::vector<SSegment>   TCutsVector;

    VARRAY      m_vProcessed;
    TARRAY      m_vTriangles;

    typedef std::vector<MYPOINT> tUniquePts;
    tUniquePts m_uniquePts;

    TCutsVector m_cuts;
    size_t      m_curCutIdx;


public:
    void CalcCircle(const Vtx& v1, const Vtx& v2, const Vtx& v3,  Tri* pTri);

    CTriangulator();
    ~CTriangulator();

    VARRAY      m_vVertices;
    Vtx m_vtxBBoxMin;
    Vtx m_vtxBBoxMax;

    int AddVertex(real x, real y, real z, bool bCollidable, bool bHideable);
    void AddSegment(const Vec3r& p1, const Vec3r& p2);
    bool GetSegment(Vec3r& p1, Vec3r& p2);
    bool Triangulate();
    TARRAY GetTriangles();
    VARRAY GetVertices();
    bool IsAntiClockwise(Tri* who);
    void PushUnique(int a, int b);

    bool DoesVertexExist2D(real x, real y, real tol) const;
    bool PrepForTriangulation(void);
    bool TriangulateNew(void);
private:
    bool IsPerpendicular(const Vtx& v1, const Vtx& v2, const Vtx& v3);
    bool Calculate(Tri* pTri);
};

#endif // CRYINCLUDE_CRYAISYSTEM_CTRIANGULATOR_H
