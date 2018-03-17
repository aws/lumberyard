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

#ifdef AZ_TESTS_ENABLED

#include "PhysicsTests.h"
#include <AzFramework/Physics/World.h>
#include <AzFramework/Physics/RigidBody.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/PhysicsComponentBus.h>
#include <AzCore/Component/TransformBus.h>

namespace Physics
{
    void PhysicsTestEnvironment::SetupEnvironment()
    {
        AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
    }

    void PhysicsTestEnvironment::TeardownEnvironment()
    {
        AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
    }

    Ptr<World> GenericPhysicsInterfaceTest::CreateTestWorld()
    {
        Ptr<World> world = nullptr;
        Ptr<WorldSettings> worldSettings = aznew WorldSettings();
        worldSettings->m_gravity = AZ::Vector3(0.0f, 0.0f, -10.0f);
        SystemRequestBus::BroadcastResult(world, &SystemRequests::CreateWorldByName, "testWorld", worldSettings);
        return world;
    }

    void GenericPhysicsInterfaceTest::DestroyTestWorld()
    {
        bool result;
        SystemRequestBus::BroadcastResult(result, &SystemRequests::DestroyWorldByName, "testWorld");
    }

    Ptr<RigidBody> AddStaticFloorToWorld(Ptr<World> world, const AZ::Transform& transform = AZ::Transform::CreateIdentity())
    {
        Ptr<RigidBodySettings> rigidBodySettings = RigidBodySettings::Create();
        rigidBodySettings->m_bodyShape = BoxShapeConfiguration::Create(AZ::Vector3(10.0f, 10.0f, 0.5f));
        Ptr<RigidBody> floor = nullptr;
        SystemRequestBus::BroadcastResult(floor, &SystemRequests::CreateRigidBody, rigidBodySettings);
        world->AddBody(floor);
        floor->SetTransform(transform);
        return floor;
    }

    Ptr<RigidBody> AddUnitBoxToWorld(Ptr<World> world, const AZ::Vector3& position, MotionType motionType = MotionType::Dynamic)
    {
        Ptr<RigidBodySettings> rigidBodySettings = RigidBodySettings::Create();
        rigidBodySettings->m_motionType = motionType;
        rigidBodySettings->m_linearDamping = 0.0f;
        rigidBodySettings->m_bodyShape = BoxShapeConfiguration::Create();
        Ptr<RigidBody> box = nullptr;
        SystemRequestBus::BroadcastResult(box, &SystemRequests::CreateRigidBody, rigidBodySettings);
        world->AddBody(box);
        box->SetTransform(AZ::Transform::CreateTranslation(position));
        return box;
    }

    Ptr<RigidBody> AddSphereToWorld(Ptr<World> world, const AZ::Vector3& position, MotionType motionType = MotionType::Dynamic)
    {
        Ptr<RigidBodySettings> rigidBodySettings = RigidBodySettings::Create();
        rigidBodySettings->m_motionType = motionType;
        rigidBodySettings->m_linearDamping = 0.0f;
        rigidBodySettings->m_bodyShape = SphereShapeConfiguration::Create();
        Ptr<RigidBody> sphere = nullptr;
        SystemRequestBus::BroadcastResult(sphere, &SystemRequests::CreateRigidBody, rigidBodySettings);
        world->AddBody(sphere);
        sphere->SetTransform(AZ::Transform::CreateTranslation(position));
        return sphere;
    }

    Ptr<RigidBody> AddCapsuleToWorld(Ptr<World> world, const AZ::Vector3& position, MotionType motionType = MotionType::Dynamic)
    {
        Ptr<RigidBodySettings> rigidBodySettings = RigidBodySettings::Create();
        rigidBodySettings->m_bodyShape = CapsuleShapeConfiguration::Create(AZ::Vector3(0.0f, -0.5f, 0.0f),
            AZ::Vector3(0.0f, 0.5f, 0.0f), 0.5f);
        rigidBodySettings->m_motionType = motionType;
        Ptr<RigidBody> capsule = nullptr;
        SystemRequestBus::BroadcastResult(capsule, &SystemRequests::CreateRigidBody, rigidBodySettings);
        world->AddBody(capsule);
        capsule->SetTransform(AZ::Transform::CreateTranslation(position));
        return capsule;
    }

    void UpdateWorld(Ptr<World> world, float timeStep, AZ::u32 numSteps)
    {
        for (AZ::u32 i = 0; i < numSteps; i++)
        {
            world->Update(timeStep);
        }
    }

    TEST_F(GenericPhysicsInterfaceTest, World_CreateNewWorld_ReturnsNewWorld)
    {
        EXPECT_TRUE(CreateTestWorld() != nullptr);
        DestroyTestWorld();
    }

    TEST_F(GenericPhysicsInterfaceTest, RayCast_CastAgainstNothing_ReturnsNoHits)
    {
        RayCastRequest request;
        request.m_start = AZ::Vector3(-100.0f, 0.0f, 0.0f);
        request.m_dir = AZ::Vector3(1.0f, 0.0f, 0.0f);
        request.m_time = 200.0f;

        RayCastResult result;

        // TODO: Change this to use a bus providing Raycast functionality
        Ptr<World> defaultWorld = nullptr;
        SystemRequestBus::BroadcastResult(defaultWorld, &SystemRequests::GetDefaultWorld);
        defaultWorld->RayCast(request, result);

        EXPECT_TRUE(result.m_hits.size() == 0);
    }

    TEST_F(GenericPhysicsInterfaceTest, RayCast_CastAgainstSphere_ReturnsHits)
    {
        auto sphereEntity = AddSphereEntity(AZ::Vector3(0.0f), 10.0f, MotionType::Static);

        RayCastRequest request;
        request.m_start = AZ::Vector3(-100.0f, 0.0f, 0.0f);
        request.m_dir = AZ::Vector3(1.0f, 0.0f, 0.0f);
        request.m_time = 200.0f;

        RayCastResult result;

        // TODO: Change this to use a bus providing Raycast functionality
        Ptr<World> defaultWorld = nullptr;
        SystemRequestBus::BroadcastResult(defaultWorld, &SystemRequests::GetDefaultWorld);
        defaultWorld->RayCast(request, result);

        EXPECT_TRUE(result.m_hits.size() != 0);

        bool hitsIncludeSphereEntity = false;
        for (auto hit : result.m_hits)
        {
            hitsIncludeSphereEntity |= (hit.m_entityId == sphereEntity->GetId());
        }
        EXPECT_TRUE(hitsIncludeSphereEntity);

        // clear up scene
        delete sphereEntity;
    }

    TEST_F(GenericPhysicsInterfaceTest, ShapeCast_CastAgainstNothing_ReturnsNoHits)
    {
        ShapeCastRequest request;
        request.m_start = AZ::Transform::CreateTranslation(AZ::Vector3(-20.0f, 0.0f, 0.0f));
        request.m_end = AZ::Transform::CreateTranslation(request.m_start.GetPosition() + AZ::Vector3(1.0f, 0.0f, 0.0f) * 20.0f);
        request.m_shapeConfiguration = SphereShapeConfiguration::Create(1.0f);

        ShapeCastResult result;

        // TODO: Change this to use a bus providing Shapecast functionality
        Ptr<World> defaultWorld = nullptr;
        SystemRequestBus::BroadcastResult(defaultWorld, &SystemRequests::GetDefaultWorld);
        defaultWorld->ShapeCast(request, result);

        EXPECT_TRUE(result.m_hits.size() == 0);
    }

    TEST_F(GenericPhysicsInterfaceTest, ShapeCast_CastAgainstSphere_ReturnsHits)
    {
        auto sphereEntity = AddSphereEntity(AZ::Vector3(0.0f), 10.0f, MotionType::Static);

        ShapeCastRequest request;
        request.m_start = AZ::Transform::CreateTranslation(AZ::Vector3(-20.0f, 0.0f, 0.0f));
        request.m_end = AZ::Transform::CreateTranslation(request.m_start.GetPosition() + AZ::Vector3(1.0f, 0.0f, 0.0f) * 20.0f);
        request.m_shapeConfiguration = SphereShapeConfiguration::Create(1.0f);

        ShapeCastResult result;

        // TODO: Change this to use a bus providing Shapecast functionality
        Ptr<World> defaultWorld = nullptr;
        SystemRequestBus::BroadcastResult(defaultWorld, &SystemRequests::GetDefaultWorld);
        defaultWorld->ShapeCast(request, result);

        EXPECT_TRUE(result.m_hits.size() != 0);

        bool hitsIncludeSphereEntity = false;
        for (auto hit : result.m_hits)
        {
            hitsIncludeSphereEntity |= (hit.m_entityId == sphereEntity->GetId());
        }
        EXPECT_TRUE(hitsIncludeSphereEntity);

        // clear up scene
        delete sphereEntity;
    }

    TEST_F(GenericPhysicsInterfaceTest, Gravity_DynamicBody_BodyFalls)
    {
        auto world = CreateTestWorld();
        auto rigidBody = AddUnitBoxToWorld(world, AZ::Vector3(0.0f, 0.0f, 100.0f));
        UpdateWorld(world, 1.0f / 60.f, 60);

        // expect velocity to be -gt and distance fallen to be 1/2gt^2, but allow quite a lot of tolerance
        // due to potential differences in back end integration schemes etc.
        EXPECT_NEAR(rigidBody->GetLinearVelocity().GetZ(), -10.0f, 0.5f);
        EXPECT_NEAR(rigidBody->GetTransform().GetPosition().GetZ(), 95.0f, 0.5f);
        EXPECT_NEAR(rigidBody->GetCenterOfMassWorld().GetZ(), 95.0f, 0.5f);
        EXPECT_NEAR(rigidBody->GetPosition().GetZ(), 95.0f, 0.5f);

        DestroyTestWorld();
    }

    TEST_F(GenericPhysicsInterfaceTest, IncreaseMass_StaggeredTowerOfBoxes_TowerOverbalances)
    {
        auto world = CreateTestWorld();

        // make a tower of boxes which is staggered but should still balance if all the blocks are the same mass
        auto boxA = AddUnitBoxToWorld(world, AZ::Vector3(0.0f, 0.0f, 0.5f), MotionType::Static);
        auto boxB = AddUnitBoxToWorld(world, AZ::Vector3(0.3f, 0.0f, 1.5f));
        auto boxC = AddUnitBoxToWorld(world, AZ::Vector3(0.6f, 0.0f, 2.5f));

        // check that the tower balances
        UpdateWorld(world, 1.0f / 60.0f, 60);
        EXPECT_NEAR(2.5f, boxC->GetPosition().GetZ(), 0.01f);

        // increasing the mass of the top block in the tower should overbalance it
        boxC->SetMass(5.0f);
        EXPECT_NEAR(1.0f, boxB->GetMass(), 0.01f);
        EXPECT_NEAR(1.0f, boxB->GetInverseMass(), 0.01f);
        EXPECT_NEAR(5.0f, boxC->GetMass(), 0.01f);
        EXPECT_NEAR(0.2f, boxC->GetInverseMass(), 0.01f);
        boxB->ForceAwake();
        boxC->ForceAwake();
        UpdateWorld(world, 1.0f / 60.0f, 300);
        EXPECT_GT(0.0f, static_cast<float>(boxC->GetPosition().GetZ()));

        DestroyTestWorld();
    }

    TEST_F(GenericPhysicsInterfaceTest, GetCenterOfMass_FallingBody_CenterOfMassCorrectDuringFall)
    {
        auto world = CreateTestWorld();
        auto boxStatic = AddUnitBoxToWorld(world, AZ::Vector3(0.0f, 0.0f, 0.0f), MotionType::Static);
        auto boxDynamic = AddUnitBoxToWorld(world, AZ::Vector3(0.0f, 0.0f, 2.0f));
        auto vectorTolerance = AZ::VectorFloat(1e-3f);

        EXPECT_TRUE(boxStatic->GetCenterOfMassWorld().IsClose(AZ::Vector3(0.0f, 0.0f, 0.0f), vectorTolerance));
        EXPECT_TRUE(boxStatic->GetCenterOfMassLocal().IsClose(AZ::Vector3(0.0f, 0.0f, 0.0f), vectorTolerance));
        EXPECT_TRUE(boxDynamic->GetCenterOfMassWorld().IsClose(AZ::Vector3(0.0f, 0.0f, 2.0f), vectorTolerance));
        EXPECT_TRUE(boxDynamic->GetCenterOfMassLocal().IsClose(AZ::Vector3(0.0f, 0.0f, 0.0f), vectorTolerance));
        UpdateWorld(world, 1.0f / 60.0f, 300);
        EXPECT_TRUE(boxStatic->GetCenterOfMassWorld().IsClose(AZ::Vector3(0.0f, 0.0f, 0.0f), vectorTolerance));
        EXPECT_TRUE(boxStatic->GetCenterOfMassLocal().IsClose(AZ::Vector3(0.0f, 0.0f, 0.0f), vectorTolerance));
        EXPECT_NEAR(static_cast<float>(boxDynamic->GetCenterOfMassWorld().GetZ()), 1.0f, 1e-3f);
        EXPECT_TRUE(boxDynamic->GetCenterOfMassLocal().IsClose(AZ::Vector3(0.0f, 0.0f, 0.0f), vectorTolerance));

        DestroyTestWorld();
    }

    TEST_F(GenericPhysicsInterfaceTest, GetEnergy_FallingBody_GainsEnergy)
    {
        auto world = CreateTestWorld();
        auto floor = AddStaticFloorToWorld(world);
        auto box = AddUnitBoxToWorld(world, AZ::Vector3(0.0f, 0.0f, 10.0f));

        // the box should start with 0 energy and gain energy as it falls
        EXPECT_NEAR(box->GetEnergy(), 0.0f, 1e-3f);
        UpdateWorld(world, 1.0f / 60.0f, 30);
        float e1 = box->GetEnergy();
        UpdateWorld(world, 1.0f / 60.0f, 30);
        float e2 = box->GetEnergy();
        EXPECT_GT(e1, 0.0f);
        EXPECT_GT(e2, e1);

        // the box has now fallen for 1 second, so it should have fallen h = 1/2gt^2 = 5m
        // and it should have gained energy mgh = 50J
        // but allow quite a lot of tolerance because don't expect physics engine to be exact
        EXPECT_NEAR(e2, 50.0f, 5.0f);

        // once the box has landed and settled, its energy should be 0 again
        UpdateWorld(world, 1.0f / 60.0f, 600);
        EXPECT_NEAR(box->GetEnergy(), 0.0f, 1e-3f);

        DestroyTestWorld();
    }

    TEST_F(GenericPhysicsInterfaceTest, GetAndSetBodyMotionType_UnitBox_MotionTypeCorrect)
    {
        auto world = CreateTestWorld();
        auto boxA = AddUnitBoxToWorld(world, AZ::Vector3(0.0f, 0.0f, 0.0f), MotionType::Static);
        auto boxB = AddUnitBoxToWorld(world, AZ::Vector3(0.0f, 0.0f, 2.0f));

        EXPECT_TRUE(boxA->GetMotionType() == MotionType::Static);
        EXPECT_TRUE(boxB->GetMotionType() == MotionType::Dynamic);
        boxB->SetMotionType(MotionType::Static);
        EXPECT_TRUE(boxB->GetMotionType() == MotionType::Static);

        DestroyTestWorld();
    }

    TEST_F(GenericPhysicsInterfaceTest, SetLinearVelocity_DynamicBox_AffectsTrajectory)
    {
        auto world = CreateTestWorld();
        auto boxA = AddUnitBoxToWorld(world, AZ::Vector3(0.0f, -5.0f, 10.0f));
        auto boxB = AddUnitBoxToWorld(world, AZ::Vector3(0.0f, 5.0f, 10.0f));
        float xA = 0.0f;
        float xB = 0.0f;

        boxA->SetLinearVelocity(AZ::Vector3(10.0f, 0.0f, 0.0f));
        for (int i = 1; i < 10; i++)
        {
            UpdateWorld(world, 1.0f / 60.0f, 10);
            EXPECT_GT(static_cast<float>(boxA->GetPosition().GetX()), xA);
            EXPECT_NEAR(boxB->GetPosition().GetX(), xB, 1e-3f);
            xA = boxA->GetPosition().GetX();
            xB = boxB->GetPosition().GetX();
        }

        DestroyTestWorld();
    }

    TEST_F(GenericPhysicsInterfaceTest, ApplyLinearImpulse_DynamicBox_AffectsTrajectory)
    {
        auto world = CreateTestWorld();
        auto boxA = AddUnitBoxToWorld(world, AZ::Vector3(0.0f, 0.0f, 100.0f));
        auto boxB = AddUnitBoxToWorld(world, AZ::Vector3(0.0f, 10.0f, 100.0f));
        float xA = 0.0f;
        float xB = 0.0f;

        boxA->ApplyLinearImpulse(AZ::Vector3(10.0f, 0.0f, 0.0f));
        for (int i = 1; i < 10; i++)
        {
            UpdateWorld(world, 1.0f / 60.0f, 10);
            EXPECT_GT(static_cast<float>(boxA->GetPosition().GetX()), xA);
            EXPECT_NEAR(boxB->GetPosition().GetX(), xB, 1e-3f);
            xA = boxA->GetPosition().GetX();
            xB = boxB->GetPosition().GetX();
        }

        DestroyTestWorld();
    }

    TEST_F(GenericPhysicsInterfaceTest, GetAngularVelocity_DynamicCapsuleOnSlope_GainsAngularVelocity)
    {
        auto world = CreateTestWorld();
        AZ::Transform slopeTransform = AZ::Transform::CreateRotationY(0.1f);
        auto slope = AddStaticFloorToWorld(world, slopeTransform);
        auto capsule = AddCapsuleToWorld(world, slopeTransform * AZ::Vector3::CreateAxisZ());

        // the capsule should roll down the slope, picking up angular velocity parallel to the Y axis
        UpdateWorld(world, 1.0f / 60.0f, 60);
        float angularVelocityMagnitude = capsule->GetAngularVelocity().GetLength();
        for (int i = 0; i < 60; i++)
        {
            world->Update(1.0f / 60.0f);
            auto angularVelocity = capsule->GetAngularVelocity();
            EXPECT_TRUE(angularVelocity.IsPerpendicular(AZ::Vector3::CreateAxisX()));
            EXPECT_TRUE(angularVelocity.IsPerpendicular(AZ::Vector3::CreateAxisZ()));
            EXPECT_TRUE(capsule->GetAngularVelocity().GetLength() > angularVelocityMagnitude);
            angularVelocityMagnitude = angularVelocity.GetLength();
        }

        DestroyTestWorld();
    }

    TEST_F(GenericPhysicsInterfaceTest, SetAngularVelocity_DynamicCapsule_StartsRolling)
    {
        auto world = CreateTestWorld();
        auto floor = AddStaticFloorToWorld(world);
        auto capsule = AddCapsuleToWorld(world, AZ::Vector3::CreateAxisZ());

        // capsule should remain stationary
        for (int i = 0; i < 60; i++)
        {
            world->Update(1.0f / 60.0f);
            EXPECT_TRUE(capsule->GetPosition().IsClose(AZ::Vector3::CreateAxisZ()));
            EXPECT_TRUE(capsule->GetLinearVelocity().IsClose(AZ::Vector3::CreateZero()));
            EXPECT_TRUE(capsule->GetAngularVelocity().IsClose(AZ::Vector3::CreateZero()));
        }

        // apply an angular velocity and it should start rolling
        auto angularVelocity = AZ::Vector3::CreateAxisY(10.0f);
        capsule->SetAngularVelocity(angularVelocity);
        EXPECT_TRUE(capsule->GetAngularVelocity().IsClose(angularVelocity));
        float x = capsule->GetPosition().GetX();

        for (int i = 0; i < 60; i++)
        {
            world->Update(1.0f / 60.0f);
            EXPECT_TRUE(capsule->GetPosition().GetX() > x);
            x = capsule->GetPosition().GetX();
        }

        DestroyTestWorld();
    }

    TEST_F(GenericPhysicsInterfaceTest, GetLinearVelocityAtWorldPoint_RollingCapsule_EdgeVelocitiesCorrect)
    {
        auto world = CreateTestWorld();
        auto floor = AddStaticFloorToWorld(world);

        // create dynamic capsule and start it rolling
        auto capsule = AddCapsuleToWorld(world, AZ::Vector3::CreateAxisZ());
        capsule->SetLinearVelocity(AZ::Vector3::CreateAxisX(5.0f));
        capsule->SetAngularVelocity(AZ::Vector3::CreateAxisY(10.0f));
        UpdateWorld(world, 1.0f / 60.0f, 60);

        // check the velocities at some points on the rim of the capsule are as expected
        for (int i = 0; i < 60; i++)
        {
            world->Update(1.0f / 60.0f);
            auto position = capsule->GetPosition();
            float speed = capsule->GetLinearVelocity().GetX();
            AZ::Vector3 z = AZ::Vector3::CreateAxisZ(0.5f);
            AZ::Vector3 x = AZ::Vector3::CreateAxisX(0.5f);

            EXPECT_TRUE(capsule->GetLinearVelocityAtWorldPoint(position - z).IsClose(AZ::Vector3::CreateZero()));
            EXPECT_TRUE(capsule->GetLinearVelocityAtWorldPoint(position + z).IsClose(AZ::Vector3(2.0f * speed, 0.0f, 0.0f)));
            EXPECT_TRUE(capsule->GetLinearVelocityAtWorldPoint(position - x).IsClose(AZ::Vector3(speed, 0.0f, speed)));
            EXPECT_TRUE(capsule->GetLinearVelocityAtWorldPoint(position + x).IsClose(AZ::Vector3(speed, 0.0f, -speed)));
        }

        DestroyTestWorld();
    }

    TEST_F(GenericPhysicsInterfaceTest, GetPosition_RollingCapsule_OrientationCorrect)
    {
        auto world = CreateTestWorld();
        auto floor = AddStaticFloorToWorld(world);

        // create dynamic capsule and start it rolling
        auto capsule = AddCapsuleToWorld(world, AZ::Vector3::CreateAxisZ());
        capsule->SetLinearVelocity(AZ::Vector3::CreateAxisX(5.0f));
        capsule->SetAngularVelocity(AZ::Vector3::CreateAxisY(10.0f));
        UpdateWorld(world, 1.0f / 60.0f, 60);

        // check the capsule orientation evolves as expected
        auto prevOrientation = capsule->GetOrientation();
        float prevX = capsule->GetPosition().GetX();
        for (int i = 0; i < 60; i++)
        {
            world->Update(1.0f / 60.0f);
            float angle = 2.0f * (capsule->GetPosition().GetX() - prevX);
            EXPECT_TRUE(capsule->GetOrientation().IsClose(prevOrientation * AZ::Quaternion::CreateRotationY(angle)));
            prevOrientation = capsule->GetOrientation();
            prevX = capsule->GetPosition().GetX();
        }

        DestroyTestWorld();
    }

    TEST_F(GenericPhysicsInterfaceTest, OffCenterImpulse_DynamicCapsule_StartsRotating)
    {
        auto world = CreateTestWorld();
        auto floor = AddStaticFloorToWorld(world);
        AZ::Vector3 posA(0.0f, -5.0f, 1.0f);
        AZ::Vector3 posB(0.0f, 0.0f, 1.0f);
        AZ::Vector3 posC(0.0f, 5.0f, 1.0f);
        auto capsuleA = AddCapsuleToWorld(world, posA);
        auto capsuleB = AddCapsuleToWorld(world, posB);
        auto capsuleC = AddCapsuleToWorld(world, posC);

        // all the capsules should be stationary initially
        for (int i = 0; i < 10; i++)
        {
            world->Update(1.0f / 60.0f);
            EXPECT_TRUE(capsuleA->GetPosition().IsClose(posA));
            EXPECT_TRUE(capsuleA->GetAngularVelocity().IsClose(AZ::Vector3::CreateZero()));
            EXPECT_TRUE(capsuleB->GetPosition().IsClose(posB));
            EXPECT_TRUE(capsuleB->GetAngularVelocity().IsClose(AZ::Vector3::CreateZero()));
            EXPECT_TRUE(capsuleC->GetPosition().IsClose(posC));
            EXPECT_TRUE(capsuleC->GetAngularVelocity().IsClose(AZ::Vector3::CreateZero()));
        }

        // apply off-center impulses to capsule A and C, and an impulse through the center of B
        AZ::Vector3 impulse(0.0f, 0.0f, 10.0f);
        capsuleA->ApplyLinearImpulseAtWorldPoint(impulse, posA + AZ::Vector3::CreateAxisX(0.5f));
        capsuleB->ApplyLinearImpulseAtWorldPoint(impulse, posB);
        capsuleC->ApplyLinearImpulseAtWorldPoint(impulse, posC + AZ::Vector3::CreateAxisX(-0.5f));

        // A and C should be rotating in opposite directions, B should still have 0 angular velocity
        for (int i = 0; i < 30; i++)
        {
            world->Update(1.0f / 60.0f);
            EXPECT_TRUE(capsuleA->GetAngularVelocity().GetY() < 0.0f);
            EXPECT_TRUE(capsuleB->GetAngularVelocity().IsClose(AZ::Vector3::CreateZero()));
            EXPECT_TRUE(capsuleC->GetAngularVelocity().GetY() > 0.0f);
        }

        DestroyTestWorld();
    }

    TEST_F(GenericPhysicsInterfaceTest, ApplyAngularImpulse_DynamicSphere_StartsRotating)
    {
        auto world = CreateTestWorld();
        auto floor = AddStaticFloorToWorld(world);

        Ptr<RigidBody> spheres[3];
        for (int i = 0; i < 3; i++)
        {
            spheres[i] = AddSphereToWorld(world, AZ::Vector3(0.0f, -5.0f + 5.0f * i, 1.0f));
        }

        // all the spheres should start stationary
        UpdateWorld(world, 1.0f / 60.0f, 10);
        for (int i = 0; i < 3; i++)
        {
            EXPECT_TRUE(spheres[i]->GetAngularVelocity().IsClose(AZ::Vector3::CreateZero()));
        }

        // apply angular impulses and they should gain angular velocity parallel to the impulse direction
        AZ::Vector3 impulses[3] = { AZ::Vector3(2.0f, 4.0f, 0.0f), AZ::Vector3(-3.0f, 1.0f, 0.0f), AZ::Vector3(-2.0f, 3.0f, 0.0f) };
        for (int i = 0; i < 3; i++)
        {
            spheres[i]->ApplyAngularImpulse(impulses[i]);
        }

        UpdateWorld(world, 1.0f / 60.0f, 10);

        for (int i = 0; i < 3; i++)
        {
            auto angVel = spheres[i]->GetAngularVelocity();
            EXPECT_TRUE(angVel.GetProjected(impulses[i]).IsClose(angVel, AZ::VectorFloat(0.1f)));
        }

        DestroyTestWorld();
    }

    TEST_F(GenericPhysicsInterfaceTest, ForceAsleep_FallingBox_BecomesStationary)
    {
        auto world = CreateTestWorld();
        auto floor = AddStaticFloorToWorld(world);
        auto box = AddUnitBoxToWorld(world, AZ::Vector3(0.0f, 0.0f, 10.0f));
        UpdateWorld(world, 1.0f / 60.0f, 60);

        EXPECT_TRUE(box->IsAwake());

        auto pos = box->GetPosition();
        box->ForceAsleep();
        EXPECT_FALSE(box->IsAwake());
        UpdateWorld(world, 1.0f / 60.0f, 30);
        EXPECT_FALSE(box->IsAwake());
        // the box should be asleep so it shouldn't have moved
        EXPECT_TRUE(box->GetPosition().IsClose(pos));

        DestroyTestWorld();
    }

    TEST_F(GenericPhysicsInterfaceTest, ForceAwake_SleepingBox_SleepStateCorrect)
    {
        auto world = CreateTestWorld();
        auto floor = AddStaticFloorToWorld(world);
        auto box = AddUnitBoxToWorld(world, AZ::Vector3(0.0f, 0.0f, 1.0f));

        UpdateWorld(world, 1.0f / 60.0f, 60);
        EXPECT_FALSE(box->IsAwake());

        box->ForceAwake();
        EXPECT_TRUE(box->IsAwake());

        UpdateWorld(world, 1.0f / 60.0f, 60);
        // the box should have gone back to sleep
        EXPECT_FALSE(box->IsAwake());

        DestroyTestWorld();
    }

    TEST_F(GenericPhysicsInterfaceTest, GetAabb_Box_ValidExtents)
    {
        auto world = CreateTestWorld();
        AZ::Vector3 posBox(0.0f, 0.0f, 0.0f);
        auto box = AddUnitBoxToWorld(world, posBox, MotionType::Static);

        EXPECT_TRUE(box->GetAabb().GetMin().IsClose(posBox - 0.5f * AZ::Vector3::CreateOne()));
        EXPECT_TRUE(box->GetAabb().GetMax().IsClose(posBox + 0.5f * AZ::Vector3::CreateOne()));

        // rotate the box and check the bounding box is still correct
        AZ::Quaternion quat = AZ::Quaternion::CreateRotationZ(0.25f * AZ::Constants::Pi);
        box->SetTransform(AZ::Transform::CreateFromQuaternionAndTranslation(quat, posBox));

        AZ::Vector3 boxExtent(sqrtf(0.5f), sqrtf(0.5f), 0.5f);
        EXPECT_TRUE(box->GetAabb().GetMin().IsClose(posBox - boxExtent));
        EXPECT_TRUE(box->GetAabb().GetMax().IsClose(posBox + boxExtent));

        DestroyTestWorld();
    }

    TEST_F(GenericPhysicsInterfaceTest, GetAabb_Sphere_ValidExtents)
    {
        auto world = CreateTestWorld();
        AZ::Vector3 posSphere(0.0f, 0.0f, 0.0f);
        auto sphere = AddSphereToWorld(world, posSphere, MotionType::Static);

        EXPECT_TRUE(sphere->GetAabb().GetMin().IsClose(posSphere - 0.5f * AZ::Vector3::CreateOne()));
        EXPECT_TRUE(sphere->GetAabb().GetMax().IsClose(posSphere + 0.5f * AZ::Vector3::CreateOne()));

        // rotate the sphere and check the bounding box is still correct
        AZ::Quaternion quat = AZ::Quaternion::CreateRotationZ(0.25f * AZ::Constants::Pi);
        sphere->SetTransform(AZ::Transform::CreateFromQuaternionAndTranslation(quat, posSphere));

        EXPECT_TRUE(sphere->GetAabb().GetMin().IsClose(posSphere - 0.5f * AZ::Vector3::CreateOne()));
        EXPECT_TRUE(sphere->GetAabb().GetMax().IsClose(posSphere + 0.5f * AZ::Vector3::CreateOne()));

        DestroyTestWorld();
    }

    TEST_F(GenericPhysicsInterfaceTest, GetAabb_Capsule_ValidExtents)
    {
        auto world = CreateTestWorld();
        AZ::Vector3 posCapsule(0.0f, 0.0f, 0.0f);
        auto capsule = AddCapsuleToWorld(world, posCapsule, MotionType::Static);

        EXPECT_TRUE(capsule->GetAabb().GetMin().IsClose(posCapsule - AZ::Vector3(0.5f, 1.0f, 0.5f)));
        EXPECT_TRUE(capsule->GetAabb().GetMax().IsClose(posCapsule + AZ::Vector3(0.5f, 1.0f, 0.5f)));

        // rotate the bodies and check the bounding boxes are still correct
        AZ::Quaternion quat = AZ::Quaternion::CreateRotationZ(0.25f * AZ::Constants::Pi);
        capsule->SetTransform(AZ::Transform::CreateFromQuaternionAndTranslation(quat, posCapsule));

        AZ::Vector3 capsuleExtent(0.5f + sqrt(0.125f), 0.5f + sqrt(0.125f), 0.5f);
        EXPECT_TRUE(capsule->GetAabb().GetMin().IsClose(posCapsule - capsuleExtent));
        EXPECT_TRUE(capsule->GetAabb().GetMax().IsClose(posCapsule + capsuleExtent));

        DestroyTestWorld();
    }
} // namespace Physics
#endif // AZ_TESTS_ENABLED
