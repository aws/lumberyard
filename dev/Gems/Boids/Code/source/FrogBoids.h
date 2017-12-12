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
#ifndef CRYINCLUDE_GAMEDLL_BOIDS_FROGBOIDS_H
#define CRYINCLUDE_GAMEDLL_BOIDS_FROGBOIDS_H
#pragma once

#include "Flock.h"
#include "BoidBird.h"

//////////////////////////////////////////////////////////////////////////
class CFrogBoid
    : public CBoidBird
{
public:
    CFrogBoid(SBoidContext& bc);
    ~CFrogBoid();
    virtual void Update(float dt, SBoidContext& bc);
    virtual void Think(float dt, SBoidContext& bc);
    virtual void Kill(const Vec3& hitPoint, const Vec3& force);
    virtual void Physicalize(SBoidContext& bc);
    virtual void OnPickup(bool bPickup, float fSpeed);
    virtual void OnCollision(SEntityEvent& event);

protected:
    void OnEnteringWater(const SBoidContext& bc);
    void UpdateWaterInteraction(const SBoidContext& bc);

    float m_maxIdleTime;
    Vec3 m_avoidanceAccel;
    float m_fTimeToNextJump;
    unsigned m_bThrown : 1;
    unsigned m_onGround : 1;

    bool m_bIsInsideWater;
    HSCRIPTFUNCTION m_pOnWaterSplashFunc;
};

//////////////////////////////////////////////////////////////////////////
// Frog Flock, is a specialized flock type for Frogs.
//////////////////////////////////////////////////////////////////////////
class CFrogFlock
    : public CFlock
{
public:
    CFrogFlock(IEntity* pEntity);
    virtual void CreateBoids(SBoidsCreateContext& ctx);
    virtual bool CreateEntities();
};

#endif // CRYINCLUDE_GAMEDLL_BOIDS_FROGBOIDS_H
