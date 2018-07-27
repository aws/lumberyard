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

// Description : Implements a damage behavior which sink the vehicle


#include "CryLegacy_precompiled.h"
#include "IVehicleSystem.h"
#include "Vehicle.h"
#include "VehicleDamageBehaviorSink.h"

//------------------------------------------------------------------------
bool CVehicleDamageBehaviorSink::Init(IVehicle* pVehicle, const CVehicleParams& table)
{
    m_pVehicle = pVehicle;
    m_isSinking = false;
    m_sinkingTimer = -1;

    // save the initial buoyancy here (prevents problems with multiple versions of this damage behaviour acting on the same boat)
    if (IPhysicalEntity* pPhysEntity = m_pVehicle->GetEntity()->GetPhysics())
    {
        pe_params_buoyancy buoyancyParams;
        if (pPhysEntity->GetParams(&buoyancyParams))
        {
            m_formerWaterDensity = buoyancyParams.kwaterDensity;
        }
    }

    return true;
}

//------------------------------------------------------------------------
void CVehicleDamageBehaviorSink::Reset()
{
    if (m_isSinking)
    {
        ChangeSinkingBehavior(false);
    }
}

//------------------------------------------------------------------------
void CVehicleDamageBehaviorSink::OnDamageEvent(EVehicleDamageBehaviorEvent event, const SVehicleDamageBehaviorEventParams& behaviorParams)
{
    if (event == eVDBE_Hit || event == eVDBE_VehicleDestroyed || event == eVDBE_ComponentDestroyed)
    {
        if (behaviorParams.componentDamageRatio >= 1.0f)
        {
            ChangeSinkingBehavior(true);
        }
    }
    else if (event == eVDBE_Repair && behaviorParams.componentDamageRatio < 1.f)
    {
        ChangeSinkingBehavior(false);
    }
}

//------------------------------------------------------------------------
void CVehicleDamageBehaviorSink::ChangeSinkingBehavior(bool isSinking)
{
    IEntity* pEntity = m_pVehicle->GetEntity();
    CRY_ASSERT(pEntity);

    if (isSinking && !m_isSinking)
    {
        if (IPhysicalEntity* pPhysEntity = pEntity->GetPhysics())
        {
            pe_params_buoyancy buoyancyParams;

            if (pPhysEntity->GetParams(&buoyancyParams))
            {
                //m_formerWaterDensity = buoyancyParams.kwaterDensity;
                buoyancyParams.kwaterDensity *= 0.5f;

                pPhysEntity->SetParams(&buoyancyParams);
            }
        }

        m_sinkingTimer = m_pVehicle->SetTimer(-1, 10000, this); // starts complete sinking
        m_isSinking = true;
    }
    else if (!isSinking && m_isSinking)
    {
        if (IPhysicalEntity* pPhysEntity = pEntity->GetPhysics())
        {
            pe_params_buoyancy buoyancyParams;

            if (pPhysEntity->GetParams(&buoyancyParams))
            {
                buoyancyParams.kwaterDensity = m_formerWaterDensity;
                pPhysEntity->SetParams(&buoyancyParams);
            }
        }

        m_sinkingTimer = m_pVehicle->KillTimer(m_sinkingTimer);
        m_isSinking = false;
    }
}

//------------------------------------------------------------------------
void CVehicleDamageBehaviorSink::OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params)
{
    switch (event)
    {
    case eVE_Timer:
    {
        if (IPhysicalEntity* pPhysEntity = m_pVehicle->GetEntity()->GetPhysics())
        {
            // decrease water density further
            pe_params_buoyancy buoyancyParams;

            if (pPhysEntity->GetParams(&buoyancyParams))
            {
                buoyancyParams.kwaterDensity -= 0.05f * m_formerWaterDensity;

                pPhysEntity->SetParams(&buoyancyParams);

                if (buoyancyParams.kwaterDensity > 0.05f * m_formerWaterDensity)
                {
                    m_sinkingTimer = m_pVehicle->SetTimer(-1, 1000, this);
                }
            }
        }
    }
    break;
    }
}

//------------------------------------------------------------------------
void CVehicleDamageBehaviorSink::Serialize(TSerialize serialize, EEntityAspects)
{
    if (serialize.GetSerializationTarget() != eST_Network)
    {
        serialize.Value("sinkTimer", m_sinkingTimer);

        bool isNowSinking = m_isSinking;
        serialize.Value("isSinking", isNowSinking);

        if (serialize.IsReading())
        {
            ChangeSinkingBehavior(isNowSinking);
        }
    }
}

DEFINE_VEHICLEOBJECT(CVehicleDamageBehaviorSink);
