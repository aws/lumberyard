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
#include <MCore/Source/Job.h>
#include <MCore/Source/JobList.h>
#include <MCore/Source/JobManager.h>
#include <EMotionFX/Source/Allocators.h>


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
        /*
            //Array<Actor::Dependency> toAdd;   // TODO: make this a class member to prevent reallocs?
            const uint32 numStepDependencies = outStep->mDependencies.GetLength();

            // for all actor dependencies
            const uint32 numActorDependencies = instance->GetNumDependencies();
            for (uint32 a=0; a<numActorDependencies; ++a)
            {
                Actor::Dependency* actorDependency = instance->GetDependency(a);

                // check if this dependency is already inside the step, if not add it
                bool found = false;
                for (uint32 i=0; i<numStepDependencies && !found; ++i)
                {
                    //if (actorDependency->mActor == outStep->mDependencies[i].mActor)
                        //found = true;

                    if (actorDependency->mAnimGraph)
                        if (actorDependency->mAnimGraph == outStep->mDependencies[i].mAnimGraph)
                            found = true;
                }

                if (found == false)
                    outStep->mDependencies.Add( *actorDependency );
                    //toAdd.Add( *actorDependency );
            }

            // add all dependencies that need to be added
            //for (uint32 n=0; n<toAdd.GetLength(); ++n)
                //outStep->mDependencies.Add( toAdd[n] );
        */
    }


    // check if there is a matching dependency for a given actor
    bool MultiThreadScheduler::CheckIfHasMatchingDependency(ActorInstance* instance, ScheduleStep* step) const
    {
        MCORE_UNUSED(instance);
        MCORE_UNUSED(step);
        /*  // iterate through all actor instance dependencies
            const uint32 numDependencies = instance->GetNumDependencies();
            for (uint32 i=0; i<numDependencies; ++i)
            {
                // iterate through all step dependencies
                const uint32 numStepDependencies = step->mDependencies.GetLength();
                for (uint32 a=0; a<numStepDependencies; ++a)
                {
                    // if there is a dependency on the same actor
                    //if (step->mDependencies[a].mActor == instance->GetDependency(i)->mActor)
                        //return true;

                    // if there is a dependency on the same animgraph
                    if (step->mDependencies[a].mAnimGraph)
                        if (step->mDependencies[a].mAnimGraph == instance->GetDependency(i)->mAnimGraph)
                            return true;
                }
            }
        */
        return false;
    }


    // log it, for debugging purposes
    void MultiThreadScheduler::Print()
    {
        // for all steps
        const uint32 numSteps = mSteps.GetLength();
        for (uint32 i = 0; i < numSteps; ++i)
        {
            MCore::LogInfo("STEP %.3d - %d", i, mSteps[i].mActorInstances.GetLength());
        }

        MCore::LogInfo("---------");
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
            //Print();
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

        /*  // make sure parents of attachments are updated as well
            const uint32 numActorInstances = actorManager.GetNumActorInstances();
            for (uint32 i=0; i<numActorInstances; ++i)
            {
                ActorInstance* actorInstance = actorManager.GetActorInstance(i);
                if (actorInstance->GetIsVisible())
                    actorInstance->RecursiveSetIsVisibleTowardsRoot( true );
            }*/

        // reset stats
        mNumUpdated.SetValue(0);
        mNumVisible.SetValue(0);
        mNumSampled.SetValue(0);

        // build one big job list with sync points inside
        MCore::JobList* jobList = MCore::JobList::Create();
        MCore::GetJobManager().GetJobPool().Lock();
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
            for (uint32 c = 0; c < numStepEntries; ++c)
            {
                ActorInstance* actorInstance = currentStep.mActorInstances[c];
                if (actorInstance->GetIsEnabled() == false)
                {
                    continue;
                }

                MCore::Job* newJob = MCore::Job::CreateWithoutLock( [this, timePassedInSeconds, actorInstance](const MCore::Job* job)
                        {
                            actorInstance->SetThreadIndex(job->GetThreadIndex());

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
                        });
                jobList->AddJob(newJob);

                mNumUpdated.Increment();
            }

            // wait for all actor instances in this step to be finished
            if (s < numSteps - 1)
            {
                jobList->AddSyncPointWithoutLock();
            }
        } // for all steps

        // execute the jobs list and wait for it to finish
        MCore::GetJobManager().GetJobPool().Unlock();
        MCore::ExecuteJobList(jobList);

        //  MCore::LogInfo("numUpdated=%d   -   numVisible=%d   -    numSampled=%d", mNumUpdated.GetValue(), mNumVisible.GetValue(), mNumSampled.GetValue());
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

        //Print();
    }


    // remove the actor instance from the schedule (excluding attachments)
    uint32 MultiThreadScheduler::RemoveActorInstance(ActorInstance* actorInstance, uint32 startStep)
    {
        MCore::LockGuardRecursive guard(mMutex);
        //MCore::LogInfo("Remove %x", actorInstance);

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

                // automatically clean up empty steps
                //if (numInstances == 0)
                //mSteps.Remove(s);

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
