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
#ifndef AZCORE_STREAMER_DRILLER_H
#define AZCORE_STREAMER_DRILLER_H 1

#include <AzCore/Driller/Driller.h>
#include <AzCore/IO/StreamerDrillerBus.h>

namespace AZ
{
    namespace Debug
    {
        /**
            * Trace messages driller class
            */
        class StreamerDriller
            : public Driller
            , public IO::StreamerDrillerBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(StreamerDriller, OSAllocator, 0)

            StreamerDriller(bool isTrackStreams = false);
            ~StreamerDriller();

        protected:
            //////////////////////////////////////////////////////////////////////////
            // Driller
            virtual const char*  GroupName() const          { return "SystemDrillers"; }
            virtual const char*  GetName() const            { return "StreamerDriller"; }
            virtual const char*  GetDescription() const     { return "Driller all IO operations using the streamer."; }
            virtual void         Start(const Param* params = NULL, int numParams = 0);
            virtual void         Stop();
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // StreamerDrillerBus
            virtual void OnDeviceMounted(IO::Device* device);
            virtual void OnDeviceUnmounted(IO::Device* device);

            virtual void OnRegisterStream(IO::GenericStream* stream, IO::OpenMode flags);
            virtual void OnUnregisterStream(IO::GenericStream* stream);

            virtual void OnReadCacheHit(IO::GenericStream* stream, AZ::u64 offset, AZ::u64 size, const char* debugName);

            virtual void OnAddRequest(const AZStd::shared_ptr<IO::Request>& request);
            virtual void OnCompleteRequest(const AZStd::shared_ptr<IO::Request>& request, IO::Request::StateType state);

            virtual void OnCancelRequest(const AZStd::shared_ptr<IO::Request>& request);
            virtual void OnRescheduleRequest(const AZStd::shared_ptr<IO::Request>& request, AZStd::chrono::system_clock::time_point newDeadline, IO::Request::PriorityType newPriority);

            // Lower level events
            virtual void OnRead(IO::GenericStream* stream, AZ::u64 byteSize, AZ::u64 byteOffset);
            virtual void OnReadComplete(IO::GenericStream* stream, AZ::u64 bytesTransferred);
            virtual void OnWrite(IO::GenericStream* stream, AZ::u64 byteSize, AZ::u64 byteOffset);
            virtual void OnWriteComplete(IO::GenericStream* stream, AZ::u64 bytesTransferred);

            // Compressor functions
            virtual void OnCompressorRead(IO::CompressorStream* stream, AZ::u64 byteSize, AZ::u64 byteOffset);
            virtual void OnCompressorReadComplete(IO::CompressorStream* stream, AZ::u64 bytesTransferred);
            virtual void OnCompressorWrite(IO::CompressorStream* stream, AZ::u64 byteSize, AZ::u64 byteOffset);
            virtual void OnCompressorWriteComplete(IO::CompressorStream* stream, AZ::u64 bytesTransferred);
            //////////////////////////////////////////////////////////////////////////

            bool    m_isTrackStreams;   ///< True if we want to driller to always (since created) to track active streams (default: false)
            vector<IO::GenericStream*>::type m_trackedStreams; ///< List of tracked streams, enabled only if m_trackStreams is true.
        };
    } // namespace Debug
} // namespace AZ

#endif // AZCORE_STREAMER_DRILLER_H
#pragma once