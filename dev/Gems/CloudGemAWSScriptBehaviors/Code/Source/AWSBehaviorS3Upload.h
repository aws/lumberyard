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
    
    class AWSS3UploadBehaviorNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual void OnSuccess(const AZStd::string& resultBody) = 0;
        virtual void OnError(const AZStd::string& errorBody) = 0;
    };

    using AWSBehaviorS3UploadNotificationsBus = AZ::EBus<AWSS3UploadBehaviorNotifications>;

    class AWSBehaviorS3UploadNotificationsBusHandler
        : public AWSBehaviorS3UploadNotificationsBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(AWSBehaviorS3UploadNotificationsBusHandler
            , "{3B9FF756-E818-4859-A880-9B826A8C804B}"
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


    class AWSBehaviorS3Upload : public AWSBehaviorBase
    {
    public:
        AWSBEHAVIOR_DEFINITION(AWSBehaviorS3Upload, "{39F44F6F-BD5B-4947-A230-A5F4CA744578}")

    private:
        AZStd::string m_bucketName;
        AZStd::string m_keyName;
        AZStd::string m_localFileName;
        AZStd::string m_contentType;

        void Download();
        void Upload();
        void Presign();
    };
}
#pragma once
