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
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/IO/StreamerRequest.h>

#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/function/function_fwd.h>
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
                    : m_maxNumOpenLimitedStream(64)
                    , m_fileMountPoint(NULL)
                    , m_cacheBlockSize(32 * 1024)
                    , m_numCacheBlocks(4)
                    , m_deviceThreadSleepTimeMS(-1)
                    , m_threadDesc(nullptr)
                {}
                unsigned int            m_maxNumOpenLimitedStream;      ///< Max number of open streams with limited flag on. \ref Stream::m_isLimited set to true.
                const char*             m_fileMountPoint;               ///< Pointer to a mount point name, so when you pass a name like "blabla.dat" it will resolve to m_fileMountPoint + "blabla.dat"
                unsigned int            m_cacheBlockSize;
                unsigned int            m_numCacheBlocks;
                int                     m_deviceThreadSleepTimeMS;      ///< Time for the device thread to sleep between each operation. Use with caution.
                AZStd::thread_desc*    m_threadDesc;    ///< Pointer to optional thread descriptor structure. It will be used for each thread spawned (one for each device).
            };

            static bool         Create(const Descriptor& desc);
            static void         Destroy();
            static bool         IsReady();
            static Streamer&    Instance();

            //@{ File based streams
            /**
             * Registers (create or find) a file stream.
             * \param flags \ref OpenMode
             */
            VirtualStream* RegisterFileStream(const char* fileName, OpenMode flags = OpenMode::ModeRead, bool isLimited = false);
            void    UnRegisterFileStream(const char* fileName);
            //@}

            /// Create or find stream from a stream name.
            bool RegisterStream(VirtualStream* stream, OpenMode flags = OpenMode::ModeRead, bool isLimited = false);
            void    UnRegisterStream(VirtualStream* stream);
            // @}



            /**
             * Read data from a stream in blocking mode.
             *
             * \param deadline - duration in microseconds from 'now'(the moment of request) when data must be
             * ready. Requests will be executed in such order so they don't miss the dead line. In case
             * two requests are both missing their dead lines, the priority will be used to determine the
             * execution order.
             * \param priority - used when we have two requests missing their deadlines.
             * \returns number of bytes read.
             */
            static SizeType Read(VirtualStream* stream, SizeType byteOffset, SizeType byteSize, void* dataBuffer, AZStd::chrono::microseconds deadline = ExecuteWhenIdle, Request::PriorityType priority = Request::PriorityType::DR_PRIORITY_NORMAL, Request::StateType* state = nullptr, const char* debugName = nullptr);

            /**
             * Read data from a stream in async mode.
             * IMPORTANT: The callback will be called from the streaming thread. It should be
             * thread safe and as quick as possible. Otherwise the streaming thread will be locked. This
             * will degrade performance. In such cases use ReadAsyncDeferred and pump ReceiveRequests.
             *
             * \param deadline - duration in microseconds from 'now'(the moment of request) when data must be
             * ready. Requests will be executed in such order so they don't miss the dead line. In case
             * two requests are both missing their dead lines, the priority will be used to determine the
             * execution order.
             * \param priority - used when we have two requests missing their deadlines.
             * \returns a request handle.
             *
             * \note If file is NOT registered it will be automatically for READING ONLY!
             */
            static RequestHandle ReadAsync(VirtualStream* stream, SizeType byteOffset, SizeType byteSize, void* dataBuffer, const Request::RequestDoneCB& callback, AZStd::chrono::microseconds deadline = ExecuteWhenIdle, Request::PriorityType priority = Request::PriorityType::DR_PRIORITY_NORMAL, const char* debugName = nullptr, bool deferCallback = false);
            /**
             * Read data from a stream in async mode.
             * The request callback will be deferred until you call ReceiveRequests. In that
             * sense the callback doesn't need to be thread safe
             * \ref ReadAsync
             */
            static RequestHandle ReadAsyncDeferred(VirtualStream* stream, SizeType byteOffset, SizeType byteSize, void* dataBuffer, const Request::RequestDoneCB& callback, AZStd::chrono::microseconds deadline = ExecuteWhenIdle, Request::PriorityType priority = Request::PriorityType::DR_PRIORITY_NORMAL, const char* debugName = nullptr);

            //@{ File base versions for reading. Prefer the use of Stream based functions, to avoid the extra CPU hit.
            SizeType Read(const char* fileName, SizeType byteOffset, SizeType& byteSize, void* dataBuffer, AZStd::chrono::microseconds deadline = ExecuteWhenIdle, Request::PriorityType priority = Request::PriorityType::DR_PRIORITY_NORMAL, Request::StateType* state = NULL, const char* debugName = nullptr);
            RequestHandle ReadAsync(const char* fileName, SizeType byteOffset, SizeType byteSize, void* dataBuffer, const Request::RequestDoneCB& callback, AZStd::chrono::microseconds deadline = ExecuteWhenIdle, Request::PriorityType priority = Request::PriorityType::DR_PRIORITY_NORMAL, const char* debugName = nullptr, bool deferCallback = false);
            RequestHandle ReadAsyncDeferred(const char* fileName, SizeType byteOffset, SizeType byteSize, void* dataBuffer, const Request::RequestDoneCB& callback, AZStd::chrono::microseconds deadline = ExecuteWhenIdle, Request::PriorityType priority = Request::PriorityType::DR_PRIORITY_NORMAL, const char* debugName = nullptr);
            //@}

            /**
             * Write data to a stream in a blocking mode.
             *
             * \param priority - in writing we use the priority only to determine write order between different
             * streams. For one stream write requests are always executed in order.
             * \returns number of bytes written
             *
             * \note Prefer the use of Stream based function, to avoid the extra CPU hit.
             * If file is NOT registered it will be automatically for WRITE ONLY! Be careful with reading/writing to the same file, make sure you
             * register the file data stream yourself with the proper parameters. This is the recommended way anyway.
             */
            static SizeType Write(VirtualStream* stream, const void* dataBuffer, SizeType byteSize, Request::PriorityType priority = Request::PriorityType::DR_PRIORITY_NORMAL, Request::StateType* state = NULL, SizeType byteOffset = EndOfStream, const char* debugName = nullptr);
            /**
             * Write data to a stream in async mode.
             * IMPORTANT: The callback will be called from the streaming thread. It should be
             * thread safe and as quick as possible. Otherwise the streaming thread will be locked. This
             * will degrade performance. In such cases use WriteAsyncDeferred and pump ReceiveRequests.
             *
             * \param priority - in writing we use the priority only to determine write order between different
             * streams. For one stream write requests are always executed in order.
             * \return a request handle.
             * \note If file is NOT registered it will be automatically for WRITE ONLY!
             */
            static RequestHandle WriteAsync(VirtualStream* stream, const void* dataBuffer, SizeType byteSize, const Request::RequestDoneCB& callback, Request::PriorityType priority = Request::PriorityType::DR_PRIORITY_NORMAL, SizeType byteOffset = EndOfStream, const char* debugName = nullptr, bool deferCallback = false);
            /**
             * Write data from a stream in async mode.
             * The request callback will be deferred until you call ReceiveRequests. In that
             * sense the callback doesn't need to be thread safe
             * \ref WriteAsync
             */
            static RequestHandle WriteAsyncDeferred(VirtualStream* stream, const void* dataBuffer, SizeType byteSize, const Request::RequestDoneCB& callback, Request::PriorityType priority = Request::PriorityType::DR_PRIORITY_NORMAL, SizeType byteOffset = EndOfStream, const char* debugName = nullptr);

            //@{ File base versions for writing. Prefer the use of Stream based functions, to avoid the extra CPU hit.
            SizeType Write(const char* fileName, const void* dataBuffer, SizeType byteSize, Request::PriorityType priority = Request::PriorityType::DR_PRIORITY_NORMAL, Request::StateType* state = NULL, SizeType byteOffset = EndOfStream, const char* debugName = nullptr);
            RequestHandle WriteAsync(const char* fileName, const void* dataBuffer, SizeType byteSize, const Request::RequestDoneCB& callback, Request::PriorityType priority = Request::PriorityType::DR_PRIORITY_NORMAL, SizeType byteOffset = EndOfStream, const char* debugName = nullptr, bool deferCallback = false);
            RequestHandle WriteAsyncDeferred(const char* fileName, const void* dataBuffer, SizeType byteSize, const Request::RequestDoneCB& callback, Request::PriorityType priority = Request::PriorityType::DR_PRIORITY_NORMAL, SizeType byteOffset = EndOfStream, const char* debugName = nullptr);
            //@}

            /// Cancel request in a blocking mode, function will NOT return until cancel is complete. Request callback will be called with Request::StateType == ST_CANCELLED.
            void CancelRequest(RequestHandle request);
            /// Cancel request in async mode, function returns instantly. Request callback will be called with Request::StateType == ST_CANCELLED. This function takes in an optional semaphore which will be forwared to the Device::CancelRequest method
            void CancelRequestAsync(RequestHandle request, AZStd::semaphore* sync = nullptr);

            /// Changes the scheduling parameters of the request.
            static void RescheduleRequest(RequestHandle request, Request::PriorityType priority, AZStd::chrono::microseconds deadline);

            /// Returns the estimated completion time from NOW is microseconds.
            static AZStd::chrono::microseconds EstimateCompletion(RequestHandle request);

            /**
             * Must be pumped to receive deferred request.
             */
            void ReceiveRequests();

            /// Finds a device that will handle a specific stream. This includes overlay/remap to other devices etc.
            Device*         FindDevice(const char* streamName);

            /// Return the value passed in the descriptor \ref Descriptor::m_maxNumOpenLimitedStream
            SizeType    GetMaxNumOpenLimitedStream() const;

        private:
            Streamer(const Descriptor& desc);
            ~Streamer();

            struct StreamerData*    m_data;
        };
    }
}
