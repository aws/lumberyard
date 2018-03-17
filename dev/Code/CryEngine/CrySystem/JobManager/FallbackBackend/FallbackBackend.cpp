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
#include "FallBackBackend.h"
#include "../JobManager.h"

JobManager::FallBackBackEnd::CFallBackBackEnd::CFallBackBackEnd()
{
}

JobManager::FallBackBackEnd::CFallBackBackEnd::~CFallBackBackEnd()
{
}

void JobManager::FallBackBackEnd::CFallBackBackEnd::AddJob(JobManager::CJobDelegator& crJob, const JobManager::TJobHandle cJobHandle, JobManager::SInfoBlock& rInfoBlock)
{
    CJobManager* __restrict pJobManager    =  CJobManager::Instance();

    // just execute the job in the calling context
    IF (rInfoBlock.HasQueue(), 0)
    {
        JobManager::SProdConsQueueBase* pQueue          = (JobManager::SProdConsQueueBase*)rInfoBlock.GetQueue();
        JobManager::SJobSyncVariable* pQueueState       = &pQueue->m_QueueRunningState;

        volatile INT_PTR* pQueuePull        = alias_cast<volatile INT_PTR*>(&pQueue->m_pPull);
        volatile INT_PTR* pQueuePush        = alias_cast<volatile INT_PTR*>(&pQueue->m_pPush);

        uint32 queueIncr                                = pQueue->m_PullIncrement;
        INT_PTR queueStart                          = pQueue->m_RingBufferStart;
        INT_PTR queueEnd                                = pQueue->m_RingBufferEnd;
        INT_PTR curPullPtr                          = *pQueuePull;

        // == process job packet == //
        void* pParamMem = (void*)curPullPtr;
        const uint32 cParamSize = (rInfoBlock.paramSize << 4);
        SAddPacketData* const __restrict pAddPacketData = (SAddPacketData*)((unsigned char*)curPullPtr + cParamSize);

        Invoker pInvoker = pJobManager->GetJobInvoker(pAddPacketData->nInvokerIndex);

        {
            // call delegator function to invoke job entry
            AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::System, "JobManager::FallBackBackEnd:RunJob");
            (*pInvoker)(pParamMem);
        }


        // mark job as finished
        IF (pAddPacketData->pJobState, 1)
        {
            pAddPacketData->pJobState->SetStopped();
        }

        // == update queue state == //
        const INT_PTR cNextPull = curPullPtr + queueIncr;
        curPullPtr = (cNextPull >= queueEnd) ? queueStart : cNextPull;

        // update pull ptr (safe since only change by a single job worker)
        *pQueuePull         = curPullPtr;

        // mark queue as finished(safe since the fallback is not threaded)
        pQueueState->SetStopped();
    }
    else
    {
        Invoker delegator = crJob.GetGenericDelegator();
        const void* pParamMem = crJob.GetJobParamData();

        // execute job function
        (*delegator)((void*)pParamMem);

        IF (rInfoBlock.GetJobState(), 1)
        {
            SJobState* pJobState = rInfoBlock.GetJobState();
            pJobState->SetStopped();
        }
    }
}
