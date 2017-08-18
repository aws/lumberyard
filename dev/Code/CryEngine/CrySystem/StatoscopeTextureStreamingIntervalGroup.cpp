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
#include "StatoscopeTextureStreamingIntervalGroup.h"

#if ENABLE_STATOSCOPE

CStatoscopeTextureStreamingIntervalGroup::CStatoscopeTextureStreamingIntervalGroup()
    : CStatoscopeIntervalGroup('t', "streaming textures",
        "['/Textures/' "
        "(string filename) "
        "(int minMipWanted) "
        "(int minMipAvailable) "
        "(int inUse)"
        "]")
{
}

void CStatoscopeTextureStreamingIntervalGroup::Enable_Impl()
{
    gEnv->pRenderer->SetTextureStreamListener(this);
}

void CStatoscopeTextureStreamingIntervalGroup::Disable_Impl()
{
    gEnv->pRenderer->SetTextureStreamListener(NULL);
}

void CStatoscopeTextureStreamingIntervalGroup::OnCreatedStreamedTexture(void* pHandle, const char* name, int nMips, int nMinMipAvailable)
{
    CStatoscopeEventWriter* pWriter = GetWriter();

    if (pWriter)
    {
        size_t payloadLen =
            GetValueLength(name) +
            GetValueLength(0) * 3
        ;

        DataWriter::EventBeginInterval* pEv = pWriter->BeginEvent<DataWriter::EventBeginInterval>(payloadLen);
        pEv->id = reinterpret_cast<UINT_PTR>(pHandle);
        pEv->classId = GetId();

        char* pPayload = (char*)(pEv + 1);
        WriteValue(pPayload, name);
        WriteValue(pPayload, nMips);
        WriteValue(pPayload, nMinMipAvailable);
        WriteValue(pPayload, 0);

        pWriter->EndEvent();
    }
}

void CStatoscopeTextureStreamingIntervalGroup::OnDestroyedStreamedTexture(void* pHandle)
{
    DataWriter::EventEndInterval* pEv = GetWriter()->BeginEvent<DataWriter::EventEndInterval>();
    pEv->id = reinterpret_cast<UINT_PTR>(pHandle);
    GetWriter()->EndEvent();
}

void CStatoscopeTextureStreamingIntervalGroup::OnTextureWantsMip(void* pHandle, int nMinMip)
{
    OnChangedMip(pHandle, 1, nMinMip);
}

void CStatoscopeTextureStreamingIntervalGroup::OnTextureHasMip(void* pHandle, int nMinMip)
{
    OnChangedMip(pHandle, 2, nMinMip);
}

void CStatoscopeTextureStreamingIntervalGroup::OnBegunUsingTextures(void** pHandles, size_t numHandles)
{
    OnChangedTextureUse(pHandles, numHandles, 1);
}

void CStatoscopeTextureStreamingIntervalGroup::OnEndedUsingTextures(void** pHandles, size_t numHandles)
{
    OnChangedTextureUse(pHandles, numHandles, 0);
}

void CStatoscopeTextureStreamingIntervalGroup::OnChangedTextureUse(void** pHandles, size_t numHandles, int inUse)
{
    CStatoscopeEventWriter* pWriter = GetWriter();

    if (pWriter)
    {
        size_t payloadLen = GetValueLength(1);
        uint32 classId = GetId();

        pWriter->BeginBlock();

        for (size_t i = 0; i < numHandles; ++i)
        {
            DataWriter::EventModifyInterval* pEv = pWriter->BeginBlockEvent<DataWriter::EventModifyInterval>(payloadLen);
            pEv->id = reinterpret_cast<UINT_PTR>(pHandles[i]);
            pEv->classId = classId;
            pEv->field = DataWriter::EventModifyInterval::FieldSplitIntervalMask | 3;

            char* pPayload = (char*)(pEv + 1);
            WriteValue(pPayload, inUse);

            pWriter->EndBlockEvent();
        }

        pWriter->EndBlock();
    }
}

void CStatoscopeTextureStreamingIntervalGroup::OnChangedMip(void* pHandle, int field, int nMinMip)
{
    CStatoscopeEventWriter* pWriter = GetWriter();

    if (pWriter)
    {
        size_t payloadLen = GetValueLength(nMinMip);
        uint32 classId = GetId();

        DataWriter::EventModifyInterval* pEv = pWriter->BeginEvent<DataWriter::EventModifyInterval>(payloadLen);
        pEv->id = reinterpret_cast<UINT_PTR>(pHandle);
        pEv->classId = classId;
        pEv->field = DataWriter::EventModifyInterval::FieldSplitIntervalMask | field;

        char* pPayload = (char*)(pEv + 1);
        WriteValue(pPayload, nMinMip);

        pWriter->EndEvent();
    }
}

#endif
