
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
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/BlendTreeFinalNode.h>
#include <EMotionFX/Source/BlendTreeTransformNode.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Parameter/FloatSliderParameter.h>


namespace EMotionFX
{
    class BlendTreeTransformNodeTests : public AnimGraphFixture
    {
    public:
        void ConstructGraph() override
        {
            AnimGraphFixture::ConstructGraph();

            m_blendTree = aznew BlendTree();
            m_animGraph->GetRootStateMachine()->AddChildNode(m_blendTree);
            m_animGraph->GetRootStateMachine()->SetEntryState(m_blendTree);

            m_transformNode = aznew BlendTreeTransformNode();
            m_blendTree->AddChildNode(m_transformNode);
            BlendTreeFinalNode* finalNode = aznew BlendTreeFinalNode();
            m_blendTree->AddChildNode(finalNode);
            finalNode->AddConnection(m_transformNode, BlendTreeTransformNode::PORTID_OUTPUT_POSE, BlendTreeFinalNode::PORTID_INPUT_POSE);
        }

        BlendTree* m_blendTree;
        BlendTreeTransformNode* m_transformNode;
    };

    // Basic test that just evaluates the node. Since the node is not doing anything,
    // The pose should not be affected. 
    TEST_F(BlendTreeTransformNodeTests, Evaluate)
    {
        Evaluate();
        const Transform& outputRoot = GetOutputTransform();
        Transform identity;
        identity.Identity();

        ASSERT_EQ(identity, outputRoot);
    }

    // Set some values to validate that the node is transforming the root
    TEST_F(BlendTreeTransformNodeTests, EvalauteTranslationBlending)
    {
        m_transformNode->SetTargetNodeName("rootNode");
        m_transformNode->SetMinTranslation(AZ::Vector3::CreateZero());
        m_transformNode->SetMaxTranslation(AZ::Vector3(10.0f, 0.0f, 0.0f));
        m_transformNode->OnUpdateUniqueData(m_animGraphInstance);

        AddValueParameter(azrtti_typeid<FloatSliderParameter>(), "translate_amount");

        BlendTreeParameterNode* parameterNode = aznew BlendTreeParameterNode();
        m_blendTree->AddChildNode(parameterNode);
        parameterNode->InitAfterLoading(m_animGraph);
        parameterNode->OnUpdateUniqueData(m_animGraphInstance);
        m_transformNode->AddConnection(parameterNode, 0, BlendTreeTransformNode::PORTID_INPUT_TRANSLATE_AMOUNT);
        m_animGraph->Reinit();
        
        MCore::AttributeFloat* translate_amount = m_animGraphInstance->GetParameterValueChecked<MCore::AttributeFloat>(0);

        translate_amount->SetValue(0.0f);
        Evaluate();
        Transform outputRoot = GetOutputTransform();
        Transform expected;
        expected.Identity();
        ASSERT_EQ(expected, outputRoot);

        translate_amount->SetValue(0.5f);
        Evaluate();
        expected.mPosition = AZ::Vector3(5.0f, 0.0f, 0.0f);
        outputRoot = GetOutputTransform();
        ASSERT_EQ(expected, outputRoot);

        translate_amount->SetValue(1.0f);
        Evaluate();
        expected.mPosition = AZ::Vector3(10.0f, 0.0f, 0.0f);
        outputRoot = GetOutputTransform();
        ASSERT_EQ(expected, outputRoot);
    }

} // end namespace EMotionFX
