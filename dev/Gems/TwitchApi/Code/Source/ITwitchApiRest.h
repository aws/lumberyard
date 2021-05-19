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

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <HttpRequestor/HttpRequestorBus.h>

namespace TwitchApi
{
    class ITwitchApiRest;
    using ITwitchApiRestPtr = AZStd::shared_ptr<ITwitchApiRest>;
    class ITwitchApiRest
    {
    public:
        static ITwitchApiRestPtr Alloc();

        virtual ~ITwitchApiRest() = default;

        //! Routes to ExecuteQueuedEvents on the notification bus
        virtual void FlushEvents() = 0;

        //! Queues an HTTP request.
        //! @param API URI of the request
        //! @param parameters HTTP parameters mapped to the format <key1>=<value1>&<key2>=<value2> and appended to the URI
        //! @param method HTTP method for the request (POST, GET, etc)
        //! @param headers HTTP headers for the request
        //! @param callback A callback to be invoked when a response to the request is received
        virtual void AddHttpRequest(AZStd::string_view URI, Aws::Http::HttpMethod method, const HttpRequestor::Headers& headers,
            const HttpRequestor::Callback& callback) = 0;

        //! Queues an HTTP request.
        //! @param API URI of the request
        //! @param parameters HTTP parameters mapped to the format <key1>=<value1>&<key2>=<value2> and appended to the URI
        //! @param method HTTP method for the request (POST, GET, etc)
        //! @param headers HTTP headers for the request
        //! @param body HTTP body for the request
        //! @param callback A callback to be invoked when a response to the request is received
        virtual void AddHttpRequest(AZStd::string_view URI, Aws::Http::HttpMethod method, const HttpRequestor::Headers& headers,
            AZStd::string_view body, const HttpRequestor::Callback& callback) = 0;

        /*
        **  Gets a set of headers containing Client-ID and Bearer (User Access OAuth token, as described in 
        ** https://dev.twitch.tv/docs/authentication/#types-of-tokens)
        */
        virtual HttpRequestor::Headers GetUserBearerHeader() = 0;
        /*
        **  Gets a set of headers containing Client-ID and Bearer (App Access OAuth token, as described in
        **  https://dev.twitch.tv/docs/authentication/#types-of-tokens)
        */
        virtual HttpRequestor::Headers GetAppBearerHeader() = 0;
        //! Gets a set of headers containing User Bearer, Client-ID and Content-Type
        virtual HttpRequestor::Headers GetUserBearerContentHeaders() = 0;
        //! Gets a set of headers containing App Bearer, Client-ID and Content-Type
        virtual HttpRequestor::Headers GetAppBearerContentHeaders() = 0;
    };
}
