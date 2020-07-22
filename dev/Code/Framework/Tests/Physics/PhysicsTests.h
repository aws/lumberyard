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

#include <AzTest/AzTest.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Math/Vector3.h>
#include <AzFramework/Physics/RigidBody.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/World.h>
#include <AzCore/Debug/TraceMessageBus.h>

namespace Physics
{
    /// Handler for the trace message bus to suppress errors / warnings that are expected due to testing particular
    /// code branches, so as to avoid filling the test output with traces.
    class ErrorHandler
        : public AZ::Debug::TraceMessageBus::Handler
    {
    public:
        explicit ErrorHandler(const char* errorPattern);
        ~ErrorHandler();
        int GetErrorCount() const;
        int GetWarningCount() const;
        bool SuppressExpectedErrors(const char* window, const char* message);

        // AZ::Debug::TraceMessageBus
        bool OnPreError(const char* window, const char* fileName, int line, const char* func, const char* message) override;
        bool OnPreWarning(const char* window, const char* fileName, int line, const char* func, const char* message) override;
        bool OnPrintf(const char* window, const char* message) override;
    private:
        AZStd::string m_errorPattern;
        int m_errorCount;
        int m_warningCount;
    };

    class GenericPhysicsFixture
        : protected Physics::DefaultWorldBus::Handler
    {
    public:
        // Helper functions for setting up test worlds using API only
        // These can be implemented here as they should not require any gem specific functions
        AZStd::shared_ptr<Physics::World> CreateTestWorld();

        void SetUpInternal();
        void TearDownInternal();

        // Helper functions for setting up entities used in tests
        // These need to be implemented in the gem as they may require gem specific components etc.
        AZ::Entity* AddSphereEntity(const AZ::Vector3& position, const float radius, const CollisionLayer& layer = CollisionLayer::Default);
        AZ::Entity* AddBoxEntity(const AZ::Vector3& position, const AZ::Vector3& dimensions, const CollisionLayer& layer = CollisionLayer::Default);
        AZ::Entity* AddCapsuleEntity(const AZ::Vector3& position, const float height, const float radius, const CollisionLayer& layer = CollisionLayer::Default);

        AZ::Entity* AddStaticSphereEntity(const AZ::Vector3& position, const float radius, const CollisionLayer& layer = CollisionLayer::Default);
        AZ::Entity* AddStaticBoxEntity(const AZ::Vector3& position, const AZ::Vector3& dimensions, const CollisionLayer& layer = CollisionLayer::Default);
        AZ::Entity* AddStaticCapsuleEntity(const AZ::Vector3& position, const float height, const float radius, const CollisionLayer& layer = CollisionLayer::Default);

        // Helper function for creating multishape entity
        struct MultiShapeConfig
        {
            AZ::Vector3 m_position; // Position of entity
            AZ::Vector3 m_rotation = AZ::Vector3(0.f, 0.f, 0.f); // Euler rotation of entity in radians
            CollisionLayer m_layer = CollisionLayer::Default; // Collision layer

            struct ShapeList
            {
                struct ShapeData
                {
                    struct Box
                    {
                        AZ::Vector3 m_extent;
                    };
                    struct Sphere
                    {
                        float m_radius;
                    };
                    struct Capsule
                    {
                        float m_height;
                        float m_radius;
                    };

                    AZ::Vector3 m_offset;
                    AZStd::variant<AZStd::monostate, Box, Sphere, Capsule> m_data;
                };

                void AddBox(AZ::Vector3 extent, AZ::Vector3 offset)
                {
                    ShapeData box;
                    ShapeData::Box boxData { extent };
                    box.m_data = boxData;
                    box.m_offset = offset;
                    m_shapesData.push_back(box);
                }
                void AddSphere(float radius, AZ::Vector3 offset)
                {
                    ShapeData sphere;
                    ShapeData::Sphere sphereData{ radius };
                    sphere.m_data = sphereData;
                    sphere.m_offset = offset;
                    m_shapesData.push_back(sphere);
                }
                void AddCapsule(float height, float radius, AZ::Vector3 offset)
                {
                    ShapeData capsule;
                    ShapeData::Capsule capsuleData{ height, radius };
                    capsule.m_data = capsuleData;
                    capsule.m_offset = offset;
                    m_shapesData.push_back(capsule);
                }

                AZStd::vector<ShapeData> m_shapesData;
            };
            ShapeList m_shapes;
        };
        AZStd::unique_ptr<AZ::Entity> AddMultiShapeEntity(const MultiShapeConfig& config);

    protected:
        // DefaultWorldBus
        AZStd::shared_ptr<World> GetDefaultWorld();

        AZStd::shared_ptr<Physics::World> m_defaultWorld;
    };

    /// Class to contain tests which any implementation of the AzFramework::Physics API should pass
    /// Each gem which implements the common physics API can run the generic API tests by:
    /// - including this header file and the appropriate .inl files.
    /// - deriving from AZ::Test::ITestEnvironment and extending the environment functions to set up the gem system component etc.
    /// - implementing the helper functions required for the tests using gem specific components etc.
    /// - adding a AZ_UNIT_TEST_HOOK with the derived environment class
    class GenericPhysicsInterfaceTest
        : protected GenericPhysicsFixture
        , public testing::Test
    {
    public:
        void SetUp() override
        {
            SetUpInternal();
        }
        void TearDown() override
        {
            TearDownInternal();
        }
    };



    class PhysicsComponentBusTest
        : public GenericPhysicsInterfaceTest
    {
        void SetUp() override
        {
            GenericPhysicsInterfaceTest::SetUp();
        }

        void TearDown() override
        {
            GenericPhysicsInterfaceTest::TearDown();
        }
    };

    // helper functions
    AZStd::shared_ptr<RigidBodyStatic> AddStaticFloorToWorld(World* world, const AZ::Transform& transform = AZ::Transform::CreateIdentity());
    AZStd::shared_ptr<RigidBodyStatic> AddStaticUnitBoxToWorld(World* world, const AZ::Vector3& position);

    AZStd::shared_ptr<RigidBody> AddUnitBoxToWorld(World* world, const AZ::Vector3& position);
    AZStd::shared_ptr<RigidBody> AddSphereToWorld(World* world, const AZ::Vector3& position);
    AZStd::shared_ptr<RigidBody> AddCapsuleToWorld(World* world, const AZ::Vector3& position);

    void UpdateWorld(World* world, float timeStep, AZ::u32 numSteps);
    void UpdateWorldSplitSim(World* world, float timeStep, AZ::u32 numSteps);
    void UpdateDefaultWorld(AZ::u32 numSteps, float timeStep = WorldConfiguration::s_defaultFixedTimeStep);
    float GetPositionElement(AZ::Entity* entity, int element);
} // namespace Physics
