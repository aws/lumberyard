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

#ifndef CRYINCLUDE_EDITOR_AI_NAVDATAGENERATION_AICOLLISION_H
#define CRYINCLUDE_EDITOR_AI_NAVDATAGENERATION_AICOLLISION_H
#pragma once


#include "ISystem.h"
#include "IPhysics.h"
#include "ITimer.h"
#include "IAgent.h"
#include "Graph.h"
#include <ICoverSystem.h>


#include <limits>

extern float walkabilityFloorUpDist;
extern float walkabilityFloorDownDist;
// assume character is this fat
extern float walkabilityRadius;
// radius of the swept sphere down to find the floor
extern float walkabilityDownRadius;
// assume character is this tall
extern float walkabilityTotalHeight;
// maximum allowed floor height change from one to the next
extern float walkabilityMaxDeltaZ;
// height of the torso capsule above foot
extern Vec3 walkabilityTorsoBaseOffset;

struct SBadFloorPosRecord
{
    SBadFloorPosRecord(const Vec3& pos)
        : pos(pos) {}

    bool operator<(const SBadFloorPosRecord& other) const
    {
        if (pos.x < other.pos.x)
        {
            return true;
        }
        else if (pos.x > other.pos.x)
        {
            return false;
        }
        else if (pos.y < other.pos.y)
        {
            return true;
        }
        else if (pos.y > other.pos.y)
        {
            return false;
        }
        else if (pos.z < other.pos.z)
        {
            return true;
        }
        else if (pos.z > other.pos.z)
        {
            return false;
        }
        else
        {
            return false;
        }
    }

    Vec3 pos;
};


/// Return the closest intersection point on surface of the swept sphere
/// and distance along the linesegment. hitPos can be zero - may be faster
bool IntersectSweptSphere(Vec3* hitPos, float& hitDist, const Lineseg& lineseg,
    float radius, EAICollisionEntities aiCollisionEntities, IPhysicalEntity** pSkipEnts = 0, int nSkipEnts = 0, int geomFlagsAny = geom_colltype0);
bool IntersectSweptSphere(Vec3* hitPos, float& hitDist, const Lineseg& lineseg,
    float radius, const std::vector<IPhysicalEntity*>& entities);

bool OverlapCylinder(const Lineseg& lineseg, float radius, const std::vector<IPhysicalEntity*>& entities);

/// Count of calls to CheckWalkability - for profiling
extern unsigned g_CheckWalkabilityCalls;

struct SCheckWalkabilityState
{
    enum ECachedWalkabilityState
    {
        CWS_NEW_QUERY, CWS_PROCESSING, CWS_RESULT
    };
    SCheckWalkabilityState()
        : state(CWS_NEW_QUERY)
        , numIterationsPerCheck(10)
        , computeHeightAlongTheLink(false)
    {}
    ECachedWalkabilityState state;
    int numIterationsPerCheck;
    // stuff below here is for internal use only
    Vec3 curFloorPos;
    Vec3 fromFloor;
    Vec3 toFloor;
    Vec3 horDelta;
    int nHorPoints;
    int curPointIndex;
    /// Tells CheckWalkability() that z floor positions computed along the link
    /// should be stored in 'heightAlongTheLink' for later analysis.
    bool computeHeightAlongTheLink;
    std::vector<float> heightAlongTheLink;
};

// NOTE Mai 24, 2007: <pvl> not really pretty, but this is a way of telling
// the CheckWalkability*() functions if the expensive floor position check needs
// to be performed.  If it doesn't (because m_pos already is a floor position),
// m_isFloor is set to true.
// Default ctor is done so that existing callers (that expect the floor check
// to be done) can remain as they are.
//
// TODO Mai 24, 2007: <pvl> CheckWalkability() interface is starting to show its
// age - most likely it should be changed to a class with state.
struct SWalkPosition
{
    Vec3 m_pos;
    bool m_isFloor;
    SWalkPosition (const Vec3& pos, bool isFloor = false)
        : m_pos(pos)
        , m_isFloor(isFloor)
    { }
};

//====================================================================
// CheckWalkability
/// Checks if it's possible to walk from a floor position derived from from
/// to a floor position derived from to! It also checks if the path
/// would cross a boundary (which can be empty). checkStart indicates
/// if the start position should be checked (yes when generating nav,
/// no when seeing if a character can reach somewhere).
/// Adds the from/to points to the list of bad floor records if they're bad
/// If this is from a graph link then the cached result can be passed in -
/// it gets updated with hashed collision info
/// Sometimes the test can take quite a long time due to the large number
/// of iterations (over long distances). So, a state can be passed in - first time
/// it should be initialised and subsequently the result of CheckWalkability should
/// only be used once the state gets set to CWS_RESULT. If you use this feature you
/// better make sure all the other params are the same between calls with the same
/// state pointer (it's OK to CheckWalkability with other or no state pointers inbetween)
//====================================================================
bool CheckWalkability(SWalkPosition f, SWalkPosition t, float paddingRadius, bool checkStart, const ListPositions& boundary,
    EAICollisionEntities aiCollisionEntities,
    SCachedPassabilityResult* pCachedResult = 0,
    SCheckWalkabilityState* pCheckWalkabilityState = 0);

/**
 * If a human waypoint link goes over flat ground without any significant
 * bumps or holes, walkability checking of this link can be reduced just to
 * intersecting a box (an approximation of volume created by sweeping
 * simplified human body shape along the link) with non-static world.
 */
bool CheckWalkabilitySimple(/*Vec3 from, Vec3 to,*/ SWalkPosition fromPos, SWalkPosition toPos, float paddingRadius, EAICollisionEntities aiCollisionEntities);

//====================================================================
// GetEntitiesFromAABB
//====================================================================
inline unsigned GetEntitiesFromAABB(IPhysicalEntity**& entities, const AABB& aabb, EAICollisionEntities aiCollisionEntities)
{
    return gEnv->pPhysicalWorld->GetEntitiesInBox(aabb.min, aabb.max, entities, aiCollisionEntities);
}

//====================================================================
// OverlapSphere
//====================================================================
inline bool OverlapSphere(const Vec3& pos, float radius, EAICollisionEntities aiCollisionEntities)
{
    primitives::sphere spherePrim;
    spherePrim.center = pos;
    spherePrim.r = radius;

    IPhysicalWorld* pPhysics = gEnv->pPhysicalWorld;
    float d = pPhysics->PrimitiveWorldIntersection(spherePrim.type, &spherePrim, Vec3(ZERO),
            aiCollisionEntities, 0, 0, geom_colltype0);

    return (d != 0.0f);
}

//====================================================================
// OverlapCapsule
//====================================================================
inline bool OverlapCapsule(const Lineseg& lineseg, float radius, EAICollisionEntities aiCollisionEntities)
{
    primitives::capsule capsulePrim;
    capsulePrim.center = 0.5f * (lineseg.start + lineseg.end);
    capsulePrim.axis = lineseg.end - lineseg.start;
    capsulePrim.hh = 0.5f * capsulePrim.axis.NormalizeSafe(Vec3_OneZ);
    capsulePrim.r = radius;

    IPhysicalWorld* pPhysics = gEnv->pPhysicalWorld;
    float d = pPhysics->PrimitiveWorldIntersection(capsulePrim.type, &capsulePrim, Vec3(ZERO),
            aiCollisionEntities, 0, 0, geom_colltype0);

    return (d != 0.0f);
}

//====================================================================
// OverlapCylinder
//====================================================================
inline bool OverlapCylinder(const Lineseg& lineseg, float radius, EAICollisionEntities aiCollisionEntities, IPhysicalEntity* pSkipEntity = NULL, int customFilter = 0)
{
    IPhysicalEntity* pSkipEnts = pSkipEntity;

    primitives::cylinder cylinderPrim;
    cylinderPrim.center = 0.5f * (lineseg.start + lineseg.end);
    cylinderPrim.axis = lineseg.end - lineseg.start;
    cylinderPrim.hh = 0.5f * cylinderPrim.axis.NormalizeSafe(Vec3_OneZ);
    cylinderPrim.r = radius;

    IPhysicalWorld* pPhysics = gEnv->pPhysicalWorld;
    float d = pPhysics->PrimitiveWorldIntersection(cylinderPrim.type, &cylinderPrim, Vec3(ZERO),
            aiCollisionEntities, 0, 0, customFilter ? customFilter : geom_colltype0, 0, 0, 0, &pSkipEnts, pSkipEntity ? 1 : 0);

    return (d != 0.0f);
}

//====================================================================
// IntersectSegment
//====================================================================
inline bool OverlapSegment(const Lineseg& lineseg, EAICollisionEntities aiCollisionEntities)
{
    Vec3 dir = lineseg.end - lineseg.start;
    ray_hit hit;
    if (gEnv->pPhysicalWorld->RayWorldIntersection(lineseg.start, dir,
            aiCollisionEntities,
            rwi_ignore_noncolliding | rwi_stop_at_pierceable, &hit, 1))
    {
        return true;
    }
    else
    {
        return false;
    }
}

//====================================================================
// IntersectSegment
//====================================================================
inline bool OverlapSegment(const Lineseg& lineseg, const std::vector<IPhysicalEntity*>& entities)
{
    Vec3 dir = lineseg.end - lineseg.start;
    ray_hit hit;
    unsigned nEntities = entities.size();
    for (unsigned iEntity = 0; iEntity < nEntities; ++iEntity)
    {
        IPhysicalEntity* pEntity = entities[iEntity];
        if (gEnv->pPhysicalWorld->RayTraceEntity(pEntity, lineseg.start, dir, &hit))
        {
            return true;
        }
    }
    return false;
}

//====================================================================
// IntersectSegment
//====================================================================
inline bool IntersectSegment(Vec3& hitPos, const Lineseg& lineseg, EAICollisionEntities aiCollisionEntities, int filter = rwi_ignore_noncolliding | rwi_stop_at_pierceable)
{
    ray_hit hit;
    int rayresult = gEnv->pPhysicalWorld->RayWorldIntersection(lineseg.start, lineseg.end - lineseg.start,
            aiCollisionEntities, filter, &hit, 1);
    if (!rayresult || hit.dist < 0.0f)
    {
        return false;
    }
    hitPos = hit.pt;
    return true;
}

//====================================================================
// IntersectSegment
//====================================================================
inline bool IntersectSegment(Vec3& hitPos, const Lineseg& lineseg, const std::vector<IPhysicalEntity*>& entities)
{
    ray_hit hit;
    float bestDist = std::numeric_limits<float>::max();
    const unsigned nEntities = entities.size();
    const Vec3 dir = lineseg.end - lineseg.start;
    for (unsigned iEntity = 0; iEntity < nEntities; ++iEntity)
    {
        IPhysicalEntity* pEntity = entities[iEntity];
        if (gEnv->pPhysicalWorld->RayTraceEntity(pEntity, lineseg.start, dir, &hit))
        {
            if (hit.dist < bestDist)
            {
                bestDist = hit.dist;
                hitPos = hit.pt;
            }
        }
    }
    return bestDist < std::numeric_limits<float>::max();
}

typedef std::set<SBadFloorPosRecord> TBadFloorRecords;
const TBadFloorRecords& GetBadFloorRecords();
void ClearBadFloorRecords();
void AddBadFloorRecord(const SBadFloorPosRecord& record);


//====================================================================
// GetFloorPos
/// returns false if no valid floor found. Checks from upDist above pos to
/// downDist below pos.
//====================================================================
inline bool GetFloorPos(Vec3& floorPos, const Vec3& pos, float upDist, float downDist, float radius, EAICollisionEntities aiCollisionEntities)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_AI);
    if (IntersectSegment(floorPos, Lineseg(pos + Vec3(0, 0, radius), pos - Vec3(0, 0, radius)), aiCollisionEntities, rwi_stop_at_pierceable | (geom_colltype_player << rwi_colltype_bit)))
    {
        return true;
    }

    Vec3 capStart = pos + Vec3(0, 0, upDist);
    Vec3 delta = Vec3(0, 0, -downDist - upDist);
    if (delta.z > 0.0f)
    {
        return false;
    }

    float hitDist;

    static float stepSize = 2.0f;
    unsigned nSteps = 1 + (unsigned)(downDist / stepSize);
    float fSteps = nSteps;
    Lineseg seg;
    for (unsigned iStep = 0; iStep < nSteps; ++iStep)
    {
        float frac1 = iStep / fSteps;
        float frac2 = (iStep + 1) / fSteps;
        seg.start = capStart + frac1 * delta;
        seg.end = capStart + frac2 * delta;
        if (IntersectSweptSphere(0, hitDist, seg, radius, aiCollisionEntities, 0, 0, rwi_stop_at_pierceable | (geom_colltype_player << rwi_colltype_bit)))
        {
            floorPos.Set(pos.x, pos.y, seg.start.z - (radius + hitDist));
            return true;
        }
    }
    return false;
}

//====================================================================
// GetFloorPos
/// returns false if no valid floor found
//====================================================================
inline bool GetFloorPos(Vec3& floorPos, const Vec3& pos, float upDist, float downDist, float radius, const std::vector<IPhysicalEntity*>& entities)
{
    if (IntersectSegment(floorPos, Lineseg(pos + Vec3(0, 0, radius), pos - Vec3(0, 0, radius)), entities))
    {
        return true;
    }

    Vec3 capStart = pos + Vec3(0, 0, upDist);
    Vec3 capEnd = pos - Vec3(0, 0, downDist);
    if (capStart.z < capEnd.z)
    {
        capStart.z = capEnd.z;
    }

    float hitDist;

    if (IntersectSweptSphere(0, hitDist, Lineseg(capStart, capEnd), radius, entities))
    {
        floorPos.Set(pos.x, pos.y, capStart.z - (radius + hitDist));
        return true;
    }
    else
    {
        return false;
    }
}

//===================================================================
// CheckBodyPos
/// Given a floor position (expected to be valid!) checks if a "standard"
/// human body will fit there. Returns true if it will.
//===================================================================
inline bool CheckBodyPos(const Vec3& floorPos, EAICollisionEntities aiCollisionEntities)
{
    Vec3 segStart = floorPos + walkabilityTorsoBaseOffset + Vec3(0, 0, walkabilityRadius);
    Vec3 segEnd = floorPos + Vec3(0, 0, walkabilityTotalHeight - walkabilityRadius);
    Lineseg torsoSeg(segStart, segEnd);
    return !OverlapCapsule(torsoSeg, walkabilityRadius, aiCollisionEntities);
}

//===================================================================
// CalcPolygonArea
//===================================================================
template<typename VecContainer, typename T>
T CalcPolygonArea(const VecContainer& pts)
{
    if (pts.size() < 2)
    {
        return T(0);
    }

    T totalCross = T(0);
    const typename VecContainer::const_iterator itEnd = pts.end();
    typename VecContainer::const_iterator it = pts.begin();
    typename VecContainer::const_iterator itNext = it;
    for (; it != itEnd; ++it)
    {
        if (++itNext == itEnd)
        {
            itNext = pts.begin();
        }
        totalCross += it->x * itNext->y - itNext->x * it->y;
    }

    return totalCross / T(2);
}

//===================================================================
// IsWoundAnticlockwise
//===================================================================
template<typename VecContainer, typename T>
bool IsWoundAnticlockwise(const VecContainer& pts)
{
    if (pts.size() < 2)
    {
        return true;
    }
    return CalcPolygonArea<VecContainer, T>(pts) > T(0);
}


//===================================================================
// EnsureShapeIsWoundAnticlockwise
//===================================================================
template<typename VecContainer, typename T>
void EnsureShapeIsWoundAnticlockwise(VecContainer& pts)
{
    if (!IsWoundAnticlockwise<VecContainer, T>(pts))
    {
        std::reverse(pts.begin(), pts.end());
    }
}

#include "AIHash.h"

//===================================================================
// GetFloorRectangleFromOrientedBox
// Returns a rectangle in XY plane from given
//===================================================================
struct SAIRect3
{
    Vec3 center;
    Vec3 axisu, axisv;
    Vec2 min, max;
};

void GetFloorRectangleFromOrientedBox(const Matrix34& tm, const AABB& box, SAIRect3& rect);


//===================================================================
// OverlapLinesegAABB2D
//===================================================================
inline bool OverlapLinesegAABB2D(const Vec3& p0, const Vec3& p1, const AABB& aabb)
{
    Vec3 c = (aabb.min + aabb.max) * 0.5f;
    Vec3 e = aabb.max - c;
    Vec3 m = (p0 + p1) * 0.5f;
    Vec3 d = p1 - m;
    m = m - c;

    // Try world coordinate axes as separating axes
    float adx = fabsf(d.x);
    if (fabsf(m.x) > e.x + adx)
    {
        return false;
    }
    float ady = fabsf(d.y);
    if (fabsf(m.y) > e.y + ady)
    {
        return false;
    }

    // Add in an epsilon term to counteract arithmetic errors when segment is
    // (near) parallel to a coordinate axis (see text for detail)
    const float EPSILON = 0.000001f;
    adx += EPSILON;
    ady += EPSILON;

    if (fabsf(m.x * d.y - m.y * d.x) > e.x * ady + e.y * adx)
    {
        return false;
    }

    // No separating axis found; segment must be overlapping AABB
    return true;
}


#endif // CRYINCLUDE_EDITOR_AI_NAVDATAGENERATION_AICOLLISION_H
