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
    class FlowNode_SNSMessageDeserialize
        : public BaseMaglevFlowNode < eNCT_Singleton >
    {
    public:
        FlowNode_SNSMessageDeserialize(IFlowNode::SActivationInfo* activationInfo);


        virtual void GetMemoryUsage(ICrySizer* sizer) const override { sizer->Add(*this); }

    protected:
        virtual const char* GetClassTag() const override;
        const char* GetFlowNodeDescription() const override { return _HELP("Deserialize a JSON message from SNS"); }
        void ProcessEvent_Internal(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* activationInfo) override;

        enum EInputs
        {
            EIP_Parse = EIP_StartIndex,
            EIP_Message
        };

        enum EOutputs
        {
            EOP_MessageBody = EOP_StartIndex,
            EOP_MessageSubject
        };

        virtual Aws::Vector<SInputPortConfig> GetInputPorts() const override;
        virtual Aws::Vector<SOutputPortConfig> GetOutputPorts() const override;
    };
} // namespace AWS



