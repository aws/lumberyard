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

// Description : Implements a damage behavior which notify the movement class


#include "CryLegacy_precompiled.h"
#include "IVehicleSystem.h"
#include "Vehicle.h"
#include "VehicleDamageBehaviorMovementNotification.h"

//------------------------------------------------------------------------
CVehicleDamageBehaviorMovementNotification::CVehicleDamageBehaviorMovementNotification()
    : m_isDamageAlwaysFull(false)
    , m_isFatal(true)
    , m_isSteeringInvolved(false)
    , m_pVehicle(NULL)
{
}

//------------------------------------------------------------------------
bool CVehicleDamageBehaviorMovementNotification::Init(IVehicle* pVehicle, const CVehicleParams& table)
{
    CRY_ASSERT(pVehicle);
    m_pVehicle = pVehicle;

    m_param = 0;

    if (CVehicleParams notificationParams = table.findChild("MovementNotification"))
    {
        notificationParams.getAttr("isSteering", m_isSteeringInvolved);
        notificationParams.getAttr("isFatal", m_isFatal);
        notificationParams.getAttr("param", m_param);
        notificationParams.getAttr("isDamageAlwaysFull", m_isDamageAlwaysFull);
    }

    return true;
}

//------------------------------------------------------------------------
void CVehicleDamageBehaviorMovementNotification::Reset()
{
}

//------------------------------------------------------------------------
void CVehicleDamageBehaviorMovementNotification::OnDamageEvent(EVehicleDamageBehaviorEvent event, const SVehicleDamageBehaviorEventParams& behaviorParams)
{
    if (event == eVDBE_Hit || event == eVDBE_VehicleDestroyed || event == eVDBE_Repair)
    {
        IVehicleMovement::EVehicleMovementEvent eventType;

        if (event == eVDBE_Repair)
        {
            eventType = IVehicleMovement::eVME_Repair;
        }
        else if (m_isSteeringInvolved)
        {
            eventType = IVehicleMovement::eVME_DamageSteering;
        }
        else
        {
            eventType = IVehicleMovement::eVME_Damage;
        }

        SVehicleMovementEventParams params;
        if (m_isDamageAlwaysFull)
        {
            if (event != eVDBE_Repair)
            {
                params.fValue = 1.0f;
            }
            else
            {
                params.fValue = 0.0f;
            }
        }
        else
        {
            if (event != eVDBE_Repair)
            {
                params.fValue = min(1.0f, behaviorParams.componentDamageRatio);
            }
            else
            {
                // round down to nearest 0.25 (ensures movement object is sent zero damage on fully repairing vehicle).
                params.fValue = ((int)(behaviorParams.componentDamageRatio * 4)) / 4.0f;
            }
        }

        params.vParam = behaviorParams.localPos;
        params.bValue = m_isFatal;
        params.iValue = m_param;
        params.pComponent = behaviorParams.pVehicleComponent;

        m_pVehicle->GetMovement()->OnEvent(eventType, params);
    }
}

DEFINE_VEHICLEOBJECT(CVehicleDamageBehaviorMovementNotification);
