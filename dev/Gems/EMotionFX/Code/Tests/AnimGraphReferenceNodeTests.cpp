
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
#include <AzCore/Asset/AssetManager.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphReferenceNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/BlendTreeFinalNode.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <EMotionFX/Source/BlendTreeTransformNode.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Parameter/FloatSliderParameter.h>
#include <EMotionFX/Source/Parameter/ParameterFactory.h>


namespace EMotionFX
{
    // Add a reference node without any asset in it
    class AnimGraphReferenceNodeBaseTests : public AnimGraphFixture
    {
    public:
        void ConstructGraph() override
        {
            AnimGraphFixture::ConstructGraph();

            m_blendTree = aznew BlendTree();
            m_blendTree->SetName("BlendTreeInParentGraph");
            m_animGraph->GetRootStateMachine()->AddChildNode(m_blendTree);
            m_animGraph->GetRootStateMachine()->SetEntryState(m_blendTree);

            m_referenceNode = aznew AnimGraphReferenceNode();
            m_referenceNode->SetName("ReferenceNodeInParentGraph");
            m_blendTree->AddChildNode(m_referenceNode);
            BlendTreeFinalNode* finalNode = aznew BlendTreeFinalNode();
            finalNode->SetName("BlendTreeFinalNodeParentGraph");
            m_blendTree->AddChildNode(finalNode);
            finalNode->AddUnitializedConnection(m_referenceNode, AnimGraphReferenceNode::PORTID_OUTPUT_POSE, BlendTreeFinalNode::PORTID_INPUT_POSE);
        }

        BlendTree* m_blendTree = nullptr;
        AnimGraphReferenceNode* m_referenceNode = nullptr;
    };

    // Basic test that just evaluates the node. Since the node is not doing anything,
    // The pose should not be affected. 
    TEST_F(AnimGraphReferenceNodeBaseTests, Evaluate)
    {
        Evaluate();
        const Transform& outputRoot = GetOutputTransform();
        Transform identity;
        identity.Identity();

        ASSERT_EQ(identity, outputRoot);
    }

    // Add a reference node with an empty asset
    class AnimGraphReferenceNodeWithAssetTests : public AnimGraphReferenceNodeBaseTests
    {
    public:
        void ConstructGraph() override
        {
            AnimGraphReferenceNodeBaseTests::ConstructGraph();

            m_referenceAnimGraph = aznew AnimGraph();
            AZ::Data::Asset<Integration::AnimGraphAsset> animGraphAsset = AZ::Data::AssetManager::Instance().CreateAsset<Integration::AnimGraphAsset>(AZ::Data::AssetId("{E8FBAEF1-CBC5-43C2-83C8-9F8812857494}"));
            animGraphAsset.GetAs<Integration::AnimGraphAsset>()->SetData(m_referenceAnimGraph);

            m_referenceNode->SetAnimGraphAsset(animGraphAsset);

            AnimGraphStateMachine* rootStateMachine = aznew AnimGraphStateMachine();
            m_referenceAnimGraph->SetRootStateMachine(rootStateMachine);
            ConstructReferenceGraph();
            m_referenceAnimGraph->InitAfterLoading();

            m_referenceNode->SetAnimGraph(m_animGraph);
            m_referenceNode->OnAssetReady(animGraphAsset);
        }

        virtual void ConstructReferenceGraph() {}

        AnimGraph* m_referenceAnimGraph = nullptr;
    };

    // Load an empty anim graph into the reference node
    TEST_F(AnimGraphReferenceNodeWithAssetTests, EvaluateEmptyAnimGraphAsset)
    {
        Evaluate();
        const Transform& outputRoot = GetOutputTransform();
        Transform identity;
        identity.Identity();

        ASSERT_EQ(identity, outputRoot);
    }

    // Add a reference node with an asset that contains a blendtree with a transform node
    class AnimGraphReferenceNodeWithContentsTests : public AnimGraphReferenceNodeWithAssetTests
    {
    public:
        void ConstructReferenceGraph() override
        {
            AnimGraphReferenceNodeWithAssetTests::ConstructReferenceGraph();

            BlendTree* blendTree = aznew BlendTree();
            blendTree->SetName("BlendTreeInReferenceGraph");
            m_referenceAnimGraph->GetRootStateMachine()->AddChildNode(blendTree);
            m_referenceAnimGraph->GetRootStateMachine()->SetEntryState(blendTree);

            m_transformNode = aznew BlendTreeTransformNode();
            m_transformNode->SetName("BlendTreeTransformNodeInReferenceGraph");
            blendTree->AddChildNode(m_transformNode);
            BlendTreeFinalNode* finalNode = aznew BlendTreeFinalNode();
            finalNode->SetName("BlendTreeFinalNodeInReferenceGraph");
            blendTree->AddChildNode(finalNode);
            finalNode->AddUnitializedConnection(m_transformNode, BlendTreeTransformNode::PORTID_OUTPUT_POSE, BlendTreeFinalNode::PORTID_INPUT_POSE);

            // Add a parameter to the reference anim graph
            {
                Parameter* parameter = ParameterFactory::Create(azrtti_typeid<FloatSliderParameter>());
                m_referenceAnimGraph->AddParameter(parameter);
            }

            BlendTreeParameterNode* parameterNode = aznew BlendTreeParameterNode();
            parameterNode->SetName("ParameterNodeInReferenceGraph");
            blendTree->AddChildNode(parameterNode);
            m_transformNode->AddUnitializedConnection(parameterNode, 0, BlendTreeTransformNode::PORTID_INPUT_TRANSLATE_AMOUNT);
        }

        void ConstructGraph() override
        {
            AnimGraphReferenceNodeWithAssetTests::ConstructGraph();

            {
                Parameter* parameter = ParameterFactory::Create(azrtti_typeid<FloatSliderParameter>());
                m_animGraph->AddParameter(parameter);
            }

            BlendTreeParameterNode* parameterNode = aznew BlendTreeParameterNode();
            parameterNode->SetName("ParameterNodeInParentGraph");
            m_blendTree->AddChildNode(parameterNode);
            m_referenceNode->AddUnitializedConnection(parameterNode, 0, 0);
        }

        BlendTreeTransformNode* m_transformNode;
    };

    // Just evaluate the node
    TEST_F(AnimGraphReferenceNodeWithContentsTests, Evaluate)
    {
        Evaluate();
        const Transform& outputRoot = GetOutputTransform();
        Transform identity;
        identity.Identity();

        ASSERT_EQ(identity, outputRoot);
    }

} // end namespace EMotionFX
