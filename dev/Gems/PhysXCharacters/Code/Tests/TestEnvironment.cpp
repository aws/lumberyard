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

#include <Components/CharacterControllerComponent.h>
#include <Components/RagdollComponent.h>
#include <PhysXCharacters/SystemBus.h>
#include <System/SystemComponent.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Physics/Utils.h>
#include <AzTest/AzTest.h>
#include <Physics/PhysicsTests.inl>
#include <PhysX/SystemComponentBus.h>

namespace PhysXCharacters
{
    AZStd::shared_ptr<Physics::World> PhysXCharactersTestEnvironment::GetDefaultWorld()
    {
        return m_defaultWorld;
    }

    void PhysXCharactersTestEnvironment::SetupEnvironment()
    {
        AZ::IO::FileIOBase::SetInstance(&m_fileIo);

        AZ::Test::GemTestEnvironment::SetupEnvironment();

        if (s_enablePvd)
        {
            bool pvdConnectionSuccessful = false;
            PhysX::SystemRequestsBus::BroadcastResult(pvdConnectionSuccessful, &PhysX::SystemRequests::ConnectToPvd);
        }

        m_defaultWorld = AZ::Interface<Physics::System>::Get()->CreateWorld(Physics::DefaultPhysicsWorldId);

        Physics::DefaultWorldBus::Handler::BusConnect();
    }

    void PhysXCharactersTestEnvironment::AddGemsAndComponents()
    {
        AddDynamicModulePaths({ "Gem.PhysX.4e08125824434932a0fe3717259caa47.v0.1.0" });
        AddComponentDescriptors({
            SystemComponent::CreateDescriptor(),
            RagdollComponent::CreateDescriptor(),
            CharacterControllerComponent::CreateDescriptor(),
            AzFramework::TransformComponent::CreateDescriptor()
            });
        AddRequiredComponents({ SystemComponent::TYPEINFO_Uuid() });
    }

    void PhysXCharactersTestEnvironment::PostCreateApplication()
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (serializeContext)
        {
            Physics::ReflectionUtils::ReflectPhysicsApi(serializeContext);
        }
    }

    void PhysXCharactersTestEnvironment::TeardownEnvironment()
    {
        Physics::DefaultWorldBus::Handler::BusDisconnect();
        m_defaultWorld = nullptr;

        // it's safe to call this even if we're not connected
        PhysX::SystemRequestsBus::Broadcast(&PhysX::SystemRequests::DisconnectFromPvd);

        AZ::Test::GemTestEnvironment::TeardownEnvironment();
    }

    void PhysXCharactersTest::SetUp()
    {
        GetDefaultWorld()->SetEventHandler(this);
    }

    void PhysXCharactersTest::TearDown()
    {
        GetDefaultWorld()->SetEventHandler(nullptr);
    }

    AZStd::shared_ptr<Physics::World> PhysXCharactersTest::GetDefaultWorld()
    {
        AZStd::shared_ptr<Physics::World> world;
        Physics::DefaultWorldBus::BroadcastResult(world, &Physics::DefaultWorldRequests::GetDefaultWorld);
        return world;
    }

    void PhysXCharactersTest::OnTriggerEnter(const Physics::TriggerEvent& triggerEvent)
    {
        m_triggerEnterEvents.push_back(triggerEvent);
    }

    void PhysXCharactersTest::OnTriggerExit(const Physics::TriggerEvent& triggerEvent)
    {
        m_triggerExitEvents.push_back(triggerEvent);
    }

    void PhysXCharactersTest::OnCollisionBegin(const Physics::CollisionEvent& collisionEvent)
    {
    }

    void PhysXCharactersTest::OnCollisionPersist(const Physics::CollisionEvent& collisionEvent)
    {
    }

    void PhysXCharactersTest::OnCollisionEnd(const Physics::CollisionEvent& collisionEvent)
    {
    }

    AZ_UNIT_TEST_HOOK(new PhysXCharactersTestEnvironment);
} // namespace PhysXCharacters
