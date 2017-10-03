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

// include required headers
#include "MultiThreadManager.h"
#include "JobManager.h"
#include "Job.h"
#include "JobList.h"
#include "LogManager.h"

// include OpenMP when it is supported
#ifdef MCORE_OPENMP_ENABLED
    #include <omp.h>
#endif


namespace MCore
{
    // simple serial execution
    void JobListExecuteSerial(JobList* jobList, bool addSyncPointAfterList, bool waitForJobListToFinish)
    {
        MCORE_UNUSED(addSyncPointAfterList);
        MCORE_UNUSED(waitForJobListToFinish);

        Job* job = jobList->GetNextJob();
        while (job)
        {
            job->SetThreadIndex(0);
            job->Execute();
            job->Destroy();
            job = jobList->GetNextJob();
        }
        jobList->Destroy();
    }


    // parallel execution using OpenMP, job lists cannot execute in parallel, but internally jobs inside lists can
#ifdef MCORE_OPENMP_ENABLED
    void JobListExecuteOpenMP(JobList* jobList, bool addSyncPointAfterList, bool waitForJobListToFinish)
    {
        MCORE_UNUSED(addSyncPointAfterList);
        MCORE_UNUSED(waitForJobListToFinish);

        // keep track of the current number of threads that are allowed to be used
        //const int numOrgThreads = omp_get_num_threads();

        // limit the max number of threads openmp is allowed to use to the desired number
        //omp_set_num_threads( maxNumThreads );

        // while there are jobs to process
        while (jobList->GetNumJobs() > 0)
        {
            // find the number of jobs to process in this iteration
            // this is either the number of jobs until the next sync point inside the list, or everything remaining
            int32 numJobsToProcess;
            const uint32 syncIndex = jobList->FindFirstSyncJobIndex();
            if (syncIndex == MCORE_INVALIDINDEX32)
            {
                numJobsToProcess = static_cast<int32>(jobList->GetNumJobs());
            }
            else
            {
                numJobsToProcess = static_cast<int32>(syncIndex + 1);
            }

            #pragma omp parallel for ordered
            for (int32 i = 0; i < numJobsToProcess; ++i)
            {
                // get the next job in the queue, as this happens in parallel do this in a critical section
                Job* job;
                    #pragma omp critical
                {
                    job = jobList->GetNextJob();
                }

                // set the thread the job is running on and execute and destroy it
                job->SetThreadIndex(omp_get_thread_num());
                job->Execute();
                job->Destroy();
            }
        }

        // get rid of the job list
        jobList->Destroy();

        // restore the number of threads we are allowed to use
        //omp_set_num_threads( numOrgThreads );
    }
#endif


    // execute the jobs using the MCore job system
    void JobListExecuteMCoreJobSystem(JobList* jobList, bool addSyncPointAfterList, bool waitForJobListToFinish)
    {
        // add the list to the job manager
        GetJobManager().AddJobList(jobList);

        // if we want to add a sync point, do so
        if (addSyncPointAfterList)
        {
            GetJobManager().AddSyncPoint();
        }

        // wait for the job list to have finished before continuing
        if (waitForJobListToFinish)
        {
            jobList->WaitUntilFinished();
        }
    }


    // execute a job list
    void ExecuteJobList(JobList* jobList, bool addSyncPointAfterList, bool waitForJobListToFinish)
    {
        GetMCore().GetJobListExecuteFunc()(jobList, addSyncPointAfterList, waitForJobListToFinish);
    }
}   // namespace MCore
