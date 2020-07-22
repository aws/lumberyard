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
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/BlendTreeFloatConstantNode.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Importer/Importer.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionInstance.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Parameter/BoolParameter.h>
#include <EMotionFX/Source/Parameter/FloatSliderParameter.h>
#include <EMotionFX/Source/Skeleton.h>
#include <EMotionFX/Source/SkeletalMotion.h>
#include <EMotionFX/Source/TransformData.h>
#include <Tests/JackGraphFixture.h>

namespace EMotionFX
{
    class AnimGraphMotionNodeFixture
        : public JackGraphFixture
    {
    public:
        void ConstructGraph() override
        {
            JackGraphFixture::ConstructGraph();
            m_jackSkeleton = m_actor->GetSkeleton();
            SetupIndices();
            SetupMirrorNodes();
            m_jackPose = m_actorInstance->GetTransformData()->GetCurrentPose();

            // Motion of Jack walking forward (Y-axis change) with right arm aiming towards front.
            AddMotionData("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",
                "jack_walk_forward_aim_zup");

            /*
              Blend tree in animgraph:
              +---------------+                         
              |m_parameterNode|---+                     
              +---------------+   |    +------------+       +---------+
                                  +--->|m_motionNode|------>|finalNode|
                                  +--->|            |       +---------+
              +---------------+   |    +------------+
              |m_fltConstNode |---+                  
              +---------------+                      
            */
            AddParameter<BoolParameter>("InPlace", false);

            BlendTreeFinalNode* finalNode = aznew BlendTreeFinalNode();
            m_fltConstNode = aznew BlendTreeFloatConstantNode();
            m_paramNode = aznew BlendTreeParameterNode();
            m_motionNode = aznew AnimGraphMotionNode();

            // Control motion and effects to be used.
            m_motionNode->AddMotionId("jack_walk_forward_aim_zup");
            m_motionNode->SetLoop(false);
            m_motionNode->SetRetarget(false);
            m_motionNode->SetReverse(false);
            m_motionNode->SetEmitEvents(false);
            m_motionNode->SetMirrorMotion(false);
            m_motionNode->SetMotionExtraction(false);

            m_blendTree = aznew BlendTree();
            m_blendTree->AddChildNode(m_motionNode);
            m_blendTree->AddChildNode(m_paramNode);
            m_blendTree->AddChildNode(m_fltConstNode);
            m_blendTree->AddChildNode(finalNode);

            m_animGraph->GetRootStateMachine()->AddChildNode(m_blendTree);
            m_animGraph->GetRootStateMachine()->SetEntryState(m_blendTree);

            finalNode->AddConnection(m_motionNode, AnimGraphMotionNode::OUTPUTPORT_POSE, BlendTreeFinalNode::INPUTPORT_POSE);
        }

        void AddMotionData(const AZStd::string& base64MotionData, const AZStd::string& motionId)
        {
            AZStd::vector<AZ::u8> skeletalMotiondata;
            AzFramework::StringFunc::Base64::Decode(skeletalMotiondata, base64MotionData.c_str(), base64MotionData.size());
            Motion* newMotion = EMotionFX::GetImporter().LoadSkeletalMotion(skeletalMotiondata.begin(), skeletalMotiondata.size());
            EMotionFX::MotionSet::MotionEntry* newMotionEntry = aznew EMotionFX::MotionSet::MotionEntry();
            newMotionEntry->SetMotion(newMotion);
            m_motionSet->AddMotionEntry(newMotionEntry);
            m_motionSet->SetMotionEntryId(newMotionEntry, motionId);
        }

        template <class paramType, class inputType>
        void ParamSetValue(const AZStd::string& paramName, const inputType& value)
        {
            const AZ::Outcome<size_t> parameterIndex = m_animGraphInstance->FindParameterIndex(paramName);
            MCore::Attribute* param = m_animGraphInstance->GetParameterValue(static_cast<AZ::u32>(parameterIndex.GetValue()));
            paramType* typeParam = static_cast<paramType*>(param);
            typeParam->SetValue(value);
        }

        bool PositionsAreMirrored(const AZ::Vector3& leftPos, const AZ::Vector3& rightPos, float tolerance)
        {
            if (!(leftPos.GetX()).IsClose(rightPos.GetX().GetAbs(), tolerance))
            {
                return false;
            }
            if (!leftPos.GetY().IsClose(rightPos.GetY(), tolerance))
            {
                return false;
            }
            if (!leftPos.GetZ().IsClose(rightPos.GetZ(), tolerance))
            {
                return false;
            }
            return true;
        }

    protected:
        AZ::u32 m_l_handIndex;
        AZ::u32 m_l_loArmIndex;
        AZ::u32 m_l_loLegIndex;
        AZ::u32 m_l_ankleIndex;
        AZ::u32 m_r_handIndex;
        AZ::u32 m_r_loArmIndex;
        AZ::u32 m_r_loLegIndex;
        AZ::u32 m_r_ankleIndex;
        AZ::u32 m_jack_rootIndex;
        AZ::u32 m_bip01__pelvisIndex;
        AnimGraphMotionNode* m_motionNode = nullptr;
        BlendTree* m_blendTree = nullptr;
        BlendTreeFloatConstantNode* m_fltConstNode = nullptr;
        BlendTreeParameterNode* m_paramNode = nullptr;
        Pose * m_jackPose = nullptr;
        Skeleton* m_jackSkeleton = nullptr;

    private:
        template<class ParameterType, class ValueType>
        void AddParameter(const AZStd::string& name, const ValueType& defaultValue)
        {
            ParameterType* parameter = aznew ParameterType();
            parameter->SetName(name);
            parameter->SetDefaultValue(defaultValue);
            m_animGraph->AddParameter(parameter);
        }

        void SetupIndices()
        {
            Node* rootNode = m_jackSkeleton->FindNodeAndIndexByName("jack_root", m_jack_rootIndex);
            Node* pelvisNode = m_jackSkeleton->FindNodeAndIndexByName("Bip01__pelvis", m_bip01__pelvisIndex);
            Node* lHandNode = m_jackSkeleton->FindNodeAndIndexByName("l_hand", m_l_handIndex);
            Node* lLoArmNode = m_jackSkeleton->FindNodeAndIndexByName("l_loArm", m_l_loArmIndex);
            Node* lLoLegNode = m_jackSkeleton->FindNodeAndIndexByName("l_loLeg", m_l_loLegIndex);
            Node* lAnkleNode = m_jackSkeleton->FindNodeAndIndexByName("l_ankle", m_l_ankleIndex);
            Node* rHandNode = m_jackSkeleton->FindNodeAndIndexByName("r_hand", m_r_handIndex);
            Node* rLoArmNode = m_jackSkeleton->FindNodeAndIndexByName("r_loArm", m_r_loArmIndex);
            Node* rLoLegNode = m_jackSkeleton->FindNodeAndIndexByName("r_loLeg", m_r_loLegIndex);
            Node* rAnkleNode = m_jackSkeleton->FindNodeAndIndexByName("r_ankle", m_r_ankleIndex);

            // Make sure all nodes exist.
            ASSERT_TRUE(rootNode && pelvisNode && lHandNode && lLoArmNode && lLoLegNode && lAnkleNode
            && rHandNode && rLoArmNode && rLoLegNode && rAnkleNode) << "All nodes used should exist.";
        }

        void SetupMirrorNodes()
        {
            m_actor->AllocateNodeMirrorInfos();
            m_actor->GetNodeMirrorInfo(m_l_handIndex).mSourceNode = static_cast<uint16>(m_r_handIndex);
            m_actor->GetNodeMirrorInfo(m_r_handIndex).mSourceNode = static_cast<uint16>(m_l_handIndex);
            m_actor->GetNodeMirrorInfo(m_l_loArmIndex).mSourceNode = static_cast<uint16>(m_r_loArmIndex);
            m_actor->GetNodeMirrorInfo(m_r_loArmIndex).mSourceNode = static_cast<uint16>(m_l_loArmIndex);
            m_actor->GetNodeMirrorInfo(m_l_loLegIndex).mSourceNode = static_cast<uint16>(m_r_loLegIndex);
            m_actor->GetNodeMirrorInfo(m_r_loLegIndex).mSourceNode = static_cast<uint16>(m_l_loLegIndex);
            m_actor->GetNodeMirrorInfo(m_l_ankleIndex).mSourceNode = static_cast<uint16>(m_r_ankleIndex);
            m_actor->GetNodeMirrorInfo(m_r_ankleIndex).mSourceNode = static_cast<uint16>(m_l_ankleIndex);
            m_actor->AutoDetectMirrorAxes();
        }
    };

    TEST_F(AnimGraphMotionNodeFixture, NoInputAndZeroEffectOutputsCorrectMotionAndPose)
    {
        AnimGraphMotionNode::UniqueData* uniqueData = static_cast<AnimGraphMotionNode::UniqueData*>(m_animGraphInstance->FindOrCreateUniqueNodeData(m_motionNode));
        
        // Check position of root and pelvis to ensure actor's motion movement is correct.
        // Follow-through during the duration(~1.06666672 seconds) of the motion.
        for (float i = 0.1f; i < 1.2f; i += 0.1f)
        {
            const AZ::Vector3 rootCurrentPos = m_jackPose->GetModelSpaceTransform(m_jack_rootIndex).mPosition;
            const AZ::Vector3 pelvisCurrentPos = m_jackPose->GetModelSpaceTransform(m_bip01__pelvisIndex).mPosition;
            GetEMotionFX().Update(1.0f / 10.0f);
            const AZ::Vector3 rootUpdatedPos = m_jackPose->GetModelSpaceTransform(m_jack_rootIndex).mPosition;
            const AZ::Vector3 pelvisUpdatedPos = m_jackPose->GetModelSpaceTransform(m_bip01__pelvisIndex).mPosition;
            const float rootDifference = rootUpdatedPos.GetY() - rootCurrentPos.GetY();
            const float pelvisDifference = pelvisUpdatedPos.GetY() - pelvisCurrentPos.GetY();

            EXPECT_TRUE(rootUpdatedPos.GetY() > rootCurrentPos.GetY()) << "Y-axis position of root should increase.";
            EXPECT_TRUE(pelvisUpdatedPos.GetY() > pelvisCurrentPos.GetY()) << "Y-axis position of pelvis should increase.";
            EXPECT_TRUE(rootDifference == pelvisDifference) << "Movement of root and pelvis should be the same.";
        }
    };

    TEST_F(AnimGraphMotionNodeFixture, NoInputAndLoopOutputsCorrectMotionAndPose)
    {
        AnimGraphMotionNode::UniqueData* uniqueData = static_cast<AnimGraphMotionNode::UniqueData*>(m_animGraphInstance->FindOrCreateUniqueNodeData(m_motionNode));
        uniqueData->mReload = true;
        m_motionNode->SetLoop(true);
        m_motionNode->InvalidateUniqueData(m_animGraphInstance);
        m_actorInstance->SetMotionExtractionEnabled(false);
        EXPECT_TRUE(m_motionNode->GetIsLooping()) << "Loop effect should be on.";
        GetEMotionFX().Update(0.0f); // Needed to trigger a refresh of motion node internals.

        // Update to half the motion's duration.
        AZ::Vector3 rootStartPos = m_jackPose->GetModelSpaceTransform(m_jack_rootIndex).mPosition;
        AZ::Vector3 pelvisStartPos = m_jackPose->GetModelSpaceTransform(m_bip01__pelvisIndex).mPosition;
        const float duration = m_motionNode->GetDuration(m_animGraphInstance);
        const float offset = duration * 0.5f;
        GetEMotionFX().Update(offset);
        EXPECT_FLOAT_EQ(uniqueData->GetCurrentPlayTime(), offset);
        AZ::Vector3 rootCurrentPos = m_jackPose->GetModelSpaceTransform(m_jack_rootIndex).mPosition;
        AZ::Vector3 pelvisCurrentPos = m_jackPose->GetModelSpaceTransform(m_bip01__pelvisIndex).mPosition;
        EXPECT_TRUE(rootCurrentPos.GetY() > rootStartPos.GetY()) << "Y-axis position of root should increase.";
        EXPECT_TRUE(pelvisCurrentPos.GetY() > pelvisStartPos.GetY()) << "Y-axis position of pelvis should increase.";

        // Update so that we cause a loop till 10% in the motion playback time.
        rootStartPos = rootCurrentPos;
        pelvisStartPos = pelvisCurrentPos;
        GetEMotionFX().Update(duration * 0.6f);
        EXPECT_FLOAT_EQ(uniqueData->GetCurrentPlayTime(), duration * 0.1f);
        rootCurrentPos = m_jackPose->GetModelSpaceTransform(m_jack_rootIndex).mPosition;
        pelvisCurrentPos = m_jackPose->GetModelSpaceTransform(m_bip01__pelvisIndex).mPosition;
        EXPECT_TRUE(rootCurrentPos.GetY() < rootStartPos.GetY()) << "Y-axis position of root should increase.";
        EXPECT_TRUE(pelvisCurrentPos.GetY() < pelvisStartPos.GetY()) << "Y-axis position of pelvis should increase.";
    };

    TEST_F(AnimGraphMotionNodeFixture, NoInputAndReverseOutputsCorrectMotionAndPose)
    {
        m_motionNode->SetReverse(true);
        AnimGraphMotionNode::UniqueData* uniqueData = static_cast<AnimGraphMotionNode::UniqueData*>(m_animGraphInstance->FindOrCreateUniqueNodeData(m_motionNode));
        uniqueData->mReload = true;
        GetEMotionFX().Update(1.1f);

        EXPECT_TRUE(m_motionNode->GetIsReversed()) << "Reverse effect should be on.";

        // Check position of root and pelvis to ensure actor's motion movement is reversed.
        // Follow-through during the duration(~1.06666672 seconds) of the motion.
        for (float i = 0.1f; i < 1.2f; i += 0.1f)
        {
            const AZ::Vector3 rootCurrentPos = m_jackPose->GetModelSpaceTransform(m_jack_rootIndex).mPosition;
            const AZ::Vector3 pelvisCurrentPos = m_jackPose->GetModelSpaceTransform(m_bip01__pelvisIndex).mPosition;
            GetEMotionFX().Update(1.0f / 10.0f);
            const AZ::Vector3 rootUpdatedPos = m_jackPose->GetModelSpaceTransform(m_jack_rootIndex).mPosition;
            const AZ::Vector3 pelvisUpdatedPos = m_jackPose->GetModelSpaceTransform(m_bip01__pelvisIndex).mPosition;
            const float rootDifference =  rootCurrentPos.GetY() - rootUpdatedPos.GetY();
            const float pelvisDifference = pelvisCurrentPos.GetY() - pelvisUpdatedPos.GetY();

            EXPECT_TRUE(rootUpdatedPos.GetY() < rootCurrentPos.GetY()) << "Y-axis position of root should decrease.";
            EXPECT_TRUE(pelvisUpdatedPos.GetY() < pelvisCurrentPos.GetY()) << "Y-axis position of pelvis should decrease.";
            EXPECT_TRUE(rootDifference == pelvisDifference) << "Movement of root and pelvis should be the same.";
        }
    };

    TEST_F(AnimGraphMotionNodeFixture, NoInputAndMirrorMotionOutputsCorrectMotionAndPose)
    {
        AnimGraphMotionNode::UniqueData* uniqueData = static_cast<AnimGraphMotionNode::UniqueData*>(m_animGraphInstance->FindOrCreateUniqueNodeData(m_motionNode));
        uniqueData->mReload = true;
        GetEMotionFX().Update(1.0f);

        // Get positions before mirroring to compare with mirrored positions later.
        const AZ::Vector3 l_handCurrentPos = m_jackPose->GetModelSpaceTransform(m_l_handIndex).mPosition;
        const AZ::Vector3 l_loArmCurrentPos = m_jackPose->GetModelSpaceTransform(m_l_loArmIndex).mPosition;
        const AZ::Vector3 l_loLegCurrentPos = m_jackPose->GetModelSpaceTransform(m_l_loLegIndex).mPosition;
        const AZ::Vector3 l_ankleCurrentPos = m_jackPose->GetModelSpaceTransform(m_l_ankleIndex).mPosition;
        const AZ::Vector3 r_handCurrentPos = m_jackPose->GetModelSpaceTransform(m_r_handIndex).mPosition;
        const AZ::Vector3 r_loArmCurrentPos = m_jackPose->GetModelSpaceTransform(m_r_loArmIndex).mPosition;
        const AZ::Vector3 r_loLegCurrentPos = m_jackPose->GetModelSpaceTransform(m_r_loLegIndex).mPosition;
        const AZ::Vector3 r_ankleCurrentPos = m_jackPose->GetModelSpaceTransform(m_r_ankleIndex).mPosition;

        m_motionNode->SetMirrorMotion(true);
        uniqueData->mReload = true;
        GetEMotionFX().Update(0.0001f);

        EXPECT_TRUE(m_motionNode->GetMirrorMotion()) << "Mirror motion effect should be on.";

        const AZ::Vector3 l_handMirroredPos = m_jackPose->GetModelSpaceTransform(m_l_handIndex).mPosition;
        const AZ::Vector3 l_loArmMirroredPos = m_jackPose->GetModelSpaceTransform(m_l_loArmIndex).mPosition;
        const AZ::Vector3 l_loLegMirroredPos = m_jackPose->GetModelSpaceTransform(m_l_loLegIndex).mPosition;
        const AZ::Vector3 l_ankleMirroredPos = m_jackPose->GetModelSpaceTransform(m_l_ankleIndex).mPosition;
        const AZ::Vector3 r_handMirroredPos = m_jackPose->GetModelSpaceTransform(m_r_handIndex).mPosition;
        const AZ::Vector3 r_loArmMirroredPos = m_jackPose->GetModelSpaceTransform(m_r_loArmIndex).mPosition;
        const AZ::Vector3 r_loLegMirroredPos = m_jackPose->GetModelSpaceTransform(m_r_loLegIndex).mPosition;
        const AZ::Vector3 r_ankleMirroredPos = m_jackPose->GetModelSpaceTransform(m_r_ankleIndex).mPosition;

        EXPECT_TRUE(PositionsAreMirrored(l_handCurrentPos, r_handMirroredPos, 0.001f)) << "Actor's left hand should be mirrored to right hand.";
        EXPECT_TRUE(PositionsAreMirrored(l_handMirroredPos, r_handCurrentPos, 0.001f)) << "Actor's right hand should be mirrored to left hand.";
        EXPECT_TRUE(PositionsAreMirrored(l_loArmCurrentPos, r_loArmMirroredPos, 0.001f)) << "Actor's left lower arm should be mirrored to right lower arm.";
        EXPECT_TRUE(PositionsAreMirrored(l_loArmMirroredPos, r_loArmCurrentPos, 0.001f)) << "Actor's right lower arm should be mirrored to left lower arm.";
        EXPECT_TRUE(PositionsAreMirrored(l_loLegCurrentPos, r_loLegMirroredPos, 0.001f)) << "Actor's left lower leg should be mirrored to right lower leg.";
        EXPECT_TRUE(PositionsAreMirrored(l_loLegMirroredPos, r_loLegCurrentPos, 0.001f)) << "Actor's right lower leg should be mirrored to left lower leg.";
        EXPECT_TRUE(PositionsAreMirrored(l_ankleCurrentPos, r_ankleMirroredPos, 0.001f)) << "Actor's left ankle should be mirrored to right ankle.";
        EXPECT_TRUE(PositionsAreMirrored(l_ankleMirroredPos, r_ankleCurrentPos, 0.001f)) << "Actor's right ankle should be mirrored to left ankle.";
    };

    TEST_F(AnimGraphMotionNodeFixture, InPlaceInputAndNoEffectOutputsCorrectMotionAndPose)
    {
        m_motionNode->AddConnection(m_paramNode, m_paramNode->FindOutputPortByName("InPlace")->mPortID, AnimGraphMotionNode::INPUTPORT_INPLACE);
        ParamSetValue<MCore::AttributeBool, bool>("InPlace", true);
        AnimGraphMotionNode::UniqueData* uniqueData = static_cast<AnimGraphMotionNode::UniqueData*>(m_animGraphInstance->FindOrCreateUniqueNodeData(m_motionNode));
        
        GetEMotionFX().Update(1.0f / 60.0f);

        EXPECT_TRUE(m_motionNode->GetIsInPlace(m_animGraphInstance)) << "In Place effect should be on.";

        // Check position of root and pelvis to ensure actor's motion movement is staying in place.
        // Follow-through during the duration(~1.06666672 seconds) of the motion.
        for (float i = 0.1f; i < 1.2f; i += 0.1f)
        {
            const AZ::Vector3 rootCurrentPos = m_jackPose->GetModelSpaceTransform(m_jack_rootIndex).mPosition;
            const AZ::Vector3 pelvisCurrentPos = m_jackPose->GetModelSpaceTransform(m_bip01__pelvisIndex).mPosition;
            const AZ::Vector3 lankleCurrentPos = m_jackPose->GetModelSpaceTransform(m_l_ankleIndex).mPosition;
            const AZ::Vector3 rankleCurrentPos = m_jackPose->GetModelSpaceTransform(m_r_ankleIndex).mPosition;
            GetEMotionFX().Update(1.0f / 10.0f);
            const AZ::Vector3 rootUpdatedPos = m_jackPose->GetModelSpaceTransform(m_jack_rootIndex).mPosition;
            const AZ::Vector3 pelvisUpdatedPos = m_jackPose->GetModelSpaceTransform(m_bip01__pelvisIndex).mPosition;
            const AZ::Vector3 lankleUpdatedPos = m_jackPose->GetModelSpaceTransform(m_l_ankleIndex).mPosition;
            const AZ::Vector3 rankleUpdatedPos = m_jackPose->GetModelSpaceTransform(m_r_ankleIndex).mPosition;
            EXPECT_TRUE(m_motionNode->GetIsInPlace(m_animGraphInstance)) << "InPlace flag of the motion node should be true.";
            EXPECT_TRUE(rootUpdatedPos.IsClose(rootCurrentPos, 0.0f)) << "Position of root should not change.";           
            EXPECT_TRUE(pelvisCurrentPos != pelvisUpdatedPos) << "Position of pelvis should change.";
            EXPECT_TRUE(lankleCurrentPos != lankleUpdatedPos) << "Position of left ankle should change.";
            EXPECT_TRUE(rankleCurrentPos != rankleUpdatedPos) << "Position of right ankle should change.";
        }
    };

    TEST_F(AnimGraphMotionNodeFixture, PlaySpeedInputAndPlaySpeedEffectOutputsCorrectMotionAndPose)
    {
        // Connect motion node's PlaySpeed input port with a float constant node for control.
        m_fltConstNode->SetValue(1.0f);
        BlendTreeConnection* playSpeedConnection = m_motionNode->AddConnection(m_fltConstNode,
            BlendTreeFloatConstantNode::OUTPUTPORT_RESULT, AnimGraphMotionNode::INPUTPORT_PLAYSPEED);

        AnimGraphMotionNode::UniqueData* uniqueData = static_cast<AnimGraphMotionNode::UniqueData*>(m_animGraphInstance->FindOrCreateUniqueNodeData(m_motionNode));
        GetEMotionFX().Update(1.0f / 60.0f);

        // Root node's initial position under the first speed factor.
        AZ::Vector3 rootInitialPosUnderSpeed1 = m_jackPose->GetModelSpaceTransform(m_jack_rootIndex).mPosition;
        uniqueData->mReload = true;
        GetEMotionFX().Update(1.1f);
        
        // Root node's final position under the first speed factor.
        AZ::Vector3 rootFinalPosUnderSpeed1 = m_jackPose->GetModelSpaceTransform(m_jack_rootIndex).mPosition;
        std::vector<float> speedFactors = { 2.0f, 3.0f, 10.0f, 100.0f };
        std::vector<float> playTimes = { 0.6f, 0.4f, 0.11f, 0.011f };
        for (AZ::u32 i = 0; i < 4; i++)
        {
            m_motionNode->Rewind(m_animGraphInstance);
            m_fltConstNode->SetValue(speedFactors[i]);
            GetEMotionFX().Update(1.0f / 60.0f);

            uniqueData->mReload = true;
            const AZ::Vector3 rootInitialPosUnderSpeed2 = m_jackPose->GetModelSpaceTransform(m_jack_rootIndex).mPosition;

            // Faster play speed requires less play time to reach its final pose.
            GetEMotionFX().Update(playTimes[i]);
            const AZ::Vector3 rootFinalPosUnderSpeed2 = m_jackPose->GetModelSpaceTransform(m_jack_rootIndex).mPosition;

            EXPECT_TRUE(rootInitialPosUnderSpeed1.IsClose(rootInitialPosUnderSpeed2, 0.0f)) << "Root initial position should be same in different motion speeds.";
            EXPECT_TRUE(rootFinalPosUnderSpeed1.IsClose(rootFinalPosUnderSpeed2, 0.0f)) << "Root final position should be same in different motion speeds.";

            // Update positions to the new speed for comparing with the next speed factor.
            rootInitialPosUnderSpeed1 = rootInitialPosUnderSpeed2;
            rootFinalPosUnderSpeed1 = rootFinalPosUnderSpeed2;
        }

        // Disconnect PlaySpeed port.
        // Check playspeed control through motion node's own method SetPlaySpeed().
        m_motionNode->RemoveConnection(playSpeedConnection);
        m_motionNode->Rewind(m_animGraphInstance);
        m_motionNode->SetMotionPlaySpeed(1.0f);
        GetEMotionFX().Update(1.0f / 60.0f);

        rootInitialPosUnderSpeed1 = m_jackPose->GetModelSpaceTransform(m_jack_rootIndex).mPosition;
        uniqueData->mReload = true;
        GetEMotionFX().Update(1.1f);
        rootFinalPosUnderSpeed1 = m_jackPose->GetModelSpaceTransform(m_jack_rootIndex).mPosition;

        // Similar test to using the InPlace input port.
        for (AZ::u32 i = 0; i < 4; i++)
        {
            m_motionNode->Rewind(m_animGraphInstance);
            m_motionNode->SetMotionPlaySpeed(speedFactors[i]);
            GetEMotionFX().Update(1.0f / 60.0f);

            uniqueData->mReload = true;
            const AZ::Vector3 rootInitialPosUnderSpeed2 = m_jackPose->GetModelSpaceTransform(m_jack_rootIndex).mPosition;

            GetEMotionFX().Update(playTimes[i]);
            const AZ::Vector3 rootFinalPosUnderSpeed2 = m_jackPose->GetModelSpaceTransform(m_jack_rootIndex).mPosition;

            EXPECT_TRUE(rootInitialPosUnderSpeed1.IsClose(rootInitialPosUnderSpeed2, 0.0f));
            EXPECT_TRUE(rootFinalPosUnderSpeed1.IsClose(rootFinalPosUnderSpeed2, 0.0f));

            rootInitialPosUnderSpeed1 = rootInitialPosUnderSpeed2;
            rootFinalPosUnderSpeed1 = rootFinalPosUnderSpeed2;
        }
    };

    TEST_F(AnimGraphMotionNodeFixture, TwoMotionsOutputsCorrectMotionAndPose)
    {
        // Add one more motion, Jack falling back and down.
        // Loop effect turned on to ensure motions are changing in loops.
        AddMotionData("TU9UIAEAAMkAAAAMAAAAAwAAAAAAAAD/////BwAAAMoAAAC4BAEAAQAAAD8AAAAAAAAAAAD/fwAAAAAAAP9/Pok2NHHegzYAAAAAAACAPwAAgD8AAIA/Pok2NHHegzYAAAAAAACAPwAAgD8AAIA/ZQAAAAAAAAAAAAAACQAAAGphY2tfcm9vdD6JNjRx3oM2AAAAAAAAAAAnUNK7x+E+vAAAAACJiAg9AUL4u2LVvbwAAAAAiYiIPYemn7sgewi9AAAAAM3MzD1HpxO7bTwdvQAAAACJiAg+G/nCOupsJb0AAAAAq6oqPumAkjslqiq9AAAAAM3MTD40nlc7wTREvQAAAADv7m4+vlTburVPfb0AAAAAiYiIPngv/7vu9aK9AAAAAJqZmT7hroC81EnMvQAAAACrqqo+62LIvIAS+b0AAAAAvLu7Pm476by7oxS+AAAAAM3MzD4Mrt28B1MwvgAAAADe3d0+Hha3vMZjUL4AAAAA7+7uPrVqjbzwaHK+AAAAAAAAAD/TaFO8lrSKvgAAAACJiAg/qwkIvMYVnL4AAAAAERERP6NVcbsyWay+AAAAAJqZGT+Jpbc6roO6vgAAAAAiIiI/YJ76OyLZxb4AAAAAq6oqP/MrdjxJIc++AAAAADMzMz+rTNs8R7DYvgAAAAC8uzs/0sUNPTQw4r4AAAAAREREP9hrFT2c0u2+AAAAAM3MTD+b/xY9vAb9vgAAAABVVVU/cEQDPURfB78AAAAA3t1dP/jl4zzbHRG/AAAAAGZmZj+46NU8OeIZvwAAAADv7m4/1TPgPG1fIr8AAAAAd3d3P7zN7jwT6Cm/AAAAAAAAgD+LRQA9WZwwvwAAAABERIQ/xGYRPdBBNr8AAAAAiYiIP/dNJD3KOju/AAAAAM3MjD+gOjI9GbY/vwAAAAAREZE/HEJGPfiYQ78AAAAAVVWVPydxXz3wy0a/AAAAAJqZmT9cFXU9SSdJvwAAAADe3Z0/JJyEPfuDSr8AAAAAIiKiP7cdiz0g1Eq/AAAAAGZmpj9e5pE9Do9KvwAAAACrqqo/XUKXPbz7Sb8AAAAA7+6uPwn7nD0FoEi/AAAAADMzsz8ie6M9BTJHvwAAAAB3d7c/WECqPaBBRr8AAAAAvLu7P6f8rT2qlkW/AAAAAAAAwD8isK89UiFFvwAAAABERMQ/qaWvPdzdRL8AAAAAiYjIP3uLrz2Yn0S/AAAAAM3MzD9waa89r2lEvwAAAAAREdE/ZkevPUY/RL8AAAAAVVXVPzctrz2EI0S/AAAAAJqZ2T++Iq89jxlEvwAAAADe3d0/viKvPY8ZRL8AAAAAIiLiP74irz2PGUS/AAAAAGZm5j++Iq89jxlEvwAAAACrquo/viKvPY8ZRL8AAAAA7+7uP74irz2PGUS/AAAAADMz8z++Iq89jxlEvwAAAAB3d/c/viKvPY8ZRL8AAAAAvLv7P74irz2PGUS/AAAAAAAAAEC+Iq89jxlEvwAAAAAiIgJAviKvPY8ZRL8AAAAAREQEQL4irz2PGUS/AAAAAGZmBkC+Iq89jxlEvwAAAACJiAhAviKvPY8ZRL8AAAAAq6oKQL4irz2PGUS/AAAAAM3MDEC+Iq89jxlEvwAAAADv7g5AviKvPY8ZRL8AAAAAERERQL4irz2PGUS/AAAAADMzE0C+Iq89jxlEvwAAAABVVRVAviKvPY8ZRL8AAAAAd3cXQL4irz2PGUS/AAAAAJqZGUC+Iq89jxlEvwAAAAC8uxtAviKvPY8ZRL8AAAAA3t0dQL4irz2PGUS/AAAAAAAAIEC+Iq89jxlEvwAAAAAiIiJAviKvPY8ZRL8AAAAAREQkQL4irz2PGUS/AAAAAGZmJkC+Iq89jxlEvwAAAACJiChAviKvPY8ZRL8AAAAAq6oqQL4irz2PGUS/AAAAAM3MLEC+Iq89jxlEvwAAAADv7i5AviKvPY8ZRL8AAAAAERExQL4irz2PGUS/AAAAADMzM0C+Iq89jxlEvwAAAABVVTVAviKvPY8ZRL8AAAAAd3c3QL4irz2PGUS/AAAAAJqZOUC+Iq89jxlEvwAAAAC8uztAviKvPY8ZRL8AAAAA3t09QL4irz2PGUS/AAAAAAAAQEC+Iq89jxlEvwAAAAAiIkJAviKvPY8ZRL8AAAAAREREQL4irz2PGUS/AAAAAGZmRkC+Iq89jxlEvwAAAACJiEhAviKvPY8ZRL8AAAAAq6pKQL4irz2PGUS/AAAAAM3MTEC+Iq89jxlEvwAAAADv7k5AviKvPY8ZRL8AAAAAERFRQL4irz2PGUS/AAAAADMzU0C+Iq89jxlEvwAAAABVVVVADADrAH4HxX8MAOsAfgfFf9xKC6UPGImm6X6BPwAAgD8AAIA/AACAPwAAAAAAAAAA6X6BPwAAgD8AAIA/AACAP2UAAABlAAAAAAAAAA0AAABCaXAwMV9fcGVsdmlz3EoLpQ8YiabpfoE/AAAAADMzE6WZmYmmcF2BP4mICD2PwhWlwvWIpqPsgD+JiIg9j8IVpUfhiqaukoA/zczMPWZmFqUfhYumaCSAP4mICD6PwhWlKVyPpnW4fj+rqio+mZkJpR+Fi6ZSRHw/zcxMPilcD6WPwpWmJrt4P+/ubj5SuA6lj8KVpqe7cz+JiIg+KVwPpeF6lKZveW8/mpmZP""s3MDKXhepSmIX1rP6uqqj5wPQql4XqUphDlZj+8u7s+MzMzpeF6lKbPQ2E/zczMPjMzM6UpXI+mAS5aP97d3T6kcD2luB6Fpql6UD/v7u4+PQoXpbgehaZwzEM/AAAAP83MDKVwPYqmc2E0P4mICD9cj0KlcD2KprNxIj8RERE/9Sg8pSlcj6bZpw4/mpkZP1yPSqUpXI+mef7zPiIiIj+4HmWluB6Fptv5yT6rqio/HoVrpY/CdaaKWp8+MzMzP1yPgqW4HoWmAYlmPry7Oz8UroelrkdhpszEID5EREQ/FK6HpbgehaZgvwo+zcxMPxSuh6WPwnWm7tkDPlVVVT9wPYqluB6FpqOxAT7e3V0/FK6Hpbgehabnygk+ZmZmPxSuh6W4HoWmok8NPu/ubj/NzIyluB6Fpr8DCj53d3c/heuRpY/CdaZu0wE+AACAP3A9iqWPwnWm2S73PUREhD9wPYqlj8J1ppKv8j2JiIg/uB6FpY/Cdaa1kuw9zcyMP3A9iqWPwnWm2NvjPRERkT/hepSlj8J1piad3D1VVZU/KVyPpY/CdabyMNk9mpmZP+F6lKWPwnWm3Y/YPd7dnT8pXI+lj8J1pjA/2D0iIqI/4XqUpY/Cdab5J9Q9ZmamPylcj6WPwnWm9LTJPauqqj9wPYqlj8J1pop0wT3v7q4/4XqUpY/CdaZLvL89MzOzPylcj6WPwnWm10a9PXd3tz+ZmZmlj8J1pqQAuz28u7s/KVyPpY/CdaZwxrk9AADAPylcj6WPwnWmLjm5PURExD8pXI+lj8J1piE8uT2JiMg/uB6FpY/CdaaAQ7k9zczMP7gehaWPwnWmFU25PRER0T+PwnWlj8J1pqtWuT1VVdU/uB6FpY/CdaYLXrk9mpnZP7gehaWPwnWm/WC5Pd7d3T+4HoWlj8J1pv1guT0iIuI/uB6FpY/Cdab9YLk9ZmbmP7gehaWPwnWm/WC5Pauq6j+4HoWlj8J1pv1guT3v7u4/uB6FpY/Cdab9YLk9MzPzP7gehaWPwnWm/WC5PXd39z+4HoWlj8J1pv1guT28u/s/uB6FpY/Cdab9YLk9AAAAQLgehaWPwnWm/WC5PSIiAkC4HoWlj8J1pv1guT1ERARAuB6FpY/Cdab9YLk9ZmYGQLgehaWPwnWm/WC5PYmICEC4HoWlj8J1pv1guT2rqgpAuB6FpY/Cdab9YLk9zcwMQLgehaWPwnWm/WC5Pe/uDkC4HoWlj8J1pv1guT0RERFAuB6FpY/Cdab9YLk9MzMTQLgehaWPwnWm/WC5PVVVFUC4HoWlj8J1pv1guT13dxdAuB6FpY/Cdab9YLk9mpkZQLgehaWPwnWm/WC5Pby7G0C4HoWlj8J1pv1guT3e3R1AuB6FpY/Cdab9YLk9AAAgQLgehaWPwnWm/WC5PSIiIkC4HoWlj8J1pv1guT1ERCRAuB6FpY/Cdab9YLk9ZmYmQLgehaWPwnWm/WC5PYmIKEC4HoWlj8J1pv1guT2rqipAuB6FpY/Cdab9YLk9zcwsQLgehaWPwnWm/WC5Pe/uLkC4HoWlj8J1pv1guT0RETFAuB6FpY/Cdab9YLk9MzMzQLgehaWPwnWm/WC5PVVVNUC4HoWlj8J1pv1guT13dzdAuB6FpY/Cdab9YLk9mpk5QLgehaWPwnWm/WC5Pby7O0C4HoWlj8J1pv1guT3e3T1AuB6FpY/Cdab9YLk9AABAQLgehaWPwnWm/WC5PSIiQkC4HoWlj8J1pv1guT1ERERAuB6FpY/Cdab9YLk9ZmZGQLgehaWPwnWm/WC5PYmISEC4HoWlj8J1pv1guT2rqkpAuB6FpY/Cdab9YLk9zcxMQLgehaWPwnWm/WC5Pe/uTkC4HoWlj8J1pv1guT0REVFAuB6FpY/Cdab9YLk9MzNTQLgehaWPwnWm/WC5PVVVVUAMAOsAfgfFfwAAAADBADYB4QL0f4mICD2YAqwAEgH2f4mIiD0DB7H/6AHKf83MzD3FDXr/8QE8f4mICD59FO4ATwBXfquqKj4qGeQCyv51fc3MTD6IG9QDtvzlfO/ubj5vHH0FRvqLfImIiD4vHZAGpvlJfJqZmT5fHvsHQ/rze6uqqj5VIA8J9Px5e7y7uz7NInIJ9gDOes3MzD7uJBYJDgYQet7d3T6tJQgIrgqXee/u7j6iJZAG7A4/eQAAAD/kJMQEhxPneImICD/hI6UCgxhdeBERET9NI0sARR1/d5qZGT8bJGL9YiEkdiIiIj+MJvr4zCXXc6uqKj8cKkPzTyqAcDMzMz/OLZbsay5pbLy7Oz93NYzpVCwkaURERD+NQtDnWCXeY83MTD/TS+TiJyQUXFVVVT9cU67e9iGsVN7dXT8QVsreER8cU2ZmZj/gVwXgLxsPU+/ubj9UWTnhcBjQUnd3dz+GWkrkORV8UwAAgD/4W+7oCxFFVEREhD8FXd7spQ24VImIiD+OXe7vIwsiVc3MjD9gXUHyfAnvVRERkT8jXQX08gecVlVVlT8dXQH1rAbfVpqZmT8LXZj1xwUWV97dnT/uXHP2tgReVyIioj++XDP4HgPQV2Zmpj9DXKf5VAF5WKuqqj/mWyL73v/1WO/urj+9Wwz88v4pWTMzsz+DW/r8r/1oWXd3tz9OW8f9OPyYWby7uz8pW2T+pfu6WQAAwD8WW7b+bfvLWURExD8XW8v+evvMWYmIyD8YW+L+jPvLWc3MzD8="
            , "jack_death_fall_back_zup");
        m_motionNode->AddMotionId("jack_death_fall_back_zup");
        AnimGraphMotionNode::UniqueData* uniqueData = static_cast<AnimGraphMotionNode::UniqueData*>(m_animGraphInstance->FindOrCreateUniqueNodeData(m_motionNode));
        uniqueData->mReload = true;
        m_motionNode->Reinit();

        m_motionNode->SetIndexMode(AnimGraphMotionNode::INDEXMODE_RANDOMIZE);
        m_motionNode->SetNextMotionAfterLoop(true);
        m_motionNode->SetLoop(true);
        GetEMotionFX().Update(1.0f / 60.0f);
        
        EXPECT_TRUE(m_motionNode->GetNumMotions() == 2) << "Motion node should have 2 motions after adding motion id.";
        EXPECT_TRUE(m_motionNode->GetIsLooping()) << "Motion node loop effect should be on.";

        // In randomized index mode, all motions should at least appear once over 10 loops.
        bool motion1Displayed = false;
        bool motion2Displayed = false;
        for (AZ::u32 i = 0; i < 20; i++)
        {
            // Run the test loop multiple times to make sure all the motion index is picked.
            uniqueData->mReload = true;
            m_motionNode->Reinit();
            GetEMotionFX().Update(2.0f);

            const uint32 motionIndex = uniqueData->mActiveMotionIndex;
            if (motionIndex == 0)
            {
                motion1Displayed = true;
            }
            else if (motionIndex == 1)
            {
                motion2Displayed = true;
            }
            else
            {
                EXPECT_TRUE(false) << "Unexpected motion index.";
            }
            if (motion1Displayed && motion2Displayed)
            {
                break;
            }
        }
        EXPECT_TRUE(motion1Displayed && motion2Displayed) << "Motion 1 and motion 2 should both have been displayed.";

        m_motionNode->SetIndexMode(AnimGraphMotionNode::INDEXMODE_RANDOMIZE_NOREPEAT);
        uniqueData->Reset();
        m_motionNode->Reinit();
        uniqueData->Update();
        uint32 currentMotionIndex = uniqueData->mActiveMotionIndex;

        // In randomized no repeat index mode, motions should change in each loop.
        for (AZ::u32 i = 0; i < 10; i++)
        {
            uniqueData->mReload = true;
            m_motionNode->Reinit();

            // As we keep and use the cached version of the unique data, we need to manually update it.
            uniqueData->Update();

            const AZ::u32 updatedMotionIndex = uniqueData->mActiveMotionIndex;
            EXPECT_TRUE(updatedMotionIndex != currentMotionIndex) << "Updated motion index should be different from its previous motion index.";
            currentMotionIndex = updatedMotionIndex;
        }

        m_motionNode->SetIndexMode(AnimGraphMotionNode::INDEXMODE_SEQUENTIAL);

        // In sequential index mode, motions should increase its index each time and wrap around. Basically iterating over the list of motions.
        for (AZ::u32 i = 0; i < 10; i++)
        {
            uniqueData->mReload = true;
            m_motionNode->Reinit();
            uniqueData->Update();
            EXPECT_NE(currentMotionIndex, uniqueData->mActiveMotionIndex) << "Updated motion index should match the expected motion index.";
            currentMotionIndex = uniqueData->mActiveMotionIndex;
        }
    };
} // end namespace EMotionFX
