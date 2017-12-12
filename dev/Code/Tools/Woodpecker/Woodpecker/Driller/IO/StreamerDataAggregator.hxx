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

#ifndef DRILLER_STREAMER_DATAAGGREGATOR_TESTS_H
#define DRILLER_STREAMER_DATAAGGREGATOR_TESTS_H

#include <AzCore/base.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/RTTI/RTTI.h>

#include "Woodpecker/Driller/DrillerAggregator.hxx"
#include "Woodpecker/Driller/DrillerAggregatorOptions.hxx"

#include "StreamerEvents.h"
#include "StreamerDataParser.h"

namespace Driller
{
    /**
     * Streamer data drilling aggregator.
     */
    class StreamerDataAggregator : public Aggregator
    {
        Q_OBJECT;
        friend class StreamerMountDeviceEvent;
        friend class StreamerUnmountDeviceEvent;
        friend class StreamerRegisterStreamEvent;
        friend class StreamerUnregisterStreamEvent;
        friend class StreamerAddRequestEvent;
        friend class StreamerCompleteRequestEvent;
        friend class StreamerCancelRequestEvent;
        friend class StreamerRescheduleRequestEvent;
        friend class StreamerOperationStartEvent;
        friend class StreamerOperationCompleteEvent;
		StreamerDataAggregator(const StreamerDataAggregator&) = delete;
    public:
        AZ_RTTI(StreamDataAggregator, "{67F86890-1E88-487F-BBF4-94C66CF47B75}");
        AZ_CLASS_ALLOCATOR(StreamerDataAggregator,AZ::SystemAllocator,0);

        enum SeekEventType
        {
            SEEK_EVENT_NONE = 0,
            SEEK_EVENT_SKIP,	// skipped offset within the same stream
            SEEK_EVENT_SWITCH	// switched to another stream within the same device
        };
        enum TransferEventType
        {
            TRANSFER_EVENT_READ = 0,
            TRANSFER_EVENT_WRITE,
            TRANSFER_EVENT_COMPRESSOR_READ,
            TRANSFER_EVENT_COMPRESSOR_WRITE
        };

        StreamerDataAggregator(int identity = 0);
        virtual ~StreamerDataAggregator();

        static AZ::u32 DrillerId()											{ return StreamerDrillerHandlerParser::GetDrillerId(); }
        virtual AZ::u32 GetDrillerId() const								{ return DrillerId(); }

        static const char* ChannelName() { return "Streamer"; }
        virtual AZ::Crc32 GetChannelId() const override { return AZ::Crc32(ChannelName()); }

        virtual AZ::Debug::DrillerHandlerParser* GetDrillerDataParser() 	{ return &m_parser; }

        virtual void ApplySettingsFromWorkspace(WorkspaceSettingsProvider*);
        virtual void ActivateWorkspaceSettings(WorkspaceSettingsProvider *);
        virtual void SaveSettingsToWorkspace(WorkspaceSettingsProvider*);

        //////////////////////////////////////////////////////////////////////////
        // Aggregator
        virtual void Reset();

        SeekEventType GetSeekType(AZ::s64);

    public slots:
        float ValueAtFrame(FrameNumberType frame ) override;
        QColor GetColor() const override;		
        QString GetName() const override;
        QString GetChannelName() const override;
        QString GetDescription() const override;
        QString GetToolTip() const override;
        AZ::Uuid GetID() const override;
        QWidget* DrillDownRequest(FrameNumberType frame) override;
        void OptionsRequest() override;

        virtual QWidget* DrillDownRequest(FrameNumberType frame, int type);
        
        virtual void OnDataViewDestroyed(QObject *dataView);
    
    protected:
        typedef AZStd::vector<Streamer::Device*>					DeviceArrayType;
        typedef AZStd::unordered_map<AZ::u64,Streamer::Stream*>		StreamMapType;
        typedef AZStd::unordered_map<AZ::u64,Streamer::Request*>	RequestMapType;
        typedef AZStd::unordered_map<QObject*,AZ::u32>				DataViewMap;

        void AdvanceToFrame(FrameNumberType frame);
        void ResetTrackingInfo();
        void KillAllViews();

    public:
        //////////////////////////////////////////////////////////////////////////
        // Derived Information
        typedef struct 
        {
            AZ::s64				m_eventId;
            AZ::u64				m_byteCount;
            TransferEventType	m_eventReason;
        } TransferInfo;
        typedef struct 
        {
            AZ::s64			m_eventId;
            SeekEventType	m_eventReason;
        } SeekInfo;

        typedef AZStd::vector<TransferInfo> TransferBreakoutType;
        typedef AZStd::vector<SeekInfo> SeeksBreakoutType;
        typedef struct
        {
            AZ::u64	m_computedThroughput;
            int m_computedSeeksCount;
            TransferBreakoutType m_transferInfo;
            SeeksBreakoutType m_seekInfo;
            AZ::u64 m_offset;
        } CombinedFrameInfo;
        typedef AZStd::vector<CombinedFrameInfo> CombinedFrameInfoType;

        typedef AZStd::map<AZ::s64, SeekEventType> EventToSeekInfoType;

        const char* GetFilenameFromStreamId(unsigned int globalEventId, AZ::u64 streamId);
        const char* GetDebugNameFromStreamId(unsigned int globalEventId, AZ::u64 streamId);
        const AZ::u64 GetOffsetFromStreamId(unsigned int globalEventId, AZ::u64 streamId);
        float ThroughputAtFrame(FrameNumberType frame);
        float SeeksAtFrame(FrameNumberType frame);
        TransferBreakoutType& ThroughputAtFrameBreakout(FrameNumberType frame);
        SeeksBreakoutType& SeeksAtFrameBreakout(FrameNumberType frame);

        DeviceArrayType			m_devices;
        StreamMapType			m_streams;
        RequestMapType			m_requests;
        CombinedFrameInfoType	m_frameInfo;
        EventToSeekInfoType		m_seeksInfo;

        StreamerDrillerHandlerParser			m_parser;

        // lazy process to derived data fields, cached results once processed
        int						m_highwaterFrame;
        DataViewMap				m_dataViews;		/// track active dialog indexes
        int						m_activeViewCount;
        typedef struct
        {
            AZ::u64	m_currentStreamId;
            AZ::u64	m_offset;
        } SeekTrackingInfo; // per device
        AZStd::unordered_map<AZ::u64,SeekTrackingInfo> m_seekTracking;

    public:
        static void Reflect(AZ::ReflectContext* context);
    };
}

#endif
