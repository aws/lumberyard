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
#pragma warning(push)
#pragma warning(disable: 4819) // Invalid character not in default code page
#include <SNS/FlowNode_SnsSubscribe.h>
#include <aws/sns/model/SubscribeRequest.h>
#pragma warning(pop)
#include <AzCore/std/string/regex.h>

namespace LmbrAWS
{
    static const char* CLASS_TAG = "AWS:Primitive:SNS:SnsSubscribe";

    FlowNode_SnsSubscribe::FlowNode_SnsSubscribe(IFlowNode::SActivationInfo* activationInfo)
        : BaseMaglevFlowNode < eNCT_Singleton >(activationInfo)
        , m_isValid(true)
    {
    }

    Aws::Vector<SInputPortConfig> FlowNode_SnsSubscribe::GetInputPorts() const
    {
        static const Aws::Vector<SInputPortConfig> inputPorts = {
            InputPortConfig_Void("Subscribe", _HELP("Subscribe to a topic in order to receive messages published to that topic.  More info can be found at http://docs.aws.amazon.com/sns/latest/dg/SubscribeTopic.html")),
            InputPortConfig<string>("Protocol", _HELP("The protocol of the endpoint to subscribe"), "Protocol",
                _UICONFIG("enum_string:http=http,https=https,email=email,email-json=email-json,sms=sms,sqs=sqs,application=application,lambda=lambda")),
            m_topicClientPort.GetConfiguration("TopicARN", _HELP("ARN of the SNS Topic to subscribe to.")),
            InputPortConfig<string>("Endpoint", _HELP("The address of the endpoint to subscribe, for eample when using an email protocol this would be the email address to receive the notifications at.  For sending to http or https info can be found at http://docs.aws.amazon.com/sns/latest/dg/SendMessageToHttp.html  More info on SNS can be found at http://docs.aws.amazon.com/sns/latest/dg/welcome.html"))
        };

        return inputPorts;
    }

    Aws::Vector<SOutputPortConfig> FlowNode_SnsSubscribe::GetOutputPorts() const
    {
        static const Aws::Vector<SOutputPortConfig> outputPorts =
        {
            OutputPortConfig<string>("SubscriptionArn", _HELP("The ARN of the created subscription"))
        };
        return outputPorts;
    }


    void FlowNode_SnsSubscribe::ProcessEvent_Internal(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* activationInfo)
    {
        if (event == eFE_Initialize && IsPortActive(activationInfo, EIP_Subscribe))
        {
            auto const& protocol = GetPortString(activationInfo, EIP_Protocol);
            auto const& endpoint = GetPortString(activationInfo, EIP_Endpoint);

            AZStd::smatch match;

            m_isValid = true;

            if (strcmp("http", protocol) == 0)
            {
                AZStd::regex m_httpRegex("^(http:\\/\\/)([0-9a-z\\.-]+)\\.([a-z\\.]{2,6})(:([0-9]{1,5}))?$", AZStd::regex::extended);
                if (!AZStd::regex_match(AZStd::string(endpoint), match, m_httpRegex))
                {
                    m_isValid = false;
                    CRY_ASSERT_MESSAGE(false, "Endpoint expected to be http address: \"http://example.com:8080\"");
                }
            }
            else if (strcmp("https", protocol) == 0)
            {
                AZStd::regex m_httpsRegex("^(https:\\/\\/)([0-9a-z\\.-]+)\\.([a-z\\.]{2,6})(:([0-9]{1,5}))?$", AZStd::regex::extended);
                if (!AZStd::regex_match(AZStd::string(endpoint), match, m_httpsRegex))
                {
                    m_isValid = false;
                    CRY_ASSERT_MESSAGE(false, "Endpoint expected to be https address: \"https://example.com:8080\"");
                }
            }
            else if (strcmp("email", protocol) == 0 || strcmp("email-json", protocol) == 0)
            {
                AZStd::regex m_emailRegex("^([a-z0-9_\\.-]+)@([0-9a-z\\.-]+)\\.([a-z\\.]{2,6})$", AZStd::regex::extended);
                if (!AZStd::regex_match(AZStd::string(endpoint), match, m_emailRegex))
                {
                    m_isValid = false;
                    CRY_ASSERT_MESSAGE(false, "Endpoint expected to be valid email address");
                }
            }
            else if (strcmp("sms", protocol) == 0)
            {
                AZStd::regex m_smsRegex("^[0-9]{4,15}$", AZStd::regex::extended);
                if (!AZStd::regex_match(AZStd::string(endpoint), match, m_smsRegex))
                {
                    m_isValid = false;
                    CRY_ASSERT_MESSAGE(false, "Endpoint expected to be phone number (only numbers, no plus, dashes or spaces allowed");
                }
            }
        }

        if (event == eFE_Activate && IsPortActive(activationInfo, EIP_Subscribe) && m_isValid)
        {
            auto client = m_topicClientPort.GetClient(activationInfo);
            if (!client.IsReady())
            {
                ErrorNotify(activationInfo->pGraph, activationInfo->myID, "Client configuration not ready.");
                return;
            }

            auto const& topicArn = client.GetTopicARN();
            auto const& endpoint = GetPortString(activationInfo, EIP_Endpoint);
            auto const& protocol = GetPortString(activationInfo, EIP_Protocol);

            Aws::SNS::Model::SubscribeRequest subscribeRequest;
            subscribeRequest.WithTopicArn(topicArn).WithEndpoint(endpoint).WithProtocol(protocol);

            auto context = std::make_shared<FlowGraphContext>(activationInfo->pGraph, activationInfo->myID);

            MARSHALL_AWS_BACKGROUND_REQUEST(SNS, client, Subscribe, FlowNode_SnsSubscribe::ApplyResult, subscribeRequest, context)
        }
    }

    void FlowNode_SnsSubscribe::ApplyResult(const Aws::SNS::Model::SubscribeRequest& request,
        const Aws::SNS::Model::SubscribeOutcome& outcome, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
    {
        if (context)
        {
            auto fgContext = std::static_pointer_cast<const FlowGraphContext>(context);

            if (outcome.IsSuccess())
            {
                SuccessNotify(fgContext->GetFlowGraph(), fgContext->GetFlowNodeId());

                string subscriptionArn = outcome.GetResult().GetSubscriptionArn().c_str();
                if (!subscriptionArn.empty())
                {
                    SFlowAddress addr(fgContext->GetFlowNodeId(), EOP_SubscriptionArn, true);
                    fgContext->GetFlowGraph()->ActivatePort(addr, subscriptionArn);
                }
            }
            else
            {
                ErrorNotify(fgContext->GetFlowGraph(), fgContext->GetFlowNodeId(), outcome.GetError().GetMessage().c_str());
            }
        }
    }

    REGISTER_CLASS_TAG_AND_FLOW_NODE(CLASS_TAG, FlowNode_SnsSubscribe);
}