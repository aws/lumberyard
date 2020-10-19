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

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <EMotionFX/Source/Parameter/ParameterFactory.h>
#include <EMotionFX/Source/MotionEventTrack.h>
#include <EMotionFX/Source/MotionEventTable.h>
#include <EMotionFX/Source/SkeletalMotion.h>
#include <EMotionFX/Source/TwoStringEventData.h>
#include <Tests/BlendSpaceFixture.h>

namespace EMotionFX
{
    void BlendSpaceFixture::ConstructGraph()
    {
        AnimGraphFixture::ConstructGraph();

        m_blendTree = aznew BlendTree();
        m_rootStateMachine->AddChildNode(m_blendTree);
        m_rootStateMachine->SetEntryState(m_blendTree);

        m_finalNode = aznew BlendTreeFinalNode();
        m_blendTree->AddChildNode(m_finalNode);
    }

    void BlendSpaceFixture::AddEvent(SkeletalMotion* motion, float time)
    {
        MotionEventTrack* eventTrack = MotionEventTrack::Create(motion);
        AZStd::shared_ptr<const TwoStringEventData> eventData = EMotionFX::GetEventManager().FindOrCreateEventData<TwoStringEventData>("TestEvent", motion->GetName());
        eventTrack->AddEvent(time, eventData);
        motion->GetEventTable()->AddTrack(eventTrack);
    }

    SkeletalSubMotion* BlendSpaceFixture::CreateSubMotion(SkeletalMotion* motion, const std::string& name, const Transform& transform)
    {
        SkeletalSubMotion* subMotion = SkeletalSubMotion::Create(name.c_str());

        subMotion->SetBindPosePos(transform.mPosition);
        subMotion->SetBindPoseRot(transform.mRotation);
        subMotion->SetPosePos(transform.mPosition);
        subMotion->SetPoseRot(transform.mRotation);

        motion->AddSubMotion(subMotion);
        return subMotion;
    }

    void BlendSpaceFixture::CreateMotions()
    {
        // Idle motion
        {
            SkeletalMotion* motion = SkeletalMotion::Create("Idle");
            SkeletalSubMotion* subMotion = CreateSubMotion(motion, m_rootJointName.c_str(), Transform::CreateIdentity());

            subMotion->CreatePosTrack();
            auto posTrack = subMotion->GetPosTrack();
            posTrack->AddKey(0.0f, AZ::Vector3(0.0f, 0.0f, 0.0f));
            posTrack->AddKey(1.0f, AZ::Vector3(0.0f, 0.0f, 0.0f));

            m_motions.emplace_back(motion);
            m_idleMotion = motion;
        }

        // Forward motion
        {
            SkeletalMotion* motion = SkeletalMotion::Create("Forward");
            SkeletalSubMotion* subMotion = CreateSubMotion(motion, m_rootJointName.c_str(), Transform::CreateIdentity());

            subMotion->CreatePosTrack();
            auto posTrack = subMotion->GetPosTrack();
            posTrack->AddKey(0.0f, AZ::Vector3(0.0f, 0.0f, 0.0f));
            posTrack->AddKey(1.0f, AZ::Vector3(0.0f, 1.0f, 0.0f));

            m_motions.emplace_back(motion);
            m_forwardMotion = motion;
        }

        // Run motion
        {
            SkeletalMotion* motion = SkeletalMotion::Create("Run");
            SkeletalSubMotion* subMotion = CreateSubMotion(motion, m_rootJointName.c_str(), Transform::CreateIdentity());

            subMotion->CreatePosTrack();
            auto posTrack = subMotion->GetPosTrack();
            posTrack->AddKey(0.0f, AZ::Vector3(0.0f, 0.0f, 0.0f));
            posTrack->AddKey(1.0f, AZ::Vector3(0.0f, 2.0f, 0.0f));

            m_motions.emplace_back(motion);
            m_runMotion = motion;
        }

        // Strafe motion
        {
            SkeletalMotion* motion = SkeletalMotion::Create("Strafe");
            SkeletalSubMotion* subMotion = CreateSubMotion(motion, m_rootJointName.c_str(), Transform::CreateIdentity());

            subMotion->CreatePosTrack();
            auto posTrack = subMotion->GetPosTrack();
            posTrack->AddKey(0.0f, AZ::Vector3(0.0f, 0.0f, 0.0f));
            posTrack->AddKey(1.0f, AZ::Vector3(1.0f, 0.0f, 0.0f));

            m_motions.emplace_back(motion);
            m_strafeMotion = motion;
        }

        // Rotate left motion
        {
            SkeletalMotion* motion = SkeletalMotion::Create("Rotate Left");
            SkeletalSubMotion* subMotion = CreateSubMotion(motion, m_rootJointName.c_str(), Transform::CreateIdentity());

            subMotion->CreateRotTrack();
            auto rotTrack = subMotion->GetRotTrack();
            rotTrack->AddKey(0.0f, AZ::Quaternion::CreateIdentity());
            rotTrack->AddKey(1.0f, AZ::Quaternion::CreateRotationZ(0.5f));

            m_motions.emplace_back(motion);
            m_rotateLeftMotion = motion;
        }

        // Forward strafe 45 deg
        {
            SkeletalMotion* motion = SkeletalMotion::Create("Forward strafe 45 deg");
            SkeletalSubMotion* subMotion = CreateSubMotion(motion, m_rootJointName.c_str(), Transform::CreateIdentity());

            subMotion->CreatePosTrack();
            auto posTrack = subMotion->GetPosTrack();
            posTrack->AddKey(0.0f, AZ::Vector3(0.0f, 0.0f, 0.0f));
            posTrack->AddKey(1.0f, AZ::Vector3(1.0f, 1.0f, 0.0f));

            m_motions.emplace_back(motion);
            m_forwardStrafe45 = motion;
        }

        // Forward slope 45 deg
        {
            SkeletalMotion* motion = SkeletalMotion::Create("Forward slope 45 deg");
            SkeletalSubMotion* subMotion = CreateSubMotion(motion, m_rootJointName.c_str(), Transform::CreateIdentity());

            subMotion->CreatePosTrack();
            auto posTrack = subMotion->GetPosTrack();
            posTrack->AddKey(0.0f, AZ::Vector3(0.0f, 0.0f, 0.0f));
            posTrack->AddKey(1.0f, AZ::Vector3(0.0f, 1.0f, 1.0f));

            m_motions.emplace_back(motion);
            m_forwardSlope45 = motion;
        }

        for (SkeletalMotion* motion : m_motions)
        {
            AddEvent(motion, 0.1f);

            motion->UpdateMaxTime();

            MotionSet::MotionEntry* motionEntry = aznew MotionSet::MotionEntry(motion->GetName(), motion->GetName(), motion);
            m_motionSet->AddMotionEntry(motionEntry);
        }
    }

    MotionSet::MotionEntry* BlendSpaceFixture::FindMotionEntry(Motion* motion) const
    {
        MotionSet::MotionEntry* entry = m_motionSet->FindMotionEntry(motion);
        EXPECT_NE(entry, nullptr) << "Cannot find motion entry for motion " << motion->GetName() << ".";
        return entry;
    }

    void BlendSpaceFixture::SetUp()
    {
        AnimGraphFixture::SetUp();

        m_eventHandler = aznew TestEventHandler();
        EMotionFX::GetEventManager().AddEventHandler(m_eventHandler);
        
        Node* rootJoint = m_actor->GetSkeleton()->GetNode(0);
        m_rootJointName = rootJoint->GetName();
        m_actor->SetMotionExtractionNode(rootJoint);

        CreateMotions();
    }

    void BlendSpaceFixture::TearDown()
    {
        for (SkeletalMotion* motion : m_motions)
        {
            motion->Destroy();
        }
        m_motions.clear();

        EMotionFX::GetEventManager().RemoveEventHandler(m_eventHandler);
        delete m_eventHandler;

        AnimGraphFixture::TearDown();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    void BlendSpace1DFixture::ConstructGraph()
    {
        BlendSpaceFixture::ConstructGraph();

        m_blendSpace1DNode = aznew BlendSpace1DNode();
        m_blendTree->AddChildNode(m_blendSpace1DNode);

        m_floatNodeX = aznew BlendTreeFloatConstantNode();
        m_blendTree->AddChildNode(m_floatNodeX);

        m_finalNode->AddConnection(m_blendSpace1DNode, BlendSpace1DNode::PORTID_OUTPUT_POSE, BlendTreeFinalNode::PORTID_INPUT_POSE);
        m_blendSpace1DNode->AddConnection(m_floatNodeX, BlendTreeFloatConstantNode::OUTPUTPORT_RESULT, BlendSpace1DNode::INPUTPORT_VALUE);
    }

    void BlendSpace1DFixture::SetUp()
    {
        BlendSpaceFixture::SetUp();

        m_blendSpace1DNode->SetEvaluatorType(azrtti_typeid<BlendSpaceMoveSpeedParamEvaluator>());
        m_blendSpace1DNode->SetMotions({
            FindMotionEntry(m_idleMotion)->GetId(),
            FindMotionEntry(m_forwardMotion)->GetId(),
            FindMotionEntry(m_runMotion)->GetId()
            });

        m_blendSpace1DNode->Reinit();
        GetEMotionFX().Update(0.0f);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    void BlendSpace2DFixture::ConstructGraph()
    {
        BlendSpaceFixture::ConstructGraph();

        m_blendSpace2DNode = aznew BlendSpace2DNode();
        m_blendTree->AddChildNode(m_blendSpace2DNode);

        m_floatNodeX = aznew BlendTreeFloatConstantNode();
        m_blendTree->AddChildNode(m_floatNodeX);

        m_floatNodeY = aznew BlendTreeFloatConstantNode();
        m_blendTree->AddChildNode(m_floatNodeY);

        m_finalNode->AddConnection(m_blendSpace2DNode, BlendSpace2DNode::PORTID_OUTPUT_POSE, BlendTreeFinalNode::PORTID_INPUT_POSE);
        m_blendSpace2DNode->AddConnection(m_floatNodeX, BlendTreeFloatConstantNode::OUTPUTPORT_RESULT, BlendSpace2DNode::INPUTPORT_XVALUE);
        m_blendSpace2DNode->AddConnection(m_floatNodeY, BlendTreeFloatConstantNode::OUTPUTPORT_RESULT, BlendSpace2DNode::INPUTPORT_YVALUE);
    }

    void BlendSpace2DFixture::SetUp()
    {
        BlendSpaceFixture::SetUp();

        m_blendSpace2DNode->SetEvaluatorTypeX(azrtti_typeid<BlendSpaceMoveSpeedParamEvaluator>());
        m_blendSpace2DNode->SetEvaluatorTypeY(azrtti_typeid<BlendSpaceLeftRightVelocityParamEvaluator>());
        m_blendSpace2DNode->SetMotions({
            FindMotionEntry(m_idleMotion)->GetId(),
            FindMotionEntry(m_forwardMotion)->GetId(),
            FindMotionEntry(m_runMotion)->GetId(),
            FindMotionEntry(m_strafeMotion)->GetId()
            });

        m_blendSpace2DNode->Reinit();
        GetEMotionFX().Update(0.0f);
    }
}
