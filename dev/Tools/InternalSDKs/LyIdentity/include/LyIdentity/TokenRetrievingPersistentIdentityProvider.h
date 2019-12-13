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

#include <LyIdentity/LyIdentity_EXPORTS.h>
// The AWS Native SDK AWSAllocator triggers a warning due to accessing members of std::allocator directly.
// AWSAllocator.h(70): warning C4996: 'std::allocator<T>::pointer': warning STL4010: Various members of std::allocator are deprecated in C++17.
// Use std::allocator_traits instead of accessing these members directly.
// You can define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING or _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS to acknowledge that you have received this warning.

// The LyIdentity library does not use AzCore, therefore the AZ_PUSH_DISABLE_WARNING macro is not available
#pragma warning(push)
#pragma warning(disable : 4251 4996)
#include <aws/identity-management/auth/PersistentCognitoIdentityProvider.h>
#pragma warning(pop)
#include <mutex>

namespace Ly
{
    namespace Identity
    {
        class LY_IDENTITY_API TokenRetrievalStrategy
        {
        public:
            virtual ~TokenRetrievalStrategy() = default;

            virtual Aws::Auth::LoginAccessTokens RetrieveLongTermTokenFromAccessToken(const Aws::String&) const = 0;
            virtual Aws::Auth::LoginAccessTokens RefreshAccessTokens(const Aws::Auth::LoginAccessTokens&) const = 0;
        };

        class LY_IDENTITY_API TokenRetrievingPersistentIdentityProvider
            : public Aws::Auth::PersistentCognitoIdentityProvider
        {
        public:

            using Base = Aws::Auth::PersistentCognitoIdentityProvider;

            TokenRetrievingPersistentIdentityProvider(const Aws::String& identityPoolId,
                const Aws::String& accountId,
                const Aws::String& provider,
                const Aws::String& authToken,
                Aws::Map<Aws::String, std::shared_ptr<TokenRetrievalStrategy> >& strategies);

            TokenRetrievingPersistentIdentityProvider(const Aws::String& identityPoolId,
                const Aws::String& accountId,
                Aws::Map<Aws::String, std::shared_ptr<TokenRetrievalStrategy> >& strategies);

            virtual ~TokenRetrievingPersistentIdentityProvider() = default;

            void Prefetch();

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
    } // namespace Identity
} // namespace Ly