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
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/string/string.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/StreamerRequest.h>
#include <AzCore/IO/Streamer/FileRange.h>

namespace AZStd
{
    class semaphore;
}

namespace AZ
{
    
    namespace IO
    {
        class Request;
        
        class DeviceRequest : public EBusTraits
        {
        public:
            static const bool EnableEventQueue = true;
            static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::Single;
            using MutexType = AZStd::recursive_mutex; // requests can be sent from any thread
            using EventQueueMutexType = AZStd::recursive_mutex; // requests can also be queued from any thread

            virtual ~DeviceRequest() = default;

            virtual void ReadRequest(AZStd::shared_ptr<Request> request) = 0;
            virtual void CancelRequest(AZStd::shared_ptr<Request> request, ::AZStd::semaphore* sync = nullptr) = 0;
            virtual void CreateDedicatedCacheRequest(RequestPath filename, FileRange range, AZStd::semaphore* sync = nullptr) = 0;
            virtual void DestroyDedicatedCacheRequest(RequestPath filename, FileRange range, AZStd::semaphore* sync = nullptr) = 0;
            virtual void FlushCacheRequest(RequestPath filename, AZStd::semaphore* sync) = 0;
            virtual void FlushCachesRequest(AZStd::semaphore* sync) = 0;
        };

        using DeviceRequestBus = ::AZ::EBus<DeviceRequest>;
    }
}
