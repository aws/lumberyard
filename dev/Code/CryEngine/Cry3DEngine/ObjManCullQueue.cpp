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

// Description : Implementation for asynchronous obj-culling queue


#include "StdAfx.h"
#include "ObjManCullQueue.h"
#include "CZBufferCuller.h"
#include <AzCore/Jobs/LegacyJobExecutor.h>

#if defined(USE_CULL_QUEUE)
# define CULL_QUEUE_USE_JOB 1
#else
# define CULL_QUEUE_USE JOB 0
#endif

#if CULL_QUEUE_USE_JOB

namespace
{
    AZ::LegacyJobExecutor s_occlusionJobCompletion;
}

#endif

NCullQueue::SCullQueue::SCullQueue()
    : cullBufWriteHeadIndex(0)
    , cullBufReadHeadIndex(0)
    , m_TestItemQueueReady(false)
    , m_BufferAndCameraReady(false)
    , m_MainFrameID(0)
    , m_pCullBuffer(NULL)
    , m_pCam(NULL)
{
    memset(cullItemBuf, 0, sizeof(cullItemBuf));
}

void NCullQueue::SCullQueue::FinishedFillingTestItemQueue()
{
#ifdef USE_CULL_QUEUE
    assert(gEnv->IsEditor() || m_TestItemQueueReady == false);
    m_TestItemQueueReady = true;
    if (m_BufferAndCameraReady == true)
    {
        //If both sets of data are ready, begin the test.
        Process();
    }
#endif
}

void NCullQueue::SCullQueue::SetTestParams(uint32 mainFrameID, const CCullBuffer* pCullBuffer, const CCamera* pCam)
{
    m_MainFrameID = mainFrameID;
    m_pCullBuffer = (CZBufferCuller*)pCullBuffer;
    m_pCam = pCam;
    assert(gEnv->IsEditor() || m_BufferAndCameraReady == false);
    m_BufferAndCameraReady = true;
    if (m_TestItemQueueReady == true)
    {
        //If both sets of data are ready, begin the test.
        Process();
    }
}

void NCullQueue::SCullQueue::Process()
{
#ifdef USE_CULL_QUEUE
    FUNCTION_PROFILER_3DENGINE;
    s_occlusionJobCompletion.StartJob(
        [this]()
        {
            this->ProcessInternal(this->m_MainFrameID, this->m_pCullBuffer, this->m_pCam);
        }
    );
#endif //USE_CULL_QUEUE
}

NCullQueue::SCullQueue::~SCullQueue()
{
}

void NCullQueue::SCullQueue::Wait()
{
    FUNCTION_PROFILER_3DENGINE;

#ifdef USE_CULL_QUEUE
    s_occlusionJobCompletion.WaitForCompletion();
    cullBufWriteHeadIndex = 0;
    cullBufReadHeadIndex = 0;
#endif
}

void NCullQueue::SCullQueue::ProcessInternal(uint32 mainFrameID, CZBufferCuller* const pCullBuffer, const CCamera* const pCam)
{
#ifdef USE_CULL_QUEUE
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::ThreeDEngine);
    CZBufferCuller& cullBuffer = *(CZBufferCuller*)pCullBuffer;
    #define localQueue (*this)
    #define localCam (*pCam)

    cullBuffer.BeginFrame(localCam);


    //FeedbackLoop:

    {
        const SCullItem* const cEnd = &localQueue.cullItemBuf[localQueue.cullBufWriteHeadIndex];
        for (uint16 a = 1, b = 0; b < 4; a <<= 1, b++)//traverse through all 4 occlusion buffers
        {
            cullBuffer.ReloadBuffer(b);
            for (SCullItem* it = &localQueue.cullItemBuf[ localQueue.cullBufReadHeadIndex ]; it != cEnd; ++it)
            {
                SCullItem& rItem = *it;
                IF (!(rItem.BufferID & a), 1)
                {
                    continue;
                }

                IF ((rItem.BufferID & 1), 0)    //zbuffer
                {
                    if (!cullBuffer.IsObjectVisible(rItem.objBox, eoot_OBJECT, 0.f, &rItem.pOcclTestVars->nLastOccludedMainFrameID))
                    {
                        rItem.pOcclTestVars->nLastOccludedMainFrameID = mainFrameID;
                    }
                    else
                    {
                        rItem.pOcclTestVars->nLastVisibleMainFrameID = mainFrameID;
                    }
                }
                else    //shadow buffer
                if (rItem.pOcclTestVars->nLastNoShadowCastMainFrameID != mainFrameID)
                {
                    if (cullBuffer.IsObjectVisible(rItem.objBox, eoot_OBJECT, 0.f, &rItem.pOcclTestVars->nLastOccludedMainFrameID))
                    {
                        rItem.pOcclTestVars->nLastShadowCastMainFrameID = mainFrameID;
                    }
                }
            }
        }

        localQueue.cullBufReadHeadIndex = (cEnd - localQueue.cullItemBuf);

        //if not visible, set occluded
        for (SCullItem* it = localQueue.cullItemBuf; it != cEnd; ++it)
        {
            SCullItem& rItem = *it;
            IF ((rItem.BufferID & 6) & (rItem.pOcclTestVars->nLastNoShadowCastMainFrameID != mainFrameID), 1)
            {
                rItem.pOcclTestVars->nLastShadowCastMainFrameID = mainFrameID;
            }
        }
    }

#endif
    #undef localQueue
    #undef localCam
}
