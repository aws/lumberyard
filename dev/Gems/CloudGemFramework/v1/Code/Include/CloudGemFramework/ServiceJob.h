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

#include <AzCore/Jobs/Job.h>

#include <CloudGemFramework/AwsApiJob.h>
#include <CloudGemFramework/ServiceJobConfig.h>
#include <CloudGemFramework/RequestBuilder.h>

namespace Aws
{
    namespace Http
    {
        class HttpClient;
        class HttpRequest;
        class HttpResponse;
    }
}

namespace CloudGemFramework
{

    /// Non-templated base class for ServiceClientJobs
    class ServiceJob
        : public AwsApiJob
    {

    public:

        // To use a different allocator, extend this class and use this macro.
        AZ_CLASS_ALLOCATOR(ServiceJob, AZ::SystemAllocator, 0);

        using IConfig = IServiceJobConfig;
        using Config = ServiceJobConfig;

        static Config* GetDefaultConfig()
        {
            static AwsApiJobConfigHolder<Config> s_configHolder{};
            return s_configHolder.GetConfig(AwsApiJob::GetDefaultConfig());
        }

    protected:

        ServiceJob(bool isAutoDelete, IConfig* config)
            : AwsApiJob(isAutoDelete, config)
            , m_readRateLimiter(config->GetReadRateLimiter())
            , m_writeRateLimiter(config->GetWriteRateLimiter())
            , m_httpClient(config->GetHttpClient())
            , m_userAgent(config->GetUserAgent())
        {
        }

        virtual bool BuildRequest(RequestBuilder& request) = 0;

        virtual void ProcessResponse(const std::shared_ptr<Aws::Http::HttpResponse>& response) = 0;

        virtual std::shared_ptr<Aws::StringStream> GetBodyContent(RequestBuilder& requestBuilder);
        /// Converts an HttpMethod to a string. Used for debug output.
        static const char* HttpMethodToString(Aws::Http::HttpMethod);

        /// Enumeration of HTTP methods
        using HttpMethod = Aws::Http::HttpMethod;

    private:

        std::shared_ptr<Aws::Utils::RateLimits::RateLimiterInterface> m_readRateLimiter;
        std::shared_ptr<Aws::Utils::RateLimits::RateLimiterInterface> m_writeRateLimiter;
        std::shared_ptr<Aws::Http::HttpClient> m_httpClient;
        Aws::String m_userAgent;

        // Calls GetRequest to get a request, sends that request, then
        // calls OnResponse when response is received or an error occurs.
        void Process() override;

        std::shared_ptr<Aws::Http::HttpRequest> GetRequest();

    };

} // namespace CloudGemFramework
