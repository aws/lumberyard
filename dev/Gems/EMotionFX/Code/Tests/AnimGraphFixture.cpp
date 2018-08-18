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

#include "AnimGraphFixture.h"
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Transform.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Source/Parameter/ParameterFactory.h>


namespace EMotionFX
{
    void AnimGraphFixture::SetUp()
    {
        SystemComponentFixture::SetUp();

        // This fixture sets up the basic pieces to test animation graphs:
        // 1) will create an actor with a root node (joint) at the origin
        // 2) will create an empty animation graph
        // 3) will create an empty motion set
        // 4) will instantiate the actor and animation graph
        {
            m_actor = Actor::Create("testActor");
            Node* rootNode = Node::Create("rootNode", m_actor->GetSkeleton());
            m_actor->AddNode(rootNode);
            m_actor->GetSkeleton()->UpdateNodeIndexValues(0);
            Pose& actorBindPose = *m_actor->GetBindPose();
            EMotionFX::Transform identity;
            identity.Identity();
            actorBindPose.SetGlobalTransform(0, identity);
        }
        {
            ConstructGraph();
            m_animGraph->InitAfterLoading();
        }
        {
            m_motionSet = aznew MotionSet("testMotionSet");
        }
        {
            m_actorInstance = ActorInstance::Create(m_actor);
            m_animGraphInstance = AnimGraphInstance::Create(m_animGraph, m_actorInstance, m_motionSet);
            m_actorInstance->SetAnimGraphInstance(m_animGraphInstance);
            m_animGraphInstance->UpdateUniqueData();
        }
    }

    void AnimGraphFixture::ConstructGraph()
    {
        m_animGraph = aznew AnimGraph();
        AnimGraphStateMachine* rootStateMachine = aznew AnimGraphStateMachine();
        m_animGraph->SetRootStateMachine(rootStateMachine);
    }

    void AnimGraphFixture::TearDown()
    {
        if (m_animGraphInstance)
        {
            m_animGraphInstance->Destroy();
            m_animGraphInstance = nullptr;
        }
        if (m_actorInstance)
        {
            m_actorInstance->Destroy();
            m_actorInstance = nullptr;
        }
        if (m_motionSet)
        {
            delete m_motionSet;
            m_motionSet = nullptr;
        }
        if (m_animGraph)
        {
            delete m_animGraph;
            m_animGraph = nullptr;
        }
        if (m_actor)
        {
            m_actor->Destroy();
            m_actor = nullptr;
        }

        SystemComponentFixture::TearDown();
    }

    void AnimGraphFixture::Evaluate()
    {
        m_actorInstance->UpdateTransformations(0.0);
    }

    const Transform& AnimGraphFixture::GetOutputTransform(uint32 nodeIndex)
    {
        const Pose* pose = m_actorInstance->GetTransformData()->GetCurrentPose();
        return pose->GetGlobalTransform(nodeIndex);
    }

    void AnimGraphFixture::AddValueParameter(const AZ::TypeId& typeId, const AZStd::string& name)
    {
        Parameter* parameter = ParameterFactory::Create(typeId);
        m_animGraph->AddParameter(parameter);
        m_animGraphInstance->AddMissingParameterValues();
    }
}
