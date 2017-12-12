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
#include <aws/queues/sqs/SQSQueue.h>
#pragma warning(default: 4355)

namespace LmbrAWS
{
    //////////////////////////////////////////////////////////////////////////////////////
    ///
    /// Starts Polling a given SQS Queue
    ///
    //////////////////////////////////////////////////////////////////////////////////////

    class FlowNode_SQSPoller
        : public BaseMaglevFlowNode<eNCT_Instanced>
    {
    public:

        FlowNode_SQSPoller(IFlowNode::SActivationInfo* activationInfo);

        virtual ~FlowNode_SQSPoller();
        virtual IFlowNodePtr Clone(IFlowNode::SActivationInfo* pActInfo)
        {
            return new FlowNode_SQSPoller(pActInfo);
        }

    protected:
        const char* GetFlowNodeDescription() const override { return _HELP("Start polling an AWS Queue"); }
        void ProcessEvent_Internal(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* activationInfo) override;
        Aws::Vector<SInputPortConfig> GetInputPorts() const override;
        Aws::Vector<SOutputPortConfig> GetOutputPorts() const override;

        virtual const char* GetClassTag() const override;
    private:
        void OnMessageReceivedHandler(const Aws::Queues::Queue<Aws::SQS::Model::Message>*, const Aws::SQS::Model::Message& message,
            bool);
        void OnArnReceivedHandler(const Aws::Queues::Sqs::SQSQueue*, const Aws::String& arn);
        Aws::String GetNameFromUrl(const Aws::String& url) const;


        enum EInputs
        {
            EIP_Start = EIP_StartIndex,
            EIP_QueueClient
        };

        enum EOutputs
        {
            EOP_ReturnedMessage = EOP_StartIndex,
            EOP_QueueArn
        };

        std::shared_ptr<Aws::Queues::Sqs::SQSQueue> m_sqsQueue;
        IFlowGraphPtr m_flowGraph;
        TFlowNodeId m_flowNodeId;
    };
} // namespace AWS
