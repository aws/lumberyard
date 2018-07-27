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

// Description : Implements a base class for vehicle flow node

#include "CryLegacy_precompiled.h"
#include "CryAction.h"
#include "IVehicleSystem.h"
#include "IFlowSystem.h"
#include "FlowSystem/Nodes/FlowBaseNode.h"
#include "FlowVehicleBase.h"

//------------------------------------------------------------------------
void CFlowVehicleBase::Init(SActivationInfo* pActivationInfo)
{
    m_nodeID = pActivationInfo->myID;
    m_pGraph = pActivationInfo->pGraph;

    if (IEntity* pEntity = pActivationInfo->pEntity)
    {
        IVehicleSystem* pVehicleSystem = CCryAction::GetCryAction()->GetIVehicleSystem();
        CRY_ASSERT(pVehicleSystem);

        if (pVehicleSystem->GetVehicle(pEntity->GetId()))
        {
            m_vehicleId = pEntity->GetId();
        }
    }
    else
    {
        m_vehicleId = 0;
    }
}

//------------------------------------------------------------------------
void CFlowVehicleBase::Delete()
{
    if (IVehicle* pVehicle = GetVehicle())
    {
        pVehicle->UnregisterVehicleEventListener(this);
    }
}

//------------------------------------------------------------------------
void CFlowVehicleBase::GetConfiguration(SFlowNodeConfig& nodeConfig)
{
}

//------------------------------------------------------------------------
void CFlowVehicleBase::ProcessEvent(EFlowEvent flowEvent, SActivationInfo* pActivationInfo)
{
    if (flowEvent == eFE_SetEntityId)
    {
        IEntity* pEntity = pActivationInfo->pEntity;

        if (pEntity)
        {
            IVehicleSystem* pVehicleSystem = CCryAction::GetCryAction()->GetIVehicleSystem();
            CRY_ASSERT(pVehicleSystem);

            if (pEntity->GetId() != m_vehicleId)
            {
                if (IVehicle* pVehicle = GetVehicle())
                {
                    pVehicle->UnregisterVehicleEventListener(this);
                }

                m_vehicleId = 0;
            }

            if (IVehicle* pVehicle = pVehicleSystem->GetVehicle(pEntity->GetId()))
            {
                pVehicle->RegisterVehicleEventListener(this, "FlowVehicleBase");
                m_vehicleId = pEntity->GetId();
            }
        }
        else
        {
            if (IVehicle* pVehicle = GetVehicle())
            {
                pVehicle->UnregisterVehicleEventListener(this);
            }
        }
    }
}

//------------------------------------------------------------------------
void CFlowVehicleBase::Serialize(SActivationInfo* pActivationInfo, TSerialize ser)
{
}

//------------------------------------------------------------------------
void CFlowVehicleBase::OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params)
{
    if (event == eVE_VehicleDeleted)
    {
        m_vehicleId = 0;
    }
}

//------------------------------------------------------------------------
IVehicle* CFlowVehicleBase::GetVehicle()
{
    if (!m_vehicleId)
    {
        return NULL;
    }

    return CCryAction::GetCryAction()->GetIVehicleSystem()->GetVehicle(m_vehicleId);
}
