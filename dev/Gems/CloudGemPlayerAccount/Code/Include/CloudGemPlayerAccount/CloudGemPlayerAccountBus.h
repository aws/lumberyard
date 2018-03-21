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
#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Memory/SystemAllocator.h>

#pragma warning(push)
#pragma warning(disable: 4355 4251) // <future> includes ppltasks.h which throws a C4355 warning: 'this' used in base member initializer list
#include <aws/cognito-idp/CognitoIdentityProviderClient.h>
#include <aws/core/client/AWSError.h>
#include <aws/core/utils/Outcome.h>
#pragma warning(pop)

#include "BasicResultInfo.h"
#include "DeliveryDetails.h"
#include "PlayerAccount.h"
#include "AccountResultInfo.h"
#include "UserAttributeList.h"
#include "UserAttributeValues.h"

using namespace Aws::CognitoIdentityProvider;

namespace CloudGemPlayerAccount
{
    class CloudGemPlayerAccountRequests
        : public AZ::EBusTraits
    {

    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual ~CloudGemPlayerAccountRequests() {}

        virtual AZ::u32 GetServiceStatus() = 0;

        virtual AZ::u32 GetCurrentUser() = 0;

        // Checks whether there are any credentials cached in memory for the given user.
        // HasCachedCredentials does not have a corresponding EBUS callback, as it is synchronous. 
        virtual bool HasCachedCredentials(const AZStd::string& /*username*/) { return false; }

        // Functions related to signing up
        virtual AZ::u32 SignUp(const AZStd::string& username, const AZStd::string& password, const UserAttributeValues& attributes) = 0;
        virtual AZ::u32 ConfirmSignUp(const AZStd::string& username, const AZStd::string& confirmationCode) = 0;
        virtual AZ::u32 ResendConfirmationCode(const AZStd::string& username) = 0;

        // Functions related to a forgotten password
        virtual AZ::u32 ForgotPassword(const AZStd::string& username) = 0;
        virtual AZ::u32 ConfirmForgotPassword(const AZStd::string& username, const AZStd::string& password, const AZStd::string& confirmationCode) = 0;

        // Functions related to signing in/out
        virtual AZ::u32 InitiateAuth(const AZStd::string& username, const AZStd::string& password) = 0;
        virtual AZ::u32 RespondToForceChangePasswordChallenge(const AZStd::string& username, const AZStd::string& currentPassword, const AZStd::string& newPassword) = 0;
        virtual AZ::u32 SignOut(const AZStd::string& username) = 0;

        // Functions that require the user to be signed in
        virtual AZ::u32 ChangePassword(const AZStd::string& username, const AZStd::string& previousPassword, const AZStd::string& proposedPassword) = 0;
        virtual AZ::u32 GlobalSignOut(const AZStd::string& username) = 0;
        virtual AZ::u32 DeleteOwnAccount(const AZStd::string& username) = 0;
        virtual AZ::u32 GetUser(const AZStd::string& username) = 0; // Returns user attribute data for the signed-in user
        virtual AZ::u32 VerifyUserAttribute(const AZStd::string& username, const AZStd::string& attributeName, const AZStd::string& confirmationCode) = 0;
        virtual AZ::u32 DeleteUserAttributes(const AZStd::string& username, const UserAttributeList& attributesToDelete) = 0;
        virtual AZ::u32 UpdateUserAttributes(const AZStd::string& username, const UserAttributeValues& attributes) = 0;

        // Player Account
        virtual AZ::u32 GetPlayerAccount() = 0;
        virtual AZ::u32 UpdatePlayerAccount(const PlayerAccount& playerAccount) = 0;
    };
    using CloudGemPlayerAccountRequestBus = AZ::EBus<CloudGemPlayerAccountRequests>;

    class CloudGemPlayerAccountRequestHandler : public CloudGemPlayerAccountRequestBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(CloudGemPlayerAccountRequestHandler, "{F9690317-4EE6-4CDC-B571-21C4A435CD99}", AZ::SystemAllocator,
            GetCurrentUser,
            SignUp,
            ConfirmSignUp,
            ResendConfirmationCode,
            ChangePassword,
            ForgotPassword,
            ConfirmForgotPassword,
            InitiateAuth,
            RespondToForceChangePasswordChallenge,
            SignOut,
            GlobalSignOut,
            DeleteOwnAccount,
            GetUser,
            VerifyUserAttribute,
            DeleteUserAttributes,
            UpdateUserAttributes,
            GetPlayerAccount,
            UpdatePlayerAccount);
    };

    class CloudGemPlayerAccountNotifications
        : public AZ::EBusTraits
    {
    public:
        virtual ~CloudGemPlayerAccountNotifications() {}

        virtual void OnGetCurrentUserComplete(const BasicResultInfo& resultInfo) {}

        virtual void OnSignUpComplete(const BasicResultInfo& resultInfo, const DeliveryDetails& deliveryDetails, bool wasConfirmed) {}
        virtual void OnConfirmSignUpComplete(const BasicResultInfo& resultInfo) {}
        virtual void OnResendConfirmationCodeComplete(const BasicResultInfo& resultInfo, const DeliveryDetails& deliveryDetails) {}

        virtual void OnForgotPasswordComplete(const BasicResultInfo& resultInfo, const DeliveryDetails& deliveryDetails) {}
        virtual void OnConfirmForgotPasswordComplete(const BasicResultInfo& resultInfo) {}

        virtual void OnInitiateAuthComplete(const BasicResultInfo& resultInfo) {}
        virtual void OnRespondToForceChangePasswordChallengeComplete(const BasicResultInfo& resultInfo) {}
        virtual void OnSignOutComplete(const BasicResultInfo& resultInfo) {} // This function always succeeds

                                                                // The following require the user to be signed in
        virtual void OnChangePasswordComplete(const BasicResultInfo& resultInfo) {}
        virtual void OnGlobalSignOutComplete(const BasicResultInfo& resultInfo) {}
        virtual void OnDeleteOwnAccountComplete(const BasicResultInfo& resultInfo) {}
        virtual void OnGetUserComplete(const BasicResultInfo& resultInfo, const UserAttributeValues& attributes, const UserAttributeList& mfaOptions) {}
        virtual void OnVerifyUserAttributeComplete(const BasicResultInfo& resultInfo) {}
        virtual void OnDeleteUserAttributesComplete(const BasicResultInfo& resultInfo) {}
        virtual void OnUpdateUserAttributesComplete(const BasicResultInfo& resultInfo, const DeliveryDetailsArray& details) {}

        virtual void OnGetPlayerAccountComplete(const AccountResultInfo& resultInfo, const PlayerAccount& playerAccount) {}
        virtual void OnUpdatePlayerAccountComplete(const AccountResultInfo& resultInfo) {}
        virtual void OnGetServiceStatusComplete(const BasicResultInfo& resultInfo) {}
    };
    using CloudGemPlayerAccountNotificationBus = AZ::EBus<CloudGemPlayerAccountNotifications>;

    class CloudGemPlayerAccountNotificationBusHandler
        : public CloudGemPlayerAccountNotificationBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(CloudGemPlayerAccountNotificationBusHandler, "{7E74121B-4746-48C7-9B66-144E80AAB26E}", AZ::SystemAllocator,
            OnGetCurrentUserComplete,
            OnSignUpComplete,
            OnConfirmSignUpComplete,
            OnResendConfirmationCodeComplete,
            OnForgotPasswordComplete,
            OnConfirmForgotPasswordComplete,
            OnInitiateAuthComplete,
            OnRespondToForceChangePasswordChallengeComplete,
            OnSignOutComplete,
            OnChangePasswordComplete,
            OnGlobalSignOutComplete,
            OnDeleteOwnAccountComplete,
            OnGetUserComplete,
            OnVerifyUserAttributeComplete,
            OnDeleteUserAttributesComplete,
            OnUpdateUserAttributesComplete,
            OnGetPlayerAccountComplete,
            OnGetServiceStatusComplete,
            OnUpdatePlayerAccountComplete);

        void OnGetCurrentUserComplete(const BasicResultInfo& resultInfo) override
        {
            Call(FN_OnGetCurrentUserComplete, resultInfo);
        }

        void OnSignUpComplete(const BasicResultInfo& resultInfo, const DeliveryDetails& deliveryDetails, bool wasConfirmed) override
        {
            Call(FN_OnSignUpComplete, resultInfo, deliveryDetails, wasConfirmed);
        }

        void OnConfirmSignUpComplete(const BasicResultInfo& resultInfo) override
        {
            Call(FN_OnConfirmSignUpComplete, resultInfo);
        }

        void OnResendConfirmationCodeComplete(const BasicResultInfo& resultInfo, const DeliveryDetails& deliveryDetails) override
        {
            Call(FN_OnResendConfirmationCodeComplete, resultInfo, deliveryDetails);
        }

        void OnForgotPasswordComplete(const BasicResultInfo& resultInfo, const DeliveryDetails& deliveryDetails) override
        {
            Call(FN_OnForgotPasswordComplete, resultInfo, deliveryDetails);
        }

        void OnConfirmForgotPasswordComplete(const BasicResultInfo& resultInfo) override
        {
            Call(FN_OnConfirmForgotPasswordComplete, resultInfo);
        }

        void OnInitiateAuthComplete(const BasicResultInfo& resultInfo) override
        {
            Call(FN_OnInitiateAuthComplete, resultInfo);
        }

        void OnRespondToForceChangePasswordChallengeComplete(const BasicResultInfo& resultInfo) override
        {
            Call(FN_OnRespondToForceChangePasswordChallengeComplete, resultInfo);
        }

        void OnSignOutComplete(const BasicResultInfo& resultInfo) override
        {
            Call(FN_OnSignOutComplete, resultInfo);
        }

        void OnChangePasswordComplete(const BasicResultInfo& resultInfo) override
        {
            Call(FN_OnChangePasswordComplete, resultInfo);
        }

        void OnGlobalSignOutComplete(const BasicResultInfo& resultInfo) override
        {
            Call(FN_OnGlobalSignOutComplete, resultInfo);
        }

        void OnDeleteOwnAccountComplete(const BasicResultInfo& resultInfo) override
        {
            Call(FN_OnDeleteOwnAccountComplete, resultInfo);
        }

        void OnGetUserComplete(const BasicResultInfo& resultInfo, const UserAttributeValues& attributes, const UserAttributeList& mfaOptions) override
        {
            Call(FN_OnGetUserComplete, resultInfo, attributes, mfaOptions);
        }

        void OnVerifyUserAttributeComplete(const BasicResultInfo& resultInfo) override
        {
            Call(FN_OnVerifyUserAttributeComplete, resultInfo);
        }

        void OnDeleteUserAttributesComplete(const BasicResultInfo& resultInfo) override
        {
            Call(FN_OnDeleteUserAttributesComplete, resultInfo);
        }

        void OnUpdateUserAttributesComplete(const BasicResultInfo& resultInfo, const DeliveryDetailsArray& details) override
        {
            Call(FN_OnUpdateUserAttributesComplete, resultInfo, details);
        }

        void OnGetPlayerAccountComplete(const AccountResultInfo& resultInfo, const PlayerAccount& playerAccount) override
        {
            Call(FN_OnGetPlayerAccountComplete, resultInfo, playerAccount);
        }

        void OnGetServiceStatusComplete(const BasicResultInfo& resultInfo) override
        {
            Call(FN_OnGetServiceStatusComplete, resultInfo);
        }

        void OnUpdatePlayerAccountComplete(const AccountResultInfo& resultInfo) override
        {
            Call(FN_OnUpdatePlayerAccountComplete, resultInfo);
        }
    };
} // namespace CloudGemPlayerAccount
