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
#ifndef AZ_UNITY_BUILD

#include <AzCore/PlatformIncl.h>
#include <AzCore/IO/Device.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/StreamerUtil.h>
#include <AzCore/IO/StreamerDrillerBus.h>
#include <AzCore/IO/StreamerRequest.h>
#include <AzCore/IO/VirtualStream.h>
#include <AzCore/IO/CompressorStream.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/containers/array.h>

#include <AzCore/std/sort.h>

#include <AzCore/Math/Crc.h>
#include <AzCore/IO/Compressor.h>

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
        Device::Device(const AZStd::string& name, FileIOBase* ioBase, const AZStd::thread_desc* threadDesc, int threadSleepTimeMS)
            : m_name(name)
            , m_currentStream(NULL)
            , m_currentStreamOffset(0)
            , m_readCache(NULL)
            , m_shutdown_thread(false)
            , m_thread(AZStd::bind(&Device::ProcessRequests, this), threadDesc)
            , m_threadSleepTimeMS(threadSleepTimeMS)
            , m_cacheSize(128 * 1024)
            , m_maxOperationSize(m_cacheSize)
            , m_isStreamOffsetSupport(true)
            , m_layerOffsetLBA(SizeType(-1))
            , m_ioBase(ioBase)
        {
            DeviceRequestBus::Handler::BusConnect();
        }

        //=========================================================================
        // CreateCache
        // [12/9/2011]
        //=========================================================================
        void Device::CreateCache(unsigned int cacheBlockSize, unsigned int numCacheBlocks)
        {
            if (numCacheBlocks > 0)
            {
                m_cacheSize = cacheBlockSize;
                m_maxOperationSize = cacheBlockSize;
                m_readCache = aznew ReadCache(m_maxOperationSize, numCacheBlocks, this);
            }
        }
        //=========================================================================
        // ~Device
        // [10/2/2009]
        //=========================================================================
        Device::~Device()
        {
            DeviceRequestBus::Handler::BusDisconnect();
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_lock);
                m_shutdown_thread.store(true, AZStd::memory_order_release);
                m_addReqCond.notify_all();
            }
            m_thread.join();

            // Delete all requests
            AZ_Warning("IO", m_pending.empty(), "We have %d pending request(s)! Cancelling...", m_pending.size());
            while (!m_pending.empty())
            {
                Request* r = &m_pending.front();
                m_pending.pop_front();
                delete r;
            }

            AZ_Warning("IO", m_completed.empty(), "We have %d completed request(s)! Ignoring...", m_completed.size());
            while (!m_completed.empty())
            {
                Request* r = &m_completed.front();
                m_completed.pop_front();
                delete r;
            }

            // Delete open streams, we prefer the user to call unregister stream.
            AZ_Warning("IO", m_streams.empty(), "We have %d stream(s) still open! Closing...", m_streams.size());
            for (auto& streamPair : m_streams)
            {
                GenericStream* stream = streamPair.second->m_stream;
                (void)stream;
                AZ_TracePrintf("IO", "	Open Stream: %s\n", stream->GetFilename());
            }
            m_streams.clear();

            // Delete all left over compressors.
            for (size_t i = 0; i < m_compressors.size(); ++i)
            {
                delete m_compressors[i];
            }
            m_compressors.clear();

            delete m_readCache;
        }

        //=========================================================================
        // RegisterFileStream
        // [10/2/2009]
        //=========================================================================
        VirtualStream* Device::RegisterFileStream(const char* fileName, OpenMode flags, bool isLimited)
        {
            AZ_Assert(m_ioBase, "Attempting to create a file stream without a valid FileIOBase object");
            
            AZStd::array<char, AZ_MAX_PATH_LEN> resolvedPath;
            m_ioBase->ResolvePath(fileName, resolvedPath.data(), resolvedPath.size());
            StreamID id = Uuid::CreateName(resolvedPath.data());

            AZStd::lock_guard<decltype(m_lock)> lock(m_lock);
            StreamMap::pair_iter_bool iter = m_streams.insert_key(id);
            AZStd::unique_ptr<StreamData>& streamData = iter.first->second;

            if (iter.second)
            {
                HandleType ioHandle;
                if (m_ioBase->Open(fileName, flags, ioHandle) != ResultCode::Success)
                {
                    AZ_Error("IO", false, "Unable to register stream with filename %s. Another handle to the file may be opened", fileName);
                    m_streams.erase(iter.first);
                    return nullptr;
                }

                streamData = AZStd::make_unique<StreamData>(aznew VirtualStream(aznew CompressorStream(aznew FileIOStream(ioHandle, flags, true), true), true), isLimited);
                // Streams that should have an open limit on them will be closed if they are over limit
                SizeType maxLimitedStreams = GetStreamer()->GetMaxNumOpenLimitedStream();
                if (isLimited && maxLimitedStreams > 0)
                {
                    m_mruStreams.push_front(streamData->m_stream);
                    for (SizeType leastRecentUsedIndex = m_mruStreams.size() - 1; leastRecentUsedIndex >= maxLimitedStreams; --leastRecentUsedIndex)
                    {
                        m_mruStreams.back()->Close();
                        m_mruStreams.pop_back();
                    }
                }
                m_ownedStreams.insert(AZStd::unique_ptr<VirtualStream>(streamData->m_stream));

                EBUS_DBG_EVENT(StreamerDrillerBus, OnRegisterStream, streamData->m_stream, flags);
            }
            else
            {
                bool newCanWrite = (flags & (OpenMode::ModeWrite | OpenMode::ModeAppend | OpenMode::ModeUpdate)) != OpenMode::Invalid;
                (void)newCanWrite;
                AZ_Assert(streamData->m_stream->CanWrite() == newCanWrite, "Stream write flags don't match what was requested (new write flag=%d, stream current write flag()=%d)", (int)newCanWrite, (int)streamData->m_stream->CanWrite());
            }

            streamData->add_ref();
            return streamData->m_stream;
        }

        //=========================================================================
        // UnRegisterFileStream
        // [10/2/2009]
        //=========================================================================
        void Device::UnRegisterFileStream(const char* fileName)
        {
            AZ_Assert(m_ioBase, "A valid FileIOBase object is required to resolve the supplied filename");
            AZStd::array<char, AZ_MAX_PATH_LEN> resolvedPath;
            m_ioBase->ResolvePath(fileName, resolvedPath.data(), resolvedPath.size());
            UnRegisterStream(Uuid::CreateName(resolvedPath.data()));
        }
    

        //=========================================================================
        // RegisterStream
        // [8/29/2012]
        //=========================================================================
        void Device::RegisterStream(VirtualStream* stream, OpenMode flags, bool isLimited)
        {
            (void)flags;
            StreamID id = Uuid::CreateName(stream->GetFilename());
            AZStd::lock_guard<decltype(m_lock)> lock(m_lock);
            StreamMap::pair_iter_bool iter = m_streams.insert_key(id);
            AZStd::unique_ptr<StreamData>& streamData = iter.first->second;

            if (iter.second)
            {
                streamData = AZStd::make_unique<StreamData>(stream, isLimited);
                // Streams that should have an open limit on them will be closed if they are over limit
                SizeType maxLimitedStreams = GetStreamer()->GetMaxNumOpenLimitedStream();
                if (isLimited && maxLimitedStreams > 0)
                {
                    m_mruStreams.push_front(streamData->m_stream);
                    for (SizeType leastRecentUsedIndex = m_mruStreams.size() - 1; leastRecentUsedIndex >= maxLimitedStreams; --leastRecentUsedIndex)
                    {
                        m_mruStreams.back()->Close();
                        m_mruStreams.pop_back();
                    }
                }
                EBUS_DBG_EVENT(StreamerDrillerBus, OnRegisterStream, stream, flags);
            }
            streamData->add_ref();
        }


        //=========================================================================
        // UnRegisterStream
        // [8/29/2012]
        //=========================================================================
        void Device::UnRegisterStream(VirtualStream* stream)
        {
            AZ_Assert(stream, "Invalid stream");
            if (stream)
            {
                stream->Close();
            }

            UnRegisterStream(Uuid::CreateName(stream->GetFilename()));
        }

        void Device::UnRegisterStream(StreamID id)
        {
            AZStd::lock_guard<decltype(m_lock)> lock(m_lock);

            auto streamIter = m_streams.find(id);
            if (streamIter != m_streams.end())
            {
                AZStd::unique_ptr<StreamData>& streamData = streamIter->second;
                EBUS_DBG_EVENT(StreamerDrillerBus, OnUnregisterStream, streamData->m_stream);

                streamData->release();
                AZ_Assert(streamData->get_ref() >= 0, "Stream with file(%s) has been unregisted more times than it has bee registered", streamData->m_stream->GetFilename());
                if (streamData->get_ref() == 0)
                {
                    if (streamData->m_stream == m_currentStream)
                    {
                        m_currentStream = nullptr;
                        m_currentStreamOffset = 0;
                    }
                    //Need to make a lvalue unique pointer for lookup into the set and release the pointer after the lookup to prevent double deletion
                    AZStd::unique_ptr<VirtualStream> wrappedStream(streamData->m_stream);
                    m_ownedStreams.erase(wrappedStream);
                    wrappedStream.release();
                    if (m_readCache)
                    {
                        m_readCache->RemoveStream(streamData->m_stream);
                    }
                    m_mruStreams.erase(AZStd::remove(m_mruStreams.begin(), m_mruStreams.end(), streamData->m_stream), m_mruStreams.end());;
                    m_streams.erase(streamIter);
                }
            }
        }

        //=========================================================================
        // FindStream
        // [10/20/2009]
        //=========================================================================
        VirtualStream* Device::FindStream(StreamID streamId)
        {
            StreamMap::iterator iter = m_streams.find(streamId);
            if (iter != m_streams.end())
            {
                return iter->second->m_stream;
            }
            return nullptr;
        }

        //=========================================================================
        // Read
        // [10/5/2009]
        //=========================================================================
        void Device::Read(Request* request)
        {
            m_pending.push_back(*request);
            EBUS_DBG_EVENT(StreamerDrillerBus, OnAddRequest, request);
        }

        //=========================================================================
        // Write
        // [10/5/2009]
        //=========================================================================
        void Device::Write(Request* request)
        {
            if (!OpenStream(request->m_stream))
            {
                // or we should push to pending callback queue....
                CompleteRequest(request, Request::StateType::ST_ERROR_FAILED_TO_OPEN_STREAM);
                return;
            }
            
            if (request->m_byteOffset == SizeType(-1))
            {
                request->m_byteOffset = request->m_stream->VirtualWrite(request->m_byteSize);
                // We cannot seek to an offset within the compressed stream  while writing because the size of the compressed bytes are unknown until they are written to the stream
                request->m_currentSeekPos = request->m_byteOffset;
            }
            m_pending.push_back(*request);
            EBUS_DBG_EVENT(StreamerDrillerBus, OnAddRequest, request);
        }

        /*!
        \brief Opens the supplied stream and adjust the most recently used stream list
        \This function is only called from the ProcessRequest thread
        \param[in] stream File stream to unregister
        \return true if the stream was able to opened or is already opened
        */
        bool Device::OpenStream(GenericStream* stream)
        {
            StreamID id = Uuid::CreateName(stream->GetFilename());
            bool isLimited;
            {
                AZStd::lock_guard<decltype(m_lock)> lock(m_lock);
                auto streamDataIter = m_streams.find(id);
                isLimited = streamDataIter != m_streams.end() ? streamDataIter->second->m_isLimited : false;
            }
            // Open the stream if needed.
            if (!stream->IsOpen())
            {
                SizeType maxLimitedStreams = GetStreamer()->GetMaxNumOpenLimitedStream();
                if (isLimited && maxLimitedStreams > 0)
                {
                    AZStd::lock_guard<decltype(m_lock)> lock(m_lock);
                    // check the MRU list
                    m_mruStreams.push_front(stream);
                    for (SizeType leastRecentUsedIndex = m_mruStreams.size() - 1; leastRecentUsedIndex >= maxLimitedStreams; --leastRecentUsedIndex)
                    {
                        m_mruStreams.back()->Close();
                        m_mruStreams.pop_back();
                    }
                }

                if (!stream->ReOpen())
                {
                    return false;
                }
            }
            else
            {
                SizeType maxLimitedStreams = GetStreamer()->GetMaxNumOpenLimitedStream();
                if (isLimited && maxLimitedStreams > 0)
                {
                    AZStd::lock_guard<decltype(m_lock)> lock(m_lock);
                    // move it to the top if needed
                    if (m_mruStreams.empty())
                    {
                        m_mruStreams.push_front(stream);
                    }
                    else if (m_mruStreams.front() != stream)
                    {
                        m_mruStreams.erase(AZStd::remove(m_mruStreams.begin(), m_mruStreams.end(), stream), m_mruStreams.end());
                        m_mruStreams.push_front(stream);
                    }
                }
            }
            return true;
        }

        /*!
        \brief Performs the read on the stream attached to the request
        \param[in] request Handle to request object which contains request priority, current state and callback
        */
        void Device::ReadStream(Request* request)
        {
            VirtualStream* stream = request->m_stream;
            if (!OpenStream(stream))
            {
                // or we should push to pending callback queue....
                CompleteRequest(request, Request::StateType::ST_ERROR_FAILED_TO_OPEN_STREAM);
                return;
            }

            // Perform the requested operation.
            AZ::IO::SizeType bytesTransfered;
            AZ::IO::SizeType requestSize = request->m_byteSize - (request->m_bytesProcessedStart + request->m_bytesProcessedEnd);
            AZ::IO::SizeType bytesToProcess = AZStd::GetMin(requestSize, m_maxOperationSize);
            void* bufferDestination = static_cast<char*>(request->m_buffer) + request->m_bytesProcessedStart;
            AZ::IO::SizeType byteOffset = request->GetStreamByteOffset() + request->m_bytesProcessedStart;

            ReadCache::BlockInfo* cacheBlock;
            // When AcquireCacheBlock align the offset to blockSize only we will use physical read to fill the cache,
            // otherwise alignment is not needed and could reduce the read speed.
            if (m_readCache && (cacheBlock = m_readCache->AcquireCacheBlock(request, !stream->IsCompressed())) != NULL)
            {
                EBUS_DBG_EVENT(StreamerDrillerBus, OnRead, stream, m_readCache->m_blockSize, cacheBlock->m_offset);

                bytesTransfered = stream->ReadAtOffset(m_readCache->m_blockSize, cacheBlock->m_data, cacheBlock->m_offset); // read raw

                if (!m_readCache->ProcessFilledCacheBlock(cacheBlock, request, bytesTransfered))
                {
                    // tricky if we did not finish the request, but we read all the data! this is an error (leave bytesToProcess as is it will trigger later
                    // if not set bytesToProcess to bytesTransfered in case we split the request
                    if (bytesTransfered == m_readCache->m_blockSize)
                    {
                        bytesToProcess = bytesTransfered;  // avoid the possible error later
                    }
                }
            }
            else
            {
                EBUS_DBG_EVENT(StreamerDrillerBus, OnRead, stream, bytesToProcess, byteOffset);

                bytesTransfered = stream->ReadAtOffset(bytesToProcess, bufferDestination, byteOffset); // read raw

                request->m_bytesProcessedStart += bytesTransfered;
            }
            EBUS_DBG_EVENT(StreamerDrillerBus, OnReadComplete, stream, bytesTransfered);


            // update current stream
            m_currentStream = stream;
            m_currentStreamOffset = byteOffset + bytesTransfered;

            // decompressor
            if ((request->m_bytesProcessedStart + request->m_bytesProcessedEnd) == request->m_byteSize)
            {
                CompleteRequest(request);
            }
            else if (bytesTransfered != bytesToProcess)
            {
                CompleteRequest(request, Request::StateType::ST_ERROR_FAILED_IN_OPERATION);
            }
            else
            {
                // if we have not finished the current request push in back for reschedule
                request->m_state = Request::StateType::ST_PENDING;
                m_pending.push_back(*request);
            }
        }

        /*!
        \brief Performs the write on the stream attached to the request
        \param[in] request Handle to request object which contains request priority, current state and callback
        */
        void Device::WriteStream(Request* request)
        {
            VirtualStream* stream = request->m_stream;
            if (!OpenStream(stream))
            {
                // or we should push to pending callback queue....
                CompleteRequest(request, Request::StateType::ST_ERROR_FAILED_TO_OPEN_STREAM);
                return;
            }

            // Perform the requested operation.
            AZ::IO::SizeType bytesTransfered;
            AZ::IO::SizeType requestSize = request->m_byteSize - (request->m_bytesProcessedStart + request->m_bytesProcessedEnd);
            AZ::IO::SizeType bytesToProcess = AZStd::GetMin(requestSize, m_maxOperationSize);
            void* bufferDestination = static_cast<char*>(request->m_buffer) + request->m_bytesProcessedStart;
            AZ::IO::SizeType byteOffset = request->GetStreamByteOffset() + request->m_bytesProcessedStart;

            EBUS_DBG_EVENT(StreamerDrillerBus, OnWrite, stream, bytesToProcess, byteOffset);

            if (!stream->IsCompressed() && request->m_currentSeekPos != static_cast<SizeType>(-1))
            {
                bytesTransfered = stream->WriteAtOffset(bytesToProcess, bufferDestination, request->m_currentSeekPos);
                request->m_currentSeekPos = stream->GetCurPos();
            }
            else
            {
                stream->Seek(0U, GenericStream::SeekMode::ST_SEEK_END);
                bytesTransfered = stream->Write(bytesToProcess, bufferDestination);
            }

            request->m_bytesProcessedStart += bytesTransfered;
            EBUS_DBG_EVENT(StreamerDrillerBus, OnWriteComplete, stream, bytesTransfered);


            // update current stream
            m_currentStream = stream;
            m_currentStreamOffset = byteOffset + bytesTransfered;

            // decompressor
            if ((request->m_bytesProcessedStart + request->m_bytesProcessedEnd) == request->m_byteSize)
            {
                CompleteRequest(request);
            }
            else if (bytesTransfered != bytesToProcess)
            {
                CompleteRequest(request, Request::StateType::ST_ERROR_FAILED_IN_OPERATION);
            }
            else
            {
                // if we have not finished the current request push in back for reschedule
                request->m_state = Request::StateType::ST_PENDING;
                m_pending.push_back(*request);
            }
        }

        //=========================================================================
        // Complete Request
        // [11/18/2011]
        //=========================================================================
        void Device::CompleteRequest(Request* request, Request::StateType state, bool isLocked)
        {
            EBUS_DBG_EVENT(StreamerDrillerBus, OnCompleteRequest, request, state);

            if (!request->m_isDeferredCB)
            {
                // if this is too long this will hold the device thread... use lockless or a separate lock
                if (request->m_callback)
                {
                    request->m_callback(request, request->m_bytesProcessedStart + request->m_bytesProcessedEnd, request->m_buffer, state);
                }
                delete request;
            }
            else
            {
                request->m_state = state;
                request->m_stream = nullptr;

                if (!isLocked)
                {
                    m_lock.lock();
                }
                m_completed.push_back(*request);
                if (!isLocked)
                {
                    m_lock.unlock();
                }
            }
        }

        /*!
        \brief Invokes the callbacks of completed request
        */
        void Device::InvokeCompletedCallbacks()
        {
            if (m_completed.empty())
            {
                m_lock.lock();
                Request::ListType completedRequest = AZStd::move(m_completed);
                m_lock.unlock();
                for (Request& request : completedRequest)
                {
                    if (request.m_callback)
                    {
                        request.m_callback(&request, request.m_bytesProcessedStart + request.m_bytesProcessedEnd, request.m_buffer, request.m_state);
                    }
                    delete &request;
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
                if (m_shutdown_thread.load(AZStd::memory_order_acquire))
                {
                    break;
                }

                {
                    AZStd::unique_lock<AZStd::recursive_mutex> lock(m_lock);
                    //Events are only added to queue using this mutex, so no issues with the queue being modified
                    if (DeviceRequestBus::QueuedEventCount() == 0 && m_pending.empty())
                    {
                        m_addReqCond.wait(lock); // Wait for commands
                    }
                    // execute commands
                    DeviceRequestBus::ExecuteQueuedEvents();

                    if (m_pending.empty())
                    {
                        continue;
                    }
                }

                // schedule requests
                ScheduleRequests();

                if (m_pending.empty())
                {
                    continue;                   // if we have read cache hits during scheduling, there may be no more requests.
                }
                Request* request = &m_pending.front();
                m_pending.pop_front();
                request->m_state = Request::StateType::ST_IN_PROCESS;
                if (request->m_operation == Request::OperationType::OT_READ)
                {
                    ReadStream(request);
                }
                else
                {
                    WriteStream(request);
                }


                // Pause the device thread if necessary.
                if (m_threadSleepTimeMS > -1)
                {
                    AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(m_threadSleepTimeMS));
                }
            }
        }

        /*!
        \brief Opens a file using the fileName and flags parameter and registers the Virtual File Stream to this device
        \param[in] request Handle to request object which contains request priority, current state and callback
        \param[in] fileName filename to open using this device
        \param[in] flags Flags used to open a stream with name @fileName
        \param[in] isLimited Should the registered stream count as a limited stream for the max number of streams to be opened
        \param[in] sync optional semaphore which is released after the request has been serviced
        */
        void Device::RegisterFileStreamRequest(Request* request, const char* fileName, OpenMode flags, bool isLimited, AZStd::semaphore* sync)
        {
            request->m_stream = RegisterFileStream(fileName, flags, isLimited);

            CompleteRequest(request);
            if (sync)
            {
                sync->release();
            }
        }

        /*!
        \brief Opens a file using the fileName and flags parameter and registers the Virtual File Stream to this device
        \param[in] request Handle to request object which contains request priority, current state and callback
        \param[in] fileName filename used
        \param[in] sync optional semaphore which is released after the request has been serviced
        */
        void Device::UnRegisterFileStreamRequest(Request* request, const char* fileName, AZStd::semaphore* sync)
        {
            UnRegisterFileStream(fileName);
            CompleteRequest(request);
            if (sync)
            {
                sync->release();
            }
        }

        /*!
        \brief Registers a Virtual File Stream to this device
        \param[in] request Handle to request object which contains request priority, current state and callback
        \param[in] stream File stream to register with device
        \param[in] flags Flags used to open this stream
        \param[in] sync optional semaphore which is released after the request has been serviced
        */
        void Device::RegisterStreamRequest(Request* request, VirtualStream* stream, OpenMode flags, bool isLimited, AZStd::semaphore* sync)
        {
            RegisterStream(stream, flags, isLimited);
            CompleteRequest(request);
            if (sync)
            {
                sync->release();
            }
        }

        /*!
        \brief Unregisters Virtual File Stream from this device
        \param[in] request Handle to request object which contains request priority, current state and callback
        \param[in] stream File stream to unregister
        \param[in] sync optional semaphore which is released after the request has been serviced
        */
        void Device::UnRegisterStreamRequest(Request* request, VirtualStream* stream, AZStd::semaphore* sync)
        {
            UnRegisterStream(stream);
            CompleteRequest(request);
            if (sync)
            {
                sync->release();
            }
        }

        /*!
        \brief Queues a Read Request for processing on the Device RegisterRequest thread
        \param[in] request Handle to request object which contains request priority, current state and callback
        \param[in] stream Stream object to use for read request. If it is nullptr then the streamID will be used to lookup a registered stream
        \param[in] streamID ID(UUID of the filename used to open the stream) which is used to lookup registered streams on this device if the supplied @stream param is nullptr
        \param[in] sync optional semaphore which is released after the request has been serviced
        */
        void Device::ReadRequest(Request* request, VirtualStream* stream, const char* filename, AZStd::semaphore* sync)
        {
            if (stream == nullptr)
            {
                AZ_Assert(m_ioBase, "Attempting to resolve path to filename withotu valid FileIOBase object");

                AZStd::array<char, AZ_MAX_PATH_LEN> resolvedPath;
                m_ioBase->ResolvePath(filename, resolvedPath.data(), resolvedPath.size());
                StreamID streamID = Uuid::CreateName(resolvedPath.data());
                stream = FindStream(streamID);
                AZ_Warning("IO", stream, "Failed to find stream id: %x in the data streamer!", streamID);
                if (stream == nullptr)
                {
                    CompleteRequest(request, Request::StateType::ST_ERROR_FAILED_TO_OPEN_STREAM, true);
                    return;
                }
            }
            request->m_stream = stream;
            Read(request);

            if (sync)
            {
                sync->release();
            }
        }

        /*!
        \brief Queues a Write Request for processing on the Device RegisterRequest thread
        \param[in] request Handle to request object which contains request priority, current state and callback
        \param[in] stream Stream object to use for read request. If it is nullptr then the streamID will be used to lookup a registered stream
        \param[in] streamID ID(UUID of the filename used to open the stream) which is used to lookup registered streams on this device if the supplied @stream param is nullptr
        \param[in] sync optional semaphore which is released after the request has been serviced
        */
        void Device::WriteRequest(Request* request, VirtualStream* stream, const char* filename, AZStd::semaphore* sync)
        {
            if (stream == nullptr)
            {
                AZ_Assert(m_ioBase, "Attempting to resolve path to filename withotu valid FileIOBase object");

                AZStd::array<char, AZ_MAX_PATH_LEN> resolvedPath;
                m_ioBase->ResolvePath(filename, resolvedPath.data(), resolvedPath.size());
                StreamID streamID = Uuid::CreateName(resolvedPath.data());
                stream = FindStream(streamID);
                AZ_Warning("IO", stream, "Failed to find stream id: %x in the data streamer!", streamID);
                if (stream == nullptr)
                {
                    CompleteRequest(request, Request::StateType::ST_ERROR_FAILED_TO_OPEN_STREAM, true);
                    return;
                }
            }
            request->m_stream = stream;
            Write(request);

            if (sync)
            {
                sync->release();
            }
        }

        //=========================================================================
        // CancelRequest
        // [10/5/2009]
        //=========================================================================
        void Device::CancelRequest(Request* request, AZStd::semaphore* sync)
        {
            AZ_Assert(request != InvalidRequestHandle, "Invalid request handle!");
            EBUS_DBG_EVENT(StreamerDrillerBus, OnCancelRequest, request);
            if (request->m_state == Request::StateType::ST_PENDING)
            {
                m_pending.erase(*request);
            }
            request->m_state = Request::StateType::ST_CANCELLED;
            request->m_callback(request, request->m_bytesProcessedStart + request->m_bytesProcessedEnd, request->m_buffer, Request::StateType::ST_CANCELLED);
            delete request;

            if (sync)
            {
                sync->release();
            }
        }

        //=========================================================================
        // RegisterCompressor
        // [12/13/2012]
        //=========================================================================
        bool Device::RegisterCompressor(Compressor* compressor)
        {
            AZ_Assert(compressor, "Invalid input!");
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_lock);
            for (size_t i = 0; i < m_compressors.size(); ++i)
            {
                if (m_compressors[i]->GetTypeId() == compressor->GetTypeId())
                {
                    return false;
                }
            }
            m_compressors.push_back(compressor);
            return true;
        }

        //=========================================================================
        // UnRegisterCompressor
        // [12/13/2012]
        //=========================================================================
        bool Device::UnRegisterCompressor(Compressor* compressor)
        {
            AZ_Assert(compressor, "Invalid input!");
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_lock);
            CompressorArray::iterator it = AZStd::find(m_compressors.begin(), m_compressors.end(), compressor);
            if (it != m_compressors.end())
            {
                m_compressors.erase(it);
                return true;
            }

            return false;
        }

        //=========================================================================
        // FindCompressor
        // [12/13/2012]
        //=========================================================================
        Compressor* Device::FindCompressor(AZ::u32 compressorId)
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_lock);
            for (size_t i = 0; i < m_compressors.size(); ++i)
            {
                if (m_compressors[i]->GetTypeId() == compressorId)
                {
                    return m_compressors[i];
                }
            }

            return NULL;
        }


        //=========================================================================
        // ReadCache
        // [8/29/2012]
        //=========================================================================
        ReadCache::ReadCache(Streamer::SizeType blockSize, size_t numBlocks, Device* device)
        {
            AZ_Assert(device != NULL, "You must provide a valid device!");
            AZ_Assert(blockSize > 0 && numBlocks > 0, "We need blockSize > 0 and numBlocks >= 1!");
            AZ_Assert((blockSize % device->m_sectorSize) == 0, "blockSize must be a multiple of the sector size (block=%u, sector=%u!", (unsigned int)blockSize, (unsigned int)device->m_sectorSize);
            m_blockInfo.get_allocator().set_name("IO-ReadCache");

            m_freeIndex = 0;
            m_blockSize = blockSize;
            m_device = device;
#if defined(AZ_PLATFORM_WINDOWS)
            m_bufferAlignRequire = m_device->m_sectorSize;
#else
            m_bufferAlignRequire = 8;
#endif
            m_blockInfo.resize(numBlocks);
            char* cacheData = static_cast<char*>(azmalloc(static_cast<size_t>(blockSize * numBlocks), static_cast<size_t>(m_device->m_sectorSize), AZ::SystemAllocator, "IO-ReadCache"));
            for (size_t i = 0; i < m_blockInfo.size(); ++i)
            {
                m_blockInfo[i].m_stream = NULL;
                m_blockInfo[i].m_offset = 0;
                m_blockInfo[i].m_dataSize = 0;
                m_blockInfo[i].m_data = cacheData + i * m_blockSize;
            }
        }

        //=========================================================================
        // ~ReadCache
        // [8/29/2012]
        //=========================================================================
        ReadCache::~ReadCache()
        {
            azfree(m_blockInfo[0].m_data, AZ::SystemAllocator, static_cast<size_t>(m_blockSize * m_blockInfo.size()), static_cast<size_t>(m_device->m_sectorSize));
        }

        //=========================================================================
        // RemoveStream
        // [8/29/2012]
        //=========================================================================
        void ReadCache::RemoveStream(GenericStream* stream)
        {
            for (size_t i = 0; i < m_blockInfo.size(); ++i)
            {
                BlockInfo& bi = m_blockInfo[i];
                if (bi.m_stream == stream)
                {
                    bi.m_stream = NULL;
                }
            }
        }

        bool ReadCache::IsInCache(Request* request)
        {
            for (size_t i = 0; i < m_blockInfo.size(); ++i)
            {
                if (m_blockInfo[i].m_stream == request->m_stream)
                {
                    if (ProcessCacheBlock(&m_blockInfo[i], request))
                    {
                        return true;
                    }
                }
            }
            return false;
        }

        //=========================================================================
        // IsInCacheUserThread
        // [8/29/2012]
        //=========================================================================
        bool ReadCache::IsInCacheUserThread(GenericStream* stream, Streamer::SizeType& requestStartStrmOffset, Streamer::SizeType& requestEndStrmOffset, void* requestDataBuffer)
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_device->m_lock);

            Streamer::SizeType requestProcessed = 0;

            for (size_t i = 0; i < m_blockInfo.size(); ++i)
            {
                if (m_blockInfo[i].m_stream == stream)
                {
                    void* buffer = reinterpret_cast<char*>(requestDataBuffer) + requestProcessed;
                    Streamer::SizeType processed = 0;
                    if (ProcessCacheBlock(&m_blockInfo[i], requestStartStrmOffset, requestEndStrmOffset, buffer, processed))
                    {
                        requestStartStrmOffset += processed;
                        requestProcessed += processed;
                    }
                    else
                    {
                        requestEndStrmOffset -= processed;
                    }

                    if (requestStartStrmOffset == requestEndStrmOffset)
                    {
                        return true;
                    }
                }
            }
            return false;
        }

        //=========================================================================
        // AcquireCacheBlock
        // [8/29/2012]
        //=========================================================================
        ReadCache::BlockInfo*   ReadCache::AcquireCacheBlock(Request* request, bool isAlignOffsetToBlockSize)
        {
            Streamer::SizeType byteOffset = request->GetStreamByteOffset() + request->m_bytesProcessedStart;
            Streamer::SizeType byteSize = request->m_byteSize - (request->m_bytesProcessedStart + request->m_bytesProcessedEnd);

            if ((byteOffset & (m_blockSize - 1)) == 0 && // offset in block size
                byteSize >= m_blockSize && // we read in m_blockSize
                (((size_t)request->m_buffer + request->m_bytesProcessedStart) & (m_bufferAlignRequire - 1)) == 0)
            {
                // no caching needed, we are aligned for unbuffered read and we have >=
                // than m_blockSize to read
                return NULL;
            }

            m_device->m_lock.lock();
            BlockInfo& bi = m_blockInfo[m_freeIndex];
            bi.m_stream = request->m_stream;
            if (isAlignOffsetToBlockSize)
            {
                bi.m_offset = byteOffset & -(AZ::s64)m_blockSize; // round down
            }
            else
            {
                bi.m_offset = byteOffset; // don't round down
            }
            bi.m_dataSize = 0;

            m_freeIndex++;
            if (m_freeIndex == m_blockInfo.size()) // wrap around
            {
                m_freeIndex = 0;
            }
            m_device->m_lock.unlock();
            return &bi;
        }

        //=========================================================================
        // ProcessCacheBlock
        // [8/29/2012]
        //=========================================================================
        inline bool ReadCache::ProcessCacheBlock(BlockInfo* bi, Streamer::SizeType requestStartStrmOffset, Streamer::SizeType requestEndStrmOffset, void* requestDataBuffer, Streamer::SizeType& requestProcessed)
        {
            Streamer::SizeType blockStart = bi->m_offset;
            Streamer::SizeType blockEnd = bi->m_offset + bi->m_dataSize;

            if (blockStart < requestEndStrmOffset && blockEnd > requestStartStrmOffset /*&& no request inside the block */)
            {
                size_t copyOffsetStart = 0;
                size_t copyOffsetEnd = bi->m_dataSize;

                size_t requestCopyOffset = 0;

                if (blockStart < requestStartStrmOffset)
                {
                    copyOffsetStart = static_cast<size_t>(requestStartStrmOffset - blockStart);
                }
                else
                {
                    requestCopyOffset = static_cast<size_t>(blockStart - requestStartStrmOffset);
                }

                if (blockEnd >= requestEndStrmOffset)
                {
                    copyOffsetEnd -= static_cast<size_t>(blockEnd - requestEndStrmOffset);
                }
                else if (requestCopyOffset > 0)  // it we have the case the cache block is within the request, we can't use it (since we need to split the request into 2)
                {
                    return false;
                }

                requestProcessed = copyOffsetEnd - copyOffsetStart;
                memcpy(static_cast<char*>(requestDataBuffer) + requestCopyOffset, bi->m_data + copyOffsetStart, static_cast<size_t>(requestProcessed));

                if (requestCopyOffset)
                {
                    return false; // copy at the end
                }
                else
                {
                    return true; // copy at the start
                }
            }
            else
            {
                requestProcessed = 0;
                return true;
            }
        }

        //=========================================================================
        // ProcessCacheBlock
        // [8/29/2012]
        //=========================================================================
        inline bool ReadCache::ProcessCacheBlock(BlockInfo* bi, Request* request)
        {
            Streamer::SizeType requestEnd = request->GetStreamByteOffset() + (request->m_byteSize - request->m_bytesProcessedEnd);
            Streamer::SizeType requestStart = request->GetStreamByteOffset() + request->m_bytesProcessedStart;
            void* requestData = static_cast<char*>(request->m_buffer) + request->m_bytesProcessedStart;
            Streamer::SizeType processed = 0;
            if (ProcessCacheBlock(bi, requestStart, requestEnd, requestData, processed))
            {
                request->m_bytesProcessedStart += processed;
            }
            else
            {
                request->m_bytesProcessedEnd += processed;
            }

            if (request->m_byteSize == (request->m_bytesProcessedStart + request->m_bytesProcessedEnd))
            {
                return true;
            }

            return false;
        }
    } // namespace IO
} // namespace AZ
#endif // #ifndef AZ_UNITY_BUILD
