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

#include "EMotionFXConfig.h"
#include <AzCore/std/containers/vector.h>
#include "BaseObject.h"
#include <MCore/Source/Array.h>
#include "AnimGraphObjectDataPool.h"
#include "AnimGraphObject.h"
#include <MCore/Source/MultiThreadManager.h>


namespace EMotionFX
{
    // forward declarations
    class BlendSpaceManager;
    class AnimGraph;
    class AnimGraphObjectFactory;
    class AnimGraphInstance;
    class ActorInstance;


    /**
     *
     *
     */
    class EMFX_API AnimGraphManager
        : public BaseObject
    {
        MCORE_MEMORYOBJECTCATEGORY(AnimGraphManager, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_MANAGER);

    public:
        struct EMFX_API AttributeInfoSet
        {
            MCORE_MEMORYOBJECTCATEGORY(AnimGraphManager::AttributeInfoSet, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_OBJECTS);

            uint32                          mTypeID;
            MCore::AttributeSettingsSet*    mAttributes;

            AttributeInfoSet()              { mAttributes = MCore::AttributeSettingsSet::Create(); mAttributes->Reserve(16); }
            ~AttributeInfoSet()
            {
                if (mAttributes)
                {
                    mAttributes->Destroy();
                }
            }
        };

        static AnimGraphManager* Create();

        void Init();
        MCORE_INLINE AnimGraphObjectFactory* GetObjectFactory() const              { return mFactory; }

        MCORE_INLINE const AnimGraphObjectDataPool& GetObjectDataPool() const      { return mObjectDataPool; }
        MCORE_INLINE AnimGraphObjectDataPool& GetObjectDataPool()                  { return mObjectDataPool; }

        MCORE_INLINE BlendSpaceManager* GetBlendSpaceManager() const { return mBlendSpaceManager; }

        // anim graph helper functions
        void AddAnimGraph(AnimGraph* setup);
        void RemoveAnimGraph(size_t index, bool delFromMemory = true);
        bool RemoveAnimGraph(AnimGraph* animGraph, bool delFromMemory = true);
        void RemoveAllAnimGraphs(bool delFromMemory = true);

        MCORE_INLINE uint32 GetNumAnimGraphs() const                                { MCore::LockGuardRecursive lock(mAnimGraphLock); return static_cast<uint32>(mAnimGraphs.size()); }
        MCORE_INLINE AnimGraph* GetAnimGraph(uint32 index) const                    { MCore::LockGuardRecursive lock(mAnimGraphLock); return mAnimGraphs[index]; }

        uint32 FindAnimGraphIndex(AnimGraph* animGraph) const;
        AnimGraph* FindAnimGraphByFileName(const char* filename, bool isTool = true) const;
        AnimGraph* FindAnimGraphByID(uint32 animGraphID) const;
        AnimGraph* FindAnimGraphByName(const char* name, bool skipAnimGraphsOwnedByRuntime = true) const;

        // anim graph instance helper functions
        void AddAnimGraphInstance(AnimGraphInstance* animGraphInstance);
        void RemoveAnimGraphInstance(size_t index, bool delFromMemory = true);
        bool RemoveAnimGraphInstance(AnimGraphInstance* animGraphInstance, bool delFromMemory = true);
        void RemoveAnimGraphInstances(AnimGraph* animGraph, bool delFromMemory = true);
        void RemoveAllAnimGraphInstances(bool delFromMemory = true);
        void UpdateInstancesUniqueDataUsingMotionSet(EMotionFX::MotionSet* motionSet);

        MCORE_INLINE uint32 GetNumAnimGraphInstances() const                        { MCore::LockGuardRecursive lock(mAnimGraphInstanceLock); return static_cast<uint32>(mAnimGraphInstances.size()); }
        MCORE_INLINE AnimGraphInstance* GetAnimGraphInstance(uint32 index) const    { MCore::LockGuardRecursive lock(mAnimGraphInstanceLock); return mAnimGraphInstances[static_cast<uint32>(index)]; }

        uint32 FindAnimGraphInstanceIndex(AnimGraphInstance* animGraphInstance) const;

        void SetAnimGraphVisualizationEnabled(bool enabled);

        AttributeInfoSet* FindAttributeInfoSet(const AnimGraphObject* object) const;
        uint32 FindAttributeInfoSetIndex(const AnimGraphObject* object) const;
        void CreateSharedData(AnimGraphObject* object);
        uint32 GetNumAttributes(const AnimGraphObject* object) const;
        MCore::AttributeSettings* RegisterAttribute(AnimGraphObject* object, const char* name, const char* internalName, const char* description, uint32 interfaceType);
        MCore::AttributeSettings* GetAttributeInfo(const AnimGraphObject* object, uint32 index) const;
        bool RemoveAttributeInfoSet(uint32 nodeTypeID, bool delFromMem);
        void RemoveAttributeInfoSets();
        bool CheckIfHasRegisteredAttributes(const AnimGraphObject* object) const;
        void LogAttributes(const AnimGraphObject* object) const;
        MCORE_INLINE AttributeInfoSet* GetAttributeInfoSet(uint32 index) const                                                  { return mAttributeInfoSets[index]; }
        MCORE_INLINE MCore::AttributeSettings* GetAttributeInfo(uint32 attributeInfoSetIndex, uint32 attributeIndex) const      { return mAttributeInfoSets[attributeInfoSetIndex]->mAttributes->GetAttribute(attributeIndex); }

    private:
        AZStd::vector<AnimGraph*>           mAnimGraphs;
        AZStd::vector<AnimGraphInstance*>   mAnimGraphInstances;
        AnimGraphObjectFactory*             mFactory;
        AnimGraphObjectDataPool             mObjectDataPool;
        BlendSpaceManager*                  mBlendSpaceManager;
        mutable MCore::MutexRecursive       mAnimGraphLock;
        mutable MCore::MutexRecursive       mAnimGraphInstanceLock;
        MCore::Array<AttributeInfoSet*>     mAttributeInfoSets;

        // constructor and destructor
        AnimGraphManager();
        ~AnimGraphManager();
    };
} // namespace EMotionFX