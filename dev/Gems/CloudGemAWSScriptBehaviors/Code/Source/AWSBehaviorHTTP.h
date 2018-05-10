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

#include "AWSBehaviorBase.h"
#include "AWSBehaviorMap.h"

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/containers/unordered_map.h>

namespace CloudGemAWSScriptBehaviors
{
    class AWSBehaviorHTTPNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual void OnSuccess(const AZStd::string& resultBody) = 0;
        virtual void OnError(const AZStd::string& errorBody) = 0;
        virtual void GetResponse(const int& responseCode, const StringMap& headerMap, const AZStd::string& contentType, const AZStd::string& responseBody) = 0;
    };

    using AWSBehaviorHTTPNotificationsBus = AZ::EBus<AWSBehaviorHTTPNotifications>;

    class AWSBehaviorHTTPNotificationsBusHandler
        : public AWSBehaviorHTTPNotificationsBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(AWSBehaviorHTTPNotificationsBusHandler
            , "{72D247E0-516F-4564-BE94-068F49C48E9F}"
            , AZ::SystemAllocator
            , OnSuccess
            , OnError
            , GetResponse
        );

        void OnSuccess(const AZStd::string& resultBody) override
        {
            Call(FN_OnSuccess, resultBody);
        }

        void OnError(const AZStd::string& errorBody) override
        {
            Call(FN_OnError, errorBody);
        }

        void GetResponse(const int& responseCode, const StringMap& headerMap, const AZStd::string& contentType, const AZStd::string& responseBody) override
        {
            Call(FN_GetResponse, responseCode, headerMap, contentType, responseBody);
        }
    };


    class AWSBehaviorHTTP : public AWSBehaviorBase
    {
    public:
        AWSBEHAVIOR_DEFINITION(AWSBehaviorHTTP, "{51A68723-5FE4-4E97-BB90-38AF89EC228C}")

    private:
        AZStd::string m_url;

        void Get();
        AZ::Job* CreateHttpGetJob(const AZStd::string& url) const;

    };
}