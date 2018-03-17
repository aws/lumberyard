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

#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/http/HttpClientFactory.h>

#include <CloudGemFramework/HttpRequestJobConfig.h>

namespace CloudGemFramework
{

    void HttpRequestJobConfig::ApplySettings()
    {

        AwsApiJobConfig::ApplySettings();

        Aws::Client::ClientConfiguration config{GetClientConfiguration()};

        m_readRateLimiter = config.readRateLimiter;
        m_writeRateLimiter = config.writeRateLimiter;
        m_userAgent = config.userAgent;
        m_httpClient = Aws::Http::CreateHttpClient(config);

    }

} // namespace CloudGemFramework
