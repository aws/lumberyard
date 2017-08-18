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
#include <aws/sns/SNSClient.h>
#pragma warning(default: 4355)

namespace LmbrAWS
{
    //////////////////////////////////////////////////////////////////////////////////////
    ///
    /// Creates an Sns publishing node
    ///
    //////////////////////////////////////////////////////////////////////////////////////
    class FlowNode_SnsNotify
        : public BaseMaglevFlowNode < eNCT_Singleton >
    {
    public:
        FlowNode_SnsNotify(IFlowNode::SActivationInfo* activationInfo);

        // ~IFlowNode
        virtual ~FlowNode_SnsNotify() {}

    protected:
        const char* GetFlowNodeDescription() const override { return _HELP("Publishes messages to an SNS Topic."); }
        void ProcessEvent_Internal(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* activationInfo) override;
        Aws::Vector<SInputPortConfig> GetInputPorts() const override;
        Aws::Vector<SOutputPortConfig> GetOutputPorts() const override;

        virtual const char* GetClassTag() const override;
    private:

        enum
        {
            EIP_Notify = EIP_StartIndex,
            EIP_Message,
            EIP_Subject,
            EIP_TopicClient
        };

        LmbrAWS::SNS::TopicClientInputPort m_topicClientPort {
            EIP_TopicClient
        };

        void ApplyResult(const Aws::SNS::Model::PublishRequest&, const Aws::SNS::Model::PublishOutcome& outcome, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context);
    };
} // namespace Amazon