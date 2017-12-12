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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "StdAfx.h"
#include "StatoscopeStreamingIntervalGroup.h"

#if ENABLE_STATOSCOPE

CStatoscopeStreamingIntervalGroup::CStatoscopeStreamingIntervalGroup()
    : CStatoscopeIntervalGroup('s', "streaming",
        "['/Streaming/' "
        "(string filename) "
        "(int stage) "
        "(int priority) "
        "(int source) "
        "(int perceptualImportance) "
        "(int64 sortKey) "
        "(int compressedSize) "
        "(int mediaType)"
        "]")
{
}

void CStatoscopeStreamingIntervalGroup::Enable_Impl()
{
    gEnv->pSystem->GetStreamEngine()->SetListener(this);
}

void CStatoscopeStreamingIntervalGroup::Disable_Impl()
{
    gEnv->pSystem->GetStreamEngine()->SetListener(NULL);
}

void CStatoscopeStreamingIntervalGroup::OnStreamEnqueue(const void* pReq, const char* filename, EStreamTaskType source, const StreamReadParams& readParams)
{
    CStatoscopeEventWriter* pWriter = GetWriter();

    if (pWriter)
    {
        size_t payloadLen =
            GetValueLength(filename) +
            GetValueLength(Stage_Waiting) +
            GetValueLength(0) * 5 +
            GetValueLength((int64)0);

        DataWriter::EventBeginInterval* pEv = pWriter->BeginEvent<DataWriter::EventBeginInterval>(payloadLen);
        pEv->id = reinterpret_cast<UINT_PTR>(pReq);
        pEv->classId = GetId();

        char* pPayload = (char*)(pEv + 1);
        WriteValue(pPayload, filename);
        WriteValue(pPayload, Stage_Waiting);
        WriteValue(pPayload, (int)readParams.ePriority);
        WriteValue(pPayload, (int)source);
        WriteValue(pPayload, (int)readParams.nPerceptualImportance);
        WriteValue(pPayload, (int64)0);
        WriteValue(pPayload, (int)0);
        WriteValue(pPayload, (int)0);

        pWriter->EndEvent();
    }
}

void CStatoscopeStreamingIntervalGroup::OnStreamComputedSortKey(const void* pReq, uint64 key)
{
    CStatoscopeEventWriter* pWriter = GetWriter();

    if (pWriter)
    {
        size_t payloadLen = GetValueLength((int64)key);
        DataWriter::EventModifyInterval* pEv = pWriter->BeginEvent<DataWriter::EventModifyInterval>(payloadLen);
        pEv->id = reinterpret_cast<UINT_PTR>(pReq);
        pEv->classId = GetId();
        pEv->field = 5;

        char* pPayload = (char*)(pEv + 1);
        WriteValue(pPayload, (int64)key);

        pWriter->EndEvent();
    }
}

void CStatoscopeStreamingIntervalGroup::OnStreamBeginIO(const void* pReq, uint32 compressedSize, uint32 readSize, EStreamSourceMediaType mediaType)
{
    CStatoscopeEventWriter* pWriter = GetWriter();

    if (pWriter)
    {
        {
            size_t payloadLen = GetValueLength((int)compressedSize);
            DataWriter::EventModifyInterval* pEv = pWriter->BeginEvent<DataWriter::EventModifyInterval>(payloadLen);
            pEv->id = reinterpret_cast<UINT_PTR>(pReq);
            pEv->classId = GetId();
            pEv->field = 6;

            char* pPayload = (char*)(pEv + 1);
            WriteValue(pPayload, (int)compressedSize);

            pWriter->EndEvent();
        }
        {
            size_t payloadLen = GetValueLength((int)mediaType);
            DataWriter::EventModifyInterval* pEv = pWriter->BeginEvent<DataWriter::EventModifyInterval>(payloadLen);
            pEv->id = reinterpret_cast<UINT_PTR>(pReq);
            pEv->classId = GetId();
            pEv->field = 7;

            char* pPayload = (char*)(pEv + 1);
            WriteValue(pPayload, (int)mediaType);

            pWriter->EndEvent();
        }
    }

    WriteChangeStageEvent(pReq, Stage_IO, true);
}

void CStatoscopeStreamingIntervalGroup::OnStreamEndIO(const void* pReq)
{
    WriteChangeStageEvent(pReq, Stage_IO, false);
}

void CStatoscopeStreamingIntervalGroup::OnStreamBeginInflate(const void* pReq)
{
    WriteChangeStageEvent(pReq, Stage_Inflate, true);
}

void CStatoscopeStreamingIntervalGroup::OnStreamEndInflate(const void* pReq)
{
    WriteChangeStageEvent(pReq, Stage_Inflate, false);
}

void CStatoscopeStreamingIntervalGroup::OnStreamBeginDecrypt(const void* pReq)
{
    WriteChangeStageEvent(pReq, Stage_Decrypt, true);
}

void CStatoscopeStreamingIntervalGroup::OnStreamEndDecrypt(const void* pReq)
{
    WriteChangeStageEvent(pReq, Stage_Decrypt, false);
}

void CStatoscopeStreamingIntervalGroup::OnStreamBeginAsyncCallback(const void* pReq)
{
    WriteChangeStageEvent(pReq, Stage_Async, true);
}

void CStatoscopeStreamingIntervalGroup::OnStreamEndAsyncCallback(const void* pReq)
{
    WriteChangeStageEvent(pReq, Stage_Async, false);
}

void CStatoscopeStreamingIntervalGroup::OnStreamPreempted(const void* pReq)
{
    WriteChangeStageEvent(pReq, Stage_Preempted, true);
}

void CStatoscopeStreamingIntervalGroup::OnStreamResumed(const void* pReq)
{
    WriteChangeStageEvent(pReq, Stage_Preempted, false);
}

void CStatoscopeStreamingIntervalGroup::OnStreamDone(const void* pReq)
{
    CStatoscopeEventWriter* pWriter = GetWriter();

    if (pWriter)
    {
        DataWriter::EventEndInterval* pEv = pWriter->BeginEvent<DataWriter::EventEndInterval>();
        pEv->id = reinterpret_cast<UINT_PTR>(pReq);
        pWriter->EndEvent();
    }
}

void CStatoscopeStreamingIntervalGroup::WriteChangeStageEvent(const void* pReq, int stage, bool entering)
{
    CStatoscopeEventWriter* pWriter = GetWriter();

    if (pWriter)
    {
        size_t payloadLen = GetValueLength(stage) * 2;
        DataWriter::EventModifyIntervalBit* pEv = pWriter->BeginEvent<DataWriter::EventModifyIntervalBit>(payloadLen);
        pEv->id = reinterpret_cast<UINT_PTR>(pReq);
        pEv->classId = GetId();
        pEv->field = DataWriter::EventModifyInterval::FieldSplitIntervalMask | 1;

        char* pPayload = (char*)(pEv + 1);

        if (entering)
        {
            WriteValue(pPayload, (int)-1);
            WriteValue(pPayload, (int)stage);
        }
        else
        {
            WriteValue(pPayload, (int)~stage);
            WriteValue(pPayload, (int)0);
        }

        pWriter->EndEvent();
    }
}

#endif
