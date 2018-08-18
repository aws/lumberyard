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

#include "AWSBehaviorAPI.h"

#include <aws/core/http/HttpClient.h>
#include <aws/core/http/HttpClientFactory.h>
#include <aws/core/auth/AWSCredentialsProvider.h>

#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Jobs/JobManagerBus.h>

#include <CloudCanvas/CloudCanvasIdentityBus.h>
#include <CloudGemFramework/ServiceJob.h>
#include <CloudGemFramework/CloudGemFrameworkBus.h>
#include <CloudCanvas/CloudCanvasMappingsBus.h>
#include <CloudCanvasCommon/CloudCanvasCommonBus.h>

/// To use a specific AWS API request you have to include each of these.
#pragma warning(disable: 4355) // <future> includes ppltasks.h which throws a C4355 warning: 'this' used in base member initializer list
#include <aws/lambda/LambdaClient.h>
#include <aws/lambda/model/ListFunctionsRequest.h>
#include <aws/lambda/model/ListFunctionsResult.h>
#include <aws/lambda/model/InvokeRequest.h>
#include <aws/lambda/model/InvokeResult.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/core/utils/Outcome.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#pragma warning(default: 4355)

namespace CloudGemAWSScriptBehaviors
{
    AWSBehaviorAPI::AWSBehaviorAPI() :
        m_resourceName(),
        m_query(),
        m_httpMethod(),
        m_jsonParam(),
        m_httpMethods()
    {
        // VS2013 doesn't support initializer lists in constructors so 
        // instead just add them to the map here
        m_httpMethods["GET"] = Aws::Http::HttpMethod::HTTP_GET;
        m_httpMethods["POST"] = Aws::Http::HttpMethod::HTTP_POST;
        m_httpMethods["DELETE"] = Aws::Http::HttpMethod::HTTP_DELETE;
        m_httpMethods["PUT"] = Aws::Http::HttpMethod::HTTP_PUT;
        m_httpMethods["PATCH"] = Aws::Http::HttpMethod::HTTP_PATCH;
    }

    void AWSBehaviorAPI::ReflectSerialization(AZ::SerializeContext* serializeContext)
    {
        if (serializeContext)
        {
            serializeContext->Class<AWSBehaviorAPI>()
                ->Field("ResourceName", &AWSBehaviorAPI::m_resourceName)
                ->Field("Query", &AWSBehaviorAPI::m_query)
                ->Field("HTTPMethod", &AWSBehaviorAPI::m_httpMethod)
                ->Version(1);
        }
    }

    void AWSBehaviorAPI::ReflectBehaviors(AZ::BehaviorContext* behaviorContext)
    {
        behaviorContext->Class<AWSBehaviorAPI>("AWSBehaviorAPI")
            ->Method("Execute", &AWSBehaviorAPI::Execute, nullptr, "Run the operation on an API")
            ->Property("ResourceName", nullptr, BehaviorValueSetter(&AWSBehaviorAPI::m_resourceName))
            ->Property("Query", nullptr, BehaviorValueSetter(&AWSBehaviorAPI::m_query))
            ->Property("HTTPMethod", nullptr, BehaviorValueSetter(&AWSBehaviorAPI::m_httpMethod));

        behaviorContext->EBus<AWSBehaviorAPINotificationsBus>("AWSBehaviorAPINotificationsBus")
            ->Handler<AWSBehaviorAPINotificationsBusHandler>();
    }

    void AWSBehaviorAPI::ReflectEditParameters(AZ::EditContext* editContext)
    {
        editContext->Class<AWSBehaviorAPI>("AWSBehaviorAPI", "Wraps AWS HTTP functionality")
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &AWSBehaviorAPI::m_httpMethod, "HTTPMethod", "The HTTP method to use, GET, POST, DELETE, PUT, PATCH")
                ->Attribute(AZ::Edit::Attributes::StringList, &AWSBehaviorAPI::GetHTTPMethodList)
        ->DataElement(AZ::Edit::UIHandlers::Default, &AWSBehaviorAPI::m_query, "Query", "The URL query string to be used for your API call")
        ->DataElement(AZ::Edit::UIHandlers::Default, &AWSBehaviorAPI::m_resourceName, "ResourceName", "The logical name for the API resource from your Cloud Canvas mappings");
    }

    void AWSBehaviorAPI::Execute()
    {
        if (m_resourceName.empty())
        {
            EBUS_EVENT(AWSBehaviorAPINotificationsBus, OnError, "AWSBehaviorAPI:Execute No API resource name provided");
            return;
        }

        AZStd::string resourceName;
        EBUS_EVENT_RESULT(resourceName, CloudGemFramework::CloudCanvasMappingsBus, GetLogicalToPhysicalResourceMapping, m_resourceName.c_str());
        if (resourceName.empty())
        {
            resourceName = m_resourceName;
        }

        resourceName += m_query;

        AZ::Job* httpGetJob = CreateHttpJob(resourceName);
        if (httpGetJob)
        {
            httpGetJob->Start();
        }
    }

    Aws::Http::HttpMethod AWSBehaviorAPI::GetHTTPMethodForString(const AZStd::string& methodName) const
    {
        if (m_httpMethods.find(methodName) == m_httpMethods.end())
        {
            AZ_Printf("AWSBehaviorAPI", "Invalid HTTP method specified: %s, using GET instead", methodName.data());
            return m_httpMethods.at("GET");
        }
        else
        {
            return m_httpMethods.at(methodName);
        }
    }

    AZStd::vector<AZStd::string> AWSBehaviorAPI::GetHTTPMethodList() const
    {
        AZStd::vector<AZStd::string> retList;
        for (auto&& kvp : m_httpMethods)
        {
            retList.emplace_back(kvp.first);
        }
        return retList;
    }

    AZ::Job* AWSBehaviorAPI::CreateHttpJob(const AZStd::string& url) const
    {
        AZ::JobContext* jobContext{ nullptr };
        EBUS_EVENT_RESULT(jobContext, CloudGemFramework::CloudGemFrameworkRequestBus, GetDefaultJobContext);
        AZ::Job* job{ nullptr };
        job = AZ::CreateJobFunction([this, url]()
        {
            auto config = Aws::Client::ClientConfiguration();
            AZStd::string caFile;
            CloudCanvas::RequestRootCAFileResult requestResult;
            EBUS_EVENT_RESULT(requestResult, CloudCanvasCommon::CloudCanvasCommonRequestBus, RequestRootCAFile, caFile);
            if (caFile.length())
            {
                AZ_TracePrintf("CloudCanvas", "AWSBehaviorAPI using caFile %s with request result %d", caFile.c_str(), requestResult);
                config.caFile = caFile.c_str();
            }
            config.requestTimeoutMs = 0;
            config.connectTimeoutMs = 30000;
            
            std::shared_ptr<Aws::Http::HttpClient> httpClient = Aws::Http::CreateHttpClient(config);
            
            Aws::String requestURL{ url.c_str() };
            std::shared_ptr<Aws::Auth::AWSCredentialsProvider> credentialsProvider;
            EBUS_EVENT_RESULT(credentialsProvider, CloudGemFramework::CloudCanvasPlayerIdentityBus, GetPlayerCredentialsProvider);
            AZStd::unique_ptr<Aws::Client::AWSAuthV4Signer> authSigner;
            if (credentialsProvider)
            {
                authSigner.reset(new Aws::Client::AWSAuthV4Signer(credentialsProvider, "execute-api", ParseRegionFromAPIRequestURL(url).c_str()));
            }

            auto httpRequest(Aws::Http::CreateHttpRequest(requestURL, GetHTTPMethodForString(m_httpMethod), Aws::Utils::Stream::DefaultResponseStreamFactoryMethod));
            if (authSigner)
            {
                authSigner->SignRequest(*httpRequest);
            }
            else
            {
                AZ_Printf("AWSBehaviorAPI", "Unable to sign API request as no credentials provider available");
            }

            auto httpResponse(httpClient->MakeRequest(*httpRequest, nullptr, nullptr));

            if (!httpResponse)
            {
                EBUS_EVENT(AWSBehaviorAPINotificationsBus, OnError, "No Response Received from request!  (Internal SDK Error)");
                return;
            }

            int responseCode = static_cast<int>(httpResponse->GetResponseCode());

            AZStd::string contentType = httpResponse->GetContentType().c_str();

            if (contentType != "application/json")
            {
                EBUS_EVENT(AWSBehaviorAPINotificationsBus, OnError, "Unexpected content type returned from API request: " + contentType);
                return;
            }

            auto& body = httpResponse->GetResponseBody();
            Aws::StringStream readableOut;
            readableOut << body.rdbuf();

            EBUS_EVENT(AWSBehaviorAPINotificationsBus, OnSuccess, AZStd::string("Success!"));
            EBUS_EVENT(AWSBehaviorAPINotificationsBus, GetResponse, responseCode, readableOut.str().c_str());
        }, true, jobContext);
        return job;
    }

    AZStd::string AWSBehaviorAPI::ParseRegionFromAPIRequestURL(const AZStd::string& apiRequestURL)
    {
        int i;

        i = apiRequestURL.find('.');
        if (i != -1)
        {
            i = apiRequestURL.find('.', i + 1);
            if (i != -1)
            {
                int offset = i + 1;
                i = apiRequestURL.find('.', offset);
                if (i != -1)
                {
                    int count = i - offset;
                    return apiRequestURL.substr(offset, count);
                }
            }
        }

        AZ_Printf(
            "AWSBehaviorAPI",
            "Service request url %s does not have the expected format. Cannot determine region from the url.",
            apiRequestURL.c_str()
        );

        return "us-east-1";
    }

}
