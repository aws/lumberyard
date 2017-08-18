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

#include <ctime>
#include <atomic>

#include <aws/core/utils/memory/stl/AWSString.h>

namespace CloudGemPlayerAccount
{
    class AuthTokenGroup
    {
    public:
        Aws::String refreshToken;
        Aws::String accessToken;
        Aws::String idToken;

        AuthTokenGroup();
        // No default constructor defined for atomics
        AuthTokenGroup(const AuthTokenGroup& tokenGroup);

        void SetExpirationTime(long long secondsUntilExpiration);
        long long GetSecondsUntilExpiration() const;
        bool IsExpired() const;
        inline long long GetExpirationTime() const { return m_tokenExpirationTime; }

    protected:
        long long m_tokenExpirationTime;
    };
}   // namespace CloudGemPlayerAccount