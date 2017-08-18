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

#ifndef CRYINCLUDE_CRYSYSTEM_JOBMANAGER_PCBACKEND_THREADBACKEND_H
#define CRYINCLUDE_CRYSYSTEM_JOBMANAGER_PCBACKEND_THREADBACKEND_H
#pragma once


#include <IJobManager.h>
#include "../JobStructs.h"


namespace JobManager
{
    class CJobManager;
    class CWorkerBackEndProfiler;
}

namespace JobManager {
    class CJobManager;

    namespace ThreadBackEnd {
        namespace detail {
            // stack size for backend worker threads
            enum
            {
                eStackSize = 256 * 1024
            };

            class CWaitForJobObject
            {
            public:
                CWaitForJobObject(uint32 nMaxCount) :
#if defined(JOB_SPIN_DURING_IDLE)
                    m_nCounter(0)
#else
                    m_Semaphore(nMaxCount)
#endif
                {}

                void SignalNewJob()
                {
#if defined(JOB_SPIN_DURING_IDLE)
                    int nCount = ~0;
                    do
                    {
                        nCount = *const_cast<volatile int*>(&m_nCounter);
                    } while (CryInterlockedCompareExchange(alias_cast<volatile LONG*>(&m_nCounter), nCount + 1, nCount) != nCount);
#else
                    m_Semaphore.Release();
#endif
                }

                bool TryGetJob()
                {
                    return false;
                }
                void WaitForNewJob(uint32 nWorkerID)
                {
#if defined(JOB_SPIN_DURING_IDLE)
                    int nCount = ~0;

                    do
                    {
                        nCount = *const_cast<volatile int*>(&m_nCounter);
                        if (nCount > 0)
                        {
                            if (CryInterlockedCompareExchange(alias_cast<volatile LONG*>(&m_nCounter), nCount - 1, nCount) == nCount)
                            {
                                break;
                            }
                        }

                        YieldProcessor();
                        YieldProcessor();
                        YieldProcessor();
                        YieldProcessor();
                        YieldProcessor();
                        YieldProcessor();
                        YieldProcessor();
                        YieldProcessor();

                        SwitchToThread();
                    } while (true);

#else
                    m_Semaphore.Acquire();
#endif
                }
            private:
#if defined(JOB_SPIN_DURING_IDLE)
                volatile int m_nCounter;
#else
                CryFastSemaphore m_Semaphore;
#endif
            };
        } // namespace detail
          // forward declarations
        class CThreadBackEnd;

        // class to represent a worker thread for the PC backend
        class CThreadBackEndWorkerThread
            : public CrySimpleThread<>
        {
        public:
            CThreadBackEndWorkerThread(CThreadBackEnd* pThreadBackend, detail::CWaitForJobObject& rSemaphore, JobManager::SJobQueue_ThreadBackEnd& rJobQueue, uint32 nId);
            ~CThreadBackEndWorkerThread();

            virtual void Run();
            virtual void Stop();

        private:
            void DoWork();
            void DoWorkProducerConsumerQueue(SInfoBlock& rInfoBlock);

            string                                                              m_sThreadName;          // name of the worker thread
            uint32                                                              m_nId;                          // id of the worker thread
            volatile bool                                                   m_bStop;
            detail::CWaitForJobObject&                      m_rSemaphore;
            JobManager::SJobQueue_ThreadBackEnd&    m_rJobQueue;
            CThreadBackEnd*                                           m_pThreadBackend;
        };

        // the implementation of the PC backend
        // has n-worker threads which use atomic operations to pull from the job queue
        // and uses a semaphore to signal the workers if there is work required
        class CThreadBackEnd
            : public IBackend
        {
        public:

            inline void* operator new(std::size_t)
            {
                size_t allocated = 0;
                return CryMalloc(sizeof(CThreadBackEnd), allocated, 128);
            }
            inline void operator delete(void* p)
            {
                CryFree(p, 128);
            }

            CThreadBackEnd();
            virtual ~CThreadBackEnd();

            bool Init(uint32 nSysMaxWorker);
            bool ShutDown();
            void Update(){}

            virtual void AddJob(JobManager::CJobDelegator& crJob, const JobManager::TJobHandle cJobHandle, JobManager::SInfoBlock& rInfoBlock);

            virtual uint32 GetNumWorkerThreads() const { return m_nNumWorkerThreads; }

            // returns the index to use for the frame profiler
            uint32 GetCurrentFrameBufferIndex() const;

#if defined(JOBMANAGER_SUPPORT_FRAMEPROFILER)
            JobManager::IWorkerBackEndProfiler* GetBackEndWorkerProfiler() const { return m_pBackEndWorkerProfiler; }
#endif

        private:
            friend class JobManager::CJobManager;

            JobManager::SJobQueue_ThreadBackEnd             m_JobQueue;                 // job queue node where jobs are pushed into and from
            detail::CWaitForJobObject                                   m_Semaphore;                // semaphore to count available jobs, to allow the workers to go sleeping instead of spinning when no work is required
            std::vector<CThreadBackEndWorkerThread*>    m_arrWorkerThreads; // array of worker threads
            uint8                                                                           m_nNumWorkerThreads;// number of worker threads

            // members required for profiling jobs in the frame profiler
#if defined(JOBMANAGER_SUPPORT_FRAMEPROFILER)
            JobManager::IWorkerBackEndProfiler*                         m_pBackEndWorkerProfiler;
#endif
        };
    } // namespace ThreadBackEnd
} // namespace JobManager

#endif // CRYINCLUDE_CRYSYSTEM_JOBMANAGER_PCBACKEND_THREADBACKEND_H
