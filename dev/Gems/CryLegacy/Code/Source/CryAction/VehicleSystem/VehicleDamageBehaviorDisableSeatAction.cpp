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

// Description : Implements a damage behavior which disables
//               seat actions for a specified seat


#include "CryLegacy_precompiled.h"

#include "VehicleDamageBehaviorDisableSeatAction.h"

#include "IVehicleSystem.h"

#include "Vehicle.h"
#include "VehicleSeat.h"

CVehicleDamageBehaviorDisableSeatAction::CVehicleDamageBehaviorDisableSeatAction()
    : m_pVehicle(NULL)
{
}

//------------------------------------------------------------------------
bool CVehicleDamageBehaviorDisableSeatAction::Init(IVehicle* pVehicle, const CVehicleParams& table)
{
    m_pVehicle = (CVehicle*) pVehicle;

    CVehicleParams actionTable = table.findChild("DisableSeatAction");
    m_seatName = actionTable.getAttr("seat");

    m_seatActionName = actionTable.getAttr("actionName");

    return true;
}

//------------------------------------------------------------------------
void CVehicleDamageBehaviorDisableSeatAction::Reset()
{
}

//------------------------------------------------------------------------
void CVehicleDamageBehaviorDisableSeatAction::OnDamageEvent(EVehicleDamageBehaviorEvent event, const SVehicleDamageBehaviorEventParams& behaviorParams)
{
    if (event != eVDBE_Hit)
    {
        return;
    }

    TVehicleSeatId seatId = m_pVehicle->GetSeatId(m_seatName.c_str());

    if (seatId == InvalidVehicleSeatId)
    {
        CryLog("DisableSeatAction damage behavior referencing invalid vehicle seat (%s)", m_seatName.c_str());
        return;
    }

    if (CVehicleSeat* pSeat = static_cast<CVehicleSeat*>(m_pVehicle->GetSeatById(seatId)))
    {
        bool all = (m_seatActionName == "all");
        TVehicleSeatActionVector& actions = pSeat->GetSeatActions();
        for (TVehicleSeatActionVector::iterator ite = actions.begin(), end = actions.end(); ite != end; ++ite)
        {
            if (all || (m_seatActionName == ite->pSeatAction->GetName()))
            {
                ite->pSeatAction->StopUsing();

                ite->isEnabled = false;
            }
        }
    }
}


DEFINE_VEHICLEOBJECT(CVehicleDamageBehaviorDisableSeatAction);
