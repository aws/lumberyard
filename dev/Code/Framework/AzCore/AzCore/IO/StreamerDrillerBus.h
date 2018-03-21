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
#ifndef AZCORE_STREAMER_DRILL_H
#define AZCORE_STREAMER_DRILL_H

#include <AzCore/base.h>
#include <AzCore/Driller/DrillerBus.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/IO/StreamerRequest.h>

namespace AZ
{
    namespace IO
    {
        class Streamer;
        class Device;
        class GenericStream;
        class CompressorStream;
        enum class OpenMode : u32;

        /**
         * StreamerDrillerInterface driller lister interface, that records event from the streamer system.
         * To issues commands to the streamer system use \ref StreamerDrillerCommandInterface.
         */
        class StreamerDrillerInterface
            : public AZ::Debug::DrillerEBusTraits
        {
        public:
            virtual ~StreamerDrillerInterface() {}

            virtual void OnDeviceMounted(Device* device) = 0;
            virtual void OnDeviceUnmounted(Device* device) = 0;

            virtual void OnRegisterStream(GenericStream* stream, OpenMode flags) = 0;
            virtual void OnUnregisterStream(GenericStream* stream) = 0;

            virtual void OnReadCacheHit(GenericStream* stream, AZ::u64 offset, AZ::u64 size, const char* debugName) = 0;

            virtual void OnAddRequest(Request* request) = 0;
            virtual void OnCompleteRequest(Request* request, Request::StateType state) = 0;

            virtual void OnCancelRequest(Request* request)  = 0;
            virtual void OnRescheduleRequest(Request* request, AZStd::chrono::system_clock::time_point newDeadline, Request::PriorityType newPriority) = 0;

            // Lower level events if the stream is NOT compressed this call will represent
            // the physical access. If the stream is compressed monitor OnCompressorXXX events.
            virtual void OnRead(GenericStream* stream, AZ::u64 byteSize, AZ::u64 byteOffset) = 0;
            virtual void OnReadComplete(GenericStream* stream, AZ::u64 bytesTransferred) = 0;
            virtual void OnWrite(GenericStream* stream, AZ::u64 byteSize, AZ::u64 byteOffset) = 0;
            virtual void OnWriteComplete(GenericStream* stream, AZ::u64 bytesTransferred) = 0;

            // Compressor functions - called from within the lower level events (OnRead,OnWrite)
            // to perform the actual physical access.
            virtual void OnCompressorRead(CompressorStream* stream, AZ::u64 byteSize, AZ::u64 byteOffset) = 0;
            virtual void OnCompressorReadComplete(CompressorStream* stream, AZ::u64 bytesTransferred) = 0;
            virtual void OnCompressorWrite(CompressorStream* stream, AZ::u64 byteSize, AZ::u64 byteOffset) = 0;
            virtual void OnCompressorWriteComplete(CompressorStream* stream, AZ::u64 bytesTransferred) = 0;
        };

        typedef AZ::EBus<StreamerDrillerInterface> StreamerDrillerBus;
    }
}

#endif // AZCORE_STREAMER_DRILL_H
#pragma once
