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

#include <API/CharacterController.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Jobs/JobManagerComponent.h>
#include <AzCore/Memory/MemoryComponent.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <AzTest/AzTest.h>
#include <AzFramework/Application/Application.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/World.h>
#include <Physics/PhysicsTests.h>
#include <Physics/PhysicsTests.inl>
#include <Tests/TestTypes.h>
#include <PhysXCharacters/SystemBus.h>

#ifdef AZ_TESTS_ENABLED

static const char* PVD_HOST = "127.0.0.1";

namespace PhysXCharacters
{
    class PhysXCharactersTestEnvironment
        : public Physics::PhysicsTestEnvironment
        , protected Physics::DefaultWorldBus::Handler
    {
    protected:
        void SetupEnvironment() override;
        void TeardownEnvironment() override;

        // DefaultWorldBus
        AZStd::shared_ptr<Physics::World> GetDefaultWorld() override
        {
            return m_defaultWorld;
        }

        // Flag to enable pvd in tests
        static const bool s_enablePvd = true;

        AZ::ComponentApplication* m_application;
        AZ::Entity* m_systemEntity;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_transformComponentDescriptor;
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        physx::PxPvdTransport* m_pvdTransport = nullptr;
        physx::PxPvd* m_pvd = nullptr;
        AZ::IO::LocalFileIO m_fileIo;
        AZStd::shared_ptr<Physics::World> m_defaultWorld;
    };

    void PhysXCharactersTestEnvironment::SetupEnvironment()
    {
        Physics::PhysicsTestEnvironment::SetupEnvironment();

        AZ::IO::FileIOBase::SetInstance(&m_fileIo);

        // Create application and descriptor
        m_application = aznew AZ::ComponentApplication;
        AZ::ComponentApplication::Descriptor appDesc;
        appDesc.m_useExistingAllocator = true;

        // Set up gems for loading
        AZ::DynamicModuleDescriptor dynamicModuleDescriptor;
        dynamicModuleDescriptor.m_dynamicLibraryPath = "Gem.PhysX.4e08125824434932a0fe3717259caa47.v0.1.0";
        appDesc.m_modules.push_back(dynamicModuleDescriptor);

        dynamicModuleDescriptor = AZ::DynamicModuleDescriptor();
        dynamicModuleDescriptor.m_dynamicLibraryPath = "Gem.PhysXCharacters.50f9ae1e09ac471bbd9d86ca8063ddf9.v0.1.0";
        appDesc.m_modules.push_back(dynamicModuleDescriptor);

        // Create system entity
        AZ::ComponentApplication::StartupParameters startupParams;
        m_systemEntity = m_application->Create(appDesc, startupParams);
        AZ_TEST_ASSERT(m_systemEntity);
        m_systemEntity->AddComponent(aznew AZ::MemoryComponent());
        m_systemEntity->AddComponent(aznew AZ::AssetManagerComponent());
        m_systemEntity->AddComponent(aznew AZ::JobManagerComponent());
        m_systemEntity->Init();
        m_systemEntity->Activate();

        // Set up transform component descriptor
        m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
        m_transformComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(AzFramework::TransformComponent::CreateDescriptor());
        m_transformComponentDescriptor->Reflect(&(*m_serializeContext));

        if (s_enablePvd)
        {
            // set up visual debugger
            m_pvdTransport = physx::PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
            m_pvd = PxCreatePvd(PxGetFoundation());
            m_pvd->connect(*m_pvdTransport, physx::PxPvdInstrumentationFlag::eALL);
        }

        Physics::SystemRequestBus::BroadcastResult(m_defaultWorld,
            &Physics::SystemRequests::CreateWorld, AZ_CRC("UnitTestWorld", 0x39d5e465));

        Physics::DefaultWorldBus::Handler::BusConnect();

    }

    void PhysXCharactersTestEnvironment::TeardownEnvironment()
    {
        Physics::DefaultWorldBus::Handler::BusDisconnect();
        m_defaultWorld = nullptr;

        if (m_pvd)
        {
            m_pvd->disconnect();
            m_pvd->release();
        }

        if (m_pvdTransport)
        {
            m_pvdTransport->release();
        }
        m_transformComponentDescriptor.release();
        m_serializeContext.release();
        delete m_application;
        Physics::PhysicsTestEnvironment::TeardownEnvironment();
    }

    class PhysXCharactersTest
        : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
        }

        void TearDown() override
        {
        }
    };

    // transform for a floor centred at x = 0, y = 0, with top at level z = 0
    static const AZ::Transform defaultFloorTransform = AZ::Transform::CreateTranslation(AZ::Vector3::CreateAxisZ(-0.5f));

    class ControllerTestBasis
    {
    public:
        ControllerTestBasis(const AZ::Transform& floorTransform = defaultFloorTransform)
        {
            SetUp(floorTransform);
        }

        void SetUp(const AZ::Transform& floorTransform = defaultFloorTransform)
        {
            Physics::DefaultWorldBus::BroadcastResult(m_world, &Physics::DefaultWorldRequests::GetDefaultWorld);

            m_floor = Physics::AddStaticFloorToWorld(m_world.get(), floorTransform);

            CharacterControllerConfiguration characterConfig;
            characterConfig.m_maximumSlopeAngle = 25.0f;
            characterConfig.m_stepHeight = 0.2f;
            Physics::CapsuleShapeConfiguration shapeConfig;

            Physics::CharacterSystemRequestBus::BroadcastResult(m_controller,
                &Physics::CharacterSystemRequests::CreateCharacter, characterConfig, shapeConfig, *m_world);

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
        ControllerTestBasis basis(slopedFloorTransform);

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

    AZ_UNIT_TEST_HOOK(new PhysXCharactersTestEnvironment);
} // namespace PhysXCharacters
#endif // AZ_TESTS_ENABLED
