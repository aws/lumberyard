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
#include "AnimGraphObjectData.h"
#include "AnimGraphInstance.h"
#include "AnimGraphNode.h"
#include "AnimGraphObjectDataPool.h"


namespace EMotionFX
{
    // constructor
    AnimGraphObjectData::AnimGraphObjectData(AnimGraphObject* object, AnimGraphInstance* animGraphInstance)
        : BaseObject()
    {
        mObject             = object;
        mAnimGraphInstance = animGraphInstance;
        mObjectFlags        = 0;
        mSubPool            = nullptr;
    }


    // destructor
    AnimGraphObjectData::~AnimGraphObjectData()
    {
        mSubPool = nullptr;
    }


    // static create
    AnimGraphObjectData* AnimGraphObjectData::Create(AnimGraphObject* object, AnimGraphInstance* animGraphInstance)
    {
        return new AnimGraphObjectData(object, animGraphInstance);
    }


    // save the object data
    uint32 AnimGraphObjectData::Save(uint8* outputBuffer) const
    {
        if (outputBuffer)
        {
            MCore::MemCopy(outputBuffer, (uint8*)this, sizeof(*this));
        }

        return sizeof(*this);
    }


    // load from a given data buffer
    uint32 AnimGraphObjectData::Load(const uint8* dataBuffer)
    {
        if (dataBuffer)
        {
            MCore::MemCopy((uint8*)this, dataBuffer, sizeof(*this));
        }

        return sizeof(*this);
    }


    // create an instance in a piece of memory
    AnimGraphObjectData* AnimGraphObjectData::Clone(void* destMem, AnimGraphObject* object, AnimGraphInstance* animGraphInstance)
    {
        return new(destMem) AnimGraphObjectData(object, animGraphInstance);
    }


    //
    void AnimGraphObjectData::SetSubPool(AnimGraphObjectDataPool::SubPool* subPool)
    {
        mSubPool = subPool;
    }


    AnimGraphObjectDataPool::SubPool* AnimGraphObjectData::GetSubPool() const
    {
        return mSubPool;
    }
}   // namespace EMotionFX
