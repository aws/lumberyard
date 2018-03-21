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

#include "UserPoolTokenRetrievalStrategy.h"

#include "CloudGemPlayerAccountSystemComponent.h"

namespace CloudGemPlayerAccount
{
    UserPoolTokenRetrievalStrategy::UserPoolTokenRetrievalStrategy(CloudGemPlayerAccountSystemComponent* playerAccountComponent)
        : m_playerAccountComponent(playerAccountComponent)
    {}

    // This method is a little odd in the context of user pools because it was designed for other auth providers where the access token and refresh token don't come together.
    Aws::Auth::LoginAccessTokens UserPoolTokenRetrievalStrategy::RetrieveLongTermTokenFromAccessToken(const Aws::String& accessToken)
    {
        if (m_playerAccountComponent)
        {
            return m_playerAccountComponent->RetrieveLongTermTokenFromAuthToken(accessToken);
        }

        Aws::Auth::LoginAccessTokens token;
        token.accessToken = accessToken;

        return token;
    }

    Aws::Auth::LoginAccessTokens UserPoolTokenRetrievalStrategy::RefreshAccessTokens(const Aws::Auth::LoginAccessTokens& tokens)
    {
        // Something to consider:   As long as the refresh token stays the same, things should be fine.
        //                          However, if the refresh token ever changes, the mapping could get out of synch

        if (m_playerAccountComponent)
        {
            return m_playerAccountComponent->RefreshAccessTokens(tokens);
        }

        return tokens;
    }
}   // namespace CloudGemPlayerAccount