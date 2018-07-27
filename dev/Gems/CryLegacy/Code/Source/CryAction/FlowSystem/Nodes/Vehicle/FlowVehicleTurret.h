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

// Description : Implements a flow node to handle vehicle turrets


#ifndef CRYINCLUDE_CRYACTION_FLOWSYSTEM_NODES_VEHICLE_FLOWVEHICLETURRET_H
#define CRYINCLUDE_CRYACTION_FLOWSYSTEM_NODES_VEHICLE_FLOWVEHICLETURRET_H
#pragma once

#include "FlowVehicleBase.h"

class CFlowVehicleTurret
    : public CFlowVehicleBase
{
public:

    CFlowVehicleTurret(SActivationInfo* pActivationInfo) { Init(pActivationInfo); }
    ~CFlowVehicleTurret() { Delete(); }

    // CFlowBaseNode
    virtual IFlowNodePtr Clone(SActivationInfo* pActivationInfo);
    virtual void GetConfiguration(SFlowNodeConfig& nodeConfig);
    virtual void ProcessEvent(EFlowEvent flowEvent, SActivationInfo* pActivationInfo);
    virtual void Serialize(SActivationInfo* pActivationInfo, TSerialize ser);
    // ~CFlowBaseNode

    // IVehicleEventListener
    virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params);
    // ~IVehicleEventListener

    void SetAimPos();

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }

protected:

    enum EInputs
    {
        IN_TRIGGER,
        IN_SEATID,
        IN_AIMANGLES,
        IN_AIMPOS,
    };

    enum EOutputs
    {
        OUT_HASREACHEDAIMGOAL,
    };

    TVehicleSeatId m_seatId;
    CVehicleSeatActionRotateTurret* m_pActionRotateTurret;

    Vec3 m_aimPos;
};

#endif // CRYINCLUDE_CRYACTION_FLOWSYSTEM_NODES_VEHICLE_FLOWVEHICLETURRET_H
