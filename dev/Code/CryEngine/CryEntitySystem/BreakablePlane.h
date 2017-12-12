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

#ifndef CRYINCLUDE_CRYENTITYSYSTEM_BREAKABLEPLANE_H
#define CRYINCLUDE_CRYENTITYSYSTEM_BREAKABLEPLANE_H

#pragma once

#include "IIndexedMesh.h"
#include "IPhysics.h"
#include "IDeferredCollisionEvent.h"

struct SProcessImpactIn;
struct SProcessImpactOut;
struct SExtractMeshIslandIn;
struct SExtractMeshIslandOut;

class CBreakablePlane
    : public IOwnedObject
{
public:
    CBreakablePlane()
    {
        m_pGrid = 0;
        m_pMat = 0;
        m_nCells.set(20, 20);
        m_cellSize = 1;
        m_maxPatchTris = 20;
        m_jointhresh = 0.3f;
        m_density = 900;
        m_bStatic = 1;
        m_pGeom = 0;
        m_pSampleRay = 0;
        *m_mtlSubstName = 0;
    }
    ~CBreakablePlane()
    {
        if (m_pGrid)
        {
            m_pGrid->Release();
        }
        if (m_pSampleRay)
        {
            m_pSampleRay->Release();
        }
        if (m_pGeom)
        {
            m_pGeom->Release();
        }
    }
    int Release() { delete this; return 0;  }

    bool SetGeometry(IStatObj* pStatObj, _smart_ptr<IMaterial> pRenderMat, int bStatic, int seed);
    void FillVertexData(CMesh* pMesh, int ivtx, const vector2df& pos, int iside);
    IStatObj* CreateFlatStatObj(int*& pIdx, vector2df* pt, vector2df* bounds, const Matrix34& mtxWorld, IParticleEffect* pEffect = 0,
        bool bNoPhys = false, bool bUseEdgeAlpha = false);
    int* Break(const Vec3& pthit, float r, vector2df*& ptout, int seed, float filterAng, float ry);
    static int ProcessImpact(const SProcessImpactIn& in, SProcessImpactOut& out);
    static void ExtractMeshIsland(const SExtractMeshIslandIn& in, SExtractMeshIslandOut& out);

    float m_cellSize;
    vector2di m_nCells;
    float m_nudge;
    int m_maxPatchTris;
    float m_jointhresh;
    float m_density;
    IBreakableGrid2d* m_pGrid;
    Matrix33 m_R;
    Vec3 m_center;
    int m_bAxisAligned;
    int m_matId;
    _smart_ptr<IMaterial> m_pMat;
    int m_matSubindex;
    char m_mtlSubstName[64];
    int m_matFlags;
    float m_z[2];
    float m_thicknessOrg;
    float m_refArea[2];
    vector2df m_ptRef[2][3];
    SMeshTexCoord m_texRef[2][3];
    SMeshTangents   m_TangentRef[2][3];
    int m_bStatic;
    int m_bOneSided;
    IGeometry* m_pGeom, * m_pSampleRay;
    static int g_nPieces;
    static float g_maxPieceLifetime;
};

#endif // CRYINCLUDE_CRYENTITYSYSTEM_BREAKABLEPLANE_H
