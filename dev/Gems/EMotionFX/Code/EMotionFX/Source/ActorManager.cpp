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
#include "EMotionFXConfig.h"
#include "ActorManager.h"
#include "ActorInstance.h"
#include "MultiThreadScheduler.h"
#include <MCore/Source/StringConversions.h>
#include <EMotionFX/Source/Allocators.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(ActorManager, ActorManagerAllocator, 0)

    // constructor
    ActorManager::ActorManager()
        : BaseObject()
    {
        mScheduler  = nullptr;

        // set memory categories
        mActorInstances.SetMemoryCategory(EMFX_MEMCATEGORY_ACTORMANAGER);
        mRootActorInstances.SetMemoryCategory(EMFX_MEMCATEGORY_ACTORMANAGER);
        mActors.SetMemoryCategory(EMFX_MEMCATEGORY_ACTORMANAGER);

        // setup the default scheduler
        SetScheduler(MultiThreadScheduler::Create());

        // reserve memory
        mActors.Reserve(512);
        mActorInstances.Reserve(1024);
        mRootActorInstances.Reserve(1024);
    }


    // destructor
    ActorManager::~ActorManager()
    {
        /*
            // destroy all actor instances
            while (GetNumActorInstances() > 0)
            {
                MCORE_ASSERT( mActorInstances[0]->GetReferenceCount() == 1 );
                mActorInstances[0]->Destroy();
            }

            // destroy all actors
            while (GetNumActors() > 0)
            {
                MCORE_ASSERT( mActors[0]->GetReferenceCount() == 1 );
                mActors[0]->Destroy();
            }

            // unregister all actor instances and actors
            UnregisterAllActorInstances();
            UnregisterAllActors();
        */
        // delete the scheduler
        mScheduler->Destroy();
    }


    // create
    ActorManager* ActorManager::Create()
    {
        return aznew ActorManager();
    }


    void ActorManager::DestroyAllActorInstances()
    {
        // destroy all actor instances
        while (GetNumActorInstances() > 0)
        {
            MCORE_ASSERT(mActorInstances[0]->GetReferenceCount() == 1);
            mActorInstances[0]->Destroy();
        }

        UnregisterAllActorInstances();
    }


    void ActorManager::DestroyAllActors()
    {
        while (GetNumActors() > 0)
        {
            MCORE_ASSERT(mActors[0]->GetReferenceCount() == 1);
            mActors[0]->Destroy();
        }

        UnregisterAllActors();
    }


    // unregister all the previously registered actor instances
    void ActorManager::UnregisterAllActorInstances()
    {
        LockActorInstances();
        mActorInstances.Clear();
        mRootActorInstances.Clear();
        if (mScheduler)
        {
            mScheduler->Clear();
        }
        UnlockActorInstances();
    }


    // set the scheduler to use
    void ActorManager::SetScheduler(ActorUpdateScheduler* scheduler, bool delExisting)
    {
        LockActorInstances();

        // delete the existing scheduler, if wanted
        if (delExisting && mScheduler)
        {
            mScheduler->Destroy();
        }

        // update the scheduler pointer
        mScheduler = scheduler;

        // adjust all visibility flags to false for all actor instances
        const uint32 numActorInstances = mActorInstances.GetLength();
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            mActorInstances[i]->SetIsVisible(false);
        }

        UnlockActorInstances();
    }


    // register the actor
    void ActorManager::RegisterActor(Actor* actor)
    {
        LockActors();

        // check if we already registered
        if (FindActorIndex(actor) != MCORE_INVALIDINDEX32)
        {
            MCore::LogWarning("EMotionFX::ActorManager::RegisterActor() - The actor at location 0x%x has already been registered as actor, most likely already by the LoadActor of the importer.", actor);
            UnlockActors();
            return;
        }

        // register it
        mActors.Add(actor);

        UnlockActors();
    }


    // register the actor instance
    void ActorManager::RegisterActorInstance(ActorInstance* actorInstance)
    {
        LockActorInstances();

        mActorInstances.Add(actorInstance);
        UpdateActorInstanceStatus(actorInstance, false);

        UnlockActorInstances();
    }


    // find the actor for a given actor name
    Actor* ActorManager::FindActorByName(const char* actorName) const
    {
        // get the number of actors and iterate through them
        const uint32 numActors = mActors.GetLength();
        for (uint32 i = 0; i < numActors; ++i)
        {
            // compare the actor names
            if (mActors[i]->GetNameString() == actorName)
            {
                return mActors[i];
            }
        }

        return nullptr;
    }


    // find the actor for a given filename
    Actor* ActorManager::FindActorByFileName(const char* fileName) const
    {
        // get the number of actors and iterate through them
        const uint32 numActors = mActors.GetLength();
        for (uint32 i = 0; i < numActors; ++i)
        {
            // compare the motion names
            if (AzFramework::StringFunc::Equal(mActors[i]->GetFileNameString().c_str(), fileName, false /* no case */))
            {
                return mActors[i];
            }
        }

        return nullptr;
    }


    // find the master actor record for a given actor
    uint32 ActorManager::FindActorIndex(Actor* actor) const
    {
        uint32 index = MCORE_INVALIDINDEX32;

        // search through all registered actors
        const uint32 numActors = mActors.GetLength();
        for (uint32 i = 0; i < numActors; ++i)
        {
            if (mActors[i] == actor) // the first entry in the array contains the original actor, as the user loaded it using the importer
            {
                index = i;
                break;
            }
        }

        return index;
    }


    // find the actor for a given actor name
    uint32 ActorManager::FindActorIndexByName(const char* actorName) const
    {
        // search through all registered actors
        const uint32 numActors = mActors.GetLength();
        for (uint32 i = 0; i < numActors; ++i)
        {
            if (mActors[i]->GetNameString() == actorName)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // find the actor for a given actor filename
    uint32 ActorManager::FindActorIndexByFileName(const char* filename) const
    {
        // search through all registered actors
        const uint32 numActors = mActors.GetLength();
        for (uint32 i = 0; i < numActors; ++i)
        {
            if (mActors[i]->GetFileNameString() == filename)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // check if the given actor instance is registered
    bool ActorManager::CheckIfIsActorInstanceRegistered(ActorInstance* actorInstance)
    {
        LockActorInstances();

        // get the number of actor instances and iterate through them
        const uint32 numActorInstances = mActorInstances.GetLength();
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            if (mActorInstances[i] == actorInstance)
            {
                UnlockActorInstances();
                return true;
            }
        }

        // in case we haven't found it return failure
        UnlockActorInstances();
        return false;
    }


    // find the given actor instance inside the actor manager and return its index
    uint32 ActorManager::FindActorInstanceIndex(ActorInstance* actorInstance) const
    {
        // get the number of actor instances and iterate through them
        const uint32 numActorInstances = mActorInstances.GetLength();
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            if (mActorInstances[i] == actorInstance)
            {
                return i;
            }
        }

        // in case we haven't found it return failure
        return MCORE_INVALIDINDEX32;
    }


    // find the actor instance by the identification number
    ActorInstance* ActorManager::FindActorInstanceByID(uint32 id) const
    {
        // get the number of actor instances and iterate through them
        const uint32 numActorInstances = mActorInstances.GetLength();
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            if (mActorInstances[i]->GetID() == id)
            {
                return mActorInstances[i];
            }
        }

        // in case we haven't found it return failure
        return nullptr;
    }


    // find the actor by the identification number
    Actor* ActorManager::FindActorByID(uint32 id) const
    {
        // get the number of actors and iterate through them
        const uint32 numActors = mActors.GetLength();
        for (uint32 i = 0; i < numActors; ++i)
        {
            // compare the ids and return the actor in case they are equal
            if (mActors[i]->GetID() == id)
            {
                return mActors[i];
            }
        }

        // in case we haven't found it return failure
        return nullptr;
    }


    // unregister a given actor instance
    void ActorManager::UnregisterActorInstance(uint32 nr)
    {
        UnregisterActorInstance(mActorInstances[nr]);
    }


    // unregister an actor
    void ActorManager::UnregisterActor(Actor* actor)
    {
        LockActors();

        // find the index in the array of actors
        const uint32 index = FindActorIndex(actor);

        // if we found it, unregister it
        if (index != MCORE_INVALIDINDEX32)
        {
            UnregisterActor(index, false);
        }

        UnlockActors();
    }


    // unregister an actor by index
    void ActorManager::UnregisterActor(uint32 nr, bool lock)
    {
        if (lock)
        {
            LockActors();
        }

        mActors.Remove(nr);

        if (lock)
        {
            UnlockActors();
        }
    }


    // update all actor instances etc
    void ActorManager::UpdateActorInstances(float timePassedInSeconds)
    {
        LockActors();
        LockActorInstances();

        // execute the schedule
        // this makes all the callback OnUpdate calls etc
        mScheduler->Execute(timePassedInSeconds);

        UnlockActorInstances();
        UnlockActors();
    }


    // unregister all the actors
    void ActorManager::UnregisterAllActors()
    {
        LockActors();

        // clear all actors
        mActors.Clear();

        // TODO: what if there are still references to the actors inside the list of registered actor instances?
        UnlockActors();
    }



    // update the actor instance status, which basically checks if the actor instance is still a root actor instance or not
    // with root actor instance we mean that it isn't attached to something
    void ActorManager::UpdateActorInstanceStatus(ActorInstance* actorInstance, bool lock)
    {
        if (lock)
        {
            LockActorInstances();
        }

        // if it is a root actor instance
        if (actorInstance->GetAttachedTo() == nullptr)
        {
            // make sure it's in the root list
            if (mRootActorInstances.Contains(actorInstance) == false)
            {
                mRootActorInstances.Add(actorInstance);
            }
        }
        else // no root actor instance
        {
            // remove it from the root list
            mRootActorInstances.RemoveByValue(actorInstance);
            mScheduler->RecursiveRemoveActorInstance(actorInstance);
        }

        if (lock)
        {
            UnlockActorInstances();
        }
    }


    // unregister an actor instance
    void ActorManager::UnregisterActorInstance(ActorInstance* instance)
    {
        LockActorInstances();

        // remove the actor instance from the list
        mActorInstances.RemoveByValue(instance);

        // remove it from the list of roots, if it is in there
        mRootActorInstances.RemoveByValue(instance);

        // remove it from the schedule
        mScheduler->RemoveActorInstance(instance);

        UnlockActorInstances();
    }


    void ActorManager::LockActorInstances()
    {
        mActorInstanceLock.Lock();
    }


    void ActorManager::UnlockActorInstances()
    {
        mActorInstanceLock.Unlock();
    }


    void ActorManager::LockActors()
    {
        mActorLock.Lock();
    }


    void ActorManager::UnlockActors()
    {
        mActorLock.Unlock();
    }


    Actor* ActorManager::GetActor(uint32 nr) const
    {
        return mActors[nr];
    }


    const MCore::Array<ActorInstance*>& ActorManager::GetActorInstanceArray() const
    {
        return mActorInstances;
    }


    ActorUpdateScheduler* ActorManager::GetScheduler() const
    {
        return mScheduler;
    }
}   // namespace EMotionFX
