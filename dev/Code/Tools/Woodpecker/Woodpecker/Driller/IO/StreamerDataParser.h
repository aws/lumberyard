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

#ifndef DRILLER_STREAMER_DRILLER_PARSER_H
#define DRILLER_STREAMER_DRILLER_PARSER_H

#include <AzCore/Driller/Stream.h>

namespace Driller
{
    class StreamerDataAggregator;

    class StreamerDrillerHandlerParser
        : public AZ::Debug::DrillerHandlerParser
    {
    public:
        enum SubTags
        {
            ST_NONE = 0,
            ST_DEVICE_MOUNTED,
            ST_STREAM_REGISTER,
            ST_READ_CACHE_HIT,
            ST_REQUEST_ADD,
            ST_REQUEST_COMPLETE,
            ST_REQUEST_RESCHEDULE,
            ST_OPERATION_READ,
            ST_OPERATION_READ_COMPLETE,
            ST_OPERATION_WRITE,
            ST_OPERATION_WRITE_COMPLETE,
            ST_OPERATION_COMPRESSOR_READ,
            ST_OPERATION_COMPRESSOR_READ_COMPLETE,
            ST_OPERATION_COMPRESSOR_WRITE,
            ST_OPERATION_COMPRESSOR_WRITE_COMPLETE,
        };

        StreamerDrillerHandlerParser()
            : m_subTag(ST_NONE)
            , m_data(NULL)
            , m_allowCacheHitsInReportedStream(false)
        {}

        static AZ::u32 GetDrillerId()                       { return AZ_CRC("StreamerDriller", 0x2f27ed88); }

        void SetAggregator(StreamerDataAggregator* data)    { m_data = data; }
        void AllowCacheHitsInReportedStream(bool onOff) { m_allowCacheHitsInReportedStream = onOff; }

        virtual AZ::Debug::DrillerHandlerParser*    OnEnterTag(AZ::u32 tagName);
        virtual void OnExitTag(DrillerHandlerParser* handler, AZ::u32 tagName);
        virtual void OnData(const AZ::Debug::DrillerSAXParser::Data& dataNode);

    protected:
        SubTags m_subTag;
        StreamerDataAggregator* m_data;
        bool m_allowCacheHitsInReportedStream;
    };
}

#endif
