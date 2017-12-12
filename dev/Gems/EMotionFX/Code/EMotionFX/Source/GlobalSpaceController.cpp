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

// include required headers
#include "GlobalSpaceController.h"
#include "ActorInstance.h"
#include "Node.h"
#include "GlobalPose.h"


namespace EMotionFX
{
    // constructor
    GlobalSpaceController::GlobalSpaceController(ActorInstance* actorInstance)
        : BaseObject()
    {
        MCORE_ASSERT(actorInstance);

        mNodeMask.SetMemoryCategory(EMFX_MEMCATEGORY_GLOBALCONTROLLERS);

        mWeightIncreasePerSecond = 0.0f;
        mWeight             = 0.0f;
        mTargetWeight       = 0.0f;
        mIsChangingWeight   = false;
        mActorInstance      = actorInstance;

        // disable all nodes on default
        const uint32 numNodes = actorInstance->GetActor()->GetSkeleton()->GetNumNodes();
        mNodeMask.Resize(numNodes);
        for (uint32 i = 0; i < numNodes; ++i) // TODO: use MCore::MemSet here?
        {
            mNodeMask[i] = false;
        }
    }


    // destructor
    GlobalSpaceController::~GlobalSpaceController()
    {
    }


    // set the weight of the controller
    void GlobalSpaceController::SetWeight(float weight, float blendTimeInSeconds)
    {
        MCORE_ASSERT(weight >= 0.0f && weight <= 1.0f); // the weight may range only from 0 to 1
        MCORE_ASSERT(blendTimeInSeconds >= 0.0f);       // only zero or positive values allowed

        // if we update the weight immediately
        if (blendTimeInSeconds < MCore::Math::epsilon)
        {
            mWeight                     = weight;
            mTargetWeight               = weight;
            mWeightIncreasePerSecond    = 0.0f;
            mIsChangingWeight           = false;

            if (mWeight < 0.00001f)
            {
                OnFullyDeactivated();
            }

            if (mWeight > 0.99999f)
            {
                OnFullyActivated();
            }

            return;
        }

        // setup the automatic blending values
        mTargetWeight               = weight;
        mIsChangingWeight           = true;
        mWeightIncreasePerSecond    = (mTargetWeight - mWeight) / blendTimeInSeconds;
    }


    // update the weight value
    void GlobalSpaceController::UpdateWeight(float timePassedInSeconds)
    {
        // if we are not blending weights, don't do anything
        if (mIsChangingWeight == false)
        {
            return;
        }

        // change the weight value
        mWeight += mWeightIncreasePerSecond * timePassedInSeconds;

        // check if we reached the target weight or not
        if (mWeightIncreasePerSecond > 0.0f)
        {
            if (mWeight >= mTargetWeight)
            {
                mWeight = mTargetWeight;
                mIsChangingWeight = false;

                // trigger the callback when we fully activated
                if (mWeight > 0.999999f)
                {
                    OnFullyActivated();
                }
            }
        }
        else
        {
            if (mWeight <= mTargetWeight)
            {
                mWeight = mTargetWeight;
                mIsChangingWeight = false;

                // trigger the callback when we fully deactivated
                if (mWeight < 0.00001f)
                {
                    OnFullyDeactivated();
                }
            }
        }
    }


    // recursively enable or disable a given node and all nodes down the hierarchy
    void GlobalSpaceController::RecursiveSetNodeMask(uint32 startNode, bool enable)
    {
        // enable or disable the current node
        mNodeMask[startNode] = enable;

        // recurse down the hierarchy
        Node* node = mActorInstance->GetActor()->GetSkeleton()->GetNode(startNode);
        const uint32 numChildNodes = node->GetNumChildNodes();
        for (uint32 i = 0; i < numChildNodes; ++i)
        {
            RecursiveSetNodeMask(node->GetChildIndex(i), enable);
        }
    }


    // copy the base class weight properties
    void GlobalSpaceController::CopyBaseClassWeightSettings(GlobalSpaceController* sourceController)
    {
        mWeight             = sourceController->mWeight;
        mTargetWeight       = sourceController->mTargetWeight;
        mIsChangingWeight   = sourceController->mIsChangingWeight;
        mWeightIncreasePerSecond = sourceController->mWeightIncreasePerSecond;
    }
}   // namespace EMotionFX
