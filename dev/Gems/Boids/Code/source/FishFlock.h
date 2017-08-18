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
#ifndef CRYINCLUDE_GAMEDLL_BOIDS_FISHFLOCK_H
#define CRYINCLUDE_GAMEDLL_BOIDS_FISHFLOCK_H
#pragma once

#include <IScriptSystem.h>
#include <IAISystem.h>
#include "Flock.h"

//////////////////////////////////////////////////////////////////////////
class CFishFlock
    : public CFlock
{
public:
    CFishFlock(IEntity* pEntity)
        : CFlock(pEntity, EFLOCK_FISH)
    {
        m_boidEntityName = "FishBoid";
        m_boidDefaultAnimName = "swim_loop";
    };
    virtual void CreateBoids(SBoidsCreateContext& ctx);
};

#endif // CRYINCLUDE_GAMEDLL_BOIDS_FISHFLOCK_H
