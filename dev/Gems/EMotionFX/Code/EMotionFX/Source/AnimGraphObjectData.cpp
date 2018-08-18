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
#include <EMotionFX/Source/Allocators.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphObjectData, AnimGraphObjectDataAllocator, 0)

    // constructor
    AnimGraphObjectData::AnimGraphObjectData(AnimGraphObject* object, AnimGraphInstance* animGraphInstance)
        : BaseObject()
    {
        mObject             = object;
        mAnimGraphInstance = animGraphInstance;
        mObjectFlags        = 0;
    }


    // destructor
    AnimGraphObjectData::~AnimGraphObjectData()
    {
    }


    // static create
    AnimGraphObjectData* AnimGraphObjectData::Create(AnimGraphObject* object, AnimGraphInstance* animGraphInstance)
    {
        return aznew AnimGraphObjectData(object, animGraphInstance);
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

}   // namespace EMotionFX
