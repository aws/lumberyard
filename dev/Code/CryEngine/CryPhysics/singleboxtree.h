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

#ifndef CRYINCLUDE_CRYPHYSICS_SINGLEBOXTREE_H
#define CRYINCLUDE_CRYPHYSICS_SINGLEBOXTREE_H
#pragma once


class CSingleBoxTree
    : public CBVTree
{
public:
    ILINE CSingleBoxTree() { m_nPrims = 1; m_bIsConvex = 0; }
    virtual int GetType() { return BVT_SINGLEBOX; }
    virtual float Build(CGeometry* pGeom);
    virtual void SetGeomConvex() { m_bIsConvex = 1; }
    void SetBox(box* pbox);
    virtual int PrepareForIntersectionTest(geometry_under_test* pGTest, CGeometry* pCollider, geometry_under_test* pGTestColl);
    virtual void GetBBox(box* pbox);
    virtual int MaxPrimsInNode() { return m_nPrims; }
    virtual void GetNodeBV(BV*& pBV, int iNode = 0, int iCaller = 0);
    virtual void GetNodeBV(BV*& pBV, const Vec3& sweepdir, float sweepstep, int iNode = 0, int iCaller = 0);
    virtual void GetNodeBV(const Matrix33& Rw, const Vec3& offsw, float scalew, BV*& pBV, int iNode = 0, int iCaller = 0);
    virtual void GetNodeBV(const Matrix33& Rw, const Vec3& offsw, float scalew, BV*& pBV, const Vec3& sweepdir, float sweepstep, int iNode = 0, int iCaller = 0);
    virtual int GetNodeContents(int iNode, BV* pBVCollider, int bColliderUsed, int bColliderLocal,
        geometry_under_test* pGTest, geometry_under_test* pGTestOp);
    virtual int GetNodeContentsIdx(int iNode, int& iStartPrim) { iStartPrim = 0; return m_nPrims; }
    virtual void MarkUsedTriangle(int itri, geometry_under_test* pGTest);

    virtual void GetMemoryStatistics(ICrySizer* pSizer);
    virtual void Save(CMemStream& stm);
    virtual void Load(CMemStream& stm, CGeometry* pGeom);

    CGeometry* m_pGeom;
    box m_Box;
    int m_nPrims;
    int m_bIsConvex;
};

#endif // CRYINCLUDE_CRYPHYSICS_SINGLEBOXTREE_H
