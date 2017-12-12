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
#ifndef CRYINCLUDE_GAMEDLL_BOIDS_BOIDCOLLISION_H
#define CRYINCLUDE_GAMEDLL_BOIDS_BOIDCOLLISION_H
#pragma once

#include "IEntitySystem.h"
#include "RayCastQueue.h"
#include "CryAction.h"
#include "CryActionPhysicQueues.h"

#include "DeferredActionQueue.h"

typedef RayCastQueue<41> GlobalRayCaster;

class CBoidCollision
{
    Vec3 m_point;
    float m_dist;
    CTimeValue m_lastCheckTime;
    float m_checkDistance;
    Vec3 m_normal;
    bool m_isRequestingRayCast;
    uint32 m_reqId;
    GlobalRayCaster::ResultCallback m_callback;

public:
    inline float Distance() const { return m_dist; }
    inline Vec3& Point() { return m_point; }
    inline Vec3& Normal() { return m_normal; }
    inline float CheckDistance() const { return m_checkDistance; }
    inline bool IsRequestingRayCast() const { return m_isRequestingRayCast; }
    inline void SetCheckDistance(float d) { m_checkDistance = d; }
    inline CTimeValue& LastCheckTime() { return m_lastCheckTime; }

    CBoidCollision()
    {
        m_isRequestingRayCast = false;
        Reset();
    }

    ~CBoidCollision()
    {
        if (m_isRequestingRayCast)
        {
            CCryAction* pCryAction = static_cast<CCryAction*>(gEnv->pGame->GetIGameFramework());
            pCryAction->GetPhysicQueues().GetRayCaster().Cancel(m_reqId);
        }
    }

    inline void SetNoCollision() { m_dist = -1; }

    void SetCollision(const RayCastResult& hitResult);
    void SetCollisionCallback(const QueuedRayID& rayID, const RayCastResult& hitResult);

    void Reset();

    inline bool IsColliding() { return m_dist >= 0; }

    inline void UpdateTime() { m_lastCheckTime = gEnv->pTimer->GetFrameStartTime(); }

    void QueueRaycast(EntityId entId, const Vec3& rayStart, const Vec3& rayDirection,
        GlobalRayCaster::ResultCallback* resultCallback = NULL);
    void RaycastCallback(const QueuedRayID& rayID, const RayCastResult& result);
};

#endif // CRYINCLUDE_GAMEDLL_BOIDS_BOIDCOLLISION_H
