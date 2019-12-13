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
#include "CloudGemPlayerAccount_precompiled.h"
#include "AuthTokenGroup.h"

// The AWS Native SDK AWSAllocator triggers a warning due to accessing members of std::allocator directly.
// AWSAllocator.h(70): warning C4996: 'std::allocator<T>::pointer': warning STL4010: Various members of std::allocator are deprecated in C++17.
// Use std::allocator_traits instead of accessing these members directly.
// You can define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING or _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS to acknowledge that you have received this warning.

AZ_PUSH_DISABLE_WARNING(4251 4996, "-Wunknown-warning-option")
#include <aws/core/utils/DateTime.h>
AZ_POP_DISABLE_WARNING

namespace
{
    // Used when comparing against the expiration time so that we give requests enough time to complete
    const double EXPIRATION_CUSHION_IN_SECONDS = 30.0;
}

namespace CloudGemPlayerAccount
{
    AuthTokenGroup::AuthTokenGroup()
        : m_tokenExpirationTime(0)
    {}

    AuthTokenGroup::AuthTokenGroup(const AuthTokenGroup& tokenGroup)
        : refreshToken(tokenGroup.refreshToken)
        , accessToken(tokenGroup.accessToken)
        , idToken(tokenGroup.idToken)
        , m_tokenExpirationTime(tokenGroup.m_tokenExpirationTime)
    {}

    void AuthTokenGroup::SetExpirationTime(long long secondsUntilExpiration)
    {
        m_tokenExpirationTime = static_cast<long long>(Aws::Utils::DateTime::ComputeCurrentTimestampInAmazonFormat()) 
                                + secondsUntilExpiration;
    }

    long long AuthTokenGroup::GetSecondsUntilExpiration() const
    {
        return m_tokenExpirationTime - static_cast<long long>(Aws::Utils::DateTime::ComputeCurrentTimestampInAmazonFormat() - EXPIRATION_CUSHION_IN_SECONDS);
    }

    bool AuthTokenGroup::IsExpired() const
    {
        return     !m_tokenExpirationTime
                || !accessToken.size() 
                || GetSecondsUntilExpiration() <= 0;
    }
}   // namespace CloudGemPlayerAccount
