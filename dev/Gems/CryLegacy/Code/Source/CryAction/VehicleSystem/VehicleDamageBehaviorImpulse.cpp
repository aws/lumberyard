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

// Description : Implements a damage behavior which apply an impulse to a
//               vehicle


#include "CryLegacy_precompiled.h"

#include "IVehicleSystem.h"

#include "Vehicle.h"
#include "VehicleDamageBehaviorImpulse.h"


//------------------------------------------------------------------------
CVehicleDamageBehaviorImpulse::CVehicleDamageBehaviorImpulse()
{
    m_pVehicle = 0;
    m_forceMin = 0.f;
    m_forceMax = 0.f;
    m_impulseDir.zero();
    m_worldSpace = false;
    m_angImpulse.zero();
    m_pImpulseLocation = 0;
}

//------------------------------------------------------------------------
bool CVehicleDamageBehaviorImpulse::Init(IVehicle* pVehicle, const CVehicleParams& table)
{
    m_pVehicle = (CVehicle*) pVehicle;

    CVehicleParams impulseParams = table.findChild("Impulse");
    if (!impulseParams)
    {
        return false;
    }

    impulseParams.getAttr("worldSpace", m_worldSpace);
    impulseParams.getAttr("forceMin", m_forceMin);
    impulseParams.getAttr("forceMax", m_forceMax);

    impulseParams.getAttr("direction", m_impulseDir);
    m_impulseDir.NormalizeSafe(Vec3(0, 0, 1));

    if (!impulseParams.getAttr("momentum", m_angImpulse))
    {
        m_angImpulse.zero();
    }

    if (impulseParams.haveAttr("helper"))
    {
        m_pImpulseLocation = m_pVehicle->GetHelper(impulseParams.getAttr("helper"));
    }
    else
    {
        m_pImpulseLocation = NULL;
    }

    return true;
}

//------------------------------------------------------------------------
void CVehicleDamageBehaviorImpulse::Reset()
{
}

//------------------------------------------------------------------------
void CVehicleDamageBehaviorImpulse::OnDamageEvent(EVehicleDamageBehaviorEvent event, const SVehicleDamageBehaviorEventParams& behaviorParams)
{
    if (event == eVDBE_Hit || event == eVDBE_VehicleDestroyed || event == eVDBE_ComponentDestroyed)
    {
        IEntity* pEntity = m_pVehicle->GetEntity();
        CRY_ASSERT(pEntity);

        IPhysicalEntity* pPhysEntity = pEntity->GetPhysics();
        if (!pPhysEntity)
        {
            return;
        }

        pe_status_dynamics dyn;
        pPhysEntity->GetStatus(&dyn);
        float vehicleMass = dyn.mass;

        float r = cry_random(0.0f, 2.f);
        float impulseForce = cry_random(m_forceMin, m_forceMax) * vehicleMass;

        Vec3 impulseDir(m_impulseDir);
        if (!m_worldSpace)
        {
            impulseDir = m_pVehicle->GetEntity()->GetWorldTM().TransformVector(impulseDir);
        }

        pe_action_impulse actionImpulse;
        Vec3& impulse = actionImpulse.impulse;
        Vec3& angImpulse = actionImpulse.angImpulse;

        impulse = impulseDir * impulseForce;
        angImpulse = m_pVehicle->GetEntity()->GetWorldTM().TransformVector(m_angImpulse);

        if (r <= 0.75f)
        {
            float r1 = cry_random(-0.35f, 0.35f);

            angImpulse += dyn.v * r1 * max(1.0f, dyn.w.GetLength());
            angImpulse *= vehicleMass;
        }
        else
        {
            float r1 = cry_random(-0.25f, 0.25f);
            float r2 = cry_random(-0.5f, 0.5f);

            impulse.z += abs(dyn.v.y) * r1 * vehicleMass;
            angImpulse.x += dyn.v.y * r2 * vehicleMass * max(1.0f, dyn.w.GetLength() * 1.5f);
        }

        if (m_pImpulseLocation)
        {
            actionImpulse.point = m_pImpulseLocation->GetWorldSpaceTranslation();
        }

        pPhysEntity->Action(&actionImpulse);
    }
}

DEFINE_VEHICLEOBJECT(CVehicleDamageBehaviorImpulse);
