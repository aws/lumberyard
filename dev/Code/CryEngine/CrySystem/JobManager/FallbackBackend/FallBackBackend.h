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

#ifndef CRYINCLUDE_CRYSYSTEM_JOBMANAGER_FALLBACKBACKEND_FALLBACKBACKEND_H
#define CRYINCLUDE_CRYSYSTEM_JOBMANAGER_FALLBACKBACKEND_FALLBACKBACKEND_H
#pragma once


#include <IJobManager.h>

namespace JobManager {
    namespace FallBackBackEnd {
        class CFallBackBackEnd
            : public IBackend
        {
        public:
            CFallBackBackEnd();
            ~CFallBackBackEnd();

            bool Init(uint32 nSysMaxWorker){return true; }
            bool ShutDown(){return true; }
            void Update(){}

            void AddJob(JobManager::CJobDelegator& crJob, const JobManager::TJobHandle cJobHandle, JobManager::SInfoBlock& rInfoBlock);

            uint32 GetNumWorkerThreads() const { return 0; }

#if defined(JOBMANAGER_SUPPORT_FRAMEPROFILER)
            virtual IWorkerBackEndProfiler* GetBackEndWorkerProfiler() const {return 0; }
#endif
        };
    } // namespace FallBackBackEnd
} // namespace JobManager

#endif // CRYINCLUDE_CRYSYSTEM_JOBMANAGER_FALLBACKBACKEND_FALLBACKBACKEND_H
