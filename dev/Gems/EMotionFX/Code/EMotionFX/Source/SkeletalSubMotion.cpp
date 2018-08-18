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
#include "SkeletalSubMotion.h"
#include <EMotionFX/Source/Allocators.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(SkeletalSubMotion, MotionAllocator, 0)


    // constructor
    SkeletalSubMotion::SkeletalSubMotion()
        : BaseObject()
    {
        EMFX_SCALECODE
        (
            mBindPoseScale.Set  (1.0f, 1.0f, 1.0f);
            mPoseScale.Set      (1.0f, 1.0f, 1.0f);
            mScaleTrack         = nullptr;
        )

        mBindPosePos.Set    (0.0f, 0.0f, 0.0f);
        SetBindPoseRot      (MCore::Quaternion(0.0f, 0.0f, 0.0f, 1.0f));
        mPosePos.Set        (0.0f, 0.0f, 0.0f);
        SetPoseRot          (MCore::Quaternion(0.0f, 0.0f, 0.0f, 1.0f));

        mPosTrack           = nullptr;
        mRotTrack           = nullptr;

        mNameID = MCORE_INVALIDINDEX32;
    }


    // extended constructor
    SkeletalSubMotion::SkeletalSubMotion(const char* name)
        : BaseObject()
    {
        EMFX_SCALECODE
        (
            mBindPoseScale.Set  (1.0f, 1.0f, 1.0f);
            mPoseScale.Set      (1.0f, 1.0f, 1.0f);
            mScaleTrack     = nullptr;
        )

        mBindPosePos.Set    (0.0f, 0.0f, 0.0f);
        SetBindPoseRot      (MCore::Quaternion(0.0f, 0.0f, 0.0f, 1.0f));
        mPosePos.Set        (0.0f, 0.0f, 0.0f);
        SetPoseRot          (MCore::Quaternion(0.0f, 0.0f, 0.0f, 1.0f));

        mPosTrack           = nullptr;
        mRotTrack           = nullptr;

        mNameID = MCore::GetStringIdPool().GenerateIdForString(name);
    }



    // destructor
    SkeletalSubMotion::~SkeletalSubMotion()
    {
        delete mPosTrack;
        delete mRotTrack;

        EMFX_SCALECODE
        (
            delete mScaleTrack;
        )
    }


    // create
    SkeletalSubMotion* SkeletalSubMotion::Create()
    {
        return aznew SkeletalSubMotion();
    }


    // create
    SkeletalSubMotion* SkeletalSubMotion::Create(const char* name)
    {
        return aznew SkeletalSubMotion(name);
    }


    // create/allocate the position track
    void SkeletalSubMotion::CreatePosTrack()
    {
        if (mPosTrack)
        {
            return;
        }

        mPosTrack = new KeyTrackLinear<AZ::PackedVector3f, AZ::PackedVector3f>();
    }


    // create/allocate the rotation track
    void SkeletalSubMotion::CreateRotTrack()
    {
        if (mRotTrack)
        {
            return;
        }

        mRotTrack = new KeyTrackLinear<MCore::Quaternion, MCore::Compressed16BitQuaternion>();
    }


#ifndef EMFX_SCALE_DISABLED
    // create/allocate the scale track
    void SkeletalSubMotion::CreateScaleTrack()
    {
        if (mScaleTrack)
        {
            return;
        }

        mScaleTrack = new KeyTrackLinear<AZ::PackedVector3f, AZ::PackedVector3f>();
    }
#endif


    // remove the position track
    void SkeletalSubMotion::RemovePosTrack()
    {
        delete mPosTrack;
        mPosTrack = nullptr;
    }


    // remove the rotation track
    void SkeletalSubMotion::RemoveRotTrack()
    {
        delete mRotTrack;
        mRotTrack = nullptr;
    }


#ifndef EMFX_SCALE_DISABLED
    // remove the scale track
    void SkeletalSubMotion::RemoveScaleTrack()
    {
        delete mScaleTrack;
        mScaleTrack = nullptr;
    }
#endif


#ifndef EMFX_SCALE_DISABLED
    // check if the scale track only contains uniform scaling values
    bool SkeletalSubMotion::CheckIfIsUniformScaled() const
    {
        // if the pose or bind pose scale values already aren't uniform we don't have to check the keyframes at all
        if (MCore::CheckIfIsUniform(mPoseScale) == false || MCore::CheckIfIsUniform(mBindPoseScale) == false)
        {
            return false;
        }

        // in case there is no scale track and the pose and bind pose scale values are uniform return success
        if (mScaleTrack == nullptr)
        {
            return true;
        }

        // get the number of keyframes and iterate through them
        const uint32 numKeys = mScaleTrack->GetNumKeys();
        for (uint32 i = 0; i < numKeys; ++i)
        {
            const AZ::Vector3& value = AZ::Vector3(mScaleTrack->GetKey(i)->GetValue());
            if (MCore::CheckIfIsUniform(value) == false)
            {
                return false;
            }
        }

        // in case all keyframes are uniformly scaled return success
        return true;
    }
#endif
} // namespace EMotionFX
