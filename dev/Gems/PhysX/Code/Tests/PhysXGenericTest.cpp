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
#include <StaticRigidBodyComponent.h>
#include <SystemComponent.h>
#include <TerrainComponent.h>
#include <ComponentDescriptors.h>
#include <AzFramework/Physics/CollisionBus.h>
#include <AzFramework/Physics/Utils.h>
#include <AzFramework/IO/LocalFileIO.h>
#include "PhysXGenericTest.h"

namespace PhysX
{
    AZ::ComponentTypeList PhysXApplication::GetRequiredSystemComponents() const
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

    void PhysXApplication::CreateReflectionManager()
    {
        AZ::ComponentApplication::CreateReflectionManager();
        PhysX::SystemComponent::InitializePhysXSDK();
        for (AZ::ComponentDescriptor* descriptor : GetDescriptors())
        {
            RegisterComponentDescriptor(descriptor);
        }
    }

    void PhysXApplication::Destroy()
    {
        AZ::ComponentApplication::Destroy();
        PhysX::SystemComponent::DestroyPhysXSDK();
    }
    
    void Environment::SetupInternal()
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
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (serializeContext)
        {
            // The reflection for generic physics API types which PhysX relies on happens in AzFramework and is not
            // called by PhysX itself, so we have to make sure it is called here
            Physics::ReflectionUtils::ReflectPhysicsApi(serializeContext);
            m_transformComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(AzFramework::TransformComponent::CreateDescriptor());
            m_transformComponentDescriptor->Reflect(serializeContext);
        }
        m_systemEntity->Init();
        m_systemEntity->Activate();

        if (s_enablePvd)
        {
            bool pvdConnectionSuccessful;
            PhysX::SystemRequestsBus::BroadcastResult(pvdConnectionSuccessful, &PhysX::SystemRequests::ConnectToPvd);
        }
    }

    void Environment::TeardownInternal()
    {
        if (s_enablePvd)
        {
            PhysX::SystemRequestsBus::Broadcast(&PhysX::SystemRequests::DisconnectFromPvd);
        }
        AZ::IO::FileIOBase::SetInstance(nullptr);

        m_transformComponentDescriptor.reset();
        m_fileIo.reset();
        m_application->Destroy();
        delete m_application;

        AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
    }

    AZ_UNIT_TEST_HOOK(new TestEnvironment);
#ifdef HAVE_BENCHMARK
    AZ_BENCHMARK_HOOK()
#endif
} // namespace PhysX

namespace Physics
{
    void GenericPhysicsFixture::SetUpInternal()
    {
        m_defaultWorld = AZ::Interface<Physics::System>::Get()->CreateWorld(Physics::DefaultPhysicsWorldId);

        Physics::DefaultWorldBus::Handler::BusConnect();
    }

    void GenericPhysicsFixture::TearDownInternal()
    {
        PhysX::MaterialManagerRequestsBus::Broadcast(&PhysX::MaterialManagerRequestsBus::Events::ReleaseAllMaterials);
        Physics::DefaultWorldBus::Handler::BusDisconnect();
        m_defaultWorld = nullptr;
    }

    AZStd::shared_ptr<World> GenericPhysicsFixture::GetDefaultWorld()
    {
        return m_defaultWorld;
    }

    AZ::Entity* GenericPhysicsFixture::AddSphereEntity(const AZ::Vector3& position, const float radius,
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

    AZ::Entity* GenericPhysicsFixture::AddBoxEntity(const AZ::Vector3& position, const AZ::Vector3& dimensions,
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

    AZ::Entity* GenericPhysicsFixture::AddStaticSphereEntity(const AZ::Vector3& position, const float radius,
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

        entity->CreateComponent<PhysX::StaticRigidBodyComponent>();

        entity->Activate();
        return entity;
    }

    AZ::Entity* GenericPhysicsFixture::AddStaticBoxEntity(const AZ::Vector3& position,
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

        entity->CreateComponent<PhysX::StaticRigidBodyComponent>();

        entity->Activate();
        return entity;
    }

    AZ::Entity* GenericPhysicsFixture::AddStaticCapsuleEntity(const AZ::Vector3& position, const float height,
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

        entity->CreateComponent<PhysX::StaticRigidBodyComponent>();

        entity->Activate();
        return entity;
    }


    AZ::Entity* GenericPhysicsFixture::AddCapsuleEntity(const AZ::Vector3& position, const float height,
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

    AZStd::unique_ptr<AZ::Entity> GenericPhysicsFixture::AddMultiShapeEntity(const MultiShapeConfig& config)
    {
        AZStd::unique_ptr<AZ::Entity> entity(aznew AZ::Entity("TestShapeEntity"));
        entity->CreateComponent(AZ::Uuid::CreateString("{22B10178-39B6-4C12-BB37-77DB45FDD3B6}")); // TransformComponent
        entity->Init();

        entity->Activate();

        AZ::TransformBus::Event(entity->GetId(), &AZ::TransformBus::Events::SetWorldTranslation, config.m_position);
        AZ::TransformBus::Event(entity->GetId(), &AZ::TransformBus::Events::SetLocalRotation, config.m_rotation);

        entity->Deactivate();

        auto colliderConfig = AZStd::make_shared<Physics::ColliderConfiguration>();
        colliderConfig->m_collisionLayer = config.m_layer;

        Physics::ShapeConfigurationList shapeconfigurationList;

        struct Visitor
        {
            AZStd::shared_ptr<Physics::ColliderConfiguration>& colliderConfig;
            Physics::ShapeConfigurationList& shapeconfigurationList;

            void operator()(const MultiShapeConfig::ShapeList::ShapeData::Box& box) const
            {
                shapeconfigurationList.push_back
                ({
                    colliderConfig,
                    AZStd::make_shared<Physics::BoxShapeConfiguration>(box.m_extent)
                });
            }
            void operator()(const MultiShapeConfig::ShapeList::ShapeData::Sphere& sphere) const
            {
                shapeconfigurationList.push_back
                ({
                    colliderConfig,
                    AZStd::make_shared<Physics::SphereShapeConfiguration>(sphere.m_radius)
                });
            }
            void operator()(const MultiShapeConfig::ShapeList::ShapeData::Capsule& capsule) const
            {
                shapeconfigurationList.push_back
                ({
                    colliderConfig,
                    AZStd::make_shared<Physics::CapsuleShapeConfiguration>(capsule.m_height, capsule.m_radius)
                });
            }
            void operator()(const AZStd::monostate&) const
            {
                AZ_Assert(false, "Invalid shape type");
            }
        };
        Visitor addShapeConfig = {colliderConfig, shapeconfigurationList};

        for (const MultiShapeConfig::ShapeList::ShapeData& shapeConfig : config.m_shapes.m_shapesData)
        {
            AZStd::visit(addShapeConfig, shapeConfig.m_data);
        }

        auto colliderComponent = entity->CreateComponent<PhysX::BaseColliderComponent>();
        colliderComponent->SetShapeConfigurationList(shapeconfigurationList);

        RigidBodyConfiguration rigidBodyConfig;
        entity->CreateComponent<PhysX::RigidBodyComponent>(rigidBodyConfig);

        entity->Activate();

        for (int i = 0; i < colliderComponent->GetShapes().size(); ++i)
        {
            colliderComponent->GetShapes()[i]->SetLocalPose(config.m_shapes.m_shapesData[i].m_offset, AZ::Quaternion::CreateIdentity());
        }

        return AZStd::unique_ptr<AZ::Entity>{ AZStd::move(entity) };
    }

} // namespace Physics

