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

#include <CloudCanvas/ICloudCanvas.h>

namespace CloudCanvasCommon
{

    class CloudCanvasCommonRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        // Some platforms (Android) need to point the http client at the certificate bundle to avoid SSL errors
        virtual CloudCanvas::RequestRootCAFileResult RequestRootCAFile(AZStd::string& resultPath) = 0;
        virtual int GetEndpointHttpResponseCode(const AZStd::string& endPoint) { return 0; }
        // Called by RequestRootCAFile - Skips platform checks
        virtual CloudCanvas::RequestRootCAFileResult GetUserRootCAFile(AZStd::string& resultPath) = 0;
    };
    using CloudCanvasCommonRequestBus = AZ::EBus<CloudCanvasCommonRequests>;

    // Bus used to send notifications about CloudCanvasCommon initialization
    class CloudCanvasCommonNotifications
        : public AZ::EBusTraits
    {
    public:

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        virtual void RootCAFileSet(const AZStd::string& caPath) {}

        // Called after we've initialized the NativeSDK - this does not guarantee that we're yet ready
        // to make Http calls on all platforms - OnPostInitialization is best for that
        virtual void ApiInitialized() {}

        // We delay some actions (Copying RootCA file) until after CrySystemInitialization currently because things
        // like logging and the file system aren't fully set up yet.  
        virtual void OnPostInitialization() {}

        virtual void BeforeShutdown() {}
        virtual void ShutdownComplete() {}
    };
    using CloudCanvasCommonNotificationsBus = AZ::EBus<CloudCanvasCommonNotifications>;
} // namespace CloudCanvasCommon
