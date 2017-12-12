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
#include "EMotionFXConfig.h"
#include "ControlledMotion.h"
#include "LocalSpaceController.h"
#include "MotionInstance.h"
#include "Actor.h"


namespace EMotionFX
{
    // constructor
    ControlledMotion::ControlledMotion(LocalSpaceController* controller)
        : Motion(nullptr)
    {
        MCORE_ASSERT(controller);
        mController = controller;
    }


    // destructor
    ControlledMotion::~ControlledMotion()
    {
    }


    // create
    ControlledMotion* ControlledMotion::Create(LocalSpaceController* controller)
    {
        return new ControlledMotion(controller);
    }


    // create the motion links
    void ControlledMotion::CreateMotionLinks(Actor* actor, MotionInstance* instance)
    {
        mController->CreateMotionLinks(actor, instance);
    }


    // update the maximum playback time
    void ControlledMotion::UpdateMaxTime()
    {
        mMaxTime = 0.0f;
    }


    // overwrite in non-mixed mode or not?
    bool ControlledMotion::GetDoesOverwriteInNonMixMode() const
    {
        return true;
    }


    // make loopable
    void ControlledMotion::MakeLoopable(float fadeTime)
    {
        MCORE_UNUSED(fadeTime);
    }


    // update the controlled motion
    void ControlledMotion::Update(const Pose* inPose, Pose* outPose, MotionInstance* instance)
    {
        mController->Update(inPose, outPose, instance);
    }
}   // namespace EMotionFX
