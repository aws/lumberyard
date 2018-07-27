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
#include <MCore/Source/Array.h>


namespace EMotionFX
{
    // forward declarations
    class PlayBackInfo;
    class Motion;
    class MotionInstance;
    class Pose;


    /**
     * The motion group class.
     * This class can group a local set of motions together, which are blended using mixer style weights.
     * The weights of these motions will add up to one and are blended in that way rather than using layers.
     * Often this is very useful when you want to blend for example four different poses for the hand, to simulate some inverse kinematics by doing pose blends.
     * In that case you could create a motion group with those four poses inside it and calculate the weights for each individual motion pose in the group.
     */
    class EMFX_API MotionGroup
        : public BaseObject
    {
        AZ_CLASS_ALLOCATOR_DECL

    public:
        /**
         * The default creation method.
         */
        static MotionGroup* Create();

        /**
         * The extended creation method.
         * @param parentMotionInstance The parent motion instance, which is the motion instance of the group itself.
         */
        static MotionGroup* Create(MotionInstance* parentMotionInstance);

        /**
         * Link this group to a given motion instance.
         * This is also called the parent motion instance sometimes.
         * @param motionInstance The motion instance to which this motion group belongs.
         */
        void LinkToMotionInstance(MotionInstance* motionInstance);

        /**
         * Add a motion to the group.
         * @param motion The motion to add.
         * @param playInfo The playback info to use for this motion.
         * @param startNodeIndex The start node index to use, or MCORE_INVALIDINDEX32 in case there is no start node to be used.
         * @result A pointer to the motion instance for the added motion.
         */
        MotionInstance* AddMotion(Motion* motion, PlayBackInfo* playInfo = nullptr, uint32 startNodeIndex = MCORE_INVALIDINDEX32);

        /**
         * Calculate the output of this group into a pose.
         * @param inPose the input pose.
         * @param outPose The output pose.
         */
        void Output(const Pose* inPose, Pose* outPose);

        /**
         * Update the group.
         * @param timePassed The delta time in seconds.
         */
        void Update(float timePassed);

        /**
         * Remove all motions from the group.
         */
        void RemoveAllMotionInstances();

        /**
         * Remove a given motion instance from the group.
         * @param instance The motion instance to remove.
         */
        void RemoveMotionInstance(MotionInstance* instance);

        /**
         * Remove a motion from the group.
         * @param motion The motion to remove (well, all instances using this motion).
         */
        void RemoveMotion(Motion* motion);

        /**
         * Remove a given motion from the group.
         * @param index The number of the motion to remove, must be in range of [0..GetNumMotions()-1].
         */
        void RemoveMotionInstance(uint32 index);

        /**
         * Get the number of motions inside the group.
         * @result The number of motions.
         */
        MCORE_INLINE uint32 GetNumMotionInstances() const                       { return mMotionInstances.GetLength(); }

        /**
         * Get a given motion instance from the group.
         * @param index The motion number to get the instance for. This must be in range of [0..GetNumMotions()-1].
         */
        MCORE_INLINE MotionInstance* GetMotionInstance(uint32 index) const      { return mMotionInstances[index]; }


    private:
        MCore::Array<MotionInstance*>   mMotionInstances;           /**< The motions inside the group. */
        MotionInstance*                 mParentMotionInstance;      /**< The parent motion instance (the group root motion instance). */

        /**
         * The default constructor.
         */
        MotionGroup();

        /**
         * The extended constructor.
         * @param parentMotionInstance The parent motion instance, which is the motion instance of the group itself.
         */
        MotionGroup(MotionInstance* parentMotionInstance);

        /**
         * The destructor.
         */
        ~MotionGroup();
    };
}   // namespace EMotionFX
