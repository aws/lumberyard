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
#include "Motion.h"


namespace EMotionFX
{
    // forward declarations
    class LocalSpaceController;


    /**
     * The controlled motion class.
     * Controlled motions work together with the LocalSpaceController class.
     * The local space controllers will output their data inside a ControlledMotion.
     * So this type of motion basically helps integrating the local space controllers inside the motion system, as
     * the local space controllers will act like regular motions, through this class.
     */
    class EMFX_API ControlledMotion
        : public Motion
    {
        MCORE_MEMORYOBJECTCATEGORY(ControlledMotion, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_MOTIONS_CONTROLLEDMOTIONS);

    public:
        enum
        {
            TYPE_ID = 0x00000005
        };

        static ControlledMotion* Create(LocalSpaceController* controller);

        /**
         * Returns the type identification number of the motion class.
         * @result The type identification number.
         */
        uint32 GetType() const override                 { return TYPE_ID; }

        /**
         * Gets the type as a description. This for example could be "FacialMotion" or "SkeletalMotion".
         * @result The string containing the type of the motion.
         */
        const char* GetTypeString() const override      { return "ControlledMotion"; }

        /**
         * Calculates the node transformation of the given node for this motion.
         * @param instance The motion instance that contains the motion links to this motion.
         * @param outTransform The node transformation that will be the output of this function.
         * @param actor The actor to apply the motion to.
         * @param node The node to apply the motion to.
         * @param timeValue The time value.
         * @param enableRetargeting Set to true if you like to enable motion retargeting, otherwise set to false.
         */
        void CalcNodeTransform(const MotionInstance* instance, Transform* outTransform, Actor* actor, Node* node, float timeValue, bool enableRetargeting) override { MCORE_UNUSED(instance); MCORE_UNUSED(outTransform); MCORE_UNUSED(actor); MCORE_UNUSED(node); MCORE_UNUSED(timeValue); MCORE_UNUSED(enableRetargeting); }

        /**
         * Creates the motion links inside a given actor.
         * So we know what nodes are effected by what motions, this allows faster updates.
         * @param actor The actor to create the links in.
         * @param instance The motion instance to use for this link.
         */
        void CreateMotionLinks(Actor* actor, MotionInstance* instance) override;

        /**
         * Update the maximum time of this animation. Normally called after loading a motion. The maximum time is
         * estimated by iterating through all motion parts searching the biggest time value.
         */
        void UpdateMaxTime() override;

        /**
         * Returns if this motion type should overwrite body parts that are not touched by a motion
         * when played in non-mixing mode. Skeletal motions would return true, because if you play a non-mixing
         * motion you want the complete body to go into the new motion.
         * However, facial motions only need to adjust the facial bones, so basically they are always played in
         * mixing mode. Still, the behaviour of mixing motions is different than motions that are played in non-mixing
         * mode. If you want to play the motion in non-mixing mode and don't want to reset the nodes that are not influenced
         * by this motion, the motion type should return false.
         * @result Returns true if the motion type resets nodes even when they are not touched by the motion, when playing in non-mix mode.
         *        Otherwise false is returned.
         */
        bool GetDoesOverwriteInNonMixMode() const override;

        /**
         * Make the motion loopable, by adding a new keyframe at the end of the keytracks.
         * This added keyframe will have the same value as the first keyframe.
         * @param fadeTime The relative offset after the last keyframe. If this value is 0.5, it means it will add
         *                 a keyframe half a second after the last keyframe currently in the keytrack.
         */
        void MakeLoopable(float fadeTime = 0.3f) override;

        /**
         * The main update method, which outputs the result for a given motion instance into a given output local pose.
         * @param inPose The current pose, as it is currently in the blending pipeline.
         * @param outPose The output pose, which this motion will modify and write its output into.
         * @param instance The motion instance to calculate the pose for.
         */
        void Update(const Pose* inPose, Pose* outPose, MotionInstance* instance) override;

        /**
         * Get the local space controller that is linked with this motion.
         */
        MCORE_INLINE LocalSpaceController* GetController() const            { return mController; }

    private:
        LocalSpaceController*   mController;    /**< The controller that controls this motion. */

    protected:
        /**
         * The constructor.
         * @param controller The local space controller that is linked with this motion.
         */
        ControlledMotion(LocalSpaceController* controller);

        /**
         * The destructor.
         */
        ~ControlledMotion();
    };
} // namespace EMotionFX
