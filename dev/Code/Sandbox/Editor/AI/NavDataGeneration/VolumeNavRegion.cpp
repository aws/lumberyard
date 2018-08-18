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

#include "VolumeNavRegion.h"
#include "CTriangulator.h"
#include "AILog.h"
#include "AICollision.h"

#include "IPhysics.h"
#include "ISystem.h"
#include "IRenderer.h"
#include "IRenderAuxGeom.h"
#include "CryFile.h"
#include "ITimer.h"
#include "I3DEngine.h"
#include "ISerialize.h"
#include "IEntitySystem.h"
#include "IConsole.h"
#include "Navigation.h"

#include <algorithm>
#include <limits>

#define BAI_3D_FILE_VERSION 21

static float volumeVoxelSize = 1.0f;

#ifdef DYNAMIC_3D_NAVIGATION
#define PASSABILITY_FLAGS AICE_ALL
#else
#define PASSABILITY_FLAGS AICE_STATIC
#endif

struct SGravityModifier
{
    IPhysicalEntity* pEntity;
    AABB aabb;
    Vec3 aabbCentre;
    float gravityStrength;
};

typedef std::vector<SGravityModifier> GravityModifiers;

//====================================================================
//  some generic helper functions
//====================================================================

//====================================================================
// GroupVoxels
//groups all the neighbouring voxels with value 1 to new value
//====================================================================
void GroupVoxels(std::vector<int>& voxels, int groupId, int xIdx, int yIdx, int zIdx, size_t dimX, size_t dimY, size_t dimZ)
{
    typedef   std::vector<size_t> TIdxList;
    static TIdxList theQueue;
    theQueue.resize(0);
    size_t curIdx = zIdx * dimY * dimX + yIdx * dimX + xIdx;
    if (voxels[curIdx] != 1)
    {
        return;
    }

    theQueue.push_back(curIdx);

    while (!theQueue.empty())
    {
        curIdx = theQueue.back();
        theQueue.pop_back();

        if (voxels[curIdx] != 1)
        {
            continue;
        }
        voxels[curIdx] = groupId;

        xIdx = curIdx % (dimY * dimX);
        xIdx = xIdx % (dimX);

        yIdx = curIdx % (dimY * dimX);
        yIdx = yIdx / (dimX);

        zIdx = curIdx / (dimY * dimX);

        if (xIdx > 0)
        {
            theQueue.push_back(zIdx * dimY * dimX + yIdx * dimX + xIdx - 1);
        }
        if (xIdx < static_cast<int>(dimX) - 1)
        {
            theQueue.push_back(zIdx * dimY * dimX + yIdx * dimX + xIdx + 1);
        }
        if (yIdx > 0)
        {
            theQueue.push_back(zIdx * dimY * dimX + (yIdx - 1) * dimX + xIdx);
        }
        if (yIdx < static_cast<int>(dimY) - 1)
        {
            theQueue.push_back(zIdx * dimY * dimX + (yIdx + 1) * dimX + xIdx);
        }
        if (zIdx > 0)
        {
            theQueue.push_back((zIdx - 1) * dimY * dimX + yIdx * dimX + xIdx);
        }
        if (zIdx < static_cast<int>(dimZ) - 1)
        {
            theQueue.push_back((zIdx + 1) * dimY * dimX + yIdx * dimX + xIdx);
        }
    }
}

//====================================================================
// IsPointInsideGravityModifier
//====================================================================
static bool IsPointInsideGravityModifier(const Vec3& pt, const SGravityModifier& gm)
{
    Vec3 testPt = pt + 100.0f * (pt - gm.aabbCentre).GetNormalizedSafe();

    ray_hit rayHit;
    int result = gEnv->pPhysicalWorld->RayTraceEntity(gm.pEntity, testPt, pt - testPt, &rayHit);
    return result != 0;
}

//====================================================================
// DoesLinkIntersectGravityModifier
//====================================================================
static bool DoesLinkIntersectGravityModifier(const Vec3& p1, const Vec3& p2,
    const GravityModifiers& gravityModifiers)
{
    unsigned num = gravityModifiers.size();
    int result;
    ray_hit rayHit;
    Vec3 linkDelta = p2 - p1;

    Vec3 linkDir = linkDelta.GetNormalizedSafe();
    AABB linkAABB(AABB::RESET);
    linkAABB.Add(p1);
    linkAABB.Add(p2);

    Vec3 pt = 0.5f * (p1 + p2);
    Vec3 gravity;
    pe_params_buoyancy junk;
    gEnv->pPhysicalWorld->CheckAreas(pt, gravity, &junk);

    for (unsigned i = 0; i < num; ++i)
    {
        const SGravityModifier& gm = gravityModifiers[i];
        if (!Overlap::AABB_AABB(linkAABB, gm.aabb))
        {
            continue;
        }

        // check gravity dir against link
        static float criticalGravityValue = 0.0f;
        static float criticalGravityMag = 1.0f;

        float gravStrength = gravity.NormalizeSafe();
        if (gravStrength < criticalGravityMag)
        {
            continue;
        }

        if (linkDir.Dot(gravity) > criticalGravityValue)
        {
            continue;
        }

        result = gEnv->pPhysicalWorld->RayTraceEntity(gm.pEntity, p1, linkDelta, &rayHit);
        if (result)
        {
            return true;
        }

        static bool checkPoints = true;
        if (checkPoints)
        {
            if (IsPointInsideGravityModifier(p1, gm))
            {
                return true;
            }
            if (IsPointInsideGravityModifier(p2, gm))
            {
                return true;
            }
        }
    }
    return false;
}

//====================================================================
// DoesLinkIntersectGravityModifier
//====================================================================
static bool DoesLinkIntersectGravityModifier(const Vec3& p1, const Vec3& p2, const Vec3& p3,
    const GravityModifiers& gravityModifiers)
{
    return DoesLinkIntersectGravityModifier(p1, p2, gravityModifiers) || DoesLinkIntersectGravityModifier(p2, p3, gravityModifiers);
}

//====================================================================
// GetGravityModifiers
//====================================================================
void GetGravityModifiers(GravityModifiers& gravityModifiers, const AABB& aabb)
{
    //  IPhysicalEntity ** ppList = 0;
    //  int numEntites = gEnv->pPhysicalWorld->GetEntitiesInBox(aabb.min, aabb.max, ppList, ent_areas);

    IPhysicalEntity* pArea = gEnv->pPhysicalWorld->GetNextArea(0);

    while (pArea)
    {
        pe_params_area gravityParams;
        pArea->GetParams(&gravityParams);
        if (!is_unused(gravityParams.gravity) && gravityParams.gravity.GetLengthSquared() > 1.0f)
        {
            gravityModifiers.push_back(SGravityModifier());
            SGravityModifier& gm = gravityModifiers.back();
            gm.pEntity = pArea;

            pe_params_bbox params;
            pArea->GetParams(&params);
            gm.aabb.min = params.BBox[0];
            gm.aabb.max = params.BBox[1];
            gm.aabbCentre = 0.5f * (gm.aabb.min + gm.aabb.max);

            pe_status_pos posParams;
            gm.gravityStrength = gravityParams.gravity.GetLength();

            if (gm.gravityStrength < 0.01f || gm.aabb.GetSize().GetLengthSquared() < 0.01f)
            {
                gravityModifiers.pop_back();
            }
        }

        pArea = gEnv->pPhysicalWorld->GetNextArea(pArea);
    }
}

//====================================================================
// PathSegmentWorldIntersection
//====================================================================
bool CVolumeNavRegion::PathSegmentWorldIntersection(const Vec3& _start,
    const Vec3& _end,
    float _radius,
    bool intersectAtStart,
    bool intersectAtEnd,
    EAICollisionEntities aiCollisionEntities) const
{
    Vec3 start(_start);
    Vec3 end(_end);
    float radius(_radius);

    Vec3 segDir = end - start;
    float segLen = segDir.NormalizeSafe(Vec3_OneX);

    float effLen = segLen;
    if (!intersectAtEnd)
    {
        effLen -= radius;
    }
    if (!intersectAtStart)
    {
        effLen -= radius;
    }

    if (effLen < 0.0f)
    {
        // just do sphere test at the mid-point
        if (!intersectAtStart && intersectAtEnd)
        {
            start += min(radius, segLen) * segDir;
        }
        else if (!intersectAtEnd && intersectAtStart)
        {
            end -= min(radius, segLen) * segDir;
        }
        Vec3 pos = 0.5f * (start + end);
        return OverlapSphere(pos, radius, aiCollisionEntities);
    }
    else
    {
        if (!intersectAtEnd)
        {
            end -= radius * segDir;
        }
        if (!intersectAtStart)
        {
            start += radius * segDir;
        }
        return OverlapCapsule(Lineseg(start, end), radius, aiCollisionEntities);
    }
}

//====================================================================
// MultiRayWorldIntersection
// does a raycast from start to end, as well as numExtraRays from
// points around start/end to approximate a cylinder. Returns true
// if any of these rays intersect
// numExtraRays should either be 0 or >= 3
//====================================================================
static bool MultiRayWorldIntersection(const Vec3& start,
    const Vec3& end,
    float radius,
    int numExtraRays = 0)
{
    AIAssert(gEnv->pPhysicalWorld);
    if (!gEnv->pPhysicalWorld)
    {
        return false;
    }

    // get normalised dir from start to end
    Vec3 lookDir = end - start;
    float len = lookDir.GetLength();
    if (len <= radius)
    {
        return false;
    }
    lookDir /= len;

    ray_hit hit;
    int rayResult = gEnv->pPhysicalWorld->RayWorldIntersection(
            start, end - start,
            ent_static, rwi_stop_at_pierceable, &hit, 1);
    if (rayResult)
    {
        return true;
    }

    // if numExtraRays > 0 then it should be >= 3
    if (0 == numExtraRays)
    {
        return false;
    }

    if (radius <= 0.0f)
    {
        return false;
    }

    AIAssert(3 <= numExtraRays);
    if (3 > numExtraRays)
    {
        return false;
    }

    // get normalised "up" vector - arbitrary except it's normal to lookDir
    Vec3 upDir;
    if (abs(lookDir * Vec3(1.0f, 0.0f, 0.0f)) >
        abs(lookDir * Vec3(0.0f, 1.0f, 0.0f)))
    {
        upDir = lookDir ^ Vec3(0.0f, 1.0f, 0.0f);
    }
    else
    {
        upDir = lookDir ^ Vec3(1.0f, 0.0f, 0.0f);
    }

    // do the extra rays to approximate a swept volume, but start/end them
    // forward/back (by 45 deg) from the original start/end point
    float newLen = len - 2.0f * radius;
    if (newLen <= 0.01f)
    {
        return false;
    }
    Vec3 extraRayDir = (newLen / len) * (end - start);
    for (int iRay = 0; iRay < numExtraRays; ++iRay)
    {
        float angle = (gf_PI * iRay) / numExtraRays;
        Vec3 offset = radius * (Matrix33::CreateRotationAA(angle, lookDir) * upDir);
        offset += radius * lookDir;
        rayResult = gEnv->pPhysicalWorld->RayWorldIntersection(
                start, offset,
                ent_static, rwi_stop_at_pierceable, &hit, 1);
        if (rayResult)
        {
            return true;
        }
        rayResult = gEnv->pPhysicalWorld->RayWorldIntersection(
                start + offset, extraRayDir,
                ent_static, rwi_stop_at_pierceable, &hit, 1);
        if (rayResult)
        {
            return true;
        }
    }
    return false;
}

//====================================================================
// DoesPathIntersectWorld
//====================================================================
bool CVolumeNavRegion::DoesPathIntersectWorld(std::vector<Vec3>& path,
    float radius,
    EAICollisionEntities aiCollisionEntities)
{
    AIAssert(gEnv->pPhysicalWorld);
    if (!gEnv->pPhysicalWorld)
    {
        return false;
    }

    unsigned numPts = path.size();
    if (numPts <= 1)
    {
        return false;
    }
    for (unsigned i = 0; i < numPts - 1; ++i)
    {
        unsigned iNext = i + 1;
        if (PathSegmentWorldIntersection(path[i], path[iNext], radius, false, true, aiCollisionEntities))
        {
            return true;
        }
    }
    return false;
}

//
//  some generic helper functions over
//
//-----------------------------------------------------------------------------------------------------
//***********************************************************************************************************************************

//====================================================================
// CVolumeNavRegion
//====================================================================
CVolumeNavRegion::CVolumeNavRegion(CNavigation* pNavigation)
    : m_pNavigation(pNavigation)
    , m_pGraph(pNavigation->GetGraph())
    , m_hideSpots(Vec3(30, 30, 30), 4096)
{
    AIAssert(m_pGraph);
    Clear();
}

//====================================================================
// CVolumeNavRegion
//====================================================================
CVolumeNavRegion::~CVolumeNavRegion(void)
{
    Clear();
}

//====================================================================
// Clear
//====================================================================
void CVolumeNavRegion::Clear()
{
    while (!m_volumes.empty())
    {
        unsigned nodeIndex = m_volumes.back()->m_graphNodeIndex;
        m_pGraph->Disconnect(nodeIndex, true);
        delete m_volumes.back();
        m_volumes.pop_back();
    }
    m_maxVolumeRadius = 0.0f;
    m_portals.clear();
}

//====================================================================
// IsPointInVolume
//====================================================================
inline bool CVolumeNavRegion::IsPointInVolume(const CVolume& vol, const Vec3& pos, float extra) const
{
    float distSq = (vol.Center(m_pGraph->GetNodeManager()) - pos).GetLengthSquared();
    if (distSq > square(vol.m_radius + extra))
    {
        return false;
    }

    if (MultiRayWorldIntersection(vol.Center(m_pGraph->GetNodeManager()), pos, 0.0f))
    {
        return false;
    }

    return true;
}

//====================================================================
// GetVoxelValue
//====================================================================
inline CVolumeNavRegion::TVoxelValue CVolumeNavRegion::GetVoxelValue(const SVoxelData& voxelData, int xIdx, int yIdx, int zIdx) const
{
    return voxelData.voxels[(zIdx * voxelData.dim.y + yIdx) * voxelData.dim.x + xIdx];
}

//====================================================================
// IndexValid
//====================================================================
inline bool IndexValid(int i, int j, int k, int maxI, int maxJ, int maxK)
{
    return (i >= 0 && i < maxI && j >= 0 && j < maxJ && k >= 0 && k < maxK);
}

//====================================================================
// GetVoxelDistanceFieldGradient
//====================================================================
Vec3 CVolumeNavRegion::GetVoxelDistanceFieldGradient(SVoxelData& voxelData, int x, int y, int z)
{
    Vec3 dir(0, 0, 0);

    TDistanceVoxelValue cv = GetDistanceVoxelValue(voxelData, x, y, z);

    for (int i = -1; i <= 1; ++i)
    {
        for (int j = -1; j <= 1; ++j)
        {
            for (int k = -1; k <= 1; ++k)
            {
                if (i == 0 && j == 0 && k == 0)
                {
                    continue;
                }
                if (!IndexValid(x + i, y + j, z + k, voxelData.dim.x, voxelData.dim.y, voxelData.dim.z))
                {
                    continue;
                }

                TDistanceVoxelValue v = GetDistanceVoxelValue(voxelData, x + i, y + j, z + k);

                Vec3 delta(i, j, k);
                delta *= cv - v;
                dir += delta;
            }
        }
    }

    dir.NormalizeSafe();

    return dir;
}

inline CVolumeNavRegion::TDistanceVoxelValue CVolumeNavRegion::GetDistanceVoxelValue(const SVoxelData& voxelData, int xIdx, int yIdx, int zIdx) const
{
    unsigned index = (zIdx * voxelData.dim.y + yIdx) * voxelData.dim.x + xIdx;
    return (index < voxelData.distanceVoxels.size()) ? voxelData.distanceVoxels[index] : 0;
}

//====================================================================
// SetVoxelValue
//====================================================================
inline void CVolumeNavRegion::SetVoxelValue(SVoxelData& voxelData, int xIdx, int yIdx, int zIdx, TVoxelValue value)
{
    voxelData.voxels[(zIdx * voxelData.dim.y + yIdx) * voxelData.dim.x + xIdx] = value;
}

//====================================================================
// AdvanceVoxel
//====================================================================
inline bool CVolumeNavRegion::AdvanceVoxel(SVoxelData& voxelData, int xIdx, int yIdx, int zIdx, TVoxelValue threshold)
{
    if ((IndexValid(xIdx, yIdx, zIdx, voxelData.dim.x,   voxelData.dim.y, voxelData.dim.z) && GetVoxelValue(voxelData, xIdx, yIdx, zIdx)   < threshold) ||
        (IndexValid(xIdx - 1, yIdx, zIdx, voxelData.dim.x, voxelData.dim.y, voxelData.dim.z) && GetVoxelValue(voxelData, xIdx - 1, yIdx, zIdx) < threshold) ||
        (IndexValid(xIdx + 1, yIdx, zIdx, voxelData.dim.x, voxelData.dim.y, voxelData.dim.z) && GetVoxelValue(voxelData, xIdx + 1, yIdx, zIdx) < threshold) ||
        (IndexValid(xIdx, yIdx - 1, zIdx, voxelData.dim.x, voxelData.dim.y, voxelData.dim.z) && GetVoxelValue(voxelData, xIdx, yIdx - 1, zIdx) < threshold) ||
        (IndexValid(xIdx, yIdx + 1, zIdx, voxelData.dim.x, voxelData.dim.y, voxelData.dim.z) && GetVoxelValue(voxelData, xIdx, yIdx + 1, zIdx) < threshold) ||
        (IndexValid(xIdx, yIdx, zIdx - 1, voxelData.dim.x, voxelData.dim.y, voxelData.dim.z) && GetVoxelValue(voxelData, xIdx, yIdx, zIdx - 1) < threshold) ||
        (IndexValid(xIdx, yIdx, zIdx + 1, voxelData.dim.x, voxelData.dim.y, voxelData.dim.z) && GetVoxelValue(voxelData, xIdx, yIdx, zIdx + 1) < threshold))
    {
        return false;
    }
    SetVoxelValue(voxelData, xIdx, yIdx, zIdx, threshold + 1);
    return true;
}

//====================================================================
// GetAABBFromShape
//====================================================================
static AABB GetAABBFromShape(const ListPositions lstPolygon, float fHeight)
{
    AABB result(AABB::RESET);
    for (ListPositions::const_iterator itrPos = lstPolygon.begin(); itrPos != lstPolygon.end(); ++itrPos)
    {
        const Vec3& pt = (*itrPos);
        result.Add(pt);
        result.Add(pt + Vec3(0, 0, fHeight));
    }
    return result;
}

//====================================================================
// StitchRegionsTogether
//====================================================================
void CVolumeNavRegion::StitchRegionsTogether(std::vector<SRegionData>& regions)
{
    unsigned nRegions = regions.size();
    // first get the volumes that are on the edges
    for (unsigned i = 0; i < nRegions; ++i)
    {
        SRegionData& regionData = regions[i];
        regionData.edgeVolumes.clear();

        unsigned nVolumes = regionData.volumes.size();
        for (unsigned j = 0; j < nVolumes; ++j)
        {
            CVolume* pVolume = regionData.volumes[j];
            AABB volAABB(pVolume->Center(m_pGraph->GetNodeManager()) + Vec3(pVolume->m_radius, pVolume->m_radius, pVolume->m_radius),
                pVolume->Center(m_pGraph->GetNodeManager()) - Vec3(pVolume->m_radius, pVolume->m_radius, pVolume->m_radius));
            bool volCompletelyInside = (0x02 == Overlap::AABB_AABB_Inside(volAABB, regionData.aabb));
            if (!volCompletelyInside)
            {
                regionData.edgeVolumes.push_back(pVolume);
            }
        }
    }

    // now pair up volumes
    for (unsigned iR1 = 0; iR1 < nRegions; ++iR1)
    {
        SRegionData& rd1 = regions[iR1];

        unsigned nV1 = rd1.edgeVolumes.size();
        for (unsigned v1 = 0; v1 < nV1; ++v1)
        {
            CVolume* pV1 = rd1.edgeVolumes[v1];

            // now check against the others
            for (unsigned iR2 = 0; iR2 < nRegions; ++iR2)
            {
                if (iR2 == iR1)
                {
                    continue;
                }
                SRegionData& rd2 = regions[iR2];

                float rSumSq = square(rd1.volumeRadius + rd2.volumeRadius);

                unsigned nV2 = rd2.edgeVolumes.size();
                for (unsigned v2 = 0; v2 < nV2; ++v2)
                {
                    CVolume* pV2 = rd2.edgeVolumes[v2];
                    // arbitrary - only attempt cxn one way
                    if (pV1 < pV2)
                    {
                        continue;
                    }

                    float distSq = (pV1->Center(m_pGraph->GetNodeManager()) - pV2->Center(m_pGraph->GetNodeManager())).GetLengthSquared();
                    if (distSq > rSumSq)
                    {
                        continue;
                    }

                    GeneratePortalVxl(m_portals, *pV1, *pV2);
                }
            }
        }
    }
}

//====================================================================
// Generate3DDebugVoxels
//====================================================================
void CVolumeNavRegion::Generate3DDebugVoxels()
{
    ICVar* p_debugDrawVolumeVoxels = gEnv->pConsole->GetCVar ("ai_DebugDrawVolumeVoxels");
    if (p_debugDrawVolumeVoxels)
    {
        p_debugDrawVolumeVoxels->Set(2);
    }

    m_debugVoxels.clear();

    CNavigation::VolumeRegions volumeRegions;
    m_pNavigation->GetVolumeRegions(volumeRegions);

    std::vector<SRegionData> allRegionData;

    unsigned nRegions = volumeRegions.size();
    for (unsigned i = 0; i < nRegions; ++i)
    {
        const string& regionName = volumeRegions[i].first;
        const SpecialArea* sa = volumeRegions[i].second;

        SRegionData regionData;

        // slightly dodgy - this assumes that the ai nav modifier name is prefixed with a 64-bit ptr sized
        // identifier (in hex), followed by space (my hack to ensure uniqueness).
        if (regionName.length() > 17)
        {
            regionData.name = string(regionName.c_str() + 17);
        }
        else
        {
            AIWarning("CVolumeNavRegion::Generate region name is too short %s", regionData.name.c_str());
            continue;
        }
        regionData.sa = sa;

        regionData.aabb = GetAABBFromShape(sa->GetPolygon(), sa->fHeight);
        regionData.volumeRadius = sa->f3DNavVolumeRadius;

        regionData.voxelData.corner = regionData.aabb.min;
        regionData.voxelData.size = volumeVoxelSize;

        // voxelisation will store the result in m_debugVoxels
        if (regionData.sa->bCalculate3DNav)
        {
            AILogProgress("Generating debug voxels for %s", regionName.c_str());

            if (!Voxelize(regionData))
            {
                AIWarning("Failed to voxelise region %s", regionName.c_str());
            }
        }
    }
}

//====================================================================
// Generate
// For each region either load it from file or calculate it.
//====================================================================
void CVolumeNavRegion::Generate(const char* szLevel, const char* szMission)
{
    Clear();
    m_AABB.Reset();

    m_debugVoxels.clear();

    CNavigation::VolumeRegions volumeRegions;
    m_pNavigation->GetVolumeRegions(volumeRegions);

    std::vector<SRegionData> allRegionData;

    unsigned nRegions = volumeRegions.size();
    for (unsigned i = 0; i < nRegions; ++i)
    {
        const string& regionName = volumeRegions[i].first;
        const SpecialArea* sa = volumeRegions[i].second;

        allRegionData.push_back(SRegionData());
        SRegionData& regionData = allRegionData.back();

        // slightly dodgy - this assumes that the ai nav modifier name is prefixed with a 64-bit ptr-sized
        // identifier (in hex), followed by space (my hack to ensure uniqueness).
        if (regionName.length() > 17)
        {
            regionData.name = string(regionName.c_str() + 17);
        }
        else
        {
            AIWarning("CVolumeNavRegion::Generate region name is too short %s", regionData.name.c_str());
            allRegionData.pop_back();
            continue;
        }
        regionData.sa = sa;

        regionData.aabb = GetAABBFromShape(sa->GetPolygon(), sa->fHeight);
        regionData.volumeRadius = sa->f3DNavVolumeRadius;

        regionData.voxelData.corner = regionData.aabb.min;
        regionData.voxelData.size = volumeVoxelSize;

        // This will generate the volumes for the regions and stitch things up within each
        // region
        if (regionData.sa->bCalculate3DNav)
        {
            CalculateRegion(regionData);
        }
        else
        {
            LoadRegion(szLevel, szMission, regionData);
        }
    }
    AILogProgress("Loaded/calculated all volume regions - now combining/stitching regions together");

    // put all the individual regions into the single containers but don't change the indices (i.e.
    // just set things up for hide-point finding.
    CombineRegions(allRegionData, false);

    AILogProgress("Calculating hidespots");
    // Now the navigation data is done we can calculate hidespots and save
    m_hideSpots.Clear(true);

    for (unsigned i = 0; i < nRegions; ++i)
    {
        SRegionData& regionData = allRegionData[i];
        if (regionData.sa->bCalculate3DNav)
        {
            // todo Danny/Mikko return early since there are some issues with calculating
            // hidespots, and anyway they're not used.
            //#if 0
            //      CalculateHideSpots(regionData, regionData.hideSpots);
            //#endif
            SaveRegion(szLevel, szMission, regionData);
        }
    }

    // put all the individual regions into the single containers
    CombineRegions(allRegionData, true);

    // now we have the individual regions stitch them together
    StitchRegionsTogether(allRegionData);
}

//====================================================================
// CombineRegions
//====================================================================
void CVolumeNavRegion::CombineRegions(std::vector<SRegionData>& allRegions, bool adjustIndices)
{
    m_AABB.Reset();
    m_volumes.clear();
    m_portals.clear();
    m_hideSpots.Clear(true);

    unsigned nRegions = allRegions.size();
    for (unsigned i = 0; i < nRegions; ++i)
    {
        SRegionData& regionData = allRegions[i];

        m_AABB.Add(regionData.aabb);
        // copy the volumes/portals into the single list, and update pointers/indices etc
        unsigned origNumVolumes = m_volumes.size();
        unsigned origNumPortals = m_portals.size();
        unsigned nNewVolumes = regionData.volumes.size();
        m_portals.insert(m_portals.end(), regionData.portals.begin(), regionData.portals.end());
        for (unsigned j = 0; j < nNewVolumes; ++j)
        {
            CVolume* pVolume = regionData.volumes[j];
            m_volumes.push_back(pVolume);
            if (adjustIndices)
            {
                m_pGraph->GetNodeManager().GetNode(pVolume->m_graphNodeIndex)->GetVolumeNavData()->nVolimeIdx += origNumVolumes;
                for (unsigned k = 0; k < pVolume->m_portalIndices.size(); ++k)
                {
                    AIAssert(pVolume->m_portalIndices[k] < regionData.portals.size());
                    pVolume->m_portalIndices[k] += origNumPortals;
                }
            }
        }

        unsigned nHideSpots = regionData.hideSpots.size();
        for (unsigned j = 0; j < nHideSpots; ++j)
        {
            m_hideSpots.AddObject(regionData.hideSpots[j]);
        }
    }
}

//====================================================================
// CalculateRegion
//====================================================================
void CVolumeNavRegion::CalculateRegion(SRegionData& regionData)
{
    AILogProgress("[AISystem:VolumeNavRegion] Calculating region %s", regionData.name.c_str());
    float absoluteStartTime = gEnv->pTimer->GetAsyncCurTime();

    int   dbgCounter = 0;

    Vec3  seedPoint;
    if (!Voxelize(regionData))
    {
        return;
    }

    float fStartTime = gEnv->pTimer->GetAsyncCurTime();
    GenerateBasicVolumes(regionData);

    regionData.hideSpots.clear();

    Vec3i ptIndex;
    int voxelsLeft = 0;
    int origNumVoxels = -1;
    while (FindSeedPoint(regionData.voxelData, seedPoint, ptIndex, voxelsLeft))
    {
        if (origNumVoxels < 0)
        {
            origNumVoxels = voxelsLeft;
        }
        // Create a volume, and adjust the voxel distances
        CVolume* pVolume = CreateVolume(regionData.volumes, seedPoint, regionData.volumeRadius,
                GetDistanceVoxelValue(regionData.voxelData, ptIndex.x, ptIndex.y, ptIndex.z));

        if (pVolume->m_distanceToGeometry > 1.0f && pVolume->m_distanceToGeometry < regionData.volumeRadius * 2.5f)
        {
            Vec3 dir = GetVoxelDistanceFieldGradient(regionData.voxelData, ptIndex.x, ptIndex.y, ptIndex.z);
            if (!dir.IsZero())
            {
                regionData.hideSpots.push_back(SVolumeHideSpot(pVolume->Center(m_pGraph->GetNodeManager()), dir));
            }
        }

        VoxelizeInvert(regionData, pVolume, ptIndex);

        AILogProgress("[AISystem:VolumeNavRegion] created volume #%d (%d = %5.2f%%%% voxels left)", dbgCounter, voxelsLeft, (100.0f *  voxelsLeft) / origNumVoxels);
        ++dbgCounter;
    }
    float fEndTime = gEnv->pTimer->GetAsyncCurTime();
    AILogProgress("[AISystem:VolumeNavRegion] Volume creation complete in %6.3f sec", fEndTime - fStartTime);

    // don't need voxels anymore
    regionData.voxelData.voxels.clear();

    // connect volumes
    AILogProgress("[AISystem:VolumeNavRegion] generating portals.....");
    fStartTime = gEnv->pTimer->GetAsyncCurTime();
    GeneratePortals(regionData);
    fEndTime = gEnv->pTimer->GetAsyncCurTime();
    AILogProgress("[AISystem:VolumeNavRegion] Portals created in %6.3f sec", fEndTime - fStartTime);

    AILogProgress("[AISystem:VolumeNavRegion] created %lu volumes in %6.3f seconds",
        regionData.volumes.size(), fEndTime - absoluteStartTime);
}



//====================================================================
// CalculatePassRadius
//====================================================================
float CVolumeNavRegion::CalculatePassRadius(CPortal& portal, const std::vector<float>& radii)
{
    for (unsigned iRad = 0; iRad < radii.size(); ++iRad)
    {
        float radius = radii[iRad] + 0.05f;

        bool intersect =
            PathSegmentWorldIntersection(portal.m_pVolumeOne->Center(m_pGraph->GetNodeManager()), portal.m_passPoint, radius, false, true, AICE_STATIC) ||
            PathSegmentWorldIntersection(portal.m_pVolumeTwo->Center(m_pGraph->GetNodeManager()), portal.m_passPoint, radius, false, true, AICE_STATIC);
        if (!intersect)
        {
            return radius;
        }
    }

    return -1.0f;
}

//====================================================================
// CalcClosestPointFromVolumeToPos
//====================================================================
Vec3 CVolumeNavRegion::CalcClosestPointFromVolumeToPos(const CVolume& volume, const Vec3& pos, float passRadius) const
{
    float hitDist;
    Lineseg lineseg(volume.Center(m_pGraph->GetNodeManager()), pos);
    if (IntersectSweptSphere(0, hitDist, lineseg, passRadius, AICE_ALL))
    {
        float linesegLen = Distance::Point_Point(lineseg.start, lineseg.end);
        float frac = hitDist / linesegLen;
        return frac * lineseg.end + (1.0f - frac) * lineseg.start;
    }
    else
    {
        return pos;
    }
}

//====================================================================
// GetEnclosing
//====================================================================
unsigned CVolumeNavRegion::GetEnclosing(const Vec3& pos, float passRadius, unsigned startIndex,
    float range, Vec3* closestValid, bool returnSuspect, const char* requesterName)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

    if (m_volumes.empty())
    {
        return 0;
    }

    // A range value less than zero results in get search based on m_maxVolumeRadius.
    if (range < 0.0f)
    {
        range = 0.0f;
    }

    if (closestValid)
    {
        *closestValid = pos;
    }

    static std::vector< std::pair<float, const CVolume*> > distVolumePairs;
    distVolumePairs.resize(0);

    static bool doNew = true;
    if (doNew)
    {
        // for big levels this is faster. For small levels brute force is faster...
        typedef std::vector< std::pair<float, unsigned> > TNodes;
        static TNodes nodes;
        nodes.resize(0);
        CAllNodesContainer& allNodes = m_pGraph->GetAllNodes();
        float totalDist = 2.0f * m_maxVolumeRadius + range;
        allNodes.GetAllNodesWithinRange(nodes, pos, totalDist, IAISystem::NAV_VOLUME);

        unsigned nNodes = nodes.size();
        for (unsigned iNode = 0; iNode < nNodes; ++iNode)
        {
            std::pair<float, unsigned>& nodePair = nodes[iNode];
            const GraphNode* pNode = m_pGraph->GetNodeManager().GetNode(nodePair.second);
            const CVolume* pVolume = GetVolume(m_volumes, pNode->GetVolumeNavData()->nVolimeIdx);

            float distSq = nodePair.first;

            // TODO/HACK! This code is potentially costly. It is here to solve some specific
            // problems in one of the Crysis levels. Ideally this should not be called.
            if (AreAllLinksBlocked(pVolume->m_graphNodeIndex, passRadius))
            {
                distSq += sqr(1000);
            }
            // End hack.

            distVolumePairs.push_back(std::pair<float, const CVolume*>(distSq, pVolume));
        }
    }
    else
    {
        unsigned numVolumes = m_volumes.size();
        // first time - do quick check - this should get it almost every time.
        for (unsigned iVolume = 0; iVolume < numVolumes; ++iVolume)
        {
            const CVolume& volume = *m_volumes[iVolume];
            float radiusExtraSq = square(2.0f * volume.m_radius + range); // be generous with the volumes found
            float distSq = (volume.Center(m_pGraph->GetNodeManager()) - pos).GetLengthSquared();
            if (distSq < radiusExtraSq)
            {
                distVolumePairs.push_back(std::pair<float, const CVolume*>(distSq, &volume));
            }
        }
    }

    unsigned numDistVolumePairs = distVolumePairs.size();
    if (numDistVolumePairs == 0)
    {
        return 0;
    }

    // sorting on pairs is done by the first value
    std::sort(distVolumePairs.begin(), distVolumePairs.end());

    // first try to get a node considering all entities. If that fails then hope
    // that ignoring dynamic entities (e.g. frozen bodies) is ok
    EAICollisionEntities collTypes[2] = {AICE_ALL, AICE_STATIC};
    for (int iCollType = 0; iCollType < 2; ++iCollType)
    {
        EAICollisionEntities collType = collTypes[iCollType];

        for (unsigned iVolume = 0; iVolume < numDistVolumePairs; ++iVolume)
        {
            const CVolume& volume = *distVolumePairs[iVolume].second;
            if (PathSegmentWorldIntersection(volume.Center(m_pGraph->GetNodeManager()), pos, passRadius, false, false, collType))
            {
                continue;
            }
            if (passRadius > m_pGraph->GetNodeManager().GetNode(volume.m_graphNodeIndex)->GetMaxLinkRadius(m_pGraph->GetLinkManager()))
            {
                continue;
            }
            if (closestValid)
            {
                *closestValid = CalcClosestPointFromVolumeToPos(volume, pos, passRadius);
            }
            return volume.m_graphNodeIndex;
        }
    }

    if (!returnSuspect)
    {
        return 0;
    }

    for (unsigned iVolume = 0; iVolume < numDistVolumePairs; ++iVolume)
    {
        const CVolume& volume = *distVolumePairs[iVolume].second;
        if (MultiRayWorldIntersection(volume.Center(m_pGraph->GetNodeManager()), pos, 0.0f))
        {
            continue;
        }
        if (passRadius > m_pGraph->GetNodeManager().GetNode(volume.m_graphNodeIndex)->GetMaxLinkRadius(m_pGraph->GetLinkManager()))
        {
            continue;
        }
        if (closestValid)
        {
            *closestValid = CalcClosestPointFromVolumeToPos(volume, pos, passRadius);
        }
        return volume.m_graphNodeIndex;
    }

    for (unsigned iVolume = 0; iVolume < numDistVolumePairs; ++iVolume)
    {
        const CVolume& volume = *distVolumePairs[iVolume].second;
        if (passRadius > m_pGraph->GetNodeManager().GetNode(volume.m_graphNodeIndex)->GetMaxLinkRadius(m_pGraph->GetLinkManager()))
        {
            continue;
        }
        AIWarning("Cannot find node for %s position: (%5.2f, %5.2f, %5.2f) - returning closest even though no line of sight",
            requesterName, pos.x, pos.y, pos.z);
        if (closestValid)
        {
            *closestValid = CalcClosestPointFromVolumeToPos(volume, pos, passRadius);
        }
        return volume.m_graphNodeIndex;
    }

    AIWarning("Cannot find any node for %s position: (%5.2f, %5.2f, %5.2f) - returning closest (no line of sight and radius is bad)",
        requesterName, pos.x, pos.y, pos.z);
    if (closestValid)
    {
        *closestValid = CalcClosestPointFromVolumeToPos(*distVolumePairs[0].second, pos, passRadius);
    }
    return distVolumePairs[0].second->m_graphNodeIndex;
}

//====================================================================
// AreAllLinksBlocked
//====================================================================
bool CVolumeNavRegion::AreAllLinksBlocked(unsigned nodeIndex, float passRadius)
{
    const GraphNode* pNode = m_pGraph->GetNodeManager().GetNode(nodeIndex);
    if (!pNode)
    {
        return false;
    }

    unsigned blocked = 0;
    unsigned count = 0;
    for (unsigned link = pNode->firstLinkIndex; link; link = m_pGraph->GetLinkManager().GetNextLink(link))
    {
        unsigned connectedIndex = m_pGraph->GetLinkManager().GetNextNode(link);
        GraphNode* pConnectedNode = m_pGraph->GetNodeManager().GetNode(connectedIndex);
        if (!pConnectedNode)
        {
            continue;
        }

        count++;

        if (m_pGraph->GetLinkManager().GetRadius(link) < passRadius)
        {
            blocked++;
            continue;
        }

        float extraLinkFactor = m_pNavigation->GetExtraLinkCost(pNode->GetPos(), pConnectedNode->GetPos());
        if (extraLinkFactor < 0.0f)
        {
            blocked++;
            continue;
        }
    }

    return blocked == count;
}

//====================================================================
// GetConnectionPoint
//====================================================================
Vec3 CVolumeNavRegion::GetConnectionPoint(TVolumes& volumes, TPortals& portals, int volumeIdx1, int volumeIdx2) const
{
    if (volumeIdx1 < 0 || static_cast<size_t>(volumeIdx1) >= volumes.size() ||
        volumeIdx2 < 0 || static_cast<size_t>(volumeIdx2) >= volumes.size())
    {
        AIWarning("[AISystem] CVolumeNavRegion::GetConnectionPoint error  >> %d %d ", volumeIdx1, volumeIdx2);
        return Vec3(0, 0, 0);
    }
    return GetVolume(volumes, volumeIdx1)->GetConnectionPoint(this, GetVolume(volumes, volumeIdx2));
}

//====================================================================
// GetVolume
//====================================================================
CVolumeNavRegion::CVolume* CVolumeNavRegion::GetVolume(TVolumes& volumes, int volumeIdx) const
{
    if (volumeIdx < 0 || static_cast<size_t>(volumeIdx) >= volumes.size())
    {
        return NULL;
    }
    return volumes[volumeIdx];
}

//====================================================================
// GetVolumeIdx
//====================================================================
int CVolumeNavRegion::GetVolumeIdx(const TVolumes& volumes, const CVolumeNavRegion::CVolume* pVolume) const
{
    TVolumes::const_iterator itr = std::find(volumes.begin(), volumes.end(), pVolume);
    if (itr == volumes.end())
    {
        return -1;
    }
    return itr - volumes.begin();
}

//====================================================================
// GetPortal
//====================================================================
CVolumeNavRegion::CPortal* CVolumeNavRegion::GetPortal(TPortals& portals, int portalIdx)
{
    if (portalIdx < 0 || static_cast<size_t>(portalIdx) >= portals.size())
    {
        return NULL;
    }
    return &portals[portalIdx];
}

//====================================================================
// GetPortal
//====================================================================
const CVolumeNavRegion::CPortal* CVolumeNavRegion::GetPortal(const TPortals& portals, int portalIdx) const
{
    if (portalIdx < 0 || static_cast<size_t>(portalIdx) >= portals.size())
    {
        return NULL;
    }
    return &portals[portalIdx];
}

//====================================================================
// GetPortalIdx
//====================================================================
int CVolumeNavRegion::GetPortalIdx(const TPortals& portals, const CVolumeNavRegion::CPortal* pPortal) const
{
    if (portals.empty())
    {
        return -1;
    }
    int index = pPortal - &portals[0];
    if (index < 0)
    {
        return -1;
    }
    else if (index >= (int) portals.size())
    {
        return -1;
    }
    else
    {
        return index;
    }
}

//====================================================================
// FindSeedPoint
// find the point from which to grow the next value
//====================================================================
bool CVolumeNavRegion::FindSeedPoint(SVoxelData& voxelData, Vec3& point, Vec3i& ptIndex, int& voxelsLeft) const
{
    voxelsLeft = 0;
    TVoxelValue bestVoxelValue = 0;
    for (int zIdx = 1; zIdx < voxelData.dim.z - 1; ++zIdx)
    {
        for (int yIdx = 1; yIdx < voxelData.dim.y - 1; ++yIdx)
        {
            for (int xIdx = 1; xIdx < voxelData.dim.x - 1; ++xIdx)
            {
                TVoxelValue voxelValue = GetVoxelValue(voxelData, xIdx, yIdx, zIdx);
                if (voxelValue > 0)
                {
                    ++voxelsLeft;
                    if (voxelValue > bestVoxelValue)
                    {
                        bestVoxelValue = voxelValue;
                        ptIndex.Set(xIdx, yIdx, zIdx);
                    }
                }
            }
        }
    }
    if (bestVoxelValue == 0)
    {
        return false;
    }

    point = voxelData.corner + voxelData.size * ptIndex;
    return true;
}

//====================================================================
// GetNavigableSpaceEntities
//====================================================================
void CVolumeNavRegion::GetNavigableSpaceEntities(std::vector<const IEntity*>& entities)
{
    entities.resize(0);

    if (!gEnv->pEntitySystem)
    {
        return;
    }

    // Need points that are definitely in valid space
    IEntityItPtr entIt = gEnv->pEntitySystem->GetEntityIterator();
    const char* navName = "NavigableSpace";
    size_t navNameLen = strlen(navName);
    if (entIt)
    {
        entIt->MoveFirst();
        while (!entIt->IsEnd())
        {
            IEntity* ent = entIt->Next();
            if (ent)
            {
                const char* name = ent->GetName();
                if (_strnicmp(name, navName, navNameLen) == 0)
                {
                    entities.push_back(ent);
                }
            }
        }
    }
}


//====================================================================
// GetNavigableSpacePoint
//====================================================================
bool CVolumeNavRegion::GetNavigableSpacePoint(const Vec3& refPos, Vec3& pos, const std::vector<const IEntity*>& entities)
{
    // use the closest NavigableSpace point
    const IEntity* ent = 0;
    float bestDistSq = std::numeric_limits<float>::max();
    unsigned nEntities = entities.size();
    for (unsigned i = 0; i < nEntities; ++i)
    {
        float distSq = (entities[i]->GetPos() - refPos).GetLengthSquared();
        if (distSq < bestDistSq)
        {
            bestDistSq = distSq;
            ent = entities[i];
        }
    }
    if (!ent)
    {
        return false;
    }

    pos = ent->GetPos();
    return true;
}

//====================================================================
// DoesVoxelContainGeometry
//====================================================================
inline bool CVolumeNavRegion::DoesVoxelContainGeometry(SVoxelData& voxelData, int i, int j, int k, primitives::box& boxPrim)
{
    IPhysicalWorld* pPhysics = gEnv->pPhysicalWorld;
    //  geom_contact *pContact = 0;
    boxPrim.center = voxelData.corner + Vec3(i * voxelData.size, j * voxelData.size, k * voxelData.size);
    float d = pPhysics->PrimitiveWorldIntersection(boxPrim.type, &boxPrim, Vec3(ZERO),
            ent_static | ent_sleeping_rigid | ent_rigid | ent_terrain | ent_ignore_noncolliding, 0,
            0, geom_colltype0, 0);
    return (d != 0.0f);
}

//====================================================================
// UpdateVoxelisationNeighbours
// voxel at (i, j, k) is navigable - for each neighbour if it's been
// processed, do nothing. If not, if it contains geometry mark it with
// 1, otherwise set it to unprocessed navigable
//====================================================================
inline void CVolumeNavRegion::UpdateVoxelisationNeighbours(SVoxelData& voxelData, int i, int j, int k, primitives::box& boxPrim)
{
    int di[] = {-1, 1, 0, 0, 0, 0};
    int dj[] = {0, 0, -1, 1, 0, 0};
    int dk[] = {0, 0, 0, 0, -1, 1};
    for (unsigned index = 0; index < 6; ++index)
    {
        int ii = i + di[index];
        int jj = j + dj[index];
        int kk = k + dk[index];
        if (ii >= 0 && ii < voxelData.dim.x &&
            jj >= 0 && jj < voxelData.dim.y &&
            kk >= 0 && kk < voxelData.dim.z)
        {
            TVoxelValue val = GetVoxelValue(voxelData, ii, jj, kk);
            if (val == 0)
            {
                bool geom = DoesVoxelContainGeometry(voxelData, ii, jj, kk, boxPrim);
                if (geom)
                {
                    SetVoxelValue(voxelData, ii, jj, kk, 1);
                }
                else
                {
                    SetVoxelValue(voxelData, ii, jj, kk, 2);
                }
            }
        }
    }
}

//====================================================================
// Voxelize
//====================================================================
bool CVolumeNavRegion::Voxelize(SRegionData& regionData)
{
    SVoxelData& voxelData = regionData.voxelData;

    Vec3  size = regionData.aabb.max - regionData.aabb.min;
    voxelData.dim.x = 1 + static_cast<size_t>((size.x / voxelData.size));
    voxelData.dim.y = 1 + static_cast<size_t>((size.y / voxelData.size));
    voxelData.dim.z = 1 + static_cast<size_t>((size.z / voxelData.size));

    AILogProgress("[AISystem:VolumeNavRegion] voxelizing. <%d x %d x %d> Total %d voxels.",
        voxelData.dim.x, voxelData.dim.y, voxelData.dim.z,
        voxelData.dim.x * voxelData.dim.y * voxelData.dim.z);

    float fStartTime = gEnv->pTimer->GetAsyncCurTime();

    voxelData.voxels.clear();

    primitives::box boxPrim;
    boxPrim.Basis.SetIdentity();
    boxPrim.bOriented = false;
    boxPrim.size.Set(0.51f * voxelData.size, 0.51f * voxelData.size, 0.51f * voxelData.size);

    // Mark voxels as unprocessed (0)
    for (size_t zIdx = 0; zIdx < voxelData.dim.z; ++zIdx)
    {
        for (size_t yIdx = 0; yIdx < voxelData.dim.y; ++yIdx)
        {
            for (size_t xIdx = 0; xIdx < voxelData.dim.x; ++xIdx)
            {
                Vec3 pt = voxelData.corner + Vec3(xIdx * voxelData.size, yIdx * voxelData.size, zIdx * voxelData.size);
                if (Overlap::Point_Polygon2D(pt, regionData.sa->GetPolygon(), &regionData.sa->GetAABB()))
                {
                    voxelData.voxels.push_back(0);
                }
                else
                {
                    voxelData.voxels.push_back(1);
                }
            }
        }
    }
    AIAssert(voxelData.voxels.size() == voxelData.dim.x * voxelData.dim.y * voxelData.dim.z);

    // Now for each navigable space entity in our region set that voxel to a value of 2 as geometry
    // will be set to 1
    std::vector<const IEntity*> entities;
    GetNavigableSpaceEntities(entities);
    unsigned nEntities = entities.size();
    unsigned nInRegion = 0;
    for (unsigned i = 0; i < nEntities; ++i)
    {
        Vec3 pos = entities[i]->GetPos();
        int indexX = (int)((pos.x - voxelData.corner.x) / voxelData.size);
        int indexY = (int)((pos.y - voxelData.corner.y) / voxelData.size);
        int indexZ = (int)((pos.z - voxelData.corner.z) / voxelData.size);

        if (indexX >= 0 && indexX < voxelData.dim.x &&
            indexY >= 0 && indexY < voxelData.dim.y &&
            indexZ >= 0 && indexZ < voxelData.dim.z)
        {
            bool geom = DoesVoxelContainGeometry(voxelData, indexX, indexY, indexZ, boxPrim);
            if (geom)
            {
                AIWarning("NavigableSpace marker at position: (%5.2f, %5.2f, %5.2f) too close to geometry", pos.x, pos.y, pos.z);
            }
            else
            {
                SetVoxelValue(voxelData, indexX, indexY, indexZ, 2);
                ++nInRegion;
            }
        }
    }
    if (nInRegion == 0)
    {
        AIError("CVolumeNavRegion Need at least valid point called NavigableSpace in region %s that indicates valid volume navigation space [Design bug]",
            regionData.name.c_str());
        return false;
    }

    // now a brute-force (benefit of no additional memory needed) flood fill - alternate directions to speed things up.
    // Once its neighbours are processed (0 values set to 1 (geometry) or 2 (unprocessed valid space)) a voxel
    // value is set to 3, 4, 5 etc (increments each iteration)
    AILogProgress("Starting voxelisation flood fill");
    int numProcessed = 1;
    TVoxelValue processedVal = 3;
    bool fmd = true;
    while (true)
    {
        AILogProgress("Flood fill %d", processedVal - 2);
        numProcessed = 0;
        for (unsigned i = 0; i < voxelData.dim.x; ++i)
        {
            for (unsigned j = 0; j < voxelData.dim.y; ++j)
            {
                for (unsigned k = 0; k < voxelData.dim.z; ++k)
                {
                    if (GetVoxelValue(voxelData, i, j, k) == 2)
                    {
                        UpdateVoxelisationNeighbours(voxelData, i, j, k, boxPrim);
                        SetVoxelValue(voxelData, i, j, k, processedVal);
                        ++numProcessed;
                    }
                }
            }
        }
        ++processedVal;
        if (0 == numProcessed)
        {
            break;
        }
        // reverse direction
        AILogProgress("Flood fill %d", processedVal - 2);
        numProcessed = 0;
        for (unsigned i = voxelData.dim.x; i-- != 0; )
        {
            for (unsigned j = voxelData.dim.y; j-- != 0; )
            {
                for (unsigned k = voxelData.dim.z; k-- != 0; )
                {
                    if (GetVoxelValue(voxelData, i, j, k) == 2)
                    {
                        UpdateVoxelisationNeighbours(voxelData, i, j, k, boxPrim);
                        SetVoxelValue(voxelData, i, j, k, processedVal);
                        ++numProcessed;
                    }
                }
            }
        }
        ++processedVal;
        if (0 == numProcessed)
        {
            break;
        }
    }

    AILogProgress("Finished voxelisation flood fill");

    /// for debugging store the flood value so we can find "leaks"
    ICVar* p_debugDrawVolumeVoxels = gEnv->pConsole->GetCVar ("ai_DebugDrawVolumeVoxels");
    if (p_debugDrawVolumeVoxels && p_debugDrawVolumeVoxels->GetIVal() != 0)
    {
        m_debugVoxels.push_back(voxelData);
    }

    // Now set all values > 1 to 1, all rest to 0, ready to advance + form the distance
    // field
    for (unsigned i = 0; i < voxelData.dim.x; ++i)
    {
        for (unsigned j = 0; j < voxelData.dim.y; ++j)
        {
            for (unsigned k = 0; k < voxelData.dim.z; ++k)
            {
                TVoxelValue val = GetVoxelValue(voxelData, i, j, k);
                if (val == 1)
                {
                    SetVoxelValue(voxelData, i, j, k, 0);
                }
                else if (val > 1)
                {
                    SetVoxelValue(voxelData, i, j, k, 1);
                }
            }
        }
    }

    float fEndTime = gEnv->pTimer->GetAsyncCurTime();
    AILogProgress("[AISystem:voxelization] complete in %6.3f sec", fEndTime - fStartTime);

    // advance - find the most "open" voxel - the farthest from geometry
    bool  isProgressing = true;
    TVoxelValue       threshold = 0;
    fStartTime = gEnv->pTimer->GetAsyncCurTime();
    while (isProgressing)
    {
        isProgressing = false;
        ++threshold;
        for (size_t zIdx = 0; zIdx < voxelData.dim.z; ++zIdx)
        {
            for (size_t yIdx = 0; yIdx < voxelData.dim.y; ++yIdx)
            {
                for (size_t xIdx = 0; xIdx < voxelData.dim.x; ++xIdx)
                {
                    if (AdvanceVoxel(voxelData, xIdx, yIdx, zIdx, threshold))
                    {
                        isProgressing = true;
                    }
                }
            }
        }
        AILogProgress("[AISystem:voxelization:advancing] threshold %d", threshold);
    }

    // finally copy the distances
    voxelData.distanceVoxels.clear();
    for (size_t i = 0; i < voxelData.voxels.size(); ++i)
    {
        TVoxelValue vv = voxelData.voxels[i];
        TDistanceVoxelValue dvv = 0;
        if (vv <= 0)
        {
            dvv = 0;
        }
        else if (vv >= std::numeric_limits<TDistanceVoxelValue>::max())
        {
            dvv = std::numeric_limits<TDistanceVoxelValue>::max();
        }
        else
        {
            dvv = (TDistanceVoxelValue) vv;
        }
        voxelData.distanceVoxels.push_back(dvv);
    }

    float fAllEndTime = gEnv->pTimer->GetAsyncCurTime();
    AILogProgress("[AISystem:voxelization]:advancing complete in %6.3f sec", fAllEndTime - fStartTime);
    return true;
}

//====================================================================
// CalculateVoxelDistances
//====================================================================
int CVolumeNavRegion::CalculateVoxelDistances(SVoxelData& voxelData, TVoxelValue maxThreshold)
{
    const Vec3i minIndex(0, 0, 0);
    const Vec3i maxIndex(voxelData.dim.x - 2, voxelData.dim.y - 2, voxelData.dim.z - 2);

    // zap things to 1 everywhere in valid space
    for (size_t zIdx = 0; zIdx < voxelData.dim.z - 1; ++zIdx)
    {
        for (size_t yIdx = 0; yIdx < voxelData.dim.y - 1; ++yIdx)
        {
            for (size_t xIdx = 0; xIdx < voxelData.dim.x - 1; ++xIdx)
            {
                if (GetVoxelValue(voxelData, xIdx, yIdx, zIdx) != 0)
                {
                    SetVoxelValue(voxelData, xIdx, yIdx, zIdx, 1);
                }
            }
        }
    }

    // see if there are any advanced voxels left; if not - we are done
    bool  isProgressing = true;
    int threshold = 0;
    while (isProgressing && threshold < maxThreshold)
    {
        isProgressing = false;
        ++threshold;
        for (size_t zIdx = 0; zIdx < voxelData.dim.z - 1; ++zIdx)
        {
            for (size_t yIdx = 0; yIdx < voxelData.dim.y - 1; ++yIdx)
            {
                for (size_t xIdx = 0; xIdx < voxelData.dim.x - 1; ++xIdx)
                {
                    if (AdvanceVoxel(voxelData, xIdx, yIdx, zIdx, threshold))
                    {
                        isProgressing = true;
                    }
                }
            }
        }
    }
    return threshold;
}

//====================================================================
// VoxelizeInvert
//====================================================================
void CVolumeNavRegion::VoxelizeInvert(SRegionData& regionData, const CVolume* pVolume, const Vec3i& ptIndex)
{
    SVoxelData& voxelData = regionData.voxelData;
    const Vec3i minIndex(1, 1, 1);
    const Vec3i maxIndex(voxelData.dim.x - 2, voxelData.dim.y - 2, voxelData.dim.z - 2);

    Vec3 delta(regionData.volumeRadius + voxelData.size, regionData.volumeRadius + voxelData.size, regionData.volumeRadius + voxelData.size);
    AABB aabb(pVolume->Center(m_pGraph->GetNodeManager()) - delta, pVolume->Center(m_pGraph->GetNodeManager()) + delta);

    for (size_t zIdx = minIndex.z; zIdx <= maxIndex.z; ++zIdx)
    {
        for (size_t yIdx = minIndex.y; yIdx <= maxIndex.y; ++yIdx)
        {
            for (size_t xIdx = minIndex.x; xIdx <= maxIndex.x; ++xIdx)
            {
                if (GetVoxelValue(voxelData, xIdx, yIdx, zIdx) != 0)
                {
                    Vec3 pos(voxelData.corner + Vec3(xIdx * voxelData.size, yIdx * voxelData.size, zIdx * voxelData.size));
                    if (aabb.IsContainPoint(pos))
                    {
                        if (IsPointInVolume(*pVolume, pos, 0.5f * voxelData.size))
                        {
                            SetVoxelValue(voxelData, xIdx, yIdx, zIdx, 0);
                        }
                    }
                }
            }
        }
    }
    // Make sure that at least the seed voxel gets zeroed, else we can get stuck
    SetVoxelValue(voxelData, ptIndex.x, ptIndex.y, ptIndex.z, 0);
}

//====================================================================
// CountHits
//====================================================================
int CountHits(IPhysicalWorld* pWorld, const Vec3& from, const Vec3& to, bool front)
{
    Vec3 begin = front ? from : to;
    Vec3 end = front ? to : from;
    static const int maxHits = 2;
    ray_hit hits[maxHits];
    Vec3 dir = (end - begin).GetNormalizedSafe();
    unsigned flags = rwi_ignore_noncolliding | rwi_ignore_back_faces;
    int count = 0;
    std::vector<IPhysicalEntity*> ignoreEnts;
    std::vector<float> ignoreEntDistances;
    while (count < 1000)
    {
        int rayresult = pWorld->RayWorldIntersection(begin, end - begin, ent_static, flags, &hits[0], maxHits,
                ignoreEnts.empty() ? 0 : &ignoreEnts[0], ignoreEnts.size());
        if (!rayresult)
        {
            return count;
        }
        ++count;
        if (rayresult == 2 && hits[1].dist > 0 && hits[1].dist < hits[0].dist)
        {
            hits[0] = hits[1];
        }
        int index = hits[0].dist < 0 ? 1 : 0;
        begin = hits[index].pt;// + dir * 0.01f;

        AIAssert(hits[index].dist >= 0.0f);
        // avoid hitting the same entity again until we've moved a significant distance
        bool foundEnt = false;
        for (unsigned i = 0; i < ignoreEnts.size(); )
        {
            ignoreEntDistances[i] += hits[index].dist;
            if (ignoreEntDistances[i] > 0.01f)
            {
                ignoreEntDistances.erase(ignoreEntDistances.begin() + i);
                ignoreEnts.erase(ignoreEnts.begin() + i);
            }
            else
            {
                if (ignoreEnts[i] == hits[index].pCollider)
                {
                    foundEnt = true;
                }
                ++i;
            }
        }
        if (!foundEnt)
        {
            ignoreEnts.push_back(hits[index].pCollider);
            ignoreEntDistances.push_back(0);
        }
    }
    return count;
}

//====================================================================
// CheckVoxel
//====================================================================
CVolumeNavRegion::ECheckVoxelResult CVolumeNavRegion::CheckVoxel(const Vec3& pos, const Vec3& definitelyValid) const
{
    static const int maxHits = 16;
    ray_hit hits[maxHits];

    // shoot rays around
    float tol = 0.01f;
    int rayresult = gEnv->pPhysicalWorld->RayWorldIntersection(pos, definitelyValid - pos, ent_static, rwi_ignore_noncolliding, &hits[0], maxHits);
    Vec3 dir = definitelyValid - pos;
    float dirLen = dir.GetLength();
    dir.NormalizeSafe();
    if (!rayresult)
    {
        return VOXEL_VALID;
    }
    int index = hits[0].dist < 0 ? 1 : 0;
    if (hits[index].dist <= dirLen && hits[index].n.Dot(dir) > tol)
    {
        return VOXEL_INVALID;
    }

    // hit a front-face first - need to count the hits
    int nFront = CountHits(gEnv->pPhysicalWorld, pos, definitelyValid, true);
    int nBack  = CountHits(gEnv->pPhysicalWorld, pos, definitelyValid, false);

    return (nFront == nBack) ? VOXEL_VALID : VOXEL_INVALID;
}

//====================================================================
// GenerateBasicVolumes
//====================================================================
void CVolumeNavRegion::GenerateBasicVolumes(SRegionData& regionData)
{
    AILogProgress("Finding basic volume locations");
    // make the volumes overlap a little bit
    float spacing = 1.8f * regionData.volumeRadius;

    // only make volumes when away from walls
    const int threshold = 1 + (int) (0.7f * regionData.volumeRadius / regionData.voxelData.size);

    Vec3 voxelSize = Vec3(regionData.voxelData.size, regionData.voxelData.size, regionData.voxelData.size);
    Vec3 min = regionData.aabb.min + voxelSize;
    Vec3 max = regionData.aabb.max - voxelSize;

    int nVolX = 1 + (int) ((max.x - min.x) / spacing);
    int nVolY = 1 + (int) ((max.y - min.y) / spacing);
    int nVolZ = 1 + (int) ((max.z - min.z) / spacing);

    float deltaX = (max.x - min.x) / nVolX;
    float deltaY = (max.y - min.y) / nVolY;
    float deltaZ = (max.z - min.z) / nVolZ;

    std::vector<Vec3i> volumeLocations;

    for (int i = 0; i <= nVolX; ++i)
    {
        for (int j = 0; j <= nVolY; ++j)
        {
            for (int k = 0; k <= nVolZ; ++k)
            {
                Vec3 positions[2] = {
                    min + Vec3(i * deltaX, j * deltaY, k * deltaZ),
                    min + Vec3(i * deltaX, j * deltaY, k * deltaZ) + 0.5f * Vec3(spacing, spacing, spacing)
                };
                for (unsigned iPos = 0; iPos < 2; ++iPos)
                {
                    Vec3 pos = positions[iPos];
                    // find the voxel
                    int voxelIx = (int) ((pos.x - regionData.aabb.min.x) / voxelSize.x);
                    int voxelIy = (int) ((pos.y - regionData.aabb.min.y) / voxelSize.y);
                    int voxelIz = (int) ((pos.z - regionData.aabb.min.z) / voxelSize.z);
                    if (voxelIx >= regionData.voxelData.dim.x || voxelIy >= regionData.voxelData.dim.y || voxelIz >= regionData.voxelData.dim.z)
                    {
                        continue;
                    }
                    if (GetVoxelValue(regionData.voxelData, voxelIx, voxelIy, voxelIz) > threshold)
                    {
                        volumeLocations.push_back(Vec3i(voxelIx, voxelIy, voxelIz));
                    }
                }
            }
        }
    }

    AILogProgress("Generating %lu basic volumes", volumeLocations.size());
    for (unsigned i = 0; i < volumeLocations.size(); ++i)
    {
        const Vec3i& ptIndex = volumeLocations[i];
        Vec3 point = regionData.voxelData.corner + Vec3(ptIndex.x * regionData.voxelData.size, ptIndex.y * regionData.voxelData.size, ptIndex.z * regionData.voxelData.size);
        CVolume* pVolume = CreateVolume(regionData.volumes, point, regionData.volumeRadius,
                GetDistanceVoxelValue(regionData.voxelData, ptIndex.x, ptIndex.y, ptIndex.z));
        VoxelizeInvert(regionData, pVolume, ptIndex);
    }
    CalculateVoxelDistances(regionData.voxelData, std::numeric_limits<TVoxelValue>::max());
    AILogProgress("Generated %lu basic volumes", volumeLocations.size());
}


//====================================================================
// CreateVolume
//====================================================================
CVolumeNavRegion::CVolume* CVolumeNavRegion::CreateVolume(TVolumes& volumes, const Vec3& pos, float radius, float distanceToGeometry)
{
    CVolume* pNewVolume = new CVolume();
    // add newly generated volume to the graph
    pNewVolume->m_graphNodeIndex = m_pGraph->CreateNewNode(IAISystem::NAV_VOLUME, pos);
    pNewVolume->m_radius = radius;
    pNewVolume->m_distanceToGeometry = distanceToGeometry;

    // add newly generated volume to the overall volumes list
    volumes.push_back(pNewVolume);

    // this index will need to be adjusted when all volumes are ready
    m_pGraph->GetNodeManager().GetNode(pNewVolume->m_graphNodeIndex)->GetVolumeNavData()->nVolimeIdx = volumes.size() - 1;

    if (radius > m_maxVolumeRadius)
    {
        m_maxVolumeRadius = radius;
    }
    return pNewVolume;
}

//====================================================================
// GeneratePortals
//====================================================================
void CVolumeNavRegion::GeneratePortals(SRegionData& regionData)
{
    typedef std::pair<CVolumeNavRegion::CVolume*, CVolumeNavRegion::CVolume*> TVolumePair;
    typedef std::vector<TVolumePair> TCloseVolumes;
    TCloseVolumes candidates;

    static float extraFactor = 1.0f;
    float rSumSq = square(volumeVoxelSize + extraFactor * 2.0f * regionData.volumeRadius);
    for (TVolumes::iterator itrSpace = regionData.volumes.begin(); itrSpace != regionData.volumes.end(); ++itrSpace)
    {
        CVolume& curVolume = *(*itrSpace);
        TVolumes::iterator itrOtherSpace = itrSpace;

        for (++itrOtherSpace; itrOtherSpace != regionData.volumes.end(); ++itrOtherSpace)
        {
            CVolume& otherVolume = *(*itrOtherSpace);
            float distSq = (curVolume.Center(m_pGraph->GetNodeManager()) - otherVolume.Center(m_pGraph->GetNodeManager())).GetLengthSquared();
            if (distSq > rSumSq)
            {
                continue;
            }
            candidates.push_back(std::make_pair(&curVolume, &otherVolume));
        }
    }

    if (candidates.empty())
    {
        AILogProgress("[AISystem:Connecting volumes] There are no candidates");
        return;
    }

    AILogProgress("[%s] %lu pairs to process.", __FUNCTION__, candidates.size());

    // for logging progress
    float completion = 0.0f;
    float completionStep = 100.0f / static_cast<float>(candidates.size());
    float lastPC = -10.0f;
    for (TCloseVolumes::iterator itr = candidates.begin(); itr != candidates.end(); ++itr)
    {
        TVolumePair& curPair = *itr;
        GeneratePortalVxl(regionData.portals, *curPair.first, *curPair.second);
        completion += completionStep;
        if (completion - lastPC >= 2.0f)
        {
            AILogProgress("[AISystem:Connecting volumes] done %.1f%%%%", completion);
            lastPC = completion;
        }
    }
}

//====================================================================
// GeneratePortalVxl
//====================================================================
void CVolumeNavRegion::GeneratePortalVxl(TPortals& portals, CVolume& firstVolume, CVolume& secondVolume)
{
    // if there is line-of-sight and can fit the max pass radius, just use that.
    const std::vector<float>& radii = m_pNavigation->Get3DPassRadii();
    if (radii.empty())
    {
        return;
    }

    float passRadius = radii[0] + 0.04f;
    bool intersect = PathSegmentWorldIntersection(firstVolume.Center(m_pGraph->GetNodeManager()), secondVolume.Center(m_pGraph->GetNodeManager()), passRadius, false, false, AICE_ALL);
    if (!intersect)
    {
        CPortal portal(&firstVolume, &secondVolume);
        portal.m_passPoint = 0.5f * (firstVolume.Center(m_pGraph->GetNodeManager()) + secondVolume.Center(m_pGraph->GetNodeManager()));
        portal.m_passRadius = passRadius;
        m_pGraph->Connect(firstVolume.m_graphNodeIndex, secondVolume.m_graphNodeIndex, portal.m_passRadius, portal.m_passRadius);
        portals.push_back(portal);
        int ind = GetPortalIdx(portals, &portals.back());
        AIAssert(ind >= 0);
        firstVolume.m_portalIndices.push_back(ind);
        secondVolume.m_portalIndices.push_back(ind);
        return;
    }

    /// no direct line, so voxelise etc
    static float voxelSize = 1.0f;
    float volRadius = max(firstVolume.m_radius, secondVolume.m_radius);
    Vec3 delta(volRadius, volRadius, volRadius);

    AABB aabb;

    float dist = Distance::Point_Point(firstVolume.Center(m_pGraph->GetNodeManager()), secondVolume.Center(m_pGraph->GetNodeManager()));
    Vec3 avgPoint = 0.5f * (firstVolume.Center(m_pGraph->GetNodeManager()) + secondVolume.Center(m_pGraph->GetNodeManager()));
    float radius = min(0.5f * dist, volRadius);
    float radiusSq = square(radius);
    aabb.min = avgPoint - 0.5f * delta;
    aabb.max = avgPoint + 0.5f * delta;

    Vec3 size = aabb.GetSize();
    Vec3 aabbCentre = aabb.GetCenter();

    size_t voxelDimX = 1 + static_cast<size_t>((size.x / voxelSize));
    size_t voxelDimY = 1 + static_cast<size_t>((size.y / voxelSize));
    size_t voxelDimZ = 1 + static_cast<size_t>((size.z / voxelSize));

    TVectorInt voxelSpace;
    voxelSpace.reserve(voxelDimX * voxelDimY * voxelDimZ);

    // fill all the open space with voxels
    for (size_t zIdx = 0; zIdx < voxelDimZ; ++zIdx)
    {
        for (size_t yIdx = 0; yIdx < voxelDimY; ++yIdx)
        {
            for (size_t xIdx = 0; xIdx < voxelDimX; ++xIdx)
            {
                int voxValue = 0;
                Vec3 curPos = aabb.min + Vec3(xIdx * voxelSize, yIdx * voxelSize, zIdx * voxelSize);
                float distToMidSq = (curPos - aabbCentre).GetLengthSquared();
                if (distToMidSq < radiusSq &&
                    IsPointInVolume(firstVolume, curPos, volRadius) &&
                    IsPointInVolume(secondVolume, curPos, volRadius))
                {
                    voxValue = 1;
                }
                voxelSpace.push_back(voxValue);
            }
        }
    }

    // now need do determine groups
    int curGroupId = 1;
    for (size_t zIdx = 0; zIdx < voxelDimZ; ++zIdx)
    {
        for (size_t yIdx = 0; yIdx < voxelDimY; ++yIdx)
        {
            for (size_t xIdx = 0; xIdx < voxelDimX; ++xIdx)
            {
                size_t curIdx = zIdx * voxelDimY * voxelDimX + yIdx * voxelDimX + xIdx;
                if (voxelSpace[curIdx] != 1)
                {
                    continue;
                }
                ++curGroupId;
                GroupVoxels(voxelSpace, curGroupId, xIdx, yIdx, zIdx, voxelDimX, voxelDimY, voxelDimZ);
            }
        }
    }

    int dbg_counterTotal = 0;
    //  AILogProgress("[%s] found %d groups.", __FUNCTION__, curGroupId-1);
    // now each group represents overlapping part of volume - let's generate portals there
    while (curGroupId > 1)
    {
        // find average point of group
        Vec3 avgGroupPoint(0, 0, 0);
        int counter = 0;
        for (size_t zIdx = 0; zIdx < voxelDimZ; ++zIdx)
        {
            for (size_t yIdx = 0; yIdx < voxelDimY; ++yIdx)
            {
                for (size_t xIdx = 0; xIdx < voxelDimX; ++xIdx)
                {
                    size_t curIdx = zIdx * voxelDimY * voxelDimX + yIdx * voxelDimX + xIdx;
                    if (voxelSpace[curIdx] != curGroupId)
                    {
                        continue;
                    }
                    ++counter;
                    Vec3 curPos = aabb.min + Vec3(xIdx * voxelSize, yIdx * voxelSize, zIdx * voxelSize);
                    avgGroupPoint += curPos;
                }
            }
        }
        // advance group
        --curGroupId;
        dbg_counterTotal += counter;
        if (counter < 3)
        {
            continue;
        }

        avgGroupPoint /= (float)counter;

        // create portal here
        CPortal portal(&firstVolume, &secondVolume);
        portal.m_passPoint = avgGroupPoint;
        portal.m_passRadius = CalculatePassRadius(portal, radii);
        if (portal.m_passRadius > 0.0f)
        {
            m_pGraph->Connect(firstVolume.m_graphNodeIndex, secondVolume.m_graphNodeIndex, portal.m_passRadius, portal.m_passRadius);
            portals.push_back(portal);
            int ind = GetPortalIdx(portals, &portals.back());
            AIAssert(ind >= 0);
            firstVolume.m_portalIndices.push_back(ind);
            secondVolume.m_portalIndices.push_back(ind);
        }
    }
}

//====================================================================
// IsConnecting
//====================================================================
bool CVolumeNavRegion::CPortal::IsConnecting(const CVolume* pOne, const CVolume* pTwo) const
{
    if (m_pVolumeOne == pOne && m_pVolumeTwo == pTwo)
    {
        return true;
    }
    if (m_pVolumeOne == pTwo && m_pVolumeTwo == pOne)
    {
        return true;
    }
    return false;
}

//====================================================================
// GetConnectionPoint
//====================================================================
Vec3 CVolumeNavRegion::CVolume::GetConnectionPoint(const CVolumeNavRegion* pVolumeNavRegion, const CVolume* otherVolume) const
{
    for (TPortalIndices::const_iterator itrPortal = m_portalIndices.begin(); itrPortal != m_portalIndices.end(); ++itrPortal)
    {
        const CPortal& portal = *pVolumeNavRegion->GetPortal(pVolumeNavRegion->m_portals, *itrPortal);
        if (portal.IsConnecting(this, otherVolume))
        {
            return portal.m_passPoint;
        }
    }
    AIWarning("[AISystem] CVolume::GetConnectionPoint - not connected");
    return Center(pVolumeNavRegion->m_pGraph->GetNodeManager());
}

//====================================================================
// SavePortal
//====================================================================
bool CVolumeNavRegion::SavePortal(TVolumes& volumes, TPortals& portals, CPortal& portal, CCryFile& cryFile)
{
    int volumeIdx1 = GetVolumeIdx(volumes, portal.m_pVolumeOne);
    cryFile.Write(&volumeIdx1, sizeof(volumeIdx1));
    int volumeIdx2 = GetVolumeIdx(volumes, portal.m_pVolumeTwo);
    cryFile.Write(&volumeIdx2, sizeof(volumeIdx2));

    cryFile.Write(&portal.m_passPoint, sizeof(portal.m_passPoint));
    cryFile.Write(&portal.m_passRadius, sizeof(portal.m_passRadius));
    return true;
}


//====================================================================
// LoadPortal
//====================================================================
bool CVolumeNavRegion::LoadPortal(TVolumes& volumes, TPortals& portals, CCryFile& cryFile)
{
    portals.push_back(CPortal(0, 0));
    CPortal& portal = portals.back();

    int volumeIdx1;
    cryFile.ReadType(&volumeIdx1);
    portal.m_pVolumeOne = GetVolume(volumes, volumeIdx1);
    int volumeIdx2;
    cryFile.ReadType(&volumeIdx2);
    portal.m_pVolumeTwo = GetVolume(volumes, volumeIdx2);

    if (!portal.m_pVolumeOne || !portal.m_pVolumeTwo)
    {
        AIWarning("Error loading portal");
        portals.pop_back();
        return false;
    }

    cryFile.ReadType(&portal.m_passPoint);
    cryFile.ReadType(&portal.m_passRadius);

    m_pGraph->Connect(portal.m_pVolumeOne->m_graphNodeIndex, portal.m_pVolumeTwo->m_graphNodeIndex, portal.m_passRadius, portal.m_passRadius);
    return true;
}

//====================================================================
// SaveVolume
//====================================================================
bool CVolumeNavRegion::SaveVolume(TVolumes& volumes, CVolume& volume, CCryFile& cryFile)
{
    Vec3 center = volume.Center(m_pGraph->GetNodeManager());
    cryFile.Write(&center, sizeof(center));
    cryFile.Write(&volume.m_radius, sizeof(volume.m_radius));
    cryFile.Write(&volume.m_distanceToGeometry, sizeof(volume.m_distanceToGeometry));

    // save portals
    uint32 counter = volume.m_portalIndices.size();
    cryFile.Write(&counter, sizeof(counter));
    if (counter > 0)
    {
        cryFile.Write(&volume.m_portalIndices[0], counter * sizeof(volume.m_portalIndices[0]));
    }
    return true;
}

//====================================================================
// LoadVolume
//====================================================================
bool CVolumeNavRegion::LoadVolume(TVolumes& volumes, CCryFile& cryFile)
{
    Vec3 center(ZERO);
    cryFile.ReadType(&center);
    float radius = 0.0f;
    cryFile.ReadType(&radius);
    float distanceToGeometry = 0.0f;
    cryFile.ReadType(&distanceToGeometry);

    CVolume* pVolume = CreateVolume(volumes, center, radius, distanceToGeometry);

    // read portals
    uint32 counter = 0;
    cryFile.ReadType(&counter);
    pVolume->m_portalIndices.resize(counter);
    if (counter > 0)
    {
        cryFile.ReadType(&pVolume->m_portalIndices[0], counter);
    }

    return true;
}

//====================================================================
// LoadHideSpots
//====================================================================
bool CVolumeNavRegion::LoadHideSpots(std::vector<SVolumeHideSpot>& hideSpots, CCryFile& cryFile)
{
    uint32 nHideSpots = 0;
    cryFile.ReadType(&nHideSpots);
    hideSpots.resize(nHideSpots);
    if (nHideSpots > 0)
    {
        cryFile.ReadType(&hideSpots[0], nHideSpots);
    }
    return true;
}

//====================================================================
// SaveHideSpots
//====================================================================
bool CVolumeNavRegion::SaveHideSpots(const std::vector<SVolumeHideSpot>& hideSpots, CCryFile& cryFile)
{
    uint32 nHideSpots = hideSpots.size();
    cryFile.Write(&nHideSpots, sizeof(nHideSpots));
    if (nHideSpots > 0)
    {
        cryFile.Write(&hideSpots[0], nHideSpots * sizeof(hideSpots[0]));
    }
    return true;
}

//====================================================================
// LoadNavigation
//====================================================================
bool CVolumeNavRegion::LoadNavigation(TVolumes& volumes, TPortals& portals, CCryFile& cryFile)
{
    int nFileVersion = BAI_3D_FILE_VERSION;
    cryFile.ReadType(&nFileVersion);

    if (nFileVersion != BAI_3D_FILE_VERSION)
    {
        AIWarning("Wrong BAI file version (found %d expected %d)!! Regenerate 3d navigation volumes in the editor.",
            nFileVersion, BAI_3D_FILE_VERSION);
        return false;
    }

    // load volumes
    uint32 volumeCount = 0;
    cryFile.ReadType(&volumeCount);
    volumes.reserve(volumeCount);
    for (uint32 iVolume = 0; iVolume < volumeCount; ++iVolume)
    {
        if (!LoadVolume(volumes, cryFile))
        {
            AIWarning("Error loading volume navigation");
            return false;
        }
    }

    // load portals
    uint32 portalCount = 0;
    cryFile.ReadType(&portalCount);
    portals.reserve(portalCount);
    for (uint32 iPortal = 0; iPortal < portalCount; ++iPortal)
    {
        if (!LoadPortal(volumes, portals, cryFile))
        {
            AIWarning("Error loading volume (portals) navigation");
            return false;
        }
    }

    for (uint32 iVolume = 0; iVolume < volumeCount; ++iVolume)
    {
        const CVolume& volume = *volumes[iVolume];
        for (uint32 i = 0; i < volume.m_portalIndices.size(); ++i)
        {
            AIAssert(volume.m_portalIndices[i] < portals.size());
        }
    }

    return true;
}

//====================================================================
// SaveNavigation
//====================================================================
bool CVolumeNavRegion::SaveNavigation(TVolumes& volumes, TPortals& portals, CCryFile& cryFile)
{
    for (uint32 iVolume = 0; iVolume < volumes.size(); ++iVolume)
    {
        const CVolume& volume = *volumes[iVolume];
        for (uint32 i = 0; i < volume.m_portalIndices.size(); ++i)
        {
            AIAssert(volume.m_portalIndices[i] < portals.size());
        }
    }

    int nFileVersion = BAI_3D_FILE_VERSION;
    cryFile.Write(&nFileVersion, sizeof(nFileVersion));

    uint32 volumeCount = volumes.size();
    cryFile.Write(&volumeCount, sizeof(volumeCount));
    for (TVolumes::const_iterator itrCurVolume = volumes.begin(); itrCurVolume != volumes.end(); ++itrCurVolume)
    {
        SaveVolume(volumes, **itrCurVolume, cryFile);
    }

    uint32 portalCount = portals.size();
    cryFile.Write(&portalCount, sizeof(portalCount));
    for (TPortals::iterator itrPortal = portals.begin(); itrPortal != portals.end(); ++itrPortal)
    {
        SavePortal(volumes, portals, *itrPortal, cryFile);
    }

    return true;
}


//====================================================================
// LoadRegion
//====================================================================
void CVolumeNavRegion::LoadRegion(const char* szLevel, const char* szMission, SRegionData& regionData)
{
    char fileName[1024];
    sprintf_s(fileName, "%s/v3d%s-region-%s.bai", szLevel, szMission, regionData.name.c_str());

    CCryFile file;
    if (!file.Open(fileName, "rb"))
    {
        AIWarning("Failed to open %s - this region will not be calculated/loaded", fileName);
        return;
    }

    file.ReadType(&regionData.volumeRadius);
    if (!LoadNavigation(regionData.volumes, regionData.portals, file))
    {
        AIWarning("Failed to load region data from %s", fileName);
    }

    if (!LoadHideSpots(regionData.hideSpots, file))
    {
        AIWarning("Failed to load region hidespots from %s", fileName);
    }
}

//====================================================================
// SaveRegion
//====================================================================
void CVolumeNavRegion::SaveRegion(const char* szLevel, const char* szMission, SRegionData& regionData)
{
    char fileName[1024];
    sprintf_s(fileName, "%s/v3d%s-region-%s.bai", szLevel, szMission, regionData.name.c_str());

    CCryFile file;
    if (!file.Open(fileName, "wb"))
    {
        AIWarning("Failed to open %s - this region will not be saved", fileName);
        return;
    }

    file.Write(&regionData.volumeRadius, sizeof(regionData.volumeRadius));
    if (!SaveNavigation(regionData.volumes, regionData.portals, file))
    {
        AIWarning("Failed to save region data to %s", fileName);
    }

    if (!SaveHideSpots(regionData.hideSpots, file))
    {
        AIWarning("Failed to save hidespots to %s", fileName);
    }
}


//====================================================================
// ReadFromFile
//====================================================================
bool CVolumeNavRegion::ReadFromFile(const char* pName)
{
    float fStartTime = gEnv->pTimer->GetAsyncCurTime();
    AILogLoading("Reading AI volumes [%s]", pName);

    CCryFile file;
    ;
    if (!file.Open(pName, "rb"))
    {
        AIWarning("could not read AI volume. [%s]", pName);
        return false;
    }

    int nFileVersion = BAI_3D_FILE_VERSION;
    file.ReadType(&nFileVersion);

    if (nFileVersion != BAI_3D_FILE_VERSION)
    {
        AIWarning("Wrong BAI file version (found %d expected %d)!! Regenerate 3d navigation volumes in the editor.",
            nFileVersion, BAI_3D_FILE_VERSION);
        return false;
    }

    file.ReadType(&m_AABB);

    if (!LoadNavigation(m_volumes, m_portals, file))
    {
        AIWarning("Failed to load volume navigation data");
        Clear();
        return false;
    }

    // [Mikko] 10.9.07 - The volume navigation hide points are not going to be used
    // in the production level, so I commented this out for now. The reason is that
    // the hash space serialization changed.

    // read hide spots
    //  m_hideSpots.ReadFromFile(file);

    AILogLoading("Read %d hidespots from file", m_hideSpots.GetNumObjects());

    AILogLoading("AI volumes loaded  successfully in %6.3f sec. [%s]",
        gEnv->pTimer->GetAsyncCurTime() - fStartTime, pName);
    AILogLoading("AI num volumes = %lu, num portals = %lu",
        m_volumes.size(), m_portals.size());
    return true;
}

//====================================================================
// WriteToFile
//====================================================================
bool CVolumeNavRegion::WriteToFile(const char* pName)
{
    CCryFile file;
    if (!file.Open(pName, "wb"))
    {
        AIWarning("could not save AI volume. [%s]", pName);
        return false;
    }
    int nFileVersion = BAI_3D_FILE_VERSION;
    file.Write(&nFileVersion, sizeof(nFileVersion));

    file.Write(&m_AABB.min, sizeof(m_AABB));

    if (!SaveNavigation(m_volumes, m_portals, file))
    {
        AIWarning("Failed to save volume navigation");
        return false;
    }

    // save hide spots
    AILogProgress("Saving %d hidespots", m_hideSpots.GetNumObjects());
    m_hideSpots.WriteToFile(file);

    AILogProgress("AI volumes saved successfully. [%s]", pName);
    return true;
}

//====================================================================
// Serialize
//====================================================================
void CVolumeNavRegion::Serialize(TSerialize ser)
{
    ser.BeginGroup("VolumeNavRegion");

    ser.EndGroup();
}

//====================================================================
// CalculateHideSpots
// just recalculates within one region
//====================================================================
void CVolumeNavRegion::CalculateHideSpots()
{
    m_hideSpots.Clear(true);

    // todo Danny/Mikko return early since there are some issues with calculating
    // hidespots, and anyway they're not used.
    //  return;

    SRegionData regionData;
    regionData.aabb = m_AABB;
    regionData.name = string("All navigation");
    regionData.sa = 0;

    std::vector<SVolumeHideSpot> hideSpots;
    CalculateHideSpots(regionData, hideSpots);

    unsigned nHideSpots = hideSpots.size();
    for (unsigned i = 0; i < nHideSpots; ++i)
    {
        m_hideSpots.AddObject(hideSpots[i]);
    }
}

inline bool CheckTriangleArea(const Vec3& p0, const Vec3& p1, const Vec3& p2)
{
    Vec3 triDir = (p1 - p0).Cross(p2 - p0);
    float len = triDir.GetLengthSquared();
    if (len < 0.0001f)
    {
        return false;
    }
    return true;
}

void SubdivideTriangle(std::deque<Triangle>& triangles, const Vec3& p0, const Vec3& p1, const Vec3& p2, float tesSize)
{
    // The triangle is tesselated by first finding the longest edge and the subidiving that.
    // The same amount of points are distributed on the two other edges and vertical stripes
    // are formed.
    //
    // Finally the vertical stripes are subdivided too and triangles are formed between the two
    // neighbor stripes. Each triangle pair is further optimized to create as uniform triangles
    // as possible.
    //
    //   top
    //       o.
    //     .'| '|..
    //   .|  |  |  |'.
    //  o.|..|..|..|..|:o
    //   bottom
    //
    //       o.
    //     .'| '|..
    //   .|--|--|--|'.
    //  o.|..|..|..|..|:o

    // Calculate the triangle edges and find the longest edge.
    const Vec3    points[3] = { p0, p1, p2 };
    int       longest = 0;
    float longestLen = 0;
    Vec3  edge[3];
    float edgeLen[3];

    for (int i = 0; i < 3; i++)
    {
        edge[i] = points[(i + 1) % 3] - points[i];
        edgeLen[i] = edge[i].len();
        if (edgeLen[i] > longestLen)
        {
            longestLen = edgeLen[i];
            longest = i;
        }
    }

    const Vec3&   edgeA = edge[(longest + 1) % 3];
    const Vec3&   edgeB = edge[(longest + 2) % 3];

    // Project the point opposite the longest edge on the longest edge.
    Vec3  dir = edge[longest].normalized();
    float dot = dir.Dot(-edgeA);
    Vec3  proj = points[longest] + dir * dot;

    std::vector<Vec3> top;
    std::vector<Vec3> bottom;
    std::vector<int>  pointCount;

    int   nPtLongest = (int)floor(edgeLen[longest] / tesSize) + 2;

    top.resize(nPtLongest);
    bottom.resize(nPtLongest);
    pointCount.resize(nPtLongest);

    // Calculate the bottom set of points.
    for (int i = 0; i < nPtLongest; i++)
    {
        float   t = (float)i / (float)(nPtLongest - 1);
        bottom[i] = points[longest] + edge[longest] * (1 - t);
    }

    float edgeALen = edgeA.len();
    float edgeBLen = edgeB.len();

    int   nExtra = max(0, (nPtLongest - 3));
    int   nPtEdgeA = max(0, (int)floor((dot / edgeLen[longest]) * nExtra + 0.5f));
    int   nPtEdgeB = nExtra - nPtEdgeA;
    nPtEdgeA += 1;
    nPtEdgeB += 2;

    // Calculate the top set of points.
    for (int i = 0; i < nPtEdgeA; i++)
    {
        float   t = (float)i / (float)nPtEdgeA;
        top[i] = points[(longest + 1) % 3] + edgeA * t;
    }

    for (int i = 0; i < nPtEdgeB; i++)
    {
        float   t = (float)i / (float)(nPtEdgeB - 1);
        top[i + nPtEdgeA] = points[(longest + 2) % 3] + edgeB * t;
    }

    for (int i = 0; i < nPtLongest; i++)
    {
        Vec3    seg = top[i] - bottom[i];
        pointCount[i] = (int)floor(seg.len() / tesSize) + 2;
    }

    int   triCount = 0;

    // Calculate the triangles.
    for (int i = 0; i < nPtLongest - 1; i++)
    {
        // 'a' and 'b' are clamped counters. That is either of them can reach
        // the target too early, but clamped last point is used as the pair until
        // all the iterations are done.
        int a = 0;
        int b = 0;
        int m = max(pointCount[i], pointCount[i + 1]);

        Vec3    seg0 = top[i] - bottom[i];
        Vec3    seg1 = top[i + 1] - bottom[i + 1];

        for (int j = 0; j < m - 1; j++)
        {
            // Calculate the four points required to create he two or one triangle.
            const float   t0 = (float)a / (float)(pointCount[i] - 1);
            const float   t1 = (float)b / (float)(pointCount[i + 1] - 1);
            const float   t2 = (float)(b + 1) / (float)(pointCount[i + 1] - 1);
            const float   t3 = (float)(a + 1) / (float)(pointCount[i] - 1);

            const Vec3    pt0 = bottom[i] + seg0 * t0;
            const Vec3    pt1 = bottom[i + 1] + seg1 * t1;
            const Vec3    pt2 = bottom[i + 1] + seg1 * t2;
            const Vec3    pt3 = bottom[i] + seg0 * t3;

            // Build the triangles so that
            float lenDiagA = (pt0 - pt2).len2();
            float lenDiagB = (pt1 - pt3).len2();
            if (lenDiagA > 0 && lenDiagA < lenDiagB)
            {
                // Handle the case that sometimes there needs to be only one triangle.
                if (b + 1 < pointCount[i + 1])
                {
                    if (CheckTriangleArea(pt2, pt1, pt0))
                    {
                        triangles.push_back(Triangle(pt2, pt1, pt0));
                    }
                    triCount++;
                    if (a + 1 < pointCount[i])
                    {
                        if (CheckTriangleArea(pt3, pt2, pt0))
                        {
                            triangles.push_back(Triangle(pt3, pt2, pt0));
                        }
                        triCount++;
                    }
                }
                else if (a + 1 < pointCount[i])
                {
                    if (CheckTriangleArea(pt3, pt1, pt0))
                    {
                        triangles.push_back(Triangle(pt3, pt1, pt0));
                    }
                    triCount++;
                }
            }
            else
            {
                // Handle the case that sometimes there needs to be only one triangle.
                if (a + 1 < pointCount[i])
                {
                    if (CheckTriangleArea(pt3, pt1, pt0))
                    {
                        triangles.push_back(Triangle(pt3, pt1, pt0));
                    }
                    triCount++;
                    if (b + 1 < pointCount[i + 1])
                    {
                        if (CheckTriangleArea(pt3, pt2, pt1))
                        {
                            triangles.push_back(Triangle(pt3, pt2, pt1));
                        }
                        triCount++;
                    }
                }
                else if (b + 1 < pointCount[i + 1])
                {
                    if (CheckTriangleArea(pt2, pt1, pt0))
                    {
                        triangles.push_back(Triangle(pt2, pt1, pt0));
                    }
                    triCount++;
                }
            }

            // Increment and clamp the point iterators.
            a++;
            b++;
            if (a >= pointCount[i])
            {
                a = pointCount[i] - 1;
            }
            if (b >= pointCount[i + 1])
            {
                b = pointCount[i + 1] - 1;
            }
        }
    }
}

//====================================================================
// CalculateHideSpots
//====================================================================
void CVolumeNavRegion::CalculateHideSpots(SRegionData& regionData, std::vector<SVolumeHideSpot>& hideSpots)
{
    AILogProgress("Obtaining the triangles for hidespots...");

    AILogProgress("voxels = %lu\n", regionData.voxelData.voxels.size());

    /*  for (TVolumes::iterator it = regionData.volumes.begin(), end = regionData.volumes.end(); it != end; ++it)
        {
        }*/

    /*
      IPhysicalEntity ** ppList = 0;
      int   numEntites = gEnv->pPhysicalWorld->GetEntitiesInBox(regionData.aabb.min, regionData.aabb.max,
        ppList, ent_static | ent_ignore_noncolliding);

      std::deque<Triangle> triangles;

      for( int i = 0; i < numEntites; ++i )
      {
        pe_status_pos status;
        status.ipart = 0;
        ppList[i]->GetStatus( &status );

        Vec3 pos = status.pos;
        Matrix33 mat(status.q);
        mat *= status.scale;

        IGeometry *geom = status.pGeom;
        int type = geom->GetType();
        switch (type)
        {
        case GEOM_BOX:
          {
            const primitives::primitive * prim = geom->GetData();
            const primitives::box* bbox = static_cast<const primitives::box*>(prim);
            Matrix33 rot = bbox->Basis.T();

            Vec3    boxVerts[8];
            int     sides[6 * 4] = {
              3, 2, 1, 0,
                0, 1, 5, 4,
                1, 2, 6, 5,
                2, 3, 7, 6,
                3, 0, 4, 7,
                4, 5, 6, 7,
            };

            boxVerts[0] = bbox->center + rot * Vec3( -bbox->size.x, -bbox->size.y, -bbox->size.z );
            boxVerts[1] = bbox->center + rot * Vec3( bbox->size.x, -bbox->size.y, -bbox->size.z );
            boxVerts[2] = bbox->center + rot * Vec3( bbox->size.x, bbox->size.y, -bbox->size.z );
            boxVerts[3] = bbox->center + rot * Vec3( -bbox->size.x, bbox->size.y, -bbox->size.z );
            boxVerts[4] = bbox->center + rot * Vec3( -bbox->size.x, -bbox->size.y, bbox->size.z );
            boxVerts[5] = bbox->center + rot * Vec3( bbox->size.x, -bbox->size.y, bbox->size.z );
            boxVerts[6] = bbox->center + rot * Vec3( bbox->size.x, bbox->size.y, bbox->size.z );
            boxVerts[7] = bbox->center + rot * Vec3( -bbox->size.x, bbox->size.y, bbox->size.z );

            for( int j = 0; j < 8; j++ )
              boxVerts[j] = pos + mat * boxVerts[j];

            for( int j = 0; j < 6; j++ )
            {
              // subdivide the side if necessary.
              Vec3  edgeU = boxVerts[sides[j*4 + 1]] - boxVerts[sides[j*4 + 0]];
              Vec3  edgeV = boxVerts[sides[j*4 + 2]] - boxVerts[sides[j*4 + 1]];
              const Vec3&   base = boxVerts[sides[j*4 + 0]];

              float lenU = edgeU.GetLength();
              float lenV = edgeV.GetLength();

              const float   quadSize = 1.5f;
              int       subdivU = (int)floorf( lenU / quadSize );
              int       subdivV = (int)floorf( lenV / quadSize );
              if( subdivU < 1 ) subdivU = 1;
              if( subdivV < 1 ) subdivV = 1;

              float deltaU = 1.0f / (float)subdivU;
              float deltaV = 1.0f / (float)subdivV;

              for( int v = 0; v < subdivV; v++ )
              {
                for( int u = 0; u < subdivU; u++ )
                {
                  float ut = (float)u * deltaU;
                  float vt = (float)v * deltaV;

                  // Create two triangle per each quad.
                  // 3 +-----+ 2
                  //   |   / |
                  //   | /   |
                  // 0 x-----x 1
                  const Vec3    v0 = base + edgeU * ut + edgeV * vt;
                  const Vec3    v1 = v0 + edgeU * deltaU;
                  const Vec3    v2 = v0 + edgeU * deltaU + edgeV * deltaV;
                  const Vec3    v3 = v0 + edgeV * deltaV;

                  if (!regionData.aabb.IsContainPoint(v0))
                    continue;
                  if (!regionData.aabb.IsContainPoint(v1))
                    continue;
                  if (!regionData.aabb.IsContainPoint(v2))
                    continue;
                  if (!regionData.aabb.IsContainPoint(v3))
                    continue;
                  triangles.push_back(Triangle(v0, v1, v2));
                  triangles.push_back(Triangle(v0, v2, v3));
                }
              }
            }

          }
          break;

        case GEOM_TRIMESH:
        case GEOM_VOXELGRID:
          {
            const primitives::primitive * prim = geom->GetData();
            const mesh_data * mesh = static_cast<const mesh_data *>(prim);

            int numVerts = mesh->nVertices;
            int numTris = mesh->nTris;

            for (int j = 0; j < numTris; ++j)
            {
              const Vec3 v0 = pos + mat * mesh->pVertices[mesh->pIndices[j*3 + 0]];
              if (!regionData.aabb.IsContainPoint(v0))
                continue;
              const Vec3 v1 = pos + mat * mesh->pVertices[mesh->pIndices[j*3 + 1]];
              if (!regionData.aabb.IsContainPoint(v1))
                continue;
              const Vec3 v2 = pos + mat * mesh->pVertices[mesh->pIndices[j*3 + 2]];
              if (!regionData.aabb.IsContainPoint(v2))
                continue;

              const float minEdgeLen = 0.25f * 0.25f;
              Vec3  e0 = v1 - v0;
              Vec3  e1 = v2 - v1;
              Vec3  e2 = v0 - v2;
              const float e0LenSqr = e0.GetLengthSquared();
              const float e1LenSqr = e1.GetLengthSquared();
              const float e2LenSqr = e2.GetLengthSquared();
              if( e0LenSqr < minEdgeLen && e1LenSqr < minEdgeLen && e2LenSqr < minEdgeLen )
                continue;

              const float subDivSize = 2.0f;
              const float subDivSizeSqr = subDivSize * subDivSize;

              if( e0LenSqr < subDivSizeSqr && e1LenSqr < subDivSizeSqr && e2LenSqr < subDivSizeSqr )
                triangles.push_back(Triangle(v0, v1, v2));
              else
                SubdivideTriangle( triangles, v0, v1, v2, subDivSize );
            }
            break;
          }
        default:
          break;
        }
      }

      unsigned nTriangles = triangles.size();
      AILogProgress("Finished obtaining %d triangles...", nTriangles);

      float hideTriDist = 2.0f;

      float lastPC = 0.0f;
      for (unsigned iTriangle = 0 ; iTriangle < nTriangles ; iTriangle += 2)
      {
        float pc = (100.0f * iTriangle) / nTriangles;
        if (pc > lastPC + 2.0f)
        {
          AILogProgress("Completed %5.2f%%%% of hidespots", pc);
          lastPC = pc;
        }
        const Triangle& tri = triangles[iTriangle];
        Vec3 midPt = (tri.v0 + tri.v1 + tri.v2 ) / 3.0f;
        Vec3 triDir = (tri.v1 - tri.v0).Cross(tri.v2 - tri.v0);
        triDir.NormalizeSafe();
        SVolumeHideSpot hideSpot(midPt + hideTriDist * triDir, -triDir);
        if (ISVolumeHideSpotGood(hideSpot, midPt))
          hideSpots.push_back(hideSpot);
      }
      AILogProgress("Completed 100%%%% of hidespots");*/
}

//====================================================================
// SVolumeHideSpotDrawer
//====================================================================
struct SVolumeHideSpotDrawer
{
    SVolumeHideSpotDrawer(IRenderer* pRenderer)
        : pRenderer(pRenderer){}
    void operator()(const SVolumeHideSpot& hs, float)
    {
        static ColorF col(0, 1, 0, 1);
        static float size = 1.0f;
        pRenderer->GetIRenderAuxGeom()->DrawCone(hs.pos + size * hs.dir, -hs.dir, 0.5f * size, size, col);
    }
private:
    IRenderer* pRenderer;
};

//====================================================================
// GetNodeFromIndex
//====================================================================
unsigned CVolumeNavRegion::GetNodeFromIndex(int index)
{
    if (index < 0 || index > (int) m_volumes.size())
    {
        return 0;
    }
    else
    {
        return m_volumes[index]->m_graphNodeIndex;
    }
}

//====================================================================
// GetDistanceToGeometry
// Returns estimated distance to geometry.
//====================================================================
float   CVolumeNavRegion::GetDistanceToGeometry(size_t volumeIdx) const
{
    if (volumeIdx < 0 || static_cast<size_t>(volumeIdx) >= m_volumes.size())
    {
        return 0;
    }
    return m_volumes[volumeIdx]->m_distanceToGeometry;
}
