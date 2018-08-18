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


namespace EMotionFX
{
    // forward declaration
    class MotionInstance;
    class Node;

    /**
     * A motion link.
     * This class links for example nodes with submotions. So that we know what keytracks modify what node.
     */
    class EMFX_API MotionLink
    {
    public:
        /**
         * The default constructor.
         * This disables the motion link on default.
         */
        MCORE_INLINE MotionLink()
            : mSubMotionIndex(MCORE_INVALIDINDEX16)       {}

        /**
         * The extended constructor.
         * When you set the subMotionIndex to anything different than MCORE_INVALIDINDEX16, it will be seen as active.
         */
        MCORE_INLINE MotionLink(uint16 subMotionIndex)
            : mSubMotionIndex(subMotionIndex)        {}

        /**
         * Get the submotion index value.
         * @result The submotion index value.
         */
        MCORE_INLINE uint16 GetSubMotionIndex() const               { return mSubMotionIndex; }

        /**
         * Set the submotion index value.
         * @param index The submotion index value. Anything other than MCORE_INVALIDINDEX16 will activate the link.
         *              The MCORE_INVALIDINDEX16 value will disable the link.
         */
        MCORE_INLINE void SetSubMotionIndex(const uint16 index)     { mSubMotionIndex = index; }

        /**
         * Check if the link is active or not.
         * @result Returns true when the link is active, otherwise false is returned.
         */
        MCORE_INLINE bool GetIsActive() const                       { return (mSubMotionIndex != MCORE_INVALIDINDEX16); }

    private:
        uint16 mSubMotionIndex;     /**< The submotion index. */
    };
} // namespace EMotionFX
