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

#ifndef CRYINCLUDE_CRYPHYSICS_VOXELBV_H
#define CRYINCLUDE_CRYPHYSICS_VOXELBV_H
#pragma once

class CVoxelBV
    : public CBVTree
{
public:
    CVoxelBV() { m_nTris = 0; }
    virtual ~CVoxelBV() {}
    virtual int GetType() { return BVT_VOXEL; }
    float Build(CGeometry* pGeom) { m_pMesh = (CTriMesh*)pGeom; return 0; }

    virtual void GetBBox(box* pbox);
    virtual int MaxPrimsInNode() { return m_nTris; }

    virtual int PrepareForIntersectionTest(geometry_under_test* pGTest, CGeometry* pCollider, geometry_under_test* pGTestColl);
    virtual void CleanupAfterIntersectionTest(geometry_under_test* pGTest);
    virtual void GetNodeBV(BV*& pBV, int iNode = 0, int iCaller = 0);
    virtual void GetNodeBV(BV*& pBV, const Vec3& sweepdir, float sweepstep, int iNode = 0, int iCaller = 0) { GetNodeBV(pBV); }
    virtual void GetNodeBV(const Matrix33& Rw, const Vec3& offsw, float scalew, BV*& pBV, int iNode = 0, int iCaller = 0);
    virtual void GetNodeBV(const Matrix33& Rw, const Vec3& offsw, float scalew, BV*& pBV, const Vec3& sweepdir, float sweepstep, int iNode = 0, int iCaller = 0)
    { GetNodeBV(Rw, offsw, scalew, pBV, 0, iCaller); }
    virtual int GetNodeContents(int iNode, BV* pBVCollider, int bColliderUsed, int bColliderLocal,
        geometry_under_test* pGTest, geometry_under_test* pGTestOp);
    virtual void MarkUsedTriangle(int itri, geometry_under_test* pGTest);
    virtual void ResetCollisionArea() { m_iBBox[0].zero(); m_iBBox[1] = m_pgrid->size; }

    CTriMesh* m_pMesh;
    voxelgrid* m_pgrid;
    Vec3i m_iBBox[2];
    int m_nTris;
};

#endif // CRYINCLUDE_CRYPHYSICS_VOXELBV_H
