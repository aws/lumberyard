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

#include <AzCore/UnitTest/UnitTest.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/tuple.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/MorphSetup.h>
#include <EMotionFX/Source/MorphSetupInstance.h>
#include <EMotionFX/Source/MorphSubMotion.h>
#include <EMotionFX/Source/MorphTargetStandard.h>
#include <EMotionFX/Source/SkeletalMotion.h>
#include <EMotionFX/Source/SkeletalSubMotion.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Skeleton.h>
#include <EMotionFX/Source/TransformData.h>
#include <Tests/Matchers.h>
#include <Tests/SystemComponentFixture.h>

#include <Tests/TestAssetCode/SimpleActors.h>
#include <Tests/TestAssetCode/ActorFactory.h>


namespace EMotionFX
{
    class MotionSamplingTests : public SystemComponentFixture
    {
    public:
        MotionSamplingTests()
            : SystemComponentFixture()
        {
        }

        void SetUp() override
        {
            SystemComponentFixture::SetUp();

            // Create the motion.
            m_motion = SkeletalMotion::Create("MotionInstanceTest");
            ASSERT_NE(m_motion, nullptr) << "Expected motion to not be a nullptr.";
            m_motion->SetMaxTime(1.0f);

            // Create the actor.
            m_actor = ActorFactory::CreateAndInit<SimpleJointChainActor>(4);
            ASSERT_NE(m_actor.get(), nullptr) << "Expected actor to not be a nullptr.";

            // Create the morph targets.
            MorphSetup* morphSetup = MorphSetup::Create();
            ASSERT_NE(morphSetup, nullptr);
            m_actor->SetMorphSetup(0, morphSetup);

            AZStd::string morphName;
            for (size_t i = 0; i < 3; ++i)
            {
                morphName = AZStd::string::format("morph%lu", i+1);
                MorphTargetStandard* morphTarget = MorphTargetStandard::Create(morphName.c_str());
                ASSERT_NE(morphTarget, nullptr) << "Expected morph target not to be a nullptr.";
                morphSetup->AddMorphTarget(morphTarget);

                // Create and add the sub motions.
                SkeletalSubMotion* subMotion = SkeletalSubMotion::Create(AZStd::string::format("joint%lu", i+1).c_str());
                m_motion->AddSubMotion(subMotion);

                // Add morph sub motion.                
                MorphSubMotion* morphSubMotion = MorphSubMotion::Create(morphTarget->GetID());
                m_motion->AddMorphSubMotion(morphSubMotion);
            }

            m_actor->PostCreateInit(/*makeGeomLodsCompatible=*/false, /*generateObb*/false, /*convertUnitType*/false);

            // Create the actor instance.
            m_actorInstance = ActorInstance::Create(m_actor.get());
            ASSERT_NE(m_actorInstance, nullptr) << "Expected actor instance to not be a nullptr.";

            // Create the motion instance.
            m_motionInstance = MotionInstance::Create(m_motion, m_actorInstance, 0);
            ASSERT_NE(m_motionInstance, nullptr) << "Expected motion instance to not be a nullptr.";
            m_motionInstance->SetMaxLoops(1);
            m_motionInstance->SetFreezeAtLastFrame(true);
            m_motionInstance->InitForSampling();
        }

        void TearDown() override
        {
            m_motionInstance->Destroy();
            m_motion->Destroy();
            m_actorInstance->Destroy();
            m_actor.reset();

            SystemComponentFixture::TearDown();
        }

    protected:
        AZStd::unique_ptr<Actor> m_actor;
        ActorInstance* m_actorInstance = nullptr;
        SkeletalMotion* m_motion = nullptr;
        MotionInstance* m_motionInstance = nullptr;
    };

    TEST_F(MotionSamplingTests, IntegrityCheck)
    {
        ASSERT_EQ(m_actor->GetNumNodes(), 4);
        ASSERT_FLOAT_EQ(m_motion->GetMaxTime(), 1.0f);
        ASSERT_EQ(m_motion->GetNumSubMotions(), 3);
        ASSERT_EQ(m_motion->GetNumMorphSubMotions(), 3);
    }

    TEST_F(MotionSamplingTests, SampleMorphs)
    {
        for (size_t i = 0; i < 3; ++i)
        {
            const float iFloat = static_cast<float>(i);
            MorphSubMotion* morphSubMotion = m_motion->GetMorphSubMotion(i);
            morphSubMotion->SetPoseWeight(iFloat / 2.0f);
            EXPECT_FLOAT_EQ(morphSubMotion->GetPoseWeight(), iFloat / 2.0f);
        }

        auto morphTrack = m_motion->GetMorphSubMotion(0)->GetKeyTrack();
        EXPECT_NE(morphTrack, nullptr);
        for (size_t i = 0; i < 11; ++i)
        {
            const float iFloat = static_cast<float>(i);
            const float value = iFloat / 10.0f;
            morphTrack->AddKey(iFloat, value);
            EXPECT_FLOAT_EQ(morphTrack->GetKey(i)->GetTime(), iFloat);
            EXPECT_TRUE(AZ::IsClose(morphTrack->GetKey(i)->GetValue(), value, 0.0001f)); // AZ::IsClose, because we use compressed floats, which makes it not match close enough.
        }

        m_motion->UpdateMaxTime();
        EXPECT_FLOAT_EQ(m_motion->GetMaxTime(), 10.0f);

        EXPECT_TRUE(m_motion->GetMorphSubMotion(0)->GetKeyTrack()->CheckIfIsAnimated(m_motion->GetMorphSubMotion(0)->GetPoseWeight(), 0.001f));
        EXPECT_FALSE(m_motion->GetMorphSubMotion(1)->GetKeyTrack()->CheckIfIsAnimated(m_motion->GetMorphSubMotion(1)->GetPoseWeight(), 0.001f));
        EXPECT_FALSE(m_motion->GetMorphSubMotion(2)->GetKeyTrack()->CheckIfIsAnimated(m_motion->GetMorphSubMotion(2)->GetPoseWeight(), 0.001f));

        using ValuePair = AZStd::pair<float, float>; // First = time, second = expectedValue.
        const AZStd::vector<ValuePair> values{ // We can't sample negative time values, as we will assert, just in case you think thats missing :)
            { 0.0f, 0.0f },
            { 2.5f, 0.25f },
            { 7.0f, 0.7f },
            { m_motion->GetMaxTime(), 1.0f },
            { m_motion->GetMaxTime() + 10.0f, 1.0f }
        };

        Pose inPose;
        Pose outPose;
        inPose.LinkToActorInstance(m_actorInstance);
        outPose.LinkToActorInstance(m_actorInstance);
        inPose.InitFromBindPose(m_actor.get());
        outPose.InitFromBindPose(m_actor.get());

        for (const ValuePair& valuePair : values)
        {
            m_motionInstance->SetCurrentTime(valuePair.first);
            m_motion->Update(&inPose, &outPose, m_motionInstance);

            EXPECT_TRUE(AZ::IsClose(outPose.GetMorphWeight(0), valuePair.second, 0.0001f));
            EXPECT_TRUE(AZ::IsClose(outPose.GetMorphWeight(1), 0.5f, 0.0001f)); // 0.5 is the pose value of the morph submotion.
            EXPECT_TRUE(AZ::IsClose(outPose.GetMorphWeight(2), 1.0f, 0.0001f)); // Same here for the 1.0 value.
        }
    }

    TEST_F(MotionSamplingTests, SampleTransforms)
    {
        SkeletalSubMotion* subMotion1 = m_motion->GetSubMotion(0);
        subMotion1->SetPosePos(AZ::Vector3(1.0f, 2.0f, 3.0f));
        subMotion1->SetPoseRot(AZ::Quaternion::CreateIdentity());

        auto posTrack = subMotion1->GetPosTrack();
        ASSERT_EQ(posTrack, nullptr);
        subMotion1->CreatePosTrack();
        posTrack = subMotion1->GetPosTrack();
        ASSERT_NE(posTrack, nullptr);

        auto rotTrack = subMotion1->GetRotTrack();
        ASSERT_EQ(rotTrack, nullptr);
        subMotion1->CreateRotTrack();
        rotTrack = subMotion1->GetRotTrack();
        ASSERT_NE(rotTrack, nullptr);

        EMFX_SCALECODE
        (
            auto scaleTrack = subMotion1->GetScaleTrack();
            ASSERT_EQ(scaleTrack, nullptr);
            subMotion1->CreateScaleTrack();
            scaleTrack = subMotion1->GetScaleTrack();
            ASSERT_NE(scaleTrack, nullptr);
        )

        const size_t numSamples = 11;
        for (size_t i = 0; i < numSamples; ++i)
        {
            const float iFloat = static_cast<float>(i) / 10.0f;

            const AZ::Vector3 position(iFloat, iFloat * 10, iFloat * 100.0f );
            posTrack->AddKey(iFloat, position);
            EXPECT_THAT(posTrack->GetKey(i)->GetValue(), IsClose(position));
            EXPECT_NEAR(posTrack->GetKey(i)->GetTime(), iFloat, 0.00001f);

            const AZ::Quaternion rotation = AZ::Quaternion::CreateRotationZ(iFloat).GetNormalized();
            rotTrack->AddKey(iFloat, rotation);
            EXPECT_THAT(rotTrack->GetKey(i)->GetValue(), IsClose(rotation));
            EXPECT_NEAR(rotTrack->GetKey(i)->GetTime(), iFloat, 0.00001f);

#ifndef EMFX_SCALE_DISABLED
            const AZ::Vector3 scale(iFloat + 1.0f, iFloat + 2.0f, iFloat + 3.0f);
            scaleTrack->AddKey(iFloat, scale);
            EXPECT_THAT(scaleTrack->GetKey(i)->GetValue(), IsClose(scale));
            EXPECT_NEAR(scaleTrack->GetKey(i)->GetTime(), iFloat, 0.00001f);
#endif
        }
        m_motion->UpdateMaxTime();
        EXPECT_FLOAT_EQ(m_motion->GetMaxTime(), 1.0f);

        // Build expected output.
        const size_t lastSampleIndex = numSamples - 1;
#ifndef EMFX_SCALE_DISABLED
        const AZ::Vector3 firstScaleSample = scaleTrack->GetKey(0)->GetValue();
        const AZ::Vector3 lastScaleSample = scaleTrack->GetLastKey()->GetValue();
#else
        const AZ::Vector3 firstScaleSample(1.0f, 1.0f, 1.0f);
        const AZ::Vector3 lastScaleSample(1.0f, 1.0f, 1.0f);
#endif
        using TransformPair = AZStd::pair<float, Transform>; // First = time, second = expectedValue.
        const AZStd::vector<TransformPair> values{
            { 0.0f,
                Transform(
                    posTrack->GetFirstKey()->GetValue(),
                    rotTrack->GetFirstKey()->GetValue(),
                    firstScaleSample) },
            { 0.25f,
                Transform(
                    AZ::Vector3(0.25f, 2.5f, 25.0f),
                    AZ::Quaternion::CreateRotationZ(0.2f).NLerp(AZ::Quaternion::CreateRotationZ(0.3f), 0.5f),
                    AZ::Vector3(1.25f, 2.25f, 3.25f)) },
            { 0.5f,
                Transform(
                    AZ::Vector3(0.5f, 5.0f, 50.0f),
                    AZ::Quaternion::CreateRotationZ(0.5f).GetNormalized(),
                    AZ::Vector3(1.5f, 2.5f, 3.5f)) },
            { 0.75f,
                Transform(
                    AZ::Vector3(0.75f, 7.5f, 75.0f),
                    AZ::Quaternion::CreateRotationZ(0.7f).NLerp(AZ::Quaternion::CreateRotationZ(0.8f), 0.5f),
                    AZ::Vector3(1.75f, 2.75f, 3.75f)) },
            { 1.0f,
                Transform(
                    posTrack->GetLastKey()->GetValue(),
                    rotTrack->GetLastKey()->GetValue(),
                    lastScaleSample) },
            { m_motion->GetMaxTime() + 1.0f,
                Transform(
                    posTrack->GetLastKey()->GetValue(),
                    rotTrack->GetLastKey()->GetValue(),
                    lastScaleSample) }
        };

        // Perform per joint sampling.
        Node* joint = m_actor->GetSkeleton()->FindNodeByName("joint1");
        for (const TransformPair& expectation : values)
        {
            Transform sampledResult = Transform::CreateIdentity();
            m_motionInstance->SetCurrentTime(expectation.first);
            m_motion->CalcNodeTransform(m_motionInstance, &sampledResult, m_actor.get(), joint, /*sampleTime=*/expectation.first, /*retarget=*/false);
            EXPECT_THAT(sampledResult.mPosition, IsClose(expectation.second.mPosition));
            EXPECT_THAT(sampledResult.mRotation, IsClose(expectation.second.mRotation));
#ifndef EMFX_SCALE_DISABLED
            EXPECT_THAT(sampledResult.mScale, IsClose(expectation.second.mScale));
#endif
        }

        // Sample the entire pose.
        Pose bindPose;
        bindPose.LinkToActorInstance(m_actorInstance);
        bindPose.InitFromBindPose(m_actor.get());
        for (const TransformPair& expectation : values)
        {
            Pose pose;
            pose.LinkToActorInstance(m_actorInstance);

            m_motionInstance->SetCurrentTime(expectation.first);
            m_motion->Update(&bindPose, &pose, m_motionInstance);

            const Transform sampledResult = pose.GetLocalSpaceTransform(joint->GetNodeIndex());
            EXPECT_THAT(sampledResult.mPosition, IsClose(expectation.second.mPosition));
            EXPECT_THAT(sampledResult.mRotation, IsClose(expectation.second.mRotation));
#ifndef EMFX_SCALE_DISABLED
            EXPECT_THAT(sampledResult.mScale, IsClose(expectation.second.mScale));
#endif
        }
    }
} // namespace EMotionFX
