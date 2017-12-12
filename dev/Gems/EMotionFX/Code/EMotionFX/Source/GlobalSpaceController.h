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

// include required files
#include "EMotionFXConfig.h"
#include <MCore/Source/Array.h>
#include "BaseObject.h"


namespace EMotionFX
{
    // forward declarations
    class GlobalPose;
    class ActorInstance;


    /**
     * The global space controller base class.
     * Global space controllers are executed after the global space matrices have been calculated after applying
     * motions and local space controllers.
     * You can see them as a post-process pass over the output of an image. An examples of a global space controller
     * can be a physics controller that applies ragdolls to a character by overwriting bone transformations.
     * It is however recommended to use local space controllers, using the LocalSpaceController class instead of using
     * a global space controller. This is because local space controllers are faster and completely integrated in the blending
     * and motion mixing pipeline. Blending in and out global space controllers is also supported, but has a higher performance
     * impact then when using local space controllers.
     */
    class EMFX_API GlobalSpaceController
        : public BaseObject
    {
        MCORE_MEMORYOBJECTCATEGORY(GlobalSpaceController, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_GLOBALCONTROLLERS);

    public:
        /**
         * Get the controller type.
         * This has to be a unique ID per global space controller type/class and can be used to identify
         * what type of controller you are dealing with.
         * @result The unique controller type ID.
         */
        virtual uint32 GetType() const = 0;

        /**
         * Get a description of the global space controller type.
         * This can for example be the name of the class, which can be useful when debugging.
         * @result A string containing the type description of this class.
         */
        virtual const char* GetTypeString() const = 0;

        /**
         * Clone the global space contoller.
         * This creates and returns an exact copy of this controller.
         * Be sure to also use the CopyBaseClassWeightSettings member to give the clone the same weight settings as this controller.
         * @param targetActorInstance The actor instance on which you want to use the cloned controller that will be returned.
         * @result A pointer to a clone of the current controller.
         */
        virtual GlobalSpaceController* Clone(ActorInstance* targetActorInstance) = 0;

        /**
         * The main update method, which performs the modifications and calculations of the new global space matrices.
         * @param outPose The global space output pose where you have to output your calculated global space matrices in.
         *                This output pose is also the input. It will contain the current set of global space matrices for all nodes.
         * @param timePassedInSeconds The time passed, in seconds, since the last update.
         */
        virtual void Update(GlobalPose* outPose, float timePassedInSeconds) = 0;

        /**
         * Activate the controller.
         * This will cause it to fully blend in during a given amount of time.
         * @param fadeInTime The time, in seconds, which it should take to smoothly blend in the controller.
         */
        virtual void Activate(float fadeInTime = 0.3f)                        { SetWeight(1.0f, fadeInTime); }

        /**
         * Deactivate the controller.
         * This will fade out the controller during a given amount of time.
         * When the controller has been deactivated, it will not have any visual impact anymore.
         * Controllers that are inactive (having a weight of 0) will not be processed.
         * @param fadeOutTime The time it should take to smoothly fade out the controller.
         */
        virtual void Deactivate(float fadeOutTime = 0.3f)                     { SetWeight(0.0f, fadeOutTime); }

        /**
         * This method is being triggered when the weight of this controller reaches zero and we are done fading out the controller completely.
         * You can overload this method in case your controller needs to do something specific when this happens.
         */
        virtual void OnFullyDeactivated() {}

        /**
         * This method is being triggered when the weight of this controller reaches a weight of 1.0f, so when we are done with fading in the controller completely.
         * You can overload this method in case your controller needs to do something specific when this happens.
         */
        virtual void OnFullyActivated() {}

        /**
         * Set the weight of the controller.
         * This has to be in range of [0..1]. A value of 0 means the controller is inactive, while a value of 1 means
         * that it is fully active.
         * @param weight The weight value, which has to be in range of [0..1].
         * @param blendTimeInSeconds The time it should take for the controller to blend into the specified weight.
         */
        void SetWeight(float weight, float blendTimeInSeconds);

        /**
         * Get the weight of the controller.
         * This weight value will be anything in range of [0..1] and it secifies how much influence the controller has.
         * A weight of 0 means it has no visual influence, and a weight of 1 means it is fully active and has full influence.
         * @result The weight of the controller, which will be in range of [0..1].
         */
        MCORE_INLINE float GetWeight() const                                    { return mWeight; }

        /**
         * Get the node mask, which describes what nodes will be modified by this controller.
         * Each node can be enabled or disabled in the node mask. This is done with a simple boolean value.
         * The size of the node mask array is equal to the value returned by GetActorInstance()->GetNumNodes().
         * Set the values to true for the nodes that you want to modify inside this controller.
         * Please beware that you have to modify the node mask. If you do not do this, you will not see any visual
         * changes by your controller. On default, after creation of the controller, all nodes are disabled (all set to false).
         * @result A pointer to the array that contains the node mask.
         */
        MCORE_INLINE MCore::Array<bool>* GetNodeMask()                          { return &mNodeMask; }

        /**
         * Get the actor instance where this controller is being applied to.
         * @result A pointer to the actor instance where this controller works on.
         */
        MCORE_INLINE ActorInstance* GetActorInstance()                          { return mActorInstance; }

        /**
         * Check if this controller is active or not.
         * Keep in mind that after calling Deactivate() this method might not directly return false.
         * It will return false when the weight is equal to zero, so when the controller has no influence.
         * @result Returns true when the controller is active. Otherwise false is returned.
         */
        bool GetIsActive() const                                                { return (mWeight > 0.0f); }

        /**
         * Recursively enable or disable a node and all nodes down the hierarchy.
         * So if you apply this to the upper arm node, then the complete arm, hand and fingers will be set with the
         * specified "enable" state.
         * @param startNode The node index of the start node, for example the upper arm.
         * @param enable Set to true when you want to mark the nodes to be modified, otherwise set to false.
         *               After creation of the controller all node mask booleans are set to false. So most likely you
         *               want to use true here.
         */
        void RecursiveSetNodeMask(uint32 startNode, bool enable);

        /**
         * Update the weight value.
         * This is used to perform the automatic weight blending.
         * So this deals with the smoothly interpolating towards the target weight in the specified amount of seconds.
         * @param timePassedInSeconds The time passed, in seconds, since the last update.
         */
        void UpdateWeight(float timePassedInSeconds);

        /**
         * Copy over all weight settings from a specified controller.
         * So this would copy over the weight related member values from the controller you specify as parameter, into this controller.
         * This should be used when cloning your controller. When you would not do this, then the weight values would not
         * be the same and your cloned controller will be in an inactive state and have no visual impact.
         * @param sourceController The source controller from which to copy the weight related member values.
         */
        void CopyBaseClassWeightSettings(GlobalSpaceController* sourceController);

    protected:
        MCore::Array<bool>  mNodeMask; /**< The node mask, which specifies for each node if it is being modified by this controller or not. */                  // TODO: use some BitArray class for this to save mem
        ActorInstance*      mActorInstance;             /**< The actor instance where this controller is working on. */
        float               mWeight;                    /**< The weight value, which is in range of [0..1]. */
        float               mTargetWeight;              /**< The target weight value, in which we want to blend. */
        float               mWeightIncreasePerSecond;   /**< The amount of weight increase per second (used for weight blending). */
        bool                mIsChangingWeight;          /**< A state that specifies if we are still blending into a new weight value or not. */

        /**
         * The constructor.
         * @param actorInstance The actor instance where this controller will work on.
         */
        GlobalSpaceController(ActorInstance* actorInstance);

        /**
         * The destructor.
         */
        virtual ~GlobalSpaceController();
    };
}   // namespace EMotionFX
