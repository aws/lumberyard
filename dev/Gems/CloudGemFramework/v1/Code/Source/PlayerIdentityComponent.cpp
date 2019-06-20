/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*/

#include "CloudGemFramework_precompiled.h"

#include "PlayerIdentityComponent.h"

#include <Identity/ResourceManagementLambdaBasedTokenRetrievalStrategy.h>

#include <ISystem.h>

#include <AzCore/base.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/EBus/EBus.h>

#include <CloudCanvas/ICloudCanvas.h>
#include <CloudCanvas/ICloudCanvasEditor.h>

#pragma warning(push)
#pragma warning(disable: 4251) //
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/core/utils/memory/stl/AWSAllocator.h>
#include <aws/core/utils/Outcome.h>
#include <aws/cognito-identity/model/GetIdRequest.h>
#include <aws/identity-management/auth/PersistentCognitoIdentityProvider.h>
#include <aws/core/Aws.h>
#include <aws/sts/STSClient.h>
#include <aws/sts/model/AssumeRoleRequest.h>
#include <aws/sts/model/AssumeRoleResult.h>
#include <aws/sts/model/Credentials.h>
#pragma warning(pop)

#include <CloudGemFramework/AwsApiJob.h>
#include <CloudCanvas/CloudCanvasMappingsBus.h>
#include <CloudGemFramework/CloudGemFrameworkBus.h>
#include <MappingsComponent.h>

namespace CloudGemFramework
{
    const char* CloudCanvasPlayerIdentityComponent::SERVICE_NAME = "CloudCanvasPlayerIdentityService";

    static const char* ALLOC_TAG = "CloudCanvasPlayerIdentity";
    static const char* PLAYER_ANON_LOGIN_POOL_NAME = "PlayerLoginIdentityPool";
    static const char* PLAYER_AUTH_ACCESS_POOL_NAME = "PlayerAccessIdentityPool";

    void CloudCanvasPlayerIdentityComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<CloudCanvasPlayerIdentityComponent, AZ::Component>()
                ->Version(0)
                ;
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->EBus<CloudCanvasPlayerIdentityNotificationBus>("PlayerIdentityNotificationBus")
                ->Handler<CloudCanvasPlayerIdentityNotificationsHandler>();
        }
    }

    void CloudCanvasPlayerIdentityComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC(SERVICE_NAME));
    }

    void CloudCanvasPlayerIdentityComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC(SERVICE_NAME));
    }

    void CloudCanvasPlayerIdentityComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("MappingsService", AZ_CRC(CloudGemFramework::CloudCanvasMappingsComponent::SERVICE_NAME)));
    }

    void CloudCanvasPlayerIdentityComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void CloudCanvasPlayerIdentityComponent::Init()
    {
        CloudCanvasCommon::CloudCanvasCommonNotificationsBus::Handler::BusConnect();
    }

    void CloudCanvasPlayerIdentityComponent::Activate()
    {
        CloudCanvasPlayerIdentityBus::Handler::BusConnect();
    }

    void CloudCanvasPlayerIdentityComponent::Deactivate()
    {
        CloudCanvasCommon::CloudCanvasCommonNotificationsBus::Handler::BusDisconnect();
        CloudCanvasPlayerIdentityBus::Handler::BusDisconnect();
    }

    void CloudCanvasPlayerIdentityComponent::ApiInitialized()
    {
        SetPlayerCredentialsProvider(Aws::MakeShared<Aws::Auth::AnonymousAWSCredentialsProvider>("PlayerIdentityInit"));
    }

    Aws::Map<Aws::String, std::shared_ptr<TokenRetrievalStrategy> > CloudCanvasPlayerIdentityComponent::InitializeTokenRetrievalStrategies(const std::shared_ptr<Aws::Lambda::LambdaClient>& client, const char* lambdaName)
    {
        auto lwaLoginStrategy = Aws::MakeShared<ResourceManagementLambdaBasedTokenRetrievalStrategy>(ALLOC_TAG, client, lambdaName, lambdaName, "amazon");
        auto facebookLoginStrategy = Aws::MakeShared<ResourceManagementLambdaBasedTokenRetrievalStrategy>(ALLOC_TAG, client, lambdaName, lambdaName, "facebook");
        auto googleLoginStrategy = Aws::MakeShared<ResourceManagementLambdaBasedTokenRetrievalStrategy>(ALLOC_TAG, client, lambdaName, lambdaName, "google");

        // Initialize to the standard mappings
        Aws::Map<Aws::String, std::shared_ptr<TokenRetrievalStrategy> > strategyMapping =
        {
            { "www.amazon.com", lwaLoginStrategy },
            { "accounts.google.com", googleLoginStrategy },
            { "graph.facebook.com", facebookLoginStrategy }
        };

        // Add any additional mappings that may have been added from outside this class (e.g., from a gem)
        for (auto& additionalStrategy : m_additionalStrategyMappings)
        {
            strategyMapping[additionalStrategy.first] = additionalStrategy.second;
        }

        return strategyMapping;
    }

    void CloudCanvasPlayerIdentityComponent::AddTokenRetrievalStrategy(const char* provider, std::shared_ptr<TokenRetrievalStrategy> strategy)
    {
        m_additionalStrategyMappings[provider] = strategy;

        // If m_authIdentityProvider has already been created, then we need to add the new strategy to its map of strategies.
        // Otherwise, if m_authIdentityProvider hasn't already been created, the new strategy will be added when m_authIdentityProvider is created
        if (m_authIdentityProvider)
        {
            // Check if the provider's tokens are expired.  If they are, ResetPlayerIdentity needs to be called to switch from anonymous to authenticated.
            // This needs to be done before adding the token retrieval strategy because checking afterward will always return non-expired tokens.
            bool expired = false;
            auto logins = m_authIdentityProvider->GetLogins();
            auto iter = logins.find(provider);
            if (iter != logins.end())
            {
                if (iter->second.longTermTokenExpiry < Aws::Utils::DateTime::ComputeCurrentTimestampInAmazonFormat())
                {
                    expired = true;
                }
            }

            std::shared_ptr<TokenRetrievingPersistentIdentityProvider> providerCast = std::static_pointer_cast<TokenRetrievingPersistentIdentityProvider>(m_authIdentityProvider);
            providerCast->AddTokenRetrievalStrategy(provider, strategy);

            if (expired)
            {
                ResetPlayerIdentity();
            }
        }
    }

    void CloudCanvasPlayerIdentityComponent::RemoveTokenRetrievalStrategy(const char* provider)
    {
        m_additionalStrategyMappings.erase(provider);

        if (m_authIdentityProvider)
        {
            std::shared_ptr<TokenRetrievingPersistentIdentityProvider> providerCast = std::static_pointer_cast<TokenRetrievingPersistentIdentityProvider>(m_authIdentityProvider);
            providerCast->RemoveTokenRetrievalStrategy(provider);
        }
    }

    bool CloudCanvasPlayerIdentityComponent::BeginResetIdentity()
    {
        if (m_resettingIdentity)
        {
            // This can happen if OnBeforeIdentityUpdate is used to adjust the identity provider before acquiring credentials
            return false;
        }
        m_resettingIdentity = true;

        EBUS_EVENT(CloudCanvasPlayerIdentityNotificationBus, OnBeforeIdentityUpdate);
        return true;
    }


    bool CloudCanvasPlayerIdentityComponent::CheckRegionHttpAccess(const AZStd::string& identityPoolId) const
    {
        auto regionend = identityPoolId.find(":");
        if (regionend == AZStd::string::npos)
        {
            AZ_Warning("CloudCanvas", false, "Unable to retrieve region from %s", identityPoolId.c_str());
            return false;
        }
        AZStd::string regionStr { identityPoolId.substr(0, regionend) };
        AZStd::string endpointStr { AZStd::string::format("s3.%s.amazonaws.com", regionStr.c_str()).c_str() };

        int responseCode { 0 };
        EBUS_EVENT_RESULT(responseCode, CloudCanvasCommon::CloudCanvasCommonRequestBus, GetEndpointHttpResponseCode, endpointStr);

        if (responseCode == 200)
        {
            AZ_TracePrintf("CloudCanvas", "HTTPS test for identity pool %s endpoint %s returns response code %d", identityPoolId.c_str(), endpointStr.c_str(), responseCode);
            return true;
        }
        else
        {
            AZ_Warning("CloudCanvas", false, "HTTPS test for identity pool %s endpoint %s returns response code %d", identityPoolId.c_str(), endpointStr.c_str(), responseCode);
            return false;
        }
    }

    /** This next bit is a little tricky. Here's how it works.
   * If resource management is configured to use the player access functionality, the mappings will contain cognito identity pools.
   * There will be 2. The first is for calling a lambda that knows how to exchange open id and oAuth tokens for refresh tokens and longterm tokens.
   * This lambda will be called by the authenticated identity pool provider to get logins to use for auth on cognito's side. Once that happens, the player has successfully
   * logged in and we can use the new cognito provider as the main credentials provider for the game. If for whatever reason, we can't setup the authenticated pool as
   * the credentials provider, then we will fall back to the anonymous pool. This should at least allow some functionality for the game until the user can configure their
   * login information with the game.
   *
   *  As far as getting the credentials for facebook, google, or amazon for player login, we first check the identity provider persistence layer itself, since this information is cached on disk
   *  after being used the first time ( @user@/.identities ). If we don't find them in the cache, then we will look and see if the token was passed to the auth_token cvar.
   *  If we can't find it in either of these places then we setup the anonymous pool and the user has to login somehow through the game (it is now the game's responsibility to handle this).
   *  This last step will actually be the way games will want to do this for steam, origin, consoles, etc.... since you can easily get auth tokens via their apis at game startup time.
   *  For those platforms, you should just call Login() the first time you launch the game and don't worry about the cvar stuff.
   */
    bool CloudCanvasPlayerIdentityComponent::ResetPlayerIdentity()
    {
        if (!BeginResetIdentity())
        {
            return false;
        }

        bool retVal = true;

        AZStd::string anonymousIdentityPoolId;
        EBUS_EVENT_RESULT(anonymousIdentityPoolId, CloudCanvasMappingsBus, GetLogicalToPhysicalResourceMapping, PLAYER_ANON_LOGIN_POOL_NAME);

        AZStd::string accountId;
        EBUS_EVENT_RESULT(accountId, CloudCanvasMappingsBus, GetLogicalToPhysicalResourceMapping, "account_id");

        // If this is a dedicated server assume the IAM server role
        SSystemGlobalEnvironment* pEnv = GetISystem()->GetGlobalEnvironment();
        if (pEnv->IsDedicated())
        {
            GetServerIdentity();
        }
        else if (!anonymousIdentityPoolId.empty())
        {
            AZ_TracePrintf("Found resource management based identity pool %s.", anonymousIdentityPoolId.c_str());

            AZStd::string identitiesFilePath;
            AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            if (fileIO)
            {
                AzFramework::StringFunc::Path::Join(fileIO->GetAlias("@user@"), ".identities", identitiesFilePath);
            }
            auto anonymousIdentityStorage = Aws::MakeShared<Aws::Auth::DefaultPersistentCognitoIdentityProvider>(ALLOC_TAG, anonymousIdentityPoolId.c_str(), accountId.c_str(), identitiesFilePath.c_str());

            AwsApiJob::Config* defaultClientSettings { nullptr };
            EBUS_EVENT_RESULT(defaultClientSettings, CloudGemFrameworkRequestBus, GetDefaultConfig);

            m_cognitoIdentityClientAnonymous = Aws::MakeShared<Aws::CognitoIdentity::CognitoIdentityClient>(ALLOC_TAG, Aws::Auth::AWSCredentials("", ""), defaultClientSettings->GetClientConfiguration());
            m_anonCredsProvider = Aws::MakeShared<Aws::Auth::CognitoCachingAnonymousCredentialsProvider>(ALLOC_TAG, anonymousIdentityStorage, m_cognitoIdentityClientAnonymous);

            AZStd::string authIdentityPoolId;
            EBUS_EVENT_RESULT(authIdentityPoolId, CloudCanvasMappingsBus, GetLogicalToPhysicalResourceMapping, PLAYER_AUTH_ACCESS_POOL_NAME);

            if (!authIdentityPoolId.empty())
            {
                AZ_TracePrintf("Found resource management based identity pool %s for authenticated access.", authIdentityPoolId.c_str());

                AZStd::string authLambdaName;
                EBUS_EVENT_RESULT(authLambdaName, CloudCanvasMappingsBus, GetLogicalToPhysicalResourceMapping, "PlayerAccessTokenExchange");

                auto loginLambdaClient = Aws::MakeShared<Aws::Lambda::LambdaClient>(ALLOC_TAG, m_anonCredsProvider, defaultClientSettings->GetClientConfiguration());

                auto tokenRetrievalStrategies = InitializeTokenRetrievalStrategies(loginLambdaClient, authLambdaName.c_str());

                m_authIdentityProvider = Aws::MakeShared<TokenRetrievingPersistentIdentityProvider>(ALLOC_TAG, authIdentityPoolId.c_str(), accountId.c_str(), tokenRetrievalStrategies, identitiesFilePath.c_str());

#ifdef AUTH_TOKEN_CVAR_ENABLED  // See security warning in PlayerIdentityComponent.h before enabling the auth_token cvar.
                if (GetISystem() && GetISystem()->GetGlobalEnvironment())
                {
                    if (auto cVarAuthToken = GetISystem()->GetGlobalEnvironment()->pConsole->GetCVar("auth_token"))
                    {
                        AZ_TracePrintf("Checking CVAR for auth_token");
                        auto authTokenVar = cVarAuthToken->GetString();
                        ParsedAuthToken authToken;
                        if (ParseAuthTokensFromConsoleVariable(authTokenVar, authToken))
                        {
                            AZ_TracePrintf("Successfully parsed auth_token %s", authTokenVar);
                            LoginAccessTokens loginAccessTokens;
                            loginAccessTokens.accessToken = authToken.code;
                            m_authIdentityProvider->PersistLogins(Aws::Map<Aws::String, LoginAccessTokens>({
                                        { authToken.provider, loginAccessTokens }
                                    }));
                        }
                        else
                        {
                            if (m_authIdentityProvider->HasLogins())
                            {
                                AZ_TracePrintf("Either the auth_token variable was empty or it failed to parse. Using cached token.");
                            }
                            else
                            {
                                AZ_TracePrintf("Either the auth_token variable was empty or it failed to parse. Using anonymous player identity.");
                            }
                        }
                    }
                }
#endif // AUTH_TOKEN_CVAR_ENABLED

                m_cognitoIdentityClientAuthenticated = Aws::MakeShared<Aws::CognitoIdentity::CognitoIdentityClient>(ALLOC_TAG, Aws::Auth::AWSCredentials("", ""), defaultClientSettings->GetClientConfiguration());
                m_authCredsProvider = Aws::MakeShared<Aws::Auth::CognitoCachingAuthenticatedCredentialsProvider>(ALLOC_TAG, m_authIdentityProvider, m_cognitoIdentityClientAuthenticated);
                auto credsWarmup = m_authCredsProvider->GetAWSCredentials();

                if (!credsWarmup.GetAWSAccessKeyId().empty())
                {
                    if (!m_authIdentityProvider->HasLogins())
                    {
                        AZ_TracePrintf("CloudCanvas", "Anonymous Credentials pulled successfully for identity pool %s. ", authIdentityPoolId.c_str());
                    }
                    else
                    {
                        AZ_TracePrintf("CloudCanvas", "Authenticated Credentials pulled successfully for identity pool %s. Tokens were cached.", authIdentityPoolId.c_str());
                    }

                    defaultClientSettings->credentialsProvider = m_authCredsProvider;
                    SetPlayerCredentialsProvider(m_authCredsProvider);

                    EBUS_EVENT(CloudCanvasPlayerIdentityNotificationBus, OnIdentityReceived);
                }
                else
                {
                    if (!CheckRegionHttpAccess(anonymousIdentityPoolId))
                    {
                        AZ_Warning("CloudCanvas", false, "Unable to verify https access");
                    }
                    else
                    {
                        // The credentials provider does not return error information.
                        // Check the logs, the AWS SDK may have logged a reason for not being able to provide AWS credentials.
                        if (!m_authIdentityProvider->HasLogins())
                        {
                            // No user-specific credentials were involved in this attempt, it was an anonymous call.  Is the identity pool configured correctly to allow anonymous identities?
                            AZ_Error("CloudCanvas", false, "Unable to get AWS credentials for the anonymous identity.  The Cognito identity pool configured as %s has to support anonymous identities.",
                                PLAYER_AUTH_ACCESS_POOL_NAME);
                        }
                        else
                        {
                            // It's possible that the user's credentials expired or were invalidated and the user needs to sign in again.
                            AZ_Error("CloudCanvas", false, "Unable to get AWS credentials for the authenticated user.");
                        }
                    }
                    retVal = false;
                }
            }
        }
        EndResetIdentity();
        return retVal;
    }

    void CloudCanvasPlayerIdentityComponent::EndResetIdentity()
    {
        // Cleared before calling OnAfterIdentityUpdate - if changes in credentialed status (Something has expired) require further adjustments to identity
        // it is the caller's responsibility to make sure the logic doesn't result in an infinite loop.
        m_resettingIdentity = false;
        EBUS_EVENT(CloudCanvasPlayerIdentityNotificationBus, OnAfterIdentityUpdate);
    }

    bool CloudCanvasPlayerIdentityComponent::Login(const char* authProvider, const char* authCode, const char* refreshToken, long long tokenExpiration)
    {
        if (m_authCredsProvider)
        {
            // Perform the first half of the Enhanced (Simplified) Authflow, as we don't (seem to) need the result of GetCredentialsForIdentity().
            // See http://docs.aws.amazon.com/cognito/latest/developerguide/authentication-flow.html for more details.
            Aws::CognitoIdentity::Model::GetIdRequest getIdRequest;
            AZStd::string accountId;
            EBUS_EVENT_RESULT(accountId, CloudCanvasMappingsBus, GetLogicalToPhysicalResourceMapping, "account_id");
            getIdRequest.SetAccountId(accountId.c_str());

            AZStd::string authIdentityPoolId;
            EBUS_EVENT_RESULT(authIdentityPoolId, CloudCanvasMappingsBus, GetLogicalToPhysicalResourceMapping, PLAYER_AUTH_ACCESS_POOL_NAME);
            getIdRequest.SetIdentityPoolId(authIdentityPoolId.c_str());
            getIdRequest.AddLogins(authProvider, authCode);
            auto getIdOutcome = m_cognitoIdentityClientAnonymous->GetId(getIdRequest);  // No reason to use the auth client for this

            if (getIdOutcome.IsSuccess())
            {
                const Aws::String& identityId = getIdOutcome.GetResult().GetIdentityId();  // This is the identity ID we want to give to the Identity Provider
                m_authIdentityProvider->PersistIdentityId(identityId);

                Aws::Auth::LoginAccessTokens loginAccessTokens;
                loginAccessTokens.accessToken = authCode;
                if (refreshToken)
                {
                    loginAccessTokens.longTermToken = refreshToken;
                }
                if (tokenExpiration >= 0)
                {
                    loginAccessTokens.longTermTokenExpiry = tokenExpiration;
                }
                m_authIdentityProvider->PersistLogins(Aws::Map<Aws::String, Aws::Auth::LoginAccessTokens>({
                            { authProvider, loginAccessTokens }
                        }));

                auto credsWarmup = m_authCredsProvider->GetAWSCredentials();
                if (!credsWarmup.GetAWSAccessKeyId().empty())
                {
                    ApplyConfiguration();
                    AZ_TracePrintf("PlayerIdentityComponent", "Successfully logged in to authenticated identity pool");
                    return true;
                }
                else
                {
                    m_authIdentityProvider->ClearLogins();
                    AZ_Warning("PlayerIdentityComponent", false, "Login failed for authenticated identity pool");
                }
            }
            else
            {
                AZ_Warning("PlayerIdentityComponent", false, "Login failed for authenticated identity pool. GetId failed with: %s", getIdOutcome.GetError().GetExceptionName().c_str());
            }
        }
        else
        {
            AZ_Warning("PlayerIdentityComponent", false, "An authenticated identity pool has not been configured. In order to use the login feature, the resource manager must be configured for player access.");
        }

        return false;
    }

    void CloudCanvasPlayerIdentityComponent::Logout()
    {
        m_authIdentityProvider->Logout();
        ApplyConfiguration();
    }

#ifdef AUTH_TOKEN_CVAR_ENABLED  // See security warning in ClientManagerImpl.h before enabling the auth_token cvar.
    bool CloudCanvasPlayerIdentityComponent::ParseAuthTokensFromConsoleVariable(const char* cVarValue, ParsedAuthToken& parsedAuthToken)
    {
        if (cVarValue && strlen(cVarValue) > 1)
        {
            AZ_TracePrintf("Attempting to parse value %s.", cVarValue);
            Aws::Vector<Aws::String> authValues = Aws::Utils::StringUtils::Split(cVarValue, ':');
            if (authValues.size() == 2)
            {
                AZ_TracePrintf("Value successfully parsed");
                parsedAuthToken.provider = authValues[0];
                parsedAuthToken.code = authValues[1];
                return true;
            }
        }
        else
        {
            AZ_TracePrintf("Value to parse is empty.");
        }

        return false;
    }
#endif // AUTH_TOKEN_CVAR_ENABLED

    bool CloudCanvasPlayerIdentityComponent::GetRefreshTokenForProvider(AZStd::string& refreshToken, const AZStd::string& provider)
    {
        if (m_authIdentityProvider)
        {
            auto logins = m_authIdentityProvider->GetLogins();
            auto iter = logins.find(provider.c_str());
            if (iter != logins.end())
            {
                refreshToken = iter->second.longTermToken.c_str();
                return true;
            }
        }
        return false;
    }

    bool CloudCanvasPlayerIdentityComponent::GetServerIdentity()
    {
        AZStd::string serverArn;
        EBUS_EVENT_RESULT(serverArn, CloudCanvasMappingsBus, GetLogicalToPhysicalResourceMapping, "server_role_arn");

        if (serverArn.empty())
        {
            AZ_Error("CloudCanvas", "Server Role was unexpectedly empty.  Verify that the IAM Server Role ARN is available in the server mappings", serverArn.c_str());
            return false;
        }

        AZStd::string roleSessionName = "LY-Dedicated";

        AwsApiJob::Config* defaultClientSettings { nullptr };
        EBUS_EVENT_RESULT(defaultClientSettings, CloudGemFrameworkRequestBus, GetDefaultConfig);
        auto sts = Aws::MakeShared<Aws::STS::STSClient>(ALLOC_TAG, defaultClientSettings->GetClientConfiguration());

        // Assume the server role
        auto sts_req = Aws::MakeShared<Aws::STS::Model::AssumeRoleRequest>(ALLOC_TAG);
        sts_req->SetRoleArn(serverArn.c_str());
        sts_req->SetRoleSessionName(roleSessionName.c_str());

        auto response = Aws::MakeShared<Aws::STS::Model::AssumeRoleOutcome>(ALLOC_TAG, sts->AssumeRole(*sts_req));

        if (!response->IsSuccess())
        {
            AZ_Error("CloudCanvas", false, "Unable to assume dedicated server role (%s) \n", response->GetError().GetMessage());
            return false;
        }
        AZ_Printf("CloudCanvas", "Assumed server role ARN: %s", response->GetResult().GetAssumedRoleUser().GetArn().c_str());

        auto result = Aws::MakeShared<Aws::STS::Model::AssumeRoleResult>(ALLOC_TAG, response->GetResult());
        auto creds = Aws::MakeShared<Aws::STS::Model::Credentials>(ALLOC_TAG, result->GetCredentials());
        auto authCredsProvider = Aws::MakeShared<Aws::Auth::SimpleAWSCredentialsProvider>(ALLOC_TAG, creds->GetAccessKeyId(), creds->GetSecretAccessKey(), creds->GetSessionToken());
        defaultClientSettings->credentialsProvider = authCredsProvider;
        SetPlayerCredentialsProvider(authCredsProvider);

        EBUS_EVENT(CloudCanvasPlayerIdentityNotificationBus, OnIdentityReceived);
        return true;
    }


    AZStd::string CloudCanvasPlayerIdentityComponent::GetIdentityId()
    {
        if (m_authIdentityProvider)
        {
            return m_authIdentityProvider->GetIdentityId().c_str();
        }
        return "";
    }

    std::shared_ptr<Aws::Auth::AWSCredentialsProvider> CloudCanvasPlayerIdentityComponent::GetPlayerCredentialsProvider()
    {
        AZStd::lock_guard<AZStd::mutex> credentialsLock { m_credentialsMutex };
        return m_credsProvider;
    }

    void CloudCanvasPlayerIdentityComponent::SetPlayerCredentialsProvider(std::shared_ptr<Aws::Auth::AWSCredentialsProvider> credsProvider)
    {
        AZStd::lock_guard<AZStd::mutex> credentialsLock { m_credentialsMutex };
        m_credsProvider = credsProvider;
    }

    bool CloudCanvasPlayerIdentityComponent::ApplyConfiguration()
    {
        bool editorHandled { false };
        EBUS_EVENT_RESULT(editorHandled, CloudCanvas::CloudCanvasEditorRequestBus, ApplyConfiguration);

        if (!editorHandled)
        {
            // If the editor didn't handle the request, we should
            ResetPlayerIdentity();
        }
        return true;
    }
}

