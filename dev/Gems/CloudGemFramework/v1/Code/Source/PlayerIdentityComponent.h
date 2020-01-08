/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*/

#pragma once

#include "CloudGemFramework_precompiled.h"

#include <AzCore/Component/Component.h>

#include <AzCore/std/containers/map.h>

#include <CloudCanvas/CloudCanvasIdentityBus.h>
#include <CloudCanvasCommon/CloudCanvasCommonBus.h>

// The AWS Native SDK AWSAllocator triggers a warning due to accessing members of std::allocator directly.
// AWSAllocator.h(70): warning C4996: 'std::allocator<T>::pointer': warning STL4010: Various members of std::allocator are deprecated in C++17.
// Use std::allocator_traits instead of accessing these members directly.
// You can define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING or _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS to acknowledge that you have received this warning.

AZ_PUSH_DISABLE_WARNING(4251 4355 4819 4996, "-Wunknown-warning-option")
#include <aws/core/utils/memory/stl/AWSStreamFwd.h>
#include <aws/cognito-identity/CognitoIdentityClient.h>
#include <aws/cognito-idp/CognitoIdentityProviderClient.h>
#include <aws/lambda/LambdaClient.h>
#include <aws/identity-management/auth/PersistentCognitoIdentityProvider.h>
#include <aws/identity-management/auth/CognitoCachingCredentialsProvider.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/core/utils/memory/stl/AWSMap.h>
#include <aws/core/utils/memory/stl/AWSString.h>
AZ_POP_DISABLE_WARNING

///////////////////////////////////////// WARNING /////////////////////////////////////////
// Support for the auth_token cvar has been deprecated and disabled by default.
// Auth tokens should not be stored in console variables to avoid the possibility of them
// being included when console variables are logged or included in bug reports.
// Instead, call EBUS_EVENT(PlayerIdentityBus, Login, providerName, code).
// Uncommenting the following will re-enable the deprecated auth_token cvar.
//
// #define AUTH_TOKEN_CVAR_ENABLED
///////////////////////////////////////////////////////////////////////////////////////////

namespace CloudGemFramework
{
    class CloudCanvasPlayerIdentityComponent
        : public AZ::Component,
        public CloudCanvasPlayerIdentityBus::Handler,
        public CloudCanvasCommon::CloudCanvasCommonNotificationsBus::Handler
    {
    public:
        AZ_COMPONENT(CloudCanvasPlayerIdentityComponent, "{AA90442E-613E-40E3-AF0E-13C6032C06E4}", AZ::Component);

        static const char* SERVICE_NAME;

        CloudCanvasPlayerIdentityComponent() = default;
        virtual ~CloudCanvasPlayerIdentityComponent() = default;

        static void Reflect(AZ::ReflectContext* reflection);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        //////////////////////////////////////////////////////////////////////////
        // CloudCanvasPlayerIdentityBus
        virtual void AddTokenRetrievalStrategy(const char* provider, std::shared_ptr<TokenRetrievalStrategy> strategy) override;
        virtual void RemoveTokenRetrievalStrategy(const char* provider) override;
        virtual bool Login(const char* authProvider, const char* authCode, const char* refreshToken = nullptr, long long tokenExpiration = 0) override;
        virtual void Logout() override;
        virtual bool ApplyConfiguration() override;
        virtual std::shared_ptr<Aws::Auth::AWSCredentialsProvider> GetPlayerCredentialsProvider() override;
        virtual void SetPlayerCredentialsProvider(std::shared_ptr<Aws::Auth::AWSCredentialsProvider>) override;
        virtual bool ResetPlayerIdentity() override;
        virtual AZStd::string GetIdentityId() override;

    protected:
        Aws::Map<Aws::String, std::shared_ptr<TokenRetrievalStrategy> > InitializeTokenRetrievalStrategies(const std::shared_ptr<Aws::Lambda::LambdaClient>& client, const char* lambdaName);
        bool GetRefreshTokenForProvider(AZStd::string& refreshToken, const AZStd::string& provider);
        bool GetServerIdentity();

        // CloudCanvasCommonNotificationBus
        void ApiInitialized() override;

    private:
        CloudCanvasPlayerIdentityComponent(const CloudCanvasPlayerIdentityComponent&) = delete;

        bool BeginResetIdentity();
        void EndResetIdentity();

        // Http Access Checks
        bool CheckRegionHttpAccess(const AZStd::string& identityPoolId) const;
        int CheckAwsEndpointResponse(const AZStd::string& regionStr) const;

        std::shared_ptr<Aws::CognitoIdentity::CognitoIdentityClient> m_cognitoIdentityClientAnonymous;
        std::shared_ptr<Aws::CognitoIdentity::CognitoIdentityClient> m_cognitoIdentityClientAuthenticated;

        std::shared_ptr<Aws::Auth::CognitoCachingCredentialsProvider> m_anonCredsProvider;
        std::shared_ptr<Aws::Auth::CognitoCachingCredentialsProvider> m_authCredsProvider;
        std::shared_ptr<Aws::Auth::PersistentCognitoIdentityProvider> m_authIdentityProvider;

        AZStd::map<Aws::String, std::shared_ptr<TokenRetrievalStrategy> > m_additionalStrategyMappings;

        std::shared_ptr<Aws::Auth::AWSCredentialsProvider> m_credsProvider;

        AZStd::atomic<bool> m_resettingIdentity{ false };
        AZStd::mutex m_credentialsMutex;
    };
}

