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

#include <AzCore/Math/Random.h>
#include <Tests/SystemComponentFixture.h>
#include <Tests/Matchers.h>
#include <MCore/Source/AzCoreConversions.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/Mesh.h>
#include <EMotionFX/Source/MeshDeformerStack.h>
#include <EMotionFX/Source/MorphSetup.h>
#include <EMotionFX/Source/MorphSetupInstance.h>
#include <EMotionFX/Source/MorphTargetStandard.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/PoseData.h>
#include <EMotionFX/Source/PoseDataFactory.h>
#include <EMotionFX/Source/PoseDataRagdoll.h>
#include <EMotionFX/Source/Skeleton.h>
#include <EMotionFX/Source/SoftSkinDeformer.h>
#include <EMotionFX/Source/SoftSkinManager.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Source/Transform.h>


namespace EMotionFX
{
    class DISABLED_PoseTests
        : public SystemComponentFixture
    {
    public:
        Node* AddJoint(const AZStd::string& name, Node* parent, const AZ::Vector3& pos, const AZ::Quaternion& rot = AZ::Quaternion::CreateIdentity(), const AZ::Vector3& scale = AZ::Vector3::CreateOne())
        {
            Skeleton* skeleton = m_actor->GetSkeleton();

            Node* newJoint = Node::Create(name.c_str(), skeleton);
            m_actor->AddNode(newJoint);

            if (parent)
            {
                newJoint->SetParentIndex(parent->GetNodeIndex());
            }

            skeleton->UpdateNodeIndexValues();
            const AZ::u32 jointIndex = newJoint->GetNodeIndex();

            if (parent)
            {
                parent->AddChild(jointIndex);
            }
            else
            {
                skeleton->AddRootNode(jointIndex);
            }

            Pose& actorBindPose = *m_actor->GetBindPose();
            actorBindPose.SetModelSpaceTransform(jointIndex, Transform(pos, MCore::AzQuatToEmfxQuat(rot), scale));

            return newJoint;
        }

        void SetUp() override
        {
            SystemComponentFixture::SetUp();

            m_actor = Actor::Create("TestActor");
            Skeleton* skeleton = m_actor->GetSkeleton();

            Node* parentJoint = nullptr;
            for (AZ::u32 i = 0; i < m_numJoints; ++i)
            {
                Node* joint = AddJoint(AZStd::string::format("joint%d", i),
                    parentJoint,
                    AZ::Vector3(0.0f, 0.0f, static_cast<float>(i)));
                parentJoint = joint;
            }
            m_actor->SetMotionExtractionNodeIndex(0);

            MorphSetup* morphSetup = MorphSetup::Create();
            m_actor->SetMorphSetup(0, morphSetup);

            for (size_t i = 0; i < m_numMorphTargets; ++i)
            {
                MorphTargetStandard* morphTarget = MorphTargetStandard::Create(AZStd::string::format("MT#%d", i).c_str());
                morphTarget->SetRangeMin(0.0f);
                morphTarget->SetRangeMax(1.0f);
                morphSetup->AddMorphTarget(morphTarget);
            }

            m_actor->ResizeTransformData();
            m_actor->PostCreateInit();

            m_actorInstance = ActorInstance::Create(m_actor);
        }

        void TearDown() override
        {
            m_actor->Destroy();
            m_actorInstance->Destroy();
            SystemComponentFixture::TearDown();
        }

        void CompareFlags(const Pose& pose, uint8 expectedFlags)
        {
            const AZ::u32 numTransforms = pose.GetNumTransforms();
            for (AZ::u32 i = 0; i < numTransforms; ++i)
            {
                EXPECT_EQ(pose.GetFlags(i), expectedFlags);
            }
        }

        void CompareFlags(const Pose& poseA, const Pose& poseB)
        {
            const AZ::u32 numTransforms = poseA.GetNumTransforms();
            EXPECT_EQ(numTransforms, poseB.GetNumTransforms());

            for (AZ::u32 i = 0; i < numTransforms; ++i)
            {
                EXPECT_EQ(poseA.GetFlags(i), poseB.GetFlags(i));
            }
        }

        void CompareMorphTargets(const Pose& poseA, const Pose& poseB)
        {
            const AZ::u32 numMorphWeights = poseA.GetNumMorphWeights();
            EXPECT_EQ(numMorphWeights, poseB.GetNumMorphWeights());

            for (AZ::u32 i = 0; i < numMorphWeights; ++i)
            {
                EXPECT_EQ(poseA.GetMorphWeight(i), poseB.GetMorphWeight(i));
            }
        }

        void CheckIfRotationIsNormalized(const MCore::Quaternion& rotation)
        {
            const float epsilon = 0.01;
            const float length = MCore::EmfxQuatToAzQuat(rotation).GetLength();
            EXPECT_TRUE(AZ::IsClose(length, 1.0f, epsilon))
                << "Rotation quaternion not normalized. Length is " << length << ".";
        }

        void ComparePoseTransforms(const Pose& poseA, const Pose& poseB)
        {
            const AZ::u32 numTransforms = poseA.GetNumTransforms();
            EXPECT_EQ(numTransforms, poseB.GetNumTransforms());

            for (AZ::u32 i = 0; i < numTransforms; ++i)
            {
                const Transform& localA = poseA.GetLocalSpaceTransform(i);
                const Transform& localB = poseB.GetLocalSpaceTransform(i);
                EXPECT_EQ(localA, localB);
                EXPECT_THAT(poseA.GetModelSpaceTransform(i), poseB.GetModelSpaceTransform(i));
            }
        }

        AZ::Quaternion CreateRandomUnnormalizedQuaternion(AZ::SimpleLcgRandom& random) const
        {
            AZ::Quaternion candidate;
            do
            {
                candidate.Set(random.GetRandomFloat(), random.GetRandomFloat(), random.GetRandomFloat(), random.GetRandomFloat());
            }
            while (AZ::IsClose(candidate.GetLength(), 1.0f, AZ::g_fltEps));

            return candidate;     
        }

    public:
        Actor* m_actor = nullptr;
        ActorInstance* m_actorInstance = nullptr;
        const AZ::u32 m_numMorphTargets = 5;
        const AZ::u32 m_numJoints = 5;

        const float m_testOffset = 10.0f;
    };

    class DISABLED_PoseTestsBoolParam
        : public DISABLED_PoseTests
        , public ::testing::WithParamInterface<bool>
    {
    };
    INSTANTIATE_TEST_CASE_P(PoseTests, DISABLED_PoseTestsBoolParam, ::testing::Bool());

    TEST_F(DISABLED_PoseTests, Clear)
    {
        Pose pose;

        pose.LinkToActor(m_actor);
        EXPECT_EQ(pose.GetNumTransforms(), m_actor->GetNumNodes());
        pose.Clear();
        EXPECT_EQ(pose.GetNumTransforms(), 0);

        pose.LinkToActor(m_actor);
        EXPECT_EQ(pose.GetNumTransforms(), m_actor->GetNumNodes());
        pose.Clear(/*clearMem*/false);
        EXPECT_EQ(pose.GetNumTransforms(), 0);
    }

    TEST_F(DISABLED_PoseTests, ClearFlags)
    {
        Pose pose;

        pose.LinkToActor(m_actor, 100);
        EXPECT_EQ(pose.GetNumTransforms(), m_actor->GetNumNodes());
        CompareFlags(pose, 100);

        pose.ClearFlags(200);
        CompareFlags(pose, 200);
    }

    TEST_F(DISABLED_PoseTests, GetSetFlags)
    {
        Pose pose;
        pose.LinkToActor(m_actor);

        const AZ::u32 numTransforms = pose.GetNumTransforms();
        for (AZ::u32 i = 0; i < numTransforms; ++i)
        {
            pose.SetFlags(i, Pose::FLAG_LOCALTRANSFORMREADY);
            EXPECT_EQ(pose.GetFlags(i), Pose::FLAG_LOCALTRANSFORMREADY);

            pose.SetFlags(i, Pose::FLAG_LOCALTRANSFORMREADY | Pose::FLAG_MODELTRANSFORMREADY);
            EXPECT_EQ(pose.GetFlags(i), Pose::FLAG_LOCALTRANSFORMREADY | Pose::FLAG_MODELTRANSFORMREADY);
        }
    }

    TEST_F(DISABLED_PoseTests, InitFromBindPose)
    {
        Pose pose;
        pose.LinkToActor(m_actor);
        pose.InitFromBindPose(m_actor);

        const Pose* bindPose = m_actor->GetBindPose();
        ComparePoseTransforms(pose, *bindPose);
        CompareFlags(pose, *bindPose);
        CompareMorphTargets(pose, *bindPose);
    }

    TEST_F(DISABLED_PoseTests, InitFromPose)
    {
        Pose poseA;
        poseA.LinkToActor(m_actor);
        const Pose* bindPose = m_actor->GetBindPose();
        poseA.InitFromPose(bindPose);

        Pose poseB;
        poseB.LinkToActor(m_actor);
        poseB.InitFromPose(&poseA);

        ComparePoseTransforms(poseA, poseB);
        CompareFlags(poseA, poseB);
        CompareMorphTargets(poseA, poseB);
    }

    TEST_F(DISABLED_PoseTests, LinkToActorInstance)
    {
        Pose pose;
        pose.LinkToActorInstance(m_actorInstance);
        EXPECT_EQ(pose.GetNumTransforms(), m_actor->GetNumNodes());
        EXPECT_EQ(pose.GetActor(), m_actor);
        EXPECT_EQ(pose.GetSkeleton(), m_actor->GetSkeleton());
        EXPECT_EQ(pose.GetActorInstance(), m_actorInstance);
    }

    TEST_F(DISABLED_PoseTests, LinkToActor)
    {
        Pose pose;
        pose.LinkToActor(m_actor);
        EXPECT_EQ(pose.GetNumTransforms(), m_actor->GetNumNodes());
        EXPECT_EQ(pose.GetActor(), m_actor);
        EXPECT_EQ(pose.GetSkeleton(), m_actor->GetSkeleton());
    }

    TEST_F(DISABLED_PoseTests, SetNumTransforms)
    {
        Pose pose;

        pose.SetNumTransforms(100);
        EXPECT_EQ(pose.GetNumTransforms(), 100);

        pose.SetNumTransforms(200);
        EXPECT_EQ(pose.GetNumTransforms(), 200);

        pose.SetNumTransforms(0);
        EXPECT_EQ(pose.GetNumTransforms(), 0);

        pose.SetNumTransforms(100);
        EXPECT_EQ(pose.GetNumTransforms(), 100);
    }

    TEST_F(DISABLED_PoseTests, ApplyMorphWeightsToActorInstance)
    {
        Pose pose;
        pose.LinkToActorInstance(m_actorInstance);
        EXPECT_EQ(pose.GetNumMorphWeights(), m_numMorphTargets);
        MorphSetupInstance* morphInstance = m_actorInstance->GetMorphSetupInstance();
        EXPECT_EQ(m_numMorphTargets, morphInstance->GetNumMorphTargets());

        AZ::SimpleLcgRandom random;
        random.SetSeed(875960);

        for (AZ::u32 i = 0; i < m_numMorphTargets; ++i)
        {
            // Zero all weights on the morph instance.
            morphInstance->GetMorphTarget(i)->SetWeight(0.0f);

            // Apply random morph target weights on the pose.
            const float newWeight = random.GetRandomFloat();
            pose.SetMorphWeight(i, newWeight);
            EXPECT_EQ(pose.GetMorphWeight(i), newWeight);
        }

        pose.ApplyMorphWeightsToActorInstance();

        // Check if all weights got correctly forwarded from the pose to the actor instance.
        for (AZ::u32 i = 0; i < m_numMorphTargets; ++i)
        {
            EXPECT_EQ(pose.GetMorphWeight(i), morphInstance->GetMorphTarget(i)->GetWeight());
        }
    }

    TEST_F(DISABLED_PoseTests, SetGetZeroMorphWeights)
    {
        Pose pose;
        pose.LinkToActor(m_actor);
        EXPECT_EQ(pose.GetNumMorphWeights(), m_numMorphTargets);

        // Set and get tests.
        for (AZ::u32 i = 0; i < m_numMorphTargets; ++i)
        {
            const float newWeight = static_cast<float>(i);
            pose.SetMorphWeight(i, newWeight);
            EXPECT_EQ(pose.GetMorphWeight(i), newWeight);
        }

        // Zero weights test.
        pose.ZeroMorphWeights();
        for (AZ::u32 i = 0; i < m_numMorphTargets; ++i)
        {
            EXPECT_EQ(pose.GetMorphWeight(i), 0.0f);
        }
    }

    TEST_F(DISABLED_PoseTests, ResizeNumMorphs)
    {
        Pose pose;
        pose.LinkToActor(m_actor);
        EXPECT_EQ(pose.GetNumMorphWeights(), m_numMorphTargets);

        pose.ResizeNumMorphs(10);
        EXPECT_EQ(pose.GetNumMorphWeights(), 10);
    }

    TEST_F(DISABLED_PoseTests, GetSetLocalSpaceTransform)
    {
        Pose pose;
        pose.LinkToActor(m_actor);
        const AZ::u32 jointIndex = 0;

        // Set the new transform.
        Transform newTransform(AZ::Vector3(1.0f, 2.0f, 3.0f), MCore::AzQuatToEmfxQuat(AZ::Quaternion(0.1f, 0.2f, 0.3f, 0.4f)), AZ::Vector3(4.0f, 5.0f, 6.0f));
        pose.SetLocalSpaceTransform(jointIndex, newTransform);

        EXPECT_TRUE(pose.GetFlags(jointIndex) & Pose::FLAG_LOCALTRANSFORMREADY);

        // All model space transforms should be invalidated.
        // The model space transform of the node doesn't get automatically updated and
        // all child node model transforms are invalidated along with the joint.
        for (AZ::u32 i = jointIndex; i < m_actor->GetNumNodes(); ++i)
        {
            EXPECT_FALSE(pose.GetFlags(i) & Pose::FLAG_MODELTRANSFORMREADY);
        }

        // Test accessor that returns the transform.
        EXPECT_EQ(pose.GetLocalSpaceTransform(jointIndex), newTransform);

        // Test accessor that writes the transform to a parameter.
        Transform compareTransform;
        pose.GetLocalSpaceTransform(jointIndex, &compareTransform);
        EXPECT_EQ(compareTransform, newTransform);
    }

    TEST_F(DISABLED_PoseTests, GetSetLocalSpaceTransformDirect)
    {
        Pose pose;
        pose.LinkToActor(m_actor);
        const AZ::u32 jointIndex = 0;

        Transform newTransform(AZ::Vector3(1.0f, 2.0f, 3.0f), MCore::AzQuatToEmfxQuat(AZ::Quaternion(0.1f, 0.2f, 0.3f, 0.4f)), AZ::Vector3(4.0f, 5.0f, 6.0f));
        pose.SetLocalSpaceTransformDirect(jointIndex, newTransform);
        EXPECT_TRUE(pose.GetFlags(jointIndex) & Pose::FLAG_LOCALTRANSFORMREADY);
        EXPECT_EQ(pose.GetLocalSpaceTransformDirect(jointIndex), newTransform);
    }

    TEST_F(DISABLED_PoseTests, GetSetModelSpaceTransform)
    {
        Pose pose;
        pose.LinkToActor(m_actor);
        const AZ::u32 jointIndex = 0;

        // Set the new transform.
        Transform newTransform(AZ::Vector3(1.0f, 2.0f, 3.0f), MCore::AzQuatToEmfxQuat(AZ::Quaternion(0.1f, 0.2f, 0.3f, 0.4f)), AZ::Vector3(4.0f, 5.0f, 6.0f));

        // Test accessor that returns the transform.
        pose.SetModelSpaceTransform(jointIndex, newTransform);

        // The local space transform gets directly updated along with SetModelSpaceTransform,
        // so both, the model space as well as the local space transforms should be ready.
        EXPECT_TRUE(pose.GetFlags(jointIndex) & Pose::FLAG_MODELTRANSFORMREADY);
        EXPECT_TRUE(pose.GetFlags(jointIndex) & Pose::FLAG_LOCALTRANSFORMREADY);

        // All child model space transforms should be invalidated as they haven't been updated yet.
        for (AZ::u32 i = jointIndex + 1; i < m_actor->GetNumNodes(); ++i)
        {
            EXPECT_FALSE(pose.GetFlags(i) & Pose::FLAG_MODELTRANSFORMREADY);
        }

        EXPECT_EQ(pose.GetModelSpaceTransform(jointIndex), newTransform);

        // Test accessor that writes the transform to a parameter.
        Transform compareTransform;
        pose.GetModelSpaceTransform(jointIndex, &compareTransform);
        EXPECT_EQ(compareTransform, newTransform);
    }

    TEST_F(DISABLED_PoseTests, GetSetModelSpaceTransformDirect)
    {
        Pose pose;
        pose.LinkToActor(m_actor);
        const AZ::u32 jointIndex = 0;

        Transform newTransform(AZ::Vector3(1.0f, 2.0f, 3.0f), MCore::AzQuatToEmfxQuat(AZ::Quaternion(0.1f, 0.2f, 0.3f, 0.4f)), AZ::Vector3(4.0f, 5.0f, 6.0f));
        pose.SetModelSpaceTransformDirect(jointIndex, newTransform);
        EXPECT_TRUE(pose.GetFlags(jointIndex) & Pose::FLAG_MODELTRANSFORMREADY);
        EXPECT_EQ(pose.GetModelSpaceTransformDirect(jointIndex), newTransform);
    }

    TEST_F(DISABLED_PoseTests, SetLocalGetModelSpaceTransform)
    {
        Pose pose;
        pose.LinkToActor(m_actor);
        pose.InitFromBindPose(m_actor);

        const Transform newTransform(AZ::Vector3(1.0f, 1.0f, 1.0f), MCore::AzQuatToEmfxQuat(AZ::Quaternion::CreateIdentity()));

        // Iterate through the joints, adjust their local space transforms and check if the model space transform adjusts automatically, accordingly.
        for (AZ::u32 i = 0; i < m_numJoints; ++i)
        {
            pose.SetLocalSpaceTransform(i, newTransform);
            EXPECT_EQ(pose.GetLocalSpaceTransform(i), newTransform);
            const float floatI = static_cast<float>(i + 1);
            EXPECT_EQ(pose.GetModelSpaceTransform(i), Transform(AZ::Vector3(floatI, floatI, floatI), MCore::AzQuatToEmfxQuat(AZ::Quaternion::CreateIdentity())));
        }
    }

    TEST_F(DISABLED_PoseTests, SetLocalDirectGetModelSpaceTransform)
    {
        Pose pose;
        pose.LinkToActor(m_actor);
        pose.InitFromBindPose(m_actor);

        const Transform newTransform(AZ::Vector3(1.0f, 1.0f, 1.0f), MCore::AzQuatToEmfxQuat(AZ::Quaternion::CreateIdentity()));

        // Same as the previous test, but this time we use the direct call which does not automatically invalidate the model space transform.
        for (AZ::u32 i = 0; i < m_numJoints; ++i)
        {
            const Transform oldModelSpaceTransform = pose.GetModelSpaceTransform(i);

            // Set the local space transform without invalidating the model space transform.
            pose.SetLocalSpaceTransformDirect(i, newTransform);
            EXPECT_EQ(pose.GetLocalSpaceTransform(i), newTransform);

            // As we used the direct call, the model space tranform did not get invalidated and updated.
            EXPECT_EQ(pose.GetModelSpaceTransformDirect(i), oldModelSpaceTransform);

            // Manually invalidate the model space transform and check the result.
            pose.InvalidateModelSpaceTransform(i);
            const float floatI = static_cast<float>(i + 1);
            EXPECT_EQ(pose.GetModelSpaceTransform(i), Transform(AZ::Vector3(floatI, floatI, floatI), MCore::AzQuatToEmfxQuat(AZ::Quaternion::CreateIdentity())));
        }
    }

    TEST_F(DISABLED_PoseTests, SetModelDirectGetLocalSpaceTransform)
    {
        Pose pose;
        pose.LinkToActor(m_actor);
        pose.InitFromBindPose(m_actor);

        // Similar to previous test, model space and local space operations are switched.
        for (AZ::u32 i = 0; i < m_numJoints; ++i)
        {
            const Transform oldLocalSpaceTransform = pose.GetLocalSpaceTransform(i);
            const Transform newTransform(Transform(AZ::Vector3(0.0f, 0.0f, static_cast<float>((i + 1) * m_testOffset)), MCore::AzQuatToEmfxQuat(AZ::Quaternion::CreateIdentity())));

            // Set the model space transform without invalidating the local space transform.
            pose.SetModelSpaceTransformDirect(i, newTransform);
            EXPECT_EQ(pose.GetModelSpaceTransformDirect(i), newTransform);

            // As we used the direct call, the local space tranform did not get invalidated and updated.
            EXPECT_EQ(pose.GetLocalSpaceTransform(i), oldLocalSpaceTransform);

            // Manually invalidate the local space transform and check the result.
            pose.InvalidateLocalSpaceTransform(i);
            EXPECT_THAT(pose.GetLocalSpaceTransform(i), IsClose(Transform(AZ::Vector3(0.0f, 0.0f, m_testOffset), MCore::AzQuatToEmfxQuat(AZ::Quaternion::CreateIdentity()))));
        }
    }

    TEST_P(DISABLED_PoseTestsBoolParam, UpdateLocalSpaceTranforms)
    {
        Pose pose;
        pose.LinkToActor(m_actor);
        pose.InitFromBindPose(m_actor);

        for (AZ::u32 i = 0; i < m_numJoints; ++i)
        {
            const Transform oldLocalSpaceTransform = pose.GetLocalSpaceTransform(i);
            const Transform newTransform(Transform(AZ::Vector3(0.0f, 0.0f, static_cast<float>((i + 1) * m_testOffset)), MCore::AzQuatToEmfxQuat(AZ::Quaternion::CreateIdentity())));

            // Set the model space transform directly, so that it won't automatically be updated.
            pose.SetModelSpaceTransformDirect(i, newTransform);
            EXPECT_EQ(pose.GetModelSpaceTransformDirect(i), newTransform);
            EXPECT_EQ(pose.GetLocalSpaceTransformDirect(i), oldLocalSpaceTransform);
        }

        // We have to manually update the local space transforms as we directly set them.
        pose.InvalidateAllLocalSpaceTransforms();

        // Update all invalidated local space transforms.
        if (GetParam())
        {
            pose.UpdateAllLocalSpaceTranforms();
        }
        else
        {
            for (AZ::u32 i = 0; i < m_numJoints; ++i)
            {
                pose.UpdateLocalSpaceTransform(i);
            }
        }

        for (AZ::u32 i = 0; i < m_numJoints; ++i)
        {
            // Get the local space transform without auto-updating them, to see if update call worked.
            EXPECT_EQ(pose.GetLocalSpaceTransformDirect(i), Transform(AZ::Vector3(0.0f, 0.0f, m_testOffset), MCore::AzQuatToEmfxQuat(AZ::Quaternion::CreateIdentity())));
        }
    }

    TEST_F(DISABLED_PoseTests, ForceUpdateFullLocalSpacePose)
    {
        Pose pose;
        pose.LinkToActor(m_actor);
        pose.InitFromBindPose(m_actor);

        for (AZ::u32 i = 0; i < m_numJoints; ++i)
        {
            const Transform oldLocalSpaceTransform = pose.GetLocalSpaceTransform(i);
            const Transform newTransform(Transform(AZ::Vector3(0.0f, 0.0f, static_cast<float>((i + 1) * m_testOffset)), MCore::AzQuatToEmfxQuat(AZ::Quaternion::CreateIdentity())));

            // Set the local space without invalidating the model space transform.
            pose.SetModelSpaceTransformDirect(i, newTransform);
            EXPECT_EQ(pose.GetModelSpaceTransformDirect(i), newTransform);
            EXPECT_EQ(pose.GetLocalSpaceTransformDirect(i), oldLocalSpaceTransform);
        }

        // Update all local space transforms regardless of the invalidate flag.
        pose.ForceUpdateFullLocalSpacePose();

        for (AZ::u32 i = 0; i < m_numJoints; ++i)
        {
            // Get the local space transform without auto-updating them, to see if update call worked.
            EXPECT_EQ(pose.GetLocalSpaceTransformDirect(i), Transform(AZ::Vector3(0.0f, 0.0f, m_testOffset), MCore::AzQuatToEmfxQuat(AZ::Quaternion::CreateIdentity())));
        }
    }

    TEST_P(DISABLED_PoseTestsBoolParam, UpdateModelSpaceTranforms)
    {
        Pose pose;
        pose.LinkToActor(m_actor);
        pose.InitFromBindPose(m_actor);
        
        for (AZ::u32 i = 0; i < m_numJoints; ++i)
        {
            const Transform oldModelSpaceTransform = pose.GetModelSpaceTransform(i);
            const Transform newTransform(Transform(AZ::Vector3(0.0f, 0.0f, m_testOffset), MCore::AzQuatToEmfxQuat(AZ::Quaternion::CreateIdentity())));

            // Set the local space and invalidate the model space transform.
            pose.SetLocalSpaceTransform(i, newTransform);
            EXPECT_EQ(pose.GetLocalSpaceTransformDirect(i), newTransform);
            EXPECT_EQ(pose.GetModelSpaceTransformDirect(i), oldModelSpaceTransform);
        }

        // Update all invalidated model space transforms.
        if (GetParam())
        {
            pose.UpdateAllModelSpaceTranforms();
        }
        else
        {
            for (AZ::u32 i = 0; i < m_numJoints; ++i)
            {
                pose.UpdateModelSpaceTransform(i);
            }
        }

        for (AZ::u32 i = 0; i < m_numJoints; ++i)
        {
            // Get the model space transform without auto-updating them, to see if the update call worked.
            EXPECT_EQ(pose.GetModelSpaceTransformDirect(i), Transform(AZ::Vector3(0.0f, 0.0f, static_cast<float>((i + 1) * m_testOffset)), MCore::AzQuatToEmfxQuat(AZ::Quaternion::CreateIdentity())));
        }
    }

    TEST_F(DISABLED_PoseTests, ForceUpdateAllModelSpaceTranforms)
    {
        Pose pose;
        pose.LinkToActor(m_actor);
        pose.InitFromBindPose(m_actor);

        for (AZ::u32 i = 0; i < m_numJoints; ++i)
        {
            const Transform oldModelSpaceTransform = pose.GetModelSpaceTransform(i);
            const Transform newTransform(Transform(AZ::Vector3(0.0f, 0.0f, m_testOffset), MCore::AzQuatToEmfxQuat(AZ::Quaternion::CreateIdentity())));

            // Set the local space without invalidating the model space transform.
            pose.SetLocalSpaceTransformDirect(i, newTransform);
            EXPECT_EQ(pose.GetLocalSpaceTransformDirect(i), newTransform);
            EXPECT_EQ(pose.GetModelSpaceTransformDirect(i), oldModelSpaceTransform);
        }

        // Update all model space transforms regardless of the invalidate flag.
        pose.ForceUpdateFullModelSpacePose();

        for (AZ::u32 i = 0; i < m_numJoints; ++i)
        {
            // Get the model space transform without auto-updating them, to see if the ForceUpdateFullModelSpacePose() worked.
            EXPECT_EQ(pose.GetModelSpaceTransformDirect(i), Transform(AZ::Vector3(0.0f, 0.0f, static_cast<float>((i + 1) * m_testOffset)), MCore::AzQuatToEmfxQuat(AZ::Quaternion::CreateIdentity())));
        }
    }

    TEST_P(DISABLED_PoseTestsBoolParam, GetWorldSpaceTransform)
    {
        Pose pose;
        pose.LinkToActorInstance(m_actorInstance);
        pose.InitFromBindPose(m_actor);

        const Transform offsetTransform(AZ::Vector3(0.0f, 0.0f, m_testOffset), MCore::AzQuatToEmfxQuat(AZ::Quaternion::CreateIdentity()));
        m_actorInstance->SetLocalSpaceTransform(offsetTransform);
        m_actorInstance->UpdateWorldTransform();

        for (AZ::u32 i = 0; i < m_numJoints; ++i)
        {
            pose.SetLocalSpaceTransform(i, offsetTransform);

            const Transform expectedWorldTransform(AZ::Vector3(0.0f, 0.0f, static_cast<float>((i + 2) * m_testOffset)), MCore::AzQuatToEmfxQuat(AZ::Quaternion::CreateIdentity()));
            if (GetParam())
            {
                EXPECT_EQ(pose.GetWorldSpaceTransform(i), expectedWorldTransform);
            }
            else
            {
                Transform worldTransform;
                pose.GetWorldSpaceTransform(i, &worldTransform);
                EXPECT_EQ(worldTransform, expectedWorldTransform);
            }
        }
    }

    TEST_F(DISABLED_PoseTests, GetMeshNodeWorldSpaceTransform)
    {
        const AZ::u32 lodLevel = 0;
        const AZ::u32 jointIndex = 0;
        Pose pose;

        // If there is no actor instance linked, expect the identity transform.
        Transform identityTransform;
        identityTransform.Identity();
        EXPECT_EQ(pose.GetMeshNodeWorldSpaceTransform(lodLevel, jointIndex), identityTransform);

        // Link the actor instance and move it so that the model and world space transforms differ.
        pose.LinkToActorInstance(m_actorInstance);
        pose.InitFromBindPose(m_actor);

        const Transform offsetTransform(AZ::Vector3(0.0f, 0.0f, m_testOffset),
            MCore::AzQuatToEmfxQuat(AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(0.0f, 1.0f, 0.0f), m_testOffset)));
        const Transform doubleOffsetTransform = offsetTransform * offsetTransform;
        m_actorInstance->SetLocalSpaceTransform(offsetTransform);
        pose.SetLocalSpaceTransform(jointIndex, offsetTransform);
        m_actorInstance->UpdateWorldTransform();
        EXPECT_EQ(m_actorInstance->GetWorldSpaceTransform(), offsetTransform);
        EXPECT_THAT(pose.GetWorldSpaceTransform(jointIndex), IsClose(doubleOffsetTransform));

        // Create a mesh and mesh defomer stack (should equal the world space transform of the joint for non-skinned meshes).
        Mesh* mesh = Mesh::Create();
        m_actor->SetMesh(lodLevel, jointIndex, mesh);
        EXPECT_EQ(pose.GetMeshNodeWorldSpaceTransform(lodLevel, jointIndex), pose.GetWorldSpaceTransform(jointIndex));
        MeshDeformerStack* meshDeformerStack = MeshDeformerStack::Create(mesh);
        m_actor->SetMeshDeformerStack(lodLevel, jointIndex, meshDeformerStack);
        EXPECT_EQ(pose.GetMeshNodeWorldSpaceTransform(lodLevel, jointIndex), pose.GetWorldSpaceTransform(jointIndex));

        // Add a skinning deformer and make sure they equal the actor instance's world space transform afterwards.
        meshDeformerStack->AddDeformer(GetSoftSkinManager().CreateDeformer(mesh));
        EXPECT_EQ(pose.GetMeshNodeWorldSpaceTransform(lodLevel, jointIndex), m_actorInstance->GetWorldSpaceTransform());
    }

    TEST_P(DISABLED_PoseTestsBoolParam, CompensateForMotionExtraction)
    {
        const AZ::u32 motionExtractionJointIndex = m_actor->GetMotionExtractionNodeIndex();
        ASSERT_NE(motionExtractionJointIndex, MCORE_INVALIDINDEX32)
            << "Motion extraction joint not set for the test actor.";

        Pose pose;
        pose.LinkToActorInstance(m_actorInstance);
        pose.InitFromBindPose(m_actor);

        const TransformData* transformData = m_actorInstance->GetTransformData();

        // Adjust the default bind pose transform for the motion extraction node in order to see if the compensation
        // for motion extraction actually works.
        Pose* bindPose = transformData->GetBindPose();
        const Transform bindPoseTransform(AZ::Vector3(1.0f, 0.0f, 0.0f), MCore::AzQuatToEmfxQuat(AZ::Quaternion::CreateIdentity()));
        bindPose->SetLocalSpaceTransform(motionExtractionJointIndex, bindPoseTransform);

        const Transform preTransform(AZ::Vector3(0.0f, 0.0f, 1.0f), 
            MCore::AzQuatToEmfxQuat(AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(0.0f, 1.0f, 0.0f), m_testOffset)));
        pose.SetLocalSpaceTransform(motionExtractionJointIndex, preTransform);

        if (GetParam())
        {
            pose.CompensateForMotionExtraction();
        }
        else
        {
            pose.CompensateForMotionExtractionDirect();
        }

        const Transform transformResult = pose.GetLocalSpaceTransform(motionExtractionJointIndex);

        Transform expectedResult = preTransform;
        ActorInstance::MotionExtractionCompensate(expectedResult, bindPoseTransform);
        EXPECT_THAT(transformResult, IsClose(expectedResult));
    }

    TEST_F(DISABLED_PoseTests, CalcTrajectoryTransform)
    {
        const AZ::u32 motionExtractionJointIndex = m_actor->GetMotionExtractionNodeIndex();
        ASSERT_NE(motionExtractionJointIndex, MCORE_INVALIDINDEX32)
            << "Motion extraction joint not set for the test actor.";

        Pose pose;
        pose.LinkToActorInstance(m_actorInstance);
        pose.InitFromBindPose(m_actor);

        pose.SetLocalSpaceTransform(motionExtractionJointIndex, Transform(AZ::Vector3(1.0f, 1.0f, 1.0f),
            MCore::AzQuatToEmfxQuat(AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(0.0f, 1.0f, 0.0f), m_testOffset))));

        const Transform transformResult = pose.CalcTrajectoryTransform();
        const Transform expectedResult = pose.GetWorldSpaceTransform(motionExtractionJointIndex).ProjectedToGroundPlane();
        EXPECT_THAT(transformResult, IsClose(expectedResult));
        EXPECT_EQ(transformResult.mPosition, AZ::Vector3(1.0f, 1.0f, 0.0f));
    }

    ///////////////////////////////////////////////////////////////////////////

    class DISABLED_PoseTestsBlendWeightParam
        : public DISABLED_PoseTests
        , public ::testing::WithParamInterface<float>
    {
    };
    INSTANTIATE_TEST_CASE_P(PoseTests, DISABLED_PoseTestsBlendWeightParam, ::testing::ValuesIn({0.0f, 0.1f, 0.25f, 0.33f, 0.5f, 0.77f, 1.0f}));

    TEST_P(DISABLED_PoseTestsBlendWeightParam, Blend)
    {
        const float blendWeight = GetParam();
        const Pose* sourcePose = m_actorInstance->GetTransformData()->GetBindPose();

        // Create a destination pose and adjust the transforms.
        Pose destPose;
        destPose.LinkToActorInstance(m_actorInstance);
        destPose.InitFromBindPose(m_actor);
        for (AZ::u32 i = 0; i < m_numJoints; ++i)
        {
            const float floatI = static_cast<float>(i);
            Transform transform(AZ::Vector3(0.0f, 0.0f, -floatI),
                MCore::AzQuatToEmfxQuat(AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(0.0f, 1.0f, 0.0f), floatI)));
            EMFX_SCALECODE
            (
                transform.mScale = AZ::Vector3(floatI, floatI, floatI);
            )
            destPose.SetLocalSpaceTransform(i, transform);
        }

        // Blend between the bind and the destination pose.
        Pose blendedPose;
        blendedPose.LinkToActorInstance(m_actorInstance);
        blendedPose.InitFromBindPose(m_actor);
        blendedPose.Blend(&destPose, blendWeight);

        // Check the blended result.
        for (AZ::u32 i = 0; i < m_numJoints; ++i)
        {
            const Transform& sourceTransform = sourcePose->GetLocalSpaceTransform(i);
            const Transform& destTransform = destPose.GetLocalSpaceTransform(i);
            const Transform& transformResult = blendedPose.GetLocalSpaceTransform(i);

            Transform expectedResult = sourceTransform;
            expectedResult.Blend(destTransform, blendWeight);
            EXPECT_THAT(transformResult, IsClose(expectedResult));
            CheckIfRotationIsNormalized(destTransform.mRotation);
        }
    }

    TEST_P(DISABLED_PoseTestsBlendWeightParam, BlendAdditiveUsingBindPose)
    {
        const float blendWeight = GetParam();
        const Pose* bindPose = m_actorInstance->GetTransformData()->GetBindPose();

        // Create a source pose and adjust the transforms.
        Pose sourcePose;
        sourcePose.LinkToActorInstance(m_actorInstance);
        sourcePose.InitFromBindPose(m_actor);
        for (AZ::u32 i = 0; i < m_numJoints; ++i)
        {
            const float floatI = static_cast<float>(i);
            Transform transform(AZ::Vector3(floatI, 0.0f, 0.0f),
                MCore::AzQuatToEmfxQuat(AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(0.0f, 1.0f, 0.0f), floatI)));
            EMFX_SCALECODE
            (
                transform.mScale = AZ::Vector3(floatI, floatI, floatI);
            )

            sourcePose.SetLocalSpaceTransform(i, transform);
        }

        // Create a destination pose and adjust the transforms.
        Pose destPose;
        destPose.LinkToActorInstance(m_actorInstance);
        destPose.InitFromBindPose(m_actor);
        for (AZ::u32 i = 0; i < m_numJoints; ++i)
        {
            const float floatI = static_cast<float>(i);
            Transform transform(AZ::Vector3(0.0f, 0.0f, -floatI),
                MCore::AzQuatToEmfxQuat(AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(1.0f, 0.0f, 0.0f), floatI)));
            EMFX_SCALECODE
            (
                transform.mScale = AZ::Vector3(floatI, floatI, floatI);
            )

            destPose.SetLocalSpaceTransform(i, transform);
        }

        // Initialize our resulting pose from the source pose and additively blend it with the destination pose.
        Pose blendedPose;
        blendedPose.LinkToActorInstance(m_actorInstance);
        blendedPose.InitFromPose(&sourcePose);
        blendedPose.BlendAdditiveUsingBindPose(&destPose, blendWeight);

        for (AZ::u32 i = 0; i < m_numJoints; ++i)
        {
            const Transform& bindPoseTransform = bindPose->GetLocalSpaceTransform(i);
            const Transform& sourceTransform = sourcePose.GetLocalSpaceTransform(i);
            const Transform& destTransform = destPose.GetLocalSpaceTransform(i);
            const Transform& transformResult = blendedPose.GetLocalSpaceTransform(i);

            Transform expectedResult = sourceTransform;
            expectedResult.BlendAdditive(destTransform, bindPoseTransform, blendWeight);
            EXPECT_THAT(transformResult, IsClose(expectedResult));
            CheckIfRotationIsNormalized(destTransform.mRotation);
        }
    }

    ///////////////////////////////////////////////////////////////////////////

    enum PoseTestsMultiplyFunction
    {
        PreMultiply,
        Multiply,
        MultiplyInverse
    };

    class DISABLED_PoseTestsMultiply
        : public DISABLED_PoseTests
        , public ::testing::WithParamInterface<PoseTestsMultiplyFunction>
    {
    };
    INSTANTIATE_TEST_CASE_P(PoseTests, DISABLED_PoseTestsMultiply, ::testing::ValuesIn({
        PreMultiply, Multiply, MultiplyInverse}));

    TEST_P(DISABLED_PoseTestsMultiply, Multiply)
    {
        Pose poseA;
        poseA.LinkToActorInstance(m_actorInstance);
        poseA.InitFromBindPose(m_actor);

        Pose poseB;
        poseB.LinkToActorInstance(m_actorInstance);
        poseB.InitFromBindPose(m_actor);

        for (AZ::u32 i = 0; i < m_numJoints; ++i)
        {
            const float floatI = static_cast<float>(i);
            const Transform transformA(AZ::Vector3(floatI, 0.0f, 0.0f),
                MCore::AzQuatToEmfxQuat(AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(1.0f, 0.0f, 0.0f), floatI)));
            const Transform transformB(AZ::Vector3(floatI, floatI, 0.0f),
                MCore::AzQuatToEmfxQuat(AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(0.0f, 1.0f, 0.0f), floatI)));
            poseA.SetLocalSpaceTransform(i, transformA);
            poseB.SetLocalSpaceTransform(i, transformB);
        }

        Pose poseResult;
        poseResult.LinkToActorInstance(m_actorInstance);
        poseResult.InitFromPose(&poseA);

        switch (GetParam())
        {
            case PreMultiply: { poseResult.PreMultiply(poseB); break; }
            case Multiply: { poseResult.Multiply(poseB); break; }
            case MultiplyInverse: { poseResult.MultiplyInverse(poseB); break; }
            default: { ASSERT_TRUE(false) << "Case not handled."; }
        }

        for (AZ::u32 i = 0; i < m_numJoints; ++i)
        {
            const Transform& transformA = poseA.GetLocalSpaceTransform(i);
            const Transform& transformB = poseB.GetLocalSpaceTransform(i);
            const Transform& transformResult = poseResult.GetLocalSpaceTransform(i);

            Transform expectedResult;
            switch (GetParam())
            {
                case PreMultiply: { expectedResult = transformA.PreMultiplied(transformB); break; }
                case Multiply: { expectedResult = transformA.Multiplied(transformB); break; }
                case MultiplyInverse: { expectedResult = transformA.PreMultiplied(transformB.Inversed()); break; }
                default: { ASSERT_TRUE(false) << "Case not handled."; }
            }

            EXPECT_THAT(transformResult, IsClose(expectedResult));
        }
    }

    ///////////////////////////////////////////////////////////////////////////

    class DISABLED_PoseTestsSum
        : public DISABLED_PoseTests
        , public ::testing::WithParamInterface<float>
    {
    };
    INSTANTIATE_TEST_CASE_P(PoseTests, DISABLED_PoseTestsSum, ::testing::ValuesIn({0.0f, 0.1f, 0.25f, 0.33f, 0.5f, 0.77f, 1.0f}));

    TEST_P(DISABLED_PoseTestsSum, Sum)
    {
        const float weight = GetParam();

        Pose poseA;
        poseA.LinkToActorInstance(m_actorInstance);
        poseA.InitFromBindPose(m_actor);

        Pose poseB;
        poseB.LinkToActorInstance(m_actorInstance);
        poseB.InitFromBindPose(m_actor);

        for (AZ::u32 i = 0; i < m_numJoints; ++i)
        {
            const float floatI = static_cast<float>(i);
            const Transform transformA(AZ::Vector3(floatI, 0.0f, 0.0f), MCore::AzQuatToEmfxQuat(AZ::Quaternion::CreateIdentity()));
            const Transform transformB(AZ::Vector3(floatI, floatI, 0.0f), MCore::AzQuatToEmfxQuat(AZ::Quaternion::CreateIdentity()));
            poseA.SetLocalSpaceTransform(i, transformA);
            poseB.SetLocalSpaceTransform(i, transformB);
        }

        const AZ::u32 numMorphWeights = poseA.GetNumMorphWeights();
        for (AZ::u32 i = 0; i < numMorphWeights; ++i)
        {
            const float floatI = static_cast<float>(i);
            poseA.SetMorphWeight(i, floatI);
            poseB.SetMorphWeight(i, floatI);
        }

        Pose poseSum;
        poseSum.LinkToActorInstance(m_actorInstance);
        poseSum.InitFromPose(&poseA);
        poseSum.Sum(&poseB, weight);

        for (AZ::u32 i = 0; i < m_numJoints; ++i)
        {
            const Transform& transformA = poseA.GetLocalSpaceTransform(i);
            const Transform& transformB = poseB.GetLocalSpaceTransform(i);
            const Transform& transformResult = poseSum.GetLocalSpaceTransform(i);

            Transform expectedResult = transformA;
            expectedResult.Add(transformB, weight);
            EXPECT_THAT(transformResult, IsClose(expectedResult));
        }

        for (AZ::u32 i = 0; i < numMorphWeights; ++i)
        {
            EXPECT_FLOAT_EQ(poseSum.GetMorphWeight(i),
                poseA.GetMorphWeight(i) + poseB.GetMorphWeight(i) * weight);
        }
    }

    ///////////////////////////////////////////////////////////////////////////

    TEST_F(DISABLED_PoseTests, MakeRelativeTo)
    {
        Pose poseA;
        poseA.LinkToActorInstance(m_actorInstance);
        poseA.InitFromBindPose(m_actor);

        Pose poseB;
        poseB.LinkToActorInstance(m_actorInstance);
        poseB.InitFromBindPose(m_actor);

        for (AZ::u32 i = 0; i < m_numJoints; ++i)
        {
            const float floatI = static_cast<float>(i);
            const Transform transformA(AZ::Vector3(floatI, floatI, floatI), MCore::AzQuatToEmfxQuat(AZ::Quaternion::CreateIdentity()));
            const Transform transformB(AZ::Vector3(floatI, floatI, floatI) - AZ::Vector3::CreateOne(), MCore::AzQuatToEmfxQuat(AZ::Quaternion::CreateIdentity()));
            poseA.SetLocalSpaceTransform(i, transformA);
            poseB.SetLocalSpaceTransform(i, transformB);
        }

        Pose poseRel;
        poseRel.LinkToActorInstance(m_actorInstance);
        poseRel.InitFromPose(&poseA);
        poseRel.MakeRelativeTo(poseB);

        for (AZ::u32 i = 0; i < m_numJoints; ++i)
        {
            const Transform& transformA = poseA.GetLocalSpaceTransform(i);
            const Transform& transformB = poseB.GetLocalSpaceTransform(i);
            const Transform& transformRel = poseRel.GetLocalSpaceTransform(i);

            const AZ::Vector3& result = transformRel.mPosition;
            const AZ::Vector3 expectedResult = transformA.mPosition - transformB.mPosition;
            EXPECT_TRUE(result.IsClose(AZ::Vector3::CreateOne()));
        }
    }

    ///////////////////////////////////////////////////////////////////////////

    enum PoseTestAdditiveFunction
    {
        MakeAdditive,
        ApplyAdditive,
        ApplyAdditiveWeight
    };

    struct PoseTestAdditiveParam
    {
        bool m_linkToActorInstance;
        PoseTestAdditiveFunction m_additiveFunction;
        float m_weight;
    };

    class DISABLED_PoseTestsAdditive
        : public DISABLED_PoseTests
        , public ::testing::WithParamInterface<PoseTestAdditiveParam>
    {
    };

    std::vector<PoseTestAdditiveParam> poseTestsAdditiveData
    {
        {true, MakeAdditive, 0.0f}, {true, ApplyAdditive, 0.0f},
        {false, MakeAdditive, 0.0f}, {false, ApplyAdditive, 0.0f},
        {false, ApplyAdditiveWeight, 0.0f}, {false, ApplyAdditiveWeight, 0.25f}, {false, ApplyAdditiveWeight, 0.5f}, {false, ApplyAdditiveWeight, 1.0f},
        {true, ApplyAdditiveWeight, 0.0f}, {true, ApplyAdditiveWeight, 0.25f}, {true, ApplyAdditiveWeight, 0.5f}, {true, ApplyAdditiveWeight, 1.0f}
    };

    INSTANTIATE_TEST_CASE_P(PoseTests, DISABLED_PoseTestsAdditive, ::testing::ValuesIn(poseTestsAdditiveData));

    TEST_P(DISABLED_PoseTestsAdditive, Additive)
    {
        const bool linkToActorInstance = GetParam().m_linkToActorInstance;
        const int additiveFunction = GetParam().m_additiveFunction;
        const float weight = GetParam().m_weight;

        Pose poseA;
        if (linkToActorInstance)
        {
            poseA.LinkToActorInstance(m_actorInstance);
        }
        else
        {
            poseA.LinkToActor(m_actor);
        }
        poseA.InitFromBindPose(m_actor);

        Pose poseB;
        if (linkToActorInstance)
        {
            poseB.LinkToActorInstance(m_actorInstance);
        }
        else
        {
            poseB.LinkToActor(m_actor);
        }
        poseB.InitFromBindPose(m_actor);

        for (AZ::u32 i = 0; i < m_numJoints; ++i)
        {
            const float floatI = static_cast<float>(i);
            const Transform transformA(AZ::Vector3(floatI, 0.0f, 0.0f),
                MCore::AzQuatToEmfxQuat(AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(1.0f, 0.0f, 0.0f), floatI)));
            const Transform transformB(AZ::Vector3(floatI, floatI, 0.0f),
                MCore::AzQuatToEmfxQuat(AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(0.0f, 1.0f, 0.0f), floatI)));
            poseA.SetLocalSpaceTransform(i, transformA);
            poseB.SetLocalSpaceTransform(i, transformB);
        }

        const AZ::u32 numMorphWeights = poseA.GetNumMorphWeights();
        for (AZ::u32 i = 0; i < numMorphWeights; ++i)
        {
            const float floatI = static_cast<float>(i);
            poseA.SetMorphWeight(i, floatI);
            poseB.SetMorphWeight(i, floatI);
        }

        Pose poseResult;
        if (linkToActorInstance)
        {
            poseResult.LinkToActorInstance(m_actorInstance);
        }
        else
        {
            poseResult.LinkToActor(m_actor);
        }
        poseResult.InitFromPose(&poseA);

        switch (additiveFunction)
        {
            case MakeAdditive: { poseResult.MakeAdditive(poseB); break; }
            case ApplyAdditive: { poseResult.ApplyAdditive(poseB); break; }
            case ApplyAdditiveWeight: { poseResult.ApplyAdditive(poseB, weight); break; }
            default: { ASSERT_TRUE(false) << "Case not handled."; }
        }

        for (AZ::u32 i = 0; i < m_numJoints; ++i)
        {
            const Transform& transformA = poseA.GetLocalSpaceTransform(i);
            const Transform& transformB = poseB.GetLocalSpaceTransform(i);
            const Transform& transformResult = poseResult.GetLocalSpaceTransform(i);

            Transform expectedResult;
            if (additiveFunction == MakeAdditive)
            {
                expectedResult.mPosition = transformA.mPosition - transformB.mPosition;
                expectedResult.mRotation = transformB.mRotation.Conjugated() * transformA.mRotation;

                EMFX_SCALECODE
                (
                    expectedResult.mScale = transformA.mScale * transformB.mScale;
                )
            }
            else if (additiveFunction == ApplyAdditive ||
                weight > 1.0f - MCore::Math::epsilon)
            {
                expectedResult.mPosition = transformA.mPosition + transformB.mPosition;
                expectedResult.mRotation = transformA.mRotation * transformB.mRotation;
                expectedResult.mRotation.Normalize();

                EMFX_SCALECODE
                (
                    expectedResult.mScale = transformA.mScale * transformB.mScale;
                )
            }
            else if (weight < MCore::Math::epsilon )
            {
                expectedResult = transformA;
            }
            else
            {
                expectedResult.mPosition = transformA.mPosition + transformB.mPosition * weight;
                expectedResult.mRotation = transformA.mRotation.NLerp(transformB.mRotation * transformA.mRotation, weight);
                expectedResult.mRotation.Normalize();

                EMFX_SCALECODE
                (
                    expectedResult.mScale = transformA.mScale * AZ::Vector3::CreateOne().Lerp(transformB.mScale, weight);
                )
            }

            EXPECT_THAT(transformResult, IsClose(expectedResult));
        }

        switch (additiveFunction)
        {
            case 0:
            {
                for (AZ::u32 i = 0; i < numMorphWeights; ++i)
                {
                    EXPECT_FLOAT_EQ(poseResult.GetMorphWeight(i),
                        poseA.GetMorphWeight(i) - poseB.GetMorphWeight(i));
                }
                break;
            }
            case 1:
            {
                for (AZ::u32 i = 0; i < numMorphWeights; ++i)
                {
                    EXPECT_FLOAT_EQ(poseResult.GetMorphWeight(i),
                        poseA.GetMorphWeight(i) + poseB.GetMorphWeight(i));
                }
                break;
            }
            case 2:
            {
                for (AZ::u32 i = 0; i < numMorphWeights; ++i)
                {
                    EXPECT_FLOAT_EQ(poseResult.GetMorphWeight(i),
                        poseA.GetMorphWeight(i) + poseB.GetMorphWeight(i) * weight);
                }
                break;
            }
            default: { ASSERT_TRUE(false) << "Case not handled."; }
        }
    }

    ///////////////////////////////////////////////////////////////////////////

    TEST_F(DISABLED_PoseTests, Zero)
    {
        Pose pose;
        pose.LinkToActor(m_actor);
        pose.InitFromBindPose(m_actor);
        pose.Zero();

        // Check if local space transforms are correctly zeroed.
        Transform zeroTransform;
        zeroTransform.Zero();
        for (AZ::u32 i = 0; i < m_numJoints; ++i)
        {
            EXPECT_EQ(pose.GetLocalSpaceTransform(i), zeroTransform);
        }

        // Check if morph target weights are all zero.
        const AZ::u32 numMorphWeights = pose.GetNumMorphWeights();
        for (AZ::u32 i = 0; i < numMorphWeights; ++i)
        {
            EXPECT_EQ(pose.GetMorphWeight(i), 0.0f);
        }
    }

    TEST_F(DISABLED_PoseTests, NormalizeQuaternions)
    {
        Pose pose;
        pose.LinkToActor(m_actor);
        pose.InitFromBindPose(m_actor);

        AZ::SimpleLcgRandom random;
        random.SetSeed(875960);

        for (AZ::u32 i = 0; i < m_numJoints; ++i)
        {
            const AZ::Quaternion randomRot = CreateRandomUnnormalizedQuaternion(random);
            Transform transformRandomRot(AZ::Vector3::CreateZero(), MCore::AzQuatToEmfxQuat(randomRot));

            pose.SetLocalSpaceTransform(i, transformRandomRot);
            EXPECT_EQ(pose.GetLocalSpaceTransform(i), transformRandomRot);
        }

        pose.NormalizeQuaternions();

        for (AZ::u32 i = 0; i < m_numJoints; ++i)
        {
            const AZ::Quaternion normalizedRot = MCore::EmfxQuatToAzQuat(pose.GetLocalSpaceTransform(i).mRotation);
            EXPECT_TRUE(AZ::IsClose(normalizedRot.GetLength(), 1.0f, AZ::g_fltEps));
        }
    }

    TEST_F(DISABLED_PoseTests, AssignmentOperator)
    {
        Pose pose;
        pose.LinkToActor(m_actor);
        pose.InitFromBindPose(m_actor);

        Pose poseCopy;
        poseCopy.LinkToActor(m_actor);
        poseCopy = pose;

        const Pose* bindPose = m_actor->GetBindPose();
        ComparePoseTransforms(poseCopy, *bindPose);
        CompareFlags(poseCopy, *bindPose);
        CompareMorphTargets(poseCopy, *bindPose);
    }

    TEST_F(DISABLED_PoseTests, GetAndPreparePoseDataType)
    {
        Pose pose;
        pose.LinkToActor(m_actor);
        PoseData* poseData = pose.GetAndPreparePoseData(azrtti_typeid<PoseDataRagdoll>(), m_actorInstance);

        EXPECT_NE(poseData, nullptr);
        EXPECT_EQ(pose.GetPoseDatas().size(), 1);
        EXPECT_EQ(poseData->RTTI_GetType(), azrtti_typeid<PoseDataRagdoll>());
        EXPECT_EQ(poseData->IsUsed(), true);
    }

    TEST_F(DISABLED_PoseTests, GetAndPreparePoseDataTemplate)
    {
        Pose pose;
        pose.LinkToActor(m_actor);
        PoseData* poseData = pose.GetAndPreparePoseData<PoseDataRagdoll>(m_actorInstance);

        EXPECT_NE(poseData, nullptr);
        EXPECT_EQ(pose.GetPoseDatas().size(), 1);
        EXPECT_EQ(poseData->RTTI_GetType(), azrtti_typeid<PoseDataRagdoll>());
        EXPECT_EQ(poseData->IsUsed(), true);
    }

    TEST_F(DISABLED_PoseTests, GetHasPoseData)
    {
        Pose pose;
        pose.LinkToActor(m_actor);
        PoseData* poseData = pose.GetAndPreparePoseData(azrtti_typeid<PoseDataRagdoll>(), m_actorInstance);

        EXPECT_NE(poseData, nullptr);
        EXPECT_EQ(pose.GetPoseDatas().size(), 1);
        EXPECT_EQ(pose.HasPoseData(azrtti_typeid<PoseDataRagdoll>()), true);
        EXPECT_EQ(pose.GetPoseDataByType(azrtti_typeid<PoseDataRagdoll>()), poseData);
        EXPECT_EQ(pose.GetPoseData<PoseDataRagdoll>(), poseData);
    }

    TEST_F(DISABLED_PoseTests, AddPoseData)
    {
        Pose pose;
        pose.LinkToActor(m_actor);
        PoseData* poseData = PoseDataFactory::Create(&pose, azrtti_typeid<PoseDataRagdoll>());
        pose.AddPoseData(poseData);

        EXPECT_NE(poseData, nullptr);
        EXPECT_EQ(pose.GetPoseDatas().size(), 1);
        EXPECT_EQ(pose.HasPoseData(azrtti_typeid<PoseDataRagdoll>()), true);
        EXPECT_EQ(pose.GetPoseDataByType(azrtti_typeid<PoseDataRagdoll>()), poseData);
        EXPECT_EQ(pose.GetPoseData<PoseDataRagdoll>(), poseData);
    }

    TEST_F(DISABLED_PoseTests, ClearPoseDatas)
    {
        Pose pose;
        pose.LinkToActor(m_actor);
        PoseData* poseData = PoseDataFactory::Create(&pose, azrtti_typeid<PoseDataRagdoll>());
        pose.AddPoseData(poseData);
        EXPECT_NE(poseData, nullptr);
        EXPECT_EQ(pose.GetPoseDatas().size(), 1);

        pose.ClearPoseDatas();
        EXPECT_TRUE(pose.GetPoseDatas().empty());
    }
} // namespace EMotionFX
