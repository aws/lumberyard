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

// Description : Implements a flow node to handle a vehicle movement features


#ifndef CRYINCLUDE_CRYACTION_FLOWSYSTEM_NODES_VEHICLE_FLOWVEHICLEMOVEMENT_H
#define CRYINCLUDE_CRYACTION_FLOWSYSTEM_NODES_VEHICLE_FLOWVEHICLEMOVEMENT_H
#pragma once

#include "FlowVehicleBase.h"

class CFlowVehicleMovement
    : public CFlowVehicleBase
{
public:

    CFlowVehicleMovement(SActivationInfo* pActivationInfo)
    {
        Init(pActivationInfo);
        m_shouldZeroMass = false;
    }
    ~CFlowVehicleMovement() { Delete(); }

    // CFlowBaseNode
    virtual IFlowNodePtr Clone(SActivationInfo* pActivationInfo);
    virtual void GetConfiguration(SFlowNodeConfig& nodeConfig);
    virtual void ProcessEvent(EFlowEvent flowEvent, SActivationInfo* pActivationInfo);
    virtual void Serialize(SActivationInfo* pActivationInfo, TSerialize ser);
    // ~CFlowBaseNode

    // IVehicleEventListener
    virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params) {}
    // ~IVehicleEventListener

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }

protected:

    enum EInputs
    {
        In_WarmupEngine,
        In_ZeroMass,
        In_RestoreMass,
    };

    enum EOutputs
    {
    };

    void ZeroMass(bool enable);

    struct SPartMass
    {
        int partid;
        float mass;

        SPartMass()
            : partid(0)
            , mass(0.0f) {}
        SPartMass(int id, float m)
            : partid(id)
            , mass(m) {}
    };

    std::vector<SPartMass> m_partMass;
    bool m_shouldZeroMass;  // on loading defer ZeroMass(true) call to next update (after vehicle physicalisation)
};


//////////////////////////////////////////////////////////////////////////

class CFlowVehicleMovementParams
    : public CFlowVehicleBase
{
public:

    CFlowVehicleMovementParams(SActivationInfo* pActivationInfo)
    {
        Init(pActivationInfo);
    }
    ~CFlowVehicleMovementParams() { Delete(); }

    // CFlowBaseNode
    virtual IFlowNodePtr Clone(SActivationInfo* pActivationInfo);
    virtual void GetConfiguration(SFlowNodeConfig& nodeConfig);
    virtual void ProcessEvent(EFlowEvent flowEvent, SActivationInfo* pActivationInfo);
    // ~CFlowBaseNode

    // IVehicleEventListener
    virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params) {}
    // ~IVehicleEventListener

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }

protected:

    enum EInputs
    {
        In_Trigger = 0,
        In_MaxSpeedFactor,
        In_MaxAcceleration
    };

    enum EOutputs
    {
    };
};


#endif // CRYINCLUDE_CRYACTION_FLOWSYSTEM_NODES_VEHICLE_FLOWVEHICLEMOVEMENT_H
