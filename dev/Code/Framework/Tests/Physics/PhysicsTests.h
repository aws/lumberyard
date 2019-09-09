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

#pragma once

#ifdef AZ_TESTS_ENABLED

#include <AzTest/AzTest.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Math/Vector3.h>
#include <AzFramework/Physics/RigidBody.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/World.h>
#include <AzCore/Debug/TraceMessageBus.h>

namespace Physics
{
    class PhysicsTestEnvironment
        : public AZ::Test::ITestEnvironment
    {
    protected:
        void SetupEnvironment() override;
        void TeardownEnvironment() override;
    };

    /**
     * Handler for the trace message bus to suppress errors / warnings that are expected due to testing particular
     * code branches, so as to avoid filling the test output with traces.
     */
    class ErrorHandler
        : public AZ::Debug::TraceMessageBus::Handler
    {
    public:
        ErrorHandler(const char* errorPattern);
        ~ErrorHandler();
        int GetErrorCount();
        bool SuppressExpectedErrors(const char* window, const char* message);

        // AZ::Debug::TraceMessageBus
        bool OnPreError(const char* window, const char* fileName, int line, const char* func, const char* message);
        bool OnPreWarning(const char* window, const char* fileName, int line, const char* func, const char* message);
    private:
        AZStd::string m_errorPattern;
        int m_errorCount;
    };

    /**
     * Class to contain tests which any implementation of the AzFramework::Physics API should pass
     * Each gem which implements the common physics API can run the generic API tests by:
     * - including this header file and the appropriate .inl files.
     * - deriving from PhysicsTestEnvironment and extending the environment functions to set up the gem system component etc.
     * - implementing the helper functions required for the tests using gem specific components etc.
     * - adding a AZ_UNIT_TEST_HOOK with the derived environment class
     */
    class GenericPhysicsInterfaceTest
        : public ::testing::Test
        , protected Physics::DefaultWorldBus::Handler
    {
    public:
        // Helper functions for setting up test worlds using API only
        // These can be implemented here as they should not require any gem specific functions
        AZStd::shared_ptr<Physics::World> CreateTestWorld();

        void SetUp() override;
        void TearDown() override;

        // Helper functions for setting up entities used in tests
        // These need to be implemented in the gem as they may require gem specific components etc.
        AZ::Entity* AddSphereEntity(const AZ::Vector3& position, const float radius, const CollisionLayer& layer = CollisionLayer::Default);
        AZ::Entity* AddBoxEntity(const AZ::Vector3& position, const AZ::Vector3& dimensions, const CollisionLayer& layer = CollisionLayer::Default);
        AZ::Entity* AddStaticBoxEntity(const AZ::Vector3& position, const AZ::Vector3& dimensions, const CollisionLayer& layer = CollisionLayer::Default);
        AZ::Entity* AddCapsuleEntity(const AZ::Vector3& position, const float height, const float radius, const CollisionLayer& layer = CollisionLayer::Default);

    protected:
        // DefaultWorldBus
        AZStd::shared_ptr<World> GetDefaultWorld();

        AZStd::shared_ptr<Physics::World> m_defaultWorld;
    };

    class PhysicsComponentBusTest
        : public GenericPhysicsInterfaceTest
    {
    };

    // helper functions
    AZStd::shared_ptr<RigidBodyStatic> AddStaticFloorToWorld(World* world, const AZ::Transform& transform = AZ::Transform::CreateIdentity());
    AZStd::shared_ptr<RigidBodyStatic> AddStaticUnitBoxToWorld(World* world, const AZ::Vector3& position);

    AZStd::shared_ptr<RigidBody> AddUnitBoxToWorld(World* world, const AZ::Vector3& position);
    AZStd::shared_ptr<RigidBody> AddSphereToWorld(World* world, const AZ::Vector3& position);
    AZStd::shared_ptr<RigidBody> AddCapsuleToWorld(World* world, const AZ::Vector3& position);

    void UpdateWorld(World* world, float timeStep, AZ::u32 numSteps);
    void UpdateDefaultWorld(float timeStep, AZ::u32 numSteps);
    float GetPositionElement(AZ::Entity* entity, int element);
} // namespace Physics

#endif // AZ_TESTS_ENABLED
