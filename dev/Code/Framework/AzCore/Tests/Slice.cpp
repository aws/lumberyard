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

#include "FileIOBaseTestTypes.h"

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Streamer.h>

#include <AzCore/Jobs/JobManager.h>
#include <AzCore/Jobs/JobContext.h>

#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Slice/SliceAssetHandler.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Math/Sfmt.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/Slice/SliceMetadataInfoComponent.h>
#include <AzCore/UnitTest/TestTypes.h>

#if defined(HAVE_BENCHMARK)
#include <benchmark/benchmark.h>
#endif

#include "Utils.h"

using namespace AZ;

namespace UnitTest
{
    using namespace AZ::Data;
    using namespace AZ::IO;

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
                serializeContext->Class<MyTestComponent1, Component>()->
                    Field("float", &MyTestComponent1::m_float)->
                    Field("int", &MyTestComponent1::m_int);
            }
        }

        float m_float = 0.f;
        int m_int = 0;
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
                serializeContext->Class<MyTestComponent2, Component>()->
                    Field("entityId", &MyTestComponent2::m_entityId);
            }
        }

        EntityId m_entityId;
    };

    class SliceTest_MockCatalog
        : public AssetCatalog
        , public AZ::Data::AssetCatalogRequestBus::Handler
    {
    private:
        AZ::Uuid randomUuid = AZ::Uuid::CreateRandom();
        AZStd::vector<AssetId> m_mockAssetIds;

    public:
        AZ_CLASS_ALLOCATOR(SliceTest_MockCatalog, AZ::SystemAllocator, 0);

        SliceTest_MockCatalog()
        {
            AssetCatalogRequestBus::Handler::BusConnect();
        }

        ~SliceTest_MockCatalog()
        {
            AssetCatalogRequestBus::Handler::BusDisconnect();
        }

        AssetId GenerateMockAssetId()
        {
            AssetId assetId = AssetId(AZ::Uuid::CreateRandom(), 0);
            m_mockAssetIds.push_back(assetId);
            return assetId;
        }
        
        //////////////////////////////////////////////////////////////////////////
        // AssetCatalogRequestBus
        AssetInfo GetAssetInfoById(const AssetId& id) override
        {
            AssetInfo result;
            result.m_assetType = AZ::AzTypeInfo<AZ::SliceAsset>::Uuid();
            for (const AssetId& assetId : m_mockAssetIds)
            {
                if (assetId == id)
                {
                    result.m_assetId = id;
                    break;
                }
            }
            return result;
        }
        //////////////////////////////////////////////////////////////////////////

        AssetStreamInfo GetStreamInfoForLoad(const AssetId& id, const AssetType& type) override
        {
            EXPECT_TRUE(type == AzTypeInfo<SliceAsset>::Uuid());
            AssetStreamInfo info;
            info.m_dataOffset = 0;
            info.m_isCustomStreamType = false;
            info.m_streamFlags = IO::OpenMode::ModeRead;

            for (int i = 0; i < m_mockAssetIds.size(); ++i)
            {
                if (m_mockAssetIds[i] == id)
                {
                    info.m_streamName = AZStd::string::format("MockSliceAssetName%d", i);
                }
            }

            if (!info.m_streamName.empty())
            {
                // this ensures tha parallel running unit tests do not overlap their files that they use.
                AZStd::string fullName = AZStd::string::format("%s%s-%s", GetTestFolderPath().c_str(), randomUuid.ToString<AZStd::string>().c_str(), info.m_streamName.c_str());
                info.m_streamName = fullName;
                info.m_dataLen = static_cast<size_t>(IO::SystemFile::Length(info.m_streamName.c_str()));
            }
            else
            {
                info.m_dataLen = 0;
            }

            return info;
        }

        AssetStreamInfo GetStreamInfoForSave(const AssetId& id, const AssetType& type) override
        {
            AssetStreamInfo info;
            info = GetStreamInfoForLoad(id, type);
            info.m_streamFlags = IO::OpenMode::ModeWrite;
            return info;
        }

        bool SaveAsset(Asset<SliceAsset>& asset)
        {
            volatile bool isDone = false;
            volatile bool succeeded = false;
            AssetBusCallbacks callbacks;
            callbacks.SetCallbacks(nullptr, nullptr, nullptr,
                [&isDone, &succeeded](const Asset<AssetData>& /*asset*/, bool isSuccessful, AssetBusCallbacks& /*callbacks*/)
            {
                isDone = true;
                succeeded = isSuccessful;
            }, nullptr, nullptr);

            callbacks.BusConnect(asset.GetId());
            asset.Save();

            while (!isDone)
            {
                AssetManager::Instance().DispatchEvents();
            }
            return succeeded;
        }
    };

    class SliceTest
        : public AllocatorsFixture
    {
    public:
        SliceTest()
            : AllocatorsFixture()
        {
        }

        void SetUp() override
        {
            AllocatorsFixture::SetUp();

            AllocatorInstance<PoolAllocator>::Create();
            AllocatorInstance<ThreadPoolAllocator>::Create();

            m_serializeContext = aznew SerializeContext(true, true);
            m_sliceDescriptor = SliceComponent::CreateDescriptor();

            m_prevFileIO = IO::FileIOBase::GetInstance();
            IO::FileIOBase::SetInstance(&m_fileIO);
            IO::Streamer::Descriptor streamerDesc;
            IO::Streamer::Create(streamerDesc);

            m_sliceDescriptor->Reflect(m_serializeContext);
            MyTestComponent1::Reflect(m_serializeContext);
            MyTestComponent2::Reflect(m_serializeContext);
            SliceMetadataInfoComponent::Reflect(m_serializeContext);
            Entity::Reflect(m_serializeContext);
            DataPatch::Reflect(m_serializeContext);

            // Create database
            Data::AssetManager::Descriptor desc;
            Data::AssetManager::Create(desc);
            Data::AssetManager::Instance().RegisterHandler(aznew SliceAssetHandler(m_serializeContext), AzTypeInfo<AZ::SliceAsset>::Uuid());
            m_catalog.reset(aznew SliceTest_MockCatalog());
            AssetManager::Instance().RegisterCatalog(m_catalog.get(), AzTypeInfo<AZ::SliceAsset>::Uuid());
        }

        void TearDown() override
        {
            m_catalog->DisableCatalog();
            m_catalog.reset();
            Data::AssetManager::Destroy();
            delete m_sliceDescriptor;
            delete m_serializeContext;

            if (IO::Streamer::IsReady())
            {
                IO::Streamer::Destroy();
            }
            IO::FileIOBase::SetInstance(m_prevFileIO);

            AllocatorInstance<PoolAllocator>::Destroy();
            AllocatorInstance<ThreadPoolAllocator>::Destroy();

            AllocatorsFixture::TearDown();
        }

        FileIOBase* m_prevFileIO{ nullptr };
        TestFileIOBase m_fileIO;
        AZStd::unique_ptr<SliceTest_MockCatalog> m_catalog;
        SerializeContext* m_serializeContext;
        ComponentDescriptor* m_sliceDescriptor;
    };

    TEST_F(SliceTest, UberTest)
    {
        SerializeContext& serializeContext = *m_serializeContext;
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
        Asset<SliceAsset> sliceAssetHolder = AssetManager::Instance().CreateAsset<SliceAsset>(m_catalog->GenerateMockAssetId());
        SliceAsset* sliceAsset1 = sliceAssetHolder.Get();
        sliceAsset1->SetData(sliceEntity, sliceComponent);
        // ASSERT_TRUE(m_catalog->SaveAsset(sliceAssetHolder));

        AZ::Data::AssetInfo assetInfo;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, sliceAssetHolder.GetId());
        ASSERT_TRUE(assetInfo.m_assetId.IsValid());

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
        AZ_TEST_ASSERT(prefabFindResult.GetReference() == nullptr); // prefab reference
        AZ_TEST_ASSERT(prefabFindResult.GetInstance() == nullptr); // prefab instance
        prefabFindResult = sliceComponent2->FindSlice(entities.front());
        AZ_TEST_ASSERT(prefabFindResult.GetReference() != nullptr); // prefab reference
        AZ_TEST_ASSERT(prefabFindResult.GetInstance() != nullptr); // prefab instance


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
        Asset<SliceAsset> sliceAssetHolder1 = AssetManager::Instance().CreateAsset<SliceAsset>(m_catalog->GenerateMockAssetId());
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

            Asset<SliceAsset> testAsset = AssetManager::Instance().CreateAsset<SliceAsset>(m_catalog->GenerateMockAssetId());
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
                AZ_TEST_ASSERT(addr.IsValid());
                const SliceComponent::SliceInstanceId instanceId = addr.GetInstance()->GetId();
                SliceComponent::EntityAncestorList ancestors;
                addr.GetReference()->GetInstanceEntityAncestry(entity->GetId(), ancestors, 1);
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
                ASSERT_TRUE(addr.IsValid());
                EXPECT_EQ(instanceId, addr.GetInstance()->GetId());
                entities.clear();
                rootSlice->GetEntities(entities);
                ASSERT_EQ(clone, entities.back());
                EXPECT_EQ(10.0f, entities.back()->FindComponent<MyTestComponent1>()->m_float);
                ancestors.clear();
                addr.GetReference()->GetInstanceEntityAncestry(clone->GetId(), ancestors, 1);
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
                AZ_TEST_ASSERT(addr.IsValid());
                const SliceComponent::SliceInstanceId instanceId = addr.GetInstance()->GetId();
                SliceComponent::EntityAncestorList ancestors;
                addr.GetReference()->GetInstanceEntityAncestry(entity->GetId(), ancestors, 1);
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
                AZ_TEST_ASSERT(addr.IsValid());
                AZ_TEST_ASSERT(addr.GetInstance()->GetId() == instanceId);
                entities.clear();
                rootSlice->GetEntities(entities);
                AZ_TEST_ASSERT(entities.back() == clone);
                AZ_TEST_ASSERT(entities.back()->FindComponent<MyTestComponent1>()->m_float == 10.0f);
                ancestors.clear();
                addr.GetReference()->GetInstanceEntityAncestry(clone->GetId(), ancestors, 1);
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

        sliceAssetHolder = nullptr; // release asset
        sliceAssetHolder1 = nullptr;

        AssetManager::Destroy();
    }

    /// a mock asset catalog which only contains two fixed assets.
    class SliceTest_RecursionDetection_Catalog
        : public AssetCatalog
        , public AssetCatalogRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(SliceTest_RecursionDetection_Catalog, AZ::SystemAllocator, 0);

        AZ::Uuid randomUuid = AZ::Uuid::CreateRandom();

        AssetId m_assetIdSlice1 = AssetId("{9DE6E611-CE12-4D78-90A5-53D106A50042}", 0);
        AssetId m_assetIdSlice2 = AssetId("{884C3DEE-3C86-4941-85E9-89103BAE314B}", 0);

        SliceTest_RecursionDetection_Catalog()
        {
            AssetCatalogRequestBus::Handler::BusConnect();
        }

        ~SliceTest_RecursionDetection_Catalog()
        {
            AssetCatalogRequestBus::Handler::BusDisconnect();
        }

        //////////////////////////////////////////////////////////////////////////
        // AssetCatalogRequestBus
        AssetInfo GetAssetInfoById(const AssetId& id) override
        {
            AssetInfo result;
            result.m_assetType = azrtti_typeid<SliceAsset>();
            if (id == m_assetIdSlice1)
            {
                result.m_assetId = id;
            }
            else if (id == m_assetIdSlice2)
            {
                result.m_assetId = id;
            }
            return result;
        }
        //////////////////////////////////////////////////////////////////////////

        AssetStreamInfo GetStreamInfoForLoad(const AssetId& id, const AssetType& type) override
        {
            EXPECT_TRUE(type == AzTypeInfo<SliceAsset>::Uuid());
            AssetStreamInfo info;
            info.m_dataOffset = 0;
            info.m_isCustomStreamType = false;
            info.m_streamFlags = IO::OpenMode::ModeRead;

            if (m_assetIdSlice1 == id)
            {
                info.m_streamName = "MySliceAsset1.txt";
            }
            else if (m_assetIdSlice2 == id)
            {
                info.m_streamName = "MySliceAsset2.txt";
            }

            if (!info.m_streamName.empty())
            {
                // this ensures tha paralell running unit tests do not overlap their files that they use.
                AZStd::string fullName = AZStd::string::format("%s%s-%s", GetTestFolderPath().c_str(), randomUuid.ToString<AZStd::string>().c_str(), info.m_streamName.c_str());
                info.m_streamName = fullName;
                info.m_dataLen = static_cast<size_t>(IO::SystemFile::Length(info.m_streamName.c_str()));
            }
            else
            {
                info.m_dataLen = 0;
            }

            return info;
        }

        AssetStreamInfo GetStreamInfoForSave(const AssetId& id, const AssetType& type) override
        {
            AssetStreamInfo info;
            info = GetStreamInfoForLoad(id, type);
            info.m_streamFlags = IO::OpenMode::ModeWrite;
            return info;
        }
    };

    class DataFlags_CleanupTest
        : public ScopedAllocatorSetupFixture
    {
    protected:
        void SetUp() override
        {
            auto allEntitiesValidFunction = [](EntityId) { return true; };
            m_dataFlags = AZStd::make_unique<SliceComponent::DataFlagsPerEntity>(allEntitiesValidFunction);
            m_addressOfSetFlag.push_back(AZ_CRC("Components"));
            m_valueOfSetFlag = DataPatch::Flag::ForceOverrideSet;
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

    TEST_F(SliceTest, PreventOverrideOfPropertyinEntityFromSlice_InstancedSlicesCantOverrideProperty)
    {
        ////////////////////////////////////////////////////////////////////////
        // Create a root slice and make it an asset
        Entity* rootSliceEntity = aznew Entity();
        SliceComponent* rootSliceComponent = rootSliceEntity->CreateComponent<SliceComponent>();
        EntityId entityId1InRootSlice;
        ComponentId componentId1InRootSlice;
        Data::Asset<SliceAsset> rootSliceAssetRef;
        {
            rootSliceComponent->SetSerializeContext(m_serializeContext);
            rootSliceEntity->Init();
            rootSliceEntity->Activate();

            // put 1 entity in the root slice
            Entity* entity1InRootSlice = aznew Entity();
            entityId1InRootSlice = entity1InRootSlice->GetId();
            MyTestComponent1* component1InRootSlice = entity1InRootSlice->CreateComponent<MyTestComponent1>();
            componentId1InRootSlice = component1InRootSlice->GetId();

            rootSliceComponent->AddEntity(entity1InRootSlice);

            // create a "fake" slice asset
            rootSliceAssetRef = Data::AssetManager::Instance().CreateAsset<SliceAsset>(m_catalog->GenerateMockAssetId());
            rootSliceAssetRef.Get()->SetData(rootSliceEntity, rootSliceComponent);
        }

        ////////////////////////////////////////////////////////////////////////
        // Create slice2, which contains an instance of the first slice
        // store slice2 in stream
        AZStd::vector<u8> slice2Buffer;
        IO::ByteContainerStream<AZStd::vector<u8>> slice2Stream(&slice2Buffer);
        {
            Entity* slice2Entity = aznew Entity();
            SliceComponent* slice2Component = slice2Entity->CreateComponent<SliceComponent>();
            slice2Component->SetSerializeContext(m_serializeContext);
            slice2Component->AddSlice(rootSliceAssetRef);

            SliceComponent::EntityList entitiesInSlice2;
            slice2Component->GetEntities(entitiesInSlice2); // this instantiates slice2 and the rootSlice

            // change a property
            entitiesInSlice2[0]->FindComponent<MyTestComponent1>()->m_int = 43;

            auto slice2ObjectStream = ObjectStream::Create(&slice2Stream, *m_serializeContext, ObjectStream::ST_BINARY);
            EXPECT_TRUE(slice2ObjectStream->WriteClass(slice2Entity));
            EXPECT_TRUE(slice2ObjectStream->Finalize());

            delete slice2Entity;
        }

        ////////////////////////////////////////////////////////////////////////
        // Re-spawn slice2, the property should still be overridden
        {
            slice2Stream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
            auto slice2Entity = Utils::LoadObjectFromStream<Entity>(slice2Stream, m_serializeContext);
            SliceComponent::EntityList entitiesInSlice2;
            slice2Entity->FindComponent<SliceComponent>()->GetEntities(entitiesInSlice2);

            EXPECT_EQ(43, entitiesInSlice2[0]->FindComponent<MyTestComponent1>()->m_int);
            delete slice2Entity;
        }

        ////////////////////////////////////////////////////////////////////////
        // Now modify the root slice to prevent override of the component's m_int value.
        {
            DataPatch::AddressType address;
            address.push_back(AZ_CRC("Components"));
            address.push_back(componentId1InRootSlice);
            address.push_back(AZ_CRC("int"));

            rootSliceComponent->SetEntityDataFlagsAtAddress(entityId1InRootSlice, address, DataPatch::Flag::PreventOverrideSet);

            // There are expectations that slice assets don't change after they're loaded,
            // so fake an "asset reload" by writing out the slice and reading it back in.
            AZStd::vector<char> rootSliceBuffer;
            IO::ByteContainerStream<AZStd::vector<char>> rootSliceStream(&rootSliceBuffer);
            auto rootSliceObjectStream = ObjectStream::Create(&rootSliceStream, *m_serializeContext, ObjectStream::ST_XML);
            EXPECT_TRUE(rootSliceObjectStream->WriteClass(rootSliceEntity));
            EXPECT_TRUE(rootSliceObjectStream->Finalize());

            rootSliceStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
            rootSliceEntity = Utils::LoadObjectFromStream<Entity>(rootSliceStream, m_serializeContext);
            rootSliceComponent = rootSliceEntity->FindComponent<SliceComponent>();
            rootSliceComponent->SetSerializeContext(m_serializeContext);
            rootSliceEntity->Init();
            rootSliceEntity->Activate();

            rootSliceAssetRef.Get()->SetData(rootSliceEntity, rootSliceComponent, true);
        }

        ////////////////////////////////////////////////////////////////////////
        // Re-spawn slice2, the property should NOT be overridden
        {
            slice2Stream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
            auto slice2Entity = Utils::LoadObjectFromStream<Entity>(slice2Stream, m_serializeContext);
            SliceComponent::EntityList entitiesInSlice2;
            slice2Entity->FindComponent<SliceComponent>()->GetEntities(entitiesInSlice2);

            EXPECT_NE(43, entitiesInSlice2[0]->FindComponent<MyTestComponent1>()->m_int);
            delete slice2Entity;
        }
    }
}

#ifdef HAVE_BENCHMARK
namespace Benchmark
{
    static void BM_Slice_GenerateNewIdsAndFixRefs(benchmark::State& state)
    {
        ComponentApplication componentApp;

        ComponentApplication::Descriptor desc;
        desc.m_useExistingAllocator = true;

        ComponentApplication::StartupParameters startupParams;
        startupParams.m_allocator = &AZ::AllocatorInstance<AZ::SystemAllocator>::Get();

        componentApp.Create(desc, startupParams);

        UnitTest::MyTestComponent1::Reflect(componentApp.GetSerializeContext());
        UnitTest::MyTestComponent2::Reflect(componentApp.GetSerializeContext());

        // we use some randomness to set up this scenario,
        // seed the generator so we get the same results each time.
        u32 randSeed[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
        AZ::Sfmt::GetInstance().Seed(randSeed, AZ_ARRAY_SIZE(randSeed));

        // setup a container with N entities
        AZ::SliceComponent::InstantiatedContainer container;
        for (int64_t entityI = 0; entityI < state.range(0); ++entityI)
        {
            auto entity = aznew AZ::Entity();

            // add components with nothing to remap, just to bulk up the volume of processed data
            entity->CreateComponent<UnitTest::MyTestComponent1>();
            entity->CreateComponent<UnitTest::MyTestComponent1>();
            entity->CreateComponent<UnitTest::MyTestComponent1>();

            // add component which references another EntityId
            auto component2 = entity->CreateComponent<UnitTest::MyTestComponent2>();
            if (entityI != 0)
            {
                component2->m_entityId = container.m_entities[AZ::Sfmt::GetInstance().Rand64() % container.m_entities.size()]->GetId();
            }

            container.m_entities.push_back(entity);
        }

        // shuffle entities so that some ID references are earlier in the data and some are later
        for (size_t i = container.m_entities.size() - 1; i > 0; --i)
        {
            AZStd::swap(container.m_entities[i], container.m_entities[AZ::Sfmt::GetInstance().Rand64() % (i+1)]);
        }

        AZStd::unordered_map<AZ::EntityId, AZ::EntityId> remappedIds;
        while (state.KeepRunning())
        {
            // Setup
            state.PauseTiming();
            AZ::SliceComponent::InstantiatedContainer* clonedContainer = componentApp.GetSerializeContext()->CloneObject(&container);
            state.ResumeTiming();

            // Timed Test
            AZ::IdUtils::Remapper<AZ::EntityId>::GenerateNewIdsAndFixRefs(clonedContainer, remappedIds, componentApp.GetSerializeContext());

            state.PauseTiming();
            delete clonedContainer;
            remappedIds.clear();
            state.ResumeTiming();
        }
    }

    BENCHMARK(BM_Slice_GenerateNewIdsAndFixRefs)->Arg(10)->Arg(1000);

} // namespace Benchmark
#endif // HAVE_BENCHMARK