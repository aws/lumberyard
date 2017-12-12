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

#include "TestTypes.h"

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Memory/PoolAllocator.h>

#include <AzFramework/Entity/EntityReference.h>
#include <AzFramework/Components/TransformComponent.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace AzFramework;

    //! Unit Test for EntityReference class which acts like an EntityId and can serialized the actual entity instance
    //! when the entity is registered with the component application bus. It provides soft ownership for an entity
    class EntityReferenceTest
        : public AllocatorsFixture
    {
    public:
        EntityReferenceTest()
            : AllocatorsFixture(15, false)
        {
            AllocatorInstance<PoolAllocator>::Create();
            AllocatorInstance<ThreadPoolAllocator>::Create();

            Entity::Reflect(&m_serializeContext);
            EntityReference::Reflect(&m_serializeContext);

            AZStd::unique_ptr<ComponentDescriptor> transformComponentDesc(AzFramework::TransformComponent::CreateDescriptor());
            transformComponentDesc->Reflect(&m_serializeContext);
        }

        ~EntityReferenceTest()
        {
            AllocatorInstance<PoolAllocator>::Destroy();
            AllocatorInstance<ThreadPoolAllocator>::Destroy();
        }

        void SetUp() override
        {
            ComponentApplication::Descriptor desc;
            desc.m_useExistingAllocator = true;
            m_app.Create(desc);
        }

        void TearDown() override
        {
            m_app.Destroy();
        }

        ComponentApplication m_app;
        SerializeContext m_serializeContext;
    };

    TEST_F(EntityReferenceTest, EntityIdTest)
    {
        AZStd::vector<EntityReference> referenceArray;
        AZ::Entity entity1;
        entity1.Init();

        AZ::Entity entity2;
        entity2.Init();

        referenceArray.emplace_back(entity1.GetId());
        referenceArray.emplace_back(entity2.GetId());

        AZ::Entity* referenceEntity{};
        AZ::ComponentApplicationBus::BroadcastResult(referenceEntity, &AZ::ComponentApplicationRequests::FindEntity, referenceArray[0]);
        EXPECT_EQ(&entity1, referenceEntity);

        AZ::ComponentApplicationBus::BroadcastResult(referenceEntity, &AZ::ComponentApplicationRequests::FindEntity, referenceArray[1]);
        EXPECT_EQ(&entity2, referenceEntity);
    }

    TEST_F(EntityReferenceTest, EntityInstanceTest)
    {
        AZStd::vector<EntityReference> referenceArray;
        AZ::Entity entity1;
        entity1.Init();

        AZ::Entity entity2;
        entity2.Init();

        referenceArray.emplace_back(&entity1);
        referenceArray.emplace_back(&entity2);

        EXPECT_EQ(&entity1, referenceArray[0].GetEntity());
        EXPECT_EQ(&entity2, referenceArray[1].GetEntity());
    }

    // Test that entity references m_entity member gets updated when the entity is added to the component application bus
    // And that entity references reset to nullptr when the entity is removed from the component application bus
    TEST_F(EntityReferenceTest, AddEntityId_InitTest)
    {
        EntityReference entityRef;
        EntityId id1{ 43 };
        {
            AZ::Entity entity1(id1);
            entityRef = entity1.GetId();
            EXPECT_EQ(id1, entityRef.GetEntityId());
            EXPECT_NE(&entity1, entityRef.GetEntity());
            EXPECT_EQ(nullptr, entityRef.GetEntity());

            entity1.Init();
            EXPECT_EQ(&entity1, entityRef.GetEntity());
        }

        EXPECT_EQ(id1, entityRef.GetEntityId());
        EXPECT_EQ(nullptr, entityRef.GetEntity());
    }

    TEST_F(EntityReferenceTest, SerializationTest)
    {
        AZ::Entity entityNotAddedToComponentApp;
        entityNotAddedToComponentApp.CreateComponent<AzFramework::TransformComponent>();

        AZ::Entity entityAddedToComponentApp;
        entityAddedToComponentApp.CreateComponent<AzFramework::TransformComponent>();
        entityAddedToComponentApp.Init();
        entityAddedToComponentApp.Activate();
        AZ::Transform outTransform = Transform::CreateIdentity();
        outTransform.SetTranslation(1.0f, -54.1f, 73.6f);
        AZ::TransformBus::Event(entityAddedToComponentApp.GetId(), &AZ::TransformBus::Events::SetLocalTM, outTransform);


        AZStd::vector<AZ::u8> binaryBuffer;

        // Serialization of EntityRef containing an EntityId which is not registered with the component application bus
        {
            EntityReference entityRef1;
            entityRef1 = entityNotAddedToComponentApp.GetId();
            AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8> > binaryStream(&binaryBuffer);
            AZ::ObjectStream* binaryObjStream = ObjectStream::Create(&binaryStream, m_serializeContext, ObjectStream::ST_BINARY);
            binaryObjStream->WriteClass(&entityRef1);
            binaryObjStream->Finalize();

            EntityReference entityRefForLoad;
            IO::ByteContainerStream<const AZStd::vector<AZ::u8> > loadStream(&binaryBuffer);
            loadStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
            AZ::Utils::LoadObjectFromStreamInPlace(loadStream, entityRefForLoad, &m_serializeContext);

            // Since @entityRef1 refers to an entity id not registered with the component application bus
            // and because it was not initailized with an Entity*, only the entity id should be serialized
            EXPECT_EQ(entityNotAddedToComponentApp.GetId(), entityRefForLoad.GetEntityId());
            EXPECT_EQ(nullptr, entityRefForLoad.GetEntity());
        }

        // Serialization of EntityRef containing an EntityId which is with the component application bus
        // This should result in the entity instance for the EntityId being serialized to an output stream
        {
            binaryBuffer.clear();
            EntityReference entityRef2;
            entityRef2 = entityAddedToComponentApp.GetId();
            AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8> > binaryStream(&binaryBuffer);
            AZ::ObjectStream* binaryObjStream = ObjectStream::Create(&binaryStream, m_serializeContext, ObjectStream::ST_BINARY);
            binaryObjStream->WriteClass(&entityRef2);
            binaryObjStream->Finalize();

            EntityReference entityRefForLoad;
            IO::ByteContainerStream<const AZStd::vector<AZ::u8> > loadStream(&binaryBuffer);
            loadStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
            AZ::Utils::LoadObjectFromStreamInPlace(loadStream, entityRefForLoad, &m_serializeContext);

            // Since @entityRef2 refers to an entity id that is registered with the component application bus
            // and both the entity id and instance should have been serialized.
            EXPECT_EQ(entityAddedToComponentApp.GetId(), entityRefForLoad.GetEntityId());

            auto* loadedEntity = entityRefForLoad.GetEntity();
            EXPECT_NE(nullptr, loadedEntity);
            EXPECT_NE(&entityAddedToComponentApp, loadedEntity);

            auto* loadedTransformComponent = loadedEntity->FindComponent<AzFramework::TransformComponent>();
            EXPECT_NE(nullptr, loadedTransformComponent);

            AZStd::unordered_map<AZ::EntityId, AZ::EntityId> remapIdMap;
            AZ::EntityUtils::GenerateNewIdsAndFixRefs(&entityRefForLoad, remapIdMap, &m_serializeContext);
            EXPECT_NE(entityAddedToComponentApp.GetId(), loadedEntity->GetId());
            loadedEntity->Init();
            loadedEntity->Activate();
            AZ::Transform loadedTransform = Transform::CreateIdentity();
            AZ::TransformBus::EventResult(loadedTransform, loadedEntity->GetId(), &AZ::TransformBus::Events::GetLocalTM);
            EXPECT_EQ(outTransform, loadedTransform);

            delete loadedEntity;
            // Loaded Entity Ref should point to nullptr after the loaded entity is deleted.
            EXPECT_EQ(nullptr, entityRefForLoad.GetEntity());
        }
    }

    TEST_F(EntityReferenceTest, RemapIdTest)
    {
        AZ::Entity remapEntity;
        AzFramework::EntityReference entityRef(&remapEntity);
        EXPECT_EQ(&remapEntity, entityRef.GetEntity());
        EXPECT_EQ(entityRef.GetEntityId(), entityRef.GetEntity()->GetId());

        AZStd::unordered_map<AZ::EntityId, AZ::EntityId> entityIdMap;
        AZ::EntityUtils::GenerateNewIdsAndFixRefs(&remapEntity, entityIdMap, &m_serializeContext);
        
        AZ_TEST_START_ASSERTTEST;
        EXPECT_NE(entityRef.GetEntityId(), entityRef.GetEntity()->GetId());
        AZ_TEST_STOP_ASSERTTEST(1);

        AZ_TEST_START_ASSERTTEST;
        EXPECT_EQ(&remapEntity, entityRef.GetEntity());
        AZ_TEST_STOP_ASSERTTEST(1);

        AzFramework::EntityReference clonedRef;
        m_serializeContext.CloneObjectInplace(clonedRef, &entityRef);
        EXPECT_EQ(nullptr, clonedRef.GetEntity());
        EXPECT_EQ(clonedRef.GetEntityId(), entityRef.GetEntityId());
    }
}
