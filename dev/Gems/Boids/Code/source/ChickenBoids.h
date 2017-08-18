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
#ifndef CRYINCLUDE_GAMEDLL_BOIDS_CHICKENBOIDS_H
#define CRYINCLUDE_GAMEDLL_BOIDS_CHICKENBOIDS_H
#pragma once

#include "Flock.h"
#include "BoidBird.h"

//////////////////////////////////////////////////////////////////////////
class CChickenBoid
    : public CBoidBird
{
public:
    CChickenBoid(SBoidContext& bc);
    virtual void Update(float dt, SBoidContext& bc);
    virtual void Think(float dt, SBoidContext& bc);
    virtual void Kill(const Vec3& hitPoint, const Vec3& force);
    virtual void Physicalize(SBoidContext& bc);
    virtual void OnPickup(bool bPickup, float fSpeed);
    virtual void OnCollision(SEntityEvent& event);
    virtual void CalcOrientation(Quat& qOrient);

protected:
    float m_maxIdleTime;
    float m_maxNonIdleTime;

    Vec3 m_avoidanceAccel;
    uint m_lastRayCastFrame;
    unsigned int m_bThrown : 1;
    unsigned int m_bScared : 1;
    unsigned int m_landing : 1; //! TODO: replace with m_status
};

//////////////////////////////////////////////////////////////////////////
// Chicken Flock, is a specialized flock type for chickens.
//////////////////////////////////////////////////////////////////////////
class CChickenFlock
    : public CFlock
{
public:
    CChickenFlock(IEntity* pEntity);
    virtual void CreateBoids(SBoidsCreateContext& ctx);
    virtual CBoidObject* CreateBoid() { return new CChickenBoid(m_bc); };
    virtual void OnAIEvent(EAIStimulusType type, const Vec3& pos, float radius, float threat, EntityId sender);
};

//////////////////////////////////////////////////////////////////////////
class CTurtleBoid
    : public CChickenBoid
{
public:
    CTurtleBoid(SBoidContext& bc)
        : CChickenBoid(bc){};
    virtual void Think(float dt, SBoidContext& bc);
};

//////////////////////////////////////////////////////////////////////////
class CTurtleFlock
    : public CChickenFlock
{
public:
    CTurtleFlock(IEntity* pEntity);
    virtual CBoidObject* CreateBoid() { return new CTurtleBoid(m_bc); };
};

#endif // CRYINCLUDE_GAMEDLL_BOIDS_CHICKENBOIDS_H
