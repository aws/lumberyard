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

#include <Util/FlowSystem/BaseMaglevFlowNode.h>

namespace LmbrAWS
{
    //////////////////////////////////////////////////////////////////////////////////////
    ///
    /// Sets the monitor update frequency
    ///
    //////////////////////////////////////////////////////////////////////////////////////
    class FlowNode_SetUpdateFrequency
        : public BaseMaglevFlowNode< eNCT_Singleton >
    {
    public:
        FlowNode_SetUpdateFrequency(SActivationInfo* activationInfo);

        virtual ~FlowNode_SetUpdateFrequency() {}

        enum EInputs
        {
            EIP_SetTimer = EIP_StartIndex,
            EIP_TimerValue,
        };

        enum EOutputs
        {
            EOP_Finished = EOP_StartIndex,
        };

        virtual const char* GetClassTag() const override;
    protected:
        const char* GetFlowNodeDescription() const override { return _HELP("Set or clear a recurring timer to poll monitored buckets"); }
        void ProcessEvent_Internal(EFlowEvent event, SActivationInfo* activationInfo) override;
        Aws::Vector<SInputPortConfig> GetInputPorts() const override;
        Aws::Vector<SOutputPortConfig> GetOutputPorts() const override;
    };
} // namespace AWS



