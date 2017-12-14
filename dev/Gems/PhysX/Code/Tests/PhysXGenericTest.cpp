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

#include <StdAfx.h>

#ifdef AZ_TESTS_ENABLED

#include <LmbrCentral/Shape/SphereShapeComponentBus.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzFramework/Application/Application.h>
#include <AzFramework/Physics/SystemComponent.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <AzCore/Memory/MemoryComponent.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <Physics/PhysicsTests.h>
#include <Physics/PhysicsTests.inl>
#include <Tests/TestTypes.h>

namespace Physics
{
    class PhysXTestEnvironment
        : public PhysicsTestEnvironment
    {
    protected:
        void SetupEnvironment() override;
        void TeardownEnvironment() override;

        AZ::ComponentApplication* m_application;
        AZ::Entity* m_systemEntity;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_transformComponentDescriptor;
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
    };

    void PhysXTestEnvironment::SetupEnvironment()
    {
        PhysicsTestEnvironment::SetupEnvironment();

        // Create application and descriptor
        m_application = aznew AZ::ComponentApplication;
        AZ::ComponentApplication::Descriptor appDesc;
        appDesc.m_useExistingAllocator = true;

        // Set up gems for loading
        AZ::DynamicModuleDescriptor dynamicModuleDescriptor;
        dynamicModuleDescriptor.m_dynamicLibraryPath = "Gem.PhysX.4e08125824434932a0fe3717259caa47.v0.1.0";
        appDesc.m_modules.push_back(dynamicModuleDescriptor);

        dynamicModuleDescriptor = AZ::DynamicModuleDescriptor();
        dynamicModuleDescriptor.m_dynamicLibraryPath = "Gem.LmbrCentral.ff06785f7145416b9d46fde39098cb0c.v0.1.0";
        appDesc.m_modules.push_back(dynamicModuleDescriptor);

        // Create system entity
        AZ::ComponentApplication::StartupParameters startupParams;
        m_systemEntity = m_application->Create(appDesc, startupParams);
        AZ_TEST_ASSERT(m_systemEntity);
        m_systemEntity->AddComponent(aznew AZ::MemoryComponent());
        m_systemEntity->AddComponent(aznew AZ::AssetManagerComponent());
        m_systemEntity->Init();
        m_systemEntity->Activate();

        // Set up transform component descriptor
        m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
        m_transformComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(AzFramework::TransformComponent::CreateDescriptor());
        m_transformComponentDescriptor->Reflect(&(*m_serializeContext));
    }

    void PhysXTestEnvironment::TeardownEnvironment()
    {
        m_transformComponentDescriptor.release();
        m_serializeContext.release();
        delete m_application;
        PhysicsTestEnvironment::TeardownEnvironment();
    }

    AZ::Entity* GenericPhysicsInterfaceTest::AddTestSphere(const AZ::Vector3& position, float radius)
    {
        auto entity = aznew AZ::Entity("TestSphereEntity");
        auto transformComponent = static_cast<AzFramework::TransformComponent*>(entity->CreateComponent(AZ::Uuid::CreateString("{22B10178-39B6-4C12-BB37-77DB45FDD3B6}")));

        entity->CreateComponent(AZ::Uuid::CreateString("{E24CBFF0-2531-4F8D-A8AB-47AF4D54BCD2}")); // SphereShapeComponent
        entity->Init();

        // TODO: Remove this Activate/Deactivate magic when handling the shape change is implemented in the collider component
        entity->Activate();

        AZ::TransformBus::Event(entity->GetId(), &AZ::TransformBus::Events::SetWorldTranslation, position);

        EBUS_EVENT_ID(entity->GetId(), LmbrCentral::SphereShapeComponentRequestsBus, SetRadius, radius);

        entity->Deactivate();

        entity->CreateComponent(AZ::Uuid::CreateString("{C53C7C88-7131-4EEB-A602-A7DF5B47898E}")); // PhysXColliderComponent
        entity->CreateComponent(AZ::Uuid::CreateString("{D4E52A70-BDE1-4819-BD3C-93AB3F4F3BE3}")); // PhysXRigidBodyComponent

        entity->Activate();
        return entity;
    }

    AZ_UNIT_TEST_HOOK(new PhysXTestEnvironment);
} // namespace Physics
#endif // AZ_TESTS_ENABLED
