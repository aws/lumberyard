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
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    /**
     * Bus for acquiring information about a given asset type, usually serviced by the relevant asset handler.
     * Extensions, load parameters, custom stream settings, etc.
     */
    class AssetTypeInfo
        : public AZ::EBusTraits
    {
    public:

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AZ::Data::AssetType BusIdType;
        typedef AZStd::recursive_mutex MutexType;
        //////////////////////////////////////////////////////////////////////////

        virtual AZ::Data::AssetType GetAssetType() const = 0;

        //! Retrieve the friendly name for the asset type.
        virtual const char* GetAssetTypeDisplayName() const { return "Unknown"; }

        virtual const char* GetGroup() const { return "Other"; }

        virtual const char* GetBrowserIcon() const { return ""; }

        virtual AZ::Uuid GetComponentTypeId() const { return AZ::Uuid::CreateNull(); }

        //! Retrieve file extensions for the asset type.
        virtual void GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions) { (void)extensions; }
    };

    using AssetTypeInfoBus = AZ::EBus<AssetTypeInfo>;
} // namespace AZ

