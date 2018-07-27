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
#include "KeyTrackLinear.h"
#include "MorphTarget.h"
#include "BaseObject.h"


namespace EMotionFX
{
    /**
     * The morph sub-motion class.
     * This class mainly contains the keyframe animation data for a given morph target in a given motion.
     */
    class EMFX_API MorphSubMotion
        : public BaseObject
    {
        AZ_CLASS_ALLOCATOR_DECL

    public:
        static MorphSubMotion* Create(uint32 id);

        /**
         * Set the new ID of this sub-motion, which basically relinks it to another morph target.
         * @param id The new ID.
         */
        void SetID(uint32 id);

        /**
         * Get the ID of this sub-motion.
         * Morph targets having the same ID will need this sub-motion to be applied to them.
         * @result The ID of this sub-motion.
         */
        MCORE_INLINE uint32 GetID() const                                   { return mID; }

        /**
         * Get a pointer to the keytrack, which contains the animation data.
         * @result A pointer to the keytrack.
         */
        KeyTrackLinear<float, MCore::Compressed16BitFloat>* GetKeyTrack();

        /**
         * The weight of the initial frame, or when there is no keyframe data.
         * @result The first frame's weight value.
         */
        float GetPoseWeight() const;

        /**
         * Set the pose weight.
         * This is the weight that is used when there is no keyframing data.
         * @param weight The weight value to use.
         */
        void SetPoseWeight(float weight);

        /**
         * Set the phoneme sets that are linked with this submotion.
         * @param phonemeSet The phoneme sets linked with this submotion.
         */
        void SetPhonemeSet(MorphTarget::EPhonemeSet phonemeSet);

        /**
         * Get the phoneme sets that are linked with this submotion.
         * @result The phoneme sets linked with this submotion.
         */
        MorphTarget::EPhonemeSet GetPhonemeSet() const;

    private:
        KeyTrackLinear<float, MCore::Compressed16BitFloat>  mKeyTrack;      /**< The keytrack containing the animation data for a given morph target. */
        uint32                                              mID;            /**< The ID of the MorphTarget where this keytrack should be used on. */
        float                                               mPoseWeight;    /**< The pose weight. */
        MorphTarget::EPhonemeSet                            mPhonemeSet;    /**< The phoneme set it represents. */

        /**
         * The constructor.
         * @param id The ID of the morph target (MorphTarget) where you want to link the keyframe data to.
         */
        MorphSubMotion(uint32 id);

        /**
         * The destructor.
         */
        ~MorphSubMotion();
    };
} // namespace EMotionFX
