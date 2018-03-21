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

#include <CloudGemAWSScriptBehaviors_precompiled.h>
#include <AzCore/EBus/EBus.h>

namespace CloudGemAWSScriptBehaviors
{
    class AWSURLBehaviorNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual void OnSuccess(const AZStd::string& resultBody) = 0;
        virtual void OnError(const AZStd::string& errorBody) = 0;
    };


    using AWSBehaviorURLNotificationsBus = AZ::EBus<AWSURLBehaviorNotifications>;

    class AWSBehaviorURLNotificationsBusHandler
        : public AWSBehaviorURLNotificationsBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(AWSBehaviorURLNotificationsBusHandler
            , "{76736901-7879-4206-B086-D2C9CF2DC57B}"
            , AZ::SystemAllocator
            , OnSuccess
            , OnError
        );

        void OnSuccess(const AZStd::string& resultBody) override
        {
            Call(FN_OnSuccess, resultBody);
        }

        void OnError(const AZStd::string& errorBody) override
        {
            Call(FN_OnError, errorBody);
        }
    };


    class AWSBehaviorURL : public AWSBehaviorBase
    {
    public:
        AWSBEHAVIOR_DEFINITION(AWSBehaviorURL, "{A8721A8F-8630-46C2-B004-5E765D362F14}")

    private:
        AZStd::string m_inputURL;

        void Decode();
    };
}
