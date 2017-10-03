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

#include "EMotionFXConfig.h"
#include "AnimGraphManager.h"
#include "AnimGraph.h"
#include "AnimGraphObjectFactory.h"
#include "AnimGraphNode.h"
#include "AnimGraphAttributeTypes.h"
#include "AnimGraphInstance.h"
#include "BlendSpaceManager.h"
#include "Importer/Importer.h"
#include "ActorManager.h"
#include "EMotionFXManager.h"

#include <MCore/Source/AttributeFactory.h>
#include <MCore/Source/AttributeSettings.h>


namespace EMotionFX
{
    AnimGraphManager::AnimGraphManager()
        : BaseObject()
        , mBlendSpaceManager(nullptr)
    {
        mAttributeInfoSets.SetMemoryCategory(EMFX_MEMCATEGORY_ANIMGRAPH_ATTRIBUTEINFOS);
        mAttributeInfoSets.Reserve(64);
    }


    AnimGraphManager::~AnimGraphManager()
    {
        // delete the factory
        delete mFactory;

        // get rid of all parameter descriptions for all blend tree node types
        //AnimGraphObject::ReleaseAttributeInfoSets();
        RemoveAttributeInfoSets();

        if (mBlendSpaceManager)
        {
            mBlendSpaceManager->Destroy();
        }
        // delete the anim graph instances and anim graphs
        //RemoveAllAnimGraphInstances(true);
        //RemoveAllAnimGraphs(true);
    }


    AnimGraphManager* AnimGraphManager::Create()
    {
        return new AnimGraphManager();
    }


    void AnimGraphManager::Init()
    {
        mAnimGraphInstances.reserve(1024);
        mAnimGraphs.reserve(128);

        mBlendSpaceManager = BlendSpaceManager::Create();

        // register custom attribute types
        MCore::GetAttributeFactory().RegisterAttribute(new AttributeRotation());
        MCore::GetAttributeFactory().RegisterAttribute(new AttributePose());
        MCore::GetAttributeFactory().RegisterAttribute(new AttributeMotionInstance());
        MCore::GetAttributeFactory().RegisterAttribute(new AttributeNodeMask());
        MCore::GetAttributeFactory().RegisterAttribute(new AttributeParameterMask());
        MCore::GetAttributeFactory().RegisterAttribute(new AttributeGoalNode());
        MCore::GetAttributeFactory().RegisterAttribute(new AttributeStateFilterLocal());
        MCore::GetAttributeFactory().RegisterAttribute(new AttributeBlendSpaceMotion());

        // create the factory
        mFactory = new AnimGraphObjectFactory();
        mFactory->Init();
    }


    void AnimGraphManager::RemoveAllAnimGraphs(bool delFromMemory)
    {
        MCore::LockGuardRecursive lock(mAnimGraphLock);

        while (!mAnimGraphs.empty())
        {
            RemoveAnimGraph(mAnimGraphs.size()-1, delFromMemory);
        }
    }


    void AnimGraphManager::RemoveAllAnimGraphInstances(bool delFromMemory)
    {
        MCore::LockGuardRecursive lock(mAnimGraphInstanceLock);

        while (!mAnimGraphInstances.empty())
        {
            RemoveAnimGraphInstance(mAnimGraphInstances.size()-1, delFromMemory);
        }
    }


    // get the attribute info set for this node type
    AnimGraphManager::AttributeInfoSet* AnimGraphManager::FindAttributeInfoSet(const AnimGraphObject* object) const
    {
        const uint32 index = FindAttributeInfoSetIndex(object);
        if (index != MCORE_INVALIDINDEX32)
        {
            return mAttributeInfoSets[index];
        }
        return nullptr;
    }


    // find the index for a given attribute info set that represents this type of condition
    uint32 AnimGraphManager::FindAttributeInfoSetIndex(const AnimGraphObject* object) const
    {
        const uint32 numSets = mAttributeInfoSets.GetLength();
        for (uint32 i = 0; i < numSets; ++i)
        {
            if (mAttributeInfoSets[i]->mTypeID == object->GetType())
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // register the shared attribute information
    void AnimGraphManager::CreateSharedData(AnimGraphObject* object)
    {
        // TODO: add lock for thread safety?
        const uint32 setIndex = FindAttributeInfoSetIndex(object);
        if (setIndex == MCORE_INVALIDINDEX32)
        {
            AttributeInfoSet* infoSet = new AttributeInfoSet();
            infoSet->mTypeID = object->GetType();
            mAttributeInfoSets.Add(infoSet);
        }
    }


    // get the number of attributes for this object type
    uint32 AnimGraphManager::GetNumAttributes(const AnimGraphObject* object) const
    {
        const uint32 numSets = mAttributeInfoSets.GetLength();
        for (uint32 i = 0; i < numSets; ++i)
        {
            if (mAttributeInfoSets[i]->mTypeID == object->GetType())
            {
                return mAttributeInfoSets[i]->mAttributes->GetNumAttributes();
            }
        }

        return 0;
    }


    // register a parameter
    MCore::AttributeSettings* AnimGraphManager::RegisterAttribute(AnimGraphObject* object, const char* name, const char* internalName, const char* description, uint32 interfaceType)
    {
        // TODO: add locks for thread safety

        // find the block for this object type, or create it when it doesn't exist
        AttributeInfoSet* set = nullptr;
        const uint32 setIndex = FindAttributeInfoSetIndex(object);
        if (setIndex == MCORE_INVALIDINDEX32)
        {
            set = new AttributeInfoSet();
            set->mTypeID = object->GetType();
            mAttributeInfoSets.Add(set);
        }
        else
        {
            set = mAttributeInfoSets[setIndex];
        }

        // add the attribute to it
        MCore::AttributeSettings* attrib = MCore::AttributeSettings::Create();
        attrib->SetName(name);
        attrib->SetInternalName(internalName);
        attrib->SetDescription(description);
        attrib->SetInterfaceType(interfaceType);
        set->mAttributes->AddAttribute(attrib);

        return attrib;
    }


    // get the parameter information for a given parameter
    MCore::AttributeSettings* AnimGraphManager::GetAttributeInfo(const AnimGraphObject* object, uint32 index) const
    {
        const uint32 setIndex = FindAttributeInfoSetIndex(object);
        MCORE_ASSERT(setIndex != MCORE_INVALIDINDEX32); // there are no attributes for this node type
        return mAttributeInfoSets[setIndex]->mAttributes->GetAttribute(index);
    }


    // static function, to release all attribute info sets
    void AnimGraphManager::RemoveAttributeInfoSets()
    {
        const uint32 numSets = mAttributeInfoSets.GetLength();
        for (uint32 i = 0; i < numSets; ++i)
        {
            delete mAttributeInfoSets[i];
        }
        mAttributeInfoSets.Clear();
    }


    // remove the attribute info set for a given node type ID
    bool AnimGraphManager::RemoveAttributeInfoSet(uint32 nodeTypeID, bool delFromMem)
    {
        const uint32 numSets = mAttributeInfoSets.GetLength();
        for (uint32 i = 0; i < numSets; ++i)
        {
            if (mAttributeInfoSets[i]->mTypeID == nodeTypeID)
            {
                if (delFromMem)
                {
                    delete mAttributeInfoSets[i];
                }
                mAttributeInfoSets.Remove(i);
                return true;
            }
        }

        return false;
    }


    // check if we already registered attributes for this object or not
    bool AnimGraphManager::CheckIfHasRegisteredAttributes(const AnimGraphObject* object) const
    {
        const uint32 setIndex = FindAttributeInfoSetIndex(object);
        if (setIndex != MCORE_INVALIDINDEX32) // if we already registered before
        {
            return true;
        }

        // no we didn't register any yet
        return false;
    }


    // log parameters
    void AnimGraphManager::LogAttributes(const AnimGraphObject* object) const
    {
        // find the attribute info set for this object type
        const uint32 setIndex = FindAttributeInfoSetIndex(object);
        if (setIndex == MCORE_INVALIDINDEX32)
        {
            MCore::LogInfo("AnimGraph Object '%s' (type=%d) has no registered attributes!", object->GetTypeString(), object->GetType());
            return;
        }

        // get the attribute info set and dump some object info
        AttributeInfoSet* set = mAttributeInfoSets[setIndex];
        const uint32 numAttributes = set->mAttributes->GetNumAttributes();
        MCore::LogInfo("AnimGraph Object '%s' (type=%d) has %d registered attributes:", object->GetTypeString(), object->GetType(), numAttributes);

        MCore::String valueString;
        valueString.Reserve(64);

        // show info for each attribute
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            const MCore::AttributeSettings* attrib = set->mAttributes->GetAttribute(i);

            MCore::LogInfo("- Attribute #%d:", i);
            MCore::LogInfo("  + Name           : %s", attrib->GetName());
            MCore::LogInfo("  + Description    : %s", attrib->GetDescription());
            MCore::LogInfo("  + Interface Type : %d", attrib->GetInterfaceType());
            MCore::LogInfo("  + Data Type      : %s", attrib->GetDefaultValue()->GetTypeString());

            object->GetAttribute(i)->ConvertToString(valueString);
            MCore::LogInfo("  + Current Value  : %s", valueString.AsChar());

            attrib->GetDefaultValue()->ConvertToString(valueString);
            MCore::LogInfo("  + Default Value  : %s", valueString.AsChar());

            if (attrib->GetMinValue())
            {
                attrib->GetMinValue()->ConvertToString(valueString);
                MCore::LogInfo("  + Minimum Value  : %s", valueString.AsChar());
            }

            if (attrib->GetMaxValue())
            {
                attrib->GetMaxValue()->ConvertToString(valueString);
                MCore::LogInfo("  + Maximum Value  : %s", valueString.AsChar());
            }
        } // for all attributes
    }


    void AnimGraphManager::AddAnimGraph(AnimGraph* setup)
    {
        MCore::LockGuardRecursive lock(mAnimGraphLock);
        mAnimGraphs.push_back(setup);
    }


    // Remove a given anim graph by index.
    void AnimGraphManager::RemoveAnimGraph(size_t index, bool delFromMemory)
    {
        MCore::LockGuardRecursive lock(mAnimGraphLock);

        if (delFromMemory)
        {
            // Destroy the memory of the anim graph and disable auto-unregister so that  the destructor of the anim graph does not
            // internally call AnimGraphManager::RemoveAnimGraph(this, false) to unregister the anim graph from the anim graph manager.
            AnimGraph* animGraph = mAnimGraphs[index];
            animGraph->SetAutoUnregister(false);
            animGraph->Destroy();
        }

        mAnimGraphs.erase(mAnimGraphs.begin() + index);
    }


    // Remove a given anim graph by pointer.
    bool AnimGraphManager::RemoveAnimGraph(AnimGraph* animGraph, bool delFromMemory)
    {
        MCore::LockGuardRecursive lock(mAnimGraphLock);

        // find the index of the anim graph and return false in case the pointer is not valid
        const uint32 animGraphIndex = FindAnimGraphIndex(animGraph);
        if (animGraphIndex == MCORE_INVALIDINDEX32)
        {
            return false;
        }

        RemoveAnimGraph(animGraphIndex, delFromMemory);
        return true;
    }


    void AnimGraphManager::AddAnimGraphInstance(AnimGraphInstance* animGraphInstance)
    {
        MCore::LockGuardRecursive lock(mAnimGraphInstanceLock);
        mAnimGraphInstances.push_back(animGraphInstance);
    }


    void AnimGraphManager::RemoveAnimGraphInstance(size_t index, bool delFromMemory)
    {
        MCore::LockGuardRecursive lock(mAnimGraphInstanceLock);

        if (delFromMemory)
        {
            AnimGraphInstance* animGraphInstance = mAnimGraphInstances[index];
            animGraphInstance->RemoveAllObjectData(true);

            // Remove all links to the anim graph instance that will get removed.
            const uint32 numActorInstances = GetActorManager().GetNumActorInstances();
            for (uint32 i = 0; i < numActorInstances; ++i)
            {
                ActorInstance* actorInstance = GetActorManager().GetActorInstance(i);
                if (animGraphInstance == actorInstance->GetAnimGraphInstance())
                {
                    actorInstance->SetAnimGraphInstance(nullptr);
                }
            }

            // Disable automatic unregistration of the anim graph instance from the anim graph manager. If we don't disable it, the anim graph instance destructor
            // would call AnimGraphManager::RemoveAnimGraphInstance(this, false) while we are already removing it by index.
            animGraphInstance->SetAutoUnregisterEnabled(false);
            animGraphInstance->Destroy();
        }

        mAnimGraphInstances.erase(mAnimGraphInstances.begin() + index);
    }


    // remove a given anim graph instance by pointer
    bool AnimGraphManager::RemoveAnimGraphInstance(AnimGraphInstance* animGraphInstance, bool delFromMemory)
    {
        MCore::LockGuardRecursive lock(mAnimGraphInstanceLock);

        // find the index of the anim graph instance and return false in case the pointer is not valid
        const uint32 instanceIndex = FindAnimGraphInstanceIndex(animGraphInstance);
        if (instanceIndex == MCORE_INVALIDINDEX32)
        {
            return false;
        }

        // remove the anim graph instance and return success
        RemoveAnimGraphInstance(instanceIndex, delFromMemory);
        return true;
    }


    void AnimGraphManager::RemoveAnimGraphInstances(AnimGraph* animGraph, bool delFromMemory)
    {
        MCore::LockGuardRecursive lock(mAnimGraphInstanceLock);

        if (mAnimGraphInstances.empty())
        {
            return;
        }

        // Remove anim graph instances back to front in case they are linked to the given anim graph.
        const size_t numInstances = mAnimGraphInstances.size();
        for (size_t i=0; i<numInstances; ++i)
        {
            const size_t reverseIndex = numInstances-1-i;

            AnimGraphInstance* instance = mAnimGraphInstances[reverseIndex];
            if (instance->GetAnimGraph() == animGraph)
            {
                RemoveAnimGraphInstance(reverseIndex, delFromMemory);
            }
        }
    }


    uint32 AnimGraphManager::FindAnimGraphIndex(AnimGraph* animGraph) const
    {
        MCore::LockGuardRecursive lock(mAnimGraphLock);

        auto iterator = AZStd::find(mAnimGraphs.begin(), mAnimGraphs.end(), animGraph);
        if (iterator == mAnimGraphs.end())
            return MCORE_INVALIDINDEX32;

        const size_t index = iterator - mAnimGraphs.begin();
        return static_cast<uint32>(index);
    }


    uint32 AnimGraphManager::FindAnimGraphInstanceIndex(AnimGraphInstance* animGraphInstance) const
    {
        MCore::LockGuardRecursive lock(mAnimGraphInstanceLock);

        auto iterator = AZStd::find(mAnimGraphInstances.begin(), mAnimGraphInstances.end(), animGraphInstance);
        if (iterator == mAnimGraphInstances.end())
            return MCORE_INVALIDINDEX32;

        const size_t index = iterator - mAnimGraphInstances.begin();
        return static_cast<uint32>(index);
    }

    
    // find a anim graph with a given filename
    AnimGraph* AnimGraphManager::FindAnimGraphByFileName(const char* filename, bool isTool) const
    {
        MCore::LockGuardRecursive lock(mAnimGraphLock);

        for (EMotionFX::AnimGraph* animGraph : mAnimGraphs)
        {
            if (animGraph->GetIsOwnedByRuntime() == isTool)
            {
                continue;
            }

            if (animGraph->GetFileNameString().CheckIfIsEqualNoCase(filename))
            {
                return animGraph;
            }
        }

        return nullptr;
    }


    // Find anim graph with a given id.
    AnimGraph* AnimGraphManager::FindAnimGraphByID(uint32 animGraphID) const
    {
        MCore::LockGuardRecursive lock(mAnimGraphLock);

        for (EMotionFX::AnimGraph* animGraph : mAnimGraphs)
        {
            if (animGraph->GetID() == animGraphID)
            {
                return animGraph;
            }
        }

        return nullptr;
    }


    // Find anim graph with a given name.
    AnimGraph* AnimGraphManager::FindAnimGraphByName(const char* name, bool skipAnimGraphsOwnedByRuntime) const
    {
        MCore::LockGuardRecursive lock(mAnimGraphLock);

        for (EMotionFX::AnimGraph* animGraph : mAnimGraphs)
        {
            if (animGraph->GetIsOwnedByRuntime() == skipAnimGraphsOwnedByRuntime)
            {
                continue;
            }

            if (animGraph->GetNameString().CheckIfIsEqualNoCase(name))
            {
                return animGraph;
            }
        }

        return nullptr;
    }


    void AnimGraphManager::SetAnimGraphVisualizationEnabled(bool enabled)
    {
        MCore::LockGuardRecursive lock(mAnimGraphInstanceLock);

        // Enable or disable anim graph visualization for all anim graph instances..
        for (AnimGraphInstance* animGraphInstance : mAnimGraphInstances)
        {
            animGraphInstance->SetVisualizationEnabled(enabled);
        }
    }


    void AnimGraphManager::UpdateInstancesUniqueDataUsingMotionSet(EMotionFX::MotionSet* motionSet)
    {
        // Update unique datas for all anim graph instances that use the given motion set.
        for (EMotionFX::AnimGraphInstance* animGraphInstance : mAnimGraphInstances)
        {
            if (animGraphInstance->GetMotionSet() != motionSet)
            {
                continue;
            }

            animGraphInstance->OnUpdateUniqueData();
        }
    }
} // namespace EMotionFX