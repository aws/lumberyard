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

#include "terrain.h"
#include <Terrain/Bus/TerrainProviderBus.h>

float CTerrain::GetBilinearZ(MeterF xWS, MeterF yWS) const
{
    if (!m_RootNode)
    {
        return TERRAIN_BOTTOM_LEVEL;
    }

    float fZ;

    // convert into hmap space
    float x1 = xWS * CTerrain::GetInvUnitSize();
    float y1 = yWS * CTerrain::GetInvUnitSize();

    if (!Cry3DEngineBase::GetTerrain() || x1 < 0 || y1 < 0)
    {
        return TERRAIN_BOTTOM_LEVEL;
    }

    int nX = fastftol_positive(x1);
    int nY = fastftol_positive(y1);

    int nHMSize = CTerrain::GetTerrainSize() / CTerrain::GetHeightMapUnitSize();

    if (!Cry3DEngineBase::GetTerrain() || nX < 0 || nY < 0 || nX >= nHMSize || nY >= nHMSize)
    {
        fZ = TERRAIN_BOTTOM_LEVEL;
    }
    else
    {
        float dx1 = x1 - nX;
        float dy1 = y1 - nY;

        float afZCorners[4];

        const CTerrainNode* pNode = GetLeafNodeAt_Units(nX, nY);

        if (pNode && pNode->GetSurfaceTile().GetHeightmap())
        {
            pNode->GetSurfaceTile().GetHeightQuad(nX, nY, afZCorners);

            if (dx1 + dy1 < 1.f)
            {
                // Lower triangle.
                fZ = afZCorners[0] * (1.f - dx1 - dy1)
                    + afZCorners[1] * dx1
                    + afZCorners[2] * dy1;
            }
            else
            {
                // Upper triangle.
                fZ = afZCorners[3] * (dx1 + dy1 - 1.f)
                    + afZCorners[2] * (1.f - dx1)
                    + afZCorners[1] * (1.f - dy1);
            }
            if (fZ < TERRAIN_BOTTOM_LEVEL)
            {
                fZ = TERRAIN_BOTTOM_LEVEL;
            }
        }
        else
        {
            fZ = TERRAIN_BOTTOM_LEVEL;
        }
    }
    return fZ;
}

bool CTerrain::RayTrace(Vec3 const& vStart, Vec3 const& vEnd, SRayTrace* prt)
{
    FUNCTION_PROFILER_3DENGINE;

    CTerrain* pTerrain = GetTerrain();

    if (!pTerrain->GetRootNode())
    {
        return false;
    }

    // Temp storage to avoid tests.
    SRayTrace s_rt;
    SRayTrace& rt = prt ? *prt : s_rt;

    float fUnitSize = (float)CTerrain::GetHeightMapUnitSize();
    float fInvUnitSize = CTerrain::GetInvUnitSize();
    int nGridSize = (int)(CTerrain::GetTerrainSize() * fInvUnitSize);

    // Convert to grid units.
    Vec3 vDelta = vEnd - vStart;
    float fMinZ = min(vStart.z, vEnd.z);

    int nX = clamp_tpl(int(vStart.x * fInvUnitSize), 0, nGridSize - 1);
    int nY = clamp_tpl(int(vStart.y * fInvUnitSize), 0, nGridSize - 1);

    int nEndX = clamp_tpl(int(vEnd.x * fInvUnitSize), 0, nGridSize - 1);
    int nEndY = clamp_tpl(int(vEnd.y * fInvUnitSize), 0, nGridSize - 1);

    for (;; )
    {
        // Get heightmap node for current point.
        CTerrainNode const* pNode = pTerrain->GetLeafNodeAt_Units(nX, nY);

        int nStepUnits = 1;

        // When entering new node, check with bounding box.
        if (pNode && pNode->GetSurfaceTile().GetHeightmap() && fMinZ <= pNode->m_LocalAABB.max.z)
        {
            const SurfaceTile& tile = pNode->GetSurfaceTile();

            // Evaluate individual sectors.
            assert((1 << m_UnitToSectorBitShift) == tile.GetSize() - 1);

            // Get cell for starting point.
            int nType = tile.GetWeight(nX, nY).PrimaryId();
            if (nType != SurfaceWeight::Hole)
            {
                // Get cell vertex values.
                float afZ[4];
                tile.GetHeightQuad(nX, nY, afZ);

                // Further zmin check.
                if (fMinZ <= afZ[0] || fMinZ <= afZ[1] || fMinZ <= afZ[2] || fMinZ <= afZ[3])
                {
                    if (prt)
                    {
                        _smart_ptr<IMaterial> pMat = pTerrain->GetSurfaceTypes()[nType].pLayerMat;
                        prt->material = pMat;
                        if (pMat && pMat->GetSubMtlCount() > 2)
                        {
                            prt->material = pMat->GetSubMtl(2);
                        }
                    }

                    // Select common point on both tris.
                    float fX0 = nX * fUnitSize;
                    float fY0 = nY * fUnitSize;
                    Vec3 vEndRel = vEnd - Vec3(fX0 + fUnitSize, fY0, afZ[1]);

                    //
                    // Intersect with bottom triangle.
                    //
                    Vec3 vTriDir1(afZ[0] - afZ[1], afZ[0] - afZ[2], fUnitSize);
                    float fET1 = vEndRel * vTriDir1;
                    if (fET1 < 0.f)
                    {
                        // End point below plane. Find intersection time.
                        float fDT = vDelta * vTriDir1;
                        if (fDT <= fET1)
                        {
                            rt.t = 1.f - fET1 / fDT;
                            rt.hitPoint = vStart + vDelta * rt.t;

                            // Check against tri boundaries.
                            if (rt.hitPoint.x >= fX0 && rt.hitPoint.y >= fY0 && rt.hitPoint.x + rt.hitPoint.y <= fX0 + fY0 + fUnitSize)
                            {
                                rt.hitNormal = vTriDir1.GetNormalized();
                                return true;
                            }
                        }
                    }

                    //
                    // Intersect with top triangle.
                    //
                    Vec3 vTriDir2(afZ[2] - afZ[3], afZ[1] - afZ[3], fUnitSize);
                    float fET2 = vEndRel * vTriDir2;
                    if (fET2 < 0.f)
                    {
                        // End point below plane. Find intersection time.
                        float fDT = vDelta * vTriDir2;
                        if (fDT <= fET2)
                        {
                            rt.t = 1.f - fET2 / fDT;
                            rt.hitPoint = vStart + vDelta * rt.t;

                            // Check against tri boundaries.
                            if (rt.hitPoint.x <= fX0 + fUnitSize && rt.hitPoint.y <= fY0 + fUnitSize && rt.hitPoint.x + rt.hitPoint.y >= fX0 + fY0 + fUnitSize)
                            {
                                rt.hitNormal = vTriDir2.GetNormalized();
                                return true;
                            }
                        }
                    }

                    // Check for end point below terrain, to correct for leaks.
                    if (nX == nEndX && nY == nEndY)
                    {
                        if (fET1 < 0.f)
                        {
                            // Lower tri.
                            if (vEnd.x + vEnd.y <= fX0 + fY0 + fUnitSize)
                            {
                                rt.t = 1.f;
                                rt.hitNormal = vTriDir1.GetNormalized();
                                rt.hitPoint = vEnd;
                                rt.hitPoint.z = afZ[0] - ((vEnd.x - fX0) * rt.hitNormal.x + (vEnd.y - fY0) * rt.hitNormal.y) / rt.hitNormal.z;
                                return true;
                            }
                        }
                        if (fET2 < 0.f)
                        {
                            // Upper tri.
                            if (vEnd.x + vEnd.y >= fX0 + fY0 + fUnitSize)
                            {
                                rt.t = 1.f;
                                rt.hitNormal = vTriDir2.GetNormalized();
                                rt.hitPoint = vEnd;
                                rt.hitPoint.z = afZ[3] - ((vEnd.x - fX0 - fUnitSize) * rt.hitNormal.x + (vEnd.y - fY0 - fUnitSize) * rt.hitNormal.y) / rt.hitNormal.z;
                                return true;
                            }
                        }
                    }
                }
            }
        }
        else
        {
            // Skip entire node.
            nStepUnits = 1 << (m_UnitToSectorBitShift + 0 /*pNode->m_nTreeLevel*/);
            assert(!pNode || nStepUnits == int(pNode->m_LocalAABB.max.x - pNode->m_LocalAABB.min.x) * fInvUnitSize);
            assert(!pNode || pNode->m_nTreeLevel == 0);
        }

        // Step out of cell.
        int nX1 = nX & ~(nStepUnits - 1);
        if (vDelta.x >= 0.f)
        {
            nX1 += nStepUnits;
        }
        float fX = (float)(nX1 * CTerrain::GetHeightMapUnitSize());

        int nY1 = nY & ~(nStepUnits - 1);
        if (vDelta.y >= 0.f)
        {
            nY1 += nStepUnits;
        }
        float fY = (float)(nY1 * CTerrain::GetHeightMapUnitSize());

        if (abs((fX - vStart.x) * vDelta.y) < abs((fY - vStart.y) * vDelta.x))
        {
            if (fX * vDelta.x >= vEnd.x * vDelta.x)
            {
                break;
            }
            if (nX1 > nX)
            {
                nX = nX1;
                if (nX >= nGridSize)
                {
                    break;
                }
            }
            else
            {
                nX = nX1 - 1;
                if (nX < 0)
                {
                    break;
                }
            }
        }
        else
        {
            if (fY * vDelta.y >= vEnd.y * vDelta.y)
            {
                break;
            }
            if (nY1 > nY)
            {
                nY = nY1;
                if (nY >= nGridSize)
                {
                    break;
                }
            }
            else
            {
                nY = nY1 - 1;
                if (nY < 0)
                {
                    break;
                }
            }
        }
    }

    return false;
}

bool CTerrain::IsHole(Meter x, Meter y) const
{
    int nX_units = x >> m_MeterToUnitBitShift;
    int nY_units = y >> m_MeterToUnitBitShift;
    int nTerrainSize_units = (CTerrain::GetTerrainSize() >> m_MeterToUnitBitShift) - 2;

    if (nX_units < 0 || nX_units > nTerrainSize_units || nY_units < 0 || nY_units > nTerrainSize_units)
    {
        return false;
    }

    return GetSurfaceWeight_Units(nX_units, nY_units).PrimaryId() == SurfaceWeight::Hole;
}

ITerrain::SurfaceWeight CTerrain::GetSurfaceWeight(Meter x, Meter y) const
{
    if (x >= 0 && y >= 0 && x <= CTerrain::GetTerrainSize() && y <= CTerrain::GetTerrainSize())
    {
        return GetSurfaceWeight_Units(x >> m_MeterToUnitBitShift, y >> m_MeterToUnitBitShift);
    }

    return SurfaceWeight();
}

float CTerrain::GetZ(Meter x, Meter y) const
{
    if (!m_RootNode)
    {
        return TERRAIN_BOTTOM_LEVEL;
    }

    return GetZ_Unit(x >> m_MeterToUnitBitShift, y >> m_MeterToUnitBitShift);
}

ITerrain::SurfaceWeight CTerrain::GetSurfaceWeight_Units(Unit x, Unit y) const
{
    Clamp_Unit(x, y);
    const CTerrainNode* pNode = GetLeafNodeAt_Units(x, y);
    if (pNode)
    {
        const SurfaceTile& tile = pNode->GetSurfaceTile();
        if (tile.GetWeightmap())
        {
            return tile.GetWeight(x, y);
        }
    }
    return SurfaceWeight();
}

float CTerrain::GetZ_Unit(Unit x, Unit y) const
{
    Clamp_Unit(x, y);
    const CTerrainNode* pNode = GetLeafNodeAt_Units(x, y);
    if (pNode)
    {
        const SurfaceTile& tile = pNode->GetSurfaceTile();
        if (tile.GetHeightmap())
        {
            return tile.GetHeight(x, y);
        }
    }
    return 0.0f;
}

bool CTerrain::IsMeshQuadFlipped(const Meter x, const Meter y, const Meter nUnitSize) const
{
    bool bFlipped = false;

    // Flip winding order to prevent surface type interpolation over long edges
    int nType10 = GetSurfaceWeight(x + nUnitSize, y).PrimaryId();
    int nType01 = GetSurfaceWeight(x, y + nUnitSize).PrimaryId();

    if (nType10 != nType01)
    {
        int nType00 = GetSurfaceWeight(x, y).PrimaryId();
        int nType11 = GetSurfaceWeight(x + nUnitSize, y + nUnitSize).PrimaryId();

        if ((nType10 == nType00 && nType10 == nType11)
            || (nType01 == nType00 && nType01 == nType11))
        {
            bFlipped = true;
        }
    }

    return bFlipped;
}

CTerrain::SCachedHeight CTerrain::m_arrCacheHeight[nHMCacheSize * nHMCacheSize];
CTerrain::SCachedSurfType CTerrain::m_arrCacheSurfType[nHMCacheSize * nHMCacheSize];

namespace
{
    static inline uint32 encodeby1(uint32 n)
    {
        n &= 0x0000ffff;
        n = (n | (n << 8)) & 0x00FF00FF;
        n = (n | (n << 4)) & 0x0F0F0F0F;
        n = (n | (n << 2)) & 0x33333333;
        n = (n | (n << 1)) & 0x55555555;
        return n;
    }
}

float CTerrain::GetHeightFromTerrain_Callback(int ix, int iy)
{
#ifdef LY_TERRAIN_RUNTIME
    if (Terrain::TerrainProviderRequestBus::HasHandlers())
    {
        float height = 0.0f;
        Terrain::TerrainProviderRequestBus::BroadcastResult(height, &Terrain::TerrainProviderRequestBus::Events::GetHeightAtIndexedPosition, ix, iy);
        return height;
    }
    else
#endif
    {
        const uint32 idx = encodeby1(ix & ((nHMCacheSize - 1))) | (encodeby1(iy & ((nHMCacheSize - 1))) << 1);
        CTerrain::SCachedHeight& rCache = m_arrCacheHeight[idx];
        if (rCache.x == ix && rCache.y == iy)
        {
            return rCache.fHeight;
        }

        rCache.x = ix;
        rCache.y = iy;

        CTerrain* terrain = CTerrain::GetTerrain();

        if (!terrain)
        {
            return 0.0f;
        }

        rCache.fHeight = terrain->GetZ_Unit(ix, iy);
        return rCache.fHeight;
    }
}

unsigned char CTerrain::GetSurfaceTypeFromTerrain_Callback(int ix, int iy)
{
#ifdef LY_TERRAIN_RUNTIME
    if (Terrain::TerrainProviderRequestBus::HasHandlers())
    {
        unsigned char surfaceType = 0;
        Terrain::TerrainProviderRequestBus::BroadcastResult(surfaceType, &Terrain::TerrainProviderRequestBus::Events::GetSurfaceTypeAtIndexedPosition, ix, iy);
        return surfaceType;
    }
    else
#endif
    {
        const uint32 idx = encodeby1(ix & ((nHMCacheSize - 1))) | (encodeby1(iy & ((nHMCacheSize - 1))) << 1);
        CTerrain::SCachedSurfType& rCache = m_arrCacheSurfType[idx];
        if (rCache.x == ix && rCache.y == iy)
        {
            return rCache.surfType;
        }

        CTerrain* terrain = CTerrain::GetTerrain();
        if (!terrain)
        {
            return 0;
        }

        rCache.x = ix;
        rCache.y = iy;
        rCache.surfType = terrain->GetSurfaceWeight_Units(ix, iy).PrimaryId();

        return rCache.surfType;
    }
}

float CTerrain::GetSlope(Meter x, Meter y) const
{
    if (!m_RootNode)
    {
        return 0;
    }

    return GetSlope_Unit(x >> m_MeterToUnitBitShift, y >> m_MeterToUnitBitShift);
}

float CTerrain::GetSlope_Unit(Unit nX_units, Unit nY_units) const
{
    const float h = GetHeightFromTerrain_Callback(nX_units, nY_units);

    const float xm1_ym1 = GetHeightFromTerrain_Callback(nX_units - 1, nY_units - 1);
    const float x_ym1 =   GetHeightFromTerrain_Callback(nX_units,     nY_units - 1);
    const float xp1_ym1 = GetHeightFromTerrain_Callback(nX_units + 1, nY_units - 1);

    const float xm1_y =   GetHeightFromTerrain_Callback(nX_units - 1, nY_units);
    const float xp1_y =   GetHeightFromTerrain_Callback(nX_units + 1, nY_units);

    const float xm1_yp1 = GetHeightFromTerrain_Callback(nX_units - 1, nY_units + 1);
    const float x_yp1 =   GetHeightFromTerrain_Callback(nX_units,     nY_units + 1);
    const float xp1_yp1 = GetHeightFromTerrain_Callback(nX_units + 1, nY_units + 1);

    float fs =
       (fabs_tpl(xm1_ym1 - h) +
        fabs_tpl(x_ym1 - h) +
        fabs_tpl(xp1_ym1 - h) +
        fabs_tpl(xm1_y - h) +
        fabs_tpl(xp1_y - h) +
        fabs_tpl(xm1_yp1 - h) +
        fabs_tpl(x_yp1 - h) +
        fabs_tpl(xp1_yp1 - h));
    fs = fs * 8.0f;
    if (fs > 255.0f)
    {
        fs = 255.0f;
    }
    return fs;
}