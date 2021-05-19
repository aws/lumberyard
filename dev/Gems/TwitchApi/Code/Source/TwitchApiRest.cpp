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

#include <AzCore/std/time.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/string/conversions.h>
#include <TwitchApi/TwitchApiNotifyBus.h>
#include <HttpRequestor/HttpRequestorBus.h>
#include "TwitchApiRest.h"

namespace TwitchApi
{
    ITwitchApiRestPtr ITwitchApiRest::Alloc()
    {
        return AZStd::make_shared<TwitchApiRest>();
    }

    void TwitchApiRest::FlushEvents()
    {
        TwitchApiNotifyBus::ExecuteQueuedEvents();
    }

    void TwitchApiRest::AddHttpRequest(AZStd::string_view URI, Aws::Http::HttpMethod method, const HttpRequestor::Headers& headers, const HttpRequestor::Callback& callback)
    {
        HttpRequestor::HttpRequestorRequestBus::Broadcast(&HttpRequestor::HttpRequestorRequests::AddRequestWithHeaders, BuildApiUrl(URI), method, headers, callback);
    }

    void TwitchApiRest::AddHttpRequest(AZStd::string_view URI, Aws::Http::HttpMethod method, const HttpRequestor::Headers& headers, AZStd::string_view body, const HttpRequestor::Callback& callback)
    {
        HttpRequestor::HttpRequestorRequestBus::Broadcast(&HttpRequestor::HttpRequestorRequests::AddRequestWithHeadersAndBody, BuildApiUrl(URI), method, headers, body, callback);
    }

    AZStd::string TwitchApiRest::BuildApiUrl(AZStd::string_view family) const
    {
        /*
        ** Return a url to the Twitch (Helix) API, https://api.twitch.tv/helix/<family>
        */
        return AZStd::string::format("%s://%s/%s/%.*s", PROTOCOL, BASE_PATH, API_NAME, aznumeric_cast<int>(family.size()), family.data());
    }

    HttpRequestor::Headers TwitchApiRest::GetUserBearerHeader()
    {
        HttpRequestor::Headers hdrs;

        AddUserBearerHeader(hdrs);
        AddClientIdHeader(hdrs);

        return hdrs;
    }

    HttpRequestor::Headers TwitchApiRest::GetAppBearerHeader()
    {
        HttpRequestor::Headers hdrs;

        AddAppBearerHeader(hdrs);
        AddClientIdHeader(hdrs);

        return hdrs;
    }

    HttpRequestor::Headers TwitchApiRest::GetUserBearerContentHeaders()
    {
        HttpRequestor::Headers hdrs;

        AddContentTypeToHeader(hdrs);
        AddUserBearerHeader(hdrs);
        AddClientIdHeader(hdrs);

        return hdrs;
    }

    HttpRequestor::Headers TwitchApiRest::GetAppBearerContentHeaders()
    {
        HttpRequestor::Headers hdrs;

        AddContentTypeToHeader(hdrs);
        AddAppBearerHeader(hdrs);
        AddClientIdHeader(hdrs);

        return hdrs;
    }

    void TwitchApiRest::AddUserBearerHeader(HttpRequestor::Headers& headers)
    {
        AZStd::string oAuthToken;
        TwitchApiRequestBus::BroadcastResult(oAuthToken, &TwitchApiRequests::GetUserAccessToken);
        headers["Authorization"] = AUTH_TYPE + oAuthToken;
    }

    void TwitchApiRest::AddAppBearerHeader(HttpRequestor::Headers& headers)
    {
        AZStd::string oAuthToken;
        TwitchApiRequestBus::BroadcastResult(oAuthToken, &TwitchApiRequests::GetAppAccessToken);
        headers["Authorization"] = AUTH_TYPE + oAuthToken;
    }

    // return the application id in a header, the rest docs refer this as the client-id, poorly named)
    void TwitchApiRest::AddClientIdHeader(HttpRequestor::Headers& headers)
    {
        AZStd::string appId;
        TwitchApiRequestBus::BroadcastResult(appId, &TwitchApiRequests::GetApplicationId);
        headers["Client-ID"] = AZStd::move(appId);
    }

    void TwitchApiRest::AddContentTypeToHeader(HttpRequestor::Headers& headers)
    {
        headers["Content-Type"] = CONTENT_TYPE;
    }

    void TwitchApiRest::AddToHeader(HttpRequestor::Headers& headers, AZStd::string_view name, AZStd::string_view key) const
    {
        headers[name] = key;
    }

    void TwitchApiRest::AddToHeader(HttpRequestor::Headers& headers, AZStd::string_view name, AZ::s64 key) const
    {
        AddToHeader(headers, name, AZStd::to_string(key));
    }

    void TwitchApiRest::AddToHeader(HttpRequestor::Headers& headers, AZStd::string_view name, AZ::u64 key) const
    {
        AddToHeader(headers, name, AZStd::to_string(key));
    }
}
