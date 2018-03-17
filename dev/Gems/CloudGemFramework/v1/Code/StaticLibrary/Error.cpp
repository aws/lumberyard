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

#include <CloudGemFramework/Error.h>
#include <CloudGemFramework/JsonObjectHandler.h>

namespace CloudGemFramework
{
    
    const AZStd::string Error::TYPE_NETWORK_ERROR{"NetworkError"};
    const AZStd::string Error::TYPE_CLIENT_ERROR{"ClientError"};
    const AZStd::string Error::TYPE_SERVICE_ERROR{"ServiceError"};
    const AZStd::string Error::TYPE_CONTENT_ERROR{"ContentError"};

    bool Error::OnJsonKey(const char* key, JsonReader& reader)
    {
        if(strcmp(key, "errorType") == 0) return reader.Accept(type);
        if(strcmp(key, "errorMessage") == 0) return reader.Accept(message);
        // Fail if there is any unexpected content. For errors, This will cause all the content to be used 
        // in the error message.
        return false; 
    }

}
