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
#include <AzCore/UnitTest/TestTypes.h>
#include <RigidBodyComponent.h>
#include <Material.h>
#include <Physics/PhysicsGenericInterfaceTests.inl>
#include <Physics/PhysicsComponentBusTests.inl>
#include <SphereColliderComponent.h>
#include <BoxColliderComponent.h>
#include <CapsuleColliderComponent.h>
#include <SystemComponent.h>
#include <TerrainComponent.h>
#include <ComponentDescriptors.h>
#include <AzFramework/Physics/CollisionBus.h>
#include <AzFramework/IO/LocalFileIO.h>

namespace PhysX
{
    // We can't load the PhysX gem the same way we do LmbrCentral, because that would lead to the AZ::Environment
    // being create twice.  This is used to initialize the PhysX system component and create the descriptors for all
    // the PhysX components.
    class PhysXApplication
        : public AZ::ComponentApplication
    {
    public:
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            AZ::ComponentTypeList components = AZ::ComponentApplication::GetRequiredSystemComponents();
            components.insert(components.end(),
                {
                    azrtti_typeid<AZ::MemoryComponent>(),
                    azrtti_typeid<AZ::AssetManagerComponent>(),
                    azrtti_typeid<AZ::JobManagerComponent>(),
                    azrtti_typeid<PhysX::SystemComponent>()
                });

            return components;
        }

        void CreateReflectionManager() override
        {
            AZ::ComponentApplication::CreateReflectionManager();
            PhysX::SystemComponent::InitializePhysXSDK();
            for (AZ::ComponentDescriptor* descriptor : GetDescriptors())
            {
                RegisterComponentDescriptor(descriptor);
            }
        }

        void Destroy() override
        {
            AZ::ComponentApplication::Destroy();
            PhysX::SystemComponent::DestroyPhysXSDK();
        }
    };

    class TestEnvironment
        : public AZ::Test::ITestEnvironment
    {
    protected:
        void SetupEnvironment() override;
        void TeardownEnvironment() override;

        // Flag to enable pvd in tests
        static const bool s_enablePvd = true;

        PhysXApplication* m_application;
        AZ::Entity* m_systemEntity;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_transformComponentDescriptor;
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::IO::LocalFileIO> m_fileIo;
    };

    void TestEnvironment::SetupEnvironment()
    {
        AZ::AllocatorInstance<AZ::SystemAllocator>::Create();

        m_fileIo = AZStd::make_unique<AZ::IO::LocalFileIO>();

        AZ::IO::FileIOBase::SetInstance(m_fileIo.get());

        char testDir[AZ_MAX_PATH_LEN];
        m_fileIo->ConvertToAbsolutePath("../Gems/PhysX/Code/Tests", testDir, AZ_MAX_PATH_LEN);
        m_fileIo->SetAlias("@test@", testDir);

        // Create application and descriptor
        m_application = aznew PhysXApplication;
        AZ::ComponentApplication::Descriptor appDesc;
        appDesc.m_useExistingAllocator = true;

        // Set up gems other than PhysX for loading
        AZ::DynamicModuleDescriptor dynamicModuleDescriptor;
        dynamicModuleDescriptor.m_dynamicLibraryPath = "Gem.LmbrCentral.ff06785f7145416b9d46fde39098cb0c.v0.1.0";
        appDesc.m_modules.push_back(dynamicModuleDescriptor);

        // Create system entity
        AZ::ComponentApplication::StartupParameters startupParams;
        m_systemEntity = m_application->Create(appDesc, startupParams);
        AZ_TEST_ASSERT(m_systemEntity);
        m_systemEntity->Init();
        m_systemEntity->Activate();

        // Set up transform component descriptor
        m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
        m_transformComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(AzFramework::TransformComponent::CreateDescriptor());
        m_transformComponentDescriptor->Reflect(&(*m_serializeContext));

        if (s_enablePvd)
        {
            bool pvdConnectionSuccessful;
            PhysX::SystemRequestsBus::BroadcastResult(pvdConnectionSuccessful, &PhysX::SystemRequests::ConnectToPvd);
        }
    }

    void TestEnvironment::TeardownEnvironment()
    {
        if (s_enablePvd)
        {
            PhysX::SystemRequestsBus::Broadcast(&PhysX::SystemRequests::DisconnectFromPvd);
        }

        m_transformComponentDescriptor.reset();
        m_serializeContext.reset();
        m_fileIo.reset();
        m_application->Destroy();
        delete m_application;

        AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
    }

    AZ_UNIT_TEST_HOOK(new TestEnvironment);
} // namespace PhysX

namespace Physics
{
    void GenericPhysicsInterfaceTest::SetUp()
    {
        Physics::SystemRequestBus::BroadcastResult(m_defaultWorld,
            &Physics::SystemRequests::CreateWorld, Physics::DefaultPhysicsWorldId);

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

    AZ::Entity* GenericPhysicsInterfaceTest::AddSphereEntity(const AZ::Vector3& position, const float radius,
        const CollisionLayer& layer)
    {
        auto entity = aznew AZ::Entity("TestSphereEntity");
        entity->CreateComponent(AZ::Uuid::CreateString("{22B10178-39B6-4C12-BB37-77DB45FDD3B6}")); // TransformComponent
        entity->Init();

        entity->Activate();

        AZ::TransformBus::Event(entity->GetId(), &AZ::TransformBus::Events::SetWorldTranslation, position);

        entity->Deactivate();

        auto colliderConfig = AZStd::make_shared<Physics::ColliderConfiguration>();
        colliderConfig->m_collisionLayer = layer;
        auto shapeConfig = AZStd::make_shared<Physics::SphereShapeConfiguration>(radius);
        auto sphereColliderComponent = entity->CreateComponent<PhysX::SphereColliderComponent>();
        sphereColliderComponent->SetShapeConfigurationList({ AZStd::make_pair(colliderConfig, shapeConfig) });

        RigidBodyConfiguration rigidBodyConfig;
        entity->CreateComponent<PhysX::RigidBodyComponent>(rigidBodyConfig);

        entity->Activate();
        return entity;
    }

    AZ::Entity* GenericPhysicsInterfaceTest::AddBoxEntity(const AZ::Vector3& position, const AZ::Vector3& dimensions,
        const CollisionLayer& layer)
    {
        auto entity = aznew AZ::Entity("TestBoxEntity");
        entity->CreateComponent(AZ::Uuid::CreateString("{22B10178-39B6-4C12-BB37-77DB45FDD3B6}")); // TransformComponent
        entity->Init();

        entity->Activate();

        AZ::TransformBus::Event(entity->GetId(), &AZ::TransformBus::Events::SetWorldTranslation, position);

        entity->Deactivate();

        auto colliderConfig = AZStd::make_shared<Physics::ColliderConfiguration>();
        colliderConfig->m_collisionLayer = layer;
        auto shapeConfig = AZStd::make_shared<Physics::BoxShapeConfiguration>(dimensions);
        auto boxColliderComponent = entity->CreateComponent<PhysX::BoxColliderComponent>();
        boxColliderComponent->SetShapeConfigurationList({ AZStd::make_pair(colliderConfig, shapeConfig) });

        RigidBodyConfiguration rigidBodyConfig;
        entity->CreateComponent<PhysX::RigidBodyComponent>(rigidBodyConfig);

        entity->Activate();
        return entity;
    }

    AZ::Entity* GenericPhysicsInterfaceTest::AddStaticBoxEntity(const AZ::Vector3& position,
        const AZ::Vector3& dimensions, const CollisionLayer& layer)
    {
        auto entity = aznew AZ::Entity("TestStaticBoxEntity");
        entity->CreateComponent(AZ::Uuid::CreateString("{22B10178-39B6-4C12-BB37-77DB45FDD3B6}")); // TransformComponent
        entity->Init();

        entity->Activate();

        AZ::TransformBus::Event(entity->GetId(), &AZ::TransformBus::Events::SetWorldTranslation, position);

        entity->Deactivate();

        auto colliderConfig = AZStd::make_shared<Physics::ColliderConfiguration>();
        colliderConfig->m_collisionLayer = layer;
        auto shapeConfig = AZStd::make_shared<Physics::BoxShapeConfiguration>(dimensions);
        auto boxColliderComponent = entity->CreateComponent<PhysX::BoxColliderComponent>();
        boxColliderComponent->SetShapeConfigurationList({ AZStd::make_pair(colliderConfig, shapeConfig) });

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

        auto colliderConfig = AZStd::make_shared<Physics::ColliderConfiguration>();
        colliderConfig->m_collisionLayer = layer;
        auto shapeConfig = AZStd::make_shared<Physics::CapsuleShapeConfiguration>(height, radius);
        auto capsuleColliderComponent = entity->CreateComponent<PhysX::CapsuleColliderComponent>();
        capsuleColliderComponent->SetShapeConfigurationList({ AZStd::make_pair(colliderConfig, shapeConfig) });

        RigidBodyConfiguration rigidBodyConfig;
        entity->CreateComponent<PhysX::RigidBodyComponent>(rigidBodyConfig);

        entity->Activate();
        return entity;
    }
} // namespace Physics
#endif // AZ_TESTS_ENABLED
