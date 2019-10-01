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

#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/random.h>
#include <MCore/Source/ReflectionSerializer.h>
#include "AnimGraphFixture.h"
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/AnimGraphTimeCondition.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/SkeletalMotion.h>
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
            actorBindPose.SetModelSpaceTransform(0, identity);
            m_actor->ResizeTransformData();
            m_actor->PostCreateInit();
        }
        {
            m_motionSet = aznew MotionSet("testMotionSet");
        }
        {
            ConstructGraph();
            m_animGraph->InitAfterLoading();
        }
        {
            m_actorInstance = ActorInstance::Create(m_actor);
            m_animGraphInstance = AnimGraphInstance::Create(m_animGraph, m_actorInstance, m_motionSet);
            m_actorInstance->SetAnimGraphInstance(m_animGraphInstance);
            m_animGraphInstance->IncreaseReferenceCount(); // Two owners now, the test and the actor instance
            m_animGraphInstance->UpdateUniqueData();
        }
    }

    void AnimGraphFixture::ConstructGraph()
    {
        m_animGraph = aznew AnimGraph();
        m_rootStateMachine = aznew AnimGraphStateMachine();
        m_rootStateMachine->SetName("Root");
        m_animGraph->SetRootStateMachine(m_rootStateMachine);
    }

    AZStd::string AnimGraphFixture::SerializeAnimGraph() const
    {
        if (!m_animGraph)
        {
            return AZStd::string();
        }

        return MCore::ReflectionSerializer::Serialize(m_animGraph).GetValue();
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
        return pose->GetModelSpaceTransform(nodeIndex);
    }

    void AnimGraphFixture::AddValueParameter(const AZ::TypeId& typeId, const AZStd::string& name)
    {
        Parameter* parameter = ParameterFactory::Create(typeId);
        parameter->SetName(name);
        m_animGraph->AddParameter(parameter);
        m_animGraphInstance->AddMissingParameterValues();
    }

    AnimGraphStateTransition* AnimGraphFixture::AddTransition(AnimGraphNode* source, AnimGraphNode* target, float time)
    {
        AnimGraphStateMachine* parentSM = azdynamic_cast<AnimGraphStateMachine*>(target->GetParentNode());
        if (!parentSM)
        {
            return nullptr;
        }

        AnimGraphStateTransition* transition = aznew AnimGraphStateTransition();

        transition->SetSourceNode(source);
        transition->SetTargetNode(target);
        transition->SetBlendTime(time);

        parentSM->AddTransition(transition);
        return transition;
    }

    AnimGraphTimeCondition* AnimGraphFixture::AddTimeCondition(AnimGraphStateTransition* transition, float countDownTime)
    {
        AnimGraphTimeCondition* condition = aznew AnimGraphTimeCondition();

        condition->SetCountDownTime(countDownTime);
        transition->AddCondition(condition);

        return condition;
    }

    AnimGraphStateTransition* AnimGraphFixture::AddTransitionWithTimeCondition(AnimGraphNode* source, AnimGraphNode* target, float blendTime, float countDownTime)
    {
        AnimGraphStateTransition* transition = AddTransition(source, target, blendTime);
        AddTimeCondition(transition, countDownTime);
        return transition;
    }

    MotionSet::MotionEntry* AnimGraphFixture::AddMotionEntry(const AZStd::string& motionId, float motionMaxTime)
    {
        SkeletalMotion* motion = SkeletalMotion::Create(motionId.c_str());
        motion->SetMaxTime(motionMaxTime);
        MotionSet::MotionEntry* motionEntry = aznew MotionSet::MotionEntry(motion->GetName(), motion->GetName(), motion);
        m_motionSet->AddMotionEntry(motionEntry);
        return motionEntry;
    }

    void AnimGraphFixture::Simulate(float simulationTime, float expectedFps, float fpsVariance,
            SimulateCallback preCallback,
            SimulateCallback postCallback,
            SimulateFrameCallback preUpdateCallback,
            SimulateFrameCallback postUpdateCallback)
    {
        AZ::SimpleLcgRandom random;
        random.SetSeed(875960);

        const float minFps = expectedFps - fpsVariance / 2.0f;
        const float maxFps = expectedFps + fpsVariance / 2.0f;

        int frame = 0;
        float time = 0.0f;

        preCallback(m_animGraphInstance);

        // Make sure to update at least once and to have a valid internal state and everything is initialized on the first frame.
        preUpdateCallback(m_animGraphInstance, time, 0.0f, frame);
        GetEMotionFX().Update(0.0f);
        postUpdateCallback(m_animGraphInstance, time, 0.0f, frame);

        while (time < simulationTime)
        {
            const float randomFps = fabs(minFps + random.GetRandomFloat() * (maxFps - minFps));
            float timeDelta = 0.0f;
            if (randomFps > 0.1f)
            {
                timeDelta = 1.0f / randomFps;
            }
            time += timeDelta;
            frame++;

            preUpdateCallback(m_animGraphInstance, time, timeDelta, frame);
            GetEMotionFX().Update(timeDelta);
            postUpdateCallback(m_animGraphInstance, time, timeDelta, frame);
        }

        postCallback(m_animGraphInstance);
    }
}