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

#include <AzCore/Component/TickBus.h>
#include <AzCore/IO/DeviceEventBus.h>
#include <AzCore/IO/StreamerRequest.h>
#include <AzCore/IO/Streamer/FileRange.h>
#include <AzCore/IO/Streamer/Statistics.h>
#include <AzCore/IO/Streamer/StreamerConfiguration.h>
#include <AzCore/IO/Streamer/StreamerContext.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/containers/deque.h>
#include <AzCore/std/containers/forward_list.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/parallel/semaphore.h>
#include <AzCore/std/parallel/condition_variable.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

namespace AZ
{
    namespace IO
    {
        class ReadCache;
        class FileIOBase;
        class FileRequest;
        class StreamStackEntry;

        struct DeviceSpecifications
        {
            enum class DeviceType
            {
                Unknown,
                Fixed, //< Internal storage devices such as hdd or ssd
                Network //< Network storage
            };

            AZStd::vector<AZStd::string> m_pathIdentifiers; //< List of prefixes from the path that can be used to find the device from a path.
            AZStd::string m_deviceName; //< String that uniquely identifies this device.
            DeviceType m_deviceType = DeviceType::Unknown; //< Type of device
            size_t m_sectorSize = 4 * 1024; //< The size of sectors used by the device.
            size_t m_recommendedCacheSize = 512 * 1024; //< The read cache size as recommended by the OS.
            bool m_hasSeekPenalty = true; //< True is there's a physical operations that needs to take place before reading non-continguous data.
        };

        /**
         * Interface for interacting with a device.
         */
        class Device
            : public DeviceRequestBus::Handler
            , public TickBus::Handler
        {
            friend class ReadCache;

        public:
            AZ_CLASS_ALLOCATOR(Device, SystemAllocator, 0)
            
            using SizeType = AZ::u64;

            /**
             * \param threadSleepTimeMS use with EXTREME caution, it will if != -1 it will insert a sleep after every read
             * to the device. This is used for internal performance tweaking in rare cases.
             */
            Device(const DeviceSpecifications& specifications, const AZStd::shared_ptr<StreamStackEntry>& streamStack, 
                StreamerContext& streamerContext, const AZStd::thread_desc* threadDesc = nullptr);
            virtual ~Device();

            const AZStd::string&    GetPhysicalName() const { return m_physicalName; }
            //! Adds a path identifying prefix (such as "C:" on Windows) to list of known identifiers. This function is not thread safe and should be
            //! called when locked through a mutex.
            void AddPathIdentifier(AZStd::string path) { m_pathIdentifiers.push_back(AZStd::move(path)); }
            //! Uses the known path identifiers to determine if this device should load the file at the provided path. This function is not thread safe 
            //! and should be called when locked through a mutex.
            bool IsDeviceForPath(const char* path) const;

            bool CompleteFileRequests();
            /*@{ Implementation of DeviceRequestBus. Note that these functions take the request by-value instead of by-reference
                so the EBus will keep a local copy. This copy is needed because the Streamer thread might not process any of these
                functions before the calling function continues*/
            void ReadRequest(AZStd::shared_ptr<Request> request) override;
            void CancelRequest(AZStd::shared_ptr<Request> request, AZStd::semaphore* sync) override;
            void CreateDedicatedCacheRequest(RequestPath filename, FileRange range, AZStd::semaphore* sync) override;
            void DestroyDedicatedCacheRequest(RequestPath filename, FileRange range, AZStd::semaphore* sync) override;
            void FlushCacheRequest(RequestPath filename, AZStd::semaphore* sync) override;
            void FlushCachesRequest(AZStd::semaphore* sync) override;
            //@}
            //! Collects various metrics that are recorded by streamer and its stream stack.
            // This function is not done through the queued EBus it doesn't require any timing and collecting metrics is thread safe.
            void CollectStatistics(AZStd::vector<Statistic>& statistics);

            //! Suspend the processing of requests. Requests are still queued.
            // This function is not done through the queued EBus because queuing it would mean the call to re-enable would get stuck in the queue.
            void SuspendProcessing();
            //! Resumes the processing of requests after it was suspended with SuspendProcessing.
            // This function is not done through the queued EBus because queuing it would mean the call to re-enable would get stuck in the queue.
            void ResumeProcessing();

            /// Invokes completed request callbacks and clears the completed callback list
            void InvokeCompletedCallbacks();

            template<typename ...FuncArgs, typename ...Args>
            inline void AddCommand(void (DeviceRequest::* func)(FuncArgs...), Args&&... args)
            {
                //Need to create AZStd::result_of and AZStd::declval type trait
                AZ_STATIC_ASSERT((AZStd::is_void<typename std::result_of<decltype(func)(DeviceRequest*, Args...)>::type>::value), "Argument cannot be bound to supplied device function argument");
                {
                    AZStd::lock_guard<AZStd::mutex> lock(m_streamerContext.GetThreadSleepLock());
                    DeviceRequestBus::QueueBroadcast(func, AZStd::forward<Args>(args)...);
                }
                m_streamerContext.WakeUpMainStreamThread();
            }

        protected:
            //! Cached information about the last entry in the stack to speed up sorting.
            //! Note that this keeps track of the last request sent to the stack, even if it has already completed.
            //! This is because the next request will follow that last request eventually so caches, etc. will be
            //! primed to read from the last file.
            struct
            {
                RequestPath m_filePath; //< The path to the active file.
                SizeType m_offset; //< The offset into the active path.
            } m_activeRead;

            enum class Priority
            {
                FirstRequest, //< The first request is the most important to process next.
                SecondRequest, //< The second request is the most important to process next.
                Equal //< Both requests are equally important.
            };
            //! Determine which of the two provided requests is more important to process next.
            Priority PrioritizeRequests(const AZStd::shared_ptr<Request>& first, const AZStd::shared_ptr<Request>& second) const;

            //! The stream stack to access files through.
            AZStd::shared_ptr<StreamStackEntry> m_streamStack;

            //! General context used by the stream stack.
            StreamerContext& m_streamerContext;

            // Stats to inform the device request scheduler and for profiling purposes.
            
#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
            //! Percentage of reads that didn't read the entire file. These can be sub-optimal if multiple portions of a file
            //! are read, especially if the file is also compressed.
            AverageWindow<size_t, double, s_statisticsWindowSize> m_partialReadPercentage;
            //! By how much time the prediction was off. This mostly covers the latter part of scheduling, which
            //! gets more precise the closer the request gets to completion.
            AverageWindow<u64, double, s_statisticsWindowSize> m_mispredictionUS;
            //! Tracks the percentage of requests that were predicted to complete after the actual completion time
            //! versus the requests that were predicted to come in earlier than predicted.
            AverageWindow<u8, double, s_statisticsWindowSize> m_latePredictionsPercentage;
            //! Percentage of reads that missed their deadline. If percentage is too high it can indicate that there are too
            //! many file requests or the deadlines of too many files are too tight.
            AverageWindow<u8, double, s_statisticsWindowSize> m_missedDeadlinePercentage;
            //! Percentage of reads that come in that are already on their deadline. Requests like this are disruptive
            //! as they cause the scheduler to prioritize these over the most optimal read layout.
            AverageWindow<u8, double, s_statisticsWindowSize> m_immediateReadsPercentage;
#endif

            void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

            //////////////////////////////////////////////////////////////////////////
            // Commands, called from the device thread with the thread sleep lock locked.
            void                    Read(const AZStd::shared_ptr<Request>& request);
            //////////////////////////////////////////////////////////////////////////

            //! Reads data from a file through the provided cache. If any data has already been stored in the cache
            //! it's reused. If less than an entire cache block is requested, the full block is read and the entire
            //! block is stored in the cache. If neither conditions are met, the cache is ignored.
            void                    ReadStream(const AZStd::shared_ptr<Request>& request);
            
            //////////////////////////////////////////////////////////////////////////
            /// DEVICE THREAD EXCLUSIVE DATA

            /// Schedule all requests.
            void ScheduleRequests();

            /// Device thread "main" function"
            void ProcessRequests();

            //! Complete a request. isLocked refers to thread sleep mutex, if true we don't need to lock it inside the function.
            void CompleteRequest(const AZStd::shared_ptr<Request>& request, Request::StateType state = Request::StateType::ST_COMPLETED, bool isLocked = false);
            
            AZStd::vector<AZStd::string>    m_pathIdentifiers;  //< Set of path prefix that identify if a file needs to accessed through this device. Accessing this member requires a lock.
            AZStd::string                   m_physicalName;     ///< Device name (doesn't change while running so it's safe to read from multiple threads.
            AZStd::forward_list<FileRequest*> m_queued; //!< The requests queued in the stream stack.
            AZStd::vector<FileRequest*> m_internalPendingBuffer; //!< To avoid allocating memory every time requests are scheduled, the temporary vector is cached here.
            AZStd::vector<AZStd::shared_ptr<Request>> m_completed;        ///< List of completed requests for this device. (must be accessed with the m_lock (locked)
            AZStd::deque<AZStd::shared_ptr<Request>> m_pending;    //< List of pending request for this device.

            AZStd::atomic_bool          m_shutdown_thread;
            AZStd::atomic_bool          m_suspendRequestProcessing; //< Requests are still queued, but not processed.
            AZStd::thread               m_thread;
        };
    }
}
