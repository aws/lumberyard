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

#include <ISystem.h>

#include <CloudGemFramework/ServiceJob.h>

namespace CloudGemFramework
{
    inline void ConfigureJsonServiceRequest(HttpRequestJob& request, AZStd::string jsonBody)
    {
        size_t len = jsonBody.length();

        if (len > 0)
        {
            AZStd::string lenStr = AZStd::string::format("%d", len);
            request.SetContentLength(lenStr);
            request.SetContentType("application/json");
            request.SetBody(std::move(jsonBody));
        }

        request.SetAccept("application/json");
        request.SetAcceptCharSet("utf-8");
    }

} // namespace CloudGemFramework
