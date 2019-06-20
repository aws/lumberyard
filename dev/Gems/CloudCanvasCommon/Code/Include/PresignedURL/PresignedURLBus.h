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
#include <AzCore/Jobs/Job.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Component/EntityId.h>

namespace CloudCanvas
{

    class IPresignedURLRequest
    {
    public:
        virtual void RequestDownloadSignedURL(const AZStd::string& signedURL, const AZStd::string& fileName, AZ::EntityId id) = 0;
        virtual AZ::Job* RequestDownloadSignedURLJob(const AZStd::string& signedURL, const AZStd::string& fileName, AZ::EntityId id) = 0;
    };

    class PresignedURLRequestBusTraits
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        using MutexType = AZStd::recursive_mutex;
        //////////////////////////////////////////////////////////////////////////
    };

    using PresignedURLRequestBus = AZ::EBus<IPresignedURLRequest, PresignedURLRequestBusTraits>;

    class PresignedURLResult
    {
    public:
        virtual void GotPresignedURLResult(const AZStd::string& requestURL, int responseCode, const AZStd::string& responseMessage, const AZStd::string& outputFile) = 0;
    };

    class PresignedURLResultBusTraits
    : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AZ::EntityId BusIdType;
        //////////////////////////////////////////////////////////////////////////
    };

    using PresignedURLResultBus = AZ::EBus<PresignedURLResult, PresignedURLResultBusTraits>;
}
