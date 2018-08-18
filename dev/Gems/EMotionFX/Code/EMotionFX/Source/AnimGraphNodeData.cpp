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

// include required headers
#include "EMotionFXConfig.h"
#include "AnimGraphNodeData.h"
#include "AnimGraphInstance.h"
#include "AnimGraphNode.h"
#include <EMotionFX/Source/Allocators.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphNodeData, AnimGraphObjectDataAllocator, 0)

    // constructor
    AnimGraphNodeData::AnimGraphNodeData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance)
        : AnimGraphObjectData(reinterpret_cast<AnimGraphObject*>(node), animGraphInstance)
    {
        mSyncIndex      = MCORE_INVALIDINDEX32;
        mInheritFlags   = 0;
        mPoseRefCount   = 0;
        mRefDataRefCount = 0;
        mDuration       = 0.0f;
        mCurrentTime    = 0.0f;
        mPreSyncTime    = 0.0f;
        mPlaySpeed      = 1.0f;
        mGlobalWeight   = 1.0f;
        mLocalWeight    = 1.0f;
        mRefCountedData = nullptr;
    }


    // destructor
    AnimGraphNodeData::~AnimGraphNodeData()
    {
    }


    // static create
    AnimGraphNodeData* AnimGraphNodeData::Create(AnimGraphNode* node, AnimGraphInstance* animGraphInstance)
    {
        return aznew AnimGraphNodeData(node, animGraphInstance);
    }
    

    // reset the sync related data
    void AnimGraphNodeData::Clear()
    {
        mDuration       = 0.0f;
        mCurrentTime    = 0.0f;
        mPreSyncTime    = 0.0f;
        mPlaySpeed      = 1.0f;
        mGlobalWeight   = 1.0f;
        mLocalWeight    = 1.0f;
        mInheritFlags   = 0;
        mSyncIndex      = MCORE_INVALIDINDEX32;
        mSyncTrack.Clear();
    }


    // init the play related settings from a given node
    void AnimGraphNodeData::Init(AnimGraphInstance* animGraphInstance, AnimGraphNode* node)
    {
        Init(reinterpret_cast<AnimGraphNodeData*>(animGraphInstance->FindUniqueObjectData(reinterpret_cast<AnimGraphObject*>(node))));
    }


    // init from existing node data
    void AnimGraphNodeData::Init(AnimGraphNodeData* nodeData)
    {
        mDuration       = nodeData->mDuration;
        mCurrentTime    = nodeData->mCurrentTime;
        mPreSyncTime    = nodeData->mPreSyncTime;
        mPlaySpeed      = nodeData->mPlaySpeed;
        mSyncIndex      = nodeData->mSyncIndex;
        mGlobalWeight   = nodeData->mGlobalWeight;
        mInheritFlags   = nodeData->mInheritFlags;
        mSyncTrack      = nodeData->mSyncTrack;
    }


    void AnimGraphNodeData::Delete()
    {
        delete this;
    }
}   // namespace EMotionFX

