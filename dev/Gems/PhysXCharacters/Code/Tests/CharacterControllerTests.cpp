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

#include <PhysXCharacters_precompiled.h>

#include "TestEnvironment.h"

#include <API/CharacterController.h>
#include <API/Utils.h>
#include <Components/CharacterControllerComponent.h>
#include <PhysXCharacters/SystemBus.h>
#include <System/SystemComponent.h>

#include <AzFramework/Components/TransformComponent.h>
#include <PhysX/ComponentTypeIds.h>
#include <PhysX/SystemComponentBus.h>

namespace PhysXCharacters
{
    class ControllerTestBasis
    {
    public:
        ControllerTestBasis(const Physics::ShapeType shapeType = Physics::ShapeType::Capsule, const AZ::Transform& floorTransform = DefaultFloorTransform)
        {
            SetUp(shapeType, floorTransform);
        }

        void SetUp(const Physics::ShapeType shapeType = Physics::ShapeType::Capsule, const AZ::Transform& floorTransform = DefaultFloorTransform)
        {
            Physics::DefaultWorldBus::BroadcastResult(m_world, &Physics::DefaultWorldRequests::GetDefaultWorld);

            m_floor = Physics::AddStaticFloorToWorld(m_world.get(), floorTransform);

            CharacterControllerConfiguration characterConfig;
            characterConfig.m_maximumSlopeAngle = 25.0f;
            characterConfig.m_stepHeight = 0.2f;

            if (shapeType == Physics::ShapeType::Box)
            {
                Physics::BoxShapeConfiguration shapeConfig(AZ::Vector3(0.5f, 0.5f, 1.0f));
                Physics::CharacterSystemRequestBus::BroadcastResult(m_controller,
                    &Physics::CharacterSystemRequests::CreateCharacter, characterConfig, shapeConfig, *m_world);
            }
            else
            {
                Physics::CapsuleShapeConfiguration shapeConfig;
                Physics::CharacterSystemRequestBus::BroadcastResult(m_controller,
                    &Physics::CharacterSystemRequests::CreateCharacter, characterConfig, shapeConfig, *m_world);
            }

            ASSERT_TRUE(m_controller != nullptr);

            m_controller->SetBasePosition(AZ::Vector3::CreateZero());
        }

        ~ControllerTestBasis()
        {
            m_controller = nullptr;
            m_world = nullptr;
            m_floor = nullptr;
        }

        void Update(const AZ::Vector3& movementDelta, AZ::u32 numTimeSteps = 1)
        {
            for (AZ::u32 i = 0; i < numTimeSteps; i++)
            {
                m_controller->TryRelativeMove(movementDelta, m_timeStep);
                m_world->Update(m_timeStep);
            }
        }

        AZStd::shared_ptr<Physics::World> m_world;
        AZStd::shared_ptr<Physics::RigidBodyStatic> m_floor;
        AZStd::unique_ptr<Physics::Character> m_controller;
        float m_timeStep = 1.0f / 60.0f;
    };

    Physics::ShapeType controllerShapeTypes[] = { Physics::ShapeType::Capsule, Physics::ShapeType::Box };

    TEST_F(PhysXCharactersTest, CharacterController_UnimpededController_MovesAtDesiredVelocity)
    {
        ControllerTestBasis basis;
        AZ::Vector3 desiredVelocity = AZ::Vector3::CreateAxisX();
        AZ::Vector3 movementDelta = desiredVelocity * basis.m_timeStep;

        for (int i = 0; i < 50; i++)
        {
            AZ::Vector3 basePosition = basis.m_controller->GetBasePosition();
            EXPECT_TRUE(basePosition.IsClose(AZ::Vector3::CreateAxisX(basis.m_timeStep * i)));
            basis.Update(movementDelta);
            EXPECT_TRUE(basis.m_controller->GetVelocity().IsClose(desiredVelocity));
        }
    }

    TEST_F(PhysXCharactersTest, CharacterController_MovingDirectlyTowardsStaticBox_StoppedByBox)
    {
        ControllerTestBasis basis;
        AZ::Vector3 movementDelta = AZ::Vector3::CreateAxisX(basis.m_timeStep);

        auto box = Physics::AddStaticUnitBoxToWorld(basis.m_world.get(), AZ::Vector3(1.5f, 0.0f, 0.5f));

        // run the simulation for a while so the controller should get to the box and stop
        basis.Update(movementDelta, 50);

        // the edge of the box is at x = 1.0, we expect to stop a distance short of that given by the sum of the
        // capsule radius (0.25) and the contact offset (0.1)
        AZ::Vector3 basePosition = basis.m_controller->GetBasePosition();
        EXPECT_TRUE(basePosition.IsClose(AZ::Vector3::CreateAxisX(0.65f)));

        // run the simulation some more and check that the controller is not moving in the direction of the box
        for (int i = 0; i < 10; i++)
        {
            AZ::Vector3 newBasePosition = basis.m_controller->GetBasePosition();
            EXPECT_TRUE(newBasePosition.IsClose(basePosition));
            EXPECT_TRUE(basis.m_controller->GetVelocity().IsClose(AZ::Vector3::CreateZero()));
            basePosition = newBasePosition;
            basis.Update(movementDelta);
        }

        box = nullptr;
    }

    TEST_F(PhysXCharactersTest, CharacterController_MovingDiagonallyTowardsStaticBox_SlidesAlongBox)
    {
        ControllerTestBasis basis;
        AZ::Vector3 movementDelta = AZ::Vector3(1.0f, 1.0f, 0.0f) * basis.m_timeStep;

        auto box = Physics::AddStaticUnitBoxToWorld(basis.m_world.get(), AZ::Vector3(1.0f, 0.5f, 0.5f));

        // run the simulation for a while so the controller should get to the box and start sliding
        basis.Update(movementDelta, 20);

        // the controller should be sliding in the y direction now
        for (int i = 0; i < 10; i++)
        {
            AZ::Vector3 velocity = basis.m_controller->GetVelocity();
            float vx = velocity.GetX();
            float vy = velocity.GetY();
            EXPECT_NEAR(vx, 0.0f, 1e-3f);
            EXPECT_NEAR(vy, 1.0f, 1e-3f);
            basis.Update(movementDelta);
        }

        box = nullptr;
    }

    TEST_F(PhysXCharactersTest, CharacterController_MovingOnSlope_CannotMoveAboveMaximumSlopeAngle)
    {
        // create a floor sloped at 30 degrees which should just be touching a controller with base position at the 
        // origin, with radius + contact offset = 0.25 + 0.1 = 0.35
        AZ::Transform slopedFloorTransform = AZ::Transform::CreateRotationY(-AZ::Constants::Pi / 6.0f);
        slopedFloorTransform.SetTranslation(AZ::Vector3::CreateAxisZ(0.35f) + slopedFloorTransform * AZ::Vector3::CreateAxisZ(-0.85f));
        ControllerTestBasis basis(Physics::ShapeType::Capsule, slopedFloorTransform);

        // we should be able to travel at right angles to the slope
        AZ::Vector3 desiredVelocity = AZ::Vector3::CreateAxisY();
        AZ::Vector3 movementDelta = desiredVelocity * basis.m_timeStep;

        for (int i = 0; i < 50; i++)
        {
            basis.Update(movementDelta);
            EXPECT_TRUE(basis.m_controller->GetVelocity().IsClose(desiredVelocity));
        }

        // we should slide if we try to travel diagonally up the slope as it is steeper than our maximum of 25 degrees
        desiredVelocity = AZ::Vector3(1.0f, 1.0f, 0.0f);
        movementDelta = desiredVelocity * basis.m_timeStep;

        // run a few frames to adjust to the change in direction
        basis.Update(movementDelta, 10);

        for (int i = 0; i < 50; i++)
        {
            basis.Update(movementDelta);
            AZ::Vector3 velocity = basis.m_controller->GetVelocity();
            float vx = velocity.GetX();
            float vy = velocity.GetY();
            EXPECT_NEAR(vx, 0.0f, 1e-3f);
            EXPECT_NEAR(vy, 1.0f, 1e-3f);
        }

        // shouldn't be able to travel directly up the 30 degree slope as our maximum slope angle is 25 degrees
        desiredVelocity = AZ::Vector3(1.0f, 0.0f, 0.0f);
        movementDelta = desiredVelocity * basis.m_timeStep;

        for (int i = 0; i < 50; i++)
        {
            basis.Update(movementDelta);
            AZ::Vector3 velocity = basis.m_controller->GetVelocity();
            EXPECT_TRUE(basis.m_controller->GetVelocity().IsClose(AZ::Vector3::CreateZero()));
        }

        // should be able to move down the slope
        desiredVelocity = AZ::Vector3(-1.0f, 0.0f, -0.5f);
        movementDelta = desiredVelocity * basis.m_timeStep;

        // run a few frames to adjust to the change in direction
        basis.Update(movementDelta, 10);

        for (int i = 0; i < 50; i++)
        {
            basis.Update(movementDelta);
            AZ::Vector3 velocity = basis.m_controller->GetVelocity();
            EXPECT_TRUE(basis.m_controller->GetVelocity().IsClose(desiredVelocity));
        }
    }

    TEST_F(PhysXCharactersTest, CharacterController_Steps_StoppedByTallStep)
    {
        ControllerTestBasis basis;

        auto shortStep = Physics::AddStaticUnitBoxToWorld(basis.m_world.get(), AZ::Vector3(1.0f, 0.0f, -0.3f));
        auto tallStep = Physics::AddStaticUnitBoxToWorld(basis.m_world.get(), AZ::Vector3(2.0f, 0.0f, 0.5f));

        AZ::Vector3 desiredVelocity = AZ::Vector3::CreateAxisX();
        AZ::Vector3 movementDelta = desiredVelocity * basis.m_timeStep;

        for (int i = 0; i < 50; i++)
        {
            basis.Update(movementDelta);
            AZ::Vector3 velocity = basis.m_controller->GetVelocity();
            float vx = velocity.GetX();
            float vy = velocity.GetY();
            EXPECT_NEAR(vx, 1.0f, 1e-3f);
            EXPECT_NEAR(vy, 0.0f, 1e-3f);
        }

        // expect the base of the controller to now be at the height of the short step (0.2)
        float expectedBaseHeight = 0.2f;
        float baseHeight = basis.m_controller->GetBasePosition().GetZ();
        EXPECT_NEAR(baseHeight, expectedBaseHeight, 1e-3f);

        // after another 50 updates, we should have been stopped by the tall step
        basis.Update(movementDelta, 50);
        EXPECT_TRUE(basis.m_controller->GetVelocity().IsClose(AZ::Vector3::CreateZero()));
        baseHeight = basis.m_controller->GetBasePosition().GetZ();
        EXPECT_NEAR(baseHeight, expectedBaseHeight, 1e-3f);

        shortStep = nullptr;
        tallStep = nullptr;
    }

    using CharacterControllerFixture = ::testing::TestWithParam<Physics::ShapeType>;

    TEST_P(CharacterControllerFixture, CharacterController_ResizedController_CannotFitUnderLowBox)
    {
        Physics::ShapeType shapeType = GetParam();
        ControllerTestBasis basis(shapeType);

        // the bottom of the box will be at height 1.0
        auto box = Physics::AddStaticUnitBoxToWorld(basis.m_world.get(), AZ::Vector3(1.0f, 0.0f, 1.5f));

        // resize the controller so that it is too tall to fit under the box
        auto controller = static_cast<PhysXCharacters::CharacterController*>(basis.m_controller.get());
        controller->Resize(1.3f);
        EXPECT_NEAR(controller->GetHeight(), 1.3f, 1e-3f);

        const AZ::Vector3 desiredVelocity = AZ::Vector3::CreateAxisX();
        const AZ::Vector3 movementDelta = desiredVelocity * basis.m_timeStep;

        basis.Update(movementDelta, 50);
        // movement should be impeded by the box because the controller is too tall to go under it
        EXPECT_TRUE(basis.m_controller->GetVelocity().IsClose(AZ::Vector3::CreateZero()));

        // resize the controller to a bit less tall than the height of the bottom of the box
        // leave some leeway under the box to account for the contact offset of the controller
        controller->Resize(0.6f);
        EXPECT_NEAR(controller->GetHeight(), 0.6f, 1e-3f);

        basis.Update(movementDelta, 50);
        // movement should now be unimpeded because the controller is short enough to go under the box
        const AZ::Vector3 velocity = basis.m_controller->GetVelocity();
        const float vx = velocity.GetX();
        const float vy = velocity.GetY();
        EXPECT_NEAR(vx, 1.0f, 1e-3f);
        EXPECT_NEAR(vy, 0.0f, 1e-3f);

        box = nullptr;
    }

    TEST_P(CharacterControllerFixture, CharacterController_ResizingToNegativeHeight_EmitsError)
    {
        Physics::ShapeType shapeType = GetParam();
        ControllerTestBasis basis(shapeType);
        auto controller = static_cast<PhysXCharacters::CharacterController*>(basis.m_controller.get());
        Physics::ErrorHandler errorHandler("PhysX requires controller height to be positive");
        controller->Resize(-0.2f);
        EXPECT_EQ(errorHandler.GetErrorCount(), 1);
    }

    INSTANTIATE_TEST_CASE_P(PhysXCharacters, CharacterControllerFixture, ::testing::ValuesIn(controllerShapeTypes));

    TEST_F(PhysXCharactersTest, CharacterController_ResizingCapsuleControllerBelowTwiceRadius_EmitsError)
    {
        ControllerTestBasis basis;

        auto controller = static_cast<PhysXCharacters::CharacterController*>(basis.m_controller.get());
        // the controller will have been made with the default radius of 0.25, so any height under 0.5 should
        // be impossible
        Physics::ErrorHandler errorHandler("Capsule height must exceed twice its radius");
        controller->Resize(0.45f);
        EXPECT_EQ(errorHandler.GetErrorCount(), 1);

        // the controller should still have the default height of 1
        EXPECT_NEAR(controller->GetHeight(), 1.0f, 1e-3f);
    }

    TEST_F(PhysXCharactersTest, CharacterController_DroppingBox_CollidesWithController)
    {
        ControllerTestBasis basis;

        auto box = Physics::AddUnitBoxToWorld(basis.m_world.get(), AZ::Vector3(0.5f, 0.0f, 5.0f));

        basis.Update(AZ::Vector3::CreateZero(), 200);

        // the box and controller have default collision layer and group so should collide
        // the box was positioned to land on its edge on the controller
        // so expect the box to have bounced off the controller and travelled in the x direction
        AZ::Vector3 boxPosition = box->GetPosition();
        float x = boxPosition.GetX();
        EXPECT_GT(x, 2.0f);

        box = nullptr;
    }

    TEST_F(PhysXCharactersTest, CharacterController_RaycastAgainstController_ReturnsHit)
    {
        // raycast on an empty scene should return no hits
        Physics::RayCastRequest request;
        request.m_start = AZ::Vector3(-100.0f, 0.0f, 0.25f);
        request.m_direction = AZ::Vector3(1.0f, 0.0f, 0.0f);
        request.m_distance = 200.0f;

        Physics::RayCastHit hit;
        Physics::WorldRequestBus::BroadcastResult(hit, &Physics::WorldRequests::RayCast, request);
        EXPECT_FALSE(hit);

        // now add a controller and raycast again
        ControllerTestBasis basis;

        // the controller won't move to its initial position with its base at the origin until one update has happened
        basis.Update(AZ::Vector3::CreateZero());

        Physics::WorldRequestBus::BroadcastResult(hit, &Physics::WorldRequests::RayCast, request);

        EXPECT_TRUE(hit);
    }

    TEST_F(PhysXCharactersTest, CharacterController_DeleteCharacterInsideTrigger_RaisesExitEvent)
    {
        // Create trigger
        Physics::ColliderConfiguration triggerConfig;
        triggerConfig.m_isTrigger = true;
        Physics::BoxShapeConfiguration boxConfig;
        boxConfig.m_dimensions = AZ::Vector3(10.0f, 10.0f, 10.0f);

        auto triggerEntity = AZStd::make_unique<AZ::Entity>("TriggerEntity");
        triggerEntity->CreateComponent<AzFramework::TransformComponent>()->SetWorldTM(AZ::Transform::Identity());
        triggerEntity->CreateComponent(PhysX::StaticRigidBodyComponentTypeId);
        PhysX::SystemRequestsBus::Broadcast(&PhysX::SystemRequests::AddColliderComponentToEntity, triggerEntity.get(), triggerConfig, boxConfig, false);
        triggerEntity->Init();
        triggerEntity->Activate();

        // Create character
        auto characterConfiguration = AZStd::make_unique<Physics::CharacterConfiguration>();
        auto characterShapeConfiguration = AZStd::make_unique<Physics::CapsuleShapeConfiguration>();
        characterShapeConfiguration->m_height = 5.0f;
        characterShapeConfiguration->m_radius = 1.0f;

        auto characterEntity = AZStd::make_unique<AZ::Entity>("CharacterEntity");
        characterEntity->CreateComponent<AzFramework::TransformComponent>()->SetWorldTM(AZ::Transform::Identity());
        characterEntity->CreateComponent<PhysXCharacters::CharacterControllerComponent>(AZStd::move(characterConfiguration), AZStd::move(characterShapeConfiguration));
        characterEntity->Init();
        characterEntity->Activate();

        // Update the world a bit to trigger Enter events
        for (int i = 0; i < 10; ++i)
        {
            GetDefaultWorld()->Update(0.1f);
        }

        // Delete the entity, and update the world to receive exit events
        characterEntity.reset();
        GetDefaultWorld()->Update(0.1f);

        EXPECT_TRUE(m_triggerEnterEvents.size() == 1);
        EXPECT_TRUE(m_triggerExitEvents.size() == 1);
    }
    TEST_F(PhysXCharactersTest, CharacterController_DisabledPhysics_DoesNotCauseError_FT)
    {
        // given a character controller 
        auto characterConfiguration = AZStd::make_unique<Physics::CharacterConfiguration>();
        auto characterShapeConfiguration = AZStd::make_unique<Physics::CapsuleShapeConfiguration>();
        characterShapeConfiguration->m_height = 5.0f;
        characterShapeConfiguration->m_radius = 1.0f;

        auto characterEntity = AZStd::make_unique<AZ::Entity>("CharacterEntity");
        characterEntity->CreateComponent<AzFramework::TransformComponent>()->SetWorldTM(AZ::Transform::Identity());
        characterEntity->CreateComponent<PhysXCharacters::CharacterControllerComponent>(AZStd::move(characterConfiguration), AZStd::move(characterShapeConfiguration));
        characterEntity->Init();
        characterEntity->Activate();

        bool physicsEnabled = false;
        Physics::WorldBodyRequestBus::EventResult(physicsEnabled, characterEntity->GetId(), &Physics::WorldBodyRequestBus::Events::IsPhysicsEnabled);
        EXPECT_TRUE(physicsEnabled);

        // when physics is disabled
        Physics::WorldBodyRequestBus::Event(characterEntity->GetId(), &Physics::WorldBodyRequestBus::Events::DisablePhysics);
        Physics::WorldBodyRequestBus::EventResult(physicsEnabled, characterEntity->GetId(), &Physics::WorldBodyRequestBus::Events::IsPhysicsEnabled);
        EXPECT_FALSE(physicsEnabled);

        // expect no error occurs when sending common events 
        AZ::Vector3 result;
        Physics::ErrorHandler errorHandler("Invalid character controller.");
        Physics::CharacterRequestBus::EventResult(result, characterEntity->GetId(), &Physics::CharacterRequestBus::Events::TryRelativeMove, AZ::Vector3::CreateZero(), 1.f);
        EXPECT_EQ(errorHandler.GetErrorCount(), 0);

        Physics::CharacterRequestBus::EventResult(result, characterEntity->GetId(), &Physics::CharacterRequestBus::Events::GetBasePosition);
        EXPECT_EQ(errorHandler.GetErrorCount(), 0);

        Physics::CharacterRequestBus::EventResult(result, characterEntity->GetId(), &Physics::CharacterRequestBus::Events::GetCenterPosition);
        EXPECT_EQ(errorHandler.GetErrorCount(), 0);

        Physics::CharacterRequestBus::EventResult(result, characterEntity->GetId(), &Physics::CharacterRequestBus::Events::GetVelocity);
        EXPECT_EQ(errorHandler.GetErrorCount(), 0);

        CharacterControllerRequestBus::Event(characterEntity->GetId(), &CharacterControllerRequestBus::Events::Resize, 2.f);
        EXPECT_EQ(errorHandler.GetErrorCount(), 0);

        float height = -1.f;
        CharacterControllerRequestBus::EventResult(height, characterEntity->GetId(), &CharacterControllerRequestBus::Events::GetHeight);
        EXPECT_EQ(errorHandler.GetErrorCount(), 0);

        AZ::TransformNotificationBus::Event(characterEntity->GetId(), &AZ::TransformNotificationBus::Events::OnTransformChanged, AZ::Transform::CreateIdentity(), AZ::Transform::CreateIdentity());
        EXPECT_EQ(errorHandler.GetErrorCount(), 0);
    }
} // namespace PhysXCharacters
