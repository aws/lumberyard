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

#include <aws/core/http/HttpTypes.h>
#include <aws/core/http/HttpResponse.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/functional.h>

namespace HttpRequestor
{
    //
    // the call back function for http requests.
    //
    using Callback = AZStd::function<void(const Aws::Utils::Json::JsonValue&, Aws::Http::HttpResponseCode)>;

    
    //
    // the call back function for any http text requests.
    //
    using TextCallback = AZStd::function<void(const AZStd::string&, Aws::Http::HttpResponseCode)>;


    //
    // a map of REST headers.
    //
    using Headers = AZStd::map<AZStd::string, AZStd::string>;
}
