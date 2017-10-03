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

// include core system
#include "EMotionFXConfig.h"


namespace EMotionFX
{
    /**
     * The keyframe class.
     * Each keyframe holds a value at a given time.
     * Interpolators can then calculate interpolated values between a set of keys, which are stored
     * inside a key track. This makes it possible to do keyframed animations.
     */
    template <class ReturnType, class StorageType>
    class KeyFrame
    {
        MCORE_MEMORYOBJECTCATEGORY(KeyFrame, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_MOTIONS_KEYTRACKS);

    public:
        /**
         * Default constructor.
         */
        KeyFrame();

        /**
         * Constructor which sets the time and value.
         * @param time The time of the keyframe, in seconds.
         * @param value The value at this time.
         */
        KeyFrame(float time, const ReturnType& value);

        /**
         * Destructor.
         */
        ~KeyFrame();

        /**
         * Return the time of the keyframe.
         * @return The time, in seconds.
         */
        MCORE_INLINE float GetTime() const;

        /**
         * Return the value of the keyframe.
         * @return The value of the keyframe.
         */
        MCORE_INLINE ReturnType GetValue() const;

        /**
         * Return the storage type value of the keyframe.
         * @return The storage type value of the keyframe.
         */
        MCORE_INLINE const StorageType& GetStorageTypeValue() const;

        /**
         * Get the value of the keyframe.
         * @param outValue The value of the keyframe.
         */
        MCORE_INLINE void GetValue(ReturnType* outValue);

        /**
         * Get the storage type value of the keyframe.
         * @param outValue The storage type value of the keyframe.
         */
        MCORE_INLINE void GetStorageTypeValue(StorageType* outValue);

        /**
         * Set the time of the keyframe.
         * @param time The time of the keyframe, in seconds.
         */
        MCORE_INLINE void SetTime(float time);

        /**
         * Set the value.
         * @param value The value.
         */
        MCORE_INLINE void SetValue(const ReturnType& value);

        /**
         * Set the storage type value.
         * @param value The storage type value.
         */
        MCORE_INLINE void SetStorageTypeValue(const StorageType& value);

    protected:
        StorageType mValue; /**< The key value. */
        float       mTime;  /**< Time in seconds. */
    };

    // include inline code
#include "KeyFrame.inl"
} // namespace EMotionFX
