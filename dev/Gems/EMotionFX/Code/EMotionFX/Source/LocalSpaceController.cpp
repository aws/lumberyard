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
#include "LocalSpaceController.h"
#include "MotionSystem.h"
#include "ActorInstance.h"
#include "MotionInstance.h"
#include "MotionManager.h"
#include "ControlledMotion.h"


namespace EMotionFX
{
    // constructor
    LocalSpaceController::LocalSpaceController(ActorInstance* actorInstance)
        : BaseObject()
    {
        MCORE_ASSERT(actorInstance);

        // init the members
        mActorInstance      = actorInstance;
        mMotionInstance     = nullptr;

        // create the controlled motion that will be used for this local space controller
        mControlledMotion = ControlledMotion::Create(this);

        // remove the motion from the motion manager as we only want to control it from inside this local space controller
        GetMotionManager().RemoveMotionByID(mControlledMotion->GetID(), false);
    }


    // destructor
    LocalSpaceController::~LocalSpaceController()
    {
        // de-activate, and instantly remove the motion instance
        Deactivate(0.0f);

        // delete the controlled motion
        mControlledMotion->Destroy();
    }


    // check if the controlled motion is playing or not
    bool LocalSpaceController::GetIsActive() const
    {
        return mActorInstance->GetMotionSystem()->CheckIfIsValidMotionInstance(mMotionInstance);
    }


    // activate the controller
    void LocalSpaceController::Activate(uint32 priorityLevel, float fadeInTime)
    {
        // do nothing when there is already a motion instance
        if (mMotionInstance)
        {
            return;
        }

        // get the motion system
        //MotionSystem* motionSystem = mActorInstance->GetMotionSystem();

        // validate if this motion instance is still playing
        //if (motionSystem->IsValidMotionInstance( mMotionInstance ))
        //return;

        // setup the playback info
        PlayBackInfo info;
        info.mBlendInTime   = fadeInTime;
        info.mPriorityLevel = priorityLevel;
        info.mMix           = true; // when a controller overwrites a controller, keep all controllers still active

        // play the motion
        mMotionInstance = mActorInstance->GetMotionSystem()->PlayMotion(mControlledMotion, &info);
    }


    // deactivate the controller
    void LocalSpaceController::Deactivate(float fadeOutTime)
    {
        // if the controller has already been deactivated or isn't active yet, do nothing
        if (mMotionInstance == nullptr)
        {
            return;
        }

        // get the motion system
        MotionSystem* motionSystem = mActorInstance->GetMotionSystem();

        // validate if this motion instance is still playing
        if (motionSystem->CheckIfIsValidMotionInstance(mMotionInstance) == false)
        {
            mMotionInstance = nullptr;
            return;
        }

        // stop the motion instance
        if (fadeOutTime < MCore::Math::epsilon)
        {
            motionSystem->RemoveMotion(mMotionInstance);
        }
        else
        {
            mMotionInstance->SetFadeTime(fadeOutTime);
            mMotionInstance->Stop();
        }

        // act like the motion instance doesn't exist anymore (it might still be fading out though)
        mMotionInstance = nullptr;
    }


    // copy over the base settings
    void LocalSpaceController::CopyBaseControllerSettings(LocalSpaceController* sourceController, bool neverActivate)
    {
        // if we should never activate the controller
        if (neverActivate)
        {
            return;
        }

        // if the controller isn't active, do nothing
        if (sourceController->GetIsActive() == false)
        {
            return;
        }

        // start the motion
        const uint32 priorityLevel = sourceController->GetMotionInstance()->GetPriorityLevel();
        Activate(priorityLevel, 0.3f);
    }
} // namespace EMotionFX
