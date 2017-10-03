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

#include <CloudGemFramework/MultipartFormData.h>
#include <CloudGemFramework/HttpRequestJob.h>

namespace CloudGemFramework
{

    /// A file upload job doesn't inherit from the same lineage as other CloudGemFramework jobs, as it follows a very different pattern for its contents using file data instead of JSON.
    /// It also uses presigned URLs for uploading, so it does not go through the same configuration as other services.
    class HttpFileUploadJob
        : public HttpRequestJob
    {
    public:
        using IConfig = IHttpRequestJobConfig;
        using Config = HttpRequestJobConfig;

        static Config* GetDefaultConfig()
        {
            static AwsApiJobConfigHolder<Config> s_configHolder{};
            return s_configHolder.GetConfig(AwsApiJob::GetDefaultConfig());
        }

    public:
        AZ_CLASS_ALLOCATOR(HttpFileUploadJob, AZ::SystemAllocator, 0);

        HttpFileUploadJob(bool isAutoDelete, IHttpRequestJobConfig* config)
            : HttpRequestJob(isAutoDelete, config)
        {
            SetMethod(HttpMethod::HTTP_POST);
        }

        /// Options for setting the file contents
        void AddFile(AZStd::string fieldName, AZStd::string fileName, const char* filePath);
        void AddFileBytes(AZStd::string fieldName, AZStd::string fileName, const void* bytes, size_t length);

        /// Adds a form field to the upload
        void AddFormField(AZStd::string fieldName, AZStd::string value);

    private:
        std::shared_ptr<Aws::Http::HttpRequest> InitializeRequest() override;

    private:
        MultipartFormData m_formData;
    };

} // namespace CloudGemFramework
