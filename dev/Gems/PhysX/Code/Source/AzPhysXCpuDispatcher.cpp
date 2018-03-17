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

#include <PhysX_precompiled.h>
#include <AzPhysXCpuDispatcher.h>
#include <AzPhysXJob.h>

namespace PhysX
{
    AzPhysXCpuDispatcher::AzPhysXCpuDispatcher()
    {
    }

    AzPhysXCpuDispatcher::~AzPhysXCpuDispatcher()
    {
    }

    AzPhysXCpuDispatcher* AzPhysXCpuDispatcherCreate()
    {
        return aznew AzPhysXCpuDispatcher();
    }

    void AzPhysXCpuDispatcher::submitTask(physx::PxBaseTask& task)
    {
        auto azJob = aznew PhysX::AzPhysXJob(task);
        azJob->Start();
    }


    physx::PxU32 AzPhysXCpuDispatcher::getWorkerCount() const
    {
        return AZ::JobContext::GetGlobalContext()->GetJobManager().GetNumWorkerThreads();
    }
} // namespace PhysX

