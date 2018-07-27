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
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/StreamerRequest.h>

namespace AZStd
{
    class semaphore;
}

namespace AZ
{
    
    namespace IO
    {
        class Request;
        class VirtualStream;

        class DeviceRequest : public EBusTraits
        {
        public:
            static const bool EnableEventQueue = true;
            static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::Single;
            using MutexType = AZStd::recursive_mutex;
            using EventQueueMutexType = ::AZStd::recursive_mutex;

            virtual ~DeviceRequest() = default;

            virtual void RegisterFileStreamRequest(Request* request, const char* filename, OpenMode flags, bool isLimited = false, AZStd::semaphore* sync = nullptr) = 0;
            virtual void UnRegisterFileStreamRequest(Request* request, const char* filename, AZStd::semaphore* sync = nullptr) = 0;
            virtual void RegisterStreamRequest(Request* request, VirtualStream* stream, OpenMode flags = OpenMode::Invalid, bool isLimited = false, AZStd::semaphore* sync = nullptr) = 0;
            virtual void UnRegisterStreamRequest(Request* request, VirtualStream* stream, AZStd::semaphore* sync = nullptr) = 0;
            virtual void ReadRequest(Request* request, VirtualStream* stream, const char* filename, AZStd::semaphore* sync = nullptr) = 0;
            virtual void WriteRequest(Request* request, VirtualStream* stream, const char* filenName, AZStd::semaphore* sync = nullptr) = 0;
            virtual void CancelRequest(Request* request, ::AZStd::semaphore* sync = nullptr) = 0;
        };

        using DeviceRequestBus = ::AZ::EBus<DeviceRequest>;
    }
}
