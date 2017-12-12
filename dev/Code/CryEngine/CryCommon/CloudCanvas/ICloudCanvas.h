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

#include <functional>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>

#include <AzCore/EBus/EBus.h>

namespace CloudCanvas
{
    enum RequestRootCAFileResult
    {
        ROOTCA_USER_FILE_RESOLVE_FAILED = -3,
        ROOTCA_FILE_COPY_FAILED = -2,
        ROOTCA_PLATFORM_NOT_SUPPORTED = -1,
        ROOTCA_USER_FILE_NOT_FOUND = 0,
        ROOTCA_FOUND_FILE_SUCCESS = 1,
    };
    // AWS API initialization is now the responsibility of the CloudCanvasCommon
    // Gem. 
    class AwsApiInitRequests
        : public AZ::EBusTraits
    {
    public:

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        virtual bool IsAwsApiInitialized() = 0;

        virtual RequestRootCAFileResult LmbrRequestRootCAFile(AZStd::string& resultStr) = 0;
    };
    using AwsApiInitRequestBus = AZ::EBus<AwsApiInitRequests>;

} // namespace LmbrAWS



