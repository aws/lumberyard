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
#include "CloudGemFramework_precompiled.h"

#include <aws/core/http/HttpRequest.h>
#include <aws/core/http/HttpClientFactory.h>

#include <CloudGemFramework/RequestBuilder.h>
#include <CloudGemFramework/ServiceJob.h>
#include <CloudGemFramework/ServiceJobUtil.h>

namespace CloudGemFramework
{
    HttpRequestJob& ServiceJob::GetHttpRequestJob()
    {
        return *this;
    }

    const HttpRequestJob& ServiceJob::GetHttpRequestJob() const
    {
        return *this;
    }

    void ServiceJob::Start()
    {
        HttpRequestJob::Start();
    }

    std::shared_ptr<Aws::Http::HttpRequest> ServiceJob::InitializeRequest()
    {
        std::shared_ptr<Aws::Http::HttpRequest> request;
        RequestBuilder requestBuilder{};

        if (BuildRequest(requestBuilder))
        {
            request = Aws::Http::CreateHttpRequest(
                requestBuilder.GetRequestUrl(),
                requestBuilder.GetHttpMethod(),
                &Aws::Utils::Stream::DefaultResponseStreamFactoryMethod
            );

            SetAWSAuthSigner(requestBuilder.GetAWSAuthSigner());

            AZStd::string bodyString;

            if (std::shared_ptr<Aws::StringStream> bodyContent = GetBodyContent(requestBuilder))
            {
                std::istreambuf_iterator<AZStd::string::value_type> eos;
                bodyString = AZStd::string{ std::istreambuf_iterator<AZStd::string::value_type>(*bodyContent), eos };
            }

            ConfigureJsonServiceRequest(*this, bodyString);
        }

        return request;
    }


    std::shared_ptr<Aws::StringStream> ServiceJob::GetBodyContent(RequestBuilder& requestBuilder)
    {
        return requestBuilder.GetBodyContent();
    }

} // namespace CloudGemFramework
