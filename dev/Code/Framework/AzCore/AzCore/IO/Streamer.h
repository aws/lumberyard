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

#include <AzCore/base.h>
#include <AzCore/IO/DeviceEventBus.h>
#include <AzCore/IO/StreamerRequest.h>
#include <AzCore/IO/Streamer/Statistics.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/function/function_fwd.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string.h>

namespace AZStd
{
    struct thread_desc;
}

namespace AZ
{
    namespace IO
    {
        class FileIOStream;
        class Streamer;
        struct StreamerData;
        class StreamStackEntry;

        class Device;

        Streamer* GetStreamer();

        /**
         * Data streamer.
         *
         */
        class Streamer
        {
        public:
            AZ_CLASS_ALLOCATOR(Streamer, SystemAllocator, 0);

            typedef AZ::u64 SizeType;

            static const SizeType EndOfStream = static_cast<SizeType>(-1);

            struct Descriptor
            {
                Descriptor()
                    : m_threadDesc(nullptr)
                {}
                AZStd::thread_desc*    m_threadDesc;    ///< Pointer to optional thread descriptor structure. It will be used for each thread spawned (one for each device).
                AZStd::shared_ptr<StreamStackEntry> m_streamStack; ///< The stack used for file reads that go directly to the file system and decompress asynchronously.
            };

            static bool         Create(const Descriptor& desc);
            static void         Destroy();
            static bool         IsReady();
            static Streamer&    Instance();

            //! Asynchronously read data from a file and immediately wait for the results.
            //! @fileName Relative path to the file to read from.
            //! @byteOffset Offset in bytes into the file.
            //! @byteSize The amount of bytes to read. The byteOffset plus byteSize can not exceed the length of the file.
            //! @databuffer The buffer to read data into. This buffer has to be at least byteSize or larger.
            //! @deadline The amount of time from the moment the request is queued to complete the request. Or "ExecuteWhenIdle" in case there's no
            //!           deadline.
            //! @priority The priority in which requests are ordered that have passed their deadline.
            //! @state Optional variable to keep track of the state of the request. This is mostly used to help determine if a request was successful or not.
            //! @debugName A name to help identify the purpose of the request during debugging.
            //! @return The number of bytes that were read into dataBuffer. If the request fails there may still be partial data available.
            SizeType Read(const char* fileName, SizeType byteOffset, SizeType byteSize, void* dataBuffer,
                AZStd::chrono::microseconds deadline = ExecuteWhenIdle, Request::PriorityType priority = Request::PriorityType::DR_PRIORITY_NORMAL, 
                Request::StateType* state = nullptr, const char* debugName = nullptr);
            //! Asynchronously read data from a file.
            //! @fileName Relative path to the file to read from.
            //! @byteOffset Offset in bytes into the file.
            //! @byteSize The amount of bytes to read. The byteOffset plus byteSize can not exceed the length of the file.
            //! @databuffer The buffer to read data into. This buffer has to be at least byteSize or larger.
            //! @callback Optional callback to call when the request has completed. It's important that the callback happens as quick as possible
            //!           because it will hold up the Streamer or decompression threads. For long callback it's recommended to have the callback
            //!           start a job to continue processing.
            //! @deadline The amount of time from the moment the request is queued to complete the request. Or "ExecuteWhenIdle" in case there's no
            //!           deadline.
            //! @priority The priority in which requests are ordered that have passed their deadline.
            //! @debugName A name to help identify the purpose of the request during debugging.
            //! @deferCallback Whether or not the callback is delayed and called from the main thread instead.
            //! @return Whether or not the read request was successfully created and queued.
            bool ReadAsync(const char* fileName, SizeType byteOffset, SizeType byteSize, void* dataBuffer, const Request::RequestDoneCB& callback, 
                AZStd::chrono::microseconds deadline = ExecuteWhenIdle, Request::PriorityType priority = Request::PriorityType::DR_PRIORITY_NORMAL, 
                const char* debugName = nullptr, bool deferCallback = false);
            
            //! Creates a read request, but doesn't queue the request for processing. This can be useful if the returned handle is used in the callback or
            //! needs to be stored before the request can complete. See ReadAsync for arguments.
            AZStd::shared_ptr<Request> CreateAsyncRead(const char* fileName, SizeType byteOffset, SizeType byteSize, void* dataBuffer, const Request::RequestDoneCB& callback,
                AZStd::chrono::microseconds deadline = ExecuteWhenIdle, Request::PriorityType priority = Request::PriorityType::DR_PRIORITY_NORMAL,
                const char* debugName = nullptr, bool deferCallback = false);
            //! Queue a previously created request for processing. This only applies to the Create* functions, other functions will automatically queue.
            bool QueueRequest(AZStd::shared_ptr<Request> request);

            /// Cancel request in a blocking mode, function will NOT return until cancel is complete. Request callback will be called with Request::StateType == ST_CANCELLED.
            void CancelRequest(const AZStd::shared_ptr<Request>& request);
            /// Cancel request in async mode, function returns instantly. Request callback will be called with Request::StateType == ST_CANCELLED. This function takes in an optional semaphore which will be forwared to the Device::CancelRequest method
            void CancelRequestAsync(const AZStd::shared_ptr<Request>& request, AZStd::semaphore* sync = nullptr);

            /// Changes the scheduling parameters of the request.
            static void RescheduleRequest(const AZStd::shared_ptr<Request>& request, Request::PriorityType priority, AZStd::chrono::microseconds deadline);

            /// Returns the estimated completion time from NOW is microseconds.
            static AZStd::chrono::microseconds EstimateCompletion(const AZStd::shared_ptr<Request>& request);

            //! Creates a dedicated cache for the provided file. This is useful for files that semi-frequently need to be 
            //! visited such as video or audio files. Files like that usually read a subsection of the file to do some
            //! processing with. In between other file requests might flush the file out of the cache. Using a dedicated
            //! cache will prevent this from happening. Caches created through this call need to be cleared with
            //! DestroyDedicatedCache
            //! This function will wait until the request has been processed by Streamer.
            void CreateDedicatedCache(const char* filename);
            //! Creates a dedicated cache for the provided file. This is useful for files that semi-frequently need to be 
            //! visited such as video or audio files. Files like that usually read a subsection of the file to do some
            //! processing with. In between other file requests might flush the file out of the cache. Using a dedicated
            //! cache will prevent this from happening. Caches created through this call need to be cleared with
            //! DestroyDedicatedCache
            //! This function will immediately return while the file cache is created in the background.
            void CreateDedicatedCacheAsync(const char* filename, AZStd::semaphore* sync = nullptr);
            //! Destroy a dedicated cache created by CreateDedicatedCache. See CreateDedicatedCache for more details.
            //! This function will wait until the request has been processed by Streamer.
            void DestroyDedicatedCache(const char* filename);
            //! Destroy a dedicated cache created by CreateDedicatedCache. See CreateDedicatedCache for more details.
            //! This function will immediately return while the file cache is destroyed in the background.
            void DestroyDedicatedCacheAsync(const char* filename, AZStd::semaphore* sync = nullptr);

            //! Clears a file from any caches in use by Streamer.
            //! This function will wait until the request has been processed by Streamer.
            void FlushCache(const char* filename);
            //! Clears a file from any caches in use by Streamer.
            //! This function will immediately return while the file cache is destroyed in the background.
            void FlushCacheAsync(const char* filename, AZStd::semaphore* sync = nullptr);
            //! Forcefully clears out all caches internally held by the available devices.
            //! This function will wait until the request has been processed by Streamer.
            void FlushCaches();
            //! Forcefully clears out all caches internally held by the available devices.
            //! This function will immediately return while the caches are cleared in the background.
            void FlushCachesAsync();

            //! Blocking call to collect statistics from all the components that make up Streamer. This is thread safe
            //! in the sense that it won't crash. Data is collected lockless from involved threads and might be slightly
            //! off in rare cases.
            void CollectStatistics(AZStd::vector<Statistic>& statistics);

            //! Looks for a StreamStackEntry in the streaming stack by name and replaces it with the 
            //! provided replacement.
            //! This call is thread safe, though it's recommended to only be done during engine initialization, because
            //! results from one requests (such as file size) may not be correct for a replaced entry (such as a file read).
            bool ReplaceStreamStackEntry(AZStd::string_view entryName, const AZStd::shared_ptr<StreamStackEntry>& replacement);

            /**
             * Must be pumped to receive deferred request.
             */
            void ReceiveRequests();

            //! Suspend the processing of requests on all devices. Requests are still queued.
            void SuspendAllDeviceProcessing();
            //! Resumes the processing of requests on all devices after a call to SuspendAllDeviceProcessing.
            void ResumeAllDeviceProcessing();

            /// Finds a device that will handle a specific stream. This includes overlay/remap to other devices etc.
            Device*         FindDevice(const char* streamName, bool resolvePath = true);

            /// Releases all currently allocated devices.
            void ReleaseDevices();

        private:
            explicit Streamer(const Descriptor& desc);
            ~Streamer();

            //! Resolves the provided to the appropriate request path.
            //! @resolvedPath The resolved path for the provided filename. This can be the path to a loose file or an archive.
            //! @isArchivedFile True if resolvedPath point to an archive, otherwise false.
            //! @foundCompressionInfo If isArchivedFile is true, this will hold the compression info for the file in the archive.
            //! @fileName The relative path to the file to look up.
            //! @return True if the path could be resolved or false if a problem was encountered.
            bool ResolveRequestPath(RequestPath& resolvedPath, bool& isArchivedFile, CompressionInfo& foundCompressionInfo, const char* fileName) const;

            struct StreamerData* m_data;
        };
    } // namespace IO
} // namespace AZ
