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

// Description : Implements a damage behavior group which handles delays and
//               some randomness


#include "CryLegacy_precompiled.h"
#include "IVehicleSystem.h"
#include "VehicleSystem/Vehicle.h"
#include "VehicleDamageBehaviorGroup.h"
#include "VehicleDamagesGroup.h"

//------------------------------------------------------------------------
bool CVehicleDamageBehaviorGroup::Init(IVehicle* pVehicle, const CVehicleParams& table)
{
    m_pVehicle = (CVehicle*) pVehicle;

    CVehicleParams groupParams = table.findChild("Group");
    if (!groupParams)
    {
        return false;
    }

    if (!groupParams.haveAttr("name"))
    {
        return false;
    }

    m_damageGroupName = groupParams.getAttr("name");
    return !m_damageGroupName.empty();
}

//------------------------------------------------------------------------
void CVehicleDamageBehaviorGroup::Reset()
{
}

//------------------------------------------------------------------------
void CVehicleDamageBehaviorGroup::OnDamageEvent(EVehicleDamageBehaviorEvent event, const SVehicleDamageBehaviorEventParams& behaviorParams)
{
    if (CVehicleDamagesGroup* pDamagesGroup = m_pVehicle->GetDamagesGroup(m_damageGroupName.c_str()))
    {
        pDamagesGroup->OnDamageEvent(event, behaviorParams);
    }
}

//------------------------------------------------------------------------
void CVehicleDamageBehaviorGroup::Serialize(TSerialize ser, EEntityAspects aspects)
{
}

//------------------------------------------------------------------------
void CVehicleDamageBehaviorGroup::Update(const float deltaTime)
{
}

void CVehicleDamageBehaviorGroup::GetMemoryUsage(ICrySizer* s) const
{
    s->AddObject(this, sizeof(*this));
    s->AddObject(m_damageGroupName);
}

DEFINE_VEHICLEOBJECT(CVehicleDamageBehaviorGroup);
