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
#include <StdAfx.h>

#include <memory>
#include <string>

#include <aws/lambda/model/InvokeRequest.h>
#include <aws/core/utils/Outcome.h>
#include <aws/core/utils/HashingUtils.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#include <aws/core/utils/ratelimiter/DefaultRateLimiter.h>
#include <aws/lambda/LambdaErrors.h>
#include "StringUtils.h"

#include "Lambda/FlowNode_LambdaInvoke.h"

namespace LmbrAWS
{
    static const char* CLASS_TAG = "AWS:Primitive:LambdaInvoke";

    FlowNode_LambdaInvoke::FlowNode_LambdaInvoke(IFlowNode::SActivationInfo* activationInfo)
        : BaseMaglevFlowNode < eNCT_Singleton >(activationInfo)
    {
    }


    void FlowNode_LambdaInvoke::ApplyResult(const Aws::Lambda::Model::InvokeRequest& request,
        const Aws::Lambda::Model::InvokeOutcome& outcome,
        const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
    {
        auto flowGraphContext = std::static_pointer_cast<const FlowGraphContext>(context);
        IFlowGraph* flowGraph = flowGraphContext->GetFlowGraph();

        if (flowGraph == nullptr)
        {
            return;
        }

        if (outcome.IsSuccess())
        {
            Aws::Lambda::Model::InvokeResult& result = const_cast<Aws::Lambda::Model::InvokeResult&>(outcome.GetResult());

            Aws::IOStream& payload = result.GetPayload();
            Aws::String payload_str((std::istreambuf_iterator<char>(payload)), std::istreambuf_iterator<char>());

            if (result.GetFunctionError() != "")
            {
                ErrorNotify(flowGraph, flowGraphContext->GetFlowNodeId(), payload_str.c_str());
            }
            else
            {
                SFlowAddress addr(flowGraphContext->GetFlowNodeId(), EOP_ResponseBody, true);
                flowGraph->ActivatePortCString(addr, payload_str.c_str());
                SuccessNotify(flowGraph, flowGraphContext->GetFlowNodeId());
            }
        }
        else
        {
            string errorInfo = _LOC("@AWS Lambda Error: ");
            if (outcome.GetError().GetErrorType() == Aws::Lambda::LambdaErrors::MISSING_AUTHENTICATION_TOKEN)
            {
                errorInfo += _LOC("@Missing authentication token. Check that Cloud Canvas resources have been deployed and uploaded.");
            }
            else
            {
                errorInfo += outcome.GetError().GetExceptionName().c_str();
            }
            ErrorNotify(flowGraph, flowGraphContext->GetFlowNodeId(), errorInfo);
        }
    }

    Aws::Vector<SInputPortConfig> FlowNode_LambdaInvoke::GetInputPorts() const
    {
        static const Aws::Vector<SInputPortConfig> inputPorts = {
            InputPortConfig_Void("Invoke"),
            m_functionClientPort.GetConfiguration("FunctionName", _HELP("A lambda function to call from the cloud")),
            InputPortConfig<string>("RequestBody", _HELP("The data that will be sent to the lambda function call as arguments in JSON format.  For more information see http://docs.aws.amazon.com/lambda/latest/dg/API_Invoke.html#API_Invoke_RequestSyntax"), "Args")
        };

        return inputPorts;
    }

    Aws::Vector<SOutputPortConfig> FlowNode_LambdaInvoke::GetOutputPorts() const
    {
        static const Aws::Vector<SOutputPortConfig> outputPorts = {
            OutputPortConfig<string>("ResponseBody", _HELP("The data that was output by the lambda function"), "Result")
        };

        return outputPorts;
    }

    void FlowNode_LambdaInvoke::ProcessEvent_Internal(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo)
    {
        if (event == eFE_Activate)
        {
            if (IsPortActive(pActInfo, EIP_Invoke))
            {
                ProcessInvokePort(pActInfo);
            }
        }
    }

    void FlowNode_LambdaInvoke::ProcessInvokePort(IFlowNode::SActivationInfo* pActInfo)
    {
        string functionName;
        string requestBody;
        IFlowGraph* flowGraph = pActInfo->pGraph;

        auto client = m_functionClientPort.GetClient(pActInfo);
        if (!client.IsReady())
        {
            ErrorNotify(flowGraph, pActInfo->myID, "Client configuration not ready.");
            return;
        }

        functionName = client.GetFunctionName();

        pActInfo->pInputPorts[EIP_RequestBody].GetValueWithConversion(requestBody);

        if (functionName == "")
        {
            ErrorNotify(flowGraph, pActInfo->myID, "Function name empty");
            return;
        }

        Aws::Lambda::Model::InvokeRequest request;
        request.SetFunctionName(functionName);
        request.SetInvocationType(Aws::Lambda::Model::InvocationType::RequestResponse);
        request.SetContentType("application/javascript");
        request.SetLogType(Aws::Lambda::Model::LogType::Tail);

        std::shared_ptr<Aws::IOStream> requestBodyStream = Aws::MakeShared<Aws::StringStream>(requestBody);
        *requestBodyStream << requestBody;
        request.SetBody(requestBodyStream);

        auto context = std::make_shared<FlowGraphContext>(flowGraph, pActInfo->myID);
        MARSHALL_AWS_BACKGROUND_REQUEST(Lambda, client, Invoke, FlowNode_LambdaInvoke::ApplyResult, request, context)
    }

    REGISTER_CLASS_TAG_AND_FLOW_NODE(CLASS_TAG, FlowNode_LambdaInvoke);
} //Namespace Amazon