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
#include <AWS_precompiled.h>
#include <SQS/FlowNode_SQSPoller.h>
#include <aws/sqs/model/GetQueueAttributesRequest.h>

#include <CloudCanvas/CloudCanvasMappingsBus.h>
#include <CloudGemFramework/AwsApiRequestJob.h>

#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Jobs/JobManagerBus.h>

namespace LmbrAWS
{
    static const char* CLASS_TAG = "AWS:Primitive:SQS:SQSPoller";
    static const char* CLIENT_CLASS_TAG = "AWS:Primitive:SQSPollerClient";

    FlowNode_SQSPoller::FlowNode_SQSPoller(IFlowNode::SActivationInfo* activationInfo)
        : BaseMaglevFlowNode < eNCT_Instanced >(activationInfo)
        , m_sqsQueue(nullptr)
    {
    }

    FlowNode_SQSPoller::~FlowNode_SQSPoller()
    {
        if (m_sqsQueue)
        {
            m_sqsQueue->StopPolling();
        }
    }

    Aws::Vector<SInputPortConfig> FlowNode_SQSPoller::GetInputPorts() const
    {
        static const Aws::Vector<SInputPortConfig> inputPorts = {
            InputPortConfig_Void("Start"),
            InputPortConfig<string>("QueueName", _HELP("Name of the SQS queue that has already been created")),
        };

        return inputPorts;
    }

    Aws::Vector<SOutputPortConfig> FlowNode_SQSPoller::GetOutputPorts() const
    {
        static const Aws::Vector<SOutputPortConfig> outputPorts =
        {
            OutputPortConfig<string>("MessageReceived", _HELP("Most recent message on the stack"), "OnMessageReceived"),
            OutputPortConfig<string>("QueueArn", _HELP("The queue arn")),
        };

        return outputPorts;
    }


    void FlowNode_SQSPoller::ProcessEvent_Internal(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* activationInfo)
    {
        if (event == eFE_Activate && IsPortActive(activationInfo, EIP_Start))
        {
            AZStd::string queueUrl = GetPortString(activationInfo, EIP_QueueClient).c_str();
            EBUS_EVENT_RESULT(queueUrl, CloudGemFramework::CloudCanvasMappingsBus, GetLogicalToPhysicalResourceMapping, queueUrl);

            m_flowGraph = activationInfo->pGraph;
            m_flowNodeId = activationInfo->myID;
 
            auto name = GetNameFromUrl(queueUrl.c_str());
            if (name.empty())
            {
                ErrorNotify(activationInfo->pGraph, activationInfo->myID, "Could not get SQS name from URL");
                return;
            }

            CloudGemFramework::AwsApiClientJobConfig<Aws::SQS::SQSClient> clientConfig;
            auto sqsClient = clientConfig.GetClient();

            m_sqsQueue = Aws::MakeShared<Aws::Queues::Sqs::SQSQueue>(CLIENT_CLASS_TAG, sqsClient, name.c_str(), 30, 1);

            m_sqsQueue->SetMessageReceivedEventHandler(std::bind(&FlowNode_SQSPoller::OnMessageReceivedHandler, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
            m_sqsQueue->SetQueueArnSuccessEventHandler(std::bind(&FlowNode_SQSPoller::OnArnReceivedHandler, this, std::placeholders::_1, std::placeholders::_2));

            // Ideally we'd pull the executor off the sqsClient, but we can't do that atm
            // I've added a task that will let us do that, but since it requires a full release, I'd like to put it off for a while
            // See https://issues.labcollab.net/browse/LMBR-9010
            CloudGemFramework::AwsApiJob::Config* clientSettings{ nullptr };
            EBUS_EVENT_RESULT(clientSettings, CloudGemFramework::CloudGemFrameworkRequestBus, GetDefaultConfig);
            auto executor = clientSettings->executor;

            auto lambda = [=]()
                {
                    m_sqsQueue->EnsureQueueIsInitialized();
                    m_sqsQueue->StartPolling();
                    m_sqsQueue->RequestArn();
                };

            AZ::JobContext* jobContext{ nullptr };
            EBUS_EVENT_RESULT(jobContext, AZ::JobManagerBus, GetGlobalContext);

            auto job = AZ::CreateJobFunction(lambda, true, jobContext);
            job->Start();
        }
        else if (event == eFE_Initialize || event == eFE_Suspend)
        {
            if (m_sqsQueue)
            {
                m_sqsQueue->StopPolling();
            }
        }
    }

    Aws::String FlowNode_SQSPoller::GetNameFromUrl(const Aws::String& url) const
    {
        if (url.empty())
        {
            return "";
        }
        auto name = url.substr(url.find_last_of("/") + 1, url.length());
        return name;
    }

    void FlowNode_SQSPoller::OnMessageReceivedHandler(const Aws::Queues::Queue<Aws::SQS::Model::Message>*, const Aws::SQS::Model::Message& message, bool)
    {
        string messageBody = message.GetBody().c_str();

        if (!messageBody.empty())
        {
            auto lambda = [=]()
                {
                    SFlowAddress addr(m_flowNodeId, EOP_ReturnedMessage, true);
                    m_flowGraph->ActivatePort(addr, messageBody);

                    SFlowAddress successAddr(m_flowNodeId, EOP_Success, true);
                    m_flowGraph->ActivatePort(successAddr, true);
                };

            AZ::JobContext* jobContext{ nullptr };
            EBUS_EVENT_RESULT(jobContext, AZ::JobManagerBus, GetGlobalContext);
            auto job = AZ::CreateJobFunction(lambda, true, jobContext);
            job->Start();
        }

        m_sqsQueue->Delete(message);
    }

    void FlowNode_SQSPoller::OnArnReceivedHandler(const Aws::Queues::Sqs::SQSQueue*, const Aws::String& arn)
    {
        string arnStr = arn.c_str();

        if (!arn.empty())
        {
            auto lambda = [=]()
                {
                    SFlowAddress arnAddr(m_flowNodeId, EOP_QueueArn, true);
                    m_flowGraph->ActivatePort(arnAddr, arnStr);
                };

            AZ::JobContext* jobContext{ nullptr };
            EBUS_EVENT_RESULT(jobContext, AZ::JobManagerBus, GetGlobalContext);
            auto job = AZ::CreateJobFunction(lambda, true, jobContext);
            job->Start();
        }
    }

    REGISTER_CLASS_TAG_AND_FLOW_NODE(CLASS_TAG, FlowNode_SQSPoller);
}
