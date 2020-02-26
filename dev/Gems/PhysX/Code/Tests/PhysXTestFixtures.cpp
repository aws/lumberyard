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
#include "PhysXTestFixtures.h"

namespace PhysX
{
    void PhysXDefaultWorldTest::SetUp() 
    {
        m_defaultWorld = AZ::Interface<Physics::System>::Get()->CreateWorld(Physics::DefaultPhysicsWorldId);
        m_defaultWorld->SetEventHandler(this);

        Physics::DefaultWorldBus::Handler::BusConnect();
    }

    void PhysXDefaultWorldTest::TearDown()
    {
        Physics::DefaultWorldBus::Handler::BusDisconnect();
        m_defaultWorld = nullptr;
    }

    AZStd::shared_ptr<Physics::World> PhysXDefaultWorldTest::GetDefaultWorld()
    {
        return m_defaultWorld;
    }

    void PhysXDefaultWorldTest::UpdateWorld(int numSteps, float deltaTime)
    {
        for (int timeStep = 0; timeStep < numSteps; timeStep++)
        {
            m_defaultWorld->Update(deltaTime);
        }
    }

    void PhysXDefaultWorldTest::OnTriggerEnter(const Physics::TriggerEvent& triggerEvent)
    {
        Physics::TriggerNotificationBus::QueueEvent(triggerEvent.m_triggerBody->GetEntityId(), &Physics::TriggerNotifications::OnTriggerEnter, triggerEvent);
    }

    void PhysXDefaultWorldTest::OnTriggerExit(const Physics::TriggerEvent& triggerEvent)
    {
        Physics::TriggerNotificationBus::QueueEvent(triggerEvent.m_triggerBody->GetEntityId(), &Physics::TriggerNotifications::OnTriggerExit, triggerEvent);
    }

    void PhysXDefaultWorldTest::OnCollisionBegin(const Physics::CollisionEvent& event)
    {
        Physics::CollisionEvent collisionEvent = event;
        Physics::CollisionNotificationBus::QueueEvent(collisionEvent.m_body1->GetEntityId(), &Physics::CollisionNotifications::OnCollisionBegin, collisionEvent);
        AZStd::swap(collisionEvent.m_body1, collisionEvent.m_body2);
        AZStd::swap(collisionEvent.m_shape1, collisionEvent.m_shape2);
        Physics::CollisionNotificationBus::QueueEvent(collisionEvent.m_body1->GetEntityId(), &Physics::CollisionNotifications::OnCollisionBegin, collisionEvent);
    }

    void PhysXDefaultWorldTest::OnCollisionPersist(const Physics::CollisionEvent& event)
    {
        Physics::CollisionEvent collisionEvent = event;
        Physics::CollisionNotificationBus::QueueEvent(collisionEvent.m_body1->GetEntityId(), &Physics::CollisionNotifications::OnCollisionPersist, collisionEvent);
        AZStd::swap(collisionEvent.m_body1, collisionEvent.m_body2);
        AZStd::swap(collisionEvent.m_shape1, collisionEvent.m_shape2);
        Physics::CollisionNotificationBus::QueueEvent(collisionEvent.m_body1->GetEntityId(), &Physics::CollisionNotifications::OnCollisionPersist, collisionEvent);
    }

    void PhysXDefaultWorldTest::OnCollisionEnd(const Physics::CollisionEvent& event)
    {
        Physics::CollisionEvent collisionEvent = event;
        Physics::CollisionNotificationBus::QueueEvent(collisionEvent.m_body1->GetEntityId(), &Physics::CollisionNotifications::OnCollisionEnd, collisionEvent);
        AZStd::swap(collisionEvent.m_body1, collisionEvent.m_body2);
        AZStd::swap(collisionEvent.m_shape1, collisionEvent.m_shape2);
        Physics::CollisionNotificationBus::QueueEvent(collisionEvent.m_body1->GetEntityId(), &Physics::CollisionNotifications::OnCollisionEnd, collisionEvent);
    }
}
