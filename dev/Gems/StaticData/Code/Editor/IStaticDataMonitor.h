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

namespace CloudCanvas
{
    namespace StaticData
    {
        class IStaticDataMonitor
        {
        public:
            virtual ~IStaticDataMonitor() {}

            virtual void RemoveAll() {}

            virtual void AddPath(const AZStd::string& sanitizedPath, bool isFile) {}
            virtual void RemovePath(const AZStd::string& sanitizedPath) {}

            virtual void AddAsset(const AZ::Data::AssetId& assetId) {}
            virtual void RemoveAsset(const AZ::Data::AssetId& assetId) {}

            virtual AZStd::string GetSanitizedName(const char* pathName) const { return{}; }
        };

        class StaticDataMonitorRequestBusTraits
            : public AZ::EBusTraits
        {
        public:
            //////////////////////////////////////////////////////////////////////////
            // EBusTraits overrides
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
            //////////////////////////////////////////////////////////////////////////
        };

        using StaticDataMonitorRequestBus = AZ::EBus<IStaticDataMonitor, StaticDataMonitorRequestBusTraits>;
    }
}