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

#include "utils.h"
#include "primitives.h"
#include "bvtree.h"
#include "geometry.h"
#include "raybv.h"

int CRayBV::GetNodeContents(int iNode, BV* pBVCollider, int bColliderUsed, int bColliderLocal,
    geometry_under_test* pGTest, geometry_under_test* pGTestOp)
{
    return m_pGeom->GetPrimitiveList(0, 1, pBVCollider->type, *pBVCollider, bColliderLocal, pGTest, pGTestOp, pGTest->primbuf, pGTest->idbuf);
    //return 1; // ray should already be in the buffer
}

void CRayBV::GetNodeBV(BV*& pBV, int iNode, int iCaller)
{
    pBV = &g_BVray;
    g_BVray.iNode = 0;
    g_BVray.type = ray::type;
    g_BVray.aray.origin = m_pray->origin;
    g_BVray.aray.dir = m_pray->dir;
}

void CRayBV::GetNodeBV(const Matrix33& Rw, const Vec3& offsw, float scalew, BV*& pBV, int iNode, int iCaller)
{
    pBV = &g_BVray;
    g_BVray.iNode = 0;
    g_BVray.type = ray::type;
    g_BVray.aray.origin = Rw * m_pray->origin * scalew + offsw;
    g_BVray.aray.dir = Rw * m_pray->dir * scalew;
}