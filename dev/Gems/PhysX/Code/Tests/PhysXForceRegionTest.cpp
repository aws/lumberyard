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
#include <PhysX_precompiled.h>

#ifdef AZ_TESTS_ENABLED

#include <AzTest/AzTest.h>

#include <BoxColliderComponent.h>
#include <ForceRegionComponent.h>
#include <RigidBodyComponent.h>

#include <PhysX/ForceRegionComponentBus.h>

#include <LmbrCentral/Shape/SplineComponentBus.h>

#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Transform.h>

#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Physics/RigidBodyBus.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/TriggerBus.h>
#include <AzFramework/Physics/World.h>
#include <AzFramework/Physics/WorldEventHandler.h>

namespace PhysX
{
    enum ForceType : AZ::u8
    {
        WorldSpaceForce
        , LocalSpaceForce
        , PointForce
        , SplineFollowForce
        , SimpleDragForce
        , LinearDampingForce
    };

    class PhysXForceRegionTest
        : public::testing::Test
        , protected Physics::DefaultWorldBus::Handler
        , protected Physics::WorldEventHandler
    {
        void SetUp() override
        {
            Physics::SystemRequestBus::BroadcastResult(m_defaultWorld,
                &Physics::SystemRequests::CreateWorld, AZ_CRC("UnitTestWorld", 0x39d5e465));
            m_defaultWorld->SetEventHandler(this);

            Physics::DefaultWorldBus::Handler::BusConnect();
        }

        void TearDown() override
        {
            Physics::DefaultWorldBus::Handler::BusDisconnect();
            m_defaultWorld = nullptr;
        }

        // DefaultWorldBus
        AZStd::shared_ptr<Physics::World> GetDefaultWorld() override
        {
            return m_defaultWorld;
        }

        // WorldEventHandler
        void OnCollisionBegin(const Physics::CollisionEvent&) override //Not used in force region tests
        {
        }

        void OnCollisionPersist(const Physics::CollisionEvent&) override //Not used in force region tests
        {
        }

        void OnCollisionEnd(const Physics::CollisionEvent&) override //Not used in force region tests
        {
        }

        void OnTriggerEnter(const Physics::TriggerEvent& triggerEvent) override
        {
            Physics::TriggerNotificationBus::Event(triggerEvent.m_triggerBody->GetEntityId()
                , &Physics::TriggerNotifications::OnTriggerEnter
                , triggerEvent);
        }

        void OnTriggerExit(const Physics::TriggerEvent& triggerEvent) override
        {
            Physics::TriggerNotificationBus::Event(triggerEvent.m_triggerBody->GetEntityId()
                , &Physics::TriggerNotifications::OnTriggerExit
                , triggerEvent);
        }



        AZStd::shared_ptr<Physics::World> m_defaultWorld;
    };

    AZStd::unique_ptr<AZ::Entity> AddTestRigidBodyCollider(AZ::Vector3& position
        , ForceType forceType
        , const char* name = "TestObjectEntity")
    {
        AZStd::unique_ptr<AZ::Entity> entity(aznew AZ::Entity(name));

        AZ::TransformConfig transformConfig;
        if (forceType == PointForce)
        {
            position.SetX(0.05f);
        }
        transformConfig.m_worldTransform = AZ::Transform::CreateTranslation(position);
        entity->CreateComponent<AzFramework::TransformComponent>()->SetConfiguration(transformConfig);

        Physics::ColliderConfiguration colliderConfiguartion;
        Physics::BoxShapeConfiguration BoxShapeConfiguration;
        entity->CreateComponent<BoxColliderComponent>(colliderConfiguartion, BoxShapeConfiguration);

        Physics::RigidBodyConfiguration rigidBodyConfig;
        entity->CreateComponent<PhysX::RigidBodyComponent>(rigidBodyConfig);

        entity->Init();
        entity->Activate();

        return entity;
    }

    template<typename ColliderType>
    AZStd::unique_ptr<AZ::Entity> AddForceRegion(const AZ::Vector3& position, ForceType forceType)
    {
        AZStd::unique_ptr<AZ::Entity> forceRegionEntity(aznew AZ::Entity("ForceRegion"));

        AZ::TransformConfig transformConfig;
        transformConfig.m_worldTransform = AZ::Transform::CreateTranslation(position);
        forceRegionEntity->CreateComponent<AzFramework::TransformComponent>()->SetConfiguration(transformConfig);

        Physics::ColliderConfiguration colliderConfiguartion;
        colliderConfiguartion.m_isTrigger = true;

        Physics::BoxShapeConfiguration BoxShapeConfiguration;

        forceRegionEntity->CreateComponent<ColliderType>(colliderConfiguartion, BoxShapeConfiguration);

        forceRegionEntity->CreateComponent<ForceRegionComponent>();

        if (forceType == SplineFollowForce)
        {
            forceRegionEntity->CreateComponent("{F0905297-1E24-4044-BFDA-BDE3583F1E57}");//SplineComponent
        }

        forceRegionEntity->Init();
        forceRegionEntity->Activate();

        if (forceType == WorldSpaceForce)
        {
            PhysX::ForceRegionRequestBus::Event(forceRegionEntity->GetId()
                , &PhysX::ForceRegionRequests::AddForceWorldSpace
                , AZ::Vector3(0.0f, 0.0f, 1.0f)
                , 100.0f);
        }
        else if (forceType == LocalSpaceForce)
        {
            PhysX::ForceRegionRequestBus::Event(forceRegionEntity->GetId()
                , &PhysX::ForceRegionRequests::AddForceLocalSpace
                , AZ::Vector3(0.0f, 0.0f, 1.0f)
                , 100.0f);
            AZ::TransformBus::Event(forceRegionEntity->GetId()
                , &AZ::TransformBus::Events::SetLocalRotation
                , AZ::Vector3(.0f, 90.0f, .0f));
        }
        else if (forceType == PointForce)
        {
            PhysX::ForceRegionRequestBus::Event(forceRegionEntity->GetId()
                , &PhysX::ForceRegionRequests::AddForcePoint
                , 100.0f);
        }
        else if (forceType == SplineFollowForce)
        {
            PhysX::ForceRegionRequestBus::Event(forceRegionEntity->GetId()
                , &PhysX::ForceRegionRequests::AddForceSplineFollow
                , 0.0f
                , 1.0f
                , 1.0f
                , 0.0f
            );

            AZStd::vector<AZ::Vector3> vertices;
            vertices.emplace_back(AZ::Vector3(0.0f, 0.0f, 12.5f));
            vertices.emplace_back(AZ::Vector3(0.25f, 0.25f, 12.0f));
            vertices.emplace_back(AZ::Vector3(0.5f, 0.5f, 12.0f));

            LmbrCentral::SplineComponentRequestBus::Event(forceRegionEntity->GetId()
                , &LmbrCentral::SplineComponentRequestBus::Events::SetVertices, vertices);
        }
        else if (forceType == SimpleDragForce)
        {
            PhysX::ForceRegionRequestBus::Event(forceRegionEntity->GetId()
                , &PhysX::ForceRegionRequests::AddForceSimpleDrag
                , 7.0f
                , 1.1f);
        }
        else if (forceType == LinearDampingForce)
        {
            PhysX::ForceRegionRequestBus::Event(forceRegionEntity->GetId()
                , &PhysX::ForceRegionRequests::AddForceLinearDamping
                , 100.0f);
        }

        return forceRegionEntity;
    }

    template<typename ColliderType>
    AZ::Vector3 TestForceVolume(ForceType forceType)
    {
        AZ::Vector3 velocity = AZ::Vector3::CreateZero();

        auto rigidBodyCollider = AddTestRigidBodyCollider(AZ::Vector3(0.0f, 0.0f, 16.0f), forceType, "TestBox");
        auto forceRegion = AddForceRegion<ColliderType>(AZ::Vector3(0.0f, 0.0f, 12.0f), forceType);

        AZStd::shared_ptr<Physics::World> world;
        Physics::DefaultWorldBus::BroadcastResult(world, &Physics::DefaultWorldRequests::GetDefaultWorld);

        //Run simulation for a while - bounces box once on force volume
        float deltaTime = 1.0f / 180.0f;
        for (int timeStep = 0; timeStep < 240; timeStep++)
        {
            world->Update(deltaTime);

            //mock game tick so that force volume exerts forces on entities in it
            AZStd::chrono::system_clock::time_point now = AZStd::chrono::system_clock::now();
            EBUS_EVENT(AZ::TickBus, OnTick, deltaTime, AZ::ScriptTimePoint(now));
        }

        Physics::RigidBodyRequestBus::EventResult(velocity
            , rigidBodyCollider->GetId()
            , &Physics::RigidBodyRequestBus::Events::GetLinearVelocity);

        return velocity;
    }

    TEST_F(PhysXForceRegionTest, ForceRegion_WorldSpaceForce_EntityVelocityZPositive)
    {
        AZ::Vector3 entityVelocity = TestForceVolume<BoxColliderComponent>(WorldSpaceForce);
        // World space force direction: AZ::Vector3(0.0f, 0.0f, 1.0f)
        EXPECT_TRUE(entityVelocity.GetZ().IsGreaterThan(0.0f)); // World space force causes box to bounce upwards
        EXPECT_TRUE(entityVelocity.GetX().IsClose(0.0f));
        EXPECT_TRUE(entityVelocity.GetY().IsClose(0.0f));
    }

    TEST_F(PhysXForceRegionTest, ForceRegion_LocalSpaceForce_EntityVelocityZPositive)
    {
        AZ::Vector3 entityVelocity = TestForceVolume<BoxColliderComponent>(LocalSpaceForce);
        // Local space force direction: AZ::Vector3(0.0f, 0.0f, 1.0f)
        // Force region was rotated about Y-axis by 90 deg
        EXPECT_TRUE(entityVelocity.GetX().IsGreaterThan(0.0f)); // Falling body should be moving in positive X direction since force region is rotated.
        EXPECT_TRUE(entityVelocity.GetY().IsClose(0.0f));
        EXPECT_TRUE(entityVelocity.GetZ().IsLessThan(0.0f)); // Gravity
    }

    TEST_F(PhysXForceRegionTest, ForceRegion_PointForce_EntityVelocityZPositive)
    {
        // Falling body was positioned at AZ::Vector3(0.05f, 0.0f, 16.0f)
        // Force region was positioned at AZ::Vector3(0.0f, 0.0f, 12.0f)
        // PointForce causes box to bounce upwards and to the right.
        AZ::Vector3 entityVelocity = TestForceVolume<BoxColliderComponent>(PointForce);
        EXPECT_TRUE(entityVelocity.GetX().IsGreaterThan(0.0f));
        EXPECT_TRUE(entityVelocity.GetY().IsClose(0.0f));
        EXPECT_TRUE(entityVelocity.GetZ().IsGreaterThan(0.0f)); 
    }

    TEST_F(PhysXForceRegionTest, ForceRegion_SplineFollowForce_EntityVelocitySpecificValue)
    {
        AZ::Vector3 entityVelocity = TestForceVolume<BoxColliderComponent>(SplineFollowForce);
        // Follow spline direction towards positive X and Y.
        EXPECT_TRUE(entityVelocity.GetX().IsGreaterThan(0.0f)); 
        EXPECT_TRUE(entityVelocity.GetY().IsGreaterThan(0.0f));
    }

    TEST_F(PhysXForceRegionTest, ForceRegion_SimpleDragForce_EntityVelocitySpecificValue)
    {
        AZ::Vector3 entityVelocity = TestForceVolume<BoxColliderComponent>(SimpleDragForce);
        EXPECT_TRUE(entityVelocity.GetZ().IsGreaterThan(-12.65f)); // Falling velocity should be slower than free fall velocity, which is -12.65.
        EXPECT_TRUE(entityVelocity.GetX().IsClose(0.0f)); // Dragging should not change original direction.
        EXPECT_TRUE(entityVelocity.GetY().IsClose(0.0f)); // Dragging should not change original direction.
    } 

    TEST_F(PhysXForceRegionTest, ForceRegion_LinearDampingForce_EntityVelocitySpecificValue)
    {
        AZ::Vector3 entityVelocity = TestForceVolume<BoxColliderComponent>(LinearDampingForce);
        EXPECT_TRUE(entityVelocity.GetZ().IsGreaterThan(-12.65f)); // Falling velocity should be slower than free fall velocity, which is -12.65.
        EXPECT_TRUE(entityVelocity.GetX().IsClose(0.0f)); // Damping should not change original direction.
        EXPECT_TRUE(entityVelocity.GetY().IsClose(0.0f)); // Damping should not change original direction.
    }
}
#endif // AZ_TESTS_ENABLED