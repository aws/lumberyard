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

#include "PhysX_precompiled.h"

#include <gmock/gmock.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Math/Vector3.h>
#include <AzFramework/Physics/World.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <PhysX/SystemComponentBus.h>
#include <PhysX/ConfigurationBus.h>
#include <Tests/EditorTestUtilities.h>
#include <Tests/PhysXTestCommon.h>

#include <AzFramework/Physics/WorldBody.h>
#include <AzFramework/Physics/Collision.h>
#include <AzFramework/Physics/Casts.h>
#include <AzFramework/Physics/Material.h>

#include <functional>

#include <AzCore/std/containers/vector.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Component/EntityId.h>

namespace PhysXEditorTests
{
    class MockWorldRequestBusHandler
        : public Physics::WorldRequestBus::Handler
    {
    public:
        MockWorldRequestBusHandler()
        {
            // Connect to default physics world, because it is the one initialized in the fixture.
            Physics::WorldRequestBus::Handler::BusConnect(Physics::DefaultPhysicsWorldId);
        }

        ~MockWorldRequestBusHandler()
        {
            Physics::WorldRequestBus::Handler::BusDisconnect();
        }

        MOCK_METHOD1(Update, void(float));
        MOCK_METHOD1(RayCast, Physics::RayCastHit(const Physics::RayCastRequest&));
        MOCK_METHOD1(RayCastMultiple, AZStd::vector<Physics::RayCastHit>(const Physics::RayCastRequest&));
        MOCK_METHOD1(ShapeCast, Physics::RayCastHit(const Physics::ShapeCastRequest&));
        MOCK_METHOD1(ShapeCastMultiple, AZStd::vector<Physics::RayCastHit>(const Physics::ShapeCastRequest&));
        MOCK_METHOD2(RegisterSuppressedCollision, void(const Physics::WorldBody&, const Physics::WorldBody&));
        MOCK_METHOD2(UnregisterSuppressedCollision, void(const Physics::WorldBody&, const Physics::WorldBody&));
        MOCK_METHOD1(AddBody, void(Physics::WorldBody&));
        MOCK_METHOD1(RemoveBody, void(Physics::WorldBody&));
        MOCK_METHOD1(SetSimFunc, void(std::function<void(void*)>));
        MOCK_METHOD1(SetEventHandler, void(Physics::WorldEventHandler*));
        MOCK_CONST_METHOD0(GetGravity, AZ::Vector3());
        MOCK_METHOD1(SetGravity, void(const AZ::Vector3&));
        MOCK_METHOD1(SetMaxDeltaTime, void(float));
        MOCK_METHOD1(SetFixedDeltaTime, void(float));
        MOCK_METHOD1(DeferDelete, void(AZStd::unique_ptr<Physics::WorldBody>));
        MOCK_METHOD1(Overlap, AZStd::vector<Physics::OverlapHit>(const Physics::OverlapRequest&));
        MOCK_METHOD2(OverlapUnbounded, void(const Physics::OverlapRequest& request, const Physics::HitCallback<Physics::OverlapHit>& hitCallback));
        MOCK_METHOD1(SetTriggerEventCallback, void(Physics::ITriggerEventCallback*));
        MOCK_METHOD1(StartSimulation, void(float));
        MOCK_METHOD0(FinishSimulation, void());
        MOCK_CONST_METHOD0(GetWorldId, AZ::Crc32());
    };

    class MockSystemNotificationBusHandler
        : public Physics::SystemNotificationBus::Handler
    {
    public:
        MockSystemNotificationBusHandler()
        {
            Physics::SystemNotificationBus::Handler::BusConnect();
        }

        ~MockSystemNotificationBusHandler()
        {
            Physics::SystemNotificationBus::Handler::BusDisconnect();
        }

        MOCK_METHOD1(OnWorldCreated, void(Physics::World*));
    };

    TEST_F(PhysXEditorFixture, SetWorldConfiguration_ForwardsConfigChangesToWorldRequestBus)
    {
        testing::StrictMock<MockWorldRequestBusHandler> mockHandler;

        // Initialize new configs with some non-default values.
        const AZ::Vector3 newGravity(2.f, 5.f, 7.f);
        const float newFixedTimeStep = 0.008f;
        const float newMaxTimeStep = 0.034f;

        Physics::WorldConfiguration newConfiguration;

        newConfiguration.m_gravity = newGravity;
        newConfiguration.m_fixedTimeStep = newFixedTimeStep;
        newConfiguration.m_maxTimeStep = newMaxTimeStep;

        EXPECT_CALL(mockHandler, SetGravity(newGravity)).Times(1);
        EXPECT_CALL(mockHandler, SetMaxDeltaTime(newMaxTimeStep)).Times(1);
        EXPECT_CALL(mockHandler, SetFixedDeltaTime(newFixedTimeStep)).Times(1);

        AZ::Interface<Physics::SystemRequests>::Get()->SetDefaultWorldConfiguration(newConfiguration);
    }

    TEST_F(PhysXEditorFixture, SystemNotificationBus_CreateWorld_HandlerReceivedOnCreateWorldNotification)
    {
        testing::StrictMock<MockSystemNotificationBusHandler> mockHandler;

        EXPECT_CALL(mockHandler, OnWorldCreated).Times(1);

        const AZ::Crc32 worldId("SystemNotificationBusTestWorld");
        auto world = AZ::Interface<Physics::System>::Get()->CreateWorld(worldId);
    }
} // namespace PhysXEditorTests
