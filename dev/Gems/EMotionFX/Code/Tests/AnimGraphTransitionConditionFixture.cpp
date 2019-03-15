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

#include "AnimGraphTransitionConditionFixture.h"
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphMotionCondition.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/MotionInstance.h>
#include <EMotionFX/Source/MotionEventTable.h>
#include <EMotionFX/Source/MotionEventTrack.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/SkeletalMotion.h>
#include <EMotionFX/Source/TwoStringEventData.h>

namespace EMotionFX
{
    void AnimGraphTransitionConditionFixture::SetUp()
    {
        SystemComponentFixture::SetUp();

        // This test sets up an anim graph with 2 motions, each of which is 1
        // second long. There is a transition from the first to the second that
        // triggers when the first is complete, and takes 0.5 seconds to
        // transition, and an identical one going the other way. During the
        // transition, the weights of the motion states should add up to 1.
        mActor = Actor::Create("testActor");
        Node* rootNode = Node::Create("rootNode", mActor->GetSkeleton());
        mActor->AddNode(rootNode);
        mActor->ResizeTransformData();
        mActor->PostCreateInit();

        mAnimGraph = aznew AnimGraph();

        mStateMachine = aznew AnimGraphStateMachine();
        mStateMachine->SetName("rootStateMachine");
        mAnimGraph->SetRootStateMachine(mStateMachine);

        mMotionNode0 = aznew AnimGraphMotionNode();
        mMotionNode1 = aznew AnimGraphMotionNode();

        mTransition = aznew AnimGraphStateTransition();
        mTransition->SetSourceNode(mMotionNode0);
        mTransition->SetTargetNode(mMotionNode1);

        mStateMachine->AddChildNode(mMotionNode0);
        mStateMachine->AddChildNode(mMotionNode1);
        mStateMachine->SetEntryState(mMotionNode0);
        mStateMachine->AddTransition(mTransition);

        mMotionSet = aznew MotionSet("testMotionSet");
        for (int nodeIndex = 0; nodeIndex < 2; ++nodeIndex)
        {
            // The motion set keeps track of motions by their name. Each motion
            // within the motion set must have a unique name.
            AZStd::string motionId = AZStd::string::format("testSkeletalMotion%i", nodeIndex);
            SkeletalMotion* motion = SkeletalMotion::Create(motionId.c_str());
            motion->SetMaxTime(1.0f);

            // 0.73 seconds would trigger frame 44 when sampling at 60 fps. The
            // event will be seen as triggered inside a motion condition, but a
            // frame later, at frame 45.
            motion->GetEventTable()->AddTrack(MotionEventTrack::Create("TestEventTrack", motion));
            AZStd::shared_ptr<const TwoStringEventData> data = EMotionFX::GetEventManager().FindOrCreateEventData<TwoStringEventData>("TestEvent", "TestParameter");
            AZStd::shared_ptr<const TwoStringEventData> rangeData = EMotionFX::GetEventManager().FindOrCreateEventData<TwoStringEventData>("TestRangeEvent", "TestParameter");
            motion->GetEventTable()->FindTrackByName("TestEventTrack")->AddEvent(0.73f, data);
            motion->GetEventTable()->FindTrackByName("TestEventTrack")->AddEvent(0.65f, 0.95f, rangeData);

            MotionSet::MotionEntry* motionEntry = aznew MotionSet::MotionEntry(motion->GetName(), motion->GetName(), motion);
            mMotionSet->AddMotionEntry(motionEntry);

            AnimGraphMotionNode* motionNode = nodeIndex == 0 ? mMotionNode0 : mMotionNode1;
            motionNode->SetName(motionId.c_str());
            motionNode->AddMotionId(motionId.c_str());

            // Disable looping of the motion nodes.
            motionNode->SetLoop(false);
        }

        // Allow subclasses to create any additional nodes before the anim
        // graph is activated
        AddNodesToAnimGraph();

        mAnimGraph->InitAfterLoading();

        mActorInstance = ActorInstance::Create(mActor);

        mAnimGraphInstance = AnimGraphInstance::Create(mAnimGraph, mActorInstance, mMotionSet);
        mActorInstance->SetAnimGraphInstance(mAnimGraphInstance);
        mAnimGraphInstance->UpdateUniqueData();
    }

    void AnimGraphTransitionConditionFixture::TearDown()
    {
        delete mAnimGraph;
        delete mMotionSet;
        if (mActorInstance)
        {
            mActorInstance->Destroy();
        }
        if (mActor)
        {
            mActor->Destroy();
        }
        SystemComponentFixture::TearDown();
    }
}
