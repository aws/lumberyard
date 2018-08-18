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

// Description : Implements a flow node to handle vehicle Attachments


#ifndef CRYINCLUDE_CRYACTION_FLOWSYSTEM_NODES_VEHICLE_FLOWVEHICLEATTACHMENT_H
#define CRYINCLUDE_CRYACTION_FLOWSYSTEM_NODES_VEHICLE_FLOWVEHICLEATTACHMENT_H
#pragma once

#include "FlowVehicleBase.h"

class CFlowVehicleAttachment
    : public CFlowVehicleBase
{
public:

    CFlowVehicleAttachment(SActivationInfo* pActivationInfo) { Init(pActivationInfo); }
    ~CFlowVehicleAttachment() { Delete(); }

    // CFlowBaseNode
    virtual void Init(SActivationInfo* pActivationInfo);
    virtual IFlowNodePtr Clone(SActivationInfo* pActivationInfo);
    virtual void GetConfiguration(SFlowNodeConfig& nodeConfig);
    virtual void ProcessEvent(EFlowEvent flowEvent, SActivationInfo* pActivationInfo);
    virtual void Serialize(SActivationInfo* pActivationInfo, TSerialize ser);
    // ~CFlowBaseNode

    // IVehicleEventListener
    virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params);
    // ~IVehicleEventListener

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }


protected:

    enum EInputs
    {
        IN_ATTACHMENT,
        IN_ENTITYID,
        IN_ATTACH,
        IN_DETACH
    };

    enum EOutputs
    {
    };

    void Attach(SActivationInfo* pActInfo, bool attach);

    FlowEntityId m_attachedId;
};

#endif // CRYINCLUDE_CRYACTION_FLOWSYSTEM_NODES_VEHICLE_FLOWVEHICLEATTACHMENT_H
