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
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <mutex>

#pragma warning(push)
#pragma warning(disable: 4355 4251) // <future> includes ppltasks.h which throws a C4355 warning: 'this' used in base member initializer list
#include <aws/cognito-idp/model/InitiateAuthRequest.h>
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/identity-management/auth/PersistentCognitoIdentityProvider.h>
#pragma warning(pop)

#include "AuthTokenGroup.h"
#include <CloudGemPlayerAccount/CloudGemPlayerAccountBus.h>

#include <CloudCanvas/CloudCanvasIdentityBus.h>

/** This component provides the ability to manage user accounts via a Cognito User Pool.
    Users can register an account, recover their password, log in, update account details, etc.

    For the sake of preserving internal state (logins, etc.), there should only be one instance
    of this component and it should exist for the lifetime of the application.
    This is *not* currently enforced in any way and may change in the future.
*/
namespace CloudGemPlayerAccount
{
    typedef std::function<void(const BasicResultInfo& basicResult, const Model::AuthenticationResultType& authenticationResult)> AuthenticateWithRefreshTokenHandler;
    typedef std::function<void(const BasicResultInfo& basicResult)> GetUserForAccessTokenHandler;
    typedef std::function<void(AuthTokenGroup refreshedTokens)> RefreshAccessTokensHandler;

    class CloudGemPlayerAccountSystemComponent
        : public AZ::Component
        , protected CloudGemPlayerAccountRequestBus::Handler
        , public CloudGemFramework::CloudCanvasPlayerIdentityNotificationBus::Handler
    {
    public:
        static const char* COMPONENT_DISPLAY_NAME;
        static const char* COMPONENT_DESCRIPTION;
        static const char* COMPONENT_CATEGORY;
        static const char* SERVICE_NAME;

        AZ_COMPONENT(CloudGemPlayerAccountSystemComponent, "{76F24D04-D36F-41EF-BE80-976190C379B6}");
		CloudGemPlayerAccountSystemComponent() = default;
        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // These functions are needed by UserPoolTokenRetrievalStrategy. They are *synchronous* functions.
        Aws::Auth::LoginAccessTokens RetrieveLongTermTokenFromAuthToken(const Aws::String& authToken);
        Aws::Auth::LoginAccessTokens RefreshAccessTokens(const Aws::Auth::LoginAccessTokens& tokens);

    protected:
		CloudGemPlayerAccountSystemComponent(const CloudGemPlayerAccountSystemComponent&) = delete;
        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // CloudGemPlayerAccountRequestBus interface implementation
        virtual AZ::u32 GetServiceStatus() override;
        virtual AZ::u32 GetCurrentUser() override;

        virtual bool HasCachedCredentials(const AZStd::string& username) override;

        virtual AZ::u32 SignUp(const AZStd::string& username, const AZStd::string& password, const UserAttributeValues& attributes) override;
        virtual AZ::u32 ConfirmSignUp(const AZStd::string& username, const AZStd::string& confirmationCode) override;
        virtual AZ::u32 ResendConfirmationCode(const AZStd::string& username) override;

        virtual AZ::u32 ForgotPassword(const AZStd::string& username) override;
        virtual AZ::u32 ConfirmForgotPassword(const AZStd::string& username, const AZStd::string& password, const AZStd::string& confirmationCode) override;

        virtual AZ::u32 InitiateAuth(const AZStd::string& username, const AZStd::string& password) override;
        virtual AZ::u32 RespondToForceChangePasswordChallenge(const AZStd::string& username, const AZStd::string& currentPassword, const AZStd::string& newPassword) override;
        virtual AZ::u32 SignOut(const AZStd::string& username) override;

        // Everything below requires the user to be signed in
        virtual AZ::u32 ChangePassword(const AZStd::string& username, const AZStd::string& previousPassword, const AZStd::string& proposedPassword) override;
        virtual AZ::u32 GlobalSignOut(const AZStd::string& username) override;
        virtual AZ::u32 DeleteOwnAccount(const AZStd::string& username) override;
        virtual AZ::u32 GetUser(const AZStd::string& username) override;
        virtual AZ::u32 VerifyUserAttribute(const AZStd::string& username, const AZStd::string& attributeName, const AZStd::string& confirmationCode) override;
        virtual AZ::u32 DeleteUserAttributes(const AZStd::string& username, const UserAttributeList& attributesToDelete) override;
        virtual AZ::u32 UpdateUserAttributes(const AZStd::string& username, const UserAttributeValues& attributes) override;

        virtual AZ::u32 GetPlayerAccount() override;
        virtual AZ::u32 UpdatePlayerAccount(const PlayerAccount& playerAccount) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // PlayerIdentityNotificationBus interface implementation
        void OnBeforeIdentityUpdate() override;
        void OnAfterIdentityUpdate() override {};

        ////////////////////////////////////////////////////////////////////////

        using AuthCallback = AZStd::function<void(const BasicResultInfo&)>;

        static AZStd::string GetTimestamp();

        // These functions should be the preferred way to set/get data over accessing m_usernamesToTokens directly for threading reasons.
        // Each returns a copy of the current state of the AuthTokenGroup.
        AuthTokenGroup CacheUserAuthDetails(const AZStd::string& username, const Model::AuthenticationResultType& authResult);
        AuthTokenGroup GetUserAuthDetails(const AZStd::string& username);

        bool GetCachedUsernameForRefreshToken(AZStd::string& username, const AZStd::string& refreshToken);
        void AuthenticateWithRefreshToken(const AuthenticateWithRefreshTokenHandler& handler, const AZStd::string& refreshToken);
        void GetUserForAcccessToken(AZ::u32 requestId, const GetUserForAccessTokenHandler& handler, const AZStd::string& accessToken);

        void RespondToAuthChallenge(AZ::u32 requestId, Model::ChallengeNameType challengeName, AZStd::string challengeType, const Model::InitiateAuthResult& result,
            const AZStd::string currentPassword, const AZStd::string newPassword, AuthCallback onComplete);
        void CallInitiateAuth(AZ::u32 requestId, const AZStd::string& username, const AZStd::string& currentPassword, const AZStd::string& newPassword,
            const AuthCallback& authCallback);

        void LocalSignOut(const AZStd::string& username);

        template<typename Job> void SignOutIfTokenIsInvalid(const Job& job, const AZStd::string& username);

        void RefreshAccessTokensIfExpired(const AZStd::string& username, const RefreshAccessTokensHandler& handler);

        AZStd::string GetPoolId();
        AZStd::string GetPoolRegion();
        AZStd::string GetClientId();

        // Serialized properties //
        AZStd::string m_userPoolLogicalName{"CloudGemPlayerAccount.PlayerUserPool"};   // E.g., "MyResourceGroupName.MyUserPoolName"
        AZStd::string m_clientAppName{"DefaultClientApp"};  // The App name of the user pool client app (found under "Apps" in the AWS console for each user pool). E.g., "WinApp"

                                        // Non-serialized properties //
        AZStd::unordered_map<AZStd::string, AuthTokenGroup> m_usernamesToTokens;    // CacheUserAuthDetails() and GetUserAuthDetails() should be the 
                                                                                    // preferred way to set/get data over accessing m_usernamesToTokens directly for threading reasons
        AZStd::string m_userPoolProviderName;   // Used in the Logins mapping. Takes the form "cognito-idp.us-east-1.amazonaws.com/us-east-1_123456789"
        std::mutex m_tokenAccessMutex;  // Used to make sure that access tokens aren't being read as they're being written to

        std::shared_ptr<Aws::Auth::AWSCredentialsProvider> m_anonymousCredentialsProvider;

        AZStd::atomic_ulong m_nextRequestId{1};
    };

} // namespace CloudGemPlayerAccount
