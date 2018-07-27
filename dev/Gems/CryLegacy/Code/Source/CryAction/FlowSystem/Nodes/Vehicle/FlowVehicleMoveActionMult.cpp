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

// Description : Implements a flow node to apply multipliers to vehicle
//               movement actions


#include "CryLegacy_precompiled.h"
#include "CryAction.h"
#include "IVehicleSystem.h"
#include "IFlowSystem.h"
#include "FlowSystem/Nodes/FlowBaseNode.h"
#include "FlowVehicleMoveActionMult.h"

//------------------------------------------------------------------------
CFlowVehicleMoveActionMult::CFlowVehicleMoveActionMult(SActivationInfo* pActivationInfo)
    : m_powerMult(1.0f)
    , m_pitchMult(1.0f)
    , m_yawMult(1.0f)
    , m_powerMustBePositive(false)
{
    Init(pActivationInfo);
}

//------------------------------------------------------------------------
IFlowNodePtr CFlowVehicleMoveActionMult::Clone(SActivationInfo* pActivationInfo)
{
    IFlowNode* pNode = new CFlowVehicleMoveActionMult(pActivationInfo);
    return pNode;
}

//------------------------------------------------------------------------
void CFlowVehicleMoveActionMult::GetConfiguration(SFlowNodeConfig& nodeConfig)
{
    CFlowVehicleBase::GetConfiguration(nodeConfig);

    static const SInputPortConfig pInConfig[] =
    {
        //InputPortConfig_AnyType("HitTrigger", _HELP("trigger which cause the vehicle to receive damages")),
        InputPortConfig_AnyType("EnableTrigger", _HELP("")),
        InputPortConfig_AnyType("DisableTrigger", _HELP("")),
        InputPortConfig<float>("PowerMult", 1.0f, _HELP("")),
        InputPortConfig<float>("RotatePitch", 1.0f, _HELP("")),
        InputPortConfig<float>("RotateYaw", 1.0f, _HELP("")),
        InputPortConfig<bool>("PowerMustBePositive", false, _HELP("")),
        {0}
    };

    static const SOutputPortConfig pOutConfig[] =
    {
        {0}
    };

    nodeConfig.sDescription = _HELP("Add multipliers to vehicle movement actions");
    nodeConfig.nFlags |= EFLN_TARGET_ENTITY;
    nodeConfig.pInputPorts = pInConfig;
    nodeConfig.pOutputPorts = pOutConfig;
    nodeConfig.SetCategory(EFLN_APPROVED);
}

//------------------------------------------------------------------------
void CFlowVehicleMoveActionMult::ProcessEvent(EFlowEvent flowEvent, SActivationInfo* pActivationInfo)
{
    CFlowVehicleBase::ProcessEvent(flowEvent, pActivationInfo);

    if (flowEvent == eFE_Activate)
    {
        m_powerMult = GetPortFloat(pActivationInfo, IN_POWERMULT);
        m_pitchMult = GetPortFloat(pActivationInfo, IN_ROTATEPITCHMULT);
        m_yawMult = GetPortFloat(pActivationInfo, IN_ROTATEYAWMULT);
        m_powerMustBePositive = GetPortBool(pActivationInfo, IN_POWERMUSTBEPOSITIVE);

        if (IVehicle* pVehicle = GetVehicle())
        {
            if (IsPortActive(pActivationInfo, IN_ENABLETRIGGER))
            {
                if (IVehicleMovement* pMovement = pVehicle->GetMovement())
                {
                    pMovement->RegisterActionFilter(this);
                }
            }

            if (IsPortActive(pActivationInfo, IN_DISABLETRIGGER))
            {
                if (IVehicleMovement* pMovement = pVehicle->GetMovement())
                {
                    pMovement->UnregisterActionFilter(this);
                }
            }
        }
    }
}

//------------------------------------------------------------------------
void CFlowVehicleMoveActionMult::Serialize(SActivationInfo* pActivationInfo, TSerialize ser)
{
}

//------------------------------------------------------------------------
void CFlowVehicleMoveActionMult::OnProcessActions(SVehicleMovementAction& movementAction)
{
    movementAction.power *= m_powerMult;
    movementAction.rotatePitch *= m_pitchMult;
    movementAction.rotateYaw *= m_yawMult;

    if (m_powerMustBePositive)
    {
        movementAction.power = max(0.0f, movementAction.power);
    }
}



REGISTER_FLOW_NODE("Vehicle:MoveActionMult", CFlowVehicleMoveActionMult);
