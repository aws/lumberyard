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
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Importer/Importer.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionInstance.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Skeleton.h>
#include <EMotionFX/Source/SkeletalMotion.h>
#include <EMotionFX/Source/MorphSubMotion.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Source/MorphSetup.h>
#include <EMotionFX/Source/MorphTargetStandard.h>
#include <EMotionFX/Source/AttachmentSkin.h>
#include <Tests/JackGraphFixture.h>

namespace EMotionFX
{
    class MorphSkinAttachmentFixture
        : public JackGraphFixture
    {
    public:
        void AddMotionData(const AZStd::string& base64MotionData, const AZStd::string& motionId)
        {
            // Load the motion from the string data.
            AZStd::vector<AZ::u8> skeletalMotiondata;
            AzFramework::StringFunc::Base64::Decode(skeletalMotiondata, base64MotionData.c_str(), base64MotionData.size());
            SkeletalMotion* newMotion = EMotionFX::GetImporter().LoadSkeletalMotion(skeletalMotiondata.begin(), skeletalMotiondata.size());

            // Create some morph sub motions.
            const AZStd::vector<AZStd::string> morphNames { "morph1", "morph2", "morph3", "morph4", "newmorph1", "newmorph2" };
            for (size_t i = 0; i < morphNames.size(); ++i)
            {                
                MorphSubMotion* morphSubMotion = MorphSubMotion::Create(MCore::GetStringIdPool().GenerateIdForString(morphNames[i]));
                morphSubMotion->SetPoseWeight(static_cast<float>(i+1) / 10.0f);
                newMotion->AddMorphSubMotion(morphSubMotion);
            }

            // Add the motion to the motion set.
            EMotionFX::MotionSet::MotionEntry* newMotionEntry = aznew EMotionFX::MotionSet::MotionEntry();
            newMotionEntry->SetMotion(newMotion);
            m_motionSet->AddMotionEntry(newMotionEntry);
            m_motionSet->SetMotionEntryId(newMotionEntry, motionId);
        }

        void ConstructGraph() override
        {
            JackGraphFixture::ConstructGraph();

            // Motion of Jack walking forward (Y-axis change) with right arm aiming towards front.
            AddMotionData("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",
                "jack_walk_forward_aim_zup");

            // Anim graph:
            //
            // +-----------------+       +------------+       +---------+
            // |m_floatConstNode |------>|m_motionNode|------>|finalNode|
            // +-----------------+       |------------+       +---------+

            BlendTreeFinalNode* finalNode = aznew BlendTreeFinalNode();
            m_floatConstNode = aznew BlendTreeFloatConstantNode();
            m_motionNode = aznew AnimGraphMotionNode();

            // Control motion and effects to be used.
            m_motionNode->AddMotionId("jack_walk_forward_aim_zup");
            m_motionNode->SetLoop(true);

            m_blendTree = aznew BlendTree();
            m_blendTree->AddChildNode(m_motionNode);
            m_blendTree->AddChildNode(m_floatConstNode);
            m_blendTree->AddChildNode(finalNode);

            m_animGraph->GetRootStateMachine()->AddChildNode(m_blendTree);
            m_animGraph->GetRootStateMachine()->SetEntryState(m_blendTree);

            finalNode->AddConnection(m_motionNode, AnimGraphMotionNode::OUTPUTPORT_POSE, BlendTreeFinalNode::INPUTPORT_POSE);
        }

        void OnPostActorCreated() override
        {
            m_attachmentActor = m_actor->Clone();

            // Create a few morph targets in the main actor.
            MorphSetup* morphSetup = MorphSetup::Create();
            m_actor->SetMorphSetup(0, morphSetup);
            morphSetup->AddMorphTarget(MorphTargetStandard::Create("morph1"));
            morphSetup->AddMorphTarget(MorphTargetStandard::Create("morph2"));
            morphSetup->AddMorphTarget(MorphTargetStandard::Create("morph3"));
            morphSetup->AddMorphTarget(MorphTargetStandard::Create("morph4"));

            // Create a few other morphs in our attachment.
            MorphSetup* attachMorphSetup = MorphSetup::Create();
            m_attachmentActor->SetMorphSetup(0, attachMorphSetup);
            attachMorphSetup->AddMorphTarget(MorphTargetStandard::Create("newmorph1"));
            attachMorphSetup->AddMorphTarget(MorphTargetStandard::Create("newmorph2"));
            attachMorphSetup->AddMorphTarget(MorphTargetStandard::Create("morph1"));
            attachMorphSetup->AddMorphTarget(MorphTargetStandard::Create("morph2"));

            // Make sure our morphs are registered in the transform data poses.
            m_actor->ResizeTransformData();
            m_attachmentActor->ResizeTransformData();
            m_attachmentActorInstance = ActorInstance::Create(m_attachmentActor);
        }

        void TearDown() override
        {
            m_attachmentActorInstance->Destroy();
            m_attachmentActor->Destroy();

            JackGraphFixture::TearDown();
        }

    protected:
        AnimGraphMotionNode* m_motionNode = nullptr;
        BlendTree* m_blendTree = nullptr;
        BlendTreeFloatConstantNode* m_floatConstNode = nullptr;
        Actor* m_attachmentActor = nullptr;
        ActorInstance* m_attachmentActorInstance = nullptr;
    };

    TEST_F(MorphSkinAttachmentFixture, TransferTestUnattached)
    {
        GetEMotionFX().Update(1.0f / 60.0f);

        // The main actor instance should receive the morph sub motion values.
        const Pose& curPose = *m_actorInstance->GetTransformData()->GetCurrentPose();
        ASSERT_EQ(curPose.GetNumMorphWeights(), 4);
        EXPECT_FLOAT_EQ(curPose.GetMorphWeight(0), 0.1f);
        EXPECT_FLOAT_EQ(curPose.GetMorphWeight(1), 0.2f);
        EXPECT_FLOAT_EQ(curPose.GetMorphWeight(2), 0.3f);
        EXPECT_FLOAT_EQ(curPose.GetMorphWeight(3), 0.4f);

        // We expect no transfer of morph weights, since we aren't attached.
        const Pose& attachPose = *m_attachmentActorInstance->GetTransformData()->GetCurrentPose();
        ASSERT_EQ(attachPose.GetNumMorphWeights(), 4);
        EXPECT_FLOAT_EQ(attachPose.GetMorphWeight(0), 0.0f);
        EXPECT_FLOAT_EQ(attachPose.GetMorphWeight(1), 0.0f);
        EXPECT_FLOAT_EQ(attachPose.GetMorphWeight(2), 0.0f);
        EXPECT_FLOAT_EQ(attachPose.GetMorphWeight(3), 0.0f);
    };

    TEST_F(MorphSkinAttachmentFixture, TransferTestAttached)
    {
        // Create the attachment.
        AttachmentSkin* skinAttachment = AttachmentSkin::Create(m_actorInstance, m_attachmentActorInstance);
        m_actorInstance->AddAttachment(skinAttachment);

        GetEMotionFX().Update(1.0f / 60.0f);

        // The main actor instance should receive the morph sub motion values.
        const Pose& curPose = *m_actorInstance->GetTransformData()->GetCurrentPose();
        ASSERT_EQ(curPose.GetNumMorphWeights(), 4);
        EXPECT_FLOAT_EQ(curPose.GetMorphWeight(0), 0.1f);
        EXPECT_FLOAT_EQ(curPose.GetMorphWeight(1), 0.2f);
        EXPECT_FLOAT_EQ(curPose.GetMorphWeight(2), 0.3f);
        EXPECT_FLOAT_EQ(curPose.GetMorphWeight(3), 0.4f);

        // The skin attachment should now receive morph values from the main actor.
        const Pose& attachPose = *m_attachmentActorInstance->GetTransformData()->GetCurrentPose();
        ASSERT_EQ(attachPose.GetNumMorphWeights(), 4);
        EXPECT_FLOAT_EQ(attachPose.GetMorphWeight(0), 0.0f);    // Once we auto register missing morphs this should be 0.5. See https://jira.agscollab.com/browse/LY-100212
        EXPECT_FLOAT_EQ(attachPose.GetMorphWeight(1), 0.0f);    // Once we auto register missing morphs this should be 0.6. See https://jira.agscollab.com/browse/LY-100212
        EXPECT_FLOAT_EQ(attachPose.GetMorphWeight(2), 0.1f);
        EXPECT_FLOAT_EQ(attachPose.GetMorphWeight(3), 0.2f);
    };
} // end namespace EMotionFX
