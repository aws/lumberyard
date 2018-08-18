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

// Description : Implements a part for vehicles which uses animated characters


#include "CryLegacy_precompiled.h"

#include "ICryAnimation.h"
#include "IVehicleSystem.h"

#include "CryAction.h"
#include "Vehicle.h"
#include "VehiclePartBase.h"
#include "VehiclePartAnimated.h"
#include "VehiclePartAnimatedChar.h"
#include "VehicleUtils.h"

//#pragma optimize("", off)
//#pragma inline_depth(0)

//------------------------------------------------------------------------
CVehiclePartAnimatedChar::CVehiclePartAnimatedChar()
{
}

//------------------------------------------------------------------------
CVehiclePartAnimatedChar::~CVehiclePartAnimatedChar()
{
}

//------------------------------------------------------------------------
void CVehiclePartAnimatedChar::Physicalize()
{
    if (m_pSharedParameters->m_isPhysicalized && GetEntity()->GetPhysics())
    {
        if (m_slot != -1)
        {
            GetEntity()->UnphysicalizeSlot(m_slot);
        }

        SEntityPhysicalizeParams params;
        params.mass = m_pSharedParameters->m_mass;
        params.density = m_pSharedParameters->m_density;

        //TODO: Check civcar for 'proper' slot allocation
        params.nSlot = 0; //m_slot;
        params.type = PE_LIVING;
        GetEntity()->PhysicalizeSlot(m_slot, params); // always returns -1 for chars

        if (m_pCharInstance)
        {
            FlagSkeleton(m_pCharInstance->GetISkeletonPose(), m_pCharInstance->GetIDefaultSkeleton());
        }

        m_pVehicle->RequestPhysicalization(this, false);
    }

    GetEntity()->SetSlotFlags(m_slot, GetEntity()->GetSlotFlags(m_slot) | ENTITY_SLOT_IGNORE_PHYSICS);
}

DEFINE_VEHICLEOBJECT(CVehiclePartAnimatedChar);
