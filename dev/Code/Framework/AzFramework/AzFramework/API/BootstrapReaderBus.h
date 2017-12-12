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

#include <AzCore/std/string/string.h>

namespace AzFramework
{
    class BootstrapReaderRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        virtual ~BootstrapReaderRequests() = default;

        /**
         * Search configuration file for key. If found, value is set, and true is returned.
         * Otherwise value is left alone, and false is returned.
         *
         * \param key           The key to search for
         * \param checkPlatform Check for platform_key as well as key
         * \param value [out]   The value found
         *
         * \returns whether or not the value was found.
         */
        virtual bool SearchConfigurationForKey(const AZStd::string& key, bool checkPlatform, AZStd::string& value) = 0;
    };
    using BootstrapReaderRequestBus = AZ::EBus<BootstrapReaderRequests>;
} // namespace AzFramework
