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
#include "MorphSubMotion.h"
#include <EMotionFX/Source/Allocators.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(MorphSubMotion, DeformerAllocator, 0)


    // constructor
    MorphSubMotion::MorphSubMotion(uint32 id)
    {
        mID             = id;
        mPoseWeight     = 0.0f;
        mPhonemeSet     = MorphTarget::PHONEMESET_NONE;
    }


    // destructor
    MorphSubMotion::~MorphSubMotion()
    {
    }


    // create method
    MorphSubMotion* MorphSubMotion::Create(uint32 id)
    {
        return aznew MorphSubMotion(id);
    }


    void MorphSubMotion::SetID(uint32 id)
    {
        mID = id;
    }


    KeyTrackLinear<float, MCore::Compressed16BitFloat>* MorphSubMotion::GetKeyTrack()
    {
        return &mKeyTrack;
    }


    float MorphSubMotion::GetPoseWeight() const
    {
        return mPoseWeight;
    }


    void MorphSubMotion::SetPoseWeight(float weight)
    {
        mPoseWeight = weight;
    }


    void MorphSubMotion::SetPhonemeSet(MorphTarget::EPhonemeSet phonemeSet)
    {
        mPhonemeSet = phonemeSet;
    }


    MorphTarget::EPhonemeSet MorphSubMotion::GetPhonemeSet() const
    {
        return mPhonemeSet;
    }
}   // namespace EMotionFX
