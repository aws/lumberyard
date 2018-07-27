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
#include "BaseObject.h"
#include "KeyTrackLinear.h"
#include <MCore/Source/StringIdPool.h>


namespace EMotionFX
{
    // forward declarations
    class Node;

    /**
     * A skeletal sub-motion.
     * This is an animated part of a complete motion. For example an animated
     * bone. An example could be the motion of the 'upper arm'. A complete set of submotions
     * together form a complete motion (for example a walk motion).
     */
    class EMFX_API SkeletalSubMotion
        : public BaseObject
    {
        AZ_CLASS_ALLOCATOR_DECL

    public:
        /**
         * Default creation.
         */
        static SkeletalSubMotion* Create();

        /**
         * Create with name.
         * @param name The name of the submotion.
         */
        static SkeletalSubMotion* Create(const char* name);

        /**
         * Returns the position keyframing track.
         * In case there is such keytrack, nullptr is returned.
         * If no keytrack exists yet, you can create it with the CreatePosTrack() method.
         * @result The position keyframing track, which can be nullptr.
         */
        MCORE_INLINE KeyTrackLinear<AZ::PackedVector3f, AZ::PackedVector3f>* GetPosTrack() const;

        /**
         * Returns the rotation keyframing track.
         * In case there is such keytrack, nullptr is returned.
         * If no keytrack exists yet, you can create it with the CreateRotTrack() method.
         * @result The rotation keyframing track, which can be nullptr.
         */
        MCORE_INLINE KeyTrackLinear<MCore::Quaternion, MCore::Compressed16BitQuaternion>* GetRotTrack() const;

        #ifndef EMFX_SCALE_DISABLED
        /**
         * Returns the scaling keyframing track.
         * In case there is such keytrack, nullptr is returned.
         * If no keytrack exists yet, you can create it with the CreateScaleTrack() method.
         * @result The scaling keyframing track, which can be nullptr.
         */
        MCORE_INLINE KeyTrackLinear<AZ::PackedVector3f, AZ::PackedVector3f>* GetScaleTrack() const;
        #endif

        /**
         * Returns the name, in form of a C character buffer.
         * @result A pointer to the null terminated character buffer, containing the  name of this submotion.
         */
        MCORE_INLINE const char* GetName() const;

        /**
         * Returns the name, in form of a String object.
         * @result A pointer to the null terminated character buffer, containing the  name of this submotion.
         */
        MCORE_INLINE const AZStd::string& GetNameString() const;

        /**
         * Get the position at the first frame of the motion, in local space (relative to the parent).
         * @result The position at the first frame of the motion.
         */
        MCORE_INLINE const AZ::Vector3& GetPosePos() const;

        /**
         * Get the rotation at the first frame of the motion, in local space (relative to the parent).
         * @result The rotation at the first frame of the motion.
         */
        MCORE_INLINE MCore::Quaternion GetPoseRot() const;

        /**
         * Get the rotation at the first frame of the motion, in local space (relative to the parent).
         * @result The rotation at the first frame of the motion.
         */
        MCORE_INLINE const MCore::Compressed16BitQuaternion& GetCompressedPoseRot() const;

        #ifndef EMFX_SCALE_DISABLED
        /**
         * Get the scale at the first frame of the motion, in local space (relative to the parent).
         * @result The original pose scaling of this part.
         */
        MCORE_INLINE const AZ::Vector3& GetPoseScale() const;
        #endif

        /**
         * Set the position at the first frame of the motion, in local space (relative to the parent).
         * @param pos The position at the first frame of the motion.
         */
        MCORE_INLINE void SetPosePos(const AZ::Vector3& pos);

        /**
         * Set the rotation at the first frame of the motion, in local space (relative to the parent).
         * @param rot The rotation at the first frame of the motion.
         */
        MCORE_INLINE void SetPoseRot(const MCore::Quaternion& rot);

        /**
         * Set the rotation at the first frame of the motion, in local space (relative to the parent).
         * @param rot The rotation at the first frame of the motion as compressed quaternion.
         */
        MCORE_INLINE void SetCompressedPoseRot(const MCore::Compressed16BitQuaternion& rot);

        #ifndef EMFX_SCALE_DISABLED
        /**
         * Set the scale at the first frame of the motion, in local space (relative to the parent).
         * @param scale The scale at the first frame of the motion.
         */
        MCORE_INLINE void SetPoseScale(const AZ::Vector3& scale);
        #endif

        /**
         * Get the bind pose position, in local space (relative to parent).
         * @result The bind pose position.
         */
        MCORE_INLINE const AZ::Vector3& GetBindPosePos() const;

        /**
         * Get the bind pose rotation, in local space (relative to parent).
         * @result The bind pose rotation quaternion.
         */
        MCORE_INLINE MCore::Quaternion GetBindPoseRot() const;

        /**
         * Get the bind pose rotation, in local space (relative to parent).
         * @result The bind pose rotation as compressed quaternion.
         */
        MCORE_INLINE const MCore::Compressed16BitQuaternion& GetCompressedBindPoseRot() const;

        #ifndef EMFX_SCALE_DISABLED
        /**
         * Get the bind pose scale, in local space (relative to parent).
         * @result The bind pose scale vector.
         */
        MCORE_INLINE const AZ::Vector3& GetBindPoseScale() const;
        #endif

        /**
         * Set the bind pose position, in local space (relative to parent).
         * @param pos The bind pose position.
         */
        MCORE_INLINE void SetBindPosePos(const AZ::Vector3& pos);

        /**
         * Set the bind pose rotation quaternion, in local space (relative to parent).
         * @param rot The quaternion representing the bind pose rotation.
         */
        MCORE_INLINE void SetBindPoseRot(const MCore::Quaternion& rot);

        /**
         * Set the bind pose rotation quaternion, in local space (relative to parent).
         * @param rot The bind pose rotation as compressed quaternion.
         */
        MCORE_INLINE void SetCompressedBindPoseRot(const MCore::Compressed16BitQuaternion& rot);

        #ifndef EMFX_SCALE_DISABLED
        /**
         * Set the bind pose scale value, in local space (relative to the parent).
         * @param scale The bind pose scale value.
         */
        MCORE_INLINE void SetBindPoseScale(const AZ::Vector3& scale);
        #endif

        /**
         * Get the ID of this submotion, that can be matched with the ID of nodes, since the ID is generated from the name string.
         * @result The ID of this submotion.
         */
        MCORE_INLINE uint32 GetID() const;

        /**
         * Create the rotation key track.
         * This should be used in case the GetRotTrack() returns nullptr and you want to add keyframes to the track.
         * Then you first have to execute this method. Also don't forget to set the interpolator in the KeyTrackLinear class.
         * And don't forget to Init the KeyTrackLinear after you have added your keys and set the interpolator.
         * In case the keytrack has already been created, nothing will happen.
         */
        void CreateRotTrack();

        /**
         * Create the position key track.
         * This should be used in case the GetPosTrack() returns nullptr and you want to add keyframes to the track.
         * Then you first have to execute this method. Also don't forget to set the interpolator in the KeyTrackLinear class.
         * And don't forget to Init the KeyTrackLinear after you have added your keys and set the interpolator.
         * In case the keytrack has already been created, nothing will happen.
         */
        void CreatePosTrack();

        #ifndef EMFX_SCALE_DISABLED
        /**
         * Create the scale key track.
         * This should be used in case the GetScaleTrack() returns nullptr and you want to add keyframes to the track.
         * Then you first have to execute this method. Also don't forget to set the interpolator in the KeyTrackLinear class.
         * And don't forget to Init the KeyTrackLinear after you have added your keys and set the interpolator.
         * In case the keytrack has already been created, nothing will happen.
         */
        void CreateScaleTrack();

        /**
         * Check if the scale track only contains uniform scaling values.
         * @return True in case the scale keytrack is uniformly scaled, false if not.
         */
        bool CheckIfIsUniformScaled() const;
        #endif

        /**
         * Removes the position key track.
         * After this, the GetPosTrack() method will return nullptr.
         * If you would want to create it again at a later stage, you can use the CreatePosTrack() method.
         */
        void RemovePosTrack();

        /**
         * Removes the rotation key track.
         * After this, the GetRotTrack() method will return nullptr.
         * If you would want to create it again at a later stage, you can use the CreateRotTrack() method.
         */
        void RemoveRotTrack();

        #ifndef EMFX_SCALE_DISABLED
        /**
         * Removes the scale key track.
         * After this, the GetScaleTrack() method will return nullptr.
         * If you would want to create it again at a later stage, you can use the CreateScaleTrack() method.
         */
        void RemoveScaleTrack();
        #endif


    private:
        KeyTrackLinear<MCore::Quaternion, MCore::Compressed16BitQuaternion>*        mRotTrack;          /**< The rotation key track. */
        KeyTrackLinear<AZ::PackedVector3f, AZ::PackedVector3f>*                     mPosTrack;          /**< The position key track. */
        MCore::Compressed16BitQuaternion                                            mPoseRot;           /**< The original base pose rotation on the first frame, uncompressed for faster performance. */
        AZ::Vector3                                                                 mPosePos;           /**< The original base pose position on the first frame. */
        AZ::Vector3                                                                 mBindPosePos;       /**< The bind pose position in local space. */
        MCore::Compressed16BitQuaternion                                            mBindPoseRot;       /**< The bind pose rotation in local space. */

        #ifndef EMFX_SCALE_DISABLED
        AZ::Vector3                                                                 mPoseScale;             /**< The original base pose scale on the first frame. */
        AZ::Vector3                                                                 mBindPoseScale;         /**< The bind pose scale in local space. */
        KeyTrackLinear<AZ::PackedVector3f, AZ::PackedVector3f>*                     mScaleTrack;            /**< The scale key track. */
        #endif

        uint32                                                                      mNameID;            /**< The unique ID, based on the name. */

        /**
         * Default constructor.
         */
        SkeletalSubMotion();

        /**
         * Constructor.
         * @param name The name of the submotion.
         */
        SkeletalSubMotion(const char* name);

        /**
         * Destructor.
         */
        ~SkeletalSubMotion();
    };

    // include the inline code
#include "SkeletalSubMotion.inl"
} // namespace EMotionFX
