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
#pragma once

#include <FlowSystem/Nodes/FlowBaseNode.h>

class FlowNode_GetUsername
    : public CFlowBaseNode < eNCT_Singleton >
{
public:

    enum EInputs
    {
        EIP_GetUsername
    };

    enum EOutputs
    {
        EOP_Username,
        EOP_Error
    };

    FlowNode_GetUsername(SActivationInfo* activationInfo);
    virtual ~FlowNode_GetUsername() {}

protected:
    void GetMemoryUsage(ICrySizer* sizer) const override { sizer->Add(*this); }
    void GetConfiguration(SFlowNodeConfig& config) override;
    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override;
};