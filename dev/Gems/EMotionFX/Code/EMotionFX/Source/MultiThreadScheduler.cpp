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

// include the required headers
#include "MultiThreadScheduler.h"
#include "ActorManager.h"
#include "ActorInstance.h"
#include "AnimGraphInstance.h"
#include "Attachment.h"
#include "WaveletCache.h"
#include "EMotionFXManager.h"
#include <EMotionFX/Source/Allocators.h>

#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Jobs/JobCompletion.h>
#include <AzCore/Jobs/JobManagerBus.h>
#include <AzCore/Jobs/JobContext.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(MultiThreadScheduler, ActorUpdateAllocator, 0)

    // constructor
    MultiThreadScheduler::MultiThreadScheduler()
        : ActorUpdateScheduler()
    {
        mSteps.SetMemoryCategory(EMFX_MEMCATEGORY_UPDATESCHEDULERS);
        mCleanTimer     = 0.0f; // time passed since last schedule cleanup, in seconds
        mSteps.Reserve(1000);
    }


    // destructor
    MultiThreadScheduler::~MultiThreadScheduler()
    {
    }


    // create
    MultiThreadScheduler* MultiThreadScheduler::Create()
    {
        return aznew MultiThreadScheduler();
    }


    // clear the schedule
    void MultiThreadScheduler::Clear()
    {
        Lock();
        mSteps.Clear();
        Unlock();
    }


    // add the actor instance dependencies to the schedule step
    void MultiThreadScheduler::AddDependenciesToStep(ActorInstance* instance, ScheduleStep* outStep)
    {
        MCORE_UNUSED(outStep);
        MCORE_UNUSED(instance);
    }


    // check if there is a matching dependency for a given actor
    bool MultiThreadScheduler::CheckIfHasMatchingDependency(ActorInstance* instance, ScheduleStep* step) const
    {
        MCORE_UNUSED(instance);
        MCORE_UNUSED(step);
        return false;
    }


    // log it, for debugging purposes
    void MultiThreadScheduler::Print()
    {
        // for all steps
        const uint32 numSteps = mSteps.GetLength();
        for (uint32 i = 0; i < numSteps; ++i)
        {
            AZ_Printf("EMotionFX", "STEP %.3d - %d", i, mSteps[i].mActorInstances.GetLength());
        }

        AZ_Printf("EMotionFX", "---------");
    }


    void MultiThreadScheduler::RemoveEmptySteps()
    {
        // process all steps
        for (uint32 s = 0; s < mSteps.GetLength(); )
        {
            // if the step �sn't empty
            if (mSteps[s].mActorInstances.GetLength() > 0)
            {
                s++;
            }
            else // otherwise remove it
            {
                mSteps.Remove(s);
            }
        }
    }


    // execute the schedule
    void MultiThreadScheduler::Execute(float timePassedInSeconds)
    {
        MCore::LockGuardRecursive guard(mMutex);

        uint32 numSteps = mSteps.GetLength();
        if (numSteps == 0)
        {
            return;
        }

        // check if we need to cleanup the schedule
        mCleanTimer += timePassedInSeconds;
        if (mCleanTimer >= 1.0f)
        {
            mCleanTimer = 0.0f;
            RemoveEmptySteps();
            numSteps = mSteps.GetLength();
        }

        //-----------------------------------------------------------

        // propagate root actor instance visibility to their attachments
        const ActorManager& actorManager = GetActorManager();
        const uint32 numRootActorInstances = actorManager.GetNumRootActorInstances();
        for (uint32 i = 0; i < numRootActorInstances; ++i)
        {
            ActorInstance* rootInstance = actorManager.GetRootActorInstance(i);
            if (rootInstance->GetIsEnabled() == false)
            {
                continue;
            }

            rootInstance->RecursiveSetIsVisible(rootInstance->GetIsVisible());
        }

        // reset stats
        mNumUpdated.SetValue(0);
        mNumVisible.SetValue(0);
        mNumSampled.SetValue(0);

        for (uint32 s = 0; s < numSteps; ++s)
        {
            const ScheduleStep& currentStep = mSteps[s];

            // skip empty steps
            const uint32 numStepEntries = currentStep.mActorInstances.GetLength();
            if (numStepEntries == 0)
            {
                continue;
            }

            // process the actor instances in the current step in parallel
            AZ::JobCompletion jobCompletion;           
            for (uint32 c = 0; c < numStepEntries; ++c)
            {
                ActorInstance* actorInstance = currentStep.mActorInstances[c];
                if (actorInstance->GetIsEnabled() == false)
                {
                    continue;
                }

                AZ::JobContext* jobContext = nullptr;
                AZ::Job* job = AZ::CreateJobFunction([this, timePassedInSeconds, actorInstance]()
                {
                    AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::Animation, "MultiThreadScheduler::Execute::ActorInstanceUpdateJob");

                    const AZ::u32 threadIndex = AZ::JobContext::GetGlobalContext()->GetJobManager().GetWorkerThreadId();                    
                    actorInstance->SetThreadIndex(threadIndex);

                    const bool isVisible = actorInstance->GetIsVisible();
                    if (isVisible)
                    {
                        mNumVisible.Increment();
                    }

                    // check if we want to sample motions
                    bool sampleMotions = false;
                    actorInstance->SetMotionSamplingTimer(actorInstance->GetMotionSamplingTimer() + timePassedInSeconds);
                    if (actorInstance->GetMotionSamplingTimer() >= actorInstance->GetMotionSamplingRate())
                    {
                        sampleMotions = true;
                        actorInstance->SetMotionSamplingTimer(0.0f);

                        if (isVisible)
                        {
                            mNumSampled.Increment();
                        }
                    }

                    // update the actor instance
                    actorInstance->UpdateTransformations(timePassedInSeconds, isVisible, sampleMotions);
                    GetWaveletCache().Shrink();
                }, true, jobContext);

                job->SetDependent(&jobCompletion);               
                job->Start();

                mNumUpdated.Increment();
            }

            jobCompletion.StartAndWaitForCompletion();
        } // for all steps
    }


    // find the next free spot in the schedule
    bool MultiThreadScheduler::FindNextFreeItem(ActorInstance* actorInstance, uint32 startStep, uint32* outStepNr)
    {
        // try out all steps
        const uint32 numSteps = mSteps.GetLength();
        for (uint32 s = startStep; s < numSteps; ++s)
        {
            // if there is a conflicting dependency, skip this step
            if (CheckIfHasMatchingDependency(actorInstance, &mSteps[s]))
            {
                continue;
            }

            // we found a free spot!
            *outStepNr = s;
            return true;
        }

        *outStepNr = numSteps;
        return false;
    }



    void MultiThreadScheduler::RecursiveInsertActorInstance(ActorInstance* instance, uint32 startStep)
    {
        MCore::LockGuardRecursive guard(mMutex);

        // find the first free location that doesn't conflict
        uint32 outStep = startStep;
        if (!FindNextFreeItem(instance, startStep, &outStep))
        {
            mSteps.Reserve(10);
            mSteps.AddEmpty();
            outStep = mSteps.GetLength() - 1;
        }

        // pre-allocate step size
        if (mSteps[outStep].mActorInstances.GetLength() % 10 == 0)
        {
            mSteps[outStep].mActorInstances.Reserve(mSteps[outStep].mActorInstances.GetLength() + 10);
        }

        if (mSteps[outStep].mDependencies.GetLength() % 5 == 0)
        {
            mSteps[outStep].mDependencies.Reserve(mSteps[outStep].mDependencies.GetLength() + 5);
        }

        // add the actor instance and its dependencies
        mSteps[ outStep ].mActorInstances.Reserve(GetEMotionFX().GetNumThreads());
        mSteps[ outStep ].mActorInstances.Add(instance);
        AddDependenciesToStep(instance, &mSteps[outStep]);

        // recursively add all attachments too
        const uint32 numAttachments = instance->GetNumAttachments();
        for (uint32 i = 0; i < numAttachments; ++i)
        {
            ActorInstance* attachment = instance->GetAttachment(i)->GetAttachmentActorInstance();
            if (attachment)
            {
                RecursiveInsertActorInstance(attachment, outStep + 1);
            }
        }
    }


    // remove the actor instance from the schedule (excluding attachments)
    uint32 MultiThreadScheduler::RemoveActorInstance(ActorInstance* actorInstance, uint32 startStep)
    {
        MCore::LockGuardRecursive guard(mMutex);

        // for all scheduler steps, starting from the specified start step number
        const uint32 numSteps = mSteps.GetLength();
        for (uint32 s = startStep; s < numSteps; ++s)
        {
            // try to see if there is anything to remove in this step
            // and if so, reconstruct the dependencies of this step
            if (mSteps[s].mActorInstances.RemoveByValue(actorInstance))
            {
                // clear the dependencies (but don't delete the memory)
                mSteps[s].mDependencies.Clear(false);

                // calculate the new dependencies for this step
                const uint32 numInstances = mSteps[s].mActorInstances.GetLength();
                for (uint32 i = 0; i < numInstances; ++i)
                {
                    AddDependenciesToStep(mSteps[s].mActorInstances[i], &mSteps[s]);
                }

                // assume that there is only one of the same actor instance in the whole schedule
                return s;
            }
        }

        return 0;
    }


    // remove the actor instance (including all of its attachments)
    void MultiThreadScheduler::RecursiveRemoveActorInstance(ActorInstance* actorInstance, uint32 startStep)
    {
        MCore::LockGuardRecursive guard(mMutex);

        // remove the actual actor instance
        const uint32 step = RemoveActorInstance(actorInstance, startStep);

        // recursively remove all attachments as well
        const uint32 numAttachments = actorInstance->GetNumAttachments();
        for (uint32 i = 0; i < numAttachments; ++i)
        {
            ActorInstance* attachment = actorInstance->GetAttachment(i)->GetAttachmentActorInstance();
            if (attachment)
            {
                RecursiveRemoveActorInstance(attachment, step);
            }
        }
    }


    void MultiThreadScheduler::Lock()
    {
        mMutex.Lock();
    }


    void MultiThreadScheduler::Unlock()
    {
        mMutex.Unlock();
    }
}   // namespace EMotionFX
