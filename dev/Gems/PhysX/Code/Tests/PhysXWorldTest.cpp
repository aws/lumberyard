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
#include <AzTest/AzTest.h>
#include <AzFramework/Physics/World.h>
#include <AzFramework/Physics/SystemBus.h>

namespace PhysX
{
    class WorldEventListener
        : public Physics::WorldNotificationBus::Handler

    {
    public:

        WorldEventListener(const char* worldId = "")
            : WorldEventListener(worldId, WorldNotifications::Default)
        {
        }

        WorldEventListener(const  char* worldId, int priority)
            : m_updateOrder(priority)
        {
            Physics::WorldNotificationBus::Handler::BusConnect(AZ::Crc32(worldId));
        }

        ~WorldEventListener()
        {
            Physics::WorldNotificationBus::Handler::BusDisconnect();
        }

        void OnPrePhysicsUpdate(float fixedDeltaTime) override
        {
            if(m_onPreUpdate)
                m_onPreUpdate(fixedDeltaTime);
            m_preUpdates.push_back(fixedDeltaTime);
        }

        void OnPostPhysicsUpdate(float fixedDeltaTime) override
        {
            if (m_onPostUpdate)
                m_onPostUpdate(fixedDeltaTime); 
            m_postUpdates.push_back(fixedDeltaTime);
        }

        int GetPhysicsTickOrder() override
        {
            return m_updateOrder;
        }

        AZStd::vector<float> m_preUpdates;
        AZStd::vector<float> m_postUpdates;
        AZStd::function<void(float)> m_onPostUpdate;
        AZStd::function<void(float)> m_onPreUpdate;
        int m_updateOrder = WorldNotifications::Default;
    };

    class PhysXWorldTest
        : public::testing::Test
    {
    protected:
        using WorldPtr = AZStd::shared_ptr<Physics::World>;

        AZStd::tuple<WorldPtr, WorldEventListener> CreateWorld(const char* worldId)
        {
            WorldPtr world;
            Physics::SystemRequestBus::BroadcastResult(world, &Physics::SystemRequests::CreateWorld, AZ::Crc32(worldId));
            return AZStd::make_tuple(world, WorldEventListener(worldId));
        }

    };

    TEST_F(PhysXWorldTest, OnPostUpdateTriggeredPerWorld)
    {
        // Given, we have two worlds
        WorldPtr world1, world2;
        WorldEventListener listener1, listener2;
        std::tie(world1, listener1) = CreateWorld("World1");
        std::tie(world2, listener2) = CreateWorld("World2");

        // When, we update each world
        float deltaTime = Physics::WorldConfiguration::s_defaultFixedTimeStep;
        world1->Update(deltaTime);
        world2->Update(deltaTime * 2);

        // Then, we should receive correct amount of updates per world
        EXPECT_EQ(1, listener1.m_postUpdates.size());
        EXPECT_EQ(2, listener2.m_postUpdates.size());
    }

    TEST_F(PhysXWorldTest, OnPreUpdateTriggeredPerWorld)
    {
        // Given, we have two worlds
        WorldPtr world1, world2;
        WorldEventListener listener1, listener2;
        std::tie(world1, listener1) = CreateWorld("World1");
        std::tie(world2, listener2) = CreateWorld("World2");

        // When, we update each world
        float deltaTime = Physics::WorldConfiguration::s_defaultFixedTimeStep;
        world1->Update(deltaTime);
        world2->Update(deltaTime * 2);

        // Then, we should receive correct amount of updates per world
        EXPECT_EQ(1, listener1.m_preUpdates.size());
        EXPECT_EQ(2, listener2.m_preUpdates.size());
    }

    TEST_F(PhysXWorldTest, WorldNotificationBusOrdered)
    {
        using namespace Physics;

        // GIVEN there is a world with multiple listeners
        const char* worldId = "World1";
        WorldPtr world;
        SystemRequestBus::BroadcastResult(world, &Physics::SystemRequests::CreateWorld, AZ::Crc32(worldId));

        // Connect the busses in randomish order.
        WorldEventListener listener1(worldId, WorldNotifications::Physics);
        WorldEventListener listener5(worldId, WorldNotifications::Default);
        WorldEventListener listener3(worldId, WorldNotifications::Components);
        WorldEventListener listener4(worldId, WorldNotifications::Scripting);
        WorldEventListener listener2(worldId, WorldNotifications::Animation);
        
        AZStd::vector<int> updateEvents;
        listener1.m_onPostUpdate = [&](float){
            updateEvents.push_back(listener1.m_updateOrder);
        };

        listener2.m_onPostUpdate = [&](float) {
            updateEvents.push_back(listener2.m_updateOrder);
        };

        listener3.m_onPostUpdate = [&](float) {
            updateEvents.push_back(listener3.m_updateOrder);
        };

        listener4.m_onPostUpdate = [&](float) {
            updateEvents.push_back(listener4.m_updateOrder);
        };

        listener5.m_onPostUpdate = [&](float) {
            updateEvents.push_back(listener5.m_updateOrder);
        };

        // WHEN the world is ticked.
        world->Update(0.017f);

        // THEN all the listeners were updated in the correct order.
        EXPECT_EQ(5, updateEvents.size());
        EXPECT_TRUE(AZStd::is_sorted(updateEvents.begin(), updateEvents.end()));
    }
}