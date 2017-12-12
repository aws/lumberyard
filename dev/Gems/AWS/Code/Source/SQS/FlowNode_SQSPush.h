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
#pragma warning(disable: 4355) // <future> includes ppltasks.h which throws a C4355 warning: 'this' used in base member initializer list
#include <aws/sqs/SQSClient.h>
#pragma warning(default: 4355)

namespace LmbrAWS
{
    //////////////////////////////////////////////////////////////////////////////////////
    ///
    /// Pushes a message to a SQS queue
    ///
    //////////////////////////////////////////////////////////////////////////////////////

    class FlowNode_SQSPush
        : public BaseMaglevFlowNode<eNCT_Singleton>
    {
    public:

        FlowNode_SQSPush(IFlowNode::SActivationInfo* activationInfo);

        virtual ~FlowNode_SQSPush() {}

    protected:
        const char* GetFlowNodeDescription() const override { return _HELP("Push a message to an AWS Queue"); }
        void ProcessEvent_Internal(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* activationInfo) override;
        Aws::Vector<SInputPortConfig> GetInputPorts() const override;
        Aws::Vector<SOutputPortConfig> GetOutputPorts() const override;

        virtual const char* GetClassTag() const override;
    private:

        void ApplyResult(const Aws::SQS::Model::SendMessageRequest& request,
            const Aws::SQS::Model::SendMessageOutcome& outcome,
            const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context);

        enum EInputs
        {
            EIP_Push = EIP_StartIndex,
            EIP_QueueClient,
            EIP_Message
        };

        enum EOutputs
        {
            EOP_ResponseBody = EOP_StartIndex
        };

        LmbrAWS::SQS::QueueClientInputPort m_queueClientPort {
            EIP_QueueClient
        };
    };
} // namespace AWS