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

#include "CryLegacy_precompiled.h"
#include "Area.h"
#include "AreaSolid.h"
#include <IRenderAuxGeom.h>
#include "Components/IComponentAudio.h"

namespace
{
    static CArea::TAreaBoxes s_areaBoxes;
}

//////////////////////////////////////////////////////////////////////////
CArea::CArea(CAreaManager* pManager)
    :   m_VOrigin(0.0f)
    ,   m_VSize(0.0f)
    ,   m_PrevFade(-1.0f)
    ,   m_fProximity(5.0f)
    ,   m_fFadeDistance(-1.0f)
    ,   m_fEnvironmentFadeDistance(0.0f)
    ,   m_AreaGroupID(INVALID_AREA_GROUP_ID)
    ,   m_nPriority(0)
    ,   m_AreaID(-1)
    ,   m_EntityID(INVALID_ENTITYID)
    ,   m_BoxMin(ZERO)
    ,   m_BoxMax(ZERO)
    ,   m_SphereCenter(0)
    ,   m_SphereRadius(0)
    ,   m_SphereRadius2(0)
    ,   m_bIsActive(false)
    ,   m_bObstructRoof(false)
    ,   m_bObstructFloor(false)
    ,   m_bEntityIdsResolved(false)
    ,   m_bAllObstructed(0)
    ,   m_nObstructedCount(0)
    ,   m_AreaSolid(0)
    ,   m_oNULLVec(ZERO)
{
    m_AreaType = ENTITY_AREA_TYPE_SHAPE;
    m_InvMatrix.SetIdentity();
    m_WorldTM.SetIdentity();
    m_pAreaManager = pManager;
    m_bInitialized = false;
    m_bHasSoundAttached = false;
    m_bAttachedSoundTested = false;
    m_mapEntityCachedAreaData.reserve(256);

    // All sides not obstructed by default
    memset(&m_abBoxSideObstruction, 0, 6);

    m_bbox_holder = s_areaBoxes.size();
    s_areaBoxes.push_back(SBoxHolder());
    s_areaBoxes[m_bbox_holder].area = this;
}


//////////////////////////////////////////////////////////////////////////
CArea::~CArea(void)
{
    ClearEntities();
    m_pAreaManager->Unregister(this);
    ReleaseAreaData();

    size_t last = s_areaBoxes.size() - 1;
    s_areaBoxes[m_bbox_holder].box = s_areaBoxes[last].box;
    (s_areaBoxes[m_bbox_holder].area = s_areaBoxes[last].area)->m_bbox_holder = m_bbox_holder;
    s_areaBoxes.erase(s_areaBoxes.begin() + last, s_areaBoxes.end());
}

//////////////////////////////////////////////////////////////////////////
void CArea::Release()
{
    delete this;
}

//////////////////////////////////////////////////////////////////////////
void CArea::SetSoundObstructionOnAreaFace(int unsigned const nFaceIndex, bool const bObstructs)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);
    switch (m_AreaType)
    {
    case ENTITY_AREA_TYPE_BOX:
    {
        m_abBoxSideObstruction[nFaceIndex].bObstructed = bObstructs ? 1 : 0;

        m_nObstructedCount = 0;
        for (unsigned int i = 0; i < 6; ++i)
        {
            if (m_abBoxSideObstruction[i].bObstructed)
            {
                ++m_nObstructedCount;
            }
        }

        m_bAllObstructed = 0;
        if (m_nObstructedCount == 6)
        {
            m_bAllObstructed = 1;
        }
    }
    break;
    case ENTITY_AREA_TYPE_SHAPE:
    {
        unsigned int const nSegmentCount = m_vpSegments.size();
        if (nFaceIndex < nSegmentCount)
        {
            m_vpSegments[nFaceIndex]->bObstructSound = bObstructs;
        }
        else
        {
            // We exceed segment count which could mean
            // that the user wants to set roof and floor sound obstruction
            if (nFaceIndex == nSegmentCount)
            {
                // The user wants to set roof sound obstruction
                m_bObstructRoof = bObstructs;
            }
            else if (nFaceIndex == nSegmentCount + 1)
            {
                // The user wants to set floor sound obstruction
                m_bObstructFloor = bObstructs;
            }
        }
    }
    break;
    }
}

//////////////////////////////////////////////////////////////////////////
void CArea::SetAreaType(EEntityAreaType type)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);
    m_AreaType = type;

    // to prevent gravityvolumes being evaluated in the
    // AreaManager::UpdatePlayer function, that caused problems.
    if (m_AreaType == ENTITY_AREA_TYPE_GRAVITYVOLUME)
    {
        m_pAreaManager->Unregister(this);
    }
}

// resets area - clears all segments in area
//////////////////////////////////////////////////////////////////////////
void CArea::ClearPoints()
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);
    a2DSegment* carSegment;

    for (unsigned int sIdx = 0; sIdx < m_vpSegments.size(); sIdx++)
    {
        carSegment = m_vpSegments[sIdx];
        delete carSegment;
    }
    m_vpSegments.clear();
    m_bInitialized = false;
}

//////////////////////////////////////////////////////////////////////////
unsigned CArea::MemStat()
{
    unsigned memSize = sizeof *this;

    memSize += m_vpSegments.size() * (sizeof(a2DSegment) + sizeof(a2DSegment*));
    return memSize;
}

//adds segment to area, calculates line parameters y=kx+b, sets horizontal/vertical flags
//////////////////////////////////////////////////////////////////////////
void    CArea::AddSegment(const a2DPoint& p0, const a2DPoint& p1, bool const bObstructSound)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);
    a2DSegment* newSegment          = new a2DSegment;
    newSegment->bObstructSound  = bObstructSound;

    //if this is horizontal line set flag. This segment is needed only for distance calculations
    if (p1.y == p0.y)
    {
        newSegment->isHorizontal = true;
    }
    else
    {
        newSegment->isHorizontal = false;
    }

    if (p0.x < p1.x)
    {
        newSegment->bbox.min.x = p0.x;
        newSegment->bbox.max.x = p1.x;
    }
    else
    {
        newSegment->bbox.min.x = p1.x;
        newSegment->bbox.max.x = p0.x;
    }

    if (p0.y < p1.y)
    {
        newSegment->bbox.min.y = p0.y;
        newSegment->bbox.max.y = p1.y;
    }
    else
    {
        newSegment->bbox.min.y = p1.y;
        newSegment->bbox.max.y = p0.y;
    }

    if (!newSegment->isHorizontal)
    {
        //if this is vertical line - spesial case
        if (p1.x == p0.x)
        {
            newSegment->k = 0;
            newSegment->b = p0.x;
        }
        else
        {
            newSegment->k = (p1.y - p0.y) / (p1.x - p0.x);
            newSegment->b = p0.y - newSegment->k * p0.x;
        }
    }
    else
    {
        newSegment->k = 0;
        newSegment->b = 0;
    }
    m_vpSegments.push_back(newSegment);
}

// calculates min distance from point within area to the border of area
// returns fade coefficient: Distance/m_Proximity
// [0 - on the very border of area, 1 - inside area, distance to border is more than m_Proximity]
//////////////////////////////////////////////////////////////////////////
float   CArea::CalcDistToPoint(a2DPoint const& point) const
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);
    if (!m_bInitialized)
    {
        return -1;
    }

    if (m_fProximity == 0.0f)
    {
        return 1.0f;
    }

    float   distMin = m_fProximity * m_fProximity;
    float   curDist;
    a2DBBox proximityBox;

    proximityBox.max.x = point.x + m_fProximity;
    proximityBox.max.y = point.y + m_fProximity;
    proximityBox.min.x = point.x - m_fProximity;
    proximityBox.min.y = point.y - m_fProximity;

    for (unsigned int sIdx = 0; sIdx < m_vpSegments.size(); sIdx++)
    {
        a2DSegment* curSg = m_vpSegments[sIdx];

        if (!m_vpSegments[sIdx]->bbox.BBoxOutBBox2D(proximityBox))
        {
            if (m_vpSegments[sIdx]->isHorizontal)
            {
                if (point.x < m_vpSegments[sIdx]->bbox.min.x)
                {
                    curDist = m_vpSegments[sIdx]->bbox.min.DistSqr(point);
                }
                else if (point.x > m_vpSegments[sIdx]->bbox.max.x)
                {
                    curDist = m_vpSegments[sIdx]->bbox.max.DistSqr(point);
                }
                else
                {
                    curDist = fabsf(point.y - m_vpSegments[sIdx]->bbox.max.y);
                }
                curDist *= curDist;
            }
            else
            {
                if (m_vpSegments[sIdx]->k == 0.0f)
                {
                    if (point.y < m_vpSegments[sIdx]->bbox.min.y)
                    {
                        curDist = m_vpSegments[sIdx]->bbox.min.DistSqr(point);
                    }
                    else if (point.y > m_vpSegments[sIdx]->bbox.max.y)
                    {
                        curDist = m_vpSegments[sIdx]->bbox.max.DistSqr(point);
                    }
                    else
                    {
                        curDist = fabsf(point.x - m_vpSegments[sIdx]->b);
                    }
                    curDist *= curDist;
                }
                else
                {
                    a2DPoint    intersection;
                    float   b2, k2;
                    k2 = -1.0f / m_vpSegments[sIdx]->k;
                    b2 = point.y - k2 * point.x;
                    intersection.x = (b2 - m_vpSegments[sIdx]->b) / (m_vpSegments[sIdx]->k - k2);
                    intersection.y = k2 * intersection.x + b2;

                    if (intersection.x < m_vpSegments[sIdx]->bbox.min.x)
                    {
                        if (m_vpSegments[sIdx]->k < 0)
                        {
                            curDist = point.DistSqr(m_vpSegments[sIdx]->bbox.min.x, m_vpSegments[sIdx]->bbox.max.y);
                        }
                        else
                        {
                            curDist = point.DistSqr(m_vpSegments[sIdx]->bbox.min);
                        }
                    }
                    else if (intersection.x > m_vpSegments[sIdx]->bbox.max.x)
                    {
                        if (m_vpSegments[sIdx]->k < 0)
                        {
                            curDist = point.DistSqr(m_vpSegments[sIdx]->bbox.max.x, m_vpSegments[sIdx]->bbox.min.y);
                        }
                        else
                        {
                            curDist = point.DistSqr(m_vpSegments[sIdx]->bbox.max);
                        }
                    }
                    else
                    {
                        curDist = intersection.DistSqr(point);
                    }
                }
                if (curDist < distMin)
                {
                    distMin = curDist;
                }
            }
        }
    }

    return sqrt_tpl(distMin) / m_fProximity; // TODO: Check if "distmin/m_fProximity*m_fProximity" is faster
}

// check if the point is within the area
// first BBox check, then count number of intersections for horizontal ray from point and area segments
// if the number is odd - the point is inside
//////////////////////////////////////////////////////////////////////////
bool CArea::CalcPointWithin(EntityId const nEntityID, Vec3 const& point3d, bool const bIgnoreHeight /* = false */, bool const bCacheResult /* = true */)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);
    bool bResult = false;

    if (m_bInitialized)
    {
        SCachedAreaData* pCachedData = NULL;

        if (nEntityID != INVALID_ENTITYID)
        {
            TEntityCachedAreaDataMap::iterator Iter(m_mapEntityCachedAreaData.find(nEntityID));

            if (Iter != m_mapEntityCachedAreaData.end())
            {
                pCachedData = &(Iter->second);
            }
        }

        if (pCachedData != NULL && (pCachedData->eFlags & eCachedAreaData_PointWithinValid) != 0)
        {
            bResult = GetCachedPointWithin(nEntityID);
        }
        else
        {
            switch (m_AreaType)
            {
            case ENTITY_AREA_TYPE_SPHERE:
            {
                Vec3 oPoint(point3d - m_SphereCenter);

                if (bIgnoreHeight)
                {
                    oPoint.z = 0.0f;
                }

                bResult = (oPoint.GetLengthSquared() < m_SphereRadius2);

                break;
            }
            case ENTITY_AREA_TYPE_BOX:
            {
                Vec3 p3d = m_InvMatrix.TransformPoint(point3d);

                if (bIgnoreHeight)
                {
                    p3d.z = m_BoxMax.z;
                }

                // And put the result into the data cache
                if ((p3d.x < m_BoxMin.x) ||
                    (p3d.y < m_BoxMin.y) ||
                    (p3d.z < m_BoxMin.z) ||
                    (p3d.x > m_BoxMax.x) ||
                    (p3d.y > m_BoxMax.y) ||
                    (p3d.z > m_BoxMax.z))
                {
                    bResult = false;
                }
                else
                {
                    bResult = true;
                }

                break;
            }
            case ENTITY_AREA_TYPE_SOLID:
            {
                if (point3d.IsValid())
                {
                    Vec3 localPoint3D = m_InvMatrix.TransformPoint(point3d);
                    bResult = m_AreaSolid->IsInside(localPoint3D);
                }

                break;
            }
            case ENTITY_AREA_TYPE_SHAPE:
            {
                bResult = true;

                if (!bIgnoreHeight)
                {
                    if (m_VSize > 0.0f)
                    {
                        if (point3d.z < m_VOrigin || point3d.z > m_VOrigin + m_VSize)
                        {
                            bResult = false;
                        }
                    }
                }

                if (bResult)
                {
                    a2DPoint const* const point = (CArea::a2DPoint*)(&point3d);

                    bResult = !m_areaBBox.PointOutBBox2D(*point);

                    if (bResult)
                    {
                        size_t cntr = 0;
                        size_t const nSegmentCount = m_vpSegments.size();

                        for (size_t sIdx = 0; sIdx < nSegmentCount; ++sIdx)
                        {
                            if (!m_vpSegments[sIdx]->isHorizontal && !m_vpSegments[sIdx]->bbox.PointOutBBox2DVertical(*point))
                            {
                                if (m_vpSegments[sIdx]->IntersectsXPosVertical(*point) || m_vpSegments[sIdx]->IntersectsXPos(*point))
                                {
                                    ++cntr;
                                }
                            }
                        }

                        bResult = ((cntr & 1) != 0);
                    }
                }

                break;
            }
            default:
            {
                CryFatalError("Unknown area type during CArea::CalcPointWithin");

                break;
            }
            }

            // Set the flags and put the result into the data cache.
            if (pCachedData != NULL && bCacheResult)
            {
                if (bIgnoreHeight)
                {
                    pCachedData->eFlags = (ECachedAreaData)(pCachedData->eFlags | eCachedAreaData_PointWithinValid | eCachedAreaData_PointWithinValidHeightIgnored);
                }
                else
                {
                    pCachedData->eFlags = (ECachedAreaData)(pCachedData->eFlags | eCachedAreaData_PointWithinValid);
                }

                pCachedData->bPointWithin = bResult;
            }
        }
    }

    return bResult;
}

//  for editor use - if point is within - returns min horizontal distance to border
//  if point out - returns -1
//////////////////////////////////////////////////////////////////////////
float   CArea::CalcPointWithinDist(EntityId const nEntityID, Vec3 const& point3d, bool const bIgnoreSoundObstruction /* = true */, bool const bCacheResult /* = true */)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);
    float   fMinDist = -1.0f;

    if (m_bInitialized)
    {
        float   fDistanceWithinSq = 0.0f;
        SCachedAreaData* pCachedData = NULL;

        if (nEntityID != INVALID_ENTITYID)
        {
            TEntityCachedAreaDataMap::iterator Iter(m_mapEntityCachedAreaData.find(nEntityID));

            if (Iter != m_mapEntityCachedAreaData.end())
            {
                pCachedData = &(Iter->second);
            }
        }

        // Only computes distance if point is inside the area.
        bool bGoAheadAndCompute = false;

        if (pCachedData != NULL)
        {
            bGoAheadAndCompute = (pCachedData->eFlags & eCachedAreaData_DistWithinSqValid) == 0 && pCachedData->ePosType == AREA_POS_TYPE_2DINSIDE_ZINSIDE;
        }
        else
        {
            // Unfortunately we need to compute the position type.
            bGoAheadAndCompute = CalcPosType(nEntityID, point3d) == AREA_POS_TYPE_2DINSIDE_ZINSIDE;
        }

        if (bGoAheadAndCompute)
        {
            switch (m_AreaType)
            {
            case ENTITY_AREA_TYPE_SPHERE:
            {
                Vec3 const sPnt = point3d - m_SphereCenter;
                float fPointLengthSq = sPnt.GetLengthSquared();

                fMinDist = m_SphereRadius - sPnt.GetLength();
                fDistanceWithinSq = m_SphereRadius2 - fPointLengthSq;

                break;
            }
            case ENTITY_AREA_TYPE_BOX:
            {
                if (!bIgnoreSoundObstruction)
                {
                    Vec3 v3ClosestPos(ZERO);
                    CalcClosestPointToObstructedBox(v3ClosestPos, fDistanceWithinSq, point3d);
                    fMinDist = (fDistanceWithinSq > 0.0f) ? sqrt_tpl(fDistanceWithinSq) : fMinDist;
                }
                else
                {
                    Vec3 oTemp(ZERO);
                    fDistanceWithinSq = ClosestPointOnHullDistSq(nEntityID, point3d, oTemp, true);
                    fMinDist = (fDistanceWithinSq > 0.0f) ? sqrt_tpl(fDistanceWithinSq) : fMinDist;
                }

                break;
            }
            case ENTITY_AREA_TYPE_SOLID:
            {
                Vec3 const localPoint3D(m_InvMatrix.TransformPoint(point3d));
                int queryFlag = bIgnoreSoundObstruction ? CAreaSolid::eSegmentQueryFlag_Obstruction : CAreaSolid::eSegmentQueryFlag_All;
                Vec3 vOnHull;

                if (m_AreaSolid->QueryNearest(localPoint3D, queryFlag, vOnHull, fDistanceWithinSq))
                {
                    fMinDist = (fDistanceWithinSq > 0.0f) ? sqrt_tpl(fDistanceWithinSq) : fMinDist;
                }

                break;
            }
            case ENTITY_AREA_TYPE_SHAPE:
            {
                if (!bIgnoreSoundObstruction)
                {
                    Vec3 v3ClosestPos;
                    CalcClosestPointToObstructedShape(nEntityID, v3ClosestPos, fDistanceWithinSq, point3d);
                    fMinDist = (fDistanceWithinSq > 0.0f) ? sqrt_tpl(fDistanceWithinSq) : fMinDist;
                }
                else
                {
                    // check distance to every line segment, remember the closest
                    size_t const nSegmentSize = m_vpSegments.size();

                    for (size_t sIdx = 0; sIdx < nSegmentSize; sIdx++)
                    {
                        float fT;
                        a2DSegment* curSg = m_vpSegments[sIdx];

                        Vec3 startSeg(curSg->GetStart().x, curSg->GetStart().y, point3d.z);
                        Vec3 endSeg(curSg->GetEnd().x, curSg->GetEnd().y, point3d.z);
                        Lineseg line(startSeg, endSeg);

                        // Returns distance from a point to a line segment, ignoring the z coordinates
                        float fDist = Distance::Point_Lineseg2D(point3d, line, fT);

                        if (fMinDist == -1)
                        {
                            fMinDist = fDist;
                        }

                        fMinDist = min(fDist, fMinDist);
                    }

                    if (m_VSize > 0.0f)
                    {
                        float fDistToRoof = fMinDist + 1.0f;
                        float fDistToFloor = fMinDist + 1.0f;

                        if (!m_bObstructFloor)
                        {
                            fDistToFloor = point3d.z - m_VOrigin;
                        }

                        if (!m_bObstructRoof)
                        {
                            fDistToRoof = m_VOrigin + m_VSize - point3d.z;
                        }

                        float   fZDist = min(fDistToFloor, fDistToRoof);
                        fMinDist = min(fMinDist, fZDist);
                    }

                    fDistanceWithinSq   = (fDistanceWithinSq > 0.0f) ? fMinDist * fMinDist : 0.0f;
                }

                break;
            }
            default:
            {
                CryFatalError("Unknown area type during CArea::CalcPointWithinDist");

                break;
            }
            }
        }

        // Set the flags and put the shortest distance into the data cache also when not computing.
        if (pCachedData != NULL && bCacheResult)
        {
            if (bIgnoreSoundObstruction)
            {
                pCachedData->eFlags = (ECachedAreaData)(pCachedData->eFlags | eCachedAreaData_DistWithinSqValid | eCachedAreaData_DistWithinSqValidNotObstructed);
            }
            else
            {
                pCachedData->eFlags = (ECachedAreaData)(pCachedData->eFlags | eCachedAreaData_DistWithinSqValid);
            }

            pCachedData->fDistanceWithinSq = fDistanceWithinSq;
        }
    }

    return fMinDist;
}

//////////////////////////////////////////////////////////////////////////
float CArea::ClosestPointOnHullDistSq(EntityId const nEntityID, Vec3 const& Point3d, Vec3& OnHull3d, bool const bIgnoreSoundObstruction /* = true */, bool const bCacheResult /* = true */)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);
    float fClosestDistance = -1.0f;

    if (m_bInitialized)
    {
        SCachedAreaData* pCachedData = NULL;

        if (nEntityID != INVALID_ENTITYID)
        {
            TEntityCachedAreaDataMap::iterator Iter(m_mapEntityCachedAreaData.find(nEntityID));

            if (Iter != m_mapEntityCachedAreaData.end())
            {
                pCachedData = &(Iter->second);
            }
        }

        if (pCachedData != NULL && (pCachedData->eFlags & eCachedAreaData_PosOnHullValid) != 0 &&
            (pCachedData->eFlags & eCachedAreaData_DistWithinSqValid) != 0)
        {
            fClosestDistance = GetCachedPointWithinDistSq(nEntityID);
            OnHull3d = GetCachedPointOnHull(nEntityID);
        }
        else
        {
            Vec3 Closest3d(ZERO);

            switch (m_AreaType)
            {
            case ENTITY_AREA_TYPE_SOLID:
            {
                CalcClosestPointToSolid(Point3d, bIgnoreSoundObstruction, fClosestDistance, &OnHull3d);

                break;
            }
            case ENTITY_AREA_TYPE_SHAPE:
            {
                if (!bIgnoreSoundObstruction)
                {
                    CalcClosestPointToObstructedShape(nEntityID, OnHull3d, fClosestDistance, Point3d);
                }
                else
                {
                    float fDistToRoof       = 0.0f;
                    float fDistToFloor  = 0.0f;
                    float fZDistTemp        = 0.0f;

                    if (m_VSize)
                    {
                        // negative means from inside to hull
                        fDistToRoof     = Point3d.z - (m_VOrigin + m_VSize);
                        fDistToFloor    = m_VOrigin - Point3d.z;

                        if (fabsf(fDistToFloor) < fabsf(fDistToRoof))
                        {
                            // below
                            if (m_bObstructFloor)
                            {
                                fDistToFloor = 0.0f;
                                fZDistTemp = fDistToRoof;
                            }
                            else
                            {
                                fDistToRoof = 0.0f;
                                fZDistTemp = fDistToFloor;
                            }
                        }
                        else
                        {
                            // above
                            if (m_bObstructRoof)
                            {
                                fDistToRoof = 0.0f;
                                fZDistTemp = fDistToFloor;
                            }
                            else
                            {
                                fDistToFloor = 0.0f;
                                fZDistTemp = fDistToRoof;
                            }
                        }
                    }

                    float fZDistSq = fZDistTemp * fZDistTemp;
                    float fXDistSq = 0.0f;

                    bool bIsIn2DShape = false;

                    if (pCachedData && (pCachedData->eFlags & eCachedAreaData_PosTypeValid) != 0)
                    {
                        bIsIn2DShape = (pCachedData->ePosType == AREA_POS_TYPE_2DINSIDE_ZABOVE)
                            || (pCachedData->ePosType == AREA_POS_TYPE_2DINSIDE_ZINSIDE)
                            || (pCachedData->ePosType == AREA_POS_TYPE_2DINSIDE_ZBELOW);
                    }
                    else
                    {
                        bIsIn2DShape = (CalcPosType(nEntityID, Point3d) == AREA_POS_TYPE_2DINSIDE_ZINSIDE);
                    }

                    //// point is not under or above area shape, so approach from the side
                    // Find the line segment that is closest to the 2d point.
                    size_t const nSegmentSize = m_vpSegments.size();

                    for (size_t sIdx = 0; sIdx < nSegmentSize; sIdx++)
                    {
                        float fT;
                        a2DSegment* curSg = m_vpSegments[sIdx];

                        Vec3 startSeg(curSg->GetStart().x, curSg->GetStart().y, Point3d.z);
                        Vec3 endSeg(curSg->GetEnd().x, curSg->GetEnd().y, Point3d.z);
                        Lineseg line(startSeg, endSeg);

                        /// Returns distance from a point to a line segment, ignoring the z coordinates
                        fXDistSq = Distance::Point_Lineseg2DSq(Point3d, line, fT);

                        float fThisDistance = 0.0f;

                        if (bIsIn2DShape && fZDistSq)
                        {
                            fThisDistance = min(fXDistSq, fZDistSq);
                        }
                        else
                        {
                            fThisDistance = fXDistSq + fZDistSq;
                        }

                        // is this closer than the previous one?
                        if (fThisDistance < fClosestDistance)
                        {
                            fClosestDistance = fThisDistance;
                            // find closest point
                            if (fZDistSq && fZDistSq < fXDistSq)
                            {
                                Closest3d = Point3d;
                                Closest3d.z = Point3d.z + fDistToFloor - fDistToRoof;
                            }
                            else
                            {
                                Closest3d = (line.GetPoint(fT));
                            }
                        }
                    }

                    OnHull3d = Closest3d;
                }

                break;
            }

            case ENTITY_AREA_TYPE_SPHERE:
            {
                Vec3 Temp = Point3d - m_SphereCenter;
                OnHull3d = Temp.normalize() * m_SphereRadius;
                OnHull3d += m_SphereCenter;
                fClosestDistance = OnHull3d.GetSquaredDistance(Point3d);

                break;
            }
            case ENTITY_AREA_TYPE_BOX:
            {
                if (!bIgnoreSoundObstruction)
                {
                    CalcClosestPointToObstructedBox(OnHull3d, fClosestDistance, Point3d);
                }
                else
                {
                    Vec3 const p3d = m_InvMatrix.TransformPoint(Point3d);
                    AABB const myAABB(m_BoxMin, m_BoxMax);
                    fClosestDistance = Distance::Point_AABBSq(p3d, myAABB, OnHull3d);

                    if (m_abBoxSideObstruction[4].bObstructed && OnHull3d.z == myAABB.max.z)
                    {
                        // Point is on the roof plane, but may be on the edge already
                        Vec2 vTop(OnHull3d.x, myAABB.max.y);
                        Vec2 vLeft(myAABB.min.x, OnHull3d.y);
                        Vec2 vLow(OnHull3d.x, myAABB.min.y);
                        Vec2 vRight(myAABB.max.x, OnHull3d.y);

                        float fDistanceToTop        = p3d.GetSquaredDistance2D(vTop);
                        float fDistanceToLeft       = p3d.GetSquaredDistance2D(vLeft);
                        float fDistanceToLow        = p3d.GetSquaredDistance2D(vLow);
                        float fDistanceToRight  = p3d.GetSquaredDistance2D(vRight);
                        float fTempMinDistance = fDistanceToTop;

                        OnHull3d = vTop;

                        if (fDistanceToLeft < fTempMinDistance)
                        {
                            OnHull3d = vLeft;
                            fTempMinDistance = fDistanceToLeft;
                        }

                        if (fDistanceToLow < fTempMinDistance)
                        {
                            OnHull3d = vLow;
                            fTempMinDistance = fDistanceToLow;
                        }

                        if (fDistanceToRight < fTempMinDistance)
                        {
                            OnHull3d = vRight;
                            fTempMinDistance = fDistanceToRight;
                        }

                        OnHull3d.z = min(myAABB.max.z, p3d.z);
                        fClosestDistance = OnHull3d.GetSquaredDistance(p3d);
                    }

                    if (m_abBoxSideObstruction[5].bObstructed && OnHull3d.z == myAABB.min.z)
                    {
                        // Point is on the roof plane, but may be on the edge already
                        Vec2 vTop(OnHull3d.x, myAABB.max.y);
                        Vec2 vLeft(myAABB.min.x, OnHull3d.y);
                        Vec2 vLow(OnHull3d.x, myAABB.min.y);
                        Vec2 vRight(myAABB.max.x, OnHull3d.y);

                        float fDistanceToTop        = p3d.GetSquaredDistance2D(vTop);
                        float fDistanceToLeft       = p3d.GetSquaredDistance2D(vLeft);
                        float fDistanceToLow        = p3d.GetSquaredDistance2D(vLow);
                        float fDistanceToRight  = p3d.GetSquaredDistance2D(vRight);
                        float fTempMinDistance = fDistanceToTop;

                        OnHull3d = vTop;

                        if (fDistanceToLeft < fTempMinDistance)
                        {
                            OnHull3d = vLeft;
                            fTempMinDistance = fDistanceToLeft;
                        }

                        if (fDistanceToLow < fTempMinDistance)
                        {
                            OnHull3d = vLow;
                            fTempMinDistance = fDistanceToLow;
                        }

                        if (fDistanceToRight < fTempMinDistance)
                        {
                            OnHull3d = vRight;
                            fTempMinDistance = fDistanceToRight;
                        }

                        OnHull3d.z = max(myAABB.min.z, p3d.z);
                        fClosestDistance = OnHull3d.GetSquaredDistance(p3d);
                    }

                    OnHull3d = m_WorldTM.TransformPoint(OnHull3d);
                }

                // TODO transform point back to world ... (why?)
                break;
            }
            default:
            {
                CryFatalError("Unknown area type during CArea::ClosestPointOnHullDistSq");

                break;
            }
            }

            // Put the shortest distance and point into the data cache.
            if (pCachedData != NULL && bCacheResult)
            {
                if (bIgnoreSoundObstruction)
                {
                    pCachedData->eFlags = (ECachedAreaData)(pCachedData->eFlags | eCachedAreaData_PosOnHullValid | eCachedAreaData_DistWithinSqValid | eCachedAreaData_DistWithinSqValidNotObstructed);
                }
                else
                {
                    pCachedData->eFlags = (ECachedAreaData)(pCachedData->eFlags | eCachedAreaData_PosOnHullValid | eCachedAreaData_DistWithinSqValid);
                }

                pCachedData->oPos              = OnHull3d;
                pCachedData->fDistanceWithinSq = fClosestDistance;
            }
        }
    }

    return fClosestDistance;
}

//////////////////////////////////////////////////////////////////////////
float CArea::CalcPointNearDistSq(EntityId const nEntityID, Vec3 const& Point3d, Vec3& OnHull3d, bool const bIgnoreSoundObstruction /* = true */, bool const bCacheResult /* = true */)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);
    float fClosestDistance = -1.0f;

    if (m_bInitialized)
    {
        SCachedAreaData* pCachedData = NULL;

        if (nEntityID != INVALID_ENTITYID)
        {
            TEntityCachedAreaDataMap::iterator Iter(m_mapEntityCachedAreaData.find(nEntityID));

            if (Iter != m_mapEntityCachedAreaData.end())
            {
                pCachedData = &(Iter->second);
            }
        }

        if (pCachedData != NULL && (pCachedData->eFlags & eCachedAreaData_PosOnHullValid) != 0 &&
            (pCachedData->eFlags & eCachedAreaData_DistNearSqValid) != 0)
        {
            fClosestDistance = GetCachedPointNearDistSq(nEntityID);
            OnHull3d = GetCachedPointOnHull(nEntityID);
        }
        else
        {
            Vec3 Closest3d(ZERO);

            switch (m_AreaType)
            {
            case ENTITY_AREA_TYPE_SOLID:
            {
                CalcClosestPointToSolid(Point3d, bIgnoreSoundObstruction, fClosestDistance, &OnHull3d);

                break;
            }
            case ENTITY_AREA_TYPE_SHAPE:
            {
                if (!bIgnoreSoundObstruction)
                {
                    CalcClosestPointToObstructedShape(nEntityID, OnHull3d, fClosestDistance, Point3d);
                }
                else
                {
                    float fZDistSq = 0.0f;
                    float fXDistSq = FLT_MAX;

                    // first find the closest edge
                    size_t const nSegmentSize = m_vpSegments.size();

                    for (size_t sIdx = 0; sIdx < nSegmentSize; sIdx++)
                    {
                        float fT = 0.0f;
                        a2DSegment const* const curSg = m_vpSegments[sIdx];

                        Vec3 startSeg(curSg->GetStart().x, curSg->GetStart().y, Point3d.z);
                        Vec3 endSeg(curSg->GetEnd().x, curSg->GetEnd().y, Point3d.z);
                        Lineseg line(startSeg, endSeg);

                        // Returns distance from a point to a line segment, ignoring the z coordinates
                        float fThisXDistSq = Distance::Point_Lineseg2DSq(Point3d, line, fT);

                        if (fThisXDistSq < fXDistSq)
                        {
                            // find closest point
                            fXDistSq = fThisXDistSq;
                            Closest3d = (line.GetPoint(fT));
                        }
                    }

                    // now we have Closest3d being the point on the 2D hull of the shape
                    if (m_VSize)
                    {
                        // negative means from inside to hull
                        float fDistToRoof = Point3d.z - (m_VOrigin + m_VSize);
                        float fDistToFloor = m_VOrigin - Point3d.z;

                        float fZRoofSq = fDistToRoof * fDistToRoof;
                        float fZFloorSq = fDistToFloor * fDistToFloor;

                        bool bIsIn2DShape = false;
                        if (pCachedData != NULL && (pCachedData->eFlags & eCachedAreaData_PosTypeValid) != 0)
                        {
                            bIsIn2DShape = (pCachedData->ePosType == AREA_POS_TYPE_2DINSIDE_ZABOVE)
                                || (pCachedData->ePosType == AREA_POS_TYPE_2DINSIDE_ZINSIDE)
                                || (pCachedData->ePosType == AREA_POS_TYPE_2DINSIDE_ZBELOW);
                        }
                        else
                        {
                            bIsIn2DShape = (CalcPosType(nEntityID, Point3d) == AREA_POS_TYPE_2DINSIDE_ZINSIDE);
                        }

                        if (bIsIn2DShape)
                        {
                            // point is below, in, or above the area

                            if ((Point3d.z < m_VOrigin + m_VSize && Point3d.z > m_VOrigin))
                            {
                                // Point is inside z-boundary
                                if (!m_bObstructRoof && (fZRoofSq < fXDistSq) && (fZRoofSq < fZFloorSq))
                                {
                                    // roof is closer than side
                                    fZDistSq = fZRoofSq;
                                    Closest3d = Point3d;
                                    fXDistSq = 0.0f;
                                }

                                if (!m_bObstructFloor && (fZFloorSq < fXDistSq) && (fZFloorSq < fZRoofSq))
                                {
                                    // floor is closer than side
                                    fZDistSq = fZFloorSq;
                                    Closest3d = Point3d;
                                    fXDistSq = 0.0f;
                                }

                                // correcting z-axis value
                                if (fZRoofSq < fZFloorSq)
                                {
                                    Closest3d.z = Point3d.z - fDistToRoof;
                                }
                                else
                                {
                                    Closest3d.z = Point3d.z - fDistToFloor;
                                }
                            }
                            else
                            {
                                // point is above or below area
                                if (fabsf(fDistToRoof) < fabsf(fDistToFloor))
                                {
                                    // being above
                                    if (!m_bObstructRoof)
                                    {
                                        // perpendicular point to Roof
                                        fXDistSq = 0.0f;
                                        Closest3d = Point3d;
                                    }
                                    // correcting z axis value
                                    Closest3d.z = m_VOrigin + m_VSize;
                                    fZDistSq = fZRoofSq;
                                }
                                else
                                {
                                    // being below
                                    if (!m_bObstructFloor)
                                    {
                                        // perpendicular point to Floor
                                        fXDistSq = 0.0f;
                                        Closest3d = Point3d;
                                    }

                                    // correcting z axis value
                                    Closest3d.z = m_VOrigin;
                                    fZDistSq = fZFloorSq;
                                }
                            }
                        }
                        else
                        {
                            // outside of 2D Shape, so diagonal or only to the side
                            if ((Point3d.z > m_VOrigin + m_VSize || Point3d.z < m_VOrigin))
                            {
                                // Point is outside z-boundary
                                // point is above or below area
                                if (fabsf(fDistToRoof) < fabsf(fDistToFloor))
                                {
                                    // being above
                                    fZDistSq = fZRoofSq;
                                    Closest3d.z = m_VOrigin + m_VSize;
                                }
                                else
                                {
                                    // being below
                                    fZDistSq = fZFloorSq;
                                    Closest3d.z = m_VOrigin;
                                }
                            }
                            else
                            {
                                // on the side and outside of the area, so point is on the face
                                // correct z-Value
                                Closest3d.z = Point3d.z;
                            }
                        }
                    }
                    else
                    {
                        // infinite high area
                        // Closest is on an edge, ZDistance is 0, so nothing really to do here
                    }

                    fClosestDistance = fXDistSq + fZDistSq;
                    OnHull3d         = Closest3d;
                }

                break;
            }
            case ENTITY_AREA_TYPE_SPHERE:
            {
                Vec3 Temp = Point3d - m_SphereCenter;
                OnHull3d = Temp.normalize() * m_SphereRadius;
                OnHull3d += m_SphereCenter;
                fClosestDistance = OnHull3d.GetSquaredDistance(Point3d);

                break;
            }
            case ENTITY_AREA_TYPE_BOX:
            {
                if (!bIgnoreSoundObstruction)
                {
                    CalcClosestPointToObstructedBox(OnHull3d, fClosestDistance, Point3d);
                }
                else
                {
                    Vec3 const p3d = m_InvMatrix.TransformPoint(Point3d);
                    AABB const myAABB(m_BoxMin, m_BoxMax);

                    fClosestDistance = Distance::Point_AABBSq(p3d, myAABB, OnHull3d);

                    if (m_abBoxSideObstruction[4].bObstructed && OnHull3d.z == myAABB.max.z)
                    {
                        // Point is on the roof plane, but may be on the edge already
                        Vec2 vTop(OnHull3d.x, myAABB.max.y);
                        Vec2 vLeft(myAABB.min.x, OnHull3d.y);
                        Vec2 vLow(OnHull3d.x, myAABB.min.y);
                        Vec2 vRight(myAABB.max.x, OnHull3d.y);

                        float fDistanceToTop        = p3d.GetSquaredDistance2D(vTop);
                        float fDistanceToLeft       = p3d.GetSquaredDistance2D(vLeft);
                        float fDistanceToLow        = p3d.GetSquaredDistance2D(vLow);
                        float fDistanceToRight  = p3d.GetSquaredDistance2D(vRight);
                        float fTempMinDistance = fDistanceToTop;

                        OnHull3d = vTop;

                        if (fDistanceToLeft < fTempMinDistance)
                        {
                            OnHull3d = vLeft;
                            fTempMinDistance = fDistanceToLeft;
                        }

                        if (fDistanceToLow < fTempMinDistance)
                        {
                            OnHull3d = vLow;
                            fTempMinDistance = fDistanceToLow;
                        }

                        if (fDistanceToRight < fTempMinDistance)
                        {
                            OnHull3d = vRight;
                            fTempMinDistance = fDistanceToRight;
                        }

                        OnHull3d.z = min(myAABB.max.z, p3d.z);
                        fClosestDistance = OnHull3d.GetSquaredDistance(p3d);
                    }

                    if (m_abBoxSideObstruction[5].bObstructed && OnHull3d.z == myAABB.min.z)
                    {
                        // Point is on the roof plane, but may be on the edge already
                        Vec2 vTop(OnHull3d.x, myAABB.max.y);
                        Vec2 vLeft(myAABB.min.x, OnHull3d.y);
                        Vec2 vLow(OnHull3d.x, myAABB.min.y);
                        Vec2 vRight(myAABB.max.x, OnHull3d.y);

                        float fDistanceToTop        = p3d.GetSquaredDistance2D(vTop);
                        float fDistanceToLeft       = p3d.GetSquaredDistance2D(vLeft);
                        float fDistanceToLow        = p3d.GetSquaredDistance2D(vLow);
                        float fDistanceToRight  = p3d.GetSquaredDistance2D(vRight);
                        float fTempMinDistance = fDistanceToTop;

                        OnHull3d = vTop;

                        if (fDistanceToLeft < fTempMinDistance)
                        {
                            OnHull3d = vLeft;
                            fTempMinDistance = fDistanceToLeft;
                        }

                        if (fDistanceToLow < fTempMinDistance)
                        {
                            OnHull3d = vLow;
                            fTempMinDistance = fDistanceToLow;
                        }

                        if (fDistanceToRight < fTempMinDistance)
                        {
                            OnHull3d = vRight;
                            fTempMinDistance = fDistanceToRight;
                        }

                        OnHull3d.z = max(myAABB.min.z, p3d.z);
                        fClosestDistance = OnHull3d.GetSquaredDistance(p3d);
                    }

                    OnHull3d = m_WorldTM.TransformPoint(OnHull3d);
                }

                break;
            }
            default:
            {
                CryFatalError("Unknown area type during CArea::CalcPointNearDistSq (1)");

                break;
            }
            }

            // Put the shortest distance and point into the data cache.
            if (pCachedData != NULL && bCacheResult)
            {
                if (bIgnoreSoundObstruction)
                {
                    pCachedData->eFlags = (ECachedAreaData)(pCachedData->eFlags | eCachedAreaData_PosOnHullValid | eCachedAreaData_DistNearSqValid | eCachedAreaData_DistNearSqValidNotObstructed);
                }
                else
                {
                    pCachedData->eFlags = (ECachedAreaData)(pCachedData->eFlags | eCachedAreaData_PosOnHullValid | eCachedAreaData_DistNearSqValid);
                }

                pCachedData->oPos            = OnHull3d;
                pCachedData->fDistanceNearSq = fClosestDistance;
            }
        }
    }

    return fClosestDistance;
}

//////////////////////////////////////////////////////////////////////////
float CArea::CalcPointNearDistSq(EntityId const nEntityID, Vec3 const& Point3d, bool const bIgnoreHeight /* = false */, bool const bIgnoreSoundObstruction /* = true */, bool const bCacheResult /* = true */)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);
    float fClosestDistance = -1.0f;

    if (m_bInitialized)
    {
        SCachedAreaData* pCachedData = NULL;

        if (nEntityID != INVALID_ENTITYID)
        {
            TEntityCachedAreaDataMap::iterator Iter(m_mapEntityCachedAreaData.find(nEntityID));

            if (Iter != m_mapEntityCachedAreaData.end())
            {
                pCachedData = &(Iter->second);
            }
        }

        if (pCachedData != NULL && (pCachedData->eFlags & eCachedAreaData_DistNearSqValid) != 0)
        {
            fClosestDistance = GetCachedPointNearDistSq(nEntityID);
        }
        else
        {
            switch (m_AreaType)
            {
            case ENTITY_AREA_TYPE_SOLID:
            {
                CalcClosestPointToSolid(Point3d, bIgnoreSoundObstruction, fClosestDistance, NULL);

                break;
            }
            case ENTITY_AREA_TYPE_SHAPE:
            {
                if (!bIgnoreSoundObstruction)
                {
                    Vec3 oTemp(ZERO);
                    CalcClosestPointToObstructedShape(nEntityID, oTemp, fClosestDistance, Point3d);
                }
                else
                {
                    float fZDistSq = 0.0f;
                    float fXDistSq = FLT_MAX;

                    for (std::vector<a2DSegment*>::const_iterator it = m_vpSegments.begin(), itEnd = m_vpSegments.end(); it != itEnd; ++it)
                    {
                        a2DSegment const* const curSg = *it;
                        a2DPoint const curSgStart = curSg->GetStart(), curSgEnd = curSg->GetEnd();

                        Lineseg line;
                        line.start.x = curSgStart.x;
                        line.start.y = curSgStart.y;
                        line.end.x = curSgEnd.x;
                        line.end.y = curSgEnd.y;

                        /// Returns distance from a point to a line segment, ignoring the z coordinates
                        float fThisXDistSq = Distance::Point_Lineseg2DSq(Point3d, line);
                        fXDistSq = min(fXDistSq, fThisXDistSq);
                    }

                    if (m_VSize)
                    {
                        // negative means from inside to hull
                        float const fDistToRoof = Point3d.z - (m_VOrigin + m_VSize);
                        float const fDistToFloor = m_VOrigin - Point3d.z;

                        float const fZRoofSq = fDistToRoof * fDistToRoof;
                        float const fZFloorSq = fDistToFloor * fDistToFloor;

                        bool bIsIn2DShape = false;

                        if (pCachedData != NULL && (pCachedData->eFlags & eCachedAreaData_PosTypeValid) != 0)
                        {
                            bIsIn2DShape = (pCachedData->ePosType == AREA_POS_TYPE_2DINSIDE_ZABOVE)
                                || (pCachedData->ePosType == AREA_POS_TYPE_2DINSIDE_ZINSIDE)
                                || (pCachedData->ePosType == AREA_POS_TYPE_2DINSIDE_ZBELOW);
                        }
                        else
                        {
                            bIsIn2DShape = (CalcPosType(nEntityID, Point3d) == AREA_POS_TYPE_2DINSIDE_ZINSIDE);
                        }

                        if (bIsIn2DShape)
                        {
                            // point is below, in, or above the area
                            if ((Point3d.z < m_VOrigin + m_VSize && Point3d.z > m_VOrigin))
                            {
                                // Point is inside z-boundary
                                if (!m_bObstructRoof && (fZRoofSq < fXDistSq) && (fZRoofSq < fZFloorSq))
                                {
                                    // roof is closer than side
                                    fZDistSq = fZRoofSq;
                                    fXDistSq = 0.0f;
                                }

                                if (!m_bObstructFloor && (fZFloorSq < fXDistSq) && (fZFloorSq < fZRoofSq))
                                {
                                    // floor is closer than side
                                    fZDistSq = fZFloorSq;
                                    fXDistSq = 0.0f;
                                }
                            }
                            else
                            {
                                // point is above or below area
                                if (fabsf(fDistToRoof) < fabsf(fDistToFloor))
                                {
                                    // being above
                                    fZDistSq = fZRoofSq;

                                    if (!m_bObstructRoof)
                                    {
                                        // perpendicular point to Roof
                                        fXDistSq = 0.0f;
                                    }
                                }
                                else
                                {
                                    // being below
                                    fZDistSq = fZFloorSq;

                                    if (!m_bObstructFloor)
                                    {
                                        // perpendicular point to Floor
                                        fXDistSq = 0.0f;
                                    }
                                }
                            }
                        }
                        else
                        {
                            // outside of 2D Shape, so diagonal or only to the side
                            if ((Point3d.z < m_VOrigin + m_VSize && Point3d.z > m_VOrigin))
                            {
                                // Point is inside z-boundary
                            }
                            else
                            {
                                // point is above or below area
                                if (fabsf(fDistToRoof) < fabsf(fDistToFloor))
                                {
                                    // being above
                                    fZDistSq = fZRoofSq;
                                }
                                else
                                {
                                    // being below
                                    fZDistSq = fZFloorSq;
                                }
                            }
                        }
                    }
                    else
                    {
                        // infinite high area
                        // Closest is on an edge, ZDistance is 0, so nothing really to do here
                    }

                    fClosestDistance = fXDistSq;

                    if (!bIgnoreHeight)
                    {
                        fClosestDistance += fZDistSq;
                    }
                }

                break;
            }
            case ENTITY_AREA_TYPE_SPHERE:
            {
                Vec3 vTemp(Point3d);

                if (bIgnoreHeight)
                {
                    vTemp.z = m_SphereCenter.z;
                }

                float const fLength = m_SphereCenter.GetDistance(vTemp) - m_SphereRadius;
                fClosestDistance = fLength * fLength;

                break;
            }
            case ENTITY_AREA_TYPE_BOX:
            {
                if (!bIgnoreSoundObstruction)
                {
                    Vec3 oTemp(ZERO);
                    CalcClosestPointToObstructedBox(oTemp, fClosestDistance, Point3d);
                }
                else
                {
                    Vec3 p3d = m_InvMatrix.TransformPoint(Point3d);
                    Vec3 OnHull3d;

                    if (bIgnoreHeight)
                    {
                        p3d.z = m_BoxMin.z;
                    }

                    AABB myAABB(m_BoxMin, m_BoxMax);

                    fClosestDistance = Distance::Point_AABBSq(p3d, myAABB, OnHull3d);

                    if (m_abBoxSideObstruction[4].bObstructed && OnHull3d.z == myAABB.max.z)
                    {
                        // Point is on the roof plane, but may be on the edge already
                        Vec2 vTop(OnHull3d.x, myAABB.max.y);
                        Vec2 vLeft(myAABB.min.x, OnHull3d.y);
                        Vec2 vLow(OnHull3d.x, myAABB.min.y);
                        Vec2 vRight(myAABB.max.x, OnHull3d.y);

                        float fDistanceToTop        = p3d.GetSquaredDistance2D(vTop);
                        float fDistanceToLeft       = p3d.GetSquaredDistance2D(vLeft);
                        float fDistanceToLow        = p3d.GetSquaredDistance2D(vLow);
                        float fDistanceToRight  = p3d.GetSquaredDistance2D(vRight);
                        float fTempMinDistance = fDistanceToTop;

                        OnHull3d = vTop;

                        if (fDistanceToLeft < fTempMinDistance)
                        {
                            OnHull3d = vLeft;
                            fTempMinDistance = fDistanceToLeft;
                        }

                        if (fDistanceToLow < fTempMinDistance)
                        {
                            OnHull3d = vLow;
                            fTempMinDistance = fDistanceToLow;
                        }

                        if (fDistanceToRight < fTempMinDistance)
                        {
                            OnHull3d = vRight;
                            fTempMinDistance = fDistanceToRight;
                        }

                        OnHull3d.z = min(myAABB.max.z, p3d.z);
                        fClosestDistance = OnHull3d.GetSquaredDistance(p3d);
                    }

                    if (m_abBoxSideObstruction[5].bObstructed && OnHull3d.z == myAABB.min.z)
                    {
                        // Point is on the floor plane, but may be on the edge already
                        Vec2 vTop(OnHull3d.x, myAABB.max.y);
                        Vec2 vLeft(myAABB.min.x, OnHull3d.y);
                        Vec2 vLow(OnHull3d.x, myAABB.min.y);
                        Vec2 vRight(myAABB.max.x, OnHull3d.y);

                        float fDistanceToTop        = p3d.GetSquaredDistance2D(vTop);
                        float fDistanceToLeft       = p3d.GetSquaredDistance2D(vLeft);
                        float fDistanceToLow        = p3d.GetSquaredDistance2D(vLow);
                        float fDistanceToRight  = p3d.GetSquaredDistance2D(vRight);
                        float fTempMinDistance = fDistanceToTop;

                        OnHull3d = vTop;

                        if (fDistanceToLeft < fTempMinDistance)
                        {
                            OnHull3d = vLeft;
                            fTempMinDistance = fDistanceToLeft;
                        }

                        if (fDistanceToLow < fTempMinDistance)
                        {
                            OnHull3d = vLow;
                            fTempMinDistance = fDistanceToLow;
                        }

                        if (fDistanceToRight < fTempMinDistance)
                        {
                            OnHull3d = vRight;
                            fTempMinDistance = fDistanceToRight;
                        }

                        OnHull3d.z = max(myAABB.min.z, p3d.z);
                        fClosestDistance = OnHull3d.GetSquaredDistance(p3d);
                    }
                }

                // TODO transform point back to world ... (why?)
                break;
            }
            default:
            {
                CryFatalError("Unknown area type during CArea::CalcPointNearDistSq (2)");

                break;
            }
            }

            // Put the shortest distance into the data cache
            if (pCachedData != NULL && bCacheResult)
            {
                if (bIgnoreSoundObstruction)
                {
                    if (bIgnoreHeight)
                    {
                        pCachedData->eFlags = (ECachedAreaData)(pCachedData->eFlags | eCachedAreaData_DistNearSqValid | eCachedAreaData_DistNearSqValidNotObstructed | eCachedAreaData_DistNearSqValidHeightIgnored);
                    }
                    else
                    {
                        pCachedData->eFlags = (ECachedAreaData)(pCachedData->eFlags | eCachedAreaData_DistNearSqValid | eCachedAreaData_DistNearSqValidNotObstructed);
                    }
                }
                else
                {
                    if (bIgnoreHeight)
                    {
                        pCachedData->eFlags = (ECachedAreaData)(pCachedData->eFlags | eCachedAreaData_DistNearSqValid | eCachedAreaData_DistNearSqValidHeightIgnored);
                    }
                    else
                    {
                        pCachedData->eFlags = (ECachedAreaData)(pCachedData->eFlags | eCachedAreaData_DistNearSqValid);
                    }
                }

                pCachedData->fDistanceNearSq = fClosestDistance;
            }
        }
    }

    return fClosestDistance;
}

//////////////////////////////////////////////////////////////////////////
EAreaPosType CArea::CalcPosType(EntityId const nEntityID, Vec3 const& rPos, bool const bCacheResult /* = true */)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);
    EAreaPosType eTempPosType   = AREA_POS_TYPE_COUNT;

    if (m_bInitialized)
    {
        SCachedAreaData* pCachedData = NULL;

        if (nEntityID != INVALID_ENTITYID)
        {
            TEntityCachedAreaDataMap::iterator Iter(m_mapEntityCachedAreaData.find(nEntityID));

            if (Iter != m_mapEntityCachedAreaData.end())
            {
                pCachedData = &(Iter->second);
            }
        }

        if (pCachedData != NULL && (pCachedData->eFlags & eCachedAreaData_PosTypeValid) != 0)
        {
            eTempPosType = GetCachedPointPosTypeWithin(nEntityID);
        }
        else
        {
            bool const bIsIn2DShape = CalcPointWithin(nEntityID, rPos, true, false);

            switch (m_AreaType)
            {
            case ENTITY_AREA_TYPE_SOLID:
            {
                Vec3 const ov3PosLocalSpace(m_InvMatrix.TransformPoint(rPos));
                AABB const boundbox(m_AreaSolid->GetBoundBox());

                if (ov3PosLocalSpace.z < boundbox.min.z)
                {
                    eTempPosType = AREA_POS_TYPE_2DINSIDE_ZBELOW;
                }
                else if (ov3PosLocalSpace.z > boundbox.max.z)
                {
                    eTempPosType = AREA_POS_TYPE_2DINSIDE_ZABOVE;
                }
                else
                {
                    if (bIsIn2DShape)
                    {
                        eTempPosType = AREA_POS_TYPE_2DINSIDE_ZINSIDE;
                    }
                }

                break;
            }
            case ENTITY_AREA_TYPE_SHAPE:
            {
                if (m_VSize > 0.0f)
                {
                    // Negative means from inside to hull
                    float const fDistToRoof  = rPos.z - (m_VOrigin + m_VSize);
                    float const fDistToFloor = m_VOrigin - rPos.z;

                    if (bIsIn2DShape)
                    {
                        // Point is below, in, or above the area
                        if ((rPos.z < m_VOrigin + m_VSize && rPos.z > m_VOrigin))
                        {
                            eTempPosType = AREA_POS_TYPE_2DINSIDE_ZINSIDE;
                        }
                        else
                        {
                            // Point is above or below area
                            if (fabsf(fDistToRoof) < fabsf(fDistToFloor))
                            {
                                eTempPosType = AREA_POS_TYPE_2DINSIDE_ZABOVE;
                            }
                            else
                            {
                                eTempPosType = AREA_POS_TYPE_2DINSIDE_ZBELOW;
                            }
                        }
                    }
                    else
                    {
                        // Outside of 2D Shape, so diagonal or only to the side
                        if ((rPos.z > m_VOrigin + m_VSize || rPos.z < m_VOrigin))
                        {
                            // Point is outside z-boundary
                            // point is above or below area
                            if (fabsf(fDistToRoof) < fabsf(fDistToFloor))
                            {
                                eTempPosType = AREA_POS_TYPE_2DOUTSIDE_ZABOVE;
                            }
                            else
                            {
                                eTempPosType = AREA_POS_TYPE_2DOUTSIDE_ZBELOW;
                            }
                        }
                        else
                        {
                            eTempPosType = AREA_POS_TYPE_2DOUTSIDE_ZINSIDE;
                        }
                    }
                }
                else
                {
                    // No height means infinite Z boundaries (no roof, no floor)
                    if (bIsIn2DShape)
                    {
                        eTempPosType = AREA_POS_TYPE_2DINSIDE_ZINSIDE;
                    }
                    else
                    {
                        eTempPosType = AREA_POS_TYPE_2DOUTSIDE_ZINSIDE;
                    }
                }

                break;
            }
            case ENTITY_AREA_TYPE_BOX:
            {
                if (m_BoxMax.z > 0.0f)
                {
                    Vec3 const ov3PosLocalSpace(m_InvMatrix.TransformPoint(rPos));
                    if (bIsIn2DShape)
                    {
                        if (ov3PosLocalSpace.z < m_BoxMin.z)        // Warning : Fix for boxes not centered around the base of an entity. Please check before back integrating.
                        {
                            eTempPosType = AREA_POS_TYPE_2DINSIDE_ZBELOW;
                        }
                        else if (ov3PosLocalSpace.z > m_BoxMax.z)
                        {
                            eTempPosType = AREA_POS_TYPE_2DINSIDE_ZABOVE;
                        }
                        else
                        {
                            eTempPosType = AREA_POS_TYPE_2DINSIDE_ZINSIDE;
                        }
                    }
                    else
                    {
                        if (ov3PosLocalSpace.z < m_BoxMin.z)        // Warning : Fix for boxes not centered around the base of an entity. Please check before back integrating.
                        {
                            eTempPosType = AREA_POS_TYPE_2DOUTSIDE_ZBELOW;
                        }
                        else if (ov3PosLocalSpace.z > m_BoxMax.z)
                        {
                            eTempPosType = AREA_POS_TYPE_2DOUTSIDE_ZABOVE;
                        }
                        else
                        {
                            eTempPosType = AREA_POS_TYPE_2DOUTSIDE_ZINSIDE;
                        }
                    }
                }
                else
                {
                    // No height means infinite Z boundaries (no roof, no floor)
                    if (bIsIn2DShape)
                    {
                        eTempPosType = AREA_POS_TYPE_2DINSIDE_ZINSIDE;
                    }
                    else
                    {
                        eTempPosType = AREA_POS_TYPE_2DOUTSIDE_ZINSIDE;
                    }
                }

                break;
            }
            case ENTITY_AREA_TYPE_SPHERE:
            {
                float const fSphereMostTop      = m_SphereCenter.z + m_SphereRadius;
                float const fSphereMostBottom   = m_SphereCenter.z - m_SphereRadius;
                bool const bAbove                           = rPos.z > fSphereMostTop;
                bool const bBelow                           = rPos.z < fSphereMostBottom;
                bool const bIsIn3DShape             = CalcPointWithin(nEntityID, rPos, false);

                if (bIsIn3DShape)
                {
                    eTempPosType = AREA_POS_TYPE_2DINSIDE_ZINSIDE;
                }
                else
                {
                    if (bIsIn2DShape)
                    {
                        // We're inside of the sphere 2D silhouette but not inside the sphere itself
                        // now check if we're above its max z or below its min z
                        if (bAbove)
                        {
                            eTempPosType = AREA_POS_TYPE_2DINSIDE_ZABOVE;
                        }

                        if (bBelow)
                        {
                            eTempPosType = AREA_POS_TYPE_2DINSIDE_ZBELOW;
                        }
                    }
                    else
                    {
                        // We're outside of the sphere 2D silhouette and outside of the sphere itself
                        // now check if we're above its max z or below its min z
                        eTempPosType = AREA_POS_TYPE_2DOUTSIDE_ZINSIDE;

                        if (bAbove)
                        {
                            eTempPosType = AREA_POS_TYPE_2DOUTSIDE_ZABOVE;
                        }

                        if (bBelow)
                        {
                            eTempPosType = AREA_POS_TYPE_2DOUTSIDE_ZBELOW;
                        }
                    }
                }

                break;
            }
            default:
            {
                CryFatalError("Unknown area type during CArea::CalcPosType");

                break;
            }
            }

            if (pCachedData != NULL && bCacheResult)
            {
                pCachedData->eFlags     = (ECachedAreaData)(pCachedData->eFlags | eCachedAreaData_PosTypeValid);
                pCachedData->ePosType   = eTempPosType;
            }
        }
    }

    return eTempPosType;
}

//////////////////////////////////////////////////////////////////////////
void CArea::CalcClosestPointToObstructedShape(EntityId const nEntityID, Vec3& outClosest, float& outClosestDistSq, Vec3 const& sourcePos)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);
    size_t const nSegmentCount = m_vpSegments.size();
    Lineseg oLine;
    Vec3 closest(ZERO);
    float closestDistSq(FLT_MAX);

    // If this area got a height
    if (m_VSize)
    {
        EAreaPosType const ePosType = CalcPosType(nEntityID, sourcePos);
        float const fRoofWorldPosZ = m_VOrigin + m_VSize;

        // Find the closest point
        // First of all check if we're either right above a non-obstructing roof or below a non-obstructing floor
        // if so, just use this data since we have the shortest distance right there
        if (ePosType == AREA_POS_TYPE_2DINSIDE_ZABOVE && !m_bObstructRoof)
        {
            closestDistSq = (sourcePos.z - fRoofWorldPosZ) * (sourcePos.z - fRoofWorldPosZ);
            closest.x   = sourcePos.x;
            closest.y   = sourcePos.y;
            closest.z   = fRoofWorldPosZ;
        }
        else if (ePosType == AREA_POS_TYPE_2DINSIDE_ZBELOW && !m_bObstructFloor)
        {
            closestDistSq = (m_VOrigin - sourcePos.z) * (m_VOrigin - sourcePos.z);
            closest.x   = sourcePos.x;
            closest.y   = sourcePos.y;
            closest.z   = m_VOrigin;
        }
        else
        {
            PrefetchLine(&m_vpSegments[0], 0);

            for (size_t nIdx = 0; nIdx < nSegmentCount; ++nIdx)
            {
                a2DSegment const* const curSg = m_vpSegments[nIdx];
                float const fCurrSegStart[2]  = {curSg->GetStart().x, curSg->GetStart().y};
                float const fCurrSegEnd[2]    = {curSg->GetEnd().x, curSg->GetEnd().y};

                // If the segment is not obstructed get the closest point to it
                if (!curSg->bObstructSound)
                {
                    float fPosZ         = sourcePos.z;
                    bool bAdjusted  = false;

                    // Adjust Z position if we're outside the boundaries
                    if (fPosZ > fRoofWorldPosZ)
                    {
                        fPosZ     = fRoofWorldPosZ;
                        bAdjusted = true;
                    }
                    else if (fPosZ < m_VOrigin)
                    {
                        fPosZ     = m_VOrigin;
                        bAdjusted = true;
                    }

                    oLine.start = Vec3(fCurrSegStart[0], fCurrSegStart[1], fPosZ);
                    oLine.end   = Vec3(fCurrSegEnd[0], fCurrSegEnd[1], fPosZ);

                    // If we're outside the Z boundaries we need to include Z on our test
                    float fT;
                    float const fTempDistToLineSq = bAdjusted ?
                        Distance::Point_LinesegSq(sourcePos, oLine, fT) :
                        Distance::Point_Lineseg2DSq(sourcePos, oLine, fT);

                    if (fTempDistToLineSq < closestDistSq)
                    {
                        closestDistSq = fTempDistToLineSq;
                        closest       = oLine.GetPoint(fT);
                    }
                }
                else
                {
                    // Otherwise we need to check the roof and the floor if we're not inside the area
                    if (ePosType != AREA_POS_TYPE_2DINSIDE_ZINSIDE)
                    {
                        // Roof
                        if (!m_bObstructRoof)
                        {
                            oLine.start = Vec3(fCurrSegStart[0], fCurrSegStart[1], fRoofWorldPosZ);
                            oLine.end   = Vec3(fCurrSegEnd[0], fCurrSegEnd[1], fRoofWorldPosZ);
                            float fT;
                            float const fTempDistToLineSq = Distance::Point_LinesegSq(sourcePos, oLine, fT);

                            if (fTempDistToLineSq < closestDistSq)
                            {
                                closestDistSq = fTempDistToLineSq;
                                closest       = oLine.GetPoint(fT);
                            }
                        }

                        // Floor
                        if (!m_bObstructFloor)
                        {
                            oLine.start = Vec3(fCurrSegStart[0], fCurrSegStart[1], m_VOrigin);
                            oLine.end   = Vec3(fCurrSegEnd[0], fCurrSegEnd[1], m_VOrigin);

                            float fT;
                            float const fTempDistToLineSq = Distance::Point_LinesegSq(sourcePos, oLine, fT);

                            if (fTempDistToLineSq < closestDistSq)
                            {
                                closestDistSq = fTempDistToLineSq;
                                closest       = oLine.GetPoint(fT);
                            }
                        }
                    }
                }
            }
        }

        // If we're inside an area we always need to check a non-obstructing roof and floor
        // this needs to be done only once right after the loop
        if (ePosType == AREA_POS_TYPE_2DINSIDE_ZINSIDE)
        {
            // Roof
            if (!m_bObstructRoof)
            {
                float const fTempDistToLineSq = (sourcePos.z - fRoofWorldPosZ) * (sourcePos.z - fRoofWorldPosZ);

                if (fTempDistToLineSq < closestDistSq)
                {
                    closestDistSq = fTempDistToLineSq;
                    closest.x   = sourcePos.x;
                    closest.y   = sourcePos.y;
                    closest.z   = fRoofWorldPosZ;
                }
            }

            // Floor
            if (!m_bObstructFloor)
            {
                float const fTempDistToLineSq = (m_VOrigin - sourcePos.z) * (m_VOrigin - sourcePos.z);

                if (fTempDistToLineSq < closestDistSq)
                {
                    closestDistSq = fTempDistToLineSq;
                    closest.x   = sourcePos.x;
                    closest.y   = sourcePos.y;
                    closest.z   = m_VOrigin;
                }
            }
        }
    }
    else // This area has got infinite height
    {
        // Find the closest distance to non-obstructing segments, at source pos height
        for (size_t nIdx = 0; nIdx < nSegmentCount; ++nIdx)
        {
            a2DSegment const* const curSg = m_vpSegments[nIdx];
            float const fCurrSegStart[2]  = {curSg->GetStart().x, curSg->GetStart().y};
            float const fCurrSegEnd[2]    = {curSg->GetEnd().x, curSg->GetEnd().y};

            // If the segment is not obstructed get the closest point and continue
            if (!curSg->bObstructSound)
            {
                oLine.start = Vec3(fCurrSegStart[0], fCurrSegStart[1], sourcePos.z);
                oLine.end   = Vec3(fCurrSegEnd[0], fCurrSegEnd[1], sourcePos.z);

                float fT;
                float const fTempDistToLineSq   = Distance::Point_Lineseg2DSq(sourcePos, oLine, fT); // Ignore Z

                if (fTempDistToLineSq < closestDistSq)
                {
                    closestDistSq = fTempDistToLineSq;
                    closest       = oLine.GetPoint(fT);
                }
            }
        }
    }

    // If there was no calculation, most likely all sides + roof + floor are obstructed, return the source position
    if (closest.IsZeroFast() && closestDistSq == FLT_MAX)
    {
        closest = sourcePos;
    }

    outClosest = closest;
    outClosestDistSq = closestDistSq;
}

//////////////////////////////////////////////////////////////////////////
void CArea::CalcClosestPointToObstructedBox(Vec3& outClosest, float& outClosestDistSq, Vec3 const& sourcePos) const
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);
    Vec3 source(sourcePos);
    Vec3 closest(sourcePos);
    outClosestDistSq = FLT_MAX;

    if (!m_bAllObstructed)
    {
        Vec3 const v3PosLocalSpace(m_InvMatrix.TransformPoint(source)), v3BoxMin = m_BoxMin, v3BoxMax = m_BoxMax;

        // Determine the closest side and check for obstruction
        int nClosestSideIdx               = -1;
        bool bAllObstructedExceptBackside = false;
        Vec3 aoBoxBackSideEdges[4];

        if (v3PosLocalSpace.x > v3BoxMax.x)
        {
            // Side 2 is closest, 4 is backside
            nClosestSideIdx = 1;

            if (m_nObstructedCount == 5 && !m_abBoxSideObstruction[3].bObstructed)
            {
                // Describe possible snapping positions on back side 4
                aoBoxBackSideEdges[0] = Vec3(v3BoxMin.x, v3BoxMin.y, v3PosLocalSpace.z);
                aoBoxBackSideEdges[1] = Vec3(v3BoxMin.x, v3PosLocalSpace.y, v3BoxMax.z);
                aoBoxBackSideEdges[2] = Vec3(v3BoxMin.x, v3BoxMax.y, v3PosLocalSpace.z);
                aoBoxBackSideEdges[3] = Vec3(v3BoxMin.x, v3PosLocalSpace.y, v3BoxMin.z);

                bAllObstructedExceptBackside = true;
            }
        }
        else if (v3PosLocalSpace.x < v3BoxMin.x)
        {
            // Side 4 is closest, 2 is backside
            nClosestSideIdx = 3;

            if (m_nObstructedCount == 5 && !m_abBoxSideObstruction[1].bObstructed)
            {
                // Describe possible snapping positions on back side 2
                aoBoxBackSideEdges[0] = Vec3(v3BoxMax.x, v3BoxMax.y, v3PosLocalSpace.z);
                aoBoxBackSideEdges[1] = Vec3(v3BoxMax.x, v3PosLocalSpace.y, v3BoxMax.z);
                aoBoxBackSideEdges[2] = Vec3(v3BoxMax.x, v3BoxMin.y, v3PosLocalSpace.z);
                aoBoxBackSideEdges[3] = Vec3(v3BoxMax.x, v3PosLocalSpace.y, v3BoxMin.z);

                bAllObstructedExceptBackside = true;
            }
        }
        else
        {
            if (v3PosLocalSpace.y < v3BoxMin.y)
            {
                // Side 0 is closest, 2 is backside
                nClosestSideIdx = 0;

                if (m_nObstructedCount == 5 && !m_abBoxSideObstruction[2].bObstructed)
                {
                    // Describe possible snapping positions on back side 2
                    aoBoxBackSideEdges[0] = Vec3(v3BoxMin.x, v3BoxMax.y, v3PosLocalSpace.z);
                    aoBoxBackSideEdges[1] = Vec3(v3PosLocalSpace.x, v3BoxMax.y, v3BoxMax.z);
                    aoBoxBackSideEdges[2] = Vec3(v3BoxMax.x, v3BoxMax.y, v3PosLocalSpace.z);
                    aoBoxBackSideEdges[3] = Vec3(v3PosLocalSpace.x, v3BoxMax.y, v3BoxMin.z);

                    bAllObstructedExceptBackside = true;
                }
            }
            else if (v3PosLocalSpace.y > v3BoxMax.y)
            {
                // Side 2 is closest, 0 is backside
                nClosestSideIdx = 2;

                if (m_nObstructedCount == 5 && !m_abBoxSideObstruction[0].bObstructed)
                {
                    // Describe possible snapping positions on back side 0
                    aoBoxBackSideEdges[0] = Vec3(v3BoxMax.x, v3BoxMin.y, v3PosLocalSpace.z);
                    aoBoxBackSideEdges[1] = Vec3(v3PosLocalSpace.x, v3BoxMin.y, v3BoxMax.z);
                    aoBoxBackSideEdges[2] = Vec3(v3BoxMin.x, v3BoxMin.y, v3PosLocalSpace.z);
                    aoBoxBackSideEdges[3] = Vec3(v3PosLocalSpace.x, v3BoxMin.y, v3BoxMin.z);

                    bAllObstructedExceptBackside = true;
                }
            }
            else
            {
                if (v3PosLocalSpace.z < v3BoxMin.z)
                {
                    // Side 5 is closest, 4 is backside
                    nClosestSideIdx = 5;

                    if (m_nObstructedCount == 5 && !m_abBoxSideObstruction[4].bObstructed)
                    {
                        // Describe possible snapping positions on back side 4
                        aoBoxBackSideEdges[0] = Vec3(v3PosLocalSpace.x, v3BoxMin.y, v3BoxMax.z);
                        aoBoxBackSideEdges[1] = Vec3(v3BoxMin.x, v3PosLocalSpace.y, v3BoxMax.z);
                        aoBoxBackSideEdges[2] = Vec3(v3PosLocalSpace.x, v3BoxMax.y, v3BoxMax.z);
                        aoBoxBackSideEdges[3] = Vec3(v3BoxMax.x, v3PosLocalSpace.y, v3BoxMax.z);

                        bAllObstructedExceptBackside = true;
                    }
                }
                else if (v3PosLocalSpace.z > v3BoxMax.z)
                {
                    // Side 4 is closest, 5 is backside
                    nClosestSideIdx = 4;

                    if (m_nObstructedCount == 5 && !m_abBoxSideObstruction[5].bObstructed)
                    {
                        // Describe possible snapping positions on back side 5
                        aoBoxBackSideEdges[0] = Vec3(v3PosLocalSpace.x, v3BoxMin.y, v3BoxMin.z);
                        aoBoxBackSideEdges[1] = Vec3(v3BoxMin.x, v3PosLocalSpace.y, v3BoxMin.z);
                        aoBoxBackSideEdges[2] = Vec3(v3PosLocalSpace.x, v3BoxMax.y, v3BoxMin.z);
                        aoBoxBackSideEdges[3] = Vec3(v3BoxMax.x, v3PosLocalSpace.y, v3BoxMin.z);

                        bAllObstructedExceptBackside = true;
                    }
                }
                else
                {
                    // We're inside the box
                    float const fDistSide[6] =
                    {
                        abs(m_abBoxSideObstruction[0].bObstructed ? FLT_MAX : v3BoxMin.y - v3PosLocalSpace.y),
                        abs(m_abBoxSideObstruction[1].bObstructed ? FLT_MAX : v3BoxMax.x - v3PosLocalSpace.x),
                        abs(m_abBoxSideObstruction[2].bObstructed ? FLT_MAX : v3BoxMax.y - v3PosLocalSpace.y),
                        abs(m_abBoxSideObstruction[3].bObstructed ? FLT_MAX : v3BoxMin.x - v3PosLocalSpace.x),
                        abs(m_abBoxSideObstruction[4].bObstructed ? FLT_MAX : v3BoxMax.z - v3PosLocalSpace.z),
                        abs(m_abBoxSideObstruction[5].bObstructed ? FLT_MAX : v3BoxMin.z - v3PosLocalSpace.z)
                    };

                    float const fClosestDist = min(min(min(fDistSide[0], fDistSide[1]), min(fDistSide[2], fDistSide[3])), min(fDistSide[4], fDistSide[5]));
                    for (unsigned int i = 0; i < 6; ++i)
                    {
                        if (fDistSide[i] == fClosestDist)
                        {
                            switch (i)
                            {
                            case 0:
                                closest = Vec3(v3PosLocalSpace.x, v3PosLocalSpace.y - fClosestDist, v3PosLocalSpace.z);
                                break;
                            case 1:
                                closest = Vec3(v3PosLocalSpace.x + fClosestDist, v3PosLocalSpace.y, v3PosLocalSpace.z);
                                break;
                            case 2:
                                closest = Vec3(v3PosLocalSpace.x, v3PosLocalSpace.y + fClosestDist, v3PosLocalSpace.z);
                                break;
                            case 3:
                                closest = Vec3(v3PosLocalSpace.x - fClosestDist, v3PosLocalSpace.y, v3PosLocalSpace.z);
                                break;
                            case 4:
                                closest = Vec3(v3PosLocalSpace.x, v3PosLocalSpace.y, v3PosLocalSpace.z + fClosestDist);
                                break;
                            case 5:
                                closest = Vec3(v3PosLocalSpace.x, v3PosLocalSpace.y, v3PosLocalSpace.z - fClosestDist);
                                break;
                            }

                            // Transform it all back to world coordinates
                            outClosest              = m_WorldTM.TransformPoint(closest);
                            outClosestDistSq    = Vec3(source - outClosest).GetLengthSquared();
                            return;
                        }
                    }
                }
            }
        }

        // We're done determining the us facing side and we're not inside the box,
        // now check if the side not obstructed and return the closest position on it
        if (nClosestSideIdx > -1 && !m_abBoxSideObstruction[nClosestSideIdx].bObstructed)
        {
            // The shortest side is not obstructed
            closest.x = min(max(v3PosLocalSpace.x, v3BoxMin.x), v3BoxMax.x);
            closest.y = min(max(v3PosLocalSpace.y, v3BoxMin.y), v3BoxMax.y);
            closest.z = min(max(v3PosLocalSpace.z, v3BoxMin.z), v3BoxMax.z);

            // Transform the result back to world values
            outClosest              = m_WorldTM.TransformPoint(closest);
            outClosestDistSq    = Vec3(source - outClosest).GetLengthSquared();
            return;
        }

        // If we get here the closest side was obstructed
        // Now describe the 6 sides by applying min and max coordinate values
        SBoxSide const aoBoxSides[6] =
        {
            SBoxSide(v3BoxMin, Vec3(v3BoxMax.x, v3BoxMin.y, v3BoxMax.z)),
            SBoxSide(Vec3(v3BoxMax.x, v3BoxMin.y, v3BoxMin.z), v3BoxMax),
            SBoxSide(Vec3(v3BoxMin.x, v3BoxMax.y, v3BoxMin.z), v3BoxMax),
            SBoxSide(v3BoxMin, Vec3(v3BoxMin.x, v3BoxMax.y, v3BoxMax.z)),
            SBoxSide(Vec3(v3BoxMin.x, v3BoxMin.y, v3BoxMax.z), v3BoxMax),
            SBoxSide(v3BoxMin, Vec3(v3BoxMax.x, v3BoxMax.y, v3BoxMin.z))
        };

        // Check all non-obstructing sides, get the closest position on them
        float fClosestDist = FLT_MAX;
        Vec3 v3ClosestPoint(ZERO);

        for (unsigned int i = 0; i < 6; ++i)
        {
            if (!m_abBoxSideObstruction[i].bObstructed)
            {
                if (bAllObstructedExceptBackside)
                {
                    // At least 2 axis must be within boundaries, which means we're facing directly a side and snapping is necessary
                    bool const bWithinX = v3PosLocalSpace.x > v3BoxMin.x && v3PosLocalSpace.x < v3BoxMax.x;
                    bool const bWithinY = v3PosLocalSpace.y > v3BoxMin.y && v3PosLocalSpace.y < v3BoxMax.y;
                    bool const bWithinZ = v3PosLocalSpace.z > v3BoxMin.z && v3PosLocalSpace.z < v3BoxMax.z;

                    if (bWithinX && (bWithinY || bWithinZ) ||
                        bWithinY && (bWithinX || bWithinZ) ||
                        bWithinZ && (bWithinX || bWithinY))
                    {
                        // Get the distance to the edges and choose the shortest one
                        float const fDistToEdges[4] =
                        {
                            Vec3(v3PosLocalSpace - aoBoxBackSideEdges[0]).GetLengthSquared(),
                            Vec3(v3PosLocalSpace - aoBoxBackSideEdges[1]).GetLengthSquared(),
                            Vec3(v3PosLocalSpace - aoBoxBackSideEdges[2]).GetLengthSquared(),
                            Vec3(v3PosLocalSpace - aoBoxBackSideEdges[3]).GetLengthSquared()
                        };

                        float const fClosestDistToEdges = min(min(fDistToEdges[0], fDistToEdges[1]), min(fDistToEdges[2], fDistToEdges[3]));
                        for (unsigned int j = 0; j < 4; ++j)
                        {
                            if (fDistToEdges[j] == fClosestDistToEdges)
                            {
                                // Snap to it
                                closest = aoBoxBackSideEdges[j];
                                break;
                            }
                        }

                        break;
                    }
                }

                v3ClosestPoint.x = min(max(v3PosLocalSpace.x, aoBoxSides[i].v3MinValues.x), aoBoxSides[i].v3MaxValues.x);
                v3ClosestPoint.y = min(max(v3PosLocalSpace.y, aoBoxSides[i].v3MinValues.y), aoBoxSides[i].v3MaxValues.y);
                v3ClosestPoint.z = min(max(v3PosLocalSpace.z, aoBoxSides[i].v3MinValues.z), aoBoxSides[i].v3MaxValues.z);

                float const fTemp = Vec3(v3PosLocalSpace - v3ClosestPoint).GetLengthSquared();
                if (fTemp < fClosestDist)
                {
                    fClosestDist    = fTemp;
                    closest = v3ClosestPoint;
                }
            }
        }

        // Transform the result back to world values
        outClosest              = m_WorldTM.TransformPoint(closest);
        outClosestDistSq    = Vec3(source - outClosest).GetLengthSquared();
    }
    else
    {
        outClosest = sourcePos;
    }
}

//////////////////////////////////////////////////////////////////////////
void CArea::CalcClosestPointToSolid(Vec3 const& rv3SourcePos, bool bIgnoreSoundObstruction, float& rfClosestDistSq, Vec3* rv3ClosestPos) const
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);
    Vec3 localPoint3D = m_InvMatrix.TransformPoint(rv3SourcePos);
    int queryFlag = bIgnoreSoundObstruction ? CAreaSolid::eSegmentQueryFlag_All : CAreaSolid::eSegmentQueryFlag_Open;
    if (m_AreaSolid->IsInside(localPoint3D))
    {
        queryFlag |= CAreaSolid::eSegmentQueryFlag_UsingReverseSegment;
    }
    Vec3 closestPos(0, 0, 0);
    m_AreaSolid->QueryNearest(localPoint3D, queryFlag, closestPos, rfClosestDistSq);
    if (rv3ClosestPos)
    {
        *rv3ClosestPos = m_WorldTM.TransformPoint(closestPos);
    }
}

//////////////////////////////////////////////////////////////////////////
void CArea::InvalidateCachedAreaData(EntityId const nEntityID)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);
    TEntityCachedAreaDataMap::iterator Iter(m_mapEntityCachedAreaData.find(nEntityID));

    if (Iter != m_mapEntityCachedAreaData.end())
    {
        SCachedAreaData& rCachedData = Iter->second;
        rCachedData.eFlags            = eCachedAreaData_None;
        rCachedData.ePosType          = AREA_POS_TYPE_COUNT;
        rCachedData.fDistanceWithinSq = FLT_MAX;
        rCachedData.fDistanceNearSq   = FLT_MAX;
        rCachedData.bPointWithin      = false;
        rCachedData.oPos.zero();
    }
}

//////////////////////////////////////////////////////////////////////////
void CArea::SetPoints(const Vec3* const vPoints, const bool* const pabSoundObstructionSegments, const int nPointsCount)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);
    ReleaseAreaData();

    m_AreaType = ENTITY_AREA_TYPE_SHAPE;
    // at least three points needed to create closed shape
    if (nPointsCount > 2)
    {
        m_bInitialized = true;
        float minZ = 10000000.0f;
        //////////////////////////////////////////////////////////////////////////
        for (int i = 0; i < nPointsCount; ++i)
        {
            if (vPoints[i].z < minZ)
            {
                minZ = vPoints[i].z;
            }
        }
        m_VOrigin = minZ;

        int pIdx;
        for (pIdx = 1; pIdx < nPointsCount; ++pIdx)
        {
            AddSegment(*((CArea::a2DPoint*)(vPoints + pIdx - 1)), *((CArea::a2DPoint*)(vPoints + pIdx)), *(pabSoundObstructionSegments + pIdx - 1));
        }
        AddSegment(*((CArea::a2DPoint*)(vPoints + pIdx - 1)), *((CArea::a2DPoint*)(vPoints)), *(pabSoundObstructionSegments + pIdx - 1));
        CalcBBox();
    }

    m_pAreaManager->SetAreaDirty(this);
}

//////////////////////////////////////////////////////////////////////////
void CArea::SetBox(const Vec3& min, const Vec3& max, const Matrix34& tm)
{
    ReleaseAreaData();

    m_AreaType          = ENTITY_AREA_TYPE_BOX;
    m_bInitialized  = true;
    m_BoxMin                = min;
    m_BoxMax                = max;
    m_InvMatrix         = tm.GetInverted();


    /*
        uint32 nErrorCode=0;
        uint32 minValid = min.IsValid();
        if (minValid==0) nErrorCode|=0x8000;
        uint32 maxValid = max.IsValid();
        if (maxValid==0) nErrorCode|=0x8000;

        if (max.x < min.x) nErrorCode|=0x0001;
        if (max.y < min.y) nErrorCode|=0x0001;
        if (max.z < min.z) nErrorCode|=0x0001;
        if (min.x < -8000) nErrorCode|=0x0002;
        if (min.y < -8000) nErrorCode|=0x0004;
        if (min.z < -8000) nErrorCode|=0x0008;
        if (max.x > +8000) nErrorCode|=0x0010;
        if (max.y > +8000) nErrorCode|=0x0020;
        if (max.z > +8000) nErrorCode|=0x0040;
        assert(nErrorCode==0);

        if (nErrorCode)
        {
            CryFatalError("Fatal Error: BBox in EntitySystem is out of range");
            //AnimWarning("CryAnimation: Invalid BBox (%.3f,%.3f,%.3f)-(%.3f,%.3f,%.3f). ModenPath: '%s'  ErrorCode: %08x",     m_AABB.min.x, m_AABB.min.y, m_AABB.min.z, m_AABB.max.x, m_AABB.max.y, m_AABB.max.z,     m_pInstance->m_pModel->GetFilePathCStr(), nErrorCode);
            assert(0);
        }
        //if (rAnim.m_nEAnimID>=numAnimations)

    */

    m_pAreaManager->SetAreaDirty(this);
}

//////////////////////////////////////////////////////////////////////////
void CArea::BeginSettingSolid(const Matrix34& worldTM)
{
    ReleaseAreaData();

    m_AreaType          = ENTITY_AREA_TYPE_SOLID;
    m_bInitialized  = true;

    m_WorldTM = worldTM;
    m_InvMatrix = m_WorldTM.GetInverted();

    m_AreaSolid = new CAreaSolid;
}

//////////////////////////////////////////////////////////////////////////
void CArea::AddConvexHullToSolid(const Vec3* verticesOfConvexHull, bool bObstruction, int numberOfVertices)
{
    if (!m_AreaSolid || m_AreaType != ENTITY_AREA_TYPE_SOLID)
    {
        return;
    }

    m_AreaSolid->AddSegment(verticesOfConvexHull, bObstruction, numberOfVertices);
}

//////////////////////////////////////////////////////////////////////////
void CArea::EndSettingSolid()
{
    if (!m_AreaSolid || m_AreaType != ENTITY_AREA_TYPE_SOLID)
    {
        return;
    }

    m_AreaSolid->BuildBSP();
    m_pAreaManager->SetAreaDirty(this);
}

//////////////////////////////////////////////////////////////////////////
void CArea::SetMatrix(const Matrix34& tm)
{
    m_InvMatrix = tm.GetInverted();
    m_WorldTM = tm;

    m_pAreaManager->SetAreaDirty(this);
}

//////////////////////////////////////////////////////////////////////////
void CArea::GetMatrix(Matrix34& tm) const
{
    tm = m_WorldTM;
}

//////////////////////////////////////////////////////////////////////////
void CArea::GetWorldBox(Vec3& rMin, Vec3& rMax) const
{
    switch (m_AreaType)
    {
    case ENTITY_AREA_TYPE_SHAPE:
    {
        rMin.x = m_areaBBox.min.x;
        rMin.y = m_areaBBox.min.y;
        rMin.z = m_VOrigin;
        rMax.x = m_areaBBox.max.x;
        rMax.y = m_areaBBox.max.y;
        rMax.z = m_VOrigin + m_VSize;

        break;
    }
    case ENTITY_AREA_TYPE_BOX:
    {
        rMin = m_WorldTM.TransformPoint(m_BoxMin);
        rMax = m_WorldTM.TransformPoint(m_BoxMax);

        break;
    }
    case ENTITY_AREA_TYPE_SPHERE:
    {
        rMin = m_SphereCenter - Vec3(m_SphereRadius, m_SphereRadius, m_SphereRadius);
        rMax = m_SphereCenter + Vec3(m_SphereRadius, m_SphereRadius, m_SphereRadius);

        break;
    }
    case ENTITY_AREA_TYPE_GRAVITYVOLUME:
    {
        rMin.zero();
        rMax.zero();

        break;
    }
    case ENTITY_AREA_TYPE_SOLID:
    {
        AABB oAABB;
        GetSolidBoundBox(oAABB);

        rMin = oAABB.min;
        rMax = oAABB.max;

        break;
    }
    default:
    {
        rMin.zero();
        rMax.zero();

        break;
    }
    }
}

//////////////////////////////////////////////////////////////////////////
void CArea::SetSphere(const Vec3& center, float fRadius)
{
    ReleaseAreaData();

    m_bInitialized = true;
    m_AreaType = ENTITY_AREA_TYPE_SPHERE;
    m_SphereCenter = center;
    m_SphereRadius = fRadius;
    m_SphereRadius2 = m_SphereRadius * m_SphereRadius;

    m_pAreaManager->SetAreaDirty(this);
}

//////////////////////////////////////////////////////////////////////////
void CArea::CalcBBox()
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);
    a2DBBox& areaBBox = m_areaBBox;
    areaBBox.min.x = m_vpSegments[0]->bbox.min.x;
    areaBBox.min.y = m_vpSegments[0]->bbox.min.y;
    areaBBox.max.x = m_vpSegments[0]->bbox.max.x;
    areaBBox.max.y = m_vpSegments[0]->bbox.max.y;
    for (unsigned int sIdx = 1; sIdx < m_vpSegments.size(); sIdx++)
    {
        if (areaBBox.min.x > m_vpSegments[sIdx]->bbox.min.x)
        {
            areaBBox.min.x = m_vpSegments[sIdx]->bbox.min.x;
        }
        if (areaBBox.min.y > m_vpSegments[sIdx]->bbox.min.y)
        {
            areaBBox.min.y = m_vpSegments[sIdx]->bbox.min.y;
        }
        if (areaBBox.max.x < m_vpSegments[sIdx]->bbox.max.x)
        {
            areaBBox.max.x = m_vpSegments[sIdx]->bbox.max.x;
        }
        if (areaBBox.max.y < m_vpSegments[sIdx]->bbox.max.y)
        {
            areaBBox.max.y = m_vpSegments[sIdx]->bbox.max.y;
        }
    }
    GetBBox() = areaBBox;
}

//////////////////////////////////////////////////////////////////////////
void CArea::AddEntity(const EntityId entId)
{
    m_vEntityID.push_back(entId);
    m_bAttachedSoundTested = false;
    m_fFadeDistance = -1.0f;

    m_pAreaManager->SetAreaDirty(this);
}

//////////////////////////////////////////////////////////////////////////
void CArea::AddEntity(const EntityGUID entGuid)
{
    m_vEntityGuid.push_back(entGuid);
    EntityId entId = GetEntitySystem()->FindEntityByGuid(entGuid);
    AddEntity(entId);
}

//////////////////////////////////////////////////////////////////////////
void CArea::ResolveEntityIds()
{
    if (m_bEntityIdsResolved)
    {
        return;
    }

    for (unsigned int i = 0; i < m_vEntityGuid.size(); i++)
    {
        EntityId entId = GetEntitySystem()->FindEntityByGuid(m_vEntityGuid[i]);
        m_vEntityID[i] = entId;
    }


    // Go through all our entity ids and have the entity system update them, to account
    // for any cloning
    unsigned int nSize = m_vEntityID.size();
    for (unsigned int eIdx = 0; eIdx < nSize; eIdx++)
    {
        m_vEntityID[eIdx] = GetEntitySystem()->GetClonedEntityId(m_vEntityID[eIdx], m_EntityID);
    }

    m_bEntityIdsResolved = true;
}

void CArea::ReleaseCachedAreaData()
{
    stl::free_container(m_mapEntityCachedAreaData);
}

//////////////////////////////////////////////////////////////////////////
float CArea::GetGreatestFadeDistance()
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);

    if (m_fFadeDistance < 0.0f || gEnv->IsEditor())
    {
        m_fFadeDistance = 0.0f;
        TEntityIDs::const_iterator Iter(m_vEntityID.begin());
        TEntityIDs::const_iterator const IterEnd(m_vEntityID.end());

        for (; Iter != IterEnd; ++Iter)
        {
            IEntity const* const pEntity = GetEntitySystem()->GetEntity((*Iter));

            if (pEntity != NULL)
            {
                IComponentAudioConstPtr const pAudioComponent = pEntity->GetComponent<IComponentAudio>();

                if (pAudioComponent != NULL)
                {
                    m_fFadeDistance = max(m_fFadeDistance, pAudioComponent->GetFadeDistance());
                }
            }
        }
    }

    return m_fFadeDistance;
}

//////////////////////////////////////////////////////////////////////////
float CArea::GetGreatestEnvironmentFadeDistance()
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);

    if (m_fEnvironmentFadeDistance < 0.0f || gEnv->IsEditor())
    {
        m_fEnvironmentFadeDistance = 0.0f;
        TEntityIDs::const_iterator Iter(m_vEntityID.begin());
        TEntityIDs::const_iterator const IterEnd(m_vEntityID.end());

        for (; Iter != IterEnd; ++Iter)
        {
            IEntity const* const pEntity = GetEntitySystem()->GetEntity((*Iter));

            if (pEntity != NULL)
            {
                IComponentAudioConstPtr const pAudioComponent = pEntity->GetComponent<IComponentAudio>();

                if (pAudioComponent != NULL)
                {
                    m_fEnvironmentFadeDistance = max(m_fEnvironmentFadeDistance, pAudioComponent->GetEnvironmentFadeDistance());
                }
            }
        }
    }

    return m_fEnvironmentFadeDistance;
}

//////////////////////////////////////////////////////////////////////////
bool    CArea::HasSoundAttached()
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);
    if (m_vEntityID.empty())
    {
        return false;
    }

    if (!m_bAttachedSoundTested)
    {
        for (unsigned int eIdx = 0; eIdx < m_vEntityID.size(); eIdx++)
        {
            IEntity* pAreaAttachedEntity = GetEntitySystem()->GetEntity(m_vEntityID[eIdx]);
            //      assert(pEntity);
            if (pAreaAttachedEntity)
            {
                IEntityClass* pEntityClass = pAreaAttachedEntity->GetClass();
                string sClassName = pEntityClass->GetName();
                if (sClassName == "AmbientVolume" || sClassName == "SoundSpot" || sClassName == "ReverbVolume")
                {
                    m_bHasSoundAttached = true;
                }
            }
        }
        m_bAttachedSoundTested = true;
    }

    return m_bHasSoundAttached;
}

//////////////////////////////////////////////////////////////////////////
void CArea::GetBBox(Vec2& vMin, Vec2& vMax) const
{
    // Only valid for shape areas.
    const a2DBBox& areaBox = GetBBox();
    vMin = Vec2(areaBox.min.x, areaBox.min.y);
    vMax = Vec2(areaBox.max.x, areaBox.max.y);
}

//////////////////////////////////////////////////////////////////////////
const CArea::a2DBBox& CArea::GetBBox() const
{
    return s_areaBoxes[m_bbox_holder].box;
}

//////////////////////////////////////////////////////////////////////////
CArea::a2DBBox& CArea::GetBBox()
{
    return s_areaBoxes[m_bbox_holder].box;
}

//////////////////////////////////////////////////////////////////////////
void CArea::GetSolidBoundBox(AABB& outBoundBox) const
{
    if (!m_AreaSolid)
    {
        return;
    }

    outBoundBox = m_AreaSolid->GetBoundBox();
}

//////////////////////////////////////////////////////////////////////////
void    CArea::AddEntites(const std::vector<EntityId>& entIDs)
{
    for (unsigned int i = 0; i < entIDs.size(); i++)
    {
        AddEntity(entIDs[i]);
    }

    // invalidating effect radius
    m_fFadeDistance = -1.0f;
}

//////////////////////////////////////////////////////////////////////////
void    CArea::ClearEntities()
{
    // tell all attached entities they have been disconnect to prevent lost entities
    IEntity* pEntity;

    if (CVar::pDrawAreaDebug->GetIVal() == 2)
    {
        CryLog("<AreaManager> Area %d Direct Event: %s", m_EntityID, "DETACH_THIS");
    }

    for (unsigned int eIdx = 0; eIdx < m_vEntityID.size(); eIdx++)
    {
        pEntity = GetEntitySystem()->GetEntity(m_vEntityID[eIdx]);
        //      assert(pEntity);
        if (pEntity)
        {
            SEntityEvent event;
            event.event = ENTITY_EVENT_DETACH_THIS;
            event.nParam[0] = m_EntityID;
            event.nParam[1] = m_AreaID;
            event.nParam[2] = 0;
            pEntity->SendEvent(event);
        }
    }

    m_vEntityID.clear();
    m_vEntityGuid.clear();
    m_PrevFade = -1.0f;
    m_bHasSoundAttached = false;

    // invalidating effect radius
    m_fFadeDistance = -1.0f;
}

//////////////////////////////////////////////////////////////////////////
void CArea::AddCachedEvent(const SEntityEvent& event)
{
    m_cachedEvents.push_back(event);
}

//////////////////////////////////////////////////////////////////////////
void CArea::ClearCachedEventsFor(EntityId const nEntityID)
{
    for (CachedEvents::iterator it = m_cachedEvents.begin(); it != m_cachedEvents.end(); )
    {
        SEntityEvent& event = *it;

        if (event.nParam[0] == nEntityID)
        {
            it = m_cachedEvents.erase(it);
            continue;
        }

        ++it;
    }
}

//////////////////////////////////////////////////////////////////////////
void CArea::ClearCachedEvents()
{
    m_cachedEvents.clear();
}

//////////////////////////////////////////////////////////////////////////
void CArea::SendCachedEventsFor(EntityId const nEntityID)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);
    size_t const nCountCachedEvents = m_cachedEvents.size();

    if (m_bInitialized && nCountCachedEvents > 0)
    {
        for (size_t i = 0; i < nCountCachedEvents; ++i)
        {
            SEntityEvent cachedEvent = m_cachedEvents[i]; // copy to avoid invalidation if vector re-allocates

            if (cachedEvent.nParam[0] == nEntityID)
            {
                if (CVar::pDrawAreaDebug->GetIVal() == 2)
                {
                    string sState;
                    if (cachedEvent.event == ENTITY_EVENT_ENTERNEARAREA)
                    {
                        sState = "ENTERNEAR";
                    }
                    if (cachedEvent.event == ENTITY_EVENT_MOVENEARAREA)
                    {
                        sState = "MOVENEAR";
                    }
                    if (cachedEvent.event == ENTITY_EVENT_ENTERAREA)
                    {
                        sState = "ENTER";
                    }
                    if (cachedEvent.event == ENTITY_EVENT_MOVEINSIDEAREA)
                    {
                        sState = "MOVEINSIDE";
                    }
                    if (cachedEvent.event == ENTITY_EVENT_LEAVEAREA)
                    {
                        sState = "LEAVE";
                    }
                    if (cachedEvent.event == ENTITY_EVENT_LEAVENEARAREA)
                    {
                        sState = "LEAVENEAR";
                    }

                    CryLog("<AreaManager> Area %d Queued Event: %s", m_EntityID, sState.c_str());
                }

                cachedEvent.nParam[1] = m_AreaID;
                cachedEvent.nParam[2] = m_EntityID;
                cachedEvent.fParam[0] = m_PrevFade;

                SendEvent(cachedEvent, false);
            }
        }

        ClearCachedEventsFor(nEntityID);
    }
}

//////////////////////////////////////////////////////////////////////////
void CArea::SendEvent(SEntityEvent& newEvent, bool bClearCachedEvents /* = true */)
{
    m_pAreaManager->OnEvent(newEvent.event, (EntityId)newEvent.nParam[0], this);

    size_t const nCountEntities = m_vEntityID.size();

    for (size_t eIdx = 0; eIdx < nCountEntities; ++eIdx)
    {
        if (IEntity* pAreaAttachedEntity = GetEntitySystem()->GetEntity(m_vEntityID[eIdx]))
        {
            pAreaAttachedEntity->SendEvent(newEvent);

            if (bClearCachedEvents)
            {
                ClearCachedEventsFor((EntityId)newEvent.nParam[0]);
            }
        }
    }
}

// do enter area - player was outside, now is inside
// calls entity OnEnterArea which calls script OnEnterArea( player, AreaID )
//////////////////////////////////////////////////////////////////////////
void CArea::EnterArea(IEntity const* const __restrict pEntity)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);
    if (m_bInitialized)
    {
        EntityId const nEntityID = pEntity->GetId();
        TEntityCachedAreaDataMap::iterator Iter(m_mapEntityCachedAreaData.find(nEntityID));

        if (Iter == m_mapEntityCachedAreaData.end())
        {
            // If we get here it means "OnAddedToAreaCache" did not get called, the entity was probably spawned within this area.
            m_mapEntityCachedAreaData.insert(std::make_pair(nEntityID, SCachedAreaData())).first;
        }

        m_PrevFade  = 1.0f;
        m_bIsActive = true;

        if (CVar::pDrawAreaDebug->GetIVal() == 2)
        {
            CryLog("<AreaManager> Area %d Direct Event: %s", m_EntityID, "ENTER");
        }

        SEntityEvent event;
        event.event = ENTITY_EVENT_ENTERAREA;
        event.nParam[0] = nEntityID;
        event.nParam[1] = m_AreaID;
        event.nParam[2] = m_EntityID;
        event.fParam[0] = 1.0f; // fading is handled within the near areas.. we've entered the inner area... ensure fade is fully on.. this fixes any time we teleport immediately into a region rather than transfering across the near region

        SendEvent(event);
    }
}


// do leave area - player was inside, now is outside
// calls entity OnLeaveArea which calls script OnLeaveArea( player, AreaID )
//////////////////////////////////////////////////////////////////////////
void CArea::LeaveArea(IEntity const* const __restrict pSrcEntity)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);
    if (m_bInitialized)
    {
        if (CVar::pDrawAreaDebug->GetIVal() == 2)
        {
            CryLog("<AreaManager> Area %d Direct Event: %s", m_EntityID, "LEAVE");
        }

        SEntityEvent event;
        event.event = ENTITY_EVENT_LEAVEAREA;
        event.nParam[0] = pSrcEntity->GetId();
        event.nParam[1] = m_AreaID;
        event.nParam[2] = m_EntityID;

        SendEvent(event);
    }
}

// do enter near area - entity was "far", now is "near"
// calls entity OnEnterNearArea which calls script OnEnterNearArea( entity(player), AreaID )
//////////////////////////////////////////////////////////////////////////
void CArea::EnterNearArea(IEntity const* const __restrict pSrcEntity)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);
    if (m_bInitialized)
    {
        EntityId const nEntityID = pSrcEntity->GetId();
        TEntityCachedAreaDataMap::iterator Iter(m_mapEntityCachedAreaData.find(nEntityID));

        if (Iter == m_mapEntityCachedAreaData.end())
        {
            // If we get here it means "OnAddedToAreaCache" did not get called, the entity was probably spawned within the near region of this area.
            m_mapEntityCachedAreaData.insert(std::make_pair(nEntityID, SCachedAreaData())).first;
        }

        if (CVar::pDrawAreaDebug->GetIVal() == 2)
        {
            CryLog("<AreaManager> Area %d Direct Event: %s", m_EntityID, "ENTERNEAR");
        }

        SEntityEvent event;
        event.event = ENTITY_EVENT_ENTERNEARAREA;
        event.nParam[0] = nEntityID;
        event.nParam[1] = m_AreaID;
        event.nParam[2] = m_EntityID;

        SendEvent(event);
    }
}

// do leave near area - entity was "near", now is "far"
// calls entity OnLeaveNearArea which calls script OnLeaveNearArea( entity(player), AreaID )
//////////////////////////////////////////////////////////////////////////
void CArea::LeaveNearArea(IEntity const* const __restrict pSrcEntity)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);
    if (m_bInitialized)
    {
        if (CVar::pDrawAreaDebug->GetIVal() == 2)
        {
            CryLog("<AreaManager> Area %d Direct Event: %s", m_EntityID, "LEAVENEAR");
        }

        SEntityEvent event;
        event.event = ENTITY_EVENT_LEAVENEARAREA;
        event.nParam[0] = pSrcEntity->GetId();
        event.nParam[1] = m_AreaID;
        event.nParam[2] = m_EntityID;

        SendEvent(event);

        // If the entity currently leaving is the last one in the area, set the area as inactive
        if (m_pAreaManager->GetNumberOfPlayersNearOrInArea(this) <= 1)
        {
            m_PrevFade  = -1.0f;
            m_bIsActive = false;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CArea::OnAddedToAreaCache(IEntity const* const pEntity)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);
    if (m_bInitialized)
    {
        EntityId const nEntityID = pEntity->GetId();
        TEntityCachedAreaDataMap::iterator Iter(m_mapEntityCachedAreaData.find(nEntityID));

        if (Iter == m_mapEntityCachedAreaData.end())
        {
            m_mapEntityCachedAreaData.insert(std::make_pair(nEntityID, SCachedAreaData())).first;
        }
        else
        {
            // Cannot yet exist, if so figure out why it wasn't removed properly or why this is called more than once!
            assert(false);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CArea::OnRemovedFromAreaCache(IEntity const* const pEntity)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);
    m_mapEntityCachedAreaData.erase(pEntity->GetId());
}

//calculate distance to area - proceed fade. player is inside of the area
//////////////////////////////////////////////////////////////////////////
void CArea::UpdateArea(Vec3 const& vPos, IEntity const* const pEntity)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);
    if (m_bInitialized)
    {
        ProceedFade(pEntity, CalculateFade(vPos));
    }
}


//calculate distance to area - proceed fade. player is inside of the area
//////////////////////////////////////////////////////////////////////////
void CArea::UpdateAreaInside(IEntity const* const __restrict pSrcEntity)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);
    if (m_bInitialized)
    {
        if (CVar::pDrawAreaDebug->GetIVal() == 2)
        {
            CryLog("<AreaManager> Area %d Direct Event: %s", m_EntityID, "MOVEINSIDE");
        }

        size_t const nCount = m_vEntityID.size();

        for (size_t eIdx = 0; eIdx < nCount; ++eIdx)
        {
            IEntity* const __restrict pEntity = GetEntitySystem()->GetEntity(m_vEntityID[eIdx]);

            if (pEntity != NULL)
            {
                SEntityEvent event;
                event.event = ENTITY_EVENT_MOVEINSIDEAREA;
                event.nParam[0] = pSrcEntity->GetId();
                event.nParam[1] = m_AreaID;
                event.nParam[2] = m_EntityID;
                event.fParam[0] = m_PrevFade;
                pEntity->SendEvent(event);
            }
        }
    }
}

// pEntity moves in an area, which is controlled by an area with a higher priority
//////////////////////////////////////////////////////////////////////////
void CArea::ExclusiveUpdateAreaInside(IEntity const* const __restrict pSrcEntity, EntityId const AreaHighEntityID, float const fadeValue, float const environmentFadeValue)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);
    if (m_bInitialized)
    {
        if (CVar::pDrawAreaDebug->GetIVal() == 2)
        {
            CryLog("<AreaManager> Area %d Direct Event: %s", m_EntityID, "MOVEINSIDE");
        }

        size_t const nCount = m_vEntityID.size();

        for (size_t eIdx = 0; eIdx < nCount; ++eIdx)
        {
            IEntity* const __restrict pEntity = GetEntitySystem()->GetEntity(m_vEntityID[eIdx]);

            if (pEntity != NULL)
            {
                SEntityEvent event;
                event.event = ENTITY_EVENT_MOVEINSIDEAREA;
                event.nParam[0] = pSrcEntity->GetId();
                event.nParam[1] = m_AreaID;
                event.nParam[2] = m_EntityID;   // AreaLowEntityID
                event.nParam[3] = AreaHighEntityID;
                event.fParam[0] = fadeValue;
                event.fParam[1] = environmentFadeValue;
                pEntity->SendEvent(event);
            }
        }
    }
}

// pEntity moves near an area, which is controlled by an area with a higher priority
//////////////////////////////////////////////////////////////////////////
void CArea::ExclusiveUpdateAreaNear(IEntity const* const __restrict pSrcEntity, EntityId const AreaHighEntityID, float const fadeValue)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);
    if (m_bInitialized)
    {
        if (CVar::pDrawAreaDebug->GetIVal() == 2)
        {
            CryLogAlways("<AreaManager> Area %d Direct Event: %s", m_EntityID, "MOVENEAR");
        }

        size_t const nCount = m_vEntityID.size();

        for (size_t eIdx = 0; eIdx < nCount; ++eIdx)
        {
            IEntity* const __restrict pEntity = GetEntitySystem()->GetEntity(m_vEntityID[eIdx]);

            if (pEntity != NULL)
            {
                SEntityEvent event;
                event.event = ENTITY_EVENT_MOVENEARAREA;
                event.nParam[0] = pSrcEntity->GetId();
                event.nParam[1] = m_AreaID;
                event.nParam[2] = m_EntityID;   // AreaLowEntityID
                event.nParam[3] = AreaHighEntityID;
                event.fParam[0] = fadeValue;
                pEntity->SendEvent(event);
            }
        }
    }
}

//calculate distance to area. player is inside of the area
//////////////////////////////////////////////////////////////////////////
float   CArea::CalculateFade(const Vec3& pos3D)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);
    float fadeCoeff = 0.0f;

    if (m_bInitialized)
    {
        a2DPoint const pos = CArea::a2DPoint(pos3D);

        switch (m_AreaType)
        {
        case ENTITY_AREA_TYPE_SOLID:
        {
            if (m_fProximity <= 0.0f)
            {
                fadeCoeff = 1.0f;
                break;
            }
            Vec3 PosOnHull(ZERO);
            float squaredDistance;
            if (!m_AreaSolid->QueryNearest(pos3D, CAreaSolid::eSegmentQueryFlag_All, PosOnHull, squaredDistance))
            {
                break;
            }
            fadeCoeff = sqrt_tpl(squaredDistance) / m_fProximity;
        }
        break;
        case     ENTITY_AREA_TYPE_SHAPE:
            fadeCoeff = CalcDistToPoint(pos);
            break;
        case     ENTITY_AREA_TYPE_SPHERE:
        {
            if (m_fProximity <= 0.0f)
            {
                fadeCoeff = 1.0f;
                break;
            }
            Vec3 Delta = pos3D - m_SphereCenter;
            fadeCoeff = (m_SphereRadius - Delta.GetLength()) / m_fProximity;
            if (fadeCoeff > 1.0f)
            {
                fadeCoeff = 1.0f;
            }
            break;
        }
        case     ENTITY_AREA_TYPE_BOX:
        {
            if (m_fProximity <= 0.0f)
            {
                fadeCoeff = 1.0f;
                break;
            }
            Vec3 p3D = m_InvMatrix.TransformPoint(pos3D);
            Vec3 MinDelta = p3D - m_BoxMin;
            Vec3 MaxDelta = m_BoxMax - p3D;
            Vec3 EdgeDist = (m_BoxMax - m_BoxMin) / 2.0f;
            if ((!EdgeDist.x) || (!EdgeDist.y) || (!EdgeDist.z))
            {
                fadeCoeff = 1.0f;
                break;
            }

            float fFadeScale = m_fProximity / 100.0f;
            EdgeDist *= fFadeScale;

            float fMinFade = MinDelta.x / EdgeDist.x;

            for (int k = 0; k < 3; k++)
            {
                float fFade1 = MinDelta[k] / EdgeDist[k];
                float fFade2 = MaxDelta[k] / EdgeDist[k];
                fMinFade = min(fMinFade, min(fFade1, fFade2));
            }     //k

            fadeCoeff = fMinFade;
            if (fadeCoeff > 1.0f)
            {
                fadeCoeff = 1.0f;
            }
            break;
        }
        }
    }

    return fadeCoeff;
}

//proceed fade. player is inside of the area
//////////////////////////////////////////////////////////////////////////
void CArea::ProceedFade(IEntity const* const __restrict pSrcEntity, float const fadeCoeff)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);
    if (m_bInitialized)
    {
        // no update if fade coefficient hasn't changed
        if (m_PrevFade != fadeCoeff)
        {
            m_PrevFade = fadeCoeff;

            if (CVar::pDrawAreaDebug->GetIVal() == 2)
            {
                CryLog("<AreaManager> Area %d Direct Event: %s", m_EntityID, "MOVEINSIDE");
            }

            size_t const nCount = m_vEntityID.size();

            for (size_t eIdx = 0; eIdx < nCount; ++eIdx)
            {
                IEntity* const __restrict pEntity = GetEntitySystem()->GetEntity(m_vEntityID[eIdx]);

                if (pEntity != NULL)
                {
                    SEntityEvent event;
                    event.event = ENTITY_EVENT_MOVEINSIDEAREA;
                    event.nParam[0] = pSrcEntity->GetId();
                    event.nParam[1] = m_EntityID;
                    event.fParam[0] = fadeCoeff;
                    pEntity->SendEvent(event);
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CArea::OnAreaCrossing(IEntity const* const __restrict pEntity)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);
    if (m_bInitialized && pEntity != NULL)
    {
        IEntity* __restrict pAreaAttachedEntity = NULL;
        std::vector<EntityId>::const_iterator const IterEnd(m_vEntityID.end());

        for (std::vector<EntityId>::const_iterator Iter(m_vEntityID.begin()); Iter != IterEnd; ++Iter)
        {
            pAreaAttachedEntity = GetEntitySystem()->GetEntity(*Iter);

            if (pAreaAttachedEntity != NULL)
            {
                SEntityEvent oEvent;
                oEvent.event     = ENTITY_EVENT_CROSS_AREA;
                oEvent.nParam[0] = pEntity->GetId();
                pAreaAttachedEntity->SendEvent(oEvent);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CArea::Draw(size_t const idx)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);
    I3DEngine* p3DEngine = gEnv->p3DEngine;
    IRenderAuxGeom* pRC = gEnv->pRenderer->GetIRenderAuxGeom();
    pRC->SetRenderFlags(e_Def3DPublicRenderflags);

    ColorB  colorsArray[] = {
        ColorB(255, 0, 0, 255),
        ColorB(0, 255, 0, 255),
        ColorB(0, 0, 255, 255),
        ColorB(255, 255, 0, 255),
        ColorB(255, 0, 255, 255),
        ColorB(0, 255, 255, 255),
        ColorB(255, 255, 255, 255),
    };
    ColorB  color = colorsArray[idx % (sizeof(colorsArray) / sizeof(ColorB))];
    ColorB  color1 = colorsArray[(idx + 1) % (sizeof(colorsArray) / sizeof(ColorB))];
    //  ColorB  color2 = colorsArray[(idx+2)%(sizeof(colorsArray)/sizeof(ColorB))];

    switch (m_AreaType)
    {
    case    ENTITY_AREA_TYPE_SOLID:
        m_AreaSolid->Draw(m_WorldTM, color, color1);
        break;
    case     ENTITY_AREA_TYPE_SHAPE:
    {
        Vec3 v0, v1;
        float   deltaZ = 0.1f;
        unsigned int nSize = m_vpSegments.size();
        for (unsigned int sIdx = 0; sIdx < nSize; sIdx++)
        {
            if (m_vpSegments[sIdx]->k < 0)
            {
                v0.x = m_vpSegments[sIdx]->bbox.min.x;
                v0.y = m_vpSegments[sIdx]->bbox.max.y;

                v1.x = m_vpSegments[sIdx]->bbox.max.x;
                v1.y = m_vpSegments[sIdx]->bbox.min.y;
            }
            else
            {
                v0.x = m_vpSegments[sIdx]->bbox.min.x;
                v0.y = m_vpSegments[sIdx]->bbox.min.y;

                v1.x = m_vpSegments[sIdx]->bbox.max.x;
                v1.y = m_vpSegments[sIdx]->bbox.max.y;
            }

            v0.z = max(m_VOrigin, p3DEngine->GetTerrainElevation(v0.x, v0.y) + deltaZ);
            v1.z = max(m_VOrigin, p3DEngine->GetTerrainElevation(v1.x, v1.y) + deltaZ);

            // draw lower line segments
            pRC->DrawLine(v0, color, v1, color);

            // Draw upper line segments and vertical edges
            if (m_VSize > 0.0f)
            {
                Vec3 v0Z = Vec3(v0.x, v0.y, m_VOrigin + m_VSize);
                Vec3 v1Z = Vec3(v1.x, v1.y, m_VOrigin + m_VSize);

                pRC->DrawLine(v0, color, v0Z, color);
                //pRC->DrawLine( v1, color, v1Z, color );
                pRC->DrawLine(v0Z, color, v1Z, color);
            }
        }
        break;
    }
    case     ENTITY_AREA_TYPE_SPHERE:
    {
        ColorB  color3 = color;
        color3.a = 64;

        pRC->SetRenderFlags(e_Def3DPublicRenderflags | e_AlphaBlended);
        pRC->DrawSphere(m_SphereCenter, m_SphereRadius, color3);
        break;
    }
    case     ENTITY_AREA_TYPE_BOX:
    {
        float fLength = m_BoxMax.x - m_BoxMin.x;
        float fWidth    = m_BoxMax.y - m_BoxMin.y;
        float fHeight = m_BoxMax.z - m_BoxMin.z;

        Vec3 v0 = m_BoxMin;
        Vec3 v1 = Vec3(m_BoxMin.x + fLength,  m_BoxMin.y,                 m_BoxMin.z);
        Vec3 v2 = Vec3(m_BoxMin.x + fLength,  m_BoxMin.y + fWidth,  m_BoxMin.z);
        Vec3 v3 = Vec3(m_BoxMin.x,                  m_BoxMin.y + fWidth,  m_BoxMin.z);
        Vec3 v4 = Vec3(m_BoxMin.x,                  m_BoxMin.y,                 m_BoxMin.z + fHeight);
        Vec3 v5 = Vec3(m_BoxMin.x + fLength,  m_BoxMin.y,                 m_BoxMin.z + fHeight);
        Vec3 v6 = Vec3(m_BoxMin.x + fLength,  m_BoxMin.y + fWidth,  m_BoxMin.z + fHeight);
        Vec3 v7 = Vec3(m_BoxMin.x,                  m_BoxMin.y + fWidth,  m_BoxMin.z + fHeight);

        v0 = m_WorldTM.TransformPoint(v0);
        v1 = m_WorldTM.TransformPoint(v1);
        v2 = m_WorldTM.TransformPoint(v2);
        v3 = m_WorldTM.TransformPoint(v3);
        v4 = m_WorldTM.TransformPoint(v4);
        v5 = m_WorldTM.TransformPoint(v5);
        v6 = m_WorldTM.TransformPoint(v6);
        v7 = m_WorldTM.TransformPoint(v7);

        // draw lower half of box
        pRC->DrawLine(v0, color1, v1, color1);
        pRC->DrawLine(v1, color1, v2, color1);
        pRC->DrawLine(v2, color1, v3, color1);
        pRC->DrawLine(v3, color1, v0, color1);

        // draw upper half of box
        pRC->DrawLine(v4, color1, v5, color1);
        pRC->DrawLine(v5, color1, v6, color1);
        pRC->DrawLine(v6, color1, v7, color1);
        pRC->DrawLine(v7, color1, v4, color1);

        // draw vertical edges
        pRC->DrawLine(v0, color1, v4, color1);
        pRC->DrawLine(v1, color1, v5, color1);
        pRC->DrawLine(v2, color1, v6, color1);
        pRC->DrawLine(v3, color1, v7, color1);

        break;
    }
    default:
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
void CArea::ReleaseAreaData()
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);
    ClearPoints();

    stl::free_container(m_mapEntityCachedAreaData);
}

//////////////////////////////////////////////////////////////////////////
void CArea::GetMemoryUsage(ICrySizer* pSizer) const
{
    SIZER_COMPONENT_NAME(pSizer, "CArea");
    if (m_AreaSolid)
    {
        m_AreaSolid->GetMemoryUsage(pSizer);
    }

    for (size_t i = 0, iSementSize(m_vpSegments.size()); i < iSementSize; ++i)
    {
        pSizer->AddObject(m_vpSegments[i], sizeof(*m_vpSegments[i]));
    }

    pSizer->AddObject(this, sizeof(*this));
}

//////////////////////////////////////////////////////////////////////////
float CArea::GetCachedPointWithinDistSq(EntityId const nEntityID)   const
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);
    float fValue = 0.0f;
    TEntityCachedAreaDataMap::const_iterator const Iter(m_mapEntityCachedAreaData.find(nEntityID));

    if (Iter != m_mapEntityCachedAreaData.end())
    {
        fValue = Iter->second.fDistanceWithinSq;
    }

    return fValue;
}

//////////////////////////////////////////////////////////////////////////
bool CArea::GetCachedPointWithin(EntityId const nEntityID) const
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);
    bool bValue = false;
    TEntityCachedAreaDataMap::const_iterator const Iter(m_mapEntityCachedAreaData.find(nEntityID));

    if (Iter != m_mapEntityCachedAreaData.end())
    {
        bValue = Iter->second.bPointWithin;
    }

    return bValue;
}

//////////////////////////////////////////////////////////////////////////
EAreaPosType CArea::GetCachedPointPosTypeWithin(EntityId const nEntityID)   const
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);
    EAreaPosType eValue = AREA_POS_TYPE_COUNT;
    TEntityCachedAreaDataMap::const_iterator const Iter(m_mapEntityCachedAreaData.find(nEntityID));

    if (Iter != m_mapEntityCachedAreaData.end())
    {
        eValue = Iter->second.ePosType;
    }

    return eValue;
}

//////////////////////////////////////////////////////////////////////////
float CArea::GetCachedPointNearDistSq(EntityId const nEntityID) const
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);
    float fValue = 0.0f;
    TEntityCachedAreaDataMap::const_iterator const Iter(m_mapEntityCachedAreaData.find(nEntityID));

    if (Iter != m_mapEntityCachedAreaData.end())
    {
        fValue = Iter->second.fDistanceNearSq;
    }

    return fValue;
}

//////////////////////////////////////////////////////////////////////////
Vec3 const& CArea::GetCachedPointOnHull(EntityId const nEntityID) const
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);
    TEntityCachedAreaDataMap::const_iterator const Iter(m_mapEntityCachedAreaData.find(nEntityID));

    if (Iter != m_mapEntityCachedAreaData.end())
    {
        return Iter->second.oPos;
    }

    return m_oNULLVec;
}

//////////////////////////////////////////////////////////////////////////
const CArea::TAreaBoxes& CArea::GetBoxHolders()
{
    return s_areaBoxes;
}

#if defined(INCLUDE_ENTITYSYSTEM_PRODUCTION_CODE)
//////////////////////////////////////////////////////////////////////////
char const* const CArea::GetAreaEntityName() const
{
    IEntitySystem const* const pIEntitySystem = gEnv->pEntitySystem;

    if (pIEntitySystem != NULL)
    {
        IEntity const* const pIEntity = pIEntitySystem->GetEntity(m_EntityID);

        if (pIEntity != NULL)
        {
            return pIEntity->GetName();
        }
    }

    return NULL;
}
#endif // INCLUDE_ENTITYSYSTEM_PRODUCTION_CODE


