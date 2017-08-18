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
#pragma warning(disable: 4355) // <future> includes ppltasks.h which throws a C4355 warning: 'this' used in base member initializer list
#include <aws/lambda/LambdaClient.h>
#pragma warning(default: 4355)
#include <Util/FlowSystem/BaseMaglevFlowNode.h>

namespace LmbrAWS
{
    //////////////////////////////////////////////////////////////////////////////////////
    ///
    /// Calls a Lambda function
    ///
    //////////////////////////////////////////////////////////////////////////////////////
    class FlowNode_LambdaInvoke
        : public BaseMaglevFlowNode<eNCT_Singleton>
    {
    public:

        FlowNode_LambdaInvoke(IFlowNode::SActivationInfo* activationInfo);

        virtual ~FlowNode_LambdaInvoke() {}

    protected:
        const char* GetFlowNodeDescription() const override { return _HELP("Invoke a lambda function in the cloud, optionally providing JSON data as arguments to your Lambda call through the Args port.  For more info see http://docs.aws.amazon.com/lambda/latest/dg/API_Invoke.html#API_Invoke_RequestSyntax"); }
        void ProcessEvent_Internal(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* activationInfo) override;
        Aws::Vector<SInputPortConfig> GetInputPorts() const override;
        Aws::Vector<SOutputPortConfig> GetOutputPorts() const override;

        virtual const char* GetClassTag() const override;
    private:

        enum EInputs
        {
            EIP_Invoke = EIP_StartIndex,
            EIP_FunctionClient,
            EIP_RequestBody
        };

        enum EOutputs
        {
            EOP_ResponseBody = EOP_StartIndex
        };

        LmbrAWS::Lambda::FunctionClientInputPort m_functionClientPort {
            EIP_FunctionClient
        };

        void ApplyResult(const Aws::Lambda::Model::InvokeRequest& request,
            const Aws::Lambda::Model::InvokeOutcome& outcome,
            const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context);

        void ProcessInvokePort(IFlowNode::SActivationInfo* pActInfo);
    };
} // namespace AWS



