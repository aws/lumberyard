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
#include "StdAfx.h"

#include "BoidCollision.h"

void CBoidCollision::SetCollision(const RayCastResult& hitResult)
{
    if (hitResult.hitCount)
    {
        const ray_hit& hit = hitResult.hits[0];
        m_dist = hit.dist;
        m_point = hit.pt;
        m_normal = hit.n;
    }
    else
    {
        SetNoCollision();
    }
}

void CBoidCollision::SetCollisionCallback(const QueuedRayID& rayID, const RayCastResult& hitResult)
{
    SetCollision(hitResult);
}

void CBoidCollision::Reset()
{
    SetNoCollision();
    m_lastCheckTime.SetMilliSeconds(0);
    m_checkDistance = 5.f;
    if (m_isRequestingRayCast)
    {
        CCryAction* pCryAction = static_cast<CCryAction*>(gEnv->pGame->GetIGameFramework());
        pCryAction->GetPhysicQueues().GetRayCaster().Cancel(m_reqId);
    }

    m_callback = NULL;
    m_isRequestingRayCast = false;
}

void CBoidCollision::QueueRaycast(EntityId entId,
    const Vec3& rayStart,
    const Vec3& rayDirection,
    GlobalRayCaster::ResultCallback* resultCallback)
{
    if (m_isRequestingRayCast)
    {
        return;
    }

    const int flags = rwi_colltype_any | rwi_ignore_back_faces | rwi_stop_at_pierceable | rwi_queue;
    const int entityTypes = ent_static | ent_sleeping_rigid | ent_rigid | ent_terrain;

    IEntity* pEntity = gEnv->pEntitySystem->GetEntity(entId);
    IPhysicalEntity* pSkipList = pEntity ? pEntity->GetPhysics() : NULL;
    int n = pSkipList ? 1 : 0;

    m_callback = resultCallback ? *resultCallback : functor(*this, &CBoidCollision::SetCollisionCallback);

    m_isRequestingRayCast = true;
    // m_callback = resultCallback;
    CCryAction* pCryAction = static_cast<CCryAction*>(gEnv->pGame->GetIGameFramework());
    m_reqId =
        pCryAction->GetPhysicQueues().GetRayCaster().Queue(RayCastRequest::HighPriority,
            RayCastRequest(rayStart,
                rayDirection,
                entityTypes,
                flags,
                pSkipList ? &pSkipList : NULL,
                n,
                1),
            functor(*this, &CBoidCollision::RaycastCallback));
    return;
}

////////////////////////////////////////////////////////////////

void CBoidCollision::RaycastCallback(const QueuedRayID& rayID, const RayCastResult& result)
{
    if (m_callback)
    {
        m_callback(rayID, result);
    }
    m_isRequestingRayCast = false;
}
