
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
#include <EMotionFX/Source/BlendTreeVector3DecomposeNode.h>
#include <EMotionFX/Source/BlendTreeVector2DecomposeNode.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/SkeletalMotion.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/MotionInstance.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/Parameter/ParameterFactory.h>
#include <EMotionFX/Source/Parameter/Vector2Parameter.h>
#include <EMotionFX/Source/Parameter/Vector3Parameter.h>
#include <EMotionFX/Source/Parameter/ValueParameter.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <MCore/Source/Array.h>

namespace EMotionFX
{

    class Vector2ToVector3CompatibilityTests : public AnimGraphFixture
    {
    public:
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
            finalNode->AddUnitializedConnection(m_blendNNode, BlendTreeBlendNNode::PORTID_OUTPUT_POSE, BlendTreeFinalNode::PORTID_INPUT_POSE);

            const int motionNodeCount = 3;
            for (int i = 0; i < motionNodeCount; ++i)
            {
                AnimGraphMotionNode* motionNode = aznew AnimGraphMotionNode();
                m_blendTree->AddChildNode(motionNode);
                m_blendNNode->AddUnitializedConnection(motionNode, AnimGraphMotionNode::PORTID_OUTPUT_POSE, i);
                
                // The motion set keeps track of motions by their name. Each motion
                // within the motion set must have a unique name.
                const AZStd::string motionId = AZStd::string::format("testSkeletalMotion%i", i);
                SkeletalMotion* motion = SkeletalMotion::Create(motionId.c_str());
                motion->SetMaxTime(1.0f);
                MotionSet::MotionEntry* motionEntry = aznew MotionSet::MotionEntry(motion->GetName(), motion->GetName(), motion);
                m_motionSet->AddMotionEntry(motionEntry);

                motionNode->AddMotionId(motionId.c_str());
            }

            {
                Parameter* parameter = ParameterFactory::Create(azrtti_typeid<Vector3Parameter>());
                parameter->SetName("parameter_vector3_test");
                m_animGraph->AddParameter(parameter);
            }
            {
                Parameter* parameter = ParameterFactory::Create(azrtti_typeid<Vector2Parameter>());
                parameter->SetName("parameter_vector2_test");
                m_animGraph->AddParameter(parameter);
            }

            BlendTreeParameterNode* parameterNode = aznew BlendTreeParameterNode();
            m_blendTree->AddChildNode(parameterNode);

            m_vector2DecomposeNode = aznew BlendTreeVector2DecomposeNode();
            m_blendTree->AddChildNode(m_vector2DecomposeNode);
            m_vector2DecomposeNode->AddUnitializedConnection(parameterNode, 0, BlendTreeVector2DecomposeNode::INPUTPORT_VECTOR);

            m_vector3DecomposeNode = aznew BlendTreeVector3DecomposeNode();
            m_blendTree->AddChildNode(m_vector3DecomposeNode);
            m_vector3DecomposeNode->AddUnitializedConnection(parameterNode, 1, BlendTreeVector3DecomposeNode::INPUTPORT_VECTOR);

            m_blendNNode->AddUnitializedConnection(m_vector3DecomposeNode, BlendTreeVector3DecomposeNode::OUTPUTPORT_X, BlendTreeBlendNNode::INPUTPORT_WEIGHT);
        }

        BlendTreeBlendNNode* m_blendNNode = nullptr;
        BlendTree* m_blendTree = nullptr;
        BlendTreeVector3DecomposeNode* m_vector3DecomposeNode = nullptr;
        BlendTreeVector2DecomposeNode* m_vector2DecomposeNode = nullptr;
    };

    TEST_F(Vector2ToVector3CompatibilityTests, Evaluation)
    {
        AZ::Outcome<size_t> vector2ParamIndexOutcome = m_animGraph->FindValueParameterIndexByName("parameter_vector2_test");
        ASSERT_TRUE(vector2ParamIndexOutcome.IsSuccess());
        AZ::Outcome<size_t> vector3ParamIndexOutcome = m_animGraph->FindValueParameterIndexByName("parameter_vector3_test");
        ASSERT_TRUE(vector3ParamIndexOutcome.IsSuccess());
        
        MCore::AttributeVector2* testVector2Parameter = static_cast<MCore::AttributeVector2*>(m_animGraphInstance->GetParameterValue(static_cast<uint32>(vector2ParamIndexOutcome.GetValue())));
        testVector2Parameter->SetValue(AZ::Vector2(-1.0f, 0.5f));

        MCore::AttributeVector3* testVector3Parameter = static_cast<MCore::AttributeVector3*>(m_animGraphInstance->GetParameterValue(static_cast<uint32>(vector3ParamIndexOutcome.GetValue())));
        testVector3Parameter->SetValue(AZ::PackedVector3f(1.0f, 2.5f, 3.5f));

        Evaluate();

        MCore::AttributeFloat* attributeFloatX = m_vector3DecomposeNode->GetOutputFloat(m_animGraphInstance, BlendTreeVector3DecomposeNode::OUTPUTPORT_X);
        ASSERT_TRUE(attributeFloatX);
        MCore::AttributeFloat* attributeFloatY = m_vector3DecomposeNode->GetOutputFloat(m_animGraphInstance, BlendTreeVector3DecomposeNode::OUTPUTPORT_Y);
        ASSERT_TRUE(attributeFloatY);
        MCore::AttributeFloat* attributeFloatZ = m_vector3DecomposeNode->GetOutputFloat(m_animGraphInstance, BlendTreeVector3DecomposeNode::OUTPUTPORT_Z);
        ASSERT_TRUE(attributeFloatZ);

        MCore::AttributeFloat* attributeFloatX1 = m_vector2DecomposeNode->GetOutputFloat(m_animGraphInstance, BlendTreeVector2DecomposeNode::OUTPUTPORT_X);
        ASSERT_TRUE(attributeFloatX1);
        MCore::AttributeFloat* attributeFloatY1 = m_vector2DecomposeNode->GetOutputFloat(m_animGraphInstance, BlendTreeVector2DecomposeNode::OUTPUTPORT_Y);
        ASSERT_TRUE(attributeFloatY1);

        const AZ::Vector2& testParameterVector2Value = testVector2Parameter->GetValue();
        const AZ::Vector3 testParameterVector3Value(testVector3Parameter->GetValue());
        const float tolerance = 0.001f;
        float outX = attributeFloatX->GetValue();
        float outY = attributeFloatY->GetValue();
        float outZ = attributeFloatZ->GetValue();
        AZ::Vector3 decomposedValuesVector3(outX, outY, outZ);
        AZ::Vector3 expectedDecomposedValuesVector3(testParameterVector2Value.GetX(), testParameterVector2Value.GetY(), 0.0f);
        float differenceLength = static_cast<float>((expectedDecomposedValuesVector3 - decomposedValuesVector3).GetLength());
        ASSERT_TRUE(AZ::IsClose(0, differenceLength, tolerance));

        m_blendNNode->RemoveConnection(m_vector3DecomposeNode, BlendTreeVector3DecomposeNode::OUTPUTPORT_X, BlendTreeBlendNNode::INPUTPORT_WEIGHT);
        m_blendNNode->AddConnection(m_vector2DecomposeNode, BlendTreeVector3DecomposeNode::OUTPUTPORT_X, BlendTreeBlendNNode::INPUTPORT_WEIGHT);

        Evaluate();

        outX = attributeFloatX1->GetValue();
        outY = attributeFloatY1->GetValue();

        AZ::Vector2 decomposedValuesVector2(outX, outY);
        AZ::Vector2 expectedDecomposedValuesVector2(testParameterVector3Value.GetX(), testParameterVector3Value.GetY());
        differenceLength = static_cast<float>((expectedDecomposedValuesVector2 - decomposedValuesVector2).GetLength());
        ASSERT_TRUE(AZ::IsClose(0, differenceLength, tolerance));
    }

} // end namespace EMotionFX
