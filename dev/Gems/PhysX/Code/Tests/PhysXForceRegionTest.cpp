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

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Transform.h>

#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Physics/RigidBodyBus.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/TriggerBus.h>
#include <AzFramework/Physics/World.h>
#include <AzFramework/Physics/WorldEventhandler.h>

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
            m_defaultWorld = AZ::Interface<Physics::System>::Get()->CreateWorld(Physics::DefaultPhysicsWorldId);
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
            Physics::TriggerNotificationBus::QueueEvent(triggerEvent.m_triggerBody->GetEntityId()
                , &Physics::TriggerNotifications::OnTriggerEnter
                , triggerEvent);
        }

        void OnTriggerExit(const Physics::TriggerEvent& triggerEvent) override
        {
            Physics::TriggerNotificationBus::QueueEvent(triggerEvent.m_triggerBody->GetEntityId()
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

        auto colliderConfiguration = AZStd::make_shared<Physics::ColliderConfiguration>();
        auto boxShapeConfiguration = AZStd::make_shared<Physics::BoxShapeConfiguration>();
        auto boxColliderComponent = entity->CreateComponent<BoxColliderComponent>();
        boxColliderComponent->SetShapeConfigurationList({ AZStd::make_pair(colliderConfiguration, boxShapeConfiguration) });

        Physics::RigidBodyConfiguration rigidBodyConfig;
        entity->CreateComponent<PhysX::RigidBodyComponent>(rigidBodyConfig);

        entity->Init();
        entity->Activate();

        return entity;
    }

    namespace DefaultForceRegionParams
    {
        const AZ::Vector3 ForceDirection(0.0f, 0.0f, 1.0f);
        const AZ::Vector3 RotationY(.0f, 90.0f, .0f);
        const float ForceMagnitude = 100.0f;
        const float DampingRatio = 0.0f;
        const float Frequency = 1.0f;
        const float TargetSpeed = 1.0f;
        const float LookAhead = 0.0f;
        const float DragCoefficient = 1.0f;
        const float VolumeDensity = 5.0f;
        const float Damping = 10.0f;
    }

    template<typename ColliderType>
    AZStd::unique_ptr<AZ::Entity> AddForceRegion(const AZ::Vector3& position, ForceType forceType)
    {
        using namespace DefaultForceRegionParams;
        AZStd::unique_ptr<AZ::Entity> forceRegionEntity(aznew AZ::Entity("ForceRegion"));

        AZ::TransformConfig transformConfig;
        transformConfig.m_worldTransform = AZ::Transform::CreateTranslation(position);
        forceRegionEntity->CreateComponent<AzFramework::TransformComponent>()->SetConfiguration(transformConfig);

        auto colliderConfiguration = AZStd::make_shared<Physics::ColliderConfiguration>();
        colliderConfiguration->m_isTrigger = true;
        auto shapeConfiguration = AZStd::make_shared<typename ColliderType::Configuration>();

        auto colliderComponent = forceRegionEntity->CreateComponent<ColliderType>();
        colliderComponent->SetShapeConfigurationList({ AZStd::make_pair(colliderConfiguration, shapeConfiguration) });

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
                , ForceDirection
                , ForceMagnitude);
        }
        else if (forceType == LocalSpaceForce)
        {
            PhysX::ForceRegionRequestBus::Event(forceRegionEntity->GetId()
                , &PhysX::ForceRegionRequests::AddForceLocalSpace
                , ForceDirection
                , ForceMagnitude);
            AZ::TransformBus::Event(forceRegionEntity->GetId()
                , &AZ::TransformBus::Events::SetLocalRotation
                , RotationY);
        }
        else if (forceType == PointForce)
        {
            PhysX::ForceRegionRequestBus::Event(forceRegionEntity->GetId()
                , &PhysX::ForceRegionRequests::AddForcePoint
                , ForceMagnitude);
        }
        else if (forceType == SplineFollowForce)
        {
            PhysX::ForceRegionRequestBus::Event(forceRegionEntity->GetId()
                , &PhysX::ForceRegionRequests::AddForceSplineFollow
                , DampingRatio
                , Frequency
                , TargetSpeed
                , LookAhead
            );

            const AZStd::vector<AZ::Vector3> vertices =
            {
                AZ::Vector3(0.0f, 0.0f, 12.5f),
                AZ::Vector3(0.25f, 0.25f, 12.0f),
                AZ::Vector3(0.5f, 0.5f, 12.0f)
            };

            LmbrCentral::SplineComponentRequestBus::Event(forceRegionEntity->GetId()
                , &LmbrCentral::SplineComponentRequestBus::Events::SetVertices, vertices);
        }
        else if (forceType == SimpleDragForce)
        {
            PhysX::ForceRegionRequestBus::Event(forceRegionEntity->GetId()
                , &PhysX::ForceRegionRequests::AddForceSimpleDrag
                , DragCoefficient
                , VolumeDensity);
        }
        else if (forceType == LinearDampingForce)
        {
            PhysX::ForceRegionRequestBus::Event(forceRegionEntity->GetId()
                , &PhysX::ForceRegionRequests::AddForceLinearDamping
                , Damping);
        }

        return forceRegionEntity;
    }

    template<typename ColliderType>
    AZ::Vector3 TestForceVolume(ForceType forceType)
    {
        AZ::Vector3 velocity = AZ::Vector3::CreateZero();

        AZ::Vector3 position(0.0f, 0.0f, 16.0f);
        auto rigidBodyCollider = AddTestRigidBodyCollider(position, forceType, "TestBox");
        auto forceRegion = AddForceRegion<ColliderType>(AZ::Vector3(0.0f, 0.0f, 12.0f), forceType);

        AZStd::shared_ptr<Physics::World> world;
        Physics::DefaultWorldBus::BroadcastResult(world, &Physics::DefaultWorldRequests::GetDefaultWorld);

        //Run simulation for a while - bounces box once on force volume
        float deltaTime = 1.0f / 180.0f;
        for (int timeStep = 0; timeStep < 240; timeStep++)
        {
            world->Update(deltaTime);
        }

        Physics::RigidBodyRequestBus::EventResult(velocity
            , rigidBodyCollider->GetId()
            , &Physics::RigidBodyRequestBus::Events::GetLinearVelocity);

        return velocity;
    }

    template<typename ColliderType>
    void TestAppliesSameMagnitude(ForceType forceType)
    {
        struct ForceRegionMagnitudeChecker
            : public ForceRegionNotificationBus::Handler
        {
            ForceRegionMagnitudeChecker()  { ForceRegionNotificationBus::Handler::BusConnect(); }
            ~ForceRegionMagnitudeChecker() { ForceRegionNotificationBus::Handler::BusDisconnect(); }

            void OnCalculateNetForce(AZ::EntityId, AZ::EntityId, const AZ::Vector3&, float netForceMagnitude)
            {
                // This callback can potentially be called in every frame, so just only catch the first failure to avoid spamming
                if (!m_failed)
                {
                    const float forceRegionMaxError = 0.05f; // Force region uses fast approximation for length calculations, hence the error
                    const bool result = AZ::IsClose(netForceMagnitude, DefaultForceRegionParams::ForceMagnitude, forceRegionMaxError);
                    if (!result)
                    {
                        m_failed = true;
                    }
                    EXPECT_TRUE(result);
                }
            }

            bool m_failed = false;
        };
        ForceRegionMagnitudeChecker magnitudeChecker;
        TestForceVolume<ColliderType>(forceType);
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

    TEST_F(PhysXForceRegionTest, ForceRegion_PointForce_AppliesSameMagnitude)
    {
        TestAppliesSameMagnitude<BoxColliderComponent>(PointForce);
    }

    TEST_F(PhysXForceRegionTest, ForceRegion_WorldSpaceForce_AppliesSameMagnitude)
    {
        TestAppliesSameMagnitude<BoxColliderComponent>(WorldSpaceForce);
    }

    TEST_F(PhysXForceRegionTest, ForceRegion_LocalSpaceForce_AppliesSameMagnitude)
    {
        TestAppliesSameMagnitude<BoxColliderComponent>(LocalSpaceForce);
    }
}
#endif // AZ_TESTS_ENABLED
