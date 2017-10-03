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
#include "BaseObject.h"


namespace EMotionFX
{
    // forward declaration
    class MotionInstance;
    class ActorInstance;
    class Transform;
    class Pose;
    class Actor;
    class Node;
    class ControlledMotion;


    /**
     * The local spacecontroller base class.
     * These controllers act like motions and allow you to modify the local space transformations.
     * Since they are fully integrated in the motion processing pipeline they naturally provide the ability to
     * perform blending and to control what controllers can overwrite other motions.
     * Be sure to manually call the Activate() method. Otherwise your controller will not have any influence as they are
     * disabled on default.
     */
    class EMFX_API LocalSpaceController
        : public BaseObject
    {
        MCORE_MEMORYOBJECTCATEGORY(LocalSpaceController, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_LOCALCONTROLLERS);

    public:
        /**
         * Returns the actor instance where this controller is working on.
         * @return A pointer to the actor instance where it is working on.
         */
        MCORE_INLINE ActorInstance* GetActorInstance() const                    { return mActorInstance; }

        /**
         * Get the unique controller type ID.
         * @result The controller type identification number.
         */
        virtual uint32 GetType() const = 0;

        /**
         * Get the type identification string.
         * This can be a description or the class name of the controller.
         * @result A pointer to the string containing the name.
         */
        virtual const char* GetTypeString() const = 0;

        /**
         * Clone the local space controller.
         * When you write your own Clone implementation for your own controller, be sure to also call the
         * method named LocalSpaceController::CopyBaseControllerSettings.
         * @param targetActorInstance The actor instance where you will add the cloned local space controller to.
         * @param neverActivate When set to false, the controller activation status will be cloned as well. This means that the
         *                      controller can already be active as well after cloning. When set to true, the clone will never be active after cloning.
         * @result A pointer to an exact clone of the this local space controller.
         */
        virtual LocalSpaceController* Clone(ActorInstance* targetActorInstance, bool neverActivate = false) = 0;

        /**
         * Create the motion links for this controller.
         * This specifies what nodes will be modified by this controller.
         * If this controller would only modify the arm of a human, only the nodes of the arm should have motion links created.
         * When your controller doesn't seem to have any effect, check if you actually setup the motion links properly or if you have
         * activated your controller using the Activate() method.
         * @param actor The actor from which the actor instance that we add this controller to has been created.
         * @param instance The motion instance that contains the playback settings. This contains also a start node that you can get with
         *                 the MotionInstance::GetStartNode() method. You might want to take this into account as well.
         */
        virtual void CreateMotionLinks(Actor* actor, MotionInstance* instance) = 0;

        /**
         * The main update method.
         * This is where the calculations and modifications happen. So this is the method that will apply the
         * modifications to the local space transformations.
         * @param inPose The input local transformations for all nodes. This contains the current blended results.
         * @param outPose The local space transformation buffer in which you output your results. So do not modify the
         *                local space transforms inside the TransformData class, but output your transformations inside
         *                this Pose.
         * @param instance The motion instance object that contains the playback settings.
         */
        virtual void Update(const Pose* inPose, Pose* outPose, MotionInstance* instance) = 0;

        //---------------------------------------------------------------------------

        /**
         * Activate the controller.
         * This will actually start playing a controlled motion. So you will see the number of motion instances increase.
         * If the controller is already active, nothing will happen.
         * @param priorityLevel The motion priority level. This can be used to control what motions/controllers can overwrite what other
         *                      motions or controllers. Motions with higher priorities will overwrite motions with lower priorities.
         * @param fadeInTime The time, in seconds, which it will take to fully blend into activated state. This can be used to smoothly
         *                   blend into the pose after the controller has been applied, rather than suddenly popping into it.
         */
        virtual void Activate(uint32 priorityLevel = 100, float fadeInTime = 0.3f);

        /**
         * Deactivate the controller.
         * This will stop playing the controlled motion that had been created when you used Activate.
         * @param fadeOutTime The fade out time, in seconds. This is used to smoothly fade out the controller.
         */
        virtual void Deactivate(float fadeOutTime = 0.3f);

        //---------------------------------------------------------------------------

        /**
         * Get the motion instance, that contains the playback settings.
         * Also this can be used to control the weight value of the controller.
         * You can use the MotionInstance::SetWeight() method for that.
         * Please beware that the returned motion instance object will be invalid when the controller is not active!
         * @result A pointer to the motion instance. This motion instance can be invalid when the controller is inactive.
         *         You can use the MotionSystem::IsValidMotionInstance() method to determine if the motion instance is valid or not.
         */
        MCORE_INLINE MotionInstance* GetMotionInstance()                        { return mMotionInstance; }

        /**
         * Check if this controller is active or not.
         * @result Returns true when the controller is active. Otherwise false is returned.
         */
        bool GetIsActive() const;

        /**
         * Copy the the class member settings over from another controller.
         * @param sourceController The controller to copy over the settings from.
         * @param neverActivate When set to false, the controller activation status will be cloned as well. This means that the
         *                      controller can already be active as well after cloning. When set to true, the clone will never be active after cloning.
         */
        void CopyBaseControllerSettings(LocalSpaceController* sourceController, bool neverActivate = false);

    protected:
        ControlledMotion*   mControlledMotion;  /**< The controlled motion object. */
        ActorInstance*      mActorInstance;     /**< The actor instance where this controller works on. */
        MotionInstance*     mMotionInstance;    /**< The motion instance. */

        /**
         * The constructor.
         * The controller is set to <b>disabled</b> on default.
         * @param actorInstance The actor instance to apply the node to.
         */
        LocalSpaceController(ActorInstance* actorInstance);

        /**
         * The destructor.
         */
        virtual ~LocalSpaceController();
    };
} // namespace EMotionFX
