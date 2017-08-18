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

// Description : Implements a flow node to handle vehicle seats


#ifndef CRYINCLUDE_CRYACTION_FLOWSYSTEM_NODES_VEHICLE_FLOWVEHICLESEAT_H
#define CRYINCLUDE_CRYACTION_FLOWSYSTEM_NODES_VEHICLE_FLOWVEHICLESEAT_H
#pragma once

#include "FlowVehicleBase.h"

class CFlowVehicleSeat
    : public CFlowVehicleBase
{
public:

    CFlowVehicleSeat(SActivationInfo* pActivationInfo);
    ~CFlowVehicleSeat() { Delete(); }

    // CFlowBaseNode
    virtual IFlowNodePtr Clone(SActivationInfo* pActivationInfo);
    virtual void GetConfiguration(SFlowNodeConfig& nodeConfig);
    virtual void ProcessEvent(EFlowEvent flowEvent, SActivationInfo* pActivationInfo);
    // ~CFlowBaseNode

    // IVehicleEventListener
    virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params);
    // ~IVehicleEventListener

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }

protected:

    void ActivateOutputPorts(SActivationInfo* pActivationInfo);

    enum EInputs
    {
        IN_SEATID,
        IN_SEATNAME,
        IN_DRIVERSEAT,
        IN_LOCK,
        IN_UNLOCK,
        IN_LOCKTYPE,
    };

    enum EOutputs
    {
        OUT_SEATID,
        OUT_PASSENGERID,
    };

    TVehicleSeatId m_seatId;
    bool m_isDriverSeatRequested;
};

#endif // CRYINCLUDE_CRYACTION_FLOWSYSTEM_NODES_VEHICLE_FLOWVEHICLESEAT_H
