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

#include <CloudGemFramework/HttpFileUploadJob.h>

#include <aws/core/http/HttpClientFactory.h>
#include <aws/core/http/HttpResponse.h>

#include <fstream>
#include <sstream>

namespace CloudGemFramework
{
    void HttpFileUploadJob::AddFile(AZStd::string fieldName, AZStd::string fileName, const char* filePath)
    {
        m_formData.AddFile(fieldName, fileName, filePath);
    }

    void HttpFileUploadJob::AddFileBytes(AZStd::string fieldName, AZStd::string fileName, const void* bytes, size_t length)
    {
        m_formData.AddFileBytes(fieldName, fileName, bytes, length);
    }

    void HttpFileUploadJob::AddFormField(AZStd::string fieldName, AZStd::string value)
    {
        m_formData.AddField(fieldName, value);
    }

    std::shared_ptr<Aws::Http::HttpRequest> HttpFileUploadJob::InitializeRequest()
    {
        // Compose the multipart form data and apply it to this request
        MultipartFormData::ComposeResult composeResult = m_formData.ComposeForm();
        SetContentLength(std::move(composeResult.m_contentLength));
        SetContentType(std::move(composeResult.m_contentType));
        SetBody(std::move(composeResult.m_content));

        // Use a standard request
        return HttpRequestJob::InitializeRequest();
    }

} // namespace CloudGemFramework
