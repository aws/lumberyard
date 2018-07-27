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

#include "SystemComponentFixture.h"

namespace EMotionFX
{
    class Actor;
    class ActorInstance;
    class AnimGraph;
    class AnimGraphInstance;
    class AnimGraphMotionNode;
    class AnimGraphStateMachine;
    class AnimGraphStateTransition;
    class AnimGraphObject;
    class MotionSet;

    class AnimGraphTransitionConditionFixture : public SystemComponentFixture
    {
    public:
        void SetUp() override;

        void TearDown() override;

        virtual void AddNodesToAnimGraph()
        {
        }

        AnimGraph* GetAnimGraph() const
        {
            return mAnimGraph;
        }

        AnimGraphInstance* GetAnimGraphInstance() const
        {
            return mAnimGraphInstance;
        }

    protected:
        AnimGraphStateMachine* mStateMachine = nullptr;
        AnimGraphInstance* mAnimGraphInstance = nullptr;
        AnimGraphMotionNode* mMotionNode0 = nullptr;
        AnimGraphMotionNode* mMotionNode1 = nullptr;
        AnimGraphStateTransition* mTransition = nullptr;
        Actor* mActor = nullptr;
        AnimGraph* mAnimGraph = nullptr;
        MotionSet* mMotionSet = nullptr;
        ActorInstance* mActorInstance = nullptr;

    };
}
