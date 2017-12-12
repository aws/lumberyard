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

#include <aws/identity-management/auth/PersistentCognitoIdentityProvider.h>
#include <mutex>

namespace CloudGemFramework
{
    class TokenRetrievalStrategy
    {
    public:
        virtual ~TokenRetrievalStrategy() = default;
        virtual Aws::Auth::LoginAccessTokens RetrieveLongTermTokenFromAccessToken(const Aws::String&) = 0;
        virtual Aws::Auth::LoginAccessTokens RefreshAccessTokens(const Aws::Auth::LoginAccessTokens&) = 0;
    };

    class TokenRetrievingPersistentIdentityProvider
        : public Aws::Auth::PersistentCognitoIdentityProvider
    {
    public:
        TokenRetrievingPersistentIdentityProvider(const Aws::String& identityPoolId, const Aws::String& accountId, const Aws::String& provider,
            const Aws::String& authToken, Aws::Map<Aws::String, std::shared_ptr<TokenRetrievalStrategy> >& strategies, const char* identitiesFilePath)
            : m_internalIdentityProvider(identityPoolId, accountId, identitiesFilePath)
            , m_provider(provider)
            , m_authToken(authToken)
            , m_tokenRetrievalStrategies(strategies)
        {}

        TokenRetrievingPersistentIdentityProvider(const Aws::String& identityPoolId, const Aws::String& accountId, Aws::Map<Aws::String, std::shared_ptr<TokenRetrievalStrategy> >& strategies, const char* identitiesFilePath)
            : m_internalIdentityProvider(identityPoolId, accountId, identitiesFilePath)
            , m_tokenRetrievalStrategies(strategies)
        {}

        virtual ~TokenRetrievingPersistentIdentityProvider() = default;

        void Prefetch();
        void AddTokenRetrievalStrategy(const char* provider, std::shared_ptr<TokenRetrievalStrategy> strategy);
        void RemoveTokenRetrievalStrategy(const char* provider);
        inline bool HasIdentityId() const override { return m_internalIdentityProvider.HasIdentityId(); }
        bool HasLogins() const override { return m_internalIdentityProvider.HasLogins(); }
        Aws::String GetIdentityId() const override { return m_internalIdentityProvider.GetIdentityId(); }
        Aws::Map<Aws::String, Aws::Auth::LoginAccessTokens> GetLogins() override;
        inline Aws::String GetAccountId() const override { return m_internalIdentityProvider.GetAccountId(); }
        inline Aws::String GetIdentityPoolId() const override { return m_internalIdentityProvider.GetIdentityPoolId(); }
        inline void PersistIdentityId(const Aws::String& identityId) override { m_internalIdentityProvider.PersistIdentityId(identityId); }
        inline void PersistLogins(const Aws::Map<Aws::String, Aws::Auth::LoginAccessTokens>& logins) override { m_internalIdentityProvider.PersistLogins(logins); }

    private:
        Aws::Auth::DefaultPersistentCognitoIdentityProvider m_internalIdentityProvider;
        Aws::String m_provider;
        Aws::String m_authToken;
        Aws::Map<Aws::String, std::shared_ptr<TokenRetrievalStrategy> > m_tokenRetrievalStrategies;
        std::mutex m_loginsMutex;
    };
}