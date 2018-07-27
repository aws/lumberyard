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

// Description : Implements an entity class for detached parts


#include "CryLegacy_precompiled.h"

#include <IViewSystem.h>
#include <IItemSystem.h>
#include <IVehicleSystem.h>
#include <IPhysics.h>
#include <ICryAnimation.h>
#include <IActorSystem.h>
#include <ISerialize.h>
#include <IAgent.h>

#include "CryAction.h"
#include "Vehicle.h"
#include "VehiclePartDetachedEntity.h"
#include "VehiclePartBase.h"

DECLARE_DEFAULT_COMPONENT_FACTORY(CVehiclePartDetachedEntity, CVehiclePartDetachedEntity)

namespace VPDE
{
    void RegisterEvents(IGameObjectExtension& goExt, IGameObject& gameObject)
    {
        const int eventID = eGFE_OnCollision;
        gameObject.UnRegisterExtForEvents(&goExt, NULL, 0);
        gameObject.RegisterExtForEvents(&goExt, &eventID, 1);
    }
}

//------------------------------------------------------------------------
CVehiclePartDetachedEntity::CVehiclePartDetachedEntity()
{
}

//------------------------------------------------------------------------
CVehiclePartDetachedEntity::~CVehiclePartDetachedEntity()
{
    GetGameObject()->EnablePhysicsEvent(false, eEPE_OnCollisionLogged);
}

//------------------------------------------------------------------------
bool CVehiclePartDetachedEntity::Init(IGameObject* pGameObject)
{
    SetGameObject(pGameObject);

    InitVehiclePart(pGameObject);

    return true;
}

//------------------------------------------------------------------------
void CVehiclePartDetachedEntity::PostInit(IGameObject* pGameObject)
{
    VPDE::RegisterEvents(*this, *pGameObject);
}

//------------------------------------------------------------------------
bool CVehiclePartDetachedEntity::ReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params)
{
    ResetGameObject();

    VPDE::RegisterEvents(*this, *pGameObject);

    InitVehiclePart(pGameObject);

    return true;
}

//------------------------------------------------------------------------
void CVehiclePartDetachedEntity::InitVehiclePart(IGameObject* pGameObject)
{
    assert(pGameObject);

    // Set so we receive render events (when GO is set to update due to being visible), allowing last seen timer to reset
    pGameObject->EnableUpdateSlot(this, 0);
    pGameObject->SetUpdateSlotEnableCondition(this, 0, eUEC_VisibleAndInRange);

    if (IPhysicalEntity* pPhysics = GetEntity()->GetPhysics())
    {
        pe_params_flags physFlags;

        if (pPhysics->GetParams(&physFlags))
        {
            physFlags.flags |= pef_log_collisions;
            pPhysics->SetParams(&physFlags);

            pGameObject->EnablePhysicsEvent(true, eEPE_OnCollisionLogged);
        }
    }

    m_timeUntilStartIsOver = 10.0f;
}

//------------------------------------------------------------------------
bool CVehiclePartDetachedEntity::GetEntityPoolSignature(TSerialize signature)
{
    signature.BeginGroup("VehiclePartDetachedEntity");
    signature.EndGroup();

    return true;
}

//------------------------------------------------------------------------
void CVehiclePartDetachedEntity::Update(SEntityUpdateContext& ctx, int slot)
{
    const float frameTime = ctx.fFrameTime;

    switch (slot)
    {
    case 0:
    {
        if (m_timeUntilStartIsOver > 0.0f)
        {
            m_timeUntilStartIsOver -= frameTime;
        }
    }
    break;
    }
}

//------------------------------------------------------------------------
void CVehiclePartDetachedEntity::ProcessEvent(SEntityEvent& event)
{
    if (event.event == ENTITY_EVENT_RESET)
    {
        gEnv->pEntitySystem->RemoveEntity(GetEntity()->GetId());
    }
}

//------------------------------------------------------------------------
void CVehiclePartDetachedEntity::HandleEvent(const SGameObjectEvent& event)
{
    if (event.event == eGFE_OnCollision)
    {
        if (m_timeUntilStartIsOver <= 0.0f)
        {
            const EventPhysCollision* pCollision = static_cast<const EventPhysCollision*>(event.ptr);
            IEntitySystem* pES = gEnv->pEntitySystem;
            IEntity* pE1 = pES->GetEntityFromPhysics(pCollision->pEntity[0]);
            IEntity* pE2 = pES->GetEntityFromPhysics(pCollision->pEntity[1]);
            IEntity* pCollider = pE1 == GetEntity() ? pE2 : pE1;

            if (pCollider)
            {
                //OnCollision( pCollider->GetId(), pCollision->pt, pCollision->n );
                IEntity* pEntity = GetEntity();
                CRY_ASSERT(pEntity);

                int slotCount = pEntity->GetSlotCount();
                if (slotCount > 1)
                {
                    for (int i = 1; i < slotCount; i++)
                    {
                        if (pEntity->IsSlotValid(i))
                        {
                            pEntity->FreeSlot(i);
                        }
                    }
                }
            }
        }
    }
}

void CVehiclePartDetachedEntity::FullSerialize(TSerialize ser)
{
    bool hasGeom = false;
    if (ser.IsWriting())
    {
        if (GetEntity()->GetStatObj(0))
        {
            hasGeom = true;
        }
    }
    ser.Value("hasGeometry", hasGeom);
    if (hasGeom)
    {
        if (ser.IsWriting())
        {
            gEnv->p3DEngine->SaveStatObj(GetEntity()->GetStatObj(0), ser);
            if (GetEntity()->GetPhysics())
            {
                GetEntity()->GetPhysics()->GetStateSnapshot(ser);
            }
        }
        else if (ser.IsReading())
        {
            IStatObj* pStatObj = gEnv->p3DEngine->LoadStatObj(ser);
            if (pStatObj)
            {
                GetEntity()->SetStatObj(pStatObj, 0, true, 200.0f);
                SEntityPhysicalizeParams physicsParams;
                if (!pStatObj->GetPhysicalProperties(physicsParams.mass, physicsParams.density))
                {
                    physicsParams.mass = 200.0f;
                }
                physicsParams.type = PE_RIGID;
                physicsParams.nFlagsOR &= pef_log_collisions;
                physicsParams.nSlot = 0;
                GetEntity()->Physicalize(physicsParams);
                if (GetEntity()->GetPhysics())
                {
                    GetEntity()->GetPhysics()->SetStateFromSnapshot(ser);
                }
            }
        }
    }
}
