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

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/string/string.h>

namespace Aws
{
    namespace Auth 
    {
        class AWSCredentialsProvider;
    }
}

namespace CloudGemFramework
{
    class TokenRetrievalStrategy;

    class CloudCanvasPlayerIdentityRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        /// Returns an AWS credentials provider that uses the players AWS 
        /// credentials, as retrieved via the player Cognito identity pool.
        virtual std::shared_ptr<Aws::Auth::AWSCredentialsProvider> GetPlayerCredentialsProvider() = 0;
        /// Set the Player Credentials Provider as described above
        virtual void SetPlayerCredentialsProvider(std::shared_ptr<Aws::Auth::AWSCredentialsProvider> credsProvider) = 0;

        virtual void AddTokenRetrievalStrategy(const char* provider, std::shared_ptr<TokenRetrievalStrategy> strategy) = 0;
        virtual void RemoveTokenRetrievalStrategy(const char* provider) = 0;
        virtual bool Login(const char* authProvider, const char* authCode, const char* refreshToken = nullptr, long long tokenExpiration = 0) = 0;
        virtual void Logout() = 0;
        virtual bool ApplyConfiguration() = 0;
        virtual bool GetRefreshTokenForProvider(AZStd::string& refreshToken, const AZStd::string& provider) = 0;
        virtual bool ResetPlayerIdentity() = 0;
        virtual AZStd::string GetIdentityId() = 0;
    };
    using CloudCanvasPlayerIdentityBus = AZ::EBus<CloudCanvasPlayerIdentityRequests>;

    class CloudCanvasPlayerIdentityNotifications
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual void OnBeforeIdentityUpdate() {};
        virtual void OnAfterIdentityUpdate() {};
        virtual void OnIdentityReceived() {};
    };
    using CloudCanvasPlayerIdentityNotificationBus = AZ::EBus<CloudCanvasPlayerIdentityNotifications>;

    class CloudCanvasPlayerIdentityNotificationsHandler : public CloudCanvasPlayerIdentityNotificationBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(CloudCanvasPlayerIdentityNotificationsHandler, "{E07CB74A-5FBE-4319-96F0-E89ADBAFCF01}", AZ::SystemAllocator,
            OnBeforeIdentityUpdate,
            OnAfterIdentityUpdate);

        void OnBeforeIdentityUpdate() override
        {
            Call(FN_OnBeforeIdentityUpdate);
        }

        void OnAfterIdentityUpdate() override
        {
            Call(FN_OnAfterIdentityUpdate);
        }
    };
} // namespace CloudCanvasCommon
