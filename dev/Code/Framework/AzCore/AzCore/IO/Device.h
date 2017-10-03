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
#ifndef AZCORE_DEVICE_H
#define AZCORE_DEVICE_H

#include <AzCore/IO/DeviceEventBus.h>
#include <AzCore/IO/StreamerRequest.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/parallel/semaphore.h>
#include <AzCore/std/parallel/conditional_variable.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

namespace AZ
{
    namespace IO
    {
        class ReadCache;
        class WriteCache;
        class Compressor;
        class VirtualStream;
        class FileIOBase;
        /**
         * Interface for interacting with a device.
         */
        class Device
            : public DeviceRequestBus::Handler
        {
            friend class ReadCache;

        public:
            using SizeType = AZ::u64;

            enum Type
            {
                DT_OPTICAL_MEDIA,
                DT_MAGNETIC_DRIVE,
                DT_FLASH_DRIVE,
                DT_MEMORY_DRIVE,
                DT_NETWORK_DRIVE,
            };

            /**
             * \param threadSleepTimeMS use with EXTREME caution, it will if != -1 it will insert a sleep after every read
             * to the device. This is used for internal performance tweaking in rare cases.
             */
            Device(const AZStd::string& name, FileIOBase* ioBase,  const AZStd::thread_desc* threadDesc = nullptr, int threadSleepTimeMS = -1);
            virtual ~Device();

            const AZStd::string&    GetName() const             { return m_name; }

            SizeType                GetSectorSize() const       { return m_sectorSize; }

            ReadCache*              GetReadCache() const { return m_readCache; } //> Retrieves the ReadCache

            /**
             * Register a compressor for the device. Compressor will be called/used from a
             * thread so the user should not use it or make it thread safe. The device assumes
             * ownership of the compressor (it will be deleted automatically if not Unregistered)
             * \returns true if compressor was registered, otherwise false (if compressor of this type Compressor::TypeId
             * was already registered.
             */
            bool                    RegisterCompressor(Compressor* compressor);
            /// Unregisters compressor (so it will not be automatically deleted). Returns false if compressor can't be found, otherwise true.
            bool                    UnRegisterCompressor(Compressor* compressor);
            /// Find compressor by id.
            Compressor*             FindCompressor(AZ::u32 compressorId);
            /// Return number of registered compressors.
            inline unsigned int     GetNumCompressors()
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_lock);
                return static_cast<unsigned int>(m_compressors.size());
            }

            /// Implementation of DeviceRequestBus
            void RegisterFileStreamRequest(Request* request, const char* filename, OpenMode flags, bool isLimited, AZStd::semaphore* sync) override;
            void UnRegisterFileStreamRequest(Request* request, const char* filename, AZStd::semaphore* sync) override;
            void RegisterStreamRequest(Request* request, VirtualStream* stream, OpenMode flags, bool isLimited, AZStd::semaphore* sync) override;
            void UnRegisterStreamRequest(Request* request, VirtualStream* stream, AZStd::semaphore* sync) override;
            void ReadRequest(Request* request, VirtualStream* stream, const char* filename, AZStd::semaphore* sync) override;
            void WriteRequest(Request* request, VirtualStream* stream, const char* filename, AZStd::semaphore* sync) override;
            void CancelRequest(Request* request, AZStd::semaphore* sync) override;

            /// Invokes completed request callbacks and clears the completed callback list
            void InvokeCompletedCallbacks();

            template<typename ...FuncArgs, typename ...Args>
            inline void AddCommand(void (DeviceRequest::* func)(FuncArgs...), Args&&... args)
            {
                //Need to create AZStd::result_of and AZStd::declval type trait
                AZ_STATIC_ASSERT((AZStd::is_void<typename std::result_of<decltype(func)(DeviceRequest*, Args...)>::type>::value), "Argument cannot be bound to supplied device function argument");
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_lock);
                DeviceRequestBus::QueueBroadcast(func, AZStd::forward<Args>(args)...);
                m_addReqCond.notify_all();
            }

        protected:
            struct StreamData;
            /// Creates the device caches. Should be called before the device thread has started.
            void CreateCache(unsigned int cacheBlockSize, unsigned int numCacheBlocks);
            /// Add command for the device thread. This function is thread safe.

            //////////////////////////////////////////////////////////////////////////
            // Commands, called from the device thread with the m_lock locked.
            VirtualStream*          RegisterFileStream(const char* fileName, OpenMode flags, bool isLimited = false);
            void                    UnRegisterFileStream(const char* fileName);


            void                    RegisterStream(VirtualStream* stream, OpenMode flags = OpenMode::Invalid, bool isLimited = false);
            void                    UnRegisterStream(VirtualStream* stream);
            void                    UnRegisterStream(StreamID streamId);

            VirtualStream*          FindStream(StreamID streamId);

            void                    Read(Request* request);
            void                    Write(Request* request);
            //////////////////////////////////////////////////////////////////////////

            bool                    OpenStream(GenericStream* stream);
            void                    ReadStream(Request* request);
            void                    WriteStream(Request* request);

            //////////////////////////////////////////////////////////////////////////
            /// DEVICE THREAD EXCLUSIVE DATA

            /// Schedule all requests.
            virtual void            ScheduleRequests() = 0;

            /// Device thread "main" function"
            void ProcessRequests();

            /// Complete a request. isLocked refers to m_lock mutex, if true we don't need to lock it inside the function.
            void CompleteRequest(Request* request, Request::StateType state = Request::StateType::ST_COMPLETED, bool isLocked = false);


            struct StreamData
            {
                StreamData() : StreamData(nullptr, false) {};
                StreamData(VirtualStream* stream, bool isLimited) : m_stream(stream), m_isLimited(isLimited), m_refCount(0) {}
                void add_ref() { m_refCount.fetch_add(1, AZStd::memory_order_acq_rel); }
                void release() { m_refCount.fetch_sub(1, AZStd::memory_order_acq_rel); }
                s32 get_ref() const { return m_refCount.load(AZStd::memory_order_acquire); }

                VirtualStream* m_stream;
                bool m_isLimited;
                AZStd::atomic<s32> m_refCount;
            };

            Type                        m_type;
            AZStd::string               m_name;                     ///< Device name (doesn't change while running so it's safe to read from multiple threads.
            Request::ListType           m_completed;                ///< List of completed requests for this device. (must be accessed with the m_lock (locked)
                                                                    // TODO rbbaklov: changed this from AZStd::mutex to support Linux (investigate how it was working before)
            AZStd::recursive_mutex          m_lock;                     ///< Device lock, used to sync all shared resources in the device.
            using StreamMap = AZStd::unordered_map<StreamID, AZStd::unique_ptr<StreamData>>; // Stream ID to steam map
            StreamMap                   m_streams;                  ///< Map with all streams. Used from the Device thread only.
            AZStd::unordered_set<AZStd::unique_ptr<VirtualStream>>  m_ownedStreams;                  ///< Streams created and owned by this device
          
            using StreamMRUList = AZStd::deque<GenericStream*>; // Stream MRU Container that supports queue like behavior of push_back/pop_front as well as delete from front
            StreamMRUList               m_mruStreams;               ///< MRU (most recently used) streams list array. Used when we specify the stream as resource limited.

            typedef AZStd::vector<Compressor*> CompressorArray;
            CompressorArray             m_compressors;

            GenericStream*              m_currentStream;            ///< Pointer to current stream.
            SizeType                    m_currentStreamOffset;      ///< Current stream offset. m_currentStream->m_diskOffset + m_currentStreamOffset will give you the position of head.

            ReadCache*                  m_readCache;                ///< Pointer to optional read cache.

            Request::ListType           m_pending;                  ///< List of pending request for this device.

            // TODO rbbaklov: changed this from AZStd::mutex to support Linux (investigate how it was working before)
            AZStd::condition_variable_any   m_addReqCond;

            AZStd::atomic_bool          m_shutdown_thread;
            AZStd::thread               m_thread;
            int                         m_threadSleepTimeMS;        ///< Time to sleep after each device request in milliseconds. use -1 (default) to ignore.

            SizeType                    m_eccBlockSize;             ///< ECC (error correction code) block size.
            SizeType                    m_cacheSize;                ///< Cache size.
            SizeType                    m_sectorSize;               ///< Sector size in bytes.
            SizeType                    m_maxOperationSize;         ///< Maximum byte size for a single operation (read or write).
            bool                        m_isStreamOffsetSupport;    ///< True if streams have diskOffset value set, false otherwise.

            SizeType                    m_layerOffsetLBA;           ///< Layer 1 offset in LBA. SizeType(-1) if not supported.
            FileIOBase*                 m_ioBase;                   ///< Pointer to FileIOBase instance used for accessing IO
        };

        /**
         * Implements a simple read cache, in addition we implement a buffered read.
         * This ensures a few things:
         *      - We read data in \ref m_blockSize, which caches small read request (read ahead)
         *      - We request data (read) from the device on \ref m_blockSize offsets (which is a multiple of the m_sectorSize),
         *      this allows us to use UNBUFFERED system reads (like: FILE_FLAG_NO_BUFFERING on windows)
         */
        class ReadCache
        {
        public:
            AZ_CLASS_ALLOCATOR(ReadCache, SystemAllocator, 0);

            SizeType      m_blockSize;    ///< Cache block size in bytes (must be multiple of the \ref m_sectorSize)
            SizeType      m_bufferAlignRequire;   ///< Alignment requirement for the destination buffer

            struct BlockInfo
            {
                GenericStream*             m_stream;
                SizeType  m_offset;
                char*               m_data;
                size_t              m_dataSize;
            };
            AZStd::vector<BlockInfo>    m_blockInfo;
            size_t                      m_freeIndex;    //< next cache block to grab. we can implement MRU etc.
            Device*                     m_device;

            ReadCache(SizeType blockSize, size_t numBlocks, Device* device);
            ~ReadCache();

            void RemoveStream(GenericStream* stream);

            bool IsInCache(Request* request);

            // called from user thread only
            bool IsInCacheUserThread(GenericStream* stream, SizeType& requestStartStrmOffset, SizeType& requestEndStrmOffset, void* requestDataBuffer);

            /// Return a cache block to be filled for this request if it's needed. Called from the streaming thread only.
            BlockInfo*  AcquireCacheBlock(Request* request, bool isAlignOffsetToBlockSize);

            inline bool ProcessFilledCacheBlock(BlockInfo* bi, Request* request, SizeType dataTransferred)
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_device->m_lock);
                bi->m_dataSize = (size_t)dataTransferred;
                return ProcessCacheBlock(bi, request);
            }

            // return true if we process from the start or false if we process at the end.
            bool ProcessCacheBlock(BlockInfo* bi, SizeType requestStartStrmOffset, SizeType requestEndStrmOffset, void* requestDataBuffer, SizeType& requestProcessed);
            bool ProcessCacheBlock(BlockInfo* bi, Request* request);
        };

        class WriteCache
        {
        public:
            AZ_CLASS_ALLOCATOR(WriteCache, SystemAllocator, 0);
        };
    }
}

#endif // AZCORE_DEVICE_H
AZCORE_DEVICE_H
