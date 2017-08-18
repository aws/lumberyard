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
#include "StdAfx.h"
#pragma warning(push)
// Core_EXPORTS.h disables 4251, but #pragma once is preventing it from being included in this block
#pragma warning(disable: 4251) // C4251: needs to have dll-interface to be used by clients of class
#include <Configuration/ClientManagerImpl.h>
#include <Configuration/ResourceManagementLambdaBasedTokenRetrievalStrategy.h>
#include <aws/cognito-identity/model/GetCredentialsForIdentityRequest.h>
#include <aws/cognito-identity/model/GetIdRequest.h>
#include <aws/identity-management/auth/CognitoCachingCredentialsProvider.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/utils/StringUtils.h>
#pragma warning(pop)

#ifdef GetObject
#undef GetObject
#endif

#include <aws/core/utils/json/JsonSerializer.h>
#include <AzFramework/StringFunc/StringFunc.h>

using namespace Aws::Client;
using namespace Aws::Auth;

namespace
{
    const char* ALLOC_TAG = "LmbrAWS::ClientManagerImpl";
    const char* PLAYER_ANON_LOGIN_POOL_NAME = "PlayerLoginIdentityPool";
    const char* PLAYER_AUTH_ACCESS_POOL_NAME = "PlayerAccessIdentityPool";
}

namespace LmbrAWS
{
    bool ClientManagerImpl::LoadLogicalMappingsFromFile(const string& mappingsFileName)
    {
        ICryPak* cryPak = gEnv->pCryPak;
        AZ::IO::HandleType mappingsFile = cryPak->FOpen(mappingsFileName, "rt");

        if (mappingsFile == AZ::IO::InvalidHandle)
        {
            gEnv->pLog->LogWarning("AWS Logical Mappings file '%s' not found", mappingsFileName.c_str());
            return false;
        }

        size_t fileSize = cryPak->FGetSize(mappingsFile);
        char* fileData = new char[fileSize];

        cryPak->FRead(fileData, fileSize, mappingsFile);

        Aws::String fileDataStr(fileData);
        Aws::Utils::Json::JsonValue jsonValue(fileDataStr);

        delete[] fileData;
        cryPak->FClose(mappingsFile);

        return LoadLogicalMappingsFromJson(jsonValue);
    }

    bool ClientManagerImpl::LoadLogicalMappingsFromJson(const Aws::Utils::Json::JsonValue& mappingsJsonData)
    {
        const char* kLogicalMappingsName = "LogicalMappings";
        const char* kResourceTypeFieldName = "ResourceType";
        const char* kPhysicalNameFieldName = "PhysicalResourceId";
        const char* kProtectedFieldName = "Protected";

        const char* kUserPoolClientCollectionName = "UserPoolClients";
        const char* kUserPoolClientIdFieldName = "ClientId";
        const char* kUserPoolClientSecretFieldName = "ClientSecret";

        if (!mappingsJsonData.WasParseSuccessful())
        {
            CRY_ASSERT_MESSAGE(false, "Could not parse logical mappings json");
            return false;
        }

        m_isProtectedMapping = mappingsJsonData.GetBool(kProtectedFieldName);


        Aws::Utils::Json::JsonValue logicalMappingsObject = mappingsJsonData.GetObject(kLogicalMappingsName);
        Aws::Map<Aws::String, Aws::Utils::Json::JsonValue> mappingObjects = logicalMappingsObject.GetAllObjects();

        for (const std::pair<Aws::String, Aws::Utils::Json::JsonValue>& mapping : mappingObjects)
        {
            const Aws::String& logicalName = mapping.first;

            const Aws::String& resourceType = mapping.second.GetString(kResourceTypeFieldName);
            const Aws::String& physicalName = mapping.second.GetString(kPhysicalNameFieldName);

            SetLogicalMapping(resourceType.c_str(), logicalName.c_str(), physicalName.c_str());

            if (resourceType == "Custom::CognitoUserPool")
            {
                Aws::Utils::Json::JsonValue clientAppsObject = mapping.second.GetObject(kUserPoolClientCollectionName);
                Aws::Map<Aws::String, Aws::Utils::Json::JsonValue> clientApps = clientAppsObject.GetAllObjects();
                for (const std::pair<Aws::String, Aws::Utils::Json::JsonValue>& currApp : clientApps)
                {
                    const Aws::String& clientName = currApp.first;
                    const Aws::String& clientId = currApp.second.GetString(kUserPoolClientIdFieldName);
                    const Aws::String& clientSecret = currApp.second.GetString(kUserPoolClientSecretFieldName);
                    SetLogicalUserPoolClientMapping(
                        string{ logicalName.c_str() },
                        string{ clientName.c_str() },
                        string{ clientId.c_str() },
                        string{ clientSecret.c_str() });
                }
            }
        }

        return true;
    }

    void ClientManagerImpl::SetLogicalMapping(const string& type, const string& logicalName, const string& physicalName)
    {
        gEnv->pLog->Log("(Cloud Canvas) Mapping %s %s to %s", type.c_str(), logicalName.c_str(), physicalName.c_str());

        GetConfigurationParameters().SetParameter(logicalName, physicalName);

        if (type == "AWS::DynamoDB::Table")
        {
            auto& settings = GetDynamoDBManager().GetTableClientSettingsCollection().GetSettings(logicalName);
            settings.tableName = physicalName;
        }
        else if (type == "Custom::CognitoIdentityPool")
        {
            auto& settings = GetCognitoIdentityManager().GetIdentityPoolClientSettingsCollection().GetSettings(logicalName);
            settings.identityPoolId = physicalName;
        }
        else if (type == "Custom::CognitoUserPool")
        {
            auto& settings = GetCognitoIdentityProviderManager().GetIdentityProviderClientSettingsCollection().GetSettings(logicalName);
            settings.userPoolId = physicalName;
        }
        else if (type == "AWS::S3::Bucket")
        {
            auto& settings = GetS3Manager().GetBucketClientSettingsCollection().GetSettings(logicalName);
            settings.bucketName = physicalName;
        }
        else if (type == "AWS::Lambda::Function")
        {
            auto& settings = GetLambdaManager().GetFunctionClientSettingsCollection().GetSettings(logicalName);
            settings.functionName = physicalName;
        }
        else if (type == "AWS::SNS::Topic")
        {
            auto& settings = GetSNSManager().GetTopicClientSettingsCollection().GetSettings(logicalName);
            settings.topicARN = physicalName;
        }
        else if (type == "AWS::SQS::Queue")
        {
            auto& settings = GetSQSManager().GetQueueClientSettingsCollection().GetSettings(logicalName);
            settings.queueName = physicalName;
        }
        else if (type == "Configuration")
        {
            GetConfigurationParameters().SetParameter(logicalName, physicalName);

            if (logicalName == "region")
            {
                GetDefaultClientSettings().region = physicalName;
                GetAnonymousClientSettings().region = physicalName;
            }
        }
    }

    void ClientManagerImpl::SetLogicalUserPoolClientMapping(const string& logicalName, const string& clientName, const string& clientId, const string& clientSecret)
    {
        auto& settings = GetCognitoIdentityProviderManager().GetIdentityProviderClientSettingsCollection().GetSettings(logicalName);

        auto& clientInfo = settings.clientApps[clientName];
        clientInfo.clientId = clientId;
        clientInfo.clientSecret = clientSecret;
    }

    Aws::Map<Aws::String, std::shared_ptr<TokenRetrievalStrategy> > ClientManagerImpl::InitializeTokenRetrievalStrategies(const std::shared_ptr<Aws::Lambda::LambdaClient>& client, const char* lambdaName)
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

    void ClientManagerImpl::AddTokenRetrievalStrategy(const char* provider, std::shared_ptr<TokenRetrievalStrategy> strategy)
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

    void ClientManagerImpl::RemoveTokenRetrievalStrategy(const char* provider)
    {
        m_additionalStrategyMappings.erase(provider);

        if (m_authIdentityProvider)
        {
            std::shared_ptr<TokenRetrievingPersistentIdentityProvider> providerCast = std::static_pointer_cast<TokenRetrievingPersistentIdentityProvider>(m_authIdentityProvider);
            providerCast->RemoveTokenRetrievalStrategy(provider);
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
    *  This last step will actually be the way games will want to do this for steam, origin, xbox, psn etc.... since you can easily get auth tokens via their apis at game startup time.
    *  For those platforms, you should just call Login() the first time you launch the game and don't worry about the cvar stuff.
    */
    bool ClientManagerImpl::ResetPlayerIdentity()
    {
        bool retVal = true;

        auto anonymousIdentityPoolId = GetCognitoIdentityManager().GetIdentityPoolClientSettingsCollection().GetSettings(PLAYER_ANON_LOGIN_POOL_NAME).identityPoolId;
        //if the poolid has not been set, then the logical name is returned, also check that didn't happen
        if (!anonymousIdentityPoolId.empty() && anonymousIdentityPoolId != PLAYER_ANON_LOGIN_POOL_NAME)
        {
            auto accountId = GetConfigurationParameters().GetParameter("account_id");

            gEnv->pLog->LogAlways("Found resource management based identity pool %s.", anonymousIdentityPoolId.c_str());

            AZStd::string identitiesFilePath;
            AzFramework::StringFunc::Path::Join(gEnv->pFileIO->GetAlias("@user@"), ".identities", identitiesFilePath);
            auto anonymousIdentityStorage = Aws::MakeShared<DefaultPersistentCognitoIdentityProvider>(ALLOC_TAG, anonymousIdentityPoolId.c_str(), accountId.c_str(), identitiesFilePath.c_str());

            auto anonymousClientSettings = GetAnonymousClientSettings();
            m_cognitoIdentityClientAnonymous = Aws::MakeShared<Aws::CognitoIdentity::CognitoIdentityClient>(ALLOC_TAG, AWSCredentials("", ""), anonymousClientSettings);
            m_anonCredsProvider = Aws::MakeShared<CognitoCachingAnonymousCredentialsProvider>(ALLOC_TAG, anonymousIdentityStorage, m_cognitoIdentityClientAnonymous);

            auto authIdentityPoolId = GetCognitoIdentityManager().GetIdentityPoolClientSettingsCollection().GetSettings(PLAYER_AUTH_ACCESS_POOL_NAME).identityPoolId;

            //if the poolid has not been set, then the logical name is returned, also check that didn't happen
            if (!authIdentityPoolId.empty() && authIdentityPoolId != PLAYER_AUTH_ACCESS_POOL_NAME)
            {
                gEnv->pLog->LogAlways("Found resource management based identity pool %s for authenticated access.", authIdentityPoolId.c_str());
                auto authLambdaName = GetLambdaManager().GetFunctionClientSettingsCollection().GetSettings("PlayerAccessTokenExchange").functionName;
                auto loginLambdaClient = Aws::MakeShared<Aws::Lambda::LambdaClient>(ALLOC_TAG, m_anonCredsProvider, anonymousClientSettings);

                auto tokenRetrievalStrategies = InitializeTokenRetrievalStrategies(loginLambdaClient, authLambdaName.c_str());

                m_authIdentityProvider = Aws::MakeShared<TokenRetrievingPersistentIdentityProvider>(ALLOC_TAG, authIdentityPoolId.c_str(), accountId.c_str(), tokenRetrievalStrategies, identitiesFilePath.c_str());

#ifdef AUTH_TOKEN_CVAR_ENABLED  // See security warning in ClientManagerImpl.h before enabling the auth_token cvar.
                if (auto cVarAuthToken = gEnv->pConsole->GetCVar("auth_token"))
                {
                    gEnv->pLog->LogAlways("Checking CVAR for auth_token");
                    auto authTokenVar = cVarAuthToken->GetString();
                    ParsedAuthToken authToken;
                    if (ParseAuthTokensFromConsoleVariable(authTokenVar, authToken))
                    {
                        gEnv->pLog->LogAlways("Successfully parsed auth_token %s", authTokenVar);
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
                            gEnv->pLog->LogAlways("Either the auth_token variable was empty or it failed to parse. Using cached token.");
                        }
                        else
                        {
                            gEnv->pLog->LogAlways("Either the auth_token variable was empty or it failed to parse. Using anonymous player identity.");
                        }
                    }
                }
#endif // AUTH_TOKEN_CVAR_ENABLED

                auto defaultClientSettings = GetDefaultClientSettings();
                m_cognitoIdentityClientAuthenticated = Aws::MakeShared<Aws::CognitoIdentity::CognitoIdentityClient>(ALLOC_TAG, AWSCredentials("", ""), defaultClientSettings);
                m_authCredsProvider = Aws::MakeShared<CognitoCachingAuthenticatedCredentialsProvider>(ALLOC_TAG, m_authIdentityProvider, m_cognitoIdentityClientAuthenticated);
                auto credsWarmup = m_authCredsProvider->GetAWSCredentials();

                if (!credsWarmup.GetAWSAccessKeyId().empty())
                {
                    if (!m_authIdentityProvider->HasLogins())
                    {
                        gEnv->pLog->LogAlways("Anonymous Credentials pulled successfully for identity pool %s. ", authIdentityPoolId.c_str());
                    }
                    else
                    {
                        gEnv->pLog->LogAlways("Authenticated Credentials pulled successfully for identity pool %s. Tokens were cached.", authIdentityPoolId.c_str());
                    }

                    GetDefaultClientSettings().credentialProvider = m_authCredsProvider;
                }
                else
                {
                    // The credentials provider does not return error information.
                    // Check the logs, the AWS SDK may have logged a reason for not being able to provide AWS credentials.
                    if (!m_authIdentityProvider->HasLogins())
                    {
                        // No user-specific credentials were involved in this attempt, it was an anonymous call.  Is the identity pool configured correctly to allow anonymous identities?
                        gEnv->pLog->LogError("Unable to get AWS credentials for the anonymous identity.  The Cognito identity pool configured as %s has to support anonymous identities.",
                            PLAYER_AUTH_ACCESS_POOL_NAME);
                    }
                    else
                    {
                        // It's possible that the user's credentials expired or were invalidated and the user needs to sign in again.
                        gEnv->pLog->LogError("Unable to get AWS credentials for the authenticated user.");
                    }
                    retVal = false;
                }
            }
        }
        return retVal;
    }

    bool ClientManagerImpl::Login(const char* authProvider, const char* authCode, const char* refreshToken, long long tokenExpiration)
    {
        if (m_authCredsProvider)
        {
            // Perform the first half of the Enhanced (Simplified) Authflow, as we don't (seem to) need the result of GetCredentialsForIdentity().
            // See http://docs.aws.amazon.com/cognito/latest/developerguide/authentication-flow.html for more details.
            Aws::CognitoIdentity::Model::GetIdRequest getIdRequest;
            getIdRequest.SetAccountId(GetConfigurationParameters().GetParameter("account_id").c_str());
            auto& authIdentityPoolId = GetCognitoIdentityManager().GetIdentityPoolClientSettingsCollection().GetSettings(PLAYER_AUTH_ACCESS_POOL_NAME).identityPoolId;
            getIdRequest.SetIdentityPoolId(authIdentityPoolId.c_str());
            getIdRequest.AddLogins(authProvider, authCode);
            auto getIdOutcome = m_cognitoIdentityClientAnonymous->GetId(getIdRequest);  // No reason to use the auth client for this

            if (getIdOutcome.IsSuccess())
            {
                const Aws::String& identityId = getIdOutcome.GetResult().GetIdentityId();  // This is the identity ID we want to give to the Identity Provider
                m_authIdentityProvider->PersistIdentityId(identityId);

                LoginAccessTokens loginAccessTokens;
                loginAccessTokens.accessToken = authCode;
                if (refreshToken)
                {
                    loginAccessTokens.longTermToken = refreshToken;
                }
                if (tokenExpiration >= 0)
                {
                    loginAccessTokens.longTermTokenExpiry = tokenExpiration;
                }
                m_authIdentityProvider->PersistLogins(Aws::Map<Aws::String, LoginAccessTokens>({
                    { authProvider, loginAccessTokens }
                }));

                auto credsWarmup = m_authCredsProvider->GetAWSCredentials();
                if (!credsWarmup.GetAWSAccessKeyId().empty())
                {
                    ApplyConfiguration();
                    gEnv->pLog->LogAlways("Successfully logged in to authenticated identity pool");
                    return true;
                }
                else
                {
                    m_authIdentityProvider->ClearLogins();
                    gEnv->pLog->LogWarning("Login failed for authenticated identity pool");
                }
            }
            else
            {
                gEnv->pLog->LogWarning("Login failed for authenticated identity pool. GetId failed with: %s", getIdOutcome.GetError().GetExceptionName().c_str());
            }
        }
        else
        {
            gEnv->pLog->LogWarning("An authenticated identity pool has not been configured. In order to use the login feature, the resource manager must be configured for player access.");
        }

        return false;
    }

    void ClientManagerImpl::Logout()
    {
        m_authIdentityProvider->Logout();
        ApplyConfiguration();
    }

#ifdef AUTH_TOKEN_CVAR_ENABLED  // See security warning in ClientManagerImpl.h before enabling the auth_token cvar.
    bool ClientManagerImpl::ParseAuthTokensFromConsoleVariable(const char* cVarValue, ParsedAuthToken& parsedAuthToken)
    {
        if (cVarValue && strlen(cVarValue) > 1)
        {
            gEnv->pLog->LogAlways("Attempting to parse value %s.", cVarValue);
            Aws::Vector<Aws::String> authValues = Aws::Utils::StringUtils::Split(cVarValue, ':');
            if (authValues.size() == 2)
            {
                gEnv->pLog->LogAlways("Value successfully parsed");
                parsedAuthToken.provider = authValues[0];
                parsedAuthToken.code = authValues[1];
                return true;
            }
        }
        else
        {
            gEnv->pLog->LogAlways("Value to parse is empty.");
        }

        return false;
    }
#endif // AUTH_TOKEN_CVAR_ENABLED

    bool ClientManagerImpl::GetRefreshTokenForProvider(string& refreshToken, const string& provider)
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
} // Namespace LmbrAWS
