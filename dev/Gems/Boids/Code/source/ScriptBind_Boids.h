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
#ifndef CRYINCLUDE_GAMEDLL_BOIDS_SCRIPTBIND_BOIDS_H
#define CRYINCLUDE_GAMEDLL_BOIDS_SCRIPTBIND_BOIDS_H
#pragma once

#include <IScriptSystem.h>

// forward declarations.
class CFlock;
struct SBoidsCreateContext;

// <title Boids>
// Syntax: Boids
// Description:
//    Provides access to boids flock functionality.
class CScriptBind_Boids
    : public CScriptableBase
{
public:
    CScriptBind_Boids(ISystem* pSystem);
    virtual ~CScriptBind_Boids(void);

    virtual void GetMemoryUsage(ICrySizer* pSizer) const { pSizer->AddObject(this, sizeof(*this)); }
    // <title CreateFlock>
    // Syntax: Boids.CreateFlock( entity,paramsTable )
    // Description:
    //    Creates a flock of boids and binds it to the given entity.
    // Arguments:
    //    entity - Valid entity table.
    //    nType - Type of the flock, can be Boids.FLOCK_BIRDS,Boids.FLOCK_FISH,Boids.FLOCK_BUGS.
    //    paramTable - Table with parameters for flock (Look at sample scripts).
    int CreateFlock(IFunctionHandler* pH, SmartScriptTable entity, int nType, SmartScriptTable paramTable);

    // <title CreateFishFlock>
    // Syntax: Boids.CreateFishFlock( entity,paramsTable )
    // Description:
    //    Creates a fishes flock and binds it to the given entity.
    // Arguments:
    //    entity - Valid entity table.
    //    paramTable - Table with parameters for flock (Look at sample scripts).
    int CreateFishFlock(IFunctionHandler* pH, SmartScriptTable entity, SmartScriptTable paramTable);

    // <title CreateBugsFlock>
    // Syntax: Boids.CreateBugsFlock( entity,paramsTable )
    // Description:
    //    Creates a bugs flock and binds it to the given entity.
    // Arguments:
    //    entity - Valid entity table.
    //    paramTable - Table with parameters for flock (Look at sample scripts).
    int CreateBugsFlock(IFunctionHandler* pH, SmartScriptTable entity, SmartScriptTable paramTable);

    // <title SetFlockParams>
    // Syntax: Boids.SetFlockParams( entity,paramsTable )
    // Description:
    //    Modifies parameters of the existing flock in the specified enity.
    // Arguments:
    //    entity - Valid entity table containing flock.
    //    paramTable - Table with parameters for flock (Look at sample scripts).
    int SetFlockParams(IFunctionHandler* pH, SmartScriptTable entity, SmartScriptTable paramTable);

    // <title EnableFlock>
    // Syntax: Boids.EnableFlock( entity,paramsTable )
    // Description:
    //    Enables/Disables flock in the entity.
    // Arguments:
    //    entity - Valid entity table containing flock.
    //    bEnable - true to enable or false to disable flock.
    int EnableFlock(IFunctionHandler* pH, SmartScriptTable entity, bool bEnable);

    // <title SetFlockPercentEnabled>
    // Syntax: Boids.SetFlockPercentEnabled( entity,paramsTable )
    // Description:
    //    Used to gradually enable flock.
    //    Depending on the percentage more or less boid objects will be rendered in flock.
    // Arguments:
    //    entity - Valid entity table containing flock.
    //    nPercent - In range 0 to 100, 0 mean no boids will be rendered,if 100 then all boids will be rendered.
    int SetFlockPercentEnabled(IFunctionHandler* pH, SmartScriptTable entity, int nPercent);

    // <title SetAttractionPoint>
    // Syntax: Boids.SetAttractionPoint( entity,paramsTable )
    // Description:
    //    Sets the one time attraction point for the boids
    // Arguments:
    //    entity - Valid entity table containing flock.
    //    point - The one time attraction point
    int SetAttractionPoint(IFunctionHandler* pH, SmartScriptTable entity, Vec3 point);

    // <title OnBoidHit>
    // Syntax: Boids.OnBoidHit( flockEntity,boidEntity,hit )
    // Description:
    //    Events that occurs on boid hit.
    // Arguments:
    //    flockEntity - Valid entity table containing flock.
    //    boidEntity - Valid entity table containing boid.
    //    hit - Valid entity table containing hit information..
    int OnBoidHit(IFunctionHandler* pH, SmartScriptTable flockEntity, SmartScriptTable boidEntity,
        SmartScriptTable hit);

    // <title CanPickup>
    // Syntax: Boids.CanPickup( flockEntity, boidEntity )
    // Description:
    //    Checks if the boid is pickable
    // Arguments:
    //    flockEntity - Valid entity table containing flock.
    //    boidEntity - Valid entity table containing boid.
    int CanPickup(IFunctionHandler* pH, SmartScriptTable flockEntity, SmartScriptTable boidEntity);

    // <title GetUsableMessage>
    // Syntax: Boids.GetUsableMessage( flockEntity )
    // Description:
    //    Gets the appropriate localized UI message for this flock
    // Arguments:
    //    flockEntity - Valid entity table containing flock.
    int GetUsableMessage(IFunctionHandler* pH, SmartScriptTable flockEntity);

    // <title OnPickup>
    // Syntax: Boids.OnPickup( flockEntity, boidEntity, bPickup, fThrowSpeed )
    // Description:
    //    Forwards the appropriate pickup action to the boid object
    // Arguments:
    //    flockEntity - Valid entity table containing flock.
    //    boidEntity - Valid entity table containing boid.
    //    bPickup - Pickup or drop/throw?
    //    fThrowSpeed - value > 5.f will kill the boid by default (no effect on pickup action)
    int OnPickup(IFunctionHandler* pH, SmartScriptTable flockEntity, SmartScriptTable boidEntity, bool bPickup,
        float fThrowSpeed);

private:
    bool ReadParamsTable(IScriptTable* pTable, struct SBoidContext& bc, SBoidsCreateContext& ctx);
    IEntity* GetEntity(IScriptTable* pEntityTable);
    CFlock* GetFlock(IScriptTable* pEntityTable);

    int CommonCreateFlock(int type, IFunctionHandler* pH, SmartScriptTable entity, SmartScriptTable paramTable);

    ISystem* m_pSystem;
    IScriptSystem* m_pScriptSystem;

    DeclareConstIntCVar(boids_enabled, 1);
};

#endif // CRYINCLUDE_GAMEDLL_BOIDS_SCRIPTBIND_BOIDS_H
