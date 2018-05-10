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

MCORE_INLINE KeyTrackLinear<AZ::PackedVector3f, AZ::PackedVector3f>* SkeletalSubMotion::GetPosTrack() const
{
    return mPosTrack;
}


MCORE_INLINE KeyTrackLinear<MCore::Quaternion, MCore::Compressed16BitQuaternion>* SkeletalSubMotion::GetRotTrack() const
{
    return mRotTrack;
}


#ifndef EMFX_SCALE_DISABLED
MCORE_INLINE KeyTrackLinear<AZ::PackedVector3f, AZ::PackedVector3f>* SkeletalSubMotion::GetScaleTrack() const
{
    return mScaleTrack;
}
#endif

MCORE_INLINE const char* SkeletalSubMotion::GetName() const
{
    return MCore::GetStringIdPool().GetName(mNameID).c_str();
}


MCORE_INLINE const AZStd::string& SkeletalSubMotion::GetNameString() const
{
    return MCore::GetStringIdPool().GetName(mNameID);
}


MCORE_INLINE const AZ::Vector3& SkeletalSubMotion::GetPosePos() const
{
    return mPosePos;
}


MCORE_INLINE MCore::Quaternion SkeletalSubMotion::GetPoseRot() const
{
    return mPoseRot.ToQuaternion();
}


MCORE_INLINE const MCore::Compressed16BitQuaternion& SkeletalSubMotion::GetCompressedPoseRot() const
{
    return mPoseRot;
}


#ifndef EMFX_SCALE_DISABLED
MCORE_INLINE const AZ::Vector3& SkeletalSubMotion::GetPoseScale() const
{
    return mPoseScale;
}
#endif


MCORE_INLINE void SkeletalSubMotion::SetPosePos(const AZ::Vector3& pos)
{
    mPosePos = pos;
}


MCORE_INLINE void SkeletalSubMotion::SetPoseRot(const MCore::Quaternion& rot)
{
    mPoseRot.FromQuaternion(rot);
}


MCORE_INLINE void SkeletalSubMotion::SetCompressedPoseRot(const MCore::Compressed16BitQuaternion& rot)
{
    mPoseRot = rot;
}


#ifndef EMFX_SCALE_DISABLED
MCORE_INLINE void SkeletalSubMotion::SetPoseScale(const AZ::Vector3& scale)
{
    mPoseScale = scale;
}
#endif

MCORE_INLINE const AZ::Vector3& SkeletalSubMotion::GetBindPosePos() const
{
    return mBindPosePos;
}


MCORE_INLINE MCore::Quaternion SkeletalSubMotion::GetBindPoseRot() const
{
    return mBindPoseRot.ToQuaternion();
}


MCORE_INLINE const MCore::Compressed16BitQuaternion& SkeletalSubMotion::GetCompressedBindPoseRot() const
{
    return mBindPoseRot;
}


#ifndef EMFX_SCALE_DISABLED
MCORE_INLINE const AZ::Vector3& SkeletalSubMotion::GetBindPoseScale() const
{
    return mBindPoseScale;
}
#endif

MCORE_INLINE void SkeletalSubMotion::SetBindPosePos(const AZ::Vector3& pos)
{
    mBindPosePos = pos;
}


MCORE_INLINE void SkeletalSubMotion::SetBindPoseRot(const MCore::Quaternion& rot)
{
    mBindPoseRot.FromQuaternion(rot);
}


MCORE_INLINE void SkeletalSubMotion::SetCompressedBindPoseRot(const MCore::Compressed16BitQuaternion& rot)
{
    mBindPoseRot = rot;
}


#ifndef EMFX_SCALE_DISABLED
MCORE_INLINE void SkeletalSubMotion::SetBindPoseScale(const AZ::Vector3& scale)
{
    mBindPoseScale = scale;
}
#endif

MCORE_INLINE uint32 SkeletalSubMotion::GetID() const
{
    return mNameID;
}
