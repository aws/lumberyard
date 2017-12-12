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
#ifndef CRYINCLUDE_GAMEDLL_BOIDS_BUGSFLOCK_H
#define CRYINCLUDE_GAMEDLL_BOIDS_BUGSFLOCK_H
#pragma once

#include "Flock.h"

/*! Single Bug.
*/
class CBoidBug
    : public CBoidObject
{
public:
    CBoidBug(SBoidContext& bc);
    void Update(float dt, SBoidContext& bc);
    void Render(SRendParams& rp, CCamera& cam, SBoidContext& bc);

private:
    void UpdateBugsBehavior(float dt, SBoidContext& bc);
    void UpdateDragonflyBehavior(float dt, SBoidContext& bc);
    void UpdateFrogsBehavior(float dt, SBoidContext& bc);
    // void CalcRandomTarget( const Vec3 &origin,SBoidContext &bc );
    friend class CBugsFlock;
    int m_objectId;

    // Vec3 m_targetPos;
    // Flags.
    unsigned m_onGround : 1; //! True if landed on ground.
    // unsigned m_landing : 1;      //! True if bird wants to land.
    // unsigned m_takingoff : 1;    //! True if bird is just take-off from land.
};

/*! Bugs Flock, is a specialized flock type for all kind of small bugs and flies around player.
*/
class CBugsFlock
    : public CFlock
{
public:
    CBugsFlock(IEntity* pEntity);
    ~CBugsFlock();

    virtual void CreateBoids(SBoidsCreateContext& ctx);

protected:
    friend class CBoidBug;
};

#endif // CRYINCLUDE_GAMEDLL_BOIDS_BUGSFLOCK_H
