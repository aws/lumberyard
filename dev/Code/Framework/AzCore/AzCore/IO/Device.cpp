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

#include <limits>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Debug/ProfilerBus.h>
#include <AzCore/PlatformIncl.h>
#include <AzCore/IO/CompressionBus.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Device.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/StreamerUtil.h>
#include <AzCore/IO/StreamerDrillerBus.h>
#include <AzCore/IO/Streamer/FileRequest.h>
#include <AzCore/IO/Streamer/StorageDrive.h>
#include <AzCore/IO/Streamer/StreamStackEntry.h>
#include <AzCore/std/containers/array.h>

#include <AzCore/std/sort.h>

#include <AzCore/Math/Crc.h>

namespace AZ
{
    namespace IO
    {

        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        // Device
        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////

        //=========================================================================
        // Device
        // [10/2/2009]
        //=========================================================================
        Device::Device(const DeviceSpecifications& specifications, const AZStd::shared_ptr<StreamStackEntry>& streamStack, 
            StreamerContext& streamerContext, const AZStd::thread_desc* threadDesc)
            : m_physicalName(specifications.m_deviceName)
            , m_shutdown_thread(false)
            , m_suspendRequestProcessing(false)
            , m_thread(AZStd::bind(&Device::ProcessRequests, this), threadDesc)
            , m_streamStack(streamStack)
            , m_streamerContext(streamerContext)
        {
            for (const AZStd::string& pathId : specifications.m_pathIdentifiers)
            {
                AddPathIdentifier(pathId);
            }
            DeviceRequestBus::Handler::BusConnect();
            TickBus::Handler::BusConnect();
        }

        //=========================================================================
        // ~Device
        // [10/2/2009]
        //=========================================================================
        Device::~Device()
        {
            TickBus::Handler::BusDisconnect();
            DeviceRequestBus::Handler::BusDisconnect();
            {
                m_shutdown_thread.store(true, AZStd::memory_order_release);
                m_streamerContext.WakeUpMainStreamThread();
            }
            m_thread.join();

            // Delete all requests
            AZ_Warning("IO", m_pending.empty(), "We have %d pending request(s)! Canceling...", m_pending.size());
            AZ_Warning("IO", m_completed.empty(), "We have %d completed request(s)! Ignoring...", m_completed.size());

            FlushCachesRequest(nullptr);
        }
        void Device::OnTick(float, ScriptTimePoint)
        {
            bool isEnabled = false;
            AZ::Debug::ProfilerRequestBus::BroadcastResult(isEnabled, &AZ::Debug::ProfilerRequests::IsActive);
            
            if (isEnabled)
            {
                AZStd::vector<Statistic> statistics;
                CollectStatistics(statistics);
                for (Statistic& stat : statistics)
                {
                    switch (stat.GetType())
                    {
                    case Statistic::Type::FloatingPoint:
                        AZ_PROFILE_DATAPOINT(AZ::Debug::ProfileCategory::AzCore, stat.GetFloatValue(), "Streamer/%s/%s", stat.GetOwner().data(), stat.GetName().data());
                        break;
                    case Statistic::Type::Integer:
                        AZ_PROFILE_DATAPOINT(AZ::Debug::ProfileCategory::AzCore, stat.GetIntegerValue(), "Streamer/%s/%s", stat.GetOwner().data(), stat.GetName().data());
                        break;
                    case Statistic::Type::Percentage:
                        AZ_PROFILE_DATAPOINT_PERCENT(AZ::Debug::ProfileCategory::AzCore, stat.GetPercentage(), "Streamer/%s/%s (percent)", stat.GetOwner().data(), stat.GetName().data());
                        break;
                    default:
                        AZ_Assert(false, "Unsupported statistic type: %i", stat.GetType());
                        break;
                    }
                }
            }
        }

        //=========================================================================
        // Read
        // [10/5/2009]
        //=========================================================================
        void Device::Read(const AZStd::shared_ptr<Request>& request)
        {
            static AZ::u64 numRequests = 0;
            request->m_requestId = numRequests++;

            m_pending.push_back(request);

#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
            m_immediateReadsPercentage.PushEntry(AZStd::chrono::system_clock::now() > request->m_deadline ? 1 : 0);
#endif
            EBUS_DBG_EVENT(StreamerDrillerBus, OnAddRequest, request);
        }

        void Device::ReadStream(const AZStd::shared_ptr<Request>& request)
        {
#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
            AZ_PROFILE_SCOPE_DYNAMIC(AZ::Debug::ProfileCategory::AzCore, "Device::ReadStream Request %i - %s from %s", 
                request->m_requestId, request->m_originalFilename.c_str(), request->m_filename.GetRelativePath());
#else
            AZ_PROFILE_SCOPE_DYNAMIC(AZ::Debug::ProfileCategory::AzCore, "Device::ReadStream Request %i - %s", 
                request->m_requestId, request->m_filename.GetRelativePath());
#endif
            FileRequest* link = m_streamerContext.GetNewInternalRequest();
            link->CreateRequestLink(AZStd::shared_ptr<Request>(request));
            m_queued.push_back(link);
            FileRequest* readRequest = m_streamerContext.GetNewInternalRequest();

            bool needsDecompression = request->NeedsDecompression();
            AZ::IO::SizeType readSize = needsDecompression ? request->m_compressionInfo.m_compressedSize : request->m_byteSize;
            AZ::IO::SizeType readOffset = needsDecompression ? request->m_compressionInfo.m_offset : request->GetStreamByteOffset();
            
            if (needsDecompression)
            {
                readRequest->CreateCompressedRead(nullptr, link, request->m_compressionInfo, request->m_buffer, request->m_byteOffset, request->m_byteSize);
            }
            else
            {
                readRequest->CreateRead(nullptr, link, request->m_buffer, request->m_filename, readOffset, readSize);
            }

            readRequest->SetStatus(FileRequest::Status::Queued);
            m_streamStack->PrepareRequest(readRequest);

#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
            bool isPartialRead = false;
            if (needsDecompression)
            {
                isPartialRead = 
                    request->GetStreamByteOffset() != 0 || 
                    request->m_byteSize != request->m_compressionInfo.m_uncompressedSize;
            }
            else
            {
                u64 fileSize = 0;
                m_streamStack->GetFileSize(fileSize, request->m_filename);
                isPartialRead = 
                    request->GetStreamByteOffset() != 0 || 
                    request->m_byteSize != fileSize;
            }
            m_partialReadPercentage.PushEntry(isPartialRead ? 1 : 0);
#endif // #if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO

            m_activeRead.m_filePath = request->m_filename;
            m_activeRead.m_offset = readOffset + readSize;
        }

        void Device::CreateDedicatedCacheRequest(RequestPath filename, FileRange range, AZStd::semaphore* sync)
        {
            m_streamStack->CreateDedicatedCache(filename, range);

            if (sync)
            {
                sync->release();
            }
        }

        void Device::DestroyDedicatedCacheRequest(RequestPath filename, FileRange range, AZStd::semaphore* sync)
        {
            m_streamStack->DestroyDedicatedCache(filename, range);
            
            if (sync)
            {
                sync->release();
            }
        }

        Device::Priority Device::PrioritizeRequests(const AZStd::shared_ptr<Request>& first, const AZStd::shared_ptr<Request>& second) const
        {
            bool firstInPanic = first->m_estimatedCompletion > first->m_deadline;
            bool secondInPanic = second->m_estimatedCompletion > second->m_deadline;
            // Both request are at risk of not completing before their deadline.
            if (firstInPanic && secondInPanic)
            {
                // Let the one with the highest priority go first.
                if (first->m_priority != second->m_priority)
                {
                    return first->m_priority < second->m_priority ? Priority::FirstRequest : Priority::SecondRequest;
                }

                // If neither has started and have the same priority, prefer to start the closest deadline.
                return first->m_deadline <= second->m_deadline ? Priority::FirstRequest : Priority::SecondRequest;
            }

            // Check if one of the requests is in panic and prefer to prioritize that request
            if (firstInPanic) { return Priority::FirstRequest; }
            if (secondInPanic) { return Priority::SecondRequest; }

            // Both are not in panic so base the order on the number of IO steps (opening files, seeking, etc.) are needed.

            bool firstInSameFile = m_activeRead.m_filePath == first->m_filename;
            bool secondInSameFile = m_activeRead.m_filePath == second->m_filename;
            // If both request are in the active file, prefer to pick the file that's closest to the last read.
            if (firstInSameFile && secondInSameFile)
            {
                SizeType firstReadOffset = (first->NeedsDecompression() ? first->m_compressionInfo.m_offset : first->GetStreamByteOffset()) + first->m_bytesProcessedStart;
                SizeType secondReadOffset = (second->NeedsDecompression() ? second->m_compressionInfo.m_offset : second->GetStreamByteOffset()) + second->m_bytesProcessedStart;
                SizeType firstSeekDistance = static_cast<SizeType>(abs(static_cast<s64>(m_activeRead.m_offset) - static_cast<s64>(firstReadOffset)));
                SizeType secondSeekDistance = static_cast<SizeType>(abs(static_cast<s64>(m_activeRead.m_offset) - static_cast<s64>(secondReadOffset)));
                return firstSeekDistance <= secondSeekDistance ? Priority::FirstRequest : Priority::SecondRequest;
            }

            // Prefer to continue in the same file so prioritize the request that's in the same file
            if (firstInSameFile) { return Priority::FirstRequest; }
            if (secondInSameFile) { return Priority::SecondRequest; }

            // If both requests need to open a new file, keep them in the same order as there's no information available
            // to indicate which request would be able to load faster or more efficiently.
            return Priority::Equal;
        }

        void Device::ScheduleRequests()
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzCore);

            AZStd::chrono::system_clock::time_point now = AZStd::chrono::system_clock::now();

            m_internalPendingBuffer.clear();
            m_streamStack->UpdateCompletionEstimates(now, m_internalPendingBuffer, m_pending);
            // Add the compression delay and copy the estimation from the internal request to the external request.
            for (FileRequest* queuedRequest : m_queued)
            {
                FileRequest::RequestLinkData& link = queuedRequest->GetRequestLinkData();
                AZ_Assert(link.m_request->IsReadOperation(), "Queued request (%llu) is not a read request as expected.", link.m_request->m_requestId);
                link.m_request->m_estimatedCompletion = queuedRequest->GetEstimatedCompletion();
            }

            // Resort the pending requests.
            if (!m_pending.empty())
            {
                AZ_PROFILE_SCOPE_DYNAMIC(AZ::Debug::ProfileCategory::AzCore, "Device::ScheduleRequests - Sorting %i requests", m_pending.size());
                auto sorter = [this](const AZStd::shared_ptr<Request>& lhs, const AZStd::shared_ptr<Request>& rhs) -> bool
                {
                    Priority priority = PrioritizeRequests(lhs, rhs);
                    switch (priority)
                    {
                    case Priority::FirstRequest:
                        return true;
                    case Priority::SecondRequest:
                        return false;
                    case Priority::Equal:
                        // If both requests can't be prioritized relative to each other than keep them in the order they
                        // were originally queued.
                        return lhs->m_requestId < rhs->m_requestId;
                    default:
                        AZ_Assert(false, "Unsupported request priority: %i.", priority);
                        return true;
                    }
                };

                AZStd::sort(m_pending.begin(), m_pending.end(), sorter);
            }
        }

        //=========================================================================
        // Complete Request
        // [11/18/2011]
        //=========================================================================
        void Device::CompleteRequest(const AZStd::shared_ptr<Request>& request, Request::StateType state, bool isLocked)
        {
            EBUS_DBG_EVENT(StreamerDrillerBus, OnCompleteRequest, request, state);

            AZ_Assert(!request->HasCompleted(), "Trying to complete a request that has already completed.");

#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
            // Only record how far off the completion prediction was if the read was successful.
            if (state == Request::StateType::ST_COMPLETED)
            {
                auto now = AZStd::chrono::system_clock::now();
                if (now > request->m_estimatedCompletion)
                {
                    m_mispredictionUS.PushEntry(AZStd::chrono::microseconds(now - request->m_estimatedCompletion).count());
                    m_latePredictionsPercentage.PushEntry(0);
                }
                else
                {
                    m_mispredictionUS.PushEntry(AZStd::chrono::microseconds(request->m_estimatedCompletion - now).count());
                    m_latePredictionsPercentage.PushEntry(1);
                }
                m_missedDeadlinePercentage.PushEntry(request->m_deadline < now ? 1 : 0);
            }
#endif

            request->m_state = state;
            if (request->m_callback)
            {
                if (!request->m_isDeferredCB)
                {
                    AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzCore, "Request callback");
                    // if this is too long this will hold the device thread... use lockless or a separate lock
                    if (request->NeedsDecompression())
                    {
                        request->m_callback(request, request->m_byteSize, request->m_buffer, state);
                    }
                    else
                    {
                        request->m_callback(request, request->m_bytesProcessedStart + request->m_bytesProcessedEnd, request->m_buffer, state);
                    }
                }
                else
                {
                    if (!isLocked)
                    {
                        m_streamerContext.GetThreadSleepLock().lock();
                    }
                    m_completed.push_back(request);
                    if (!isLocked)
                    {
                        m_streamerContext.GetThreadSleepLock().unlock();
                    }
                }
            }
        }

        /*!
        \brief Invokes the callbacks of completed request
        */
        void Device::InvokeCompletedCallbacks()
        {
            if (!m_completed.empty())
            {
                AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzCore);
                m_streamerContext.GetThreadSleepLock().lock();
                AZStd::vector<AZStd::shared_ptr<Request>> completedRequests = AZStd::move(m_completed);
                m_streamerContext.GetThreadSleepLock().unlock();
                
                for(const AZStd::shared_ptr<Request>& request : completedRequests)
                {
                    AZ_Assert(request, "Stored request was unexpectedly deleted.");
                    AZ_Assert(request->m_isDeferredCB, "Queued callback was not marked to be deferred.");
                    AZ_Assert(request->m_callback, "Only requests with a deferred callback should be queued for later execution.");
                    if (request->NeedsDecompression())
                    {
                        request->m_callback(request, request->m_byteSize, request->m_buffer, request->m_state);
                    }
                    else
                    {
                        request->m_callback(request, request->m_bytesProcessedStart + request->m_bytesProcessedEnd, request->m_buffer, request->m_state);
                    }
                }
            }
        }

        //=========================================================================
        // ProcessRequests
        // [10/2/2009]
        //=========================================================================
        void Device::ProcessRequests()
        {
            while (!m_shutdown_thread.load(AZStd::memory_order_acquire))
            {
                auto predicate = [this]() -> bool
                {
                    if (m_shutdown_thread.load(AZStd::memory_order_acquire))
                    {
                        return true;
                    }

                    if (m_suspendRequestProcessing.load(AZStd::memory_order_acquire))
                    {
                        return false;
                    }

                    if (m_streamerContext.HasRequestsToFinalize())
                    {
                        return true;
                    }

                    if (DeviceRequestBus::QueuedEventCount() == 0 && m_pending.empty())
                    {
                        return false;
                    }
                    return true;
                };
                {
                    AZStd::unique_lock<AZStd::mutex> lock(m_streamerContext.GetThreadSleepLock());
                    m_streamerContext.GetThreadSleepCondition().wait(lock, predicate);
                    
                    // execute commands
                    DeviceRequestBus::ExecuteQueuedEvents();
                }

                ScheduleRequests();
                do
                {
                    do
                    {
                        if (m_pending.empty())
                        {
                            continue;
                        }

                        AZStd::shared_ptr<Request> request = m_pending.front();
                        if (m_streamStack->GetAvailableRequestSlots() > 0)
                        {
                            m_pending.pop_front();
                            request->m_state = Request::StateType::ST_IN_PROCESS;
                            if (request->IsReadOperation())
                            {
                                ReadStream(request);
                            }
                            else
                            {
                                AZ_Assert(false, "Unsupported operation (%i) queued for AZ::IO::Streamer, or operation not processed at the appropriate point.", request->m_operation);
                            }
                        }
                    } while (CompleteFileRequests());
                } while (m_streamStack->ExecuteRequests());
            }
        }

        bool Device::CompleteFileRequests()
        {
            AZStd::vector<FileRequest*> completedRequestLinks;
            bool hasFinalizedRequests = m_streamerContext.FinalizeCompletedRequests(completedRequestLinks);
            if (completedRequestLinks.empty())
            {
                return hasFinalizedRequests;
            }

            for (FileRequest* requestLink : completedRequestLinks)
            {
                AZ_Assert(requestLink->GetOperationType() == FileRequest::Operation::RequestLink,
                    "File request of type %i was passed to the Device as a RequestLink and can't be completed.", requestLink->GetOperationType());
                FileRequest::Status status = requestLink->GetStatus();
                FileRequest::RequestLinkData& link = requestLink->GetRequestLinkData();
                switch (status)
                {
                case FileRequest::Status::Completed:
                    link.m_request->m_bytesProcessedStart = link.m_request->m_byteSize;
                    CompleteRequest(link.m_request, Request::StateType::ST_COMPLETED, false);
                    break;
                case FileRequest::Status::Canceled:
                    CompleteRequest(link.m_request, Request::StateType::ST_CANCELLED, false);
                    break;
                case FileRequest::Status::Failed:
                    CompleteRequest(link.m_request, Request::StateType::ST_ERROR_FAILED_IN_OPERATION, false);
                    break;
                default:
                    AZ_Assert(false, "File request queued for finalization is still in status %i.", status);
                    CompleteRequest(link.m_request, Request::StateType::ST_ERROR_FAILED_IN_OPERATION, false);
                }

                if (link.m_cancelSync)
                {
                    link.m_cancelSync->release();
                }

                auto it = AZStd::find(m_queued.begin(), m_queued.end(), requestLink);
                AZ_Assert(it != m_queued.end(), "A job was finalized that's not been queued.");
                m_queued.erase(it);

                m_streamerContext.RecycleRequest(requestLink);
            }

            return true;
        }

        /*!
        \brief Queues a Read Request for processing on the Device RegisterRequest thread
        \param[in] request Handle to request object which contains request priority, current state and callback
        \param[in] sync optional semaphore which is released after the request has been serviced
        */
        void Device::ReadRequest(AZStd::shared_ptr<Request> request)
        {
            if (!request->m_filename.IsValid())
            {
                AZ_Assert(false, "No file provided for streaming.");
                CompleteRequest(request, Request::StateType::ST_ERROR_FAILED_TO_OPEN_FILE, true);
                return;
            }

            Read(request);

            // For synchronization the read request doesn't use a semaphore but instead the callback
            // on the request is attached to SyncRequestCallback.
        }

        //=========================================================================
        // CancelRequest
        // [10/5/2009]
        //=========================================================================
        void Device::CancelRequest(AZStd::shared_ptr<Request> request, AZStd::semaphore* sync)
        {
            AZ_Assert(request, "Invalid request handle!");
            EBUS_DBG_EVENT(StreamerDrillerBus, OnCancelRequest, request);
            
            if (request->IsPendingProcessing())
            {
                auto it = AZStd::find(m_pending.begin(), m_pending.end(), request);
                if (it != m_pending.end())
                {
                    m_pending.erase(it);
                }
                CompleteRequest(request, Request::StateType::ST_CANCELLED);
                if (sync)
                {
                    sync->release();
                }
            }
            else
            {
                for (FileRequest* internalRequest : m_queued)
                {
                    FileRequest::RequestLinkData& link = internalRequest->GetRequestLinkData();
                    if (link.m_request == request)
                    {
                        link.m_cancelSync = sync;
                        FileRequest* cancel = m_streamerContext.GetNewInternalRequest();
                        cancel->CreateCancel(internalRequest);
                        m_streamStack->PrepareRequest(cancel);
                        break;
                    }
                }
            }
        }

        void Device::FlushCacheRequest(RequestPath filename, AZStd::semaphore* sync)
        {
            m_streamStack->FlushCache(filename);

            if (sync)
            {
                sync->release();
            }
        }

        void Device::FlushCachesRequest(AZStd::semaphore* sync)
        {
            m_streamStack->FlushEntireCache();

            if (sync)
            {
                sync->release();
            }
        }

        void Device::CollectStatistics(AZStd::vector<Statistic>& statistics)
        {
            m_streamStack->CollectStatistics(statistics);

#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
            statistics.push_back(Statistic::CreatePercentage("Device", "Partial reads", m_partialReadPercentage.CalculateAverage()));
            statistics.push_back(Statistic::CreateFloat("Device", "Completion misprediction (ms)", m_mispredictionUS.CalculateAverage() / 1000.0));
            statistics.push_back(Statistic::CreatePercentage("Device", "Late vs early predictions", m_latePredictionsPercentage.CalculateAverage()));
            statistics.push_back(Statistic::CreatePercentage("Device", "Missed deadlines", m_missedDeadlinePercentage.CalculateAverage()));
            statistics.push_back(Statistic::CreatePercentage("Device", "Immediate reads", m_immediateReadsPercentage.CalculateAverage()));
#endif

            statistics.push_back(Statistic::CreateInteger("Device", "Available slots", m_streamStack->GetAvailableRequestSlots()));
            statistics.push_back(Statistic::CreateInteger("Device", "Num pending requests", m_pending.size()));
        }

        void Device::SuspendProcessing()
        {
            m_suspendRequestProcessing = true;
        }

        void Device::ResumeProcessing()
        {
            m_suspendRequestProcessing = false;
            m_streamerContext.WakeUpMainStreamThread();
        }

        bool Device::IsDeviceForPath(const char* path) const
        {
            for (const AZStd::string& identifier : m_pathIdentifiers)
            {
                if (strncmp(path, identifier.c_str(), identifier.size()) == 0)
                {
                    return true;
                }
            }
            return false;
        }
    } // namespace IO
} // namespace AZ
