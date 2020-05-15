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

#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/BlendTreeFinalNode.h>
#include <EMotionFX/Source/BlendTreeFloatConstantNode.h>
#include <EMotionFX/Source/BlendTreeRagdollNode.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <Tests/AnimGraphFixture.h>

namespace EMotionFX
{
    // Tests if the activation input port can be controlled with a non-boolean/float values from a const float node.
    class BlendTreeRagdollNode_ConstFloatActivateInputTest
        : public AnimGraphFixture
        , public ::testing::WithParamInterface<float>
    {
    public:
        void ConstructGraph() override
        {
            AnimGraphFixture::ConstructGraph();
            m_blendTreeAnimGraph = AnimGraphFactory::Create<OneBlendTreeNodeAnimGraph>();
            m_rootStateMachine = m_blendTreeAnimGraph->GetRootStateMachine();
            BlendTree* blendTree = m_blendTreeAnimGraph->GetBlendTreeNode();

            /*
                +-------------+    +---------+    +------------+
                | Const Float |--->| Ragdoll |--->| Final Node |
                +-------------+    +---------+    +------------+
            */
            BlendTreeFloatConstantNode* floatConstNode = aznew BlendTreeFloatConstantNode();
            floatConstNode->SetValue(GetParam());
            blendTree->AddChildNode(floatConstNode);

            m_ragdollNode = aznew BlendTreeRagdollNode();
            blendTree->AddChildNode(m_ragdollNode);

            BlendTreeFinalNode* finalNode = aznew BlendTreeFinalNode();
            blendTree->AddChildNode(finalNode);

            m_ragdollNode->AddUnitializedConnection(floatConstNode, BlendTreeFloatConstantNode::PORTID_OUTPUT_RESULT, BlendTreeRagdollNode::PORTID_ACTIVATE);
            finalNode->AddUnitializedConnection(m_ragdollNode, BlendTreeRagdollNode::PORTID_OUTPUT_POSE, BlendTreeFinalNode::PORTID_INPUT_POSE);
            m_blendTreeAnimGraph->InitAfterLoading();
        }

        void SetUp() override
        {
            AnimGraphFixture::SetUp();
            m_animGraphInstance->Destroy();
            m_animGraphInstance = m_blendTreeAnimGraph->GetAnimGraphInstance(m_actorInstance, m_motionSet);
        }

        AZStd::unique_ptr<OneBlendTreeNodeAnimGraph> m_blendTreeAnimGraph;
        BlendTreeRagdollNode* m_ragdollNode = nullptr;
    };

    TEST_P(BlendTreeRagdollNode_ConstFloatActivateInputTest, BlendTreeRagdollNode_ConstFloatActivateInputTest)
    {
        GetEMotionFX().Update(0.0f);

        const float constFloatValue = GetParam();
        const bool isActivated = m_ragdollNode->IsActivated(m_animGraphInstance);

        EXPECT_EQ(!MCore::Math::IsFloatZero(constFloatValue), isActivated)
            << "Activation expected in case const float value is not zero.";
    }

    INSTANTIATE_TEST_CASE_P(BlendTreeRagdollNode_ConstFloatActivateInputTest,
        BlendTreeRagdollNode_ConstFloatActivateInputTest,
        ::testing::ValuesIn({ -1.0f, 0.0f, 0.1f, 1.0f }));
} // namespace EMotionFX
