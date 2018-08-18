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

// Description : Implements a flow node to enable/disable vehicle handbrake.

#include "CryLegacy_precompiled.h"
#include "CryAction.h"
#include "IVehicleSystem.h"
#include "IFlowSystem.h"
#include "FlowSystem/Nodes/FlowBaseNode.h"
#include "VehicleSystem/Vehicle.h"
#include "FlowVehicleHandbrake.h"

//------------------------------------------------------------------------
CFlowVehicleHandbrake::CFlowVehicleHandbrake(SActivationInfo* pActivationInfo)
{
    Init(pActivationInfo);
}

//------------------------------------------------------------------------
CFlowVehicleHandbrake::~CFlowVehicleHandbrake()
{
    Delete();
}

//------------------------------------------------------------------------
IFlowNodePtr CFlowVehicleHandbrake::Clone(SActivationInfo* pActivationInfo)
{
    return new CFlowVehicleHandbrake(pActivationInfo);
}

//------------------------------------------------------------------------
void CFlowVehicleHandbrake::GetConfiguration(SFlowNodeConfig& nodeConfig)
{
    CFlowVehicleBase::GetConfiguration(nodeConfig);

    static const SInputPortConfig pInConfig[] =
    {
        InputPortConfig<SFlowSystemVoid>("Activate", _HELP("Activate handbrake")),
        InputPortConfig<SFlowSystemVoid>("Deactivate", _HELP("Deactivate handbrake")),
        InputPortConfig<float>                  ("ResetTimer", 5.0f, _HELP("Time (in seconds) before handbrake is re-activated")),
        {0}
    };

    static const SOutputPortConfig pOutConfig[] =
    {
        {0}
    };

    nodeConfig.sDescription = _HELP("Toggle vehicle handbrake");
    nodeConfig.pInputPorts  = pInConfig;
    nodeConfig.pOutputPorts = pOutConfig;

    nodeConfig.nFlags |= EFLN_TARGET_ENTITY;

    nodeConfig.SetCategory(EFLN_APPROVED);
}

//------------------------------------------------------------------------
void CFlowVehicleHandbrake::ProcessEvent(EFlowEvent flowEvent, SActivationInfo* pActivationInfo)
{
    CFlowVehicleBase::ProcessEvent(flowEvent, pActivationInfo);

    if (flowEvent == eFE_Activate)
    {
        if (IVehicle* pVehicle = GetVehicle())
        {
            bool    activate = IsPortActive(pActivationInfo, eIn_Activate);

            bool    deactivate = IsPortActive(pActivationInfo, eIn_Deactivate);

            if (activate || deactivate)
            {
                SVehicleMovementEventParams vehicleMovementEventParams;

                vehicleMovementEventParams.bValue = activate;
                vehicleMovementEventParams.fValue   = GetPortFloat(pActivationInfo, eIn_ResetTimer);

                pVehicle->GetMovement()->OnEvent(IVehicleMovement::eVME_EnableHandbrake, vehicleMovementEventParams);
            }
        }
    }
}

//------------------------------------------------------------------------
void CFlowVehicleHandbrake::OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params)
{
    CFlowVehicleBase::OnVehicleEvent(event, params);
}

//------------------------------------------------------------------------
void CFlowVehicleHandbrake::GetMemoryUsage(ICrySizer* pCrySizer) const
{
    pCrySizer->Add(*this);
}

REGISTER_FLOW_NODE("Vehicle:Handbrake", CFlowVehicleHandbrake);