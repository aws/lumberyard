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

#pragma once

#include <AzCore/Debug/Profiler.h>
#include <AzCore/Jobs/JobFunction.h>

class MergedMeshJobExecutor final
{
public:
    MergedMeshJobExecutor() = default;

    MergedMeshJobExecutor(const MergedMeshJobExecutor&) = delete;

    ~MergedMeshJobExecutor()
    {
        WaitForCompletion();
    }

    template <class Function>
    inline void StartJob(const Function& processFunction, AZ::JobContext* context = nullptr)
    {
        ++m_jobCount;
        AZ::Job* job = AZ::CreateJobFunction([this, processFunction]() { processFunction(); --m_jobCount; }, true, context);
        job->Start();
    }

    inline void WaitForCompletion()
    {
        while (IsRunning())
        {
            AZ_PROFILE_FUNCTION_STALL(AZ::Debug::ProfileCategory::AzCore);
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(0));
        }
    }

    // Are there presently jobs in-flight (queued or running)?
    inline bool IsRunning()
    {
        return m_jobCount != 0;
    }

private:
    AZStd::atomic_int m_jobCount{ 0 };
};
