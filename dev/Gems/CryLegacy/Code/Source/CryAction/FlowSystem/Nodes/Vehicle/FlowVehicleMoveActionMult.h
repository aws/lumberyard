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


#ifndef CRYINCLUDE_CRYACTION_FLOWSYSTEM_NODES_VEHICLE_FLOWVEHICLEMOVEACTIONMULT_H
#define CRYINCLUDE_CRYACTION_FLOWSYSTEM_NODES_VEHICLE_FLOWVEHICLEMOVEACTIONMULT_H
#pragma once

#include "FlowVehicleBase.h"

class CFlowVehicleMoveActionMult
    : public CFlowVehicleBase
    , public IVehicleMovementActionFilter
{
public:

    CFlowVehicleMoveActionMult(SActivationInfo* pActivationInfo);
    ~CFlowVehicleMoveActionMult() { Delete(); }

    // CFlowBaseNode
    virtual IFlowNodePtr Clone(SActivationInfo* pActivationInfo);
    virtual void GetConfiguration(SFlowNodeConfig& nodeConfig);
    virtual void ProcessEvent(EFlowEvent flowEvent, SActivationInfo* pActivationInfo);
    virtual void Serialize(SActivationInfo* pActivationInfo, TSerialize ser);
    // ~CFlowBaseNode

    // IVehicleMovementActionFilter
    void OnProcessActions(SVehicleMovementAction& movementAction);
    // ~IVehicleMovementActionFilter

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }

protected:

    enum EInputs
    {
        IN_ENABLETRIGGER = 0,
        IN_DISABLETRIGGER,

        IN_POWERMULT,
        IN_ROTATEPITCHMULT,
        IN_ROTATEYAWMULT,

        IN_POWERMUSTBEPOSITIVE,
    };

    enum EOutputs
    {
    };


    float m_powerMult;
    float m_pitchMult;
    float m_yawMult;
    bool m_powerMustBePositive;
};

#endif // CRYINCLUDE_CRYACTION_FLOWSYSTEM_NODES_VEHICLE_FLOWVEHICLEMOVEACTIONMULT_H
