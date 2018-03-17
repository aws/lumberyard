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

#include "AWSBehaviorHTTP.h"

#include <aws/core/http/HttpClient.h>
#include <aws/core/http/HttpClientFactory.h>

#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Jobs/JobManagerBus.h>

/// To use a specific AWS API request you have to include each of these.
#pragma warning(disable: 4355) // <future> includes ppltasks.h which throws a C4355 warning: 'this' used in base member initializer list
#include <aws/lambda/LambdaClient.h>
#include <aws/lambda/model/ListFunctionsRequest.h>
#include <aws/lambda/model/ListFunctionsResult.h>
#include <aws/lambda/model/InvokeRequest.h>
#include <aws/lambda/model/InvokeResult.h>
#include <aws/core/utils/Outcome.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#pragma warning(default: 4355)

namespace CloudGemAWSScriptBehaviors
{
    AWSBehaviorHTTP::AWSBehaviorHTTP() :
        m_url()
    {

    }

    void AWSBehaviorHTTP::ReflectSerialization(AZ::SerializeContext* serializeContext)
    {
        if (serializeContext)
        {
            serializeContext->Class<AWSBehaviorHTTP>()
                ->Field("URL", &AWSBehaviorHTTP::m_url)
                ->Version(1);
        }
    }

    void AWSBehaviorHTTP::ReflectBehaviors(AZ::BehaviorContext* behaviorContext)
    {
        behaviorContext->Class<AWSBehaviorHTTP>("AWSBehaviorHTTP")
            ->Method("Get", &AWSBehaviorHTTP::Get, nullptr, "HTTP GET operation on AWS")
            ->Property("URL", nullptr, BehaviorValueSetter(&AWSBehaviorHTTP::m_url));

        behaviorContext->EBus<AWSBehaviorHTTPNotificationsBus>("AWSBehaviorHTTPNotificationsBus")
            ->Handler<AWSBehaviorHTTPNotificationsBusHandler>();
    }

    void AWSBehaviorHTTP::ReflectEditParameters(AZ::EditContext* editContext)
    {
        editContext->Class<AWSBehaviorHTTP>("AWSBehaviorHTTP", "Wraps AWS HTTP functionality")
            ->DataElement(AZ::Edit::UIHandlers::Default, &AWSBehaviorHTTP::m_url, "URL", "URL to get");
    }

    void AWSBehaviorHTTP::Get()
    {
        if (m_url.empty())
        {
            EBUS_EVENT(AWSBehaviorHTTPNotificationsBus, OnError, "AWS HTTP Get: No URL provided");
            return;
        }

        AZ::Job* httpGetJob = CreateHttpGetJob(m_url);
        if (httpGetJob)
        {
            httpGetJob->Start();
        }

    }

    AZ::Job* AWSBehaviorHTTP::CreateHttpGetJob(const AZStd::string& url) const
    {
        AZ::JobContext* jobContext{ nullptr };
        EBUS_EVENT_RESULT(jobContext, AZ::JobManagerBus, GetGlobalContext);
        AZ::Job* job{ nullptr };
        job = AZ::CreateJobFunction([url]()
        {
            std::shared_ptr<Aws::Http::HttpClient> httpClient = Aws::Http::CreateHttpClient(Aws::Client::ClientConfiguration());

            Aws::String requestURL{ url.c_str() };
            auto httpRequest(Aws::Http::CreateHttpRequest(requestURL, Aws::Http::HttpMethod::HTTP_GET, Aws::Utils::Stream::DefaultResponseStreamFactoryMethod));
            auto httpResponse(httpClient->MakeRequest(*httpRequest, nullptr, nullptr));

            if (!httpResponse)
            {
                EBUS_EVENT(AWSBehaviorHTTPNotificationsBus, OnError, "No Response Received from request!  (Internal SDK Error)");
                return;
            }

            int responseCode = static_cast<int>(httpResponse->GetResponseCode());

            auto headerMap = httpResponse->GetHeaders();
            AZStd::unordered_map<AZStd::string, AZStd::string> stdHeaderMap;
            for (auto headerContent : headerMap)
            {
                stdHeaderMap.emplace(headerContent.first.c_str(), headerContent.second.c_str());
            }

            AZStd::string contentType = httpResponse->GetContentType().c_str();

            AZStd::string returnString;
            auto& body = httpResponse->GetResponseBody();
            Aws::StringStream readableOut;
            readableOut << body.rdbuf();
            returnString = readableOut.str().c_str();
            
            EBUS_EVENT(AWSBehaviorHTTPNotificationsBus, OnSuccess, AZStd::string("Success!"));
            EBUS_EVENT(AWSBehaviorHTTPNotificationsBus, GetResponse, responseCode, stdHeaderMap, contentType, returnString);
        }, true, jobContext);
        return job;
    }
}