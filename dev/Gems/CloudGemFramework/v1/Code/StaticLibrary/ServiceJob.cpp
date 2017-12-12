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
#include "StdAfx.h"

#include <aws/core/http/HttpRequest.h>
#include <aws/core/http/HttpResponse.h>
#include <aws/core/http/HttpClient.h>
#include <aws/core/http/HttpClientFactory.h>
#include <aws/core/utils/stream/ResponseStream.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#include <aws/core/auth/AWSAuthSigner.h>

#include <CloudGemFramework/ServiceJob.h>

namespace CloudGemFramework
{

    void ServiceJob::Process()
    {
        // Someday the AWS Http client may support real I/O. The GetRequest
        // and OnResponse methods are designed with that in mind. When that
        // feature is available, we can use the AZ::Job defined Increment-
        // DependentCount method, start the async i/o, and call WaitFor-
        // Children. When the i/o completes, we would call Decrement-
        // DependentCount, which would cause WaitForChildren to return. We
        // would then call OnResponse.
        std::shared_ptr<Aws::Http::HttpRequest> request = GetRequest();
        if(request)
        {
            std::shared_ptr<Aws::Http::HttpResponse> response = m_httpClient->MakeRequest(
                *request.get(),
                m_readRateLimiter.get(),
                m_writeRateLimiter.get()
            );
            ProcessResponse(response);
        }
        else
        {
            // We need to Process a null response to indicate no request was ever made.
            ProcessResponse({});
        }
    }

    const char* ServiceJob::HttpMethodToString(Aws::Http::HttpMethod httpMethod)
    {
        switch(httpMethod)
        {
        case Aws::Http::HttpMethod::HTTP_GET:
            return "GET";
        case Aws::Http::HttpMethod::HTTP_POST:
            return "POST";
        case Aws::Http::HttpMethod::HTTP_DELETE:
            return "DELETE";
        case Aws::Http::HttpMethod::HTTP_PUT:
            return "PUT";
        case Aws::Http::HttpMethod::HTTP_HEAD:
            return "HEAD";
        case Aws::Http::HttpMethod::HTTP_PATCH:
            return "PATCH";
        default:
            AZ_Error("TODO", false, "Unknown HttpMethod: %i", httpMethod);
            return "UNKNOWN";
        }
    }

    std::shared_ptr<Aws::Http::HttpRequest> ServiceJob::GetRequest()
    {

        RequestBuilder requestBuilder{};
        if(BuildRequest(requestBuilder))
        {

            std::shared_ptr<Aws::Http::HttpRequest> request = Aws::Http::CreateHttpRequest(
                requestBuilder.GetRequestUrl(),
                requestBuilder.GetHttpMethod(),
                &Aws::Utils::Stream::DefaultResponseStreamFactoryMethod
            );

            std::shared_ptr<Aws::StringStream> bodyContent = GetBodyContent(requestBuilder);
            if(bodyContent)
            {
                size_t len = bodyContent->str().size();
                if (len > 0)
                {
                    AZStd::string lenstr = AZStd::string::format("%d", len);
                    request->SetContentLength(lenstr.c_str());
                    request->SetContentType("application/json");
                    request->AddContentBody(bodyContent);
                }
            }

            request->SetAccept("application/json");
            request->SetAcceptCharSet("utf-8");
            request->SetUserAgent(m_userAgent);

            auto awsAuthSigner = requestBuilder.GetAWSAuthSigner();
            if (awsAuthSigner)
            {
                awsAuthSigner->SignRequest(*request);
            }

            return request;

        }
        else
        {
            return nullptr; // indicate an error, OnResponse will not be called
        }

    }

    std::shared_ptr<Aws::StringStream> ServiceJob::GetBodyContent(RequestBuilder& requestBuilder)
    {
        return requestBuilder.GetBodyContent();
    }

} // namespace CloudGemFramework
