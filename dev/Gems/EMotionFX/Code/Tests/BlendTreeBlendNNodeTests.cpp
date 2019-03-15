
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
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/BlendTreeBlendNNode.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/SkeletalMotion.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/MotionInstance.h>
#include <EMotionFX/Source/Motion.h>
#include <AzCore/std/containers/unordered_map.h>
#include <EMotionFX/Source/Parameter/FloatSliderParameter.h>
#include <EMotionFX/Source/Parameter/ParameterFactory.h>
#include <EMotionFX/Source/Parameter/ValueParameter.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <MCore/Source/Array.h>

namespace EMotionFX
{

    class BlendTreeBlendNNodeTests : public AnimGraphFixture
    {
    public:
        void TearDown() override
        {
            if (m_motionNodes)
            {
                delete m_motionNodes;
            }
            AnimGraphFixture::TearDown();
        }

        void ConstructGraph() override
        {
            AnimGraphFixture::ConstructGraph();
            
            m_blendTree = aznew BlendTree();
            m_animGraph->GetRootStateMachine()->AddChildNode(m_blendTree);
            m_animGraph->GetRootStateMachine()->SetEntryState(m_blendTree);

            m_blendNNode = aznew BlendTreeBlendNNode();
            m_blendTree->AddChildNode(m_blendNNode);
            BlendTreeFinalNode* finalNode = aznew BlendTreeFinalNode();
            m_blendTree->AddChildNode(finalNode);
            finalNode->AddConnection(m_blendNNode, BlendTreeBlendNNode::PORTID_OUTPUT_POSE, BlendTreeFinalNode::PORTID_INPUT_POSE);

            const int motionNodeCount = 3;
            for (int i = 0; i < motionNodeCount; ++i)
            {
                AnimGraphMotionNode* motionNode = aznew AnimGraphMotionNode();
                m_blendTree->AddChildNode(motionNode);
                m_blendNNode->AddConnection(motionNode, AnimGraphMotionNode::PORTID_OUTPUT_POSE, i);
                m_motionNodes->push_back(motionNode);
            }
            m_blendNNode->UpdateParamWeights();
            m_blendNNode->SetParamWeightsEquallyDistributed(-1.0f, 1.0f);

            {
                Parameter* parameter = ParameterFactory::Create(azrtti_typeid<FloatSliderParameter>());
                parameter->SetName("parameter_test");
                m_animGraph->AddParameter(parameter);
            }

            BlendTreeParameterNode* parameterNode = aznew BlendTreeParameterNode();
            m_blendTree->AddChildNode(parameterNode);
            m_blendNNode->AddUnitializedConnection(parameterNode, 0, BlendTreeBlendNNode::INPUTPORT_WEIGHT);
        }

        void SetUp() override
        {
            m_motionNodes = new AZStd::vector<AnimGraphMotionNode*>();
            AnimGraphFixture::SetUp();
            for (size_t i = 0; i < m_motionNodes->size(); ++i)
            {
                // The motion set keeps track of motions by their name. Each motion
                // within the motion set must have a unique name.
                AZStd::string motionId = AZStd::string::format("testSkeletalMotion%i", i);
                SkeletalMotion* motion = SkeletalMotion::Create(motionId.c_str());
                motion->SetMaxTime(1.0f);
                MotionSet::MotionEntry* motionEntry = aznew MotionSet::MotionEntry(motion->GetName(), motion->GetName(), motion);
                m_motionSet->AddMotionEntry(motionEntry);

                (*m_motionNodes)[i]->AddMotionId(motionId.c_str());
            }
        }
        AZStd::vector<AnimGraphMotionNode*>* m_motionNodes = nullptr;
        BlendTreeBlendNNode* m_blendNNode = nullptr;
        BlendTree* m_blendTree = nullptr;
    };

    TEST_F(BlendTreeBlendNNodeTests, RandomizeMotion)
    {
        bool success = true;

        MCore::AttributeFloat* testParameter = m_animGraphInstance->GetParameterValueChecked<MCore::AttributeFloat>(0);
        testParameter->SetValue(-10.0f);
        Evaluate();
        AnimGraphNode* outNodeA;
        AnimGraphNode* outNodeB;
        uint32 outIndexA;
        uint32 outIndexB;
        float  outWeight;
        m_blendNNode->FindBlendNodes(m_animGraphInstance, &outNodeA, &outNodeB, &outIndexA, &outIndexB, &outWeight);
        success = success && outNodeA == outNodeB;
        success = success && outNodeA == static_cast<AnimGraphNode*>(m_motionNodes->front());
        success = success && outWeight <= 0;

        testParameter->SetValue(-1.0f);
        Evaluate();

        m_blendNNode->FindBlendNodes(m_animGraphInstance, &outNodeA, &outNodeB, &outIndexA, &outIndexB, &outWeight);
        success = success && outNodeA == outNodeB;
        success = success && outNodeA == static_cast<AnimGraphNode*>((*m_motionNodes).front());
        success = success && outWeight <= 0.0f;

        testParameter->SetValue(-0.5f);
        Evaluate();
        const float tolerance = 0.001f;

        m_blendNNode->FindBlendNodes(m_animGraphInstance, &outNodeA, &outNodeB, &outIndexA, &outIndexB, &outWeight);
        success = success && outNodeA != outNodeB;
        success = success && outNodeA == static_cast<AnimGraphNode*>(m_motionNodes->front());
        success = success && outNodeB == static_cast<AnimGraphNode*>((*m_motionNodes)[1]);
        success = success && outWeight > 0.5f - tolerance && outWeight < 0.5f + tolerance;

        testParameter->SetValue(0.5f);
        Evaluate();

        m_blendNNode->FindBlendNodes(m_animGraphInstance, &outNodeA, &outNodeB, &outIndexA, &outIndexB, &outWeight);
        success = success && outNodeA != outNodeB;
        success = success && outNodeA == static_cast<AnimGraphNode*>((*m_motionNodes)[1]);
        success = success && outNodeB == static_cast<AnimGraphNode*>((*m_motionNodes)[2]);
        success = success && outWeight > 0.5f - tolerance && outWeight < 0.5f + tolerance;

        testParameter->SetValue(1.0f);
        Evaluate();

        m_blendNNode->FindBlendNodes(m_animGraphInstance, &outNodeA, &outNodeB, &outIndexA, &outIndexB, &outWeight);
        success = success && outNodeA == outNodeB;
        success = success && outNodeA == static_cast<AnimGraphNode*>((*m_motionNodes).back());
        success = success && outWeight <= 0.0f;

        testParameter->SetValue(10.0f);
        Evaluate();

        m_blendNNode->FindBlendNodes(m_animGraphInstance, &outNodeA, &outNodeB, &outIndexA, &outIndexB, &outWeight);
        success = success && outNodeA == outNodeB;
        success = success && outNodeA == static_cast<AnimGraphNode*>(m_motionNodes->back());
        success = success && outWeight <=0;

        ASSERT_TRUE(success);
    }

} // end namespace EMotionFX
