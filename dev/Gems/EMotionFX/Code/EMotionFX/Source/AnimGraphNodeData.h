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

// include the required headers
#include "EMotionFXConfig.h"
#include "AnimGraphObjectData.h"
#include "AnimGraphSyncTrack.h"


namespace EMotionFX
{
    // forward declarations
    class AnimGraphObject;
    class AnimGraphInstance;
    class AnimGraphNode;
    class AnimGraphRefCountedData;


    /**
     *
     *
     *
     */
    class EMFX_API AnimGraphNodeData
        : public AnimGraphObjectData
    {
        MCORE_MEMORYOBJECTCATEGORY(AnimGraphNodeData, MCore::MCORE_SIMD_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_OBJECTUNIQUEDATA);
        EMFX_ANIMGRAPHOBJECTDATA_IMPLEMENT_LOADSAVE

    public:
        enum
        {
            INHERITFLAGS_BACKWARD       = 1 << 0,
            INHERITFLAGS_LOOPED         = 1 << 1
        };

        AnimGraphNodeData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance);
        virtual ~AnimGraphNodeData();

        static AnimGraphNodeData* Create(AnimGraphNode* node, AnimGraphInstance* animGraphInstance);

        void Clear();
        virtual AnimGraphObjectData* Clone(void* destMem, AnimGraphObject* object, AnimGraphInstance* animGraphInstance) override;

        void Init(AnimGraphInstance* animGraphInstance, AnimGraphNode* node);
        void Init(AnimGraphNodeData* nodeData);
        virtual uint32 GetClassSize() const override                            { return sizeof(AnimGraphNodeData); }

        MCORE_INLINE AnimGraphNode* GetNode() const                            { return reinterpret_cast<AnimGraphNode*>(mObject); }
        MCORE_INLINE void SetNode(AnimGraphNode* node)                         { mObject = reinterpret_cast<AnimGraphObject*>(node); }

        MCORE_INLINE void SetSyncIndex(uint32 syncIndex)                        { mSyncIndex = syncIndex; }
        MCORE_INLINE uint32 GetSyncIndex() const                                { return mSyncIndex; }

        MCORE_INLINE void SetCurrentPlayTime(float absoluteTime)                { mCurrentTime = absoluteTime; }
        MCORE_INLINE float GetCurrentPlayTime() const                           { return mCurrentTime; }

        MCORE_INLINE void SetPlaySpeed(float speed)                             { mPlaySpeed = speed; }
        MCORE_INLINE float GetPlaySpeed() const                                 { return mPlaySpeed; }

        MCORE_INLINE void SetDuration(float durationInSeconds)                  { mDuration = durationInSeconds; }
        MCORE_INLINE float GetDuration() const                                  { return mDuration; }

        MCORE_INLINE void SetPreSyncTime(float timeInSeconds)                   { mPreSyncTime = timeInSeconds; }
        MCORE_INLINE float GetPreSyncTime() const                               { return mPreSyncTime; }

        MCORE_INLINE void SetGlobalWeight(float weight)                         { mGlobalWeight = weight; }
        MCORE_INLINE float GetGlobalWeight() const                              { return mGlobalWeight; }

        MCORE_INLINE void SetLocalWeight(float weight)                          { mLocalWeight = weight; }
        MCORE_INLINE float GetLocalWeight() const                               { return mLocalWeight; }

        MCORE_INLINE uint8 GetInheritFlags() const                              { return mInheritFlags; }

        MCORE_INLINE bool GetIsBackwardPlaying() const                          { return (mInheritFlags & INHERITFLAGS_BACKWARD) != 0; }
        MCORE_INLINE bool GetHasLooped() const                                  { return (mInheritFlags & INHERITFLAGS_LOOPED) != 0; }

        MCORE_INLINE void SetLoopedFlag()                                       { mInheritFlags |= INHERITFLAGS_LOOPED; }
        MCORE_INLINE void SetBackwardFlag()                                     { mInheritFlags |= INHERITFLAGS_BACKWARD; }
        MCORE_INLINE void ClearInheritFlags()                                   { mInheritFlags = 0; }

        MCORE_INLINE uint8 GetPoseRefCount() const                              { return mPoseRefCount; }
        MCORE_INLINE void IncreasePoseRefCount()                                { mPoseRefCount++; }
        MCORE_INLINE void DecreasePoseRefCount()                                { mPoseRefCount--; }
        MCORE_INLINE void SetPoseRefCount(uint8 refCount)                       { mPoseRefCount = refCount; }

        MCORE_INLINE uint8 GetRefDataRefCount() const                           { return mRefDataRefCount; }
        MCORE_INLINE void IncreaseRefDataRefCount()                             { mRefDataRefCount++; }
        MCORE_INLINE void DecreaseRefDataRefCount()                             { mRefDataRefCount--; }
        MCORE_INLINE void SetRefDataRefCount(uint8 refCount)                    { mRefDataRefCount = refCount; }

        MCORE_INLINE void SetRefCountedData(AnimGraphRefCountedData* data)     { mRefCountedData = data; }
        MCORE_INLINE AnimGraphRefCountedData* GetRefCountedData() const        { return mRefCountedData; }

        MCORE_INLINE const AnimGraphSyncTrack& GetSyncTrack() const            { return mSyncTrack; }
        MCORE_INLINE AnimGraphSyncTrack& GetSyncTrack()                        { return mSyncTrack; }
        MCORE_INLINE void SetSyncTrack(const AnimGraphSyncTrack& syncTrack)    { mSyncTrack = syncTrack; }

    protected:
        float       mDuration;
        float       mCurrentTime;
        float       mPlaySpeed;
        float       mPreSyncTime;
        float       mGlobalWeight;
        float       mLocalWeight;
        uint32      mSyncIndex;             /**< The last used sync track index. */
        uint8       mPoseRefCount;
        uint8       mRefDataRefCount;
        uint8       mInheritFlags;
        AnimGraphRefCountedData*   mRefCountedData;
        AnimGraphSyncTrack         mSyncTrack;

        virtual void Delete() override;
    };
}   // namespace EMotionFX
