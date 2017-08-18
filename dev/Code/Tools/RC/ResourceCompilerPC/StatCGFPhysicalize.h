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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_STATCGFPHYSICALIZE_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_STATCGFPHYSICALIZE_H
#pragma once

#include "PhysWorld.h"
#include "CGFContent.h"

//////////////////////////////////////////////////////////////////////////
class CPhysicsInterface
{
public:
    CPhysicsInterface();
    ~CPhysicsInterface();

    enum EPhysicalizeResult
    {
        ePR_Empty,
        ePR_Ok,
        ePR_Fail
    };

    EPhysicalizeResult Physicalize(CNodeCGF* pNodeCGF, CContentCGF* pCGF);
    bool DeletePhysicalProxySubsets(CNodeCGF* pNodeCGF, bool bCga);
    void ProcessBreakablePhysics(CContentCGF* pCompiledCGF, CContentCGF* pSrcCGF);
    int CheckNodeBreakable(CNodeCGF* pNode, IGeometry* pGeom = 0);
    IGeomManager* GetGeomManager() { return m_pGeomManager; }

    // When node already physicalized, physicalize it again.
    void RephysicalizeNode(CNodeCGF* pNodeCGF, CContentCGF* pCGF);

private:
    EPhysicalizeResult PhysicalizeGeomType(int nGeomType, CNodeCGF* pNodeCGF, CContentCGF* pCGF);

    IGeomManager* m_pGeomManager;
    CPhysWorldLoader m_physLoader;
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_STATCGFPHYSICALIZE_H
