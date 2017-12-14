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

    TEST_F(GenericPhysicsInterfaceTest, World_CreateNewWorld_ReturnsNewWorld)
    {
        Physics::Ptr<Physics::World> world = nullptr;
        Physics::Ptr<Physics::WorldSettings> worldSettings = aznew Physics::WorldSettings();
        EBUS_EVENT_RESULT(world, Physics::SystemRequestBus, CreateWorldByName, "testWorld", worldSettings);
        EXPECT_TRUE(world != nullptr);
    }

    TEST_F(GenericPhysicsInterfaceTest, RayCast_CastAgainstNothing_ReturnsNoHits)
    {
        Physics::RayCastRequest request;
        request.m_start = AZ::Vector3(-100.0f, 0.0f, 0.0f);
        request.m_dir = AZ::Vector3(1.0f, 0.0f, 0.0f);
        request.m_time = 200.0f;

        Physics::RayCastResult result;

        // TODO: Change this to use a bus providing Raycast functionality
        Physics::Ptr<Physics::World> defaultWorld = nullptr;
        EBUS_EVENT_RESULT(defaultWorld, Physics::SystemRequestBus, GetDefaultWorld);
        defaultWorld->RayCast(request, result);

        EXPECT_TRUE(result.m_hits.size() == 0);
    }

    TEST_F(GenericPhysicsInterfaceTest, RayCast_CastAgainstSphere_ReturnsHits)
    {
        auto sphereEntity = AddTestSphere(AZ::Vector3(0.0f), 10.0f);

        Physics::RayCastRequest request;
        request.m_start = AZ::Vector3(-100.0f, 0.0f, 0.0f);
        request.m_dir = AZ::Vector3(1.0f, 0.0f, 0.0f);
        request.m_time = 200.0f;

        Physics::RayCastResult result;

        // TODO: Change this to use a bus providing Raycast functionality
        Physics::Ptr<Physics::World> defaultWorld = nullptr;
        EBUS_EVENT_RESULT(defaultWorld, Physics::SystemRequestBus, GetDefaultWorld);
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
        Physics::ShapeCastRequest request;
        request.m_start = AZ::Transform::CreateTranslation(AZ::Vector3(-20.0f, 0.0f, 0.0f));
        request.m_end = AZ::Transform::CreateTranslation(request.m_start.GetPosition() + AZ::Vector3(1.0f, 0.0f, 0.0f) * 20.0f);
        request.m_shapeConfiguration = Physics::SphereShapeConfiguration::Create(1.0f);

        Physics::ShapeCastResult result;

        // TODO: Change this to use a bus providing Shapecast functionality
        Physics::Ptr<Physics::World> defaultWorld = nullptr;
        EBUS_EVENT_RESULT(defaultWorld, Physics::SystemRequestBus, GetDefaultWorld);
        defaultWorld->ShapeCast(request, result);

        EXPECT_TRUE(result.m_hits.size() == 0);
    }

    TEST_F(GenericPhysicsInterfaceTest, ShapeCast_CastAgainstSphere_ReturnsHits)
    {
        auto sphereEntity = AddTestSphere(AZ::Vector3(0.0f), 10.0f);

        Physics::ShapeCastRequest request;
        request.m_start = AZ::Transform::CreateTranslation(AZ::Vector3(-20.0f, 0.0f, 0.0f));
        request.m_end = AZ::Transform::CreateTranslation(request.m_start.GetPosition() + AZ::Vector3(1.0f, 0.0f, 0.0f) * 20.0f);
        request.m_shapeConfiguration = Physics::SphereShapeConfiguration::Create(1.0f);

        Physics::ShapeCastResult result;

        // TODO: Change this to use a bus providing Shapecast functionality
        Physics::Ptr<Physics::World> defaultWorld = nullptr;
        EBUS_EVENT_RESULT(defaultWorld, Physics::SystemRequestBus, GetDefaultWorld);
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
} // namespace Physics
#endif // AZ_TESTS_ENABLED
