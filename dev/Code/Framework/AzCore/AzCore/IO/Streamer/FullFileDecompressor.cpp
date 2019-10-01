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

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/IO/Streamer/FileRequest.h>
#include <AzCore/IO/Streamer/FullFileDecompressor.h>
#include <AzCore/IO/Streamer/StreamerContext.h>
#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Jobs/JobManager.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/std/parallel/thread.h>

namespace AZ
{
    namespace IO
    {
        bool FullFileDecompressor::DecompressionInformation::IsProcessing() const
        {
            return !!m_compressedData;
        }
        
        FullFileDecompressor::FullFileDecompressor(u32 maxNumReads, u32 maxNumJobs)
            : StreamStackEntry("Full file decompressor")
            , m_maxNumReads(maxNumReads)
            , m_maxNumJobs(maxNumJobs)
        {
            JobManagerDesc jobDesc;
            u32 numThreads = AZ::GetMin(maxNumJobs, AZStd::thread::hardware_concurrency());
            for (u32 i = 0; i < numThreads; ++i)
            {
                jobDesc.m_workerThreads.push_back(JobManagerThreadDesc());
            }
            m_decompressionJobManager = AZStd::make_unique<JobManager>(jobDesc);
            m_decompressionjobContext = AZStd::make_unique<JobContext>(*m_decompressionJobManager);

            m_processingJobs = AZStd::make_unique<DecompressionInformation[]>(maxNumJobs);

            m_readBuffers = AZStd::make_unique<Buffer[]>(maxNumReads);
            m_readRequests = AZStd::make_unique<FileRequest*[]>(maxNumReads);
            m_readBufferStatus = AZStd::make_unique<ReadBufferStatus[]>(maxNumReads);
            for (u32 i = 0; i < maxNumReads; ++i)
            {
                m_readBufferStatus[i] = ReadBufferStatus::Unused;
            }
        }

        void FullFileDecompressor::PrepareRequest(FileRequest* request)
        {
            AZ_Assert(request, "PrepareRequest was provided a null request.");

            switch (request->GetOperationType())
            {
            case FileRequest::Operation::CompressedRead:
                m_pendingRequests.push_back(request);
                break;
            default:
                StreamStackEntry::PrepareRequest(request);
                break;
            }
        }

        bool FullFileDecompressor::ExecuteRequests()
        {
            bool result = false;
            // First queue jobs as this might open up new read slots.
            if (m_numInFlightReads > 0 && m_numRunningJobs < m_maxNumJobs)
            {
                result = StartDecompressions();
            }
            // Queue as many new reads as possible.
            while (!m_pendingRequests.empty() && m_numInFlightReads < m_maxNumReads)
            {
                FileRequest* request = m_pendingRequests.front();
                StartReadFile(request);
                m_pendingRequests.pop_front();
                result = true;
            }

#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
            bool allPendingDecompression = true;
            bool allReading = true;
            for (u32 i = 0; i < m_maxNumReads; ++i)
            {
                allPendingDecompression =
                    allPendingDecompression && (m_readBufferStatus[i] == ReadBufferStatus::PendingDecompression);
                allReading =
                    allReading && (m_readBufferStatus[i] == ReadBufferStatus::ReadInFlight);
            }
            m_decompressionBound.PushEntry(allPendingDecompression ? 1 : 0);
            m_readBound.PushEntry(allReading && (m_numRunningJobs < m_maxNumJobs) ? 1 : 0);
#endif

            return StreamStackEntry::ExecuteRequests() || result;
        }

        void FullFileDecompressor::FinalizeRequest(FileRequest* request)
        {
            switch(request->GetOperationType())
            {
            case FileRequest::Operation::Read:
                FinishReadFile(request);
                break;
            case  FileRequest::Operation::Wait:
                FinishDecompression(request);
                break;
            default:
                AZ_Assert(false, "Unexpected file request operation %i asked to be finalized by FullFileDecompressor.", request->GetOperationType());
            }
        }

        s32 FullFileDecompressor::GetAvailableRequestSlots() const
        {
            s32 next = StreamStackEntry::GetAvailableRequestSlots();
            return AZStd::min(next, aznumeric_cast<s32>(m_maxNumReads - m_numInFlightReads));
        }

        void FullFileDecompressor::UpdateCompletionEstimates(AZStd::chrono::system_clock::time_point now, AZStd::vector<FileRequest*>& internalPending,
            AZStd::deque<AZStd::shared_ptr<Request>>& externalPending)
        {
            AZStd::reverse_copy(m_pendingRequests.begin(), m_pendingRequests.end(), AZStd::back_inserter(internalPending));
            StreamStackEntry::UpdateCompletionEstimates(now, internalPending, externalPending);

            double decompressionSpeed = static_cast<double>(m_bytesDecompressed.GetTotal()) / static_cast<double>(m_decompressionDurationMicroSec.GetTotal());
            AZStd::chrono::microseconds cummulativeDelay = AZStd::chrono::microseconds::max();

            for (u32 i = 0; i < m_maxNumJobs; ++i)
            {
                if (m_processingJobs[i].IsProcessing())
                {
                    FileRequest* compressedRequest = m_processingJobs[i].m_waitRequest->GetParent();
                    AZ_Assert(compressedRequest, "A wait request attached to FullFileDecompressor was completed but didn't have a parent compressed request.");
                    FileRequest::CompressedReadData& data = compressedRequest->GetCompressedReadData();

                    size_t bytesToDecompress = data.m_compressionInfo.m_compressedSize;
                    auto totalTimeDecompressionTime = AZStd::chrono::microseconds(static_cast<u64>(bytesToDecompress / decompressionSpeed));
                    auto timeProcessing = now - m_processingJobs[i].m_jobStartTime;
                    auto timeLeft = totalTimeDecompressionTime > timeProcessing ? totalTimeDecompressionTime - timeProcessing : AZStd::chrono::microseconds(0);
                    cummulativeDelay = AZStd::min(timeLeft, cummulativeDelay);
                    m_processingJobs[i].m_waitRequest->SetEstimatedCompletion(now + timeLeft);
                }
            }

            AZStd::chrono::microseconds decompressionDelay = AZStd::chrono::microseconds(static_cast<u64>(m_decompressionJobDelayMicroSec.CalculateAverage()));
            AZStd::chrono::microseconds smallestDecompressionDuration = AZStd::chrono::microseconds::max();
            for (u32 i = 0; i < m_maxNumReads; ++i)
            {
                AZStd::chrono::system_clock::time_point baseTime;
                switch (m_readBufferStatus[i])
                {
                case ReadBufferStatus::Unused:
                    continue;
                case ReadBufferStatus::ReadInFlight:
                    baseTime = m_readRequests[i]->GetEstimatedCompletion();
                    break;
                case ReadBufferStatus::PendingDecompression:
                    baseTime = now;
                    break;
                default:
                    AZ_Assert(false, "Unsupported buffer type: %i.", m_readBufferStatus[i]);
                    continue;
                }
                
                baseTime += cummulativeDelay;
                baseTime += decompressionDelay;

                FileRequest* compressedRequest = m_processingJobs[i].m_waitRequest->GetParent();
                AZ_Assert(compressedRequest, "A wait request attached to FullFileDecompressor was completed but didn't have a parent compressed request.");
                FileRequest::CompressedReadData& data = compressedRequest->GetCompressedReadData();

                size_t bytesToDecompress = data.m_compressionInfo.m_compressedSize;
                auto totalDecompressionTime = AZStd::chrono::microseconds(static_cast<u64>(bytesToDecompress / decompressionSpeed));
                smallestDecompressionDuration = AZStd::min(smallestDecompressionDuration, totalDecompressionTime);
                baseTime += totalDecompressionTime;
                m_readRequests[i]->SetEstimatedCompletion(baseTime);
            }
            cummulativeDelay += smallestDecompressionDuration;

            for (FileRequest* pending : internalPending)
            {
                if (pending->GetOperationType() == FileRequest::Operation::CompressedRead)
                {
                    FileRequest::CompressedReadData& data = pending->GetCompressedReadData();

                    AZStd::chrono::microseconds processingTime = decompressionDelay;
                    size_t bytesToDecompress = data.m_compressionInfo.m_compressedSize;
                    processingTime += AZStd::chrono::microseconds(static_cast<u64>(bytesToDecompress / decompressionSpeed));

                    cummulativeDelay += processingTime;
                    pending->SetEstimatedCompletion(pending->GetEstimatedCompletion() + processingTime);
                }
            }

            for (AZStd::shared_ptr<Request>& request : externalPending)
            {
                if (request->NeedsDecompression())
                {
                    size_t bytesToDecompress = request->m_compressionInfo.m_compressedSize;
                    request->m_estimatedCompletion += decompressionDelay;
                    request->m_estimatedCompletion += cummulativeDelay;
                    request->m_estimatedCompletion += AZStd::chrono::microseconds(static_cast<u64>(bytesToDecompress / decompressionSpeed));
                }
            }
        }

        void FullFileDecompressor::CollectStatistics(AZStd::vector<Statistic>& statistics) const
        {
            constexpr double bytesToMB = 1.0 / (1024.0 * 1024.0);
            constexpr double usToSec = 1.0 / (1000.0 * 1000.0);
            constexpr double usToMs = 1.0 / 1000.0;

            if (m_bytesDecompressed.GetNumRecorded() > 0)
            {
                //It only makes sense to add decompression statistics when reading from PAK files.
                statistics.push_back(Statistic::CreateInteger(m_name, "Available decompression slots", m_maxNumJobs - m_numRunningJobs));
                statistics.push_back(Statistic::CreateInteger(m_name, "Available read slots", m_maxNumReads - m_numInFlightReads));
                statistics.push_back(Statistic::CreateFloat(m_name, "Buffer memory (MB)", m_memoryUsage * bytesToMB));

                double averageJobStartDelay = m_decompressionJobDelayMicroSec.CalculateAverage() * usToMs;
                statistics.push_back(Statistic::CreateFloat(m_name, "Decompression job delay (avg. ms)", averageJobStartDelay));

                double totalBytesDecompressedMB = m_bytesDecompressed.GetTotal() * bytesToMB;
                double totalDecompressionTimeSec = m_decompressionDurationMicroSec.GetTotal() * usToSec;
                statistics.push_back(Statistic::CreateFloat(m_name, "Decompression Speed (avg. mbps)", totalBytesDecompressedMB / totalDecompressionTimeSec));

#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
                statistics.push_back(Statistic::CreatePercentage(m_name, "Decompression bound (percentage)", m_decompressionBound.CalculateAverage()));
                statistics.push_back(Statistic::CreatePercentage(m_name, "Read bound (percentage)", m_readBound.CalculateAverage()));
#endif
            }

            StreamStackEntry::CollectStatistics(statistics);
        }

        u32 FullFileDecompressor::GetNumRunningJobs() const
        {
            return m_numRunningJobs;
        }

        void FullFileDecompressor::StartReadFile(FileRequest* compressedReadRequest)
        {
            if (!m_next)
            {
                compressedReadRequest->SetStatus(FileRequest::Status::Failed);
                m_context->MarkRequestAsCompleted(compressedReadRequest);
                return;
            }

            for (u32 i = 0; i < m_maxNumReads; ++i)
            {
                if (m_readBufferStatus[i] == ReadBufferStatus::Unused)
                {
                    FileRequest::CompressedReadData& data = compressedReadRequest->GetCompressedReadData();
                    CompressionInfo& info = data.m_compressionInfo;
                    m_readBuffers[i] = Buffer(new u8[data.m_compressionInfo.m_compressedSize]);
                    m_memoryUsage += data.m_compressionInfo.m_compressedSize;

                    FileRequest* archiveReadRequest = m_context->GetNewInternalRequest();
                    archiveReadRequest->CreateRead(this, compressedReadRequest, m_readBuffers[i].get(), info.m_archiveFilename, info.m_offset, info.m_compressedSize);
                    m_next->PrepareRequest(archiveReadRequest);
                    
                    m_readRequests[i] = archiveReadRequest;
                    m_readBufferStatus[i] = ReadBufferStatus::ReadInFlight;

                    AZ_Assert(m_numInFlightReads < m_maxNumReads, "A FileRequest was queued for reading in FullFileDecompressor, but there's no slots available.");
                    m_numInFlightReads++;

                    return;
                }
            }
            AZ_Assert(false, "m_numInFlightReads (%u) indicates read slots are available, but none were found.", m_numInFlightReads);
        }

        void FullFileDecompressor::FinishReadFile(FileRequest* readRequest)
        {
            u32 readSlot = m_maxNumReads;
            for (u32 i = 0; i < m_maxNumReads; ++i)
            {
                if (m_readRequests[i] == readRequest)
                {
                    readSlot = i;
                    break;
                }
            }
            AZ_Assert(readSlot != m_maxNumReads, "The FileRequest FullFileDecompressor needs to complete wasn't queued.");

            FileRequest* compressedRequest = readRequest->GetParent();
            AZ_Assert(compressedRequest, "Read requests started by FullFileDecompressor is missing a parent request.");
            
            if (readRequest->GetStatus() == FileRequest::Status::Completed)
            {
                m_readBufferStatus[readSlot] = ReadBufferStatus::PendingDecompression;
                
                FileRequest* waitRequest = m_context->GetNewInternalRequest();
                waitRequest->CreateWait(this, compressedRequest);
                m_readRequests[readSlot] = waitRequest;
            }
            else
            {
                FileRequest::CompressedReadData& data = compressedRequest->GetCompressedReadData();
                m_memoryUsage -= data.m_compressionInfo.m_compressedSize;

                m_readBuffers[readSlot].release();
                m_readRequests[readSlot] = nullptr;
                m_readBufferStatus[readSlot] = ReadBufferStatus::Unused;
                AZ_Assert(m_numInFlightReads > 0, "Trying to decrement a read request after was canceled or failed in FullFileDecompressor, but no read requests are supposed to be queued.");
                m_numInFlightReads--;
            }
        }

        bool FullFileDecompressor::StartDecompressions()
        {
            bool queuedJobs = false;
            u32 jobSlot = 0;
            for (u32 readSlot = 0; readSlot < m_maxNumReads; ++readSlot)
            {
                // Find completed read.
                if (m_readBufferStatus[readSlot] != ReadBufferStatus::PendingDecompression)
                {
                    continue;
                }

                // Find decompression slot
                for (; jobSlot < m_maxNumJobs; ++jobSlot)
                {
                    if (m_processingJobs[jobSlot].IsProcessing())
                    {
                        continue;
                    }
                    
                    FileRequest* waitRequest = m_readRequests[readSlot];
                    FileRequest* compressedRequest = waitRequest->GetParent();
                    AZ_Assert(compressedRequest, "Read requests started by FullFileDecompressor is missing a parent request.");

                    // Add this wait so the compressed request isn't fully completed yet as only the read part is now done. The
                    // job thread will finish this wait, which in turn will trigger this function again on the main streaming thread.

                    DecompressionInformation& info = m_processingJobs[jobSlot];
                    info.m_compressedData = AZStd::move(m_readBuffers[readSlot]);
                    info.m_waitRequest = waitRequest;
                    info.m_queueStartTime = AZStd::chrono::high_resolution_clock::now();
                    info.m_jobStartTime = info.m_queueStartTime; // Set these to the same in case the scheduler requests an update before the job has started.

                    AZ::Job* decompressionJob;
                    FileRequest::CompressedReadData& data = compressedRequest->GetCompressedReadData();
                    if (data.m_readOffset == 0 && data.m_readSize == data.m_compressionInfo.m_uncompressedSize)
                    {
                        auto job = [this, &info]()
                        {
                            FullDecompression(m_context, info);
                        };
                        decompressionJob = AZ::CreateJobFunction(job, true, m_decompressionjobContext.get());
                    }
                    else
                    {
                        m_memoryUsage += data.m_compressionInfo.m_uncompressedSize;
                        auto job = [this, &info]()
                        {
                            PartialDecompression(m_context, info);
                        };
                        decompressionJob = AZ::CreateJobFunction(job, true, m_decompressionjobContext.get());
                    }
                    ++m_numRunningJobs;
                    decompressionJob->Start();

                    m_readRequests[readSlot] = nullptr;
                    m_readBufferStatus[readSlot] = ReadBufferStatus::Unused;
                    AZ_Assert(m_numInFlightReads > 0, "Trying to decrement a read request after it's queued for decompression in FullFileDecompressor, but no read requests are supposed to be queued.");
                    m_numInFlightReads--;

                    queuedJobs = true;
                    break;
                }

                if (m_numInFlightReads == 0 || m_numRunningJobs == m_maxNumJobs)
                {
                    return queuedJobs;
                }
            }
            return queuedJobs;
        }

        void FullFileDecompressor::FinishDecompression(FileRequest* waitRequest)
        {
            for (u32 i = 0; i < m_maxNumJobs; ++i)
            {
                if (m_processingJobs[i].m_waitRequest == waitRequest)
                {
                    auto endTime = AZStd::chrono::high_resolution_clock::now();

                    FileRequest* compressedRequest = m_processingJobs[i].m_waitRequest->GetParent();
                    AZ_Assert(compressedRequest, "A wait request attached to FullFileDecompressor was completed but didn't have a parent compressed request.");
                    FileRequest::CompressedReadData& data = compressedRequest->GetCompressedReadData();
                    m_memoryUsage -= data.m_compressionInfo.m_compressedSize;
                    if (data.m_readOffset != 0 || data.m_readSize != data.m_compressionInfo.m_uncompressedSize)
                    {
                        m_memoryUsage -= data.m_compressionInfo.m_uncompressedSize;
                    }

                    m_decompressionJobDelayMicroSec.PushEntry(AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(
                        m_processingJobs[i].m_jobStartTime - m_processingJobs[i].m_queueStartTime).count());
                    m_decompressionDurationMicroSec.PushEntry(AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(
                        endTime - m_processingJobs[i].m_jobStartTime).count());
                    m_bytesDecompressed.PushEntry(data.m_compressionInfo.m_compressedSize);

                    m_processingJobs[i].m_compressedData.reset();
                    AZ_Assert(m_numRunningJobs > 0, "About to complete a decompression job, but the internal count doesn't see a running job.");
                    --m_numRunningJobs;
                    return;
                }
            }
            AZ_Assert(false, "Wait request being processed wasn't queued by FullFileDecompressor.");
        }

        void FullFileDecompressor::FullDecompression(StreamerContext* context, DecompressionInformation& info)
        {
            info.m_jobStartTime = AZStd::chrono::high_resolution_clock::now();

            FileRequest* compressedRequest = info.m_waitRequest->GetParent();
            AZ_Assert(compressedRequest, "A wait request attached to FullFileDecompressor was completed but didn't have a parent compressed request.");
            FileRequest::CompressedReadData& request = compressedRequest->GetCompressedReadData();
            CompressionInfo& compressionInfo = request.m_compressionInfo;
            
            AZ_Assert(request.m_readOffset == 0, "FullFileDecompressor is doing a full decompression on a file request with an offset (%zu).", 
                request.m_readOffset);
            AZ_Assert(compressionInfo.m_uncompressedSize == request.m_readSize,
                "FullFileDecompressor is doing a full decompression, but the target buffer size (%llu) doesn't match the decompressed size (%zu).",
                request.m_readSize, compressionInfo.m_uncompressedSize);
            
            bool success = compressionInfo.m_decompressor(compressionInfo, info.m_compressedData.get(), compressionInfo.m_compressedSize,
                request.m_output, compressionInfo.m_uncompressedSize);
            info.m_waitRequest->SetStatus(success ? FileRequest::Status::Completed : FileRequest::Status::Failed);
            
            context->MarkRequestAsCompleted(info.m_waitRequest);
            context->WakeUpMainStreamThread();
        }

        void FullFileDecompressor::PartialDecompression(StreamerContext* context, DecompressionInformation& info)
        {
            info.m_jobStartTime = AZStd::chrono::high_resolution_clock::now();

            FileRequest* compressedRequest = info.m_waitRequest->GetParent();
            AZ_Assert(compressedRequest, "A wait request attached to FullFileDecompressor was completed but didn't have a parent compressed request.");
            FileRequest::CompressedReadData& request = compressedRequest->GetCompressedReadData();
            CompressionInfo& compressionInfo = request.m_compressionInfo;

            Buffer decompressionBuffer = Buffer(new u8[compressionInfo.m_uncompressedSize]);
            bool success = compressionInfo.m_decompressor(compressionInfo, info.m_compressedData.get(), compressionInfo.m_compressedSize,
                decompressionBuffer.get(), compressionInfo.m_uncompressedSize);
            info.m_waitRequest->SetStatus(success ? FileRequest::Status::Completed : FileRequest::Status::Failed);
            
            memcpy(request.m_output, decompressionBuffer.get() + request.m_readOffset, request.m_readSize);

            context->MarkRequestAsCompleted(info.m_waitRequest);
            context->WakeUpMainStreamThread();
        }
    } // namespace IO
} // namespace AZ
