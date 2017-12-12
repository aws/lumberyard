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

#ifndef CRYINCLUDE_CRYSYSTEM_STATOSCOPESTREAMINGINTERVALGROUP_H
#define CRYINCLUDE_CRYSYSTEM_STATOSCOPESTREAMINGINTERVALGROUP_H
#pragma once


#include "Statoscope.h"

#include "IStreamEngine.h"

#if ENABLE_STATOSCOPE

class CStatoscopeStreamingIntervalGroup
    : public CStatoscopeIntervalGroup
    , public IStreamEngineListener
{
public:
    enum
    {
        Stage_Waiting           = 0,
        Stage_IO                    = (1 << 0),
        Stage_Inflate           = (1 << 1),
        Stage_Async             = (1 << 2),
        Stage_Preempted     = (1 << 3),
        Stage_Decrypt           = (1 << 4),
    };

public:
    CStatoscopeStreamingIntervalGroup();

    void Enable_Impl();
    void Disable_Impl();

public: // IStreamEngineListener Members
    void OnStreamEnqueue(const void* pReq, const char* filename, EStreamTaskType source, const StreamReadParams& readParams);
    void OnStreamComputedSortKey(const void* pReq, uint64 key);
    void OnStreamBeginIO(const void* pReq, uint32 compressedSize, uint32 readSize, EStreamSourceMediaType mediaType);
    void OnStreamEndIO(const void* pReq);
    void OnStreamBeginInflate(const void* pReq);
    void OnStreamEndInflate(const void* pReq);
    void OnStreamBeginDecrypt(const void* pReq);
    void OnStreamEndDecrypt(const void* pReq);
    void OnStreamBeginAsyncCallback(const void* pReq);
    void OnStreamEndAsyncCallback(const void* pReq);
    void OnStreamDone(const void* pReq);
    void OnStreamPreempted(const void* pReq);
    void OnStreamResumed(const void* pReq);

private:
    void WriteChangeStageEvent(const void* pReq, int stage, bool entering);
};

#endif

#endif // CRYINCLUDE_CRYSYSTEM_STATOSCOPESTREAMINGINTERVALGROUP_H
