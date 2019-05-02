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

#ifdef AZ_TESTS_ENABLED

#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzFramework/Application/Application.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <AzCore/Memory/MemoryComponent.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/Jobs/JobManagerComponent.h>
#include <Physics/PhysicsTests.h>
#include <Tests/TestTypes.h>
#include <RigidBodyComponent.h>
#include <Material.h>
#include <Physics/PhysicsGenericInterfaceTests.inl>
#include <Physics/PhysicsComponentBusTests.inl>
#include <SphereColliderComponent.h>
#include <BoxColliderComponent.h>
#include <CapsuleColliderComponent.h>
#include <AzFramework/Physics/CollisionBus.h>
#include <AzFramework/IO/LocalFileIO.h>

static const char* PVD_HOST = "127.0.0.1";

namespace Physics
{
    class PhysXTestEnvironment
        : public PhysicsTestEnvironment
    {
    protected:
        void SetupEnvironment() override;
        void TeardownEnvironment() override;

        // Flag to enable pvd in tests
        static const bool s_enablePvd = true;

        AZ::ComponentApplication* m_application;
        AZ::Entity* m_systemEntity;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_transformComponentDescriptor;
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::IO::LocalFileIO> m_fileIo;
        physx::PxPvdTransport* m_pvdTransport = nullptr;
        physx::PxPvd* m_pvd = nullptr;
    };

    void PhysXTestEnvironment::SetupEnvironment()
    {
        PhysicsTestEnvironment::SetupEnvironment();

        m_fileIo = AZStd::make_unique<AZ::IO::LocalFileIO>();

        AZ::IO::FileIOBase::SetInstance(m_fileIo.get());

        char testDir[AZ_MAX_PATH_LEN];
        m_fileIo->ConvertToAbsolutePath("../Gems/PhysX/Code/Tests", testDir, AZ_MAX_PATH_LEN);
        m_fileIo->SetAlias("@test@", testDir);

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
        m_systemEntity->AddComponent(aznew AZ::JobManagerComponent());
        m_systemEntity->Init();
        m_systemEntity->Activate();

        // Set up transform component descriptor
        m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
        m_transformComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(AzFramework::TransformComponent::CreateDescriptor());
        m_transformComponentDescriptor->Reflect(&(*m_serializeContext));

        if (s_enablePvd)
        {
            // set up visual debugger
            m_pvdTransport = physx::PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
            m_pvd = PxCreatePvd(PxGetFoundation());
            m_pvd->connect(*m_pvdTransport, physx::PxPvdInstrumentationFlag::eALL);
        }
    }

    void PhysXTestEnvironment::TeardownEnvironment()
    {
        if (m_pvd)
        {
            m_pvd->disconnect();
            m_pvd->release();
        }

        if (m_pvdTransport)
        {
            m_pvdTransport->release();
        }
        m_transformComponentDescriptor.reset();
        m_serializeContext.reset();
        m_fileIo.reset();
        delete m_application;
        PhysicsTestEnvironment::TeardownEnvironment();
    }

    void GenericPhysicsInterfaceTest::SetUp()
    {
        Physics::SystemRequestBus::BroadcastResult(m_defaultWorld, 
            &Physics::SystemRequests::CreateWorld, AZ_CRC("UnitTestWorld", 0x39d5e465));

        Physics::DefaultWorldBus::Handler::BusConnect();
    }

    void GenericPhysicsInterfaceTest::TearDown()
    {
        PhysX::MaterialManagerRequestsBus::Broadcast(&PhysX::MaterialManagerRequestsBus::Events::ReleaseAllMaterials);
        Physics::DefaultWorldBus::Handler::BusDisconnect();
        m_defaultWorld = nullptr;
    }

    AZStd::shared_ptr<World> GenericPhysicsInterfaceTest::GetDefaultWorld()
    {
        return m_defaultWorld;
    }

    AZ::Entity* GenericPhysicsInterfaceTest::AddSphereEntity(const AZ::Vector3& position, const float radius, const CollisionLayer& layer)
    {
        auto entity = aznew AZ::Entity("TestSphereEntity");
        entity->CreateComponent(AZ::Uuid::CreateString("{22B10178-39B6-4C12-BB37-77DB45FDD3B6}")); // TransformComponent
        entity->Init();

        entity->Activate();

        AZ::TransformBus::Event(entity->GetId(), &AZ::TransformBus::Events::SetWorldTranslation, position);

        entity->Deactivate();

        Physics::ColliderConfiguration colliderConfig;
        colliderConfig.m_collisionLayer = layer;
        Physics::SphereShapeConfiguration shapeConfig(radius);
        entity->CreateComponent<PhysX::SphereColliderComponent>(colliderConfig, shapeConfig);
        RigidBodyConfiguration rigidBodyConfig;
        entity->CreateComponent<PhysX::RigidBodyComponent>(rigidBodyConfig);

        entity->Activate();
        return entity;
    }

    AZ::Entity* GenericPhysicsInterfaceTest::AddBoxEntity(const AZ::Vector3& position, const AZ::Vector3& dimensions, const CollisionLayer& layer)
    {
        auto entity = aznew AZ::Entity("TestBoxEntity");
        entity->CreateComponent(AZ::Uuid::CreateString("{22B10178-39B6-4C12-BB37-77DB45FDD3B6}")); // TransformComponent
        entity->Init();

        entity->Activate();

        AZ::TransformBus::Event(entity->GetId(), &AZ::TransformBus::Events::SetWorldTranslation, position);

        entity->Deactivate();

        Physics::ColliderConfiguration colliderConfig;
        colliderConfig.m_collisionLayer = layer;
        Physics::BoxShapeConfiguration shapeConfig(dimensions);
        entity->CreateComponent<PhysX::BoxColliderComponent>(colliderConfig, shapeConfig);
        RigidBodyConfiguration rigidBodyConfig;
        entity->CreateComponent<PhysX::RigidBodyComponent>(rigidBodyConfig);

        entity->Activate();
        return entity;
    }

    AZ::Entity* GenericPhysicsInterfaceTest::AddStaticBoxEntity(const AZ::Vector3& position, const AZ::Vector3& dimensions, const CollisionLayer& layer)
    {
        auto entity = aznew AZ::Entity("TestStaticBoxEntity");
        entity->CreateComponent(AZ::Uuid::CreateString("{22B10178-39B6-4C12-BB37-77DB45FDD3B6}")); // TransformComponent
        entity->Init();

        entity->Activate();

        AZ::TransformBus::Event(entity->GetId(), &AZ::TransformBus::Events::SetWorldTranslation, position);

        entity->Deactivate();

        Physics::ColliderConfiguration colliderConfig; 
        colliderConfig.m_collisionLayer = layer;
        Physics::BoxShapeConfiguration shapeConfig(dimensions);
        entity->CreateComponent<PhysX::BoxColliderComponent>(colliderConfig, shapeConfig);

        entity->Activate();
        return entity;
    }

    AZ::Entity* GenericPhysicsInterfaceTest::AddCapsuleEntity(const AZ::Vector3& position, const float height,
        const float radius, const CollisionLayer& layer)
    {
        auto entity = aznew AZ::Entity("TestCapsuleEntity");
        entity->CreateComponent(AZ::Uuid::CreateString("{22B10178-39B6-4C12-BB37-77DB45FDD3B6}")); // TransformComponent
        entity->Init();

        entity->Activate();

        AZ::TransformBus::Event(entity->GetId(), &AZ::TransformBus::Events::SetWorldTranslation, position);

        entity->Deactivate();

        Physics::ColliderConfiguration colliderConfig;
        colliderConfig.m_collisionLayer = layer;
        Physics::CapsuleShapeConfiguration shapeConfig(height, radius);
        entity->CreateComponent<PhysX::CapsuleColliderComponent>(colliderConfig, shapeConfig);
        RigidBodyConfiguration rigidBodyConfig;
        entity->CreateComponent<PhysX::RigidBodyComponent>(rigidBodyConfig);

        entity->Activate();
        return entity;
    }

    AZ_UNIT_TEST_HOOK(new PhysXTestEnvironment);
} // namespace Physics
#endif // AZ_TESTS_ENABLED
