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

#include <CloudGemFramework/HttpRequestJob.h>

#include <aws/core/http/HttpResponse.h>

#include <sstream>

namespace CloudGemFramework
{

    bool HttpRequestJob::BuildRequest(RequestBuilder& request)
    {
        request.SetHttpMethod(m_httpMethod);
        request.SetRequestUrl(m_url);
        return true;
    }

    void HttpRequestJob::SetHttpMethod(AZStd::string method)
    {
        if (method == "POST")
        {
            m_httpMethod = Aws::Http::HttpMethod::HTTP_POST;
        }
        else if (method == "GET")
        {
            m_httpMethod = Aws::Http::HttpMethod::HTTP_GET;
        }
        else if (method == "DELETE")
        {
            m_httpMethod = Aws::Http::HttpMethod::HTTP_DELETE;
        }
        else if (method == "PUT")
        {
            m_httpMethod = Aws::Http::HttpMethod::HTTP_PUT;
        }
        else if (method == "HEAD")
        {
            m_httpMethod = Aws::Http::HttpMethod::HTTP_HEAD;
        }
        else if (method == "PATCH")
        {
            m_httpMethod = Aws::Http::HttpMethod::HTTP_PATCH;
        }
        else
        {
            m_httpMethod = Aws::Http::HttpMethod::HTTP_GET;
        }
    }

    std::shared_ptr<Aws::StringStream> HttpRequestJob::GetBodyContent(RequestBuilder& requestBuilder)
    {
        requestBuilder.WriteJsonBodyRaw(m_jsonBody.c_str());
        return requestBuilder.GetBodyContent();
    }

    void HttpRequestJob::Success(int responseCode, AZStd::string content)
    {
        if (m_successCallback)
        {
            m_successCallback(responseCode, content);
        }
    }

    void HttpRequestJob::Failure(int responseCode)
    {
        if (m_failureCallback)
        {
            m_failureCallback(responseCode);
        }
    }

    void HttpRequestJob::ProcessResponse(const std::shared_ptr<Aws::Http::HttpResponse>& response)
    {
        if (!response)
        {
            // If we didn't get a response no request was made.  This happens when the SDK has an internal server error
            // or if we weren't configured properly - no mappings in order to supply an endpoint URL for example.
            Failure(0);
            return;
        }

        // get response code
        int responseCode = static_cast<int>(response->GetResponseCode());
        if (responseCode >= 200 && responseCode <= 299)
        {
            // read body into string
            Aws::IOStream& responseBody = response->GetResponseBody();
            std::istreambuf_iterator<AZStd::string::value_type> eos;
            AZStd::string responseContent = AZStd::string{std::istreambuf_iterator<AZStd::string::value_type>(responseBody),eos};
            Success(responseCode, responseContent);
        }
        else
        {
            Failure(responseCode);
        }
    }

} // namespace CloudGemFramework
