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
#include "AICollision.h"

TBadFloorRecords badFloorRecords;

const TBadFloorRecords& GetBadFloorRecords() {return badFloorRecords; }

void ClearBadFloorRecords() {badFloorRecords.clear(); }

void AddBadFloorRecord(const SBadFloorPosRecord& record) {badFloorRecords.insert(record); }

static EAICollisionEntities aiCollisionEntitiesTable[] =
{
    AICE_STATIC,
    AICE_ALL,
    AICE_ALL_SOFT,
    AICE_DYNAMIC,
    AICE_STATIC_EXCEPT_TERRAIN,
    AICE_ALL_EXCEPT_TERRAIN,
    AICE_ALL_INLUDING_LIVING
};

//====================================================================
// IntersectSweptSphere
// hitPos is optional - may be faster if 0
//====================================================================
bool IntersectSweptSphere(Vec3* hitPos, float& hitDist, const Lineseg& lineseg, float radius, EAICollisionEntities aiCollisionEntities, IPhysicalEntity** pSkipEnts, int nSkipEnts, int geomFlagsAny)
{
    IPhysicalWorld* pPhysics = gEnv->pPhysicalWorld;
    primitives::sphere spherePrim;
    spherePrim.center = lineseg.start;
    spherePrim.r = radius;

    Vec3 dir = lineseg.end - lineseg.start;

    geom_contact* pContact = 0;
    geom_contact** ppContact = hitPos ? &pContact : 0;
    int geomFlagsAll = 0;

    float d = pPhysics->PrimitiveWorldIntersection(spherePrim.type, &spherePrim, dir,
            aiCollisionEntities, ppContact,
            geomFlagsAll, geomFlagsAny, 0, 0, 0, pSkipEnts, nSkipEnts);

    if (d > 0.0f)
    {
        hitDist = d;
        if (pContact && hitPos)
        {
            *hitPos = pContact->pt;
        }
        return true;
    }
    else
    {
        return false;
    }
}

//====================================================================
// IntersectSweptSphere
//====================================================================
bool IntersectSweptSphere(Vec3* hitPos, float& hitDist, const Lineseg& lineseg, float radius, const std::vector<IPhysicalEntity*>& entities)
{
    IPhysicalWorld* pPhysics = gEnv->pPhysicalWorld;
    primitives::sphere spherePrim;
    spherePrim.center = lineseg.start;
    spherePrim.r = radius;

    Vec3 dir = lineseg.end - lineseg.start;

    ray_hit hit;
    unsigned nEntities = entities.size();
    hitDist = std::numeric_limits<float>::max();
    for (unsigned iEntity = 0; iEntity < nEntities; ++iEntity)
    {
        IPhysicalEntity* pEntity = entities[iEntity];
        if (pPhysics->CollideEntityWithBeam(pEntity, lineseg.start, dir, radius, &hit))
        {
            if (hit.dist < hitDist)
            {
                if (hitPos)
                {
                    *hitPos = hit.pt;
                }
                hitDist = hit.dist;
            }
        }
    }
    return hitDist < std::numeric_limits<float>::max();
}

//====================================================================
// OverlapCylinder
//====================================================================
bool OverlapCylinder(const Lineseg& lineseg, float radius, const std::vector<IPhysicalEntity*>& entities)
{
    primitives::cylinder cylinderPrim;
    cylinderPrim.center = 0.5f * (lineseg.start + lineseg.end);
    cylinderPrim.axis = lineseg.end - lineseg.start;
    cylinderPrim.hh = 0.5f * cylinderPrim.axis.NormalizeSafe(Vec3_OneZ);
    cylinderPrim.r = radius;

    ray_hit hit;
    unsigned nEntities = entities.size();
    IPhysicalWorld* pPhysics = gEnv->pPhysicalWorld;
    for (unsigned iEntity = 0; iEntity < nEntities; ++iEntity)
    {
        IPhysicalEntity* pEntity = entities[iEntity];
        if (pPhysics->CollideEntityWithPrimitive(pEntity, cylinderPrim.type, &cylinderPrim, Vec3(ZERO), &hit))
        {
            return true;
        }
    }
    return false;
}

// Passability parameters

// for finding the start/end positions
float walkabilityFloorUpDist = 0.1f;
float walkabilityFloorDownDist = 5.0f;
// assume character is this fat
float walkabilityRadius = 0.25f;
// radius of the swept sphere down to find the floor
float walkabilityDownRadius = 0.06f;
// assume character is this tall
float walkabilityTotalHeight = 1.8f;
// maximum allowed floor height change from one to the next
float walkabilityMaxDeltaZ = 0.6f;
// height of the torso capsule above foot
Vec3 walkabilityTorsoBaseOffset(0.0f, 0.0f, walkabilityMaxDeltaZ);
// Separation between sample points (horizontal)
float walkabilitySampleDist = 0.2f;

struct STimeChecker
{
    STimeChecker(const char* msg, float maxTimeMS)
        : maxTime(maxTimeMS)
        , msg(msg)
    {
        startTime = gEnv->pTimer->GetAsyncTime();
    }
    ~STimeChecker()
    {
        float delta = 1000.0f * (gEnv->pTimer->GetAsyncTime() - startTime).GetSeconds();
        if (delta > maxTime)
        {
            AIWarning("%s measured time %fms is greated than allowed time %fms", msg, delta, maxTime);
            int breakMe = 1;
        }
    }

    CTimeValue startTime;
    float maxTime;
    const char* msg;
};

unsigned g_CheckWalkabilityCalls;
//====================================================================
// CheckWalkability
//====================================================================
bool CheckWalkability(/*Vec3 from, Vec3 to*/ SWalkPosition fromPos, SWalkPosition toPos, float paddingRadius, bool checkStart, const ListPositions& boundary,
    EAICollisionEntities aiCollisionEntities,
    SCachedPassabilityResult* pCachedResult,
    SCheckWalkabilityState* pCheckWalkabilityState)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_AI);
    ++g_CheckWalkabilityCalls;

    Vec3 from = fromPos.m_pos;
    Vec3 to = toPos.m_pos;

    static SCheckWalkabilityState dummyWalkabilityState;

    const float checkRadius = walkabilityRadius + paddingRadius;

    if (pCheckWalkabilityState)
    {
        if (pCheckWalkabilityState->state == SCheckWalkabilityState::CWS_RESULT)
        {
            // programming error by caller
            AIError("CheckWalkability bad state");
            return false;
        }
    }
    else
    {
        pCheckWalkabilityState = &dummyWalkabilityState;
        dummyWalkabilityState.state = SCheckWalkabilityState::CWS_NEW_QUERY;
        dummyWalkabilityState.numIterationsPerCheck = 100000;
    }

    if (pCheckWalkabilityState->state == SCheckWalkabilityState::CWS_NEW_QUERY)
    {
        // set this so we don't have to worry with the early-outs
        pCheckWalkabilityState->state = SCheckWalkabilityState::CWS_RESULT;

        if (pCachedResult)
        {
            if (aiCollisionEntitiesTable[pCachedResult->aiCollisionEntitiesIndex] != aiCollisionEntities || pCachedResult->checkStart != checkStart)
            {
                // cached result is invalid
                pCachedResult->Reset(0, false);
                pCachedResult->aiCollisionEntitiesIndex = aiCollisionEntities == AICE_STATIC ? 0 : 1;
                pCachedResult->checkStart = checkStart;
            }
            else if (pCachedResult->AABBTestHash == 1 && pCachedResult->OBBTestHash == 1) // has just had reset called on it
            {
                return pCachedResult->walkableResult;
            }
        }

        if (Overlap::Lineseg_Polygon2D(Lineseg(from, to), boundary))
        {
            if (pCachedResult)
            {
                pCachedResult->Reset(1, false);
            }
            return false;
        }

        if (fromPos.m_isFloor)
        {
            pCheckWalkabilityState->fromFloor = fromPos.m_pos;
        }
        else
        {
            if (!GetFloorPos(pCheckWalkabilityState->fromFloor, from, walkabilityFloorUpDist, walkabilityFloorDownDist, walkabilityDownRadius, AICE_ALL))
            {
                AddBadFloorRecord(SBadFloorPosRecord(from));
                if (pCachedResult)
                {
                    pCachedResult->Reset(1, false);
                }
                return false;
            }
        }
        if (toPos.m_isFloor)
        {
            pCheckWalkabilityState->toFloor = toPos.m_pos;
        }
        else
        {
            if (!GetFloorPos(pCheckWalkabilityState->toFloor, to, walkabilityFloorUpDist, walkabilityFloorDownDist, walkabilityDownRadius, AICE_ALL))
            {
                AddBadFloorRecord(SBadFloorPosRecord(to));
                if (pCachedResult)
                {
                    pCachedResult->Reset(1, false);
                }
                return false;
            }
        }

        Vec3 delta = pCheckWalkabilityState->toFloor - pCheckWalkabilityState->fromFloor;
        pCheckWalkabilityState->horDelta.Set(delta.x, delta.y, 0.0f);
        float horDist = pCheckWalkabilityState->horDelta.GetLength();
        pCheckWalkabilityState->nHorPoints = 1 + (int) (horDist / walkabilitySampleDist);

        if (pCachedResult)
        {
            static std::vector<IPhysicalEntity*> physicalEntities;
            physicalEntities.resize(0);

            float minZ = pCheckWalkabilityState->fromFloor.z - 0.5f * pCheckWalkabilityState->nHorPoints * walkabilityMaxDeltaZ;
            float maxZ = pCheckWalkabilityState->fromFloor.z + 0.5f * pCheckWalkabilityState->nHorPoints * walkabilityMaxDeltaZ + walkabilityTotalHeight;

            AABB aabb(AABB::RESET);
            aabb.Add(pCheckWalkabilityState->fromFloor, checkRadius);
            aabb.Add(Vec3(pCheckWalkabilityState->toFloor.x, pCheckWalkabilityState->toFloor.y, minZ), checkRadius);
            aabb.Add(Vec3(pCheckWalkabilityState->toFloor.x, pCheckWalkabilityState->toFloor.y, maxZ), checkRadius);
            IPhysicalEntity** entities;
            unsigned nEntities = GetEntitiesFromAABB(entities, aabb, aiCollisionEntities);
            if (0 == nEntities)
            {
                pCachedResult->Reset(1, false);
                return false;
            }

            unsigned AABBHash = GetHashFromEntities(entities, nEntities);
            if (AABBHash == pCachedResult->AABBTestHash)
            {
                return pCachedResult->walkableResult;
            }
            pCachedResult->AABBTestHash = AABBHash;

#if 1
            // narrow down the list to those intersecting an orientated box
            Vec3 horWalkDelta = pCheckWalkabilityState->toFloor - pCheckWalkabilityState->fromFloor;
            horWalkDelta.z = 0.0f;
            float horWalkDist = horWalkDelta.GetLength();
            Vec3 horWalkDir = horWalkDelta.GetNormalizedSafe(Vec3_OneY);

            primitives::box boxPrim;
            boxPrim.center = 0.5f * (pCheckWalkabilityState->fromFloor + pCheckWalkabilityState->toFloor);
            boxPrim.center.z += 0.5f * walkabilityTotalHeight;
            boxPrim.size.Set(checkRadius, 0.5f * horWalkDist + checkRadius, 0.5f * (maxZ - minZ));
            boxPrim.bOriented = 1;
            boxPrim.Basis.SetFromVectors(Vec3(0, 0, 1).Cross(horWalkDir), horWalkDir, Vec3(0, 0, 1));
            boxPrim.Basis.Transpose();

            OBB obb;
            obb.SetOBB(boxPrim.Basis, boxPrim.size, ZERO);
            ray_hit hit;
            for (unsigned iEntity = 0; iEntity < nEntities; ++iEntity)
            {
                pe_params_bbox params;
                entities[iEntity]->GetParams(&params);
                //      if ( (params.BBox[0].IsEquivalent(ZERO) && params.BBox[1].IsEquivalent(ZERO) ) ||
                //           Overlap::AABB_OBB(AABB(params.BBox[0], params.BBox[1]), boxPrim.center, obb))
                if (gEnv->pPhysicalWorld->CollideEntityWithPrimitive(entities[iEntity], boxPrim.type, &boxPrim, ZERO, &hit))
                {
                    physicalEntities.push_back(entities[iEntity]);
                }
            }

            unsigned OBBHash = physicalEntities.empty() ? 1 : GetHashFromEntities(&physicalEntities[0], physicalEntities.size());
            if (OBBHash == pCachedResult->OBBTestHash)
            {
                return pCachedResult->walkableResult;
            }
            pCachedResult->OBBTestHash = OBBHash;
#else
            pCachedResult->OBBTestHash = 1;
#endif
        }

        // optionally check we can actually fit the torso at the start.
        if (checkStart)
        {
            Vec3 segStart = pCheckWalkabilityState->fromFloor + walkabilityTorsoBaseOffset;
            Vec3 segEnd = pCheckWalkabilityState->fromFloor + Vec3(0, 0, walkabilityTotalHeight);
            Lineseg torsoSeg(segStart, segEnd);
            if (OverlapCylinder(torsoSeg, checkRadius, aiCollisionEntities))
            {
                if (pCachedResult)
                {
                    pCachedResult->walkableResult = false;
                }
                return false;
            }
        }

        // always check the end
        {
            Vec3 segStart = pCheckWalkabilityState->toFloor + walkabilityTorsoBaseOffset;
            Vec3 segEnd = pCheckWalkabilityState->toFloor + Vec3(0, 0, walkabilityTotalHeight);
            Lineseg torsoSeg(segStart, segEnd);
            if (OverlapCylinder(torsoSeg, checkRadius, aiCollisionEntities))
            {
                if (pCachedResult)
                {
                    pCachedResult->walkableResult = false;
                }
                return false;
            }
        }

        bool swapped = false;
        if (!checkStart && pCheckWalkabilityState->toFloor.z < pCheckWalkabilityState->fromFloor.z)
        {
            swapped = true;
            std::swap(pCheckWalkabilityState->toFloor, pCheckWalkabilityState->fromFloor);
            std::swap(to, from);
            pCheckWalkabilityState->horDelta = -pCheckWalkabilityState->horDelta;
        }

        // destination is too high
        if (pCheckWalkabilityState->nHorPoints * walkabilityMaxDeltaZ < abs(pCheckWalkabilityState->toFloor.z - pCheckWalkabilityState->fromFloor.z))
        {
            if (pCachedResult)
            {
                pCachedResult->walkableResult = false;
            }
            return false;
        }

        pCheckWalkabilityState->curFloorPos = pCheckWalkabilityState->fromFloor;
        pCheckWalkabilityState->curPointIndex = 1;

        if (pCheckWalkabilityState->computeHeightAlongTheLink)
        {
            pCheckWalkabilityState->heightAlongTheLink.push_back (pCheckWalkabilityState->fromFloor.z);
        }
    } // end of the huge check to see if this is a new check (pCheckWalkabilityState)

    pCheckWalkabilityState->state = SCheckWalkabilityState::CWS_RESULT;

    int lastIndexToCheck = pCheckWalkabilityState->curPointIndex + pCheckWalkabilityState->numIterationsPerCheck - 1;
    for (; pCheckWalkabilityState->curPointIndex <= pCheckWalkabilityState->nHorPoints; ++pCheckWalkabilityState->curPointIndex)
    {
        Vec3 nextFloorPos = pCheckWalkabilityState->fromFloor + pCheckWalkabilityState->curPointIndex * pCheckWalkabilityState->horDelta / pCheckWalkabilityState->nHorPoints;
        nextFloorPos.z = pCheckWalkabilityState->curFloorPos.z + walkabilityMaxDeltaZ;

        // First test if we can get the base of the torso there
        if (OverlapSegment(Lineseg(pCheckWalkabilityState->curFloorPos + walkabilityTorsoBaseOffset, nextFloorPos + walkabilityTorsoBaseOffset), aiCollisionEntities))
        {
            if (pCachedResult)
            {
                pCachedResult->walkableResult = false;
            }
            return false;
        }
        // base of torso is OK - now check down (bearing in mind we've already moved up.
        // Start a bit high so we don't start underneath tables etc
        if (!GetFloorPos(nextFloorPos, nextFloorPos, walkabilityFloorUpDist, 2.0f * walkabilityMaxDeltaZ, walkabilityDownRadius, AICE_ALL))
        {
            if (pCachedResult)
            {
                pCachedResult->walkableResult = false;
            }
            return false;
        }
        if (abs(nextFloorPos.z - pCheckWalkabilityState->curFloorPos.z) > walkabilityMaxDeltaZ)
        {
            if (pCachedResult)
            {
                pCachedResult->walkableResult = false;
            }
            return false;
        }

        if (pCheckWalkabilityState->computeHeightAlongTheLink)
        {
            pCheckWalkabilityState->heightAlongTheLink.push_back (nextFloorPos.z);
        }

        // check we can actually fit the torso there - except the start/destination is already checked (bear in
        // mind the direction might be reversed)
        if (pCheckWalkabilityState->curPointIndex != pCheckWalkabilityState->nHorPoints)
        {
            Vec3 segStart = nextFloorPos + walkabilityTorsoBaseOffset;
            Vec3 segEnd = nextFloorPos + Vec3(0, 0, walkabilityTotalHeight);
            Lineseg torsoSeg(segStart, segEnd);
            if (OverlapCylinder(torsoSeg, checkRadius, aiCollisionEntities))
            {
                if (pCachedResult)
                {
                    pCachedResult->walkableResult = false;
                }
                return false;
            }
        }

        pCheckWalkabilityState->curFloorPos = nextFloorPos;

        if (pCheckWalkabilityState->curPointIndex >= lastIndexToCheck)
        {
            pCheckWalkabilityState->state = SCheckWalkabilityState::CWS_PROCESSING;
            ++pCheckWalkabilityState->curPointIndex;
            return false;
        }
    }

    pCheckWalkabilityState->state = SCheckWalkabilityState::CWS_RESULT;
    if (abs(pCheckWalkabilityState->curFloorPos.z - pCheckWalkabilityState->toFloor.z) > walkabilityMaxDeltaZ)
    {
        if (pCachedResult)
        {
            pCachedResult->walkableResult = false;
        }
        return false;
    }

    if (pCachedResult)
    {
        pCachedResult->walkableResult = true;
    }
    return true;
}

bool CheckWalkabilitySimple(/*Vec3 from, Vec3 to,*/ SWalkPosition fromPos, SWalkPosition toPos, float paddingRadius, EAICollisionEntities aiCollisionEntities)
{
    Vec3 from (fromPos.m_pos);
    Vec3 to (toPos.m_pos);

    //  GetAISystem()->AddDebugLine(from, to, 255,0,0, 5.0f);

    Vec3 fromFloor, toFloor;

    if (fromPos.m_isFloor)
    {
        fromFloor = fromPos.m_pos;
    }
    else
    {
        if (!GetFloorPos(fromFloor, from, walkabilityFloorUpDist, walkabilityFloorDownDist, walkabilityDownRadius, AICE_ALL))
        {
            AddBadFloorRecord(SBadFloorPosRecord(from));
            return false;
        }
    }

    if (toPos.m_isFloor)
    {
        toFloor = toPos.m_pos;
    }
    else
    {
        if (!GetFloorPos(toFloor, to, walkabilityFloorUpDist, walkabilityFloorDownDist, walkabilityDownRadius, AICE_ALL))
        {
            AddBadFloorRecord(SBadFloorPosRecord(to));
            return false;
        }
    }

    const float checkRadius = walkabilityRadius + paddingRadius;

    float minZ = std::min (fromFloor.z, toFloor.z);
    float maxZ = std::max (fromFloor.z, toFloor.z) + walkabilityTotalHeight;

    AABB aabb(AABB::RESET);
    aabb.Add(Vec3(fromFloor.x, fromFloor.y, minZ));
    aabb.Add(Vec3(toFloor.x, toFloor.y, maxZ));
    const float padding = checkRadius * sqrtf(2.0f);
    aabb.min.x -= padding;
    aabb.min.y -= padding;
    aabb.max.x += padding;
    aabb.max.y += padding;

    IPhysicalEntity** entities;
    unsigned nEntities = GetEntitiesFromAABB(entities, aabb, aiCollisionEntities);

    if (0 == nEntities)
    {
        return true;
    }

    Vec3 horWalkDelta = toFloor - fromFloor;
    horWalkDelta.z = 0.0f;
    float horWalkDist = horWalkDelta.GetLength();
    Vec3 horWalkDir = horWalkDelta.GetNormalizedSafe(Vec3_OneY);

    const float boxHeight = walkabilityTotalHeight - walkabilityMaxDeltaZ;
    primitives::box boxPrim;
    boxPrim.center = 0.5f * (fromFloor + toFloor);
    boxPrim.center.z += walkabilityMaxDeltaZ + 0.5f * boxHeight;
    boxPrim.size.Set(checkRadius, 0.5f * horWalkDist + checkRadius, boxHeight / 2);
    boxPrim.bOriented = 1;
    boxPrim.Basis.SetFromVectors(horWalkDir.Cross(Vec3(0, 0, 1)), horWalkDir, Vec3(0, 0, 1));
    boxPrim.Basis.Transpose();


    //[Alexey] Because of return false between CreatePrimitive and Release we had leaks here!!!
    //_smart_ptr<IGeometry> box = gEnv->pPhysicalWorld->GetGeomManager()->CreatePrimitive(primitives::box::type,&boxPrim);
    //box->Release();
    // NOTE Sep 7, 2007: <pvl> made static after SetData() interface became
    // available, to avoid per-frame allocations
    static IGeometry* box = gEnv->pPhysicalWorld->GetGeomManager()->CreatePrimitive(primitives::box::type, &boxPrim);
    box->SetData (&boxPrim);

    pe_status_nparts snp;
    pe_status_pos sp;
    geom_world_data gwd;
    intersection_params ip;
    ip.bStopAtFirstTri = true;
    ip.bNoAreaContacts = true;
    ip.bNoBorder = true;
    ip.bNoIntersection = 1;

    //#define DBG_WALK
    for (int i = 0; i < nEntities; i++)
    {
        for (sp.ipart = entities[i]->GetStatus(&snp) - 1; sp.ipart >= 0; --sp.ipart)
        {
            sp.partid = -1;
            // NOTE Mai 31, 2007: <pvl> I'm not sure if this could happen, or what it
            // means if it does - but if it does, just ignore this part
            if (0 == entities[i]->GetStatus(&sp) || !sp.pGeomProxy)
            {
                continue;
            }
            gwd.offset = sp.pos;
            gwd.R = Matrix33(sp.q);
            gwd.scale = sp.scale;
            geom_contact dummy;
            geom_contact* dummy_ptr;
            {
                WriteLockCond lock;
                if (sp.pGeomProxy->IntersectLocked(box, &gwd, 0, &ip, dummy_ptr, lock))
                {
#ifdef DBG_WALK
                    // debug
                    OBB obb;
                    obb.SetOBB (boxPrim.Basis.GetTransposed(), boxPrim.size, Vec3(ZERO));
                    GetAISystem()->AddDebugBox(boxPrim.center, obb, 128, 0, 0, 5.0f);
#endif

                    return false;
                }
                else
                {
                    OBB obb;
                    obb.SetOBB (boxPrim.Basis.GetTransposed(), boxPrim.size, Vec3 (ZERO));
                    if (Overlap::Point_OBB (sp.pGeomProxy->GetCenter (), boxPrim.center, obb))
                    {
#ifdef DBG_WALK
                        // debug
                        GetAISystem()->AddDebugBox(boxPrim.center, obb, 128, 0, 0, 5.0f);
#endif
                        return false;
                    }
                }
            } // lock
        }
    }

    //box->Release();

#ifdef DBG_WALK
    // debug
    OBB obb;
    obb.SetOBB (boxPrim.Basis.GetTransposed(), boxPrim.size, Vec3 (ZERO));
    GetAISystem()->AddDebugBox(boxPrim.center, obb, 0, 128, 0, 8.0f);
#endif

    return true;
}


//===================================================================
// GetFloorRectangleFromOrientedBox
//===================================================================
void GetFloorRectangleFromOrientedBox(const Matrix34& tm, const AABB& box, SAIRect3& rect)
{
    Matrix33 tmRot(tm);
    tmRot.Transpose();

    Vec3    corners[8];
    SetAABBCornerPoints(box, corners);

    rect.center = tm.GetTranslation();

    rect.axisu = tm.GetColumn1();
    rect.axisu.z = 0;
    rect.axisu.Normalize();
    rect.axisv.Set(rect.axisu.y, -rect.axisu.x, 0);

    Vec3 tu = tmRot.TransformVector(rect.axisu);
    Vec3 tv = tmRot.TransformVector(rect.axisv);

    rect.min.x = FLT_MAX;
    rect.min.y = FLT_MAX;
    rect.max.x = -FLT_MAX;
    rect.max.y = -FLT_MAX;

    for (unsigned i = 0; i < 8; ++i)
    {
        float du = tu.Dot(corners[i]);
        float dv = tv.Dot(corners[i]);
        rect.min.x = min(rect.min.x, du);
        rect.max.x = max(rect.max.x, du);
        rect.min.y = min(rect.min.y, dv);
        rect.max.y = max(rect.max.y, dv);
    }
}