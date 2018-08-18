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

// Description : Implements a flow node to vehicle seats


#ifndef CRYINCLUDE_CRYACTION_FLOWSYSTEM_NODES_VEHICLE_FLOWVEHICLEBASE_H
#define CRYINCLUDE_CRYACTION_FLOWSYSTEM_NODES_VEHICLE_FLOWVEHICLEBASE_H
#pragma once

#include <FlowSystem/Nodes/FlowBaseNode.h>

class CFlowVehicleBase
    : public CFlowBaseNode<eNCT_Instanced>
    , public IVehicleEventListener
{
public:

    virtual void Init(SActivationInfo* pActivationInfo);
    virtual void Delete();

    // CFlowBaseNode
    virtual void GetConfiguration(SFlowNodeConfig& nodeConfig);
    virtual void ProcessEvent(EFlowEvent flowEvent, SActivationInfo* pActivationInfo);
    virtual void Serialize(SActivationInfo* pActivationInfo, TSerialize ser);
    // ~CFlowBaseNode

    // IVehicleEventListener
    virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params);
    // ~IVehicleEventListener

protected:

    IVehicle* GetVehicle();

    IFlowGraph* m_pGraph;
    TFlowNodeId m_nodeID;

    FlowEntityId m_vehicleId;
};

#endif // CRYINCLUDE_CRYACTION_FLOWSYSTEM_NODES_VEHICLE_FLOWVEHICLEBASE_H
