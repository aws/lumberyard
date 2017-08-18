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
#include <AzCore/IO/StreamerRequest.h>
#include <AzCore/IO/VirtualStream.h>

namespace AZ
{
namespace IO
{
    //////////////////////////////////////////////////////////////////////////
    // Globals
    const AZStd::chrono::microseconds ExecuteWhenIdle = AZStd::chrono::microseconds::max();

    //Request
    Request::Request(SizeType byteOffset, SizeType byteSize, void* buffer, const RequestDoneCB& cb, bool isDeferredCB, PriorityType priority, OperationType op, const char* debugName)
        : m_stream(nullptr)
        , m_byteOffset(byteOffset)
        , m_byteSize(byteSize)
        , m_buffer(buffer)
        , m_bytesProcessedStart(0)
        , m_bytesProcessedEnd(0)
        , m_currentSeekPos(byteOffset)
        , m_callback(cb)
        , m_isDeferredCB(isDeferredCB)
        , m_state(StateType::ST_PENDING)
        , m_priority(priority)
        , m_operation(op)
        , m_debugName(debugName)
    {
    }

    Request::~Request()
    {
    }
    //=========================================================================
    // SetDeadline
    // [8/29/2012]
    //=========================================================================
    void Request::SetDeadline(AZStd::chrono::microseconds deadline)
    {
        if (deadline == ExecuteWhenIdle)
        {
            AZ_Assert(m_priority == PriorityType::DR_PRIORITY_NORMAL, "You MUST provide a time estimate, when you change priorities! Priorities are used ONLY to solve time estimate collision!");
            m_deadline = AZStd::chrono::system_clock::time_point::max();
        }
        else
        {
            m_deadline = AZStd::chrono::system_clock::now() + deadline;
        }
    }

    Request::SizeType Request::GetStreamByteOffset() const
    {
        return m_byteOffset;
    }

    bool Request::Sort(Request& left, Request& right)
    {
        if (left.m_estimatedCompletion <= left.m_deadline)
        {
            if (right.m_estimatedCompletion <= right.m_deadline)
            {
                return left.m_estimatedCompletion <= right.m_estimatedCompletion; // both request will be done with in the dead line
            }
            else
            {
                return false;
            }
        }
        else
        {
            if (right.m_estimatedCompletion > right.m_deadline)
            {
                // well we are missing both deadlines
                if (left.m_priority != right.m_priority) // sort on priority
                {
                    return left.m_priority < right.m_priority;
                }
                else
                {
                    return left.m_estimatedCompletion <= right.m_estimatedCompletion; // as quick as possible or return left.m_deadline <= right.m_deadline;
                }
            }
        }
        return true;
    }
} // namespace IO
} // namespace AZ
