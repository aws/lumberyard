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
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/map.h>

namespace ThirdPartySDKs
{
    using SDKHandle = AZStd::size_t;

    enum class SDKAvailableState
    {
        /*
         * An SDK is considered available if all of its valid
         * example files exist on disk.
         */
        Available,
        Partially,
        NotAvailable
    };

    class SDKRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = ThirdPartySDKs::SDKHandle;
        //////////////////////////////////////////////////////////////////////////

        virtual AZStd::string GetIdentifier() const = 0;
        virtual AZStd::string GetDisplayName() const = 0;
        virtual AZStd::string GetVersion() const = 0;
        virtual AZStd::string GetSourcePath() const = 0;
        virtual AZStd::string GetDescription() const = 0;
        virtual AZStd::string GetDetailedInstruction() const = 0;
        virtual bool DoTagsMatch(const AZStd::set<AZStd::string>& tags) const = 0;
        virtual bool IsOptional() const = 0;

        virtual AZStd::set<AZStd::string> GetDependencies() const = 0;

        virtual AZStd::set<AZStd::string> GetAvailablePlatforms() const = 0;

        virtual SDKAvailableState GetAvailableState(
            const AZStd::string& thirdPartyPath,
            const bool shouldOnlyCheckTracked) const = 0;
        
        virtual SDKAvailableState GetTagBasedDiskState(
            const AZStd::string& thirdPartyPath,
            const AZStd::set<AZStd::string>& enabledTags) const = 0;

        virtual AZStd::set<AZStd::string> GetFilteredPlatforms(
            const AZStd::set<AZStd::string>& tags) const = 0;

        virtual void SetupSymlinks(
            const AZStd::set<AZStd::string>& tags,
            const AZStd::string& thirdPartyPath) = 0;

    };
    using SDKRequestBus = AZ::EBus<SDKRequests>;

    /**
     * Bus for requesting Third Party SDKs operations.
     */
    class SDKsControllerRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        virtual void LoadThirdPartySDKs(const AZStd::string& thirdPartyPath) = 0;

        virtual AZStd::map<SDKHandle, SDKAvailableState> GetAllSDKsAvailableState() const = 0;
        virtual bool DoesSDKExist(const AZStd::string& sdkIdentifier) const = 0;
        virtual SDKHandle GetSDKHandle(const AZStd::string& sdkIdentifier) const = 0;
    };

    using SDKsControllerRequestBus = AZ::EBus<SDKsControllerRequests>;
}
