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
#ifndef CRYINCLUDE_GAMEDLL_BOIDS_BOIDFISH_H
#define CRYINCLUDE_GAMEDLL_BOIDS_BOIDFISH_H
#pragma once

#include <IScriptSystem.h>
#include <IAISystem.h>
#include "BoidObject.h"

//////////////////////////////////////////////////////////////////////////
//! Boid object with fish behavior.
//////////////////////////////////////////////////////////////////////////

class CBoidFish
    : public CBoidObject
{
public:
    CBoidFish(SBoidContext& bc);
    ~CBoidFish();

    virtual void Update(float dt, SBoidContext& bc);
    virtual void Kill(const Vec3& hitPoint, const Vec3& force);
    virtual void Physicalize(SBoidContext& bc);

protected:
    void SpawnParticleEffect(const Vec3& pos, SBoidContext& bc, int nEffect);

    float m_dyingTime; // Deisred height this birds want to fly at.
    SmartScriptTable vec_Bubble;

    enum EScriptFunc
    {
        SPAWN_BUBBLE,
        SPAWN_SPLASH,
    };
    HSCRIPTFUNCTION m_pOnSpawnBubbleFunc;
    HSCRIPTFUNCTION m_pOnSpawnSplashFunc;
};

#endif // CRYINCLUDE_GAMEDLL_BOIDS_BOIDFISH_H
