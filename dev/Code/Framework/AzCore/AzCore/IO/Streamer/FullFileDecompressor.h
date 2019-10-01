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

#include <AzCore/IO/Streamer/Statistics.h>
#include <AzCore/IO/Streamer/StreamerConfiguration.h>
#include <AzCore/IO/Streamer/StreamStackEntry.h>
#include <AzCore/Jobs/JobManager.h>
#include <AzCore/Jobs/JobContext.h>
#include <AzCore/std/chrono/clocks.h>
#include <AzCore/std/containers/deque.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ
{
    namespace IO
    {
        //! Entry in the streaming stack that decompresses files from an archive that are stored
        //! as single files and without equally distributed seek points.
        //! Because the target archive has compressed the entire file, it needs to be decompressed
        //! completely, so even if the file is partially read, it needs to be fully loaded. This
        //! also means that there's no upper limit to the memory so every decompression job will
        //! need to allocate memory as a temporary buffer (in-place decompression is not supported).
        //! Finally, the lack of an upper limit also means that the duration of the decompression job
        //! can vary largely so a dedicated job system is used to decompress on to avoid blocking
        //! the main job system from working.
        class FullFileDecompressor
            : public StreamStackEntry
        {
        public:
            FullFileDecompressor(u32 maxNumReads, u32 maxNumJobs);
            ~FullFileDecompressor() override = default;

            void PrepareRequest(FileRequest* request) override;
            bool ExecuteRequests() override;
            void FinalizeRequest(FileRequest* request) override;
            s32 GetAvailableRequestSlots() const override;
            void UpdateCompletionEstimates(AZStd::chrono::system_clock::time_point now, AZStd::vector<FileRequest*>& internalPending,
                AZStd::deque<AZStd::shared_ptr<Request>>& externalPending) override;

            void CollectStatistics(AZStd::vector<Statistic>& statistics) const override;

            u32 GetNumRunningJobs() const;

        private:
            using Buffer = AZStd::unique_ptr<u8[]>;

            enum class ReadBufferStatus : uint8_t
            {
                Unused,
                ReadInFlight,
                PendingDecompression
            };

            struct DecompressionInformation
            {
                bool IsProcessing() const;

                AZStd::chrono::high_resolution_clock::time_point m_queueStartTime;
                AZStd::chrono::high_resolution_clock::time_point m_jobStartTime;
                Buffer m_compressedData;
                FileRequest* m_waitRequest{ nullptr };
            };
            
            void StartReadFile(FileRequest* compressedReadRequest);
            void FinishReadFile(FileRequest* readRequest);
            bool StartDecompressions();
            void FinishDecompression(FileRequest* waitRequest);
            
            static void FullDecompression(StreamerContext* context, DecompressionInformation& info);
            static void PartialDecompression(StreamerContext* context, DecompressionInformation& info);

            AZStd::deque<FileRequest*> m_pendingRequests;

            AverageWindow<size_t, double, s_statisticsWindowSize> m_decompressionJobDelayMicroSec;
            AverageWindow<size_t, double, s_statisticsWindowSize> m_decompressionDurationMicroSec;
            AverageWindow<size_t, double, s_statisticsWindowSize> m_bytesDecompressed;
#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
            AverageWindow<u8, double, s_statisticsWindowSize> m_decompressionBound;
            AverageWindow<u8, double, s_statisticsWindowSize> m_readBound;
#endif
            
            AZStd::unique_ptr<Buffer[]> m_readBuffers;
            // Nullptr if not reading, the read request if reading the file and the wait request for decompression when waiting on decompression.
            AZStd::unique_ptr<FileRequest*[]> m_readRequests;
            AZStd::unique_ptr<ReadBufferStatus[]> m_readBufferStatus;
            
            AZStd::unique_ptr<DecompressionInformation[]> m_processingJobs;
            AZStd::unique_ptr<JobManager> m_decompressionJobManager;
            AZStd::unique_ptr<JobContext> m_decompressionjobContext;

            size_t m_memoryUsage{ 0 }; //!< Amount of memory used for buffers by the decompressor.
            u32 m_maxNumReads{ 2 };
            u32 m_numInFlightReads{ 0 };
            u32 m_maxNumJobs{ 1 };
            u32 m_numRunningJobs{ 0 };
        };
    } // namespace IO
} // namespace AZ
