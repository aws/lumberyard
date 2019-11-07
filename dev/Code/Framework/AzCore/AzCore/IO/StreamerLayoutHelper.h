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
#ifndef AZCORE_STREAMER_STATS_H
#define AZCORE_STREAMER_STATS_H

#include <AzCore/IO/Streamer.h>
#include <AzCore/IO/StreamerDrillerBus.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>

namespace AZ
{
    namespace IO
    {
        class StreamerLayoutHelper
            : public StreamerDrillerBus::Handler
        {
            typedef AZStd::unordered_map<StreamID, unsigned int> TranstionTargetMap;
            struct StatsLayoutEntry
            {
                AZStd::string           m_name;         ///< Stored stream name (since the actual stream can be destroyed).
                AZ::u64                 m_bytesRead;    ///< How much data have we read from this stream.
                TranstionTargetMap      m_transitions;  ///< Number of transitions from this stream to others.
                bool                    m_isLayer0;     ///< True if stream belongs to layer 0, false if layer 1
                unsigned int            m_splitFactor;  ///< Split factor the higher the number the more expensive to split at this node.
                unsigned int            m_layoutGroup;  ///< Layout group id. Used to speed up seek group sorting.
                StatsLayoutEntry*       m_layoutPrev;   ///< Previous stream on the layout. Used when we build seek layout.
                StatsLayoutEntry*       m_layoutNext;   ///< Previous stream on the layout. Used when we build seek layout.

                static bool SortOnReadSize(StatsLayoutEntry* left, StatsLayoutEntry* right)  {   return left->m_bytesRead > right->m_bytesRead; }
            };
            typedef AZStd::unordered_map<StreamID, StatsLayoutEntry> StreamLayoutMap;

            struct SeekLink
            {
                unsigned int weight;
                StatsLayoutEntry* stream1;
                StatsLayoutEntry* stream2;
                static bool   Sort(SeekLink* left, SeekLink* right)      { return left->weight > right->weight; }
            };

            Device*         m_targetDevice;
            StreamID        m_currentStreamId;
            StreamLayoutMap m_layoutData;

            void LinkUnattachedStream(StatsLayoutEntry* unattachedStream, StatsLayoutEntry* attachedStream);

        public:
            AZ_CLASS_ALLOCATOR(StreamerLayoutHelper, SystemAllocator, 0);
            /**
             * \param deviceName - example PC - "C:\",X360 - "game:\", PS3 - "/dev_bdvd/" // ACCEPTED_USE
             */
            StreamerLayoutHelper(const char* deviceName);

            //////////////////////////////////////////////////////////////////////////
            // StreamerDrillerBus
            virtual void OnDeviceMounted(AZ::IO::Device* /*device*/)        {}
            virtual void OnDeviceUnmounted(AZ::IO::Device* /*device*/)                              {}
            virtual void OnRegisterStream(AZ::IO::GenericStream* stream, OpenMode flags);
            virtual void OnUnregisterStream(AZ::IO::GenericStream* /*stream*/) {}
            virtual void OnReadCacheHit(AZ::IO::GenericStream* /*stream*/, AZ::u64 /*offset*/, AZ::u64 /*size*/, const char* /*debugName*/) {}
            virtual void OnAddRequest(const AZStd::shared_ptr<Request>& /*request*/) {}
            virtual void OnCompleteRequest(const AZStd::shared_ptr<Request>& /*request*/, AZ::IO::Request::StateType /*state*/) {}
            virtual void OnCancelRequest(const AZStd::shared_ptr<Request>& /*request*/)  {}
            virtual void OnRescheduleRequest(const AZStd::shared_ptr<Request>& /*request*/, AZStd::chrono::system_clock::time_point /*newDeadline*/, Request::PriorityType /*newPriority*/) {}

            virtual void OnRead(AZ::IO::GenericStream* /*stream*/, AZ::u64 /*byteSize*/, AZ::u64 /*byteOffset*/) {}
            virtual void OnReadComplete(AZ::IO::GenericStream* stream, AZ::u64 bytesTransferred);
            virtual void OnWrite(AZ::IO::GenericStream* /*stream*/, AZ::u64 /*byteSize*/, AZ::u64 /*byteOffset*/) {}
            virtual void OnWriteComplete(AZ::IO::GenericStream* /*stream*/, AZ::u64 /*bytesTransferred*/) {}

            // As of now we arrange the layout based on uncompressed reads, which should be the same (as we order files) if they were compressed.
            virtual void OnCompressorRead(AZ::IO::CompressorStream* /*stream*/, AZ::u64 /*byteSize*/, AZ::u64 /*byteOffset*/) {}
            virtual void OnCompressorReadComplete(AZ::IO::CompressorStream* /*stream*/, AZ::u64 /*bytesTransferred*/) {}
            virtual void OnCompressorWrite(AZ::IO::CompressorStream* /*stream*/, AZ::u64 /*byteSize*/, AZ::u64 /*byteOffset*/) {}
            virtual void OnCompressorWriteComplete(AZ::IO::CompressorStream* /*stream*/, AZ::u64 /*bytesTransferred*/) {}
            //////////////////////////////////////////////////////////////////////////

            /**
             * Debug feature - generate an optimal layout map for reading (seek based). For this feature to operate you need to set
             * Descriptor::m_isGenerateOptimizedLayoutMap.
             * \note stream which can write are ignored.
             * \param layout - a list of stream names for to minimize seek times. Based on them you can make the split between layers (if you need to use one)
             * for each entry we provide a pair of data. First is the stream full name (as registered in the system), second value is the split factor. The lower
             * the value the more suitable place to break the connection and place the rest of files on a different location on the disk (for faster load speed) or
             * just a different data layer. Ideally if you split where the split factor is 0 means there will not penalty at all, since we never cross from this files.
             */
            void GenerateOptimalSeekDeviceReadLayout(AZStd::vector<AZStd::pair<AZStd::string, unsigned int> >& layout);
            /**
             * Debug feature - generate an optimal layout map for reading (data transfer based). Streams in the beginning
             * need a faster transfer than the one in the end. In other words the streams is sorted of how much data we read from each stream.
             * \ref GenerateOptimalSeekDeviceReadLayout
             */
            void GenerateOptimalTransferDeviceReadLayout(AZStd::vector<AZStd::string>& layer0, AZStd::vector<AZStd::string>& layer1);
        };
    }
}

#endif // AZCORE_STREAMER_STATS_H
#pragma once
