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
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/std/containers/intrusive_list.h>

#include <AzCore/std/chrono/clocks.h>

namespace AZ
{
namespace IO
{
    class VirtualStream;
    
    using StreamID = AZ::Uuid;   /// Sha1 of the stream name.
    static const StreamID       InvalidStreamID = AZ::Uuid::CreateNull();
    extern const AZStd::chrono::microseconds ExecuteWhenIdle;

    class Request : public ::AZStd::intrusive_list_node<Request>
    {
    public:
        AZ_CLASS_ALLOCATOR(Request, ThreadPoolAllocator, 0)
        enum class StateType : u8;
        using SizeType = u64;
        using RequestDoneCB = ::AZStd::function<void(Request* /*request handle*/, SizeType /* num bytes read */, void* /*buffer*/, StateType /* request state */)>;

        enum class OperationType : u8
        {
            OT_READ = 0,
            OT_WRITE,
        };

        enum class StateType : u8
        {
            ST_PENDING,
            ST_IN_PROCESS,
            ST_COMPLETED,
            ST_CANCELLED,
            ST_ERROR_FAILED_TO_OPEN_STREAM,
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

        Request(SizeType byteOffset, SizeType byteSize, void* buffer, const RequestDoneCB& cb, bool isDeferredCB, PriorityType priority, OperationType op, const char* debugName);
        ~Request();

        void SetDeadline(::AZStd::chrono::microseconds deadline);
        SizeType GetStreamByteOffset() const;
        
        static bool Sort(Request& left, Request& right);

        using ListType = ::AZStd::intrusive_list< Request, ::AZStd::list_base_hook<Request> >;

        VirtualStream* m_stream;
        SizeType        m_byteOffset;
        SizeType        m_byteSize;
        void*           m_buffer;
        SizeType        m_bytesProcessedStart;
        SizeType        m_bytesProcessedEnd;
        SizeType        m_currentSeekPos;
        ::AZStd::chrono::system_clock::time_point  m_deadline;
        ::AZStd::chrono::system_clock::time_point  m_estimatedCompletion;
        RequestDoneCB   m_callback;
        bool            m_isDeferredCB;
        StateType       m_state;
        PriorityType    m_priority;
        OperationType   m_operation;
        const char*     m_debugName;        ///< Pointer to a request debug name.
    };

    using RequestHandle = Request*;
    static const RequestHandle  InvalidRequestHandle = 0;
    static const RequestHandle  RequestCacheHitHandle = (Request*)AZ_INVALID_POINTER;

} // namespace IO
} // namespace AZ
