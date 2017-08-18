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

#ifdef LMBRAWS_EXPORTS
#define LMBRAWS_API DLL_EXPORT
#else
#define LMBRAWS_API DLL_IMPORT
#endif

#include <aws/core/http/HttpResponse.h>

namespace LmbrAWS
{
    // In the SDK an enum from Http::HttpResponse::GetResponseCode is now an enum class, and is being passed into templates which don't like that.
    typedef int HttpResponseCodeType;

    // This is simply to minimize the number of places we're making this ugly cast
    LMBRAWS_API HttpResponseCodeType GetResponseCodeType(Aws::Http::HttpResponseCode responseCode);
}