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

#include <Tests/AnimGraphFixture.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphBindPoseNode.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/BlendTreeBlend2Node.h>
#include <EMotionFX/Source/BlendTreeFloatConstantNode.h>
#include <EMotionFX/Source/BlendTreeRangeRemapperNode.h>
#include <EMotionFX/Source/EMotionFXManager.h>

namespace EMotionFX
{
    struct BlendTreeRangeRemapperNodeTestData
    {
        // Range of the Range Remapper Node Input/Output
        float m_minInputFloat;
        float m_maxInputFloat;
        float m_minOutputFloat;
        float m_maxOutputFloat;

        // Input floats for the Range Remapper Node, and expected outputs
        // Generally output is a linear conversion of the input
        // TODO: When min > max input range, output is always maxOutput, create a warning in graph analyzer
        std::vector<float> m_inputFloats;
        std::vector<float> m_expectedOutputs;
    };

    class BlendTreeRangeRemapperNodeFixture
        : public AnimGraphFixture
        , public ::testing::WithParamInterface<BlendTreeRangeRemapperNodeTestData>
    {
    public:
        void ConstructGraph() override
        {
            AnimGraphFixture::ConstructGraph();
            m_param = GetParam();

            /*
                                          +------------+
                                          |bindPoseNode|--+
                                          +------------+  +>+----------+    +---------+
                                                            |blend2Node|--->|finalNode|
            +-----------------+    +-------------------+  +>+----------+    +---------+
            |floatConstantNode|--->|m_RangeRemapperNode|--+
            +-----------------+    +-------------------+
            */
            m_rangeRemapperNode = aznew BlendTreeRangeRemapperNode();
            m_floatConstantNode = aznew BlendTreeFloatConstantNode();
            AnimGraphBindPoseNode* bindPoseNode = aznew AnimGraphBindPoseNode();
            BlendTreeBlend2Node* blend2Node = aznew BlendTreeBlend2Node();
            BlendTreeFinalNode* finalNode = aznew BlendTreeFinalNode();

            m_blendTree = aznew BlendTree();
            m_blendTree->AddChildNode(m_rangeRemapperNode);
            m_blendTree->AddChildNode(m_floatConstantNode);
            m_blendTree->AddChildNode(bindPoseNode);
            m_blendTree->AddChildNode(blend2Node);
            m_blendTree->AddChildNode(finalNode);

            m_rangeRemapperNode->AddConnection(m_floatConstantNode, BlendTreeFloatConstantNode::OUTPUTPORT_RESULT, BlendTreeRangeRemapperNode::INPUTPORT_X);
            blend2Node->AddConnection(bindPoseNode, AnimGraphBindPoseNode::PORTID_OUTPUT_POSE, BlendTreeBlend2Node::INPUTPORT_POSE_A);
            blend2Node->AddConnection(bindPoseNode, AnimGraphBindPoseNode::PORTID_OUTPUT_POSE, BlendTreeBlend2Node::INPUTPORT_POSE_B);
            blend2Node->AddConnection(m_rangeRemapperNode, BlendTreeRangeRemapperNode::OUTPUTPORT_RESULT, BlendTreeBlend2Node::INPUTPORT_WEIGHT);
            finalNode->AddConnection(blend2Node, BlendTreeBlend2Node::OUTPUTPORT_POSE, BlendTreeFinalNode::INPUTPORT_POSE);

            m_animGraph->GetRootStateMachine()->AddChildNode(m_blendTree);
            m_animGraph->GetRootStateMachine()->SetEntryState(m_blendTree);
        }
    protected:
        BlendTree* m_blendTree = nullptr;
        BlendTreeRangeRemapperNode* m_rangeRemapperNode = nullptr;
        BlendTreeFloatConstantNode* m_floatConstantNode = nullptr;
        BlendTreeRangeRemapperNodeTestData m_param;
    };

    TEST_P(BlendTreeRangeRemapperNodeFixture, OutputsCorrectFloatTest)
    {
        // Setup Blend Tree Range Remapper Node's input/output ranges 
        m_rangeRemapperNode->SetInputMin(m_param.m_minInputFloat);
        m_rangeRemapperNode->SetInputMax(m_param.m_maxInputFloat);
        m_rangeRemapperNode->SetOutputMin(m_param.m_minOutputFloat);
        m_rangeRemapperNode->SetOutputMax(m_param.m_maxOutputFloat);

        // Test with input floats and compare outputs with expected outputs
        AZ::u32 outputIndex = 0;
        for (float inputFloat : m_param.m_inputFloats)
        {
            m_floatConstantNode->SetValue(inputFloat);
            GetEMotionFX().Update(1.0f / 60.0f);

            const float actualOutput = m_rangeRemapperNode->GetOutputFloat(m_animGraphInstance,
                BlendTreeRangeRemapperNode::OUTPUTPORT_RESULT)->GetValue();
            const float expectedOutput = m_param.m_expectedOutputs[outputIndex];

            EXPECT_EQ(actualOutput, expectedOutput) << "Expected Output: " << expectedOutput;
            outputIndex++;
        }
    };

    std::vector<BlendTreeRangeRemapperNodeTestData> blendTreeRangeRemapperNodeTestData
    {
        {
            0.0f, 0.0f, 0.0f, 0.0f, {-1.1f, 0.0f, 1.1f}, {0.0f, 0.0f, 0.0f}
        },
        {
            0.0f, 1.0f, 0.0f, 1.0f, {-0.5f, 0.0f, 0.5f, 1.5f}, {0.0f, 0.0f, 0.5f, 1.0f}
        },
        {
            0.0f, 1.0f, -10.0f, 10.0f, {-0.5f, 0.0f, 0.5f, 1.5f}, {-10.0f, -10.0f, 0.0f, 10.0f}
        },
        {
            -10.0f, 10.0f, 0.0f, 1.0f, {-10.5f, 0.0f, 5.5f, 11.5f}, {0.0f, 0.5f, 0.775f, 1.0f}
        },
        {
            5.0f, 0.0f, 0.0f, 5.0f, {-1.5f, 0.0f, 4.5f, 11.5f}, {5.0f, 5.0f, 5.0f, 5.0f}
        },
        {
            0.0f, 5.0f, 5.0f, 0.0f, {-1.5f, 0.0f, 4.5f, 11.5f}, {5.0f, 5.0f, 0.5f, 0.0f}
        },
        {
            5.0f, 0.0f, 5.0f, 0.0f, {-1.5f, 0.0f, 4.5f, 11.5f}, {0.0f, 0.0f, 0.0f, 0.0f}
        },
        {
            -AZ_FLT_MAX, AZ_FLT_MAX, -AZ_FLT_MAX, AZ_FLT_MAX, {-1.5f, 0.0f, 4.5f, 11.5f}, {-1.5f, 0.0f, 4.5f, 11.5f}
        }
    };

    INSTANTIATE_TEST_CASE_P(BlendTreeRangeRemapperNode_ValidOutputTests,
        BlendTreeRangeRemapperNodeFixture,
        ::testing::ValuesIn(blendTreeRangeRemapperNodeTestData)
    );
} // end namespace EMotionFX