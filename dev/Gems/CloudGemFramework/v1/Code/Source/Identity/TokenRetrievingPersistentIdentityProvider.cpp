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
#include <aws/core/utils/DateTime.h>

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