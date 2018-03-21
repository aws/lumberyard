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

#include <LyzardSDK/Base.h>
#include <AzCore/EBus/EBus.h>

#include <ThirdPartySDKs/ThirdPartySDKsAPI.h>
#include <Packages/PackagesAPI.h>

namespace Setup
{
    /**
    * Bus for requesting SDK Installation operations and notifications.
    */
    class SDKInstallationRequest
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = ThirdPartySDKs::SDKHandle;
        //////////////////////////////////////////////////////////////////////////

        virtual Lyzard::StringOutcome RegisterSDKDownload(const AZStd::set<AZStd::string>& enabledTags) = 0;

        // Async function -listen to SDKInstallationNotifications for notifications.
        virtual Lyzard::StringOutcome DownloadSDKAsync() = 0;
        // Blocking version. Returns on error or finished.
        virtual Lyzard::StringOutcome DownloadSDK() = 0;
    };
    using SDKInstallationRequestBus = AZ::EBus<SDKInstallationRequest>;

    /**
     * Bus for requesting Lumberyard Setup operations.
     */
    class SetupRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
                
        virtual AZ::Outcome<ThirdPartySDKs::SDKHandle, AZStd::string> RegisterSDKInstallation(
            const AZStd::string& sdkIdentifier,
            const AZStd::string& thirdPartyPath) = 0;

        virtual AZ::Outcome<AZStd::vector<AZStd::string>, AZStd::string> RegisterAllSDKInstallations(
            const AZStd::string& thirdPartyPath,
            bool shouldRequireOptionalSDKs,
            const AZStd::set<AZStd::string>& enabledTags) = 0;
    };
    using SetupRequestBus = AZ::EBus<SetupRequests>;

    /**
    * Notifications for Setup
    */
    class SDKInstallationNotification
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = ThirdPartySDKs::SDKHandle;
        //////////////////////////////////////////////////////////////////////////

        virtual ~SDKInstallationNotification() = default;

        virtual void OnSDKDownloadDone(bool isSuccess) { };
        virtual void OnSDKStateChanged(const Packages::PackageStates& sdkState) { };
        virtual void OnSDKSetupComplete() { };
        virtual void OnDownloadProgressUpdated(float downloadProgress) { };
    };
    using SDKInstallationNotificationBus = AZ::EBus<SDKInstallationNotification>;
}
