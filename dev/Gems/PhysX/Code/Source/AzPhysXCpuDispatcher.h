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

#include <PhysXSystemComponent.h>

namespace PhysX
{
    /**
    * CPU dispatcher which directs tasks submitted by PhysX to the lumberyard scheduling system.
    */
    class AzPhysXCpuDispatcher
        : public physx::PxCpuDispatcher
    {
    public:
        AZ_CLASS_ALLOCATOR(AzPhysXCpuDispatcher, PhysXAllocator, 0);

        AzPhysXCpuDispatcher();
        ~AzPhysXCpuDispatcher();

        void release();

        //---------------------------------------------------------------------------------
        // PxCpuDispatcher implementation
        //---------------------------------------------------------------------------------
        virtual void submitTask(physx::PxBaseTask& task);

        physx::PxU32 getWorkerCount() const override
        {
            // TODO: We can get the number of worker threads from the current context AZ::JobManager::GetNumWorkerThreads()
            // However we need to wait for a couple of fixes from jwright@. That's 451283 and 455554.
            // We could merge from directly from the con_yorktown branch but it was decided to wait for it to go to main and then merge from there.
            return 4;
        }
    };

    /**
    * Creates a CPU dispatcher which directs tasks submitted by PhysX to the lumberyard scheduling system.
    */
    AzPhysXCpuDispatcher* AzPhysXCpuDispatcherCreate();
} // namespace PhysX
