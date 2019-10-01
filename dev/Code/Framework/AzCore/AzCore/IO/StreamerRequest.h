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
#include <AzCore/IO/CompressionBus.h>
#include <AzCore/IO/RequestPath.h>
#include <AzCore/IO/Streamer/StreamerConfiguration.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/std/chrono/clocks.h>
#include <AzCore/std/hash.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
namespace IO
{
    using StreamID = AZ::Uuid;   /// Sha1 of the stream name.
    static const StreamID       InvalidStreamID = AZ::Uuid::CreateNull();
    extern const AZStd::chrono::microseconds ExecuteWhenIdle;

    class Device;

    class Request
    {
    public:
        AZ_CLASS_ALLOCATOR(Request, ThreadPoolAllocator, 0)
        enum class StateType : u8;
        using SizeType = u64;
        using RequestDoneCB = ::AZStd::function<void(const AZStd::shared_ptr<class Request>& /*request handle*/, SizeType /* num bytes read */, void* /*buffer*/, StateType /* request state */)>;

        enum class OperationType : u8
        {
            NONE, //< No operation will happen/
            UTIL, //< Operation is handled outside the main processing loop. Util function are often processed while reading/decompression is happening.
            DIRECT_READ, //< File is directly read from disk (a.k.a. loose files).
            ARCHIVED_READ, //< File is part of an archive, but can be directly read.
            FULL_DECOMPRESSED_READ, //< File is compressed, but can be fully decompressed to the target buffer.
            PARTIAL_DECOMPRESSED_READ //< File is fully compressed, but only a subsection is needed for the output.
        };

        enum class StateType : u8
        {
            // The order of these enums is important for several functions such as HasCompleted and WaitingForProcessing.
            ST_PREPARING,
            ST_PENDING,
            ST_IN_PROCESS,
            ST_COMPLETED,
            ST_CANCELLED,
            ST_ERROR_FAILED_TO_OPEN_FILE,
            ST_ERROR_FAILED_IN_OPERATION,
        };

        enum class PriorityType : u8
        {
            // IMPORTANT: Order and size is important don't change it,
            DR_PRIORITY_CRITICAL = 0,
            DR_PRIORITY_ABOVE_NORMAL,
            DR_PRIORITY_NORMAL,
            DR_PRIORITY_BELOW_NORMAL,
        };

        Request(Device* device, RequestPath filename, SizeType byteOffset, SizeType byteSize, void* buffer, const RequestDoneCB& cb,
            bool isDeferredCB, PriorityType priority, OperationType op, const char* debugName);
        ~Request() = default;

        bool HasCompleted() const;
        bool IsPendingProcessing() const;

        void SetDeadline(::AZStd::chrono::microseconds deadline);
        SizeType GetStreamByteOffset() const;
        void AddDecompressionInfo(CompressionInfo info);

        //! Whether or not the operation is one of the read operations.
        bool IsReadOperation() const;
        //! Whether or not the operation needs to read from an archive.
        bool IsReadFromArchiveOperation() const;
        //! Whether or the operation requires a decompression step after completing.
        bool NeedsDecompression() const;

        RequestPath     m_filename;
#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
        //! The filename can be updated to the containing archive filename. This is store the original name for debugging and profiling purposes.
        AZStd::string   m_originalFilename;
#endif
        //! Every request is assigned an id to guarantee that assets that are processed on a first-come-first-served basis
        //! if they're otherwise equal. This also helps with debugging issues to uniquely identify a request.
        AZ::u64         m_requestId = 0;    
        Device*         m_device; //< Device this request runs on or will be running on.
        SizeType        m_byteOffset;
        SizeType        m_byteSize;
        void*           m_buffer;
        SizeType        m_bytesProcessedStart;
        SizeType        m_bytesProcessedEnd;
        SizeType        m_currentSeekPos;
        ::AZStd::chrono::system_clock::time_point  m_deadline;
        ::AZStd::chrono::system_clock::time_point  m_estimatedCompletion;
        RequestDoneCB   m_callback;
        AZStd::atomic<StateType> m_state;
        bool            m_isDeferredCB;
        PriorityType    m_priority;
        OperationType   m_operation;
        CompressionInfo m_compressionInfo;
        const char*     m_debugName;        //< Pointer to a request debug name.
    };
} // namespace IO
} // namespace AZ