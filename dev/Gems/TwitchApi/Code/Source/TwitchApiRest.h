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

#include "ITwitchApiRest.h"

namespace TwitchApi
{
    class TwitchApiRest : public ITwitchApiRest
    {
    public:
        ~TwitchApiRest() override = default;
        void FlushEvents() override;

        void AddHttpRequest(AZStd::string_view URI, Aws::Http::HttpMethod method, const HttpRequestor::Headers& headers,
            const HttpRequestor::Callback& callback) override;
        void AddHttpRequest(AZStd::string_view URI, Aws::Http::HttpMethod method, const HttpRequestor::Headers& headers,
            AZStd::string_view body, const HttpRequestor::Callback& callback) override;

        //! Gets a set of headers containing User Access Bearer and Client-ID
        HttpRequestor::Headers GetUserBearerHeader() override;
        //! Gets a set of headers containing App Access Bearer and Client-ID
        HttpRequestor::Headers GetAppBearerHeader() override;
        //! Gets a set of headers containing User Bearer, Client-ID and Content-Type
        HttpRequestor::Headers GetUserBearerContentHeaders() override;
        //! Gets a set of headers containing App Bearer, Client-ID and Content-Type
        HttpRequestor::Headers GetAppBearerContentHeaders() override;

    private:
        static constexpr char const* PROTOCOL = "https";                  //!< Protocol to use, typically https.
        static constexpr char const* BASE_PATH = "api.twitch.tv";         //!< Base path for the Twitch API.
        static constexpr char const* API_NAME = "helix";                  //!< The name for the helix api.
        static constexpr char const* AUTH_TYPE = "Bearer ";               //!< Authorization type.
        static constexpr char const* CONTENT_TYPE = "application/json";   //!< Content type.

        //! Builds a URL for the API specified by kAPI
        //! @param family The extension of the API to build a URL for
        AZStd::string BuildApiUrl(AZStd::string_view family) const;

        //! Appends the User Bearer (aka User Access OAuth) header
        void AddUserBearerHeader(HttpRequestor::Headers& headers);
        //! Appends the App Bearer (aka App Access OAuth) header
        void AddAppBearerHeader(HttpRequestor::Headers& headers);
        //! Appends the Client-ID (aka Application ID) header
        void AddClientIdHeader(HttpRequestor::Headers& headers);
        //! Appends the Content-Type header
        void AddContentTypeToHeader(HttpRequestor::Headers& headers);

        void AddToHeader(HttpRequestor::Headers& headers, AZStd::string_view name, AZStd::string_view key) const;
        void AddToHeader(HttpRequestor::Headers& headers, AZStd::string_view name, AZ::s64 key) const;
        void AddToHeader(HttpRequestor::Headers& headers, AZStd::string_view name, AZ::u64 key) const;
    };
}

