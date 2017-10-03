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

#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Slice/SliceAssetHandler.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/Slice/SliceMetadataInfoComponent.h>

using namespace AZ;

namespace UnitTest
{
    class MyTestComponent1
        : public Component
    {
    public:
        AZ_COMPONENT(MyTestComponent1, "{5D3B5B45-64DF-4F88-8003-271646B6CA27}");

        void Activate() override
        {
        }

        void Deactivate() override
        {
        }

        static void Reflect(ReflectContext* reflection)
        {
            SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<MyTestComponent1>()->
                    Field("float", &MyTestComponent1::m_float)->
                    Field("int", &MyTestComponent1::m_int);
            }
        }

        float m_float;
        int m_int;
    };

    class MyTestComponent2
        : public Component
    {
    public:
        AZ_COMPONENT(MyTestComponent2, "{ACD7B78D-0C30-46E8-A52E-61D4B33D5528}");

        void Activate() override
        {
        }

        void Deactivate() override
        {
        }

        static void Reflect(ReflectContext* reflection)
        {
            SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<MyTestComponent2>()->
                    Field("entityId", &MyTestComponent2::m_entityId);
            }
        }

        EntityId m_entityId;
    };


    class SliceTest
        : public AllocatorsFixture
    {
    public:
        SliceTest()
            : AllocatorsFixture(150)
        {
        }

        void SetUp() override
        {
            AllocatorsFixture::SetUp();

            AllocatorInstance<PoolAllocator>::Create();
            AllocatorInstance<ThreadPoolAllocator>::Create();
        }

        void TearDown() override
        {
            AllocatorInstance<PoolAllocator>::Destroy();
            AllocatorInstance<ThreadPoolAllocator>::Destroy();

            AllocatorsFixture::TearDown();
        }

        void run()
        {
            SerializeContext serializeContext(true, true);
            ComponentDescriptor* sliceDescriptor = SliceComponent::CreateDescriptor();
            sliceDescriptor->Reflect(&serializeContext);
            MyTestComponent1::Reflect(&serializeContext);
            MyTestComponent2::Reflect(&serializeContext);
            SliceMetadataInfoComponent::Reflect(&serializeContext);
            Entity::Reflect(&serializeContext);
            DataPatch::Reflect(&serializeContext);
            // Create database
            Data::AssetManager::Descriptor desc;
            Data::AssetManager::Create(desc);
            Data::AssetManager::Instance().RegisterHandler(aznew SliceAssetHandler(&serializeContext), AzTypeInfo<AZ::SliceAsset>::Uuid());

            AZStd::vector<char> binaryBuffer;

            Entity* sliceEntity = nullptr;
            SliceComponent* sliceComponent = nullptr;

            Entity* entity1 = nullptr;
            //Entity* entity2 = nullptr;

            MyTestComponent1* component1 = nullptr;


            /////////////////////////////////////////////////////////////////////////////
            // Test prefab without base prefab - aka root prefab
            /////////////////////////////////////////////////////////////////////////////
            sliceEntity = aznew Entity();
            sliceComponent = aznew SliceComponent();
            sliceComponent->SetSerializeContext(&serializeContext);
            sliceEntity->AddComponent(sliceComponent);
            sliceEntity->Init();
            sliceEntity->Activate();

            // Create an entity with a component to be part of the prefab
            entity1 = aznew Entity();
            component1 = aznew MyTestComponent1();
            component1->m_float = 2.0f;
            component1->m_int = 11;
            entity1->AddComponent(component1);

            sliceComponent->AddEntity(entity1);

            // store to stream
            IO::ByteContainerStream<AZStd::vector<char> > binaryStream(&binaryBuffer);
            ObjectStream* binaryObjStream = ObjectStream::Create(&binaryStream, serializeContext, ObjectStream::ST_BINARY);
            binaryObjStream->WriteClass(sliceEntity);
            AZ_TEST_ASSERT(binaryObjStream->Finalize());

            // load from stream and check values
            binaryStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
            Entity* sliceEntity2 = Utils::LoadObjectFromStream<Entity>(binaryStream, &serializeContext);
            AZ_TEST_ASSERT(sliceEntity2);
            AZ_TEST_ASSERT(sliceEntity2->FindComponent<SliceComponent>() != nullptr);
            AZ_TEST_ASSERT(sliceEntity2->FindComponent<SliceComponent>()->GetSlices().empty());

            const SliceComponent::EntityList& entityContainer = sliceEntity2->FindComponent<SliceComponent>()->GetNewEntities();
            AZ_TEST_ASSERT(entityContainer.size() == 1);
            AZ_TEST_ASSERT(entityContainer.front()->FindComponent<MyTestComponent1>());
            AZ_TEST_ASSERT(entityContainer.front()->FindComponent<MyTestComponent1>()->m_float == 2.0f);
            AZ_TEST_ASSERT(entityContainer.front()->FindComponent<MyTestComponent1>()->m_int == 11);

            delete sliceEntity2;
            sliceEntity2 = nullptr;
            /////////////////////////////////////////////////////////////////////////////

            /////////////////////////////////////////////////////////////////////////////
            // Test slice component with a single base prefab

            // Create "fake" asset - so we don't have to deal with the asset database, etc.
            Data::Asset<SliceAsset> sliceAssetHolder = Data::AssetManager::Instance().CreateAsset<SliceAsset>(Data::AssetId("{8FCBA25A-8E2F-4D76-ABCD-5EFDFD9BAD47}"));
            SliceAsset* sliceAsset1 = sliceAssetHolder.Get();
            sliceAsset1->SetData(sliceEntity, sliceComponent);

            sliceEntity2 = aznew Entity;
            SliceComponent* sliceComponent2 = sliceEntity2->CreateComponent<SliceComponent>();
            sliceComponent2->SetSerializeContext(&serializeContext);
            sliceComponent2->AddSlice(sliceAssetHolder);
            sliceEntity2->Init();
            sliceEntity2->Activate();
            SliceComponent::EntityList entities;
            sliceComponent2->GetEntities(entities); // get all entities (Instantiated if needed)
            AZ_TEST_ASSERT(entities.size() == 1);
            AZ_TEST_ASSERT(entities.front()->FindComponent<MyTestComponent1>());
            AZ_TEST_ASSERT(entities.front()->FindComponent<MyTestComponent1>()->m_float == 2.0f);
            AZ_TEST_ASSERT(entities.front()->FindComponent<MyTestComponent1>()->m_int == 11);

            // Modify Component
            entities.front()->FindComponent<MyTestComponent1>()->m_float = 5.0f;

            // Add a new entity
            Entity* entity2 = aznew Entity();
            entity2->CreateComponent<MyTestComponent2>();
            sliceComponent2->AddEntity(entity2);

            // FindSlice test
            auto prefabFindResult = sliceComponent2->FindSlice(entity2);
            AZ_TEST_ASSERT(prefabFindResult.first == nullptr); // prefab reference
            AZ_TEST_ASSERT(prefabFindResult.second == nullptr); // prefab instance
            prefabFindResult = sliceComponent2->FindSlice(entities.front());
            AZ_TEST_ASSERT(prefabFindResult.first != nullptr); // prefab reference
            AZ_TEST_ASSERT(prefabFindResult.second != nullptr); // prefab instance


            // store to stream
            binaryBuffer.clear();
            binaryStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
            binaryObjStream = ObjectStream::Create(&binaryStream, serializeContext, ObjectStream::ST_XML);
            binaryObjStream->WriteClass(sliceEntity2);
            AZ_TEST_ASSERT(binaryObjStream->Finalize());

            // load from stream and validate
            binaryStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
            Entity* sliceEntity2Clone = Utils::LoadObjectFromStream<Entity>(binaryStream, &serializeContext);
            AZ_TEST_ASSERT(sliceEntity2Clone);
            AZ_TEST_ASSERT(sliceEntity2Clone->FindComponent<SliceComponent>() != nullptr);
            AZ_TEST_ASSERT(sliceEntity2Clone->FindComponent<SliceComponent>()->GetSlices().size() == 1);
            AZ_TEST_ASSERT(sliceEntity2Clone->FindComponent<SliceComponent>()->GetNewEntities().size() == 1);
            // instantiate the loaded component
            sliceEntity2Clone->FindComponent<SliceComponent>()->SetSerializeContext(&serializeContext);
            entities.clear();
            sliceEntity2Clone->FindComponent<SliceComponent>()->GetEntities(entities);
            AZ_TEST_ASSERT(entities.size() == 2); // original entity (prefab) plus the newly added one
            AZ_TEST_ASSERT(entities.front()->FindComponent<MyTestComponent1>());
            AZ_TEST_ASSERT(entities.front()->FindComponent<MyTestComponent1>()->m_float == 5.0f); // test modified field
            AZ_TEST_ASSERT(entities.front()->FindComponent<MyTestComponent1>()->m_int == 11);
            AZ_TEST_ASSERT(entities.back()->FindComponent<MyTestComponent2>());

            ////////////////////////////////////////////////////////////////////////////

            /////////////////////////////////////////////////////////////////////////////
            // Test slice component 3 levels of customization
            Data::Asset<SliceAsset> sliceAssetHolder1 = Data::AssetManager::Instance().CreateAsset<SliceAsset>(Data::AssetId("{57B76D9B-F0D0-429F-A723-64610BF9D4C0}"));
            SliceAsset* sliceAsset2 = sliceAssetHolder1.Get();
            sliceAsset2->SetData(sliceEntity2, sliceComponent2);

            Entity* sliceEntity3 = aznew Entity();
            SliceComponent* sliceComponent3 = sliceEntity3->CreateComponent<SliceComponent>();
            sliceComponent3->SetSerializeContext(&serializeContext);
            sliceEntity3->Init();
            sliceEntity3->Activate();
            sliceComponent3->AddSlice(sliceAsset2);
            entities.clear();
            sliceComponent3->GetEntities(entities);
            AZ_TEST_ASSERT(entities.size() == 2);
            AZ_TEST_ASSERT(entities.front()->FindComponent<MyTestComponent1>());
            AZ_TEST_ASSERT(entities.front()->FindComponent<MyTestComponent1>()->m_float == 5.0f);
            AZ_TEST_ASSERT(entities.front()->FindComponent<MyTestComponent1>()->m_int == 11);
            AZ_TEST_ASSERT(entities.back()->FindComponent<MyTestComponent2>());

            // Modify Component
            entities.front()->FindComponent<MyTestComponent1>()->m_int = 22;

            // Remove a entity
            sliceComponent3->RemoveEntity(entities.back());

            // store to stream
            binaryBuffer.clear();
            binaryStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
            binaryObjStream = ObjectStream::Create(&binaryStream, serializeContext, ObjectStream::ST_XML);
            binaryObjStream->WriteClass(sliceEntity3);
            AZ_TEST_ASSERT(binaryObjStream->Finalize());

            // modify root asset (to make sure that changes propgate properly), we have not overridden MyTestComponent1::m_float
            entities.clear();
            sliceComponent2->GetEntities(entities);
            entities.front()->FindComponent<MyTestComponent1>()->m_float = 15.0f;

            // load from stream and validate
            binaryStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
            Entity* sliceEntity3Clone = Utils::LoadObjectFromStream<Entity>(binaryStream, &serializeContext);
            // instantiate the loaded component
            sliceEntity3Clone->FindComponent<SliceComponent>()->SetSerializeContext(&serializeContext);
            sliceEntity3Clone->Init();
            sliceEntity3Clone->Activate();
            entities.clear();
            sliceEntity3Clone->FindComponent<SliceComponent>()->GetEntities(entities);
            AZ_TEST_ASSERT(entities.size() == 1);
            AZ_TEST_ASSERT(entities.front()->FindComponent<MyTestComponent1>());
            AZ_TEST_ASSERT(entities.front()->FindComponent<MyTestComponent1>()->m_float == 15.0f); // this value was modified in the base prefab
            AZ_TEST_ASSERT(entities.front()->FindComponent<MyTestComponent1>()->m_int == 22);

            /////////////////////////////////////////////////////////////////////////////

            /////////////////////////////////////////////////////////////////////////////
            // Testing patching instance-owned entities in an existing prefab.
            {
                Entity rootEntity;
                SliceComponent* rootSlice = rootEntity.CreateComponent<SliceComponent>();
                rootSlice->SetSerializeContext(&serializeContext);
                rootEntity.Init();
                rootEntity.Activate();

                Data::Asset<SliceAsset> testAsset = Data::AssetManager::Instance().CreateAsset<SliceAsset>(Data::AssetId("{BF0B1C16-6456-43EF-A9C8-C3F2A8EEA7CE}"));
                Entity* testAssetEntity = aznew Entity;
                SliceComponent* testAssetSlice = testAssetEntity->CreateComponent<SliceComponent>();
                testAssetSlice->SetSerializeContext(&serializeContext);
                
                // Add a couple test entities to the prefab asset.
                Entity* testEntity1 = aznew Entity();
                testEntity1->CreateComponent<MyTestComponent1>()->m_float = 15.f;
                testAssetSlice->AddEntity(testEntity1);
                Entity* testEntity2 = aznew Entity();
                testEntity2->CreateComponent<MyTestComponent1>()->m_float = 5.f;
                testAssetSlice->AddEntity(testEntity2);

                testAsset.Get()->SetData(testAssetEntity, testAssetSlice);

                // Test removing an entity from an existing prefab instance, cloning it, and re-adding it.
                // This emulates what's required for fast undo/redo entity restore for slice-owned entities.
                {
                    rootSlice->AddSlice(testAsset);
                    entities.clear();
                    rootSlice->GetEntities(entities);

                    // Grab one of the entities to remove, clone, and re-add.
                    AZ::Entity* entity = entities.back();
                    AZ_TEST_ASSERT(entity);
                    SliceComponent::SliceInstanceAddress addr = rootSlice->FindSlice(entity);
                    AZ_TEST_ASSERT(addr.first && addr.second);
                    const SliceComponent::SliceInstanceId instanceId = addr.second->GetId();
                    SliceComponent::EntityAncestorList ancestors;
                    addr.first->GetInstanceEntityAncestry(entity->GetId(), ancestors, 1);
                    AZ_TEST_ASSERT(ancestors.size() == 1);
                    SliceComponent::EntityRestoreInfo restoreInfo;
                    AZ_TEST_ASSERT(rootSlice->GetEntityRestoreInfo(entity->GetId(), restoreInfo));

                    // Duplicate the entity and make a data change we can validate later.
                    AZ::Entity* clone = serializeContext.CloneObject(entity);
                    clone->FindComponent<MyTestComponent1>()->m_float = 10.f;
                    // Remove the original. We have two entities in the instance, so this will not wipe the instance.
                    rootSlice->RemoveEntity(entity);
                    // Patch it back into the prefab.
                    rootSlice->RestoreEntity(clone, restoreInfo);
                    // Re-retrieve the entity. We should find it, and it should match the data of the clone.
                    addr = rootSlice->FindSlice(clone);
                    ASSERT_NE(nullptr, addr.first);
                    EXPECT_EQ(instanceId, addr.second->GetId());
                    entities.clear();
                    rootSlice->GetEntities(entities);
                    ASSERT_EQ(clone, entities.back());
                    EXPECT_EQ(10.0f, entities.back()->FindComponent<MyTestComponent1>()->m_float);
                    ancestors.clear();
                    addr.first->GetInstanceEntityAncestry(clone->GetId(), ancestors, 1);
                    EXPECT_EQ(1, ancestors.size());
                }

                // We also support re-adding when the reference no longer exists, so run the above
                // test, but such that the entity we remove is the last one, in turn destroying
                // the whole instance & reference.
                {
                    rootSlice->RemoveSlice(testAsset);
                    rootSlice->AddSlice(testAsset);
                    entities.clear();
                    rootSlice->GetEntities(entities);

                    // Remove one of the entities.
                    rootSlice->RemoveEntity(entities.front());

                    AZ::Entity* entity = entities.back();
                    AZ_TEST_ASSERT(entity);
                    SliceComponent::SliceInstanceAddress addr = rootSlice->FindSlice(entity);
                    AZ_TEST_ASSERT(addr.first && addr.second);
                    const SliceComponent::SliceInstanceId instanceId = addr.second->GetId();
                    SliceComponent::EntityAncestorList ancestors;
                    addr.first->GetInstanceEntityAncestry(entity->GetId(), ancestors, 1);
                    AZ_TEST_ASSERT(ancestors.size() == 1);
                    SliceComponent::EntityRestoreInfo restoreInfo;
                    AZ_TEST_ASSERT(rootSlice->GetEntityRestoreInfo(entity->GetId(), restoreInfo));

                    // Duplicate the entity and make a data change we can validate later.
                    AZ::Entity* clone = serializeContext.CloneObject(entity);
                    clone->FindComponent<MyTestComponent1>()->m_float = 10.f;
                    // Remove the original. We have two entities in the instance, so this will not wipe the instance.
                    rootSlice->RemoveEntity(entity);
                    // Patch it back into the prefab.
                    rootSlice->RestoreEntity(clone, restoreInfo);
                    // Re-retrieve the entity. We should find it, and it should match the data of the clone.
                    addr = rootSlice->FindSlice(clone);
                    AZ_TEST_ASSERT(addr.first);
                    AZ_TEST_ASSERT(addr.second->GetId() == instanceId);
                    entities.clear();
                    rootSlice->GetEntities(entities);
                    AZ_TEST_ASSERT(entities.back() == clone);
                    AZ_TEST_ASSERT(entities.back()->FindComponent<MyTestComponent1>()->m_float == 10.0f);
                    ancestors.clear();
                    addr.first->GetInstanceEntityAncestry(clone->GetId(), ancestors, 1);
                    AZ_TEST_ASSERT(ancestors.size() == 1);
                }
            }
            /////////////////////////////////////////////////////////////////////////////

            // just reset the asset, don't delete the owned entity (we delete it later)
            sliceAsset1->SetData(nullptr, nullptr, false);
            sliceAsset2->SetData(nullptr, nullptr, false);

            delete sliceEntity;
            delete sliceEntity2;
            delete sliceEntity2Clone;
            delete sliceEntity3;
            delete sliceEntity3Clone;
            delete sliceDescriptor;

            sliceAssetHolder = nullptr; // release asset
            sliceAssetHolder1 = nullptr;

            Data::AssetManager::Destroy();
        }
    };

    TEST_F(SliceTest, Test)
    {
        run();
    }

    class DataFlags_CleanupTest
        : public AllocatorsFixture
    {
    protected:
        void SetUp() override
        {
            AllocatorsFixture::SetUp();

            auto allEntitiesValidFunction = [](EntityId) { return true; };
            m_dataFlags = AZStd::make_unique<SliceComponent::DataFlagsPerEntity>(allEntitiesValidFunction);
            m_addressOfSetFlag.push_back(AZ_CRC("Components"));
            m_valueOfSetFlag = DataPatch::Flag::ForceOverride;
            m_entityIdToGoMissing = Entity::MakeId();
            m_entityIdToRemain = Entity::MakeId();
            m_remainingEntity = AZStd::make_unique<Entity>(m_entityIdToRemain);
            m_remainingEntityList.push_back(m_remainingEntity.get());

            m_dataFlags->SetEntityDataFlagsAtAddress(m_entityIdToGoMissing, m_addressOfSetFlag, m_valueOfSetFlag);
            m_dataFlags->SetEntityDataFlagsAtAddress(m_entityIdToRemain, m_addressOfSetFlag, m_valueOfSetFlag);

            m_dataFlags->Cleanup(m_remainingEntityList);
        }

        void TearDown() override
        {
            m_remainingEntity.reset();
            m_dataFlags.reset();

            AllocatorsFixture::TearDown();
        }

        AZStd::unique_ptr<SliceComponent::DataFlagsPerEntity> m_dataFlags;
        DataPatch::AddressType m_addressOfSetFlag;
        DataPatch::Flags m_valueOfSetFlag;
        EntityId m_entityIdToGoMissing;
        EntityId m_entityIdToRemain;
        AZStd::unique_ptr<Entity> m_remainingEntity;
        SliceComponent::EntityList m_remainingEntityList;
    };

    TEST_F(DataFlags_CleanupTest, MissingEntitiesPruned)
    {
        EXPECT_TRUE(m_dataFlags->GetEntityDataFlags(m_entityIdToGoMissing).empty());
    }

    TEST_F(DataFlags_CleanupTest, ValidEntitiesRemain)
    {
        EXPECT_EQ(m_dataFlags->GetEntityDataFlagsAtAddress(m_entityIdToRemain, m_addressOfSetFlag), m_valueOfSetFlag);
    }
}

