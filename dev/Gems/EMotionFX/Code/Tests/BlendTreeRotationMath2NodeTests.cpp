
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
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/BlendTreeGetTransformNode.h>
#include <EMotionFX/Source/BlendTreeSetTransformNode.h>
#include <EMotionFX/Source/AnimGraphBindPoseNode.h>
#include <EMotionFX/Source/BlendTreeRotationMath2Node.h>


namespace EMotionFX
{
    class BlendTreeRotationMath2NodeTests : public AnimGraphFixture
    {
    public:
        void ConstructGraph() override
        {
            AnimGraphFixture::ConstructGraph();

            m_blendTree = aznew BlendTree();
            m_animGraph->GetRootStateMachine()->AddChildNode(m_blendTree);
            m_animGraph->GetRootStateMachine()->SetEntryState(m_blendTree);

            AnimGraphBindPoseNode* bindPoseNode = aznew AnimGraphBindPoseNode();
            m_blendTree->AddChildNode(bindPoseNode);

            m_getTransformNode = aznew BlendTreeGetTransformNode();
            m_blendTree->AddChildNode(m_getTransformNode);

            m_rotationMathNode = aznew BlendTreeRotationMath2Node();
            m_blendTree->AddChildNode(m_rotationMathNode);

            m_setTransformNode = aznew BlendTreeSetTransformNode();
            m_blendTree->AddChildNode(m_setTransformNode);

            m_getTransformNode->AddUnitializedConnection(bindPoseNode, AnimGraphBindPoseNode::PORTID_OUTPUT_POSE, BlendTreeFinalNode::PORTID_INPUT_POSE);
            m_setTransformNode->AddUnitializedConnection(bindPoseNode, AnimGraphBindPoseNode::PORTID_OUTPUT_POSE, BlendTreeFinalNode::PORTID_INPUT_POSE);

            m_rotationMathNode->AddUnitializedConnection(m_getTransformNode, BlendTreeGetTransformNode::OUTPUTPORT_ROTATION, BlendTreeRotationMath2Node::INPUTPORT_X);
            m_setTransformNode->AddUnitializedConnection(m_rotationMathNode, BlendTreeRotationMath2Node::OUTPUTPORT_RESULT_QUATERNION, BlendTreeSetTransformNode::INPUTPORT_ROTATION);


            BlendTreeFinalNode* finalNode = aznew BlendTreeFinalNode();
            m_blendTree->AddChildNode(finalNode);


            finalNode->AddUnitializedConnection(m_setTransformNode, BlendTreeSetTransformNode::PORTID_OUTPUT_POSE, BlendTreeFinalNode::PORTID_INPUT_POSE);
        }

        BlendTree* m_blendTree;
        BlendTreeGetTransformNode* m_getTransformNode;
        BlendTreeRotationMath2Node* m_rotationMathNode;
        BlendTreeSetTransformNode* m_setTransformNode;
    };

    TEST_F(BlendTreeRotationMath2NodeTests, EvalauteTranslationBlending)
    {
        BlendTreeGetTransformNode::UniqueData* getNodeUniqueData = static_cast<BlendTreeGetTransformNode::UniqueData*>(m_getTransformNode->FindUniqueNodeData(m_animGraphInstance));
        getNodeUniqueData->m_nodeIndex = 0;

        BlendTreeSetTransformNode::UniqueData* setNodeUniqueData = static_cast<BlendTreeSetTransformNode::UniqueData*>(m_setTransformNode->FindUniqueNodeData(m_animGraphInstance));
        setNodeUniqueData->m_nodeIndex = 0;

        m_getTransformNode->OnUpdateUniqueData(m_animGraphInstance);
        m_setTransformNode->OnUpdateUniqueData(m_animGraphInstance);

        MCore::Quaternion expectedRotation(0.0f, MCore::Math::pi * 0.25f, 0.0f);

        m_rotationMathNode->SetDefaultValue(expectedRotation);

        Evaluate();
        Transform outputRoot = GetOutputTransform();
        Transform expected;
        expected.Identity();
        expected.Set(AZ::Vector3::CreateZero(), expectedRotation);
        bool success = AZ::IsClose(expected.mRotation.w, outputRoot.mRotation.w, 0.0001f);
        success = success && AZ::IsClose(expected.mRotation.x, outputRoot.mRotation.x, 0.0001f);
        success = success && AZ::IsClose(expected.mRotation.y, outputRoot.mRotation.y, 0.0001f);
        success = success && AZ::IsClose(expected.mRotation.z, outputRoot.mRotation.z, 0.0001f);

        m_rotationMathNode->SetMathFunction(EMotionFX::BlendTreeRotationMath2Node::MATHFUNCTION_INVERSE_MULTIPLY);
        
        expectedRotation.Inverse();
        Evaluate();
        outputRoot = GetOutputTransform();
        expected.Identity();
        expected.Set(AZ::Vector3::CreateZero(), expectedRotation);
        success = success && AZ::IsClose(expected.mRotation.w, outputRoot.mRotation.w, 0.0001f);
        success = success && AZ::IsClose(expected.mRotation.x, outputRoot.mRotation.x, 0.0001f);
        success = success && AZ::IsClose(expected.mRotation.y, outputRoot.mRotation.y, 0.0001f);
        success = success && AZ::IsClose(expected.mRotation.z, outputRoot.mRotation.z, 0.0001f);


        ASSERT_TRUE(success);
    }

} // end namespace EMotionFX
