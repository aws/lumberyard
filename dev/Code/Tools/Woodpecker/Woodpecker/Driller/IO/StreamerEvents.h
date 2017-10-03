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

#ifndef DRILLER_STREAMER_EVENTS_H
#define DRILLER_STREAMER_EVENTS_H

#include "Woodpecker/Driller/DrillerEvent.h"

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>

namespace Driller
{
    namespace Streamer
    {
        enum StreamOperationType
        {
            SOP_INVALID,
            SOP_READ,
            SOP_WRITE,
            SOP_COMPRESSOR_READ,
            SOP_COMPRESSOR_WRITE,
        };

        struct Operation
        {
            Operation(StreamOperationType type)
                : m_type(type)
                , m_offset(0)
                , m_size(0)
                , m_bytesTransferred(0)
            {}

            StreamOperationType m_type;
            AZ::u64             m_offset;
            AZ::u64             m_size;
            AZ::u64             m_bytesTransferred;
        };

        struct Request
        {
            Request()
                : m_id(0)
                , m_offset(0)
                , m_size(0)
                , m_deadline(0)
                , m_priority(0)
                , m_operation(SOP_INVALID)
                , m_completeState(0)
                , m_debugName(nullptr)
            {}

            AZ::u64     m_streamId;
            AZ::u64     m_id;
            AZ::u64     m_offset;
            AZ::u64     m_size;
            AZ::u64     m_deadline;
            char        m_priority;
            char        m_operation;
            int         m_completeState;
            const char* m_debugName;
        };

        struct Stream
        {
            Stream()
                : m_id(0)
                , m_name(nullptr)
                , m_flags(0)
                , m_size(0)
                , m_operation(nullptr)
                , m_isCompressed(false)
            {}
            AZ::u64     m_deviceId;
            AZ::u64     m_id;
            const char* m_name;
            int         m_flags;
            AZ::u64     m_size;
            Operation*  m_operation;
            bool        m_isCompressed;
        };

        struct Device
        {
            Device()
                : m_id(0)
                , m_name(nullptr)
            {}
            AZ::u64     m_id;
            const char* m_name;
        };

        enum StreamEventType
        {
            SET_DEVICE_MOUNTED = 0,
            SET_DEVICE_UNMOUNTED,
            SET_REGISTER_STREAM,
            SET_UNREGISTER_STREAM,
            SET_READ_CACHE_HIT,
            SET_ADD_REQUEST,
            SET_CANCEL_REQUEST,
            SET_RESCHEDULE_REQUEST,
            SET_COMPLETE_REQUEST,
            SET_OPERATION_START,
            SET_OPERATION_COMPLETE,
        };
    }

    class StreamerMountDeviceEvent
        : public DrillerEvent
    {
    public:
        AZ_CLASS_ALLOCATOR(StreamerMountDeviceEvent, AZ::SystemAllocator, 0);
        AZ_RTTI(StreamerMountDeviceEvent, "{48A71AB4-D0DB-4114-868F-1AB76366C2B0}", DrillerEvent);

        StreamerMountDeviceEvent()
            : DrillerEvent(Streamer::SET_DEVICE_MOUNTED) {}

        virtual void    StepForward(Aggregator* data);
        virtual void    StepBackward(Aggregator* data);

        Streamer::Device    m_deviceData;
    };

    class StreamerUnmountDeviceEvent
        : public DrillerEvent
    {
    public:
        AZ_CLASS_ALLOCATOR(StreamerUnmountDeviceEvent, AZ::SystemAllocator, 0);
        AZ_RTTI(StreamerUnmountDeviceEvent, "{FB38404E-C744-4FA3-8DFC-F699A06BFCFD}", DrillerEvent);

        StreamerUnmountDeviceEvent()
            : DrillerEvent(Streamer::SET_DEVICE_UNMOUNTED)
            , m_deviceId(0)
            , m_unmountedDeviceData(nullptr)
        {}

        virtual void    StepForward(Aggregator* data);
        virtual void    StepBackward(Aggregator* data);

        AZ::u64             m_deviceId;
        Streamer::Device*   m_unmountedDeviceData;
    };

    class StreamerRegisterStreamEvent
        : public DrillerEvent
    {
    public:
        AZ_CLASS_ALLOCATOR(StreamerRegisterStreamEvent, AZ::SystemAllocator, 0);
        AZ_RTTI(StreamerRegisterStreamEvent, "{366B65C4-8226-4A09-9BBE-E42FAE4AAB5B}", DrillerEvent);

        StreamerRegisterStreamEvent()
            : DrillerEvent(Streamer::SET_REGISTER_STREAM) {}

        virtual void    StepForward(Aggregator* data);
        virtual void    StepBackward(Aggregator* data);

        Streamer::Stream    m_streamData;
    };

    class StreamerUnregisterStreamEvent
        : public DrillerEvent
    {
    public:
        AZ_CLASS_ALLOCATOR(StreamerUnregisterStreamEvent, AZ::SystemAllocator, 0);
        AZ_RTTI(StreamerUnregisterStreamEvent, "{C6B04FB2-4299-4FC2-B715-C885089E91F0}", DrillerEvent);

        StreamerUnregisterStreamEvent()
            : DrillerEvent(Streamer::SET_UNREGISTER_STREAM)
            , m_streamId(0)
            , m_removedStreamData(nullptr)
        {}

        virtual void    StepForward(Aggregator* data);
        virtual void    StepBackward(Aggregator* data);

        AZ::u64             m_streamId;
        Streamer::Stream*   m_removedStreamData;
    };


    class StreamerReadCacheHit
        : public DrillerEvent
    {
    public:
        AZ_CLASS_ALLOCATOR(StreamerReadCacheHit, AZ::SystemAllocator, 0);
        AZ_RTTI(StreamerReadCacheHit, "{47734717-3283-47C3-B551-73B81FFC98A4}", DrillerEvent);

        StreamerReadCacheHit()
            : DrillerEvent(Streamer::SET_READ_CACHE_HIT)
        {}

        virtual void    StepForward(Aggregator* data);
        virtual void    StepBackward(Aggregator* data);

        AZ::u64     m_streamId;
        AZ::u64     m_offset;
        AZ::u64     m_size;
        const char* m_debugName;
    };

    class StreamerAddRequestEvent
        : public DrillerEvent
    {
    public:
        AZ_CLASS_ALLOCATOR(StreamerAddRequestEvent, AZ::SystemAllocator, 0);
        AZ_RTTI(StreamerAddRequestEvent, "{22986A14-9A44-4D7A-B68C-C7C64D3A33E7}", DrillerEvent);

        StreamerAddRequestEvent()
            : DrillerEvent(Streamer::SET_ADD_REQUEST)
            , m_timeStamp(0)
        {}

        virtual void    StepForward(Aggregator* data);
        virtual void    StepBackward(Aggregator* data);

        Streamer::Request   m_requestData;
        AZ::u64 m_timeStamp;
    };

    class StreamerCompleteRequestEvent
        : public DrillerEvent
    {
    public:
        AZ_CLASS_ALLOCATOR(StreamerCompleteRequestEvent, AZ::SystemAllocator, 0);
        AZ_RTTI(StreamerCompleteRequestEvent, "{E644CEB7-187A-45AE-A08D-434EECDA8DA1}", DrillerEvent);

        StreamerCompleteRequestEvent()
            : DrillerEvent(Streamer::SET_COMPLETE_REQUEST)
            , m_requestId(0)
            , m_state(-1)
            , m_oldState(-1)
            , m_removedRequest(nullptr)
            , m_timeStamp(0)
        {}

        virtual void    StepForward(Aggregator* data);
        virtual void    StepBackward(Aggregator* data);

        AZ::u64 m_requestId;
        int     m_state;
        int     m_oldState;
        Streamer::Request* m_removedRequest;
        AZ::u64 m_timeStamp;
    };

    class StreamerCancelRequestEvent
        : public DrillerEvent
    {
    public:
        AZ_CLASS_ALLOCATOR(StreamerCancelRequestEvent, AZ::SystemAllocator, 0);
        AZ_RTTI(StreamerCancelRequestEvent, "{8BAC3584-DE72-4CF7-9A9A-E6B0AB7ED09D}", DrillerEvent);

        StreamerCancelRequestEvent()
            : DrillerEvent(Streamer::SET_CANCEL_REQUEST)
            , m_requestId(0)
            , m_cancelledRequestData(nullptr)
        {}

        virtual void    StepForward(Aggregator* data);
        virtual void    StepBackward(Aggregator* data);

        AZ::u64             m_requestId;
        Streamer::Request*  m_cancelledRequestData;
    };

    class StreamerRescheduleRequestEvent
        : public DrillerEvent
    {
    public:
        AZ_CLASS_ALLOCATOR(StreamerRescheduleRequestEvent, AZ::SystemAllocator, 0);
        AZ_RTTI(StreamerRescheduleRequestEvent, "{EAC98934-8792-4BF5-85B2-22E5F95BED6E}", DrillerEvent);

        StreamerRescheduleRequestEvent()
            : DrillerEvent(Streamer::SET_RESCHEDULE_REQUEST)
            , m_requestId(0)
            , m_newDeadline(0)
            , m_newPriority(-1)
            , m_oldDeadline(0)
            , m_oldPriority(-1)
            , m_rescheduledRequestData(nullptr)
        {}

        virtual void    StepForward(Aggregator* data);
        virtual void    StepBackward(Aggregator* data);

        AZ::u64     m_requestId;
        AZ::u64     m_newDeadline;
        char        m_newPriority;
        AZ::u64     m_oldDeadline;
        char        m_oldPriority;
        Streamer::Request*  m_rescheduledRequestData;
    };

    class StreamerOperationStartEvent
        : public DrillerEvent
    {
    public:
        AZ_CLASS_ALLOCATOR(StreamerOperationStartEvent, AZ::SystemAllocator, 0);
        AZ_RTTI(StreamerOperationStartEvent, "{35792D49-58CF-4A61-A0E9-4ABEBFFB9AB2}", DrillerEvent);

        StreamerOperationStartEvent(Streamer::StreamOperationType type)
            : DrillerEvent(Streamer::SET_OPERATION_START)
            , m_operation(type)
            , m_streamId(0)
            , m_previousOperation(nullptr)
            , m_stream(nullptr)
            , m_timeStamp(0)
        {}

        virtual void    StepForward(Aggregator* data);
        virtual void    StepBackward(Aggregator* data);

        AZ::u64                 m_streamId;
        Streamer::Operation     m_operation;
        Streamer::Operation*    m_previousOperation;
        Streamer::Stream*       m_stream;
        AZ::u64                 m_timeStamp;
    };

    class StreamerOperationCompleteEvent
        : public DrillerEvent
    {
    public:
        AZ_CLASS_ALLOCATOR(StreamerOperationCompleteEvent, AZ::SystemAllocator, 0);

        AZ_RTTI(StreamerOperationCompleteEvent, "{D23974D5-319E-473D-94ED-CF491596A095}", DrillerEvent);

        StreamerOperationCompleteEvent(Streamer::StreamOperationType type)
            : DrillerEvent(Streamer::SET_OPERATION_COMPLETE)
            , m_type(type)
            , m_streamId(0)
            , m_bytesTransferred(0)
            , m_stream(nullptr)
            , m_timeStamp(0)
        {}

        virtual void    StepForward(Aggregator* data);
        virtual void    StepBackward(Aggregator* data);

        Streamer::StreamOperationType   m_type;
        AZ::u64                         m_streamId;
        AZ::u64                         m_bytesTransferred;
        Streamer::Stream*               m_stream;
        AZ::u64                         m_timeStamp;
    };
}

#endif
