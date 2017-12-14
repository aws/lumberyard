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
#include <AzCore/std/containers/bitset.h>
#include <LyzardSDK/Base.h>
#include <AzCore/std/parallel/mutex.h>

namespace Packages
{
    using PackageId = AZStd::size_t;
    using PackageGroupId = AZStd::size_t;

    enum class PackageState
    {
        Incomplete,
        AcquiringManifest,
        AcquiringFilelist,
        AcquiringPackage,
        Unpacking,
        Moving,
        Validating,
        Complete,

        PackageStateTotal
    };
    using PackageStates = AZStd::bitset<static_cast<size_t>(PackageState::PackageStateTotal)>;


    static AZStd::string PackageStateToString(const PackageState& state)
    {
        switch (state)
        {
        case PackageState::Incomplete:
            return "Incomplete";
        case PackageState::AcquiringManifest:
            return "Acquiring Manifest";
        case PackageState::AcquiringFilelist:
            return "Acquiring Filelist";
        case PackageState::AcquiringPackage:
            return "Acquiring Package";
        case PackageState::Unpacking:
            return "Unpacking";
        case PackageState::Moving:
            return "Moving";
        case PackageState::Validating:
            return "Validating";
        case PackageState::Complete:
            return "Complete";
        default:
            AZ_Assert(false, "Invalid PackageState");
            return "Invalid PackageState";
        }
    }

    static AZStd::string GetPackageStatusString(PackageStates states)
    {
        states[static_cast<size_t>(PackageState::Incomplete)] = 0;
        states[static_cast<size_t>(PackageState::Complete)] = 0;

        AZStd::string stateString;
        AZStd::string separator(" | ");
        auto statesSize = states.size();
        for (unsigned int i = 0; i < statesSize; ++i)
        {
            if (states.test(i))
            {
                AZStd::string status = PackageStateToString(static_cast<PackageState>(i));
                stateString += (!stateString.empty() ? separator + status : status);
            }
        }

        return stateString;
    }

    /**
     * Bus for requesting Packages operations.
     */
    class PackagesRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        virtual Lyzard::StringOutcome InitializeController() = 0;

        virtual AZ::Outcome<PackageId, AZStd::string> RegisterPackage(
            const AZStd::string& name, 
            const AZStd::string& version, 
            const AZStd::string& platform,
            const AZStd::string& uri, 
            const AZStd::string& destination) = 0;

        virtual Lyzard::StringOutcome DownloadPackageAsync(const PackageId& identifier) = 0;

        virtual AZ::Outcome<PackageGroupId, AZStd::string> CreatePackageGroup(const AZStd::vector<PackageId>& packageIds) = 0;
        virtual Lyzard::StringOutcome AddPackageToGroup(const PackageGroupId& packageGroupId, const PackageId& packageId) = 0;
        virtual Lyzard::StringOutcome DownloadPackageGroupAsync(const PackageGroupId& identifier) = 0;
    };
    using PackagesRequestBus = AZ::EBus<PackagesRequests>;

    /**
    * Bus for notifications of Packages operations.
    */
    class PackagesNotification
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = PackageId;
        using MutexType = AZStd::mutex;
        //////////////////////////////////////////////////////////////////////////
        
        virtual ~PackagesNotification() = default;

        virtual void OnPackageInfoReady() { };
        virtual void OnPackageDownloadFinished(bool success) { };
        virtual void OnPackageStateChanged(const PackageStates& oldStates, const PackageStates& newStates) { };
        virtual void OnPackageProgressUpdated(float ratio) { };
    };
    using PackagesNotificationBus = AZ::EBus<PackagesNotification>;

    /**
    * Bus for notifications of PackageGroup operations.
    */
    class PackageGroupNotification
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = PackageGroupId;
        //////////////////////////////////////////////////////////////////////////

        virtual ~PackageGroupNotification() = default;

        virtual void OnGroupDownloadDone(bool isSuccess) { };
        virtual void OnGroupStateChanged(const Packages::PackageStates& sdkState) { };
        virtual void OnGroupProgressUpdated(const float progress) { };
    };
    using PackageGroupNotificationBus = AZ::EBus<PackageGroupNotification>;
}
