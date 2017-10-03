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

namespace Aws
{
    namespace Lambda
    {
        class LambdaClient;

        namespace Model
        {
            class InvokeRequest;
        }
    }
}

namespace CloudGemFramework
{
    class ResourceManagementLambdaBasedTokenRetrievalStrategy
        : public TokenRetrievalStrategy
    {
    public:
        ResourceManagementLambdaBasedTokenRetrievalStrategy(const std::shared_ptr<Aws::Lambda::LambdaClient>& lambdaClient, const Aws::String& lambdaToInvokeForAccessToken,
            const Aws::String& lambdaToInvokeForRefresh, const Aws::String& authProvider)
            : m_lambdaClient(lambdaClient)
            , m_lambdaToInvokeForAccessToken(lambdaToInvokeForAccessToken)
            , m_lambdaToInvokeForRefresh(lambdaToInvokeForRefresh)
            , m_authProvider(authProvider)
        {}

        virtual ~ResourceManagementLambdaBasedTokenRetrievalStrategy() = default;
        Aws::Auth::LoginAccessTokens RetrieveLongTermTokenFromAccessToken(const Aws::String&) override;
        Aws::Auth::LoginAccessTokens RefreshAccessTokens(const Aws::Auth::LoginAccessTokens&) override;

    private:
        Aws::Auth::LoginAccessTokens InvokeLambdaForAccessTokens(const Aws::Lambda::Model::InvokeRequest&) const;

        std::shared_ptr<Aws::Lambda::LambdaClient> m_lambdaClient;
        Aws::String m_lambdaToInvokeForAccessToken;
        Aws::String m_lambdaToInvokeForRefresh;
        Aws::String m_authProvider;
    };
}