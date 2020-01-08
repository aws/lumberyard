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
#include <CloudGemFramework_precompiled.h>
#include <Identity/TokenRetrievingPersistentIdentityProvider.h>
// The AWS Native SDK AWSAllocator triggers a warning due to accessing members of std::allocator directly.
// AWSAllocator.h(70): warning C4996: 'std::allocator<T>::pointer': warning STL4010: Various members of std::allocator are deprecated in C++17.
// Use std::allocator_traits instead of accessing these members directly.
// You can define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING or _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS to acknowledge that you have received this warning.

#include <AzCore/PlatformDef.h>
AZ_PUSH_DISABLE_WARNING(4251 4996, "-Wunknown-warning-option")
#include <aws/core/utils/DateTime.h>
AZ_POP_DISABLE_WARNING

using namespace Aws::Auth;

namespace CloudGemFramework
{
    void TokenRetrievingPersistentIdentityProvider::Prefetch()
    {
        GetLogins();
    }

    void TokenRetrievingPersistentIdentityProvider::AddTokenRetrievalStrategy(const char* provider, std::shared_ptr<TokenRetrievalStrategy> strategy)
    {
        m_tokenRetrievalStrategies[provider] = strategy;
    }

    void TokenRetrievingPersistentIdentityProvider::RemoveTokenRetrievalStrategy(const char* provider)
    {
        m_tokenRetrievalStrategies.erase(provider);
    }

    Aws::Map<Aws::String, LoginAccessTokens> TokenRetrievingPersistentIdentityProvider::GetLogins()
    {
        std::lock_guard<std::mutex> locker(m_loginsMutex);

        auto existingLogins = m_internalIdentityProvider.GetLogins();
        auto passedInProviderLoginIter = existingLogins.find(m_provider);

        if (!m_provider.empty() && (passedInProviderLoginIter == existingLogins.end() || passedInProviderLoginIter->second.accessToken.empty()))
        {
            LoginAccessTokens newTokens;
            newTokens.accessToken = m_authToken;
            existingLogins[m_provider] = newTokens;
        }

        bool dirty = false;
        for (auto& login : existingLogins)
        {
            auto foundStrategyIter = m_tokenRetrievalStrategies.find(login.first);
            if (foundStrategyIter != m_tokenRetrievalStrategies.end())
            {
                auto& tokenRetrievalStrategy = foundStrategyIter->second;
                auto currentTokens = login.second;
                Aws::String longTermToken = currentTokens.longTermToken;
                long long expiry = currentTokens.longTermTokenExpiry;

                if (longTermToken.empty())
                {
                    dirty = true;
                    currentTokens = tokenRetrievalStrategy->RetrieveLongTermTokenFromAccessToken(currentTokens.accessToken);
                }
                else if (expiry < Aws::Utils::DateTime::ComputeCurrentTimestampInAmazonFormat())
                {
                    dirty = true;
                    currentTokens = tokenRetrievalStrategy->RefreshAccessTokens(currentTokens);
                }

                login.second = currentTokens;
            }
        }

        if (dirty)
        {
            PersistLogins(existingLogins);
        }

        return existingLogins;
    }
}