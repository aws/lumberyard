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

#include <AzCore/Math/PackedVector3.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/AnimGraphTimeCondition.h>
#include <EMotionFX/Source/Mesh.h>
#include <EMotionFX/Source/MotionEventTable.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/SkeletalMotion.h>
#include <EMotionFX/Source/SkeletalSubMotion.h>
#include <EMotionFX/Source/TwoStringEventData.h>
#include <Tests/Printers.h>
#include <Tests/UI/UIFixture.h>

namespace EMotionFX
{
    TEST_F(UIFixture, CanDeleteMotionWhenMotionIsBeingBlended)
    {
        Actor* actor = Actor::Create("testActor");
        Node* rootJoint = Node::Create("rootNode", actor->GetSkeleton());
        rootJoint->SetNodeIndex(0);
        actor->AddNode(rootJoint);

        // Create motions
        SkeletalSubMotion* rootJointXMotion = SkeletalSubMotion::Create();
        rootJointXMotion->CreatePosTrack();
        rootJointXMotion->GetPosTrack()->AddKey(0.0f, AZ::PackedVector3f(0.0f, 0.0f, 0.0f));
        rootJointXMotion->GetPosTrack()->AddKey(1.0f, AZ::PackedVector3f(1.0f, 0.0f, 0.0f));

        SkeletalSubMotion* rootJointYMotion = SkeletalSubMotion::Create();
        rootJointYMotion->CreatePosTrack();
        rootJointYMotion->GetPosTrack()->AddKey(0.0f, AZ::PackedVector3f(0.0f, 0.0f, 0.0f));
        rootJointYMotion->GetPosTrack()->AddKey(1.0f, AZ::PackedVector3f(0.0f, 1.0f, 0.0f));

        SkeletalMotion* xmotion = SkeletalMotion::Create("xmotion");
        xmotion->SetFileName("xmotion.motion");
        xmotion->SetMaxTime(1.0f);
        xmotion->AddSubMotion(rootJointXMotion);
        xmotion->GetEventTable()->AutoCreateSyncTrack(xmotion);
        xmotion->GetEventTable()->GetSyncTrack()->AddEvent(0.0f, GetEventManager().FindOrCreateEventData<TwoStringEventData>("leftFoot", ""));
        xmotion->GetEventTable()->GetSyncTrack()->AddEvent(0.5f, GetEventManager().FindOrCreateEventData<TwoStringEventData>("rightFoot", ""));

        SkeletalMotion* ymotion = SkeletalMotion::Create("ymotion");
        ymotion->SetFileName("ymotion.motion");
        ymotion->SetMaxTime(1.0f);
        ymotion->AddSubMotion(rootJointYMotion);
        ymotion->GetEventTable()->AutoCreateSyncTrack(ymotion);
        ymotion->GetEventTable()->GetSyncTrack()->AddEvent(0.25f, GetEventManager().FindOrCreateEventData<TwoStringEventData>("leftFoot", ""));
        ymotion->GetEventTable()->GetSyncTrack()->AddEvent(0.75f, GetEventManager().FindOrCreateEventData<TwoStringEventData>("rightFoot", ""));

        actor->SetMesh(0, rootJoint->GetNodeIndex(), Mesh::Create());
        actor->ResizeTransformData();
        actor->PostCreateInit(true, true, false);

        // Create anim graph
        AnimGraphMotionNode* motionX = aznew AnimGraphMotionNode();
        motionX->SetName("xmotion");
        motionX->SetMotionIds({"xmotion"});

        AnimGraphMotionNode* motionY = aznew AnimGraphMotionNode();
        motionY->SetName("ymotion");
        motionY->SetMotionIds({"ymotion"});

        AnimGraphTimeCondition* condition = aznew AnimGraphTimeCondition();
        condition->SetCountDownTime(0.0f);

        AnimGraphStateTransition* transition = aznew AnimGraphStateTransition();
        transition->SetSourceNode(motionX);
        transition->SetTargetNode(motionY);
        transition->SetBlendTime(5.0f);
        transition->AddCondition(condition);
        transition->SetSyncMode(AnimGraphStateTransition::SYNCMODE_TRACKBASED);

        AnimGraphStateMachine* rootState = aznew AnimGraphStateMachine();
        rootState->SetEntryState(motionX);
        rootState->AddChildNode(motionX);
        rootState->AddChildNode(motionY);
        rootState->AddTransition(transition);

        AZStd::unique_ptr<AnimGraph> animGraph = AZStd::make_unique<AnimGraph>();
        animGraph->SetRootStateMachine(rootState);

        animGraph->InitAfterLoading();

        // Create Motion Set
        MotionSet::MotionEntry* motionEntryX = aznew MotionSet::MotionEntry(xmotion->GetName(), xmotion->GetName(), xmotion);
        MotionSet::MotionEntry* motionEntryY = aznew MotionSet::MotionEntry(ymotion->GetName(), ymotion->GetName(), ymotion);

        AZStd::unique_ptr<MotionSet> motionSet = AZStd::make_unique<MotionSet>();
        motionSet->SetID(0);
        motionSet->AddMotionEntry(motionEntryX);
        motionSet->AddMotionEntry(motionEntryY);

        // Create ActorInstance
        ActorInstance* actorInstance = ActorInstance::Create(actor);

        // Create AnimGraphInstance
        AnimGraphInstance* animGraphInstance = AnimGraphInstance::Create(animGraph.get(), actorInstance, motionSet.get());
        actorInstance->SetAnimGraphInstance(animGraphInstance);

        GetEMotionFX().Update(0.0f);
        GetEMotionFX().Update(0.5f);
        GetEMotionFX().Update(2.0f);

        EXPECT_THAT(
            rootState->GetActiveStates(animGraphInstance),
            ::testing::Pointwise(::testing::Eq(), AZStd::vector<AnimGraphNode*>{motionX, motionY})
        );

        AZStd::string result;
        EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand("MotionSetRemoveMotion -motionSetID 0 -motionIds xmotion", result)) << result.c_str();
        EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand("RemoveMotion -filename xmotion.motion", result)) << result.c_str();

        GetEMotionFX().Update(0.5f);

        EXPECT_THAT(
            rootState->GetActiveStates(animGraphInstance),
            ::testing::Pointwise(::testing::Eq(), AZStd::vector<AnimGraphNode*>{motionX, motionY})
        );

        actorInstance->Destroy();
        actor->Destroy();
    }
} // namespace EMotionFX
