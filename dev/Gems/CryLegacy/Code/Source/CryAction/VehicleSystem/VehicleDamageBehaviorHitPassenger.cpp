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

// Description : Implements a damage behavior group which gives hit to the
//               passenger inside the vehicle


#include "CryLegacy_precompiled.h"
#include "IActorSystem.h"
#include "IVehicleSystem.h"
#include "VehicleSystem/Vehicle.h"
#include "VehicleDamageBehaviorHitPassenger.h"
#include "IGameRulesSystem.h"

//------------------------------------------------------------------------
CVehicleDamageBehaviorHitPassenger::CVehicleDamageBehaviorHitPassenger()
{
}

//------------------------------------------------------------------------
bool CVehicleDamageBehaviorHitPassenger::Init(IVehicle* pVehicle, const CVehicleParams& table)
{
    m_pVehicle = pVehicle;

    CVehicleParams hitPassTable = table.findChild("HitPassenger");
    if (!hitPassTable)
    {
        return false;
    }

    if (!hitPassTable.getAttr("damage", m_damage))
    {
        return false;
    }

    if (!hitPassTable.getAttr("isDamagePercent", m_isDamagePercent))
    {
        m_isDamagePercent = false;
    }

    return true;
}

//------------------------------------------------------------------------
void CVehicleDamageBehaviorHitPassenger::OnDamageEvent(EVehicleDamageBehaviorEvent event, const SVehicleDamageBehaviorEventParams& behaviorParams)
{
    if (event != eVDBE_Hit)
    {
        return;
    }

    IActorSystem* pActorSystem = gEnv->pGame->GetIGameFramework()->GetIActorSystem();
    CRY_ASSERT(pActorSystem);

    for (TVehicleSeatId seatId = InvalidVehicleSeatId + 1;
         seatId != m_pVehicle->GetLastSeatId(); seatId++)
    {
        if (IVehicleSeat* pSeat = m_pVehicle->GetSeatById(seatId))
        {
            EntityId passengerId = pSeat->GetPassenger();

            if (IActor* pActor = pActorSystem->GetActor(passengerId))
            {
                int damage = m_damage;

                if (m_isDamagePercent)
                {
                    damage = (int)(pActor->GetMaxHealth() * float(m_damage) / 100.f);
                }

                if (gEnv->bServer)
                {
                    if (IGameRules* pGameRules = gEnv->pGame->GetIGameFramework()->GetIGameRulesSystem()->GetCurrentGameRules())
                    {
                        HitInfo hit;

                        hit.targetId = pActor->GetEntityId();
                        hit.shooterId = behaviorParams.shooterId;
                        hit.weaponId = 0;
                        hit.damage = (float)damage;
                        hit.type = 0;

                        pGameRules->ServerHit(hit);
                    }
                }
            }
        }
    }
}

//------------------------------------------------------------------------
void CVehicleDamageBehaviorHitPassenger::GetMemoryUsage(ICrySizer* s) const
{
    s->Add(*this);
}

DEFINE_VEHICLEOBJECT(CVehicleDamageBehaviorHitPassenger);
