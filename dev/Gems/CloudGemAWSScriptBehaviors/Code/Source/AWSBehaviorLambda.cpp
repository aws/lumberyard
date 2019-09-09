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

#include <CloudGemAWSScriptBehaviors_precompiled.h>

#include "AWSBehaviorLambda.h"

#include <CloudCanvas/CloudCanvasMappingsBus.h>

/// To use a specific AWS API request you have to include each of these.
#pragma warning(push)
#pragma warning(disable: 4355) // <future> includes ppltasks.h which throws a C4355 warning: 'this' used in base member initializer list
#include <aws/lambda/LambdaClient.h>
#include <aws/lambda/model/ListFunctionsRequest.h>
#include <aws/lambda/model/ListFunctionsResult.h>
#include <aws/lambda/model/InvokeRequest.h>
#include <aws/lambda/model/InvokeResult.h>
#include <aws/core/utils/Outcome.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#pragma warning(pop)

namespace CloudGemAWSScriptBehaviors
{
    AWSBehaviorLambda::AWSBehaviorLambda() :
        m_inFunctionName(),
        m_inRequestBody()
    {

    }

    void AWSBehaviorLambda::ReflectSerialization(AZ::SerializeContext* serializeContext)
    {
        if (serializeContext)
        {
            serializeContext->Class<AWSBehaviorLambda>()
                ->Field("functionName", &AWSBehaviorLambda::m_inFunctionName)
                ->Field("requestBody", &AWSBehaviorLambda::m_inRequestBody)
                ->Version(3);
        }
    }

    void AWSBehaviorLambda::ReflectBehaviors(AZ::BehaviorContext* behaviorContext)
    {
        behaviorContext->Class<AWSBehaviorLambda>("AWSLambda")
            ->Method("InvokeAWSLambda", &AWSBehaviorLambda::Invoke, nullptr, "Invokes the method on AWS")
            ->Property("functionName", nullptr, BehaviorValueSetter(&AWSBehaviorLambda::m_inFunctionName))
            ->Property("requestBody", nullptr, BehaviorValueSetter(&AWSBehaviorLambda::m_inRequestBody));

        behaviorContext->EBus<AWSBehaviorLambdaNotificationsBus>("AWSLambdaHandler")
            ->Handler<AWSBehaviorLambdaNotificationsBusHandler>();
    }

    void AWSBehaviorLambda::ReflectEditParameters(AZ::EditContext* editContext)
    {
        editContext->Class<AWSBehaviorLambda>("AWSLambda", "Wraps AWS Lambda functionality")
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &AWSBehaviorLambda::m_inFunctionName, "functionName", "Name of the AWS Lambda function to invoke")
            ->Attribute(AZ::Edit::Attributes::StringList, &AWSBehaviorLambda::GetFunctionNames)
            ->DataElement(AZ::Edit::UIHandlers::Default, &AWSBehaviorLambda::m_inRequestBody, "requestBody", "The data that will be sent to the lambda function call as arguments in JSON format.");
    }

    AZStd::vector<AZStd::string> AWSBehaviorLambda::GetFunctionNames()
    {
        AZStd::vector<AZStd::string> functionNames;
        CloudGemFramework::CloudCanvasMappingsBus::BroadcastResult(functionNames, &CloudGemFramework::CloudCanvasMappingsBus::Events::GetMappingsOfType, "AWS::Lambda::Function");

        if (AZStd::find(functionNames.begin(), functionNames.end(), m_inFunctionName) == functionNames.end())
        {
            m_inFunctionName = functionNames.size() > 0 ? functionNames[0] : "";
        }
        

        return functionNames;
    }


    void AWSBehaviorLambda::Invoke()
    {
        if (m_inFunctionName.empty())
        {
            EBUS_EVENT(AWSBehaviorLambdaNotificationsBus, OnError, "AWS Lambda Invoke: No function name provided");
            return;
        }

        using LambdaInvokeRequestJob = AWS_API_REQUEST_JOB(Lambda, Invoke);
        LambdaInvokeRequestJob::Config config(LambdaInvokeRequestJob::GetDefaultConfig());
        AZStd::string region;
        EBUS_EVENT_RESULT(region, CloudGemFramework::CloudCanvasMappingsBus, GetLogicalToPhysicalResourceMapping, "region");
        config.region = region.c_str();

        auto job = LambdaInvokeRequestJob::Create(
            [](LambdaInvokeRequestJob* job) // OnSuccess handler
            {
                Aws::IOStream& stream = job->result.GetPayload();
                std::istreambuf_iterator<AZStd::string::value_type> eos;
                AZStd::string content = AZStd::string{ std::istreambuf_iterator<AZStd::string::value_type>(stream),eos };
                AZStd::function<void()> notifyOnMainThread = [content]()
                {
                    AWSBehaviorLambdaNotificationsBus::Broadcast(&AWSBehaviorLambdaNotificationsBus::Events::OnSuccess, content.c_str());
                };
                AZ::TickBus::QueueFunction(notifyOnMainThread);
            },
            [](LambdaInvokeRequestJob* job) // OnError handler
            {
                Aws::String errorMessage = job->error.GetMessage();
                AZStd::function<void()> notifyOnMainThread = [errorMessage]()
                {
                    AWSBehaviorLambdaNotificationsBus::Broadcast(&AWSBehaviorLambdaNotificationsBus::Events::OnError, errorMessage.c_str());
                };
                AZ::TickBus::QueueFunction(notifyOnMainThread);
            },
            &config
        );

        std::shared_ptr<Aws::StringStream> stream = std::make_shared<Aws::StringStream>();
        *stream << m_inRequestBody.c_str();

        AZStd::string resourceFunctionName;
        EBUS_EVENT_RESULT(resourceFunctionName, CloudGemFramework::CloudCanvasMappingsBus, GetLogicalToPhysicalResourceMapping, m_inFunctionName.c_str());
        if (resourceFunctionName.empty())
        {
            // if there is no logical resource, user may have put in a physical resource, so try that
            // of course it could have been a typo too
            // future version of this will request available lambda names in to the edit context
            // so a list may be presented to the user, but that's not available yet
            resourceFunctionName = m_inFunctionName;
        }

        job->request.SetFunctionName(resourceFunctionName.c_str());
        job->request.SetBody(stream);
        job->Start();


    }
}