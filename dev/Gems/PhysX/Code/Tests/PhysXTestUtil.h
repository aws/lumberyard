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

#include <AzFramework/Physics/CollisionNotificationBus.h>
#include <AzFramework/Physics/TriggerBus.h>

namespace PhysX
{
    //! CollisionCallbacksListener listens to collision events for a particular entity id.
    class CollisionCallbacksListener
        : public Physics::CollisionNotificationBus::Handler
    {
    public:
        CollisionCallbacksListener(AZ::EntityId entityId);
        ~CollisionCallbacksListener();

        void OnCollisionBegin(const Physics::CollisionEvent& collision) override;
        void OnCollisionPersist(const Physics::CollisionEvent& collision) override;
        void OnCollisionEnd(const Physics::CollisionEvent& collision) override;

        AZStd::vector<Physics::CollisionEvent> m_beginCollisions;
        AZStd::vector<Physics::CollisionEvent> m_persistCollisions;
        AZStd::vector<Physics::CollisionEvent> m_endCollisions;

        AZStd::function<void(const Physics::CollisionEvent& collisionEvent)> m_onCollisionBegin;
        AZStd::function<void(const Physics::CollisionEvent& collisionEvent)> m_onCollisionPersist;
        AZStd::function<void(const Physics::CollisionEvent& collisionEvent)> m_onCollisionEnd;
    };

    //! TestTriggerAreaNotificationListener listens to trigger events for a particular entity id.
    class TestTriggerAreaNotificationListener
        : protected Physics::TriggerNotificationBus::Handler
    {
    public:
        TestTriggerAreaNotificationListener(AZ::EntityId triggerAreaEntityId);
        ~TestTriggerAreaNotificationListener();

        void OnTriggerEnter(const Physics::TriggerEvent& event) override;
        void OnTriggerExit(const Physics::TriggerEvent& event) override;

        const AZStd::vector<Physics::TriggerEvent>& GetEnteredEvents() const { return m_enteredEvents; }
        const AZStd::vector<Physics::TriggerEvent>& GetExitedEvents() const { return m_exitedEvents; }

        AZStd::function<void(const Physics::TriggerEvent& event)> m_onTriggerEnter;
        AZStd::function<void(const Physics::TriggerEvent& event)> m_onTriggerExit;

    private:
        AZStd::vector<Physics::TriggerEvent> m_enteredEvents;
        AZStd::vector<Physics::TriggerEvent> m_exitedEvents;
    };
}