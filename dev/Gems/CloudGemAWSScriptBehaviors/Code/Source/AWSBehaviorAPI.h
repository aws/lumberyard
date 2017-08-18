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
#include "AWSBehaviorJSON.h"

#include <AzCore/Ebus/Ebus.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/core/auth/AWSAuthSigner.h>
#include <CloudGemFramework/ServiceRequestJobConfig.h>

namespace CloudGemAWSScriptBehaviors
{
    class AWSBehaviorAPINotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual void OnSuccess(const AZStd::string& resultBody) = 0;
        virtual void OnError(const AZStd::string& errorBody) = 0;
        virtual void GetResponse(const int& responseCode, const AZStd::string& JSONresponseData) = 0;
    };

    using AWSBehaviorAPINotificationsBus = AZ::EBus<AWSBehaviorAPINotifications>;

    class AWSBehaviorAPINotificationsBusHandler
        : public AWSBehaviorAPINotificationsBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(AWSBehaviorAPINotificationsBusHandler
            , "{271EC20B-7566-43DA-B9A0-A854E8801FE2}"
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

        void GetResponse(const int& responseCode, const AZStd::string& JSONresponseData) override
        {
            Call(FN_GetResponse, responseCode, JSONresponseData);
        }
    };

    class AWSBehaviorAPI : public AWSBehaviorBase
    {
    public:
        AWSBEHAVIOR_DEFINITION(AWSBehaviorAPI, "{9634F50E-B296-4029-A96C-638958AB62A0}")

    private:
        AZStd::string m_resourceName;
        AZStd::string m_query;
        AZStd::string m_httpMethod;
        AZStd::string m_jsonParam;
        
        // Maps method strings to AWS API method enums
        AZStd::unordered_map<AZStd::string, Aws::Http::HttpMethod> m_httpMethods;

        void Execute();
        AZ::Job* CreateHttpJob(const AZStd::string& url) const;

        Aws::Http::HttpMethod GetHTTPMethodForString(const AZStd::string& methodName) const;
        AZStd::vector<AZStd::string> GetHTTPMethodList() const;
        static AZStd::string ParseRegionFromAPIRequestURL(const AZStd::string& apiRequestURL);
    };
}