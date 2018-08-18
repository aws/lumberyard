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

// Description : Subclass of VehiclePartEntity that can be asked to detach at a random point in the future


#include "CryLegacy_precompiled.h"

//This Include
#include "VehiclePartEntityDelayedDetach.h"


//------------------------------------------------------------------------
CVehiclePartEntityDelayedDetach::CVehiclePartEntityDelayedDetach()
    : CVehiclePartEntity()
    , m_detachTimer(-1.0f)
{
}

//------------------------------------------------------------------------
CVehiclePartEntityDelayedDetach::~CVehiclePartEntityDelayedDetach()
{
}

//------------------------------------------------------------------------
void CVehiclePartEntityDelayedDetach::Update(const float frameTime)
{
    CVehiclePartEntity::Update(frameTime);

    if (EntityAttached() && m_detachTimer >= 0.0f)
    {
        m_detachTimer -= frameTime;

        if (m_detachTimer <= 0.0f)
        {
            SVehicleEventParams vehicleEventParams;
            vehicleEventParams.entityId = GetPartEntityId();
            vehicleEventParams.iParam       = m_pVehicle->GetEntityId();

            m_pVehicle->BroadcastVehicleEvent(eVE_OnDetachPartEntity, vehicleEventParams);

            m_detachTimer = -1.0f;
        }
    }
}

//------------------------------------------------------------------------
void CVehiclePartEntityDelayedDetach::OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params)
{
    CVehiclePartEntity::OnVehicleEvent(event,  params);

    switch (event)
    {
    case eVE_RequestDelayedDetachAllPartEntities:
    {
        //we're a part entity, so want to detach.
        //don't reset timer if all ready set
        if (m_detachTimer < 0.0f && EntityAttached())
        {
            //random time between min + max wait
            m_detachTimer = cry_random(params.fParam, params.fParam2);
        }
        break;
    }

    case eVE_Sleep:
    {
        //if we were scheduled to delay detach, do so now as we won't receive further updates
        if (m_detachTimer >= 0.0f && EntityAttached())
        {
            m_detachTimer = -1.0f;

            SVehicleEventParams vehicleEventParams;

            vehicleEventParams.entityId = GetPartEntityId();
            vehicleEventParams.iParam       = m_pVehicle->GetEntityId();

            m_pVehicle->BroadcastVehicleEvent(eVE_OnDetachPartEntity, vehicleEventParams);
        }
        break;
    }
    }
}

//------------------------------------------------------------------------
DEFINE_VEHICLEOBJECT(CVehiclePartEntityDelayedDetach)