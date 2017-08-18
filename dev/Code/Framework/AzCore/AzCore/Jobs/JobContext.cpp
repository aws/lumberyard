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
#ifndef AZ_UNITY_BUILD

#include <AzCore/Jobs/JobContext.h>

#include <AzCore/Jobs/Job.h>
#include <AzCore/Jobs/JobManager.h>

namespace AZ
{
    static EnvironmentVariable<JobContext*> g_jobContextInstance;
    static const char* s_jobContextName = "JobContext";

    void JobContext::SetGlobalContext(JobContext* context)
    {
        if (!g_jobContextInstance)
        {
            g_jobContextInstance = Environment::CreateVariable<JobContext*>(s_jobContextName);
            (*g_jobContextInstance) = nullptr;
        }

        // At this point we're guaranteed to have g_jobContextInstance. Its value might be nullptr.
        if ((context) && (g_jobContextInstance) && (*g_jobContextInstance))
        {
            AZ_Error("JobContext", false, "JobContext::SetGlobalContext was called without first destroying the old context and setting it to nullptr");
        }

        (*g_jobContextInstance) = context;
    }

    JobContext* JobContext::GetGlobalContext()
    {
        if (!g_jobContextInstance)
        {
            g_jobContextInstance = Environment::FindVariable<JobContext*>(s_jobContextName);
        }

        return g_jobContextInstance ? *g_jobContextInstance : nullptr;
    }

    JobContext* JobContext::GetParentContext()
    {
        JobContext* globalContext = GetGlobalContext();
        AZ_Assert(globalContext, "Unable to get a parent context unless a global context has been assigned with JobContext::SetGlobalContext");
        Job* currentJob = globalContext->GetJobManager().GetCurrentJob();
        return currentJob ? currentJob->GetContext() : globalContext;
    }
}

#endif // #ifndef AZ_UNITY_BUILD