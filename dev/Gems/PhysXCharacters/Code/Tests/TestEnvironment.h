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

#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/Physics/WorldEventhandler.h>
#include <AzTest/GemTestEnvironment.h>
#include <Physics/PhysicsTests.h>
#include <PhysX/PhysXLocks.h>

namespace PhysXCharacters
{
    class PhysXCharactersTestEnvironment
        : public AZ::Test::GemTestEnvironment
        , protected Physics::DefaultWorldBus::Handler
    {
    protected:
        void SetupEnvironment() override;
        void TeardownEnvironment() override;
        void AddGemsAndComponents() override;
        void PostCreateApplication() override;

        // DefaultWorldBus
        AZStd::shared_ptr<Physics::World> GetDefaultWorld() override;

        // Flag to enable pvd in tests
        static const bool s_enablePvd = true;

        AZ::IO::LocalFileIO m_fileIo;
        AZStd::shared_ptr<Physics::World> m_defaultWorld;
    };

    class PhysXCharactersTest
        : public ::testing::Test
        , public Physics::WorldEventHandler
    {
    protected:
        void SetUp() override;
        void TearDown() override;
        AZStd::shared_ptr<Physics::World> GetDefaultWorld();

        // WorldEventHandler
        void OnTriggerEnter(const Physics::TriggerEvent& triggerEvent) override;
        void OnTriggerExit(const Physics::TriggerEvent& triggerEvent) override;
        void OnCollisionBegin(const Physics::CollisionEvent& collisionEvent) override;
        void OnCollisionPersist(const Physics::CollisionEvent& collisionEvent) override;
        void OnCollisionEnd(const Physics::CollisionEvent& collisionEvent) override;

        AZStd::vector<Physics::TriggerEvent> m_triggerEnterEvents;
        AZStd::vector<Physics::TriggerEvent> m_triggerExitEvents;
    };

    // transform for a floor centred at x = 0, y = 0, with top at level z = 0
    static const AZ::Transform DefaultFloorTransform = AZ::Transform::CreateTranslation(AZ::Vector3::CreateAxisZ(-0.5f));
} // namespace PhysXCharacters
