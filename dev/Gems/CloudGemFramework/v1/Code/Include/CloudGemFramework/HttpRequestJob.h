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

#include <CloudGemFramework/ServiceJob.h>

namespace CloudGemFramework
{
    using OnSuccessFunction = AZStd::function<void(int, AZStd::string)>;
    using OnFailureFunction = AZStd::function<void(int)>;
    /////////////////////////////////////////////
    // Basic Http Job
    /////////////////////////////////////////////
    class HttpRequestJob
      : public ServiceJob
    {
    public:
      AZ_CLASS_ALLOCATOR(HttpRequestJob, AZ::SystemAllocator, 0)
      HttpRequestJob(bool isAutoDelete, IServiceJobConfig* config, OnSuccessFunction success = OnSuccessFunction{}, OnFailureFunction failure = OnFailureFunction{})
        : ServiceJob(isAutoDelete, config)
        , m_successCallback{success}
        , m_failureCallback{failure}
      {
      }

        virtual ~HttpRequestJob() {}
        void ProcessResponse(const std::shared_ptr<Aws::Http::HttpResponse>&) override;

        void SetHttpMethod(AZStd::string);
        void SetUrl(Aws::String url) { m_url = url; }
        void SetJsonBody(Aws::String jsonBody) { m_jsonBody = jsonBody; }


    protected:
        bool BuildRequest(RequestBuilder&) override;
        std::shared_ptr<Aws::StringStream> GetBodyContent(RequestBuilder& requestBuilder) override;

    private:
        void Success(int responseCode, AZStd::string content);
        void Failure(int responseCode);
        Aws::String m_url;
        Aws::Http::HttpMethod m_httpMethod;
        Aws::String m_jsonBody;

        OnSuccessFunction m_successCallback;
        OnFailureFunction m_failureCallback;
    };
} // namespace CloudGemFramework
