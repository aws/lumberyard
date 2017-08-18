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

#ifndef CRYINCLUDE_CRYPHYSICS_RAYBV_H
#define CRYINCLUDE_CRYPHYSICS_RAYBV_H
#pragma once

class CRayBV
    : public CBVTree
{
public:
    CRayBV() {}
    virtual int GetType() { return BVT_RAY; }
    virtual float Build(CGeometry* pGeom) { m_pGeom = pGeom; return 0.0f; }
    void SetRay(ray* pray) { m_pray = pray; }
    virtual void GetNodeBV(BV*& pBV, int iNode = 0, int iCaller = 0);
    virtual void GetNodeBV(BV*& pBV, const Vec3& sweepdir, float sweepstep, int iNode = 0, int iCaller = 0) {}
    virtual void GetNodeBV(const Matrix33& Rw, const Vec3& offsw, float scalew, BV*& pBV, int iNode = 0, int iCaller = 0);
    virtual void GetNodeBV(const Matrix33& Rw, const Vec3& offsw, float scalew, BV*& pBV, const Vec3& sweepdir, float sweepstep, int iNode = 0, int iCaller = 0) {}
    virtual int GetNodeContents(int iNode, BV* pBVCollider, int bColliderUsed, int bColliderLocal,
        geometry_under_test* pGTest, geometry_under_test* pGTestOp);

    CGeometry* m_pGeom;
    ray* m_pray;
};

#endif // CRYINCLUDE_CRYPHYSICS_RAYBV_H

