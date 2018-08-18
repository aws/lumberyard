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

// Description : Implements several utility functions for vehicles


#include "CryLegacy_precompiled.h"

#include "VehicleUtils.h"

#include "CryAction.h"
#include "PersistentDebug.h"

#include "Vehicle.h"
#include "VehicleAnimation.h"
#include "VehicleCVars.h"
#include "VehicleComponent.h"
#include "VehicleHelper.h"
#include "VehicleSeat.h"
#include "VehicleSeatGroup.h"
#include "VehicleSeatActionWeapons.h"


//------------------------------------------------------------------------
void VehicleUtils::DumpDamageBehaviorEvent(const SVehicleDamageBehaviorEventParams& params)
{
#if ENABLE_VEHICLE_DEBUG
    CryLog("=================================");
    CryLog("SVehicleDamageBehaviorEventParams");
    CryLog("localPos (%.2f, %.2f, %.2f)", params.localPos.x, params.localPos.y, params.localPos.z);
    CryLog("shooterId: %u", params.shooterId);
    CryLog("radius: %.2f", params.radius);
    CryLog("hitValue: %.2f", params.hitValue);
    CryLog("componentDamageRatio: %.2f", params.componentDamageRatio);
    CryLog("randomness: %.1f", params.randomness);

    if (params.pVehicleComponent)
    {
        CryLog("pVehicleComponent: <%s>", ((CVehicleComponent*)params.pVehicleComponent)->GetName().c_str());
    }
#endif
}


//------------------------------------------------------------------------
void VehicleUtils::DrawTM(const Matrix34& tm, const char* name, bool clear)
{
#if ENABLE_VEHICLE_DEBUG
    IPersistentDebug* pDebug = CCryAction::GetCryAction()->GetIPersistentDebug();
    pDebug->Begin(name, clear);

    const static ColorF red(1, 0, 0, 1);
    const static ColorF green(0, 1, 0, 1);
    const static ColorF blue(0, 0, 1, 1);

    float timeout = 0.1f;
    float radius = 1.5f;

    pDebug->AddDirection(tm.GetTranslation(), radius, tm.GetColumn(0), red, timeout);
    pDebug->AddDirection(tm.GetTranslation(), radius, tm.GetColumn(1), green, timeout);
    pDebug->AddDirection(tm.GetTranslation(), radius, tm.GetColumn(2), blue, timeout);
#endif
}

//------------------------------------------------------------------------
void VehicleUtils::DumpSlots(IVehicle* pVehicle)
{
#if ENABLE_VEHICLE_DEBUG
    if (VehicleCVars().v_debugdraw)
    {
        IEntity* pEntity = pVehicle->GetEntity();

        for (int i = 0; i < pEntity->GetSlotCount(); ++i)
        {
            SEntitySlotInfo info;
            if (pEntity->GetSlotInfo(i, info))
            {
                if (info.pCharacter)
                {
                    CryLog("slot %i: <char> %s", i, info.pCharacter->GetFilePath());
                }
                else if (info.pStatObj)
                {
                    CryLog("slot %i: <statobj> %s", i, info.pStatObj->GetGeoName());
                }
                else if (info.pParticleEmitter)
                {
                    CryLog("slot %i: <particle> %s", i, info.pParticleEmitter->GetName());
                }
                else
                {
                    CryLog("slot %i: <UNKNOWN>", i);
                }
            }
        }
    }
#endif
}