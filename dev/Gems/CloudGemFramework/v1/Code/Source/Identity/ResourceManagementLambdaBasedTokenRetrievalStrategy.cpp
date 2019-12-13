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
#include <Identity/ResourceManagementLambdaBasedTokenRetrievalStrategy.h>
#include <AzCore/PlatformDef.h>
AZ_PUSH_DISABLE_WARNING(4251 4355 4996, "-Wunknown-warning-option")
#include <aws/lambda/model/InvokeRequest.h>
#include <aws/lambda/LambdaClient.h>
#include <aws/core/utils/Outcome.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/core/utils/logging/LogMacros.h>
AZ_POP_DISABLE_WARNING

using namespace Aws::Lambda;
using namespace Aws::Lambda::Model;
using namespace Aws::Utils::Json;
using namespace Aws::Http;
using namespace Aws::Auth;
using namespace std::chrono;

namespace CloudGemFramework
{
    static const char* CLASS_TAG = "LmbrAWS::Configuration::ResourceManagementLambdaBasedTokenRetrievalStrategy";
    static const char* ACCESS_TOKEN = "access_token";
    static const char* EXPIRES_IN = "expires_in";
    static const char* REFRESH_TOKEN = "refresh_token";
    static const char* PROVIDER = "auth_provider";
    static const char* CODE = "code";
    static const char* CONTENT_TYPE = "application/json";

    LoginAccessTokens ResourceManagementLambdaBasedTokenRetrievalStrategy::InvokeLambdaForAccessTokens(const Aws::Lambda::Model::InvokeRequest& invokeRequest) const
    {
        high_resolution_clock::time_point timeStamp = high_resolution_clock::now();
        auto outcome(m_lambdaClient->Invoke(invokeRequest));

        if (outcome.IsSuccess())
        {
            auto result(outcome.GetResultWithOwnership());
            JsonValue resultData(result.GetPayload());
            AWS_LOGSTREAM_INFO(CLASS_TAG, "Payload for lambda is " << resultData.View().WriteReadable().c_str());
            LoginAccessTokens accessTokens;

            if (resultData.View().ValueExists(ACCESS_TOKEN))
            {
                accessTokens.accessToken = resultData.View().GetString(ACCESS_TOKEN);
            }

            if (resultData.View().ValueExists(EXPIRES_IN))
            {
                accessTokens.longTermTokenExpiry = duration_cast<seconds>(timeStamp.time_since_epoch()).count() + resultData.View().GetInt64(EXPIRES_IN);
            }

            if (resultData.View().ValueExists(REFRESH_TOKEN))
            {
                accessTokens.longTermToken = resultData.View().GetString(REFRESH_TOKEN);
            }

            return accessTokens;
        }

        return LoginAccessTokens();
    }

    LoginAccessTokens ResourceManagementLambdaBasedTokenRetrievalStrategy::RetrieveLongTermTokenFromAccessToken(const Aws::String& authToken)
    {
        InvokeRequest invokeRequest;

        JsonValue requestData;
        requestData.WithString(PROVIDER, m_authProvider);
        requestData.WithString(CODE, authToken);

        auto ss(Aws::MakeShared<Aws::StringStream>(CLASS_TAG));
        *ss << requestData.View().WriteCompact();

        invokeRequest.SetFunctionName(m_lambdaToInvokeForAccessToken);
        invokeRequest.SetContentType(CONTENT_TYPE);
        invokeRequest.SetBody(ss);

        return InvokeLambdaForAccessTokens(invokeRequest);
    }

    LoginAccessTokens ResourceManagementLambdaBasedTokenRetrievalStrategy::RefreshAccessTokens(const LoginAccessTokens& accessTokens)
    {
        InvokeRequest invokeRequest;

        JsonValue requestData;
        requestData.WithString(PROVIDER, m_authProvider);
        requestData.WithString(REFRESH_TOKEN, accessTokens.longTermToken);

        auto ss(Aws::MakeShared<Aws::StringStream>(CLASS_TAG));
        *ss << requestData.View().WriteCompact();

        invokeRequest.SetFunctionName(m_lambdaToInvokeForRefresh);
        invokeRequest.SetContentType(CONTENT_TYPE);
        invokeRequest.SetBody(ss);

        return InvokeLambdaForAccessTokens(invokeRequest);
    }
}