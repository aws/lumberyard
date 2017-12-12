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

#include <Identity/TokenRetrievingPersistentIdentityProvider.h>
#include "CloudGemPlayerAccountSystemComponent.h"

namespace CloudGemPlayerAccount
{
    class RegistrationComponent;

    class UserPoolTokenRetrievalStrategy
        : public CloudGemFramework::TokenRetrievalStrategy
    {
    public:
        UserPoolTokenRetrievalStrategy(CloudGemPlayerAccountSystemComponent* playerAccountComponent);

        virtual ~UserPoolTokenRetrievalStrategy() = default;
        virtual Aws::Auth::LoginAccessTokens RetrieveLongTermTokenFromAccessToken(const Aws::String& accessToken) override;
        virtual Aws::Auth::LoginAccessTokens RefreshAccessTokens(const Aws::Auth::LoginAccessTokens& tokens) override;

    protected:
        // This component is responsible for actually getting/managing tokens. We just talk to it.
        CloudGemPlayerAccountSystemComponent* m_playerAccountComponent;
    };
}   // namespace CloudGemPlayerAccount