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

#include "LmbrCentral_precompiled.h"
#include <AzTest/AzTest.h>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <LmbrCentral/Rendering/MeshAsset.h>
#include <LmbrCentral/Rendering/MaterialAsset.h>
#include <Rendering/MaterialAssetHandler.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzCore/Memory/PoolAllocator.h>

#include <Mocks/ISystemMock.h>
#include <Mocks/I3DEngineMock.h>
#include <Mocks/IMaterialManagerMock.h>
#include <AzCore/UnitTest/Mocks/MockFileIOBase.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace AZ::Data;
    using namespace LmbrCentral;

    using ::testing::NiceMock;
    using ::testing::_;
    using ::testing::Return;

    // Create a Mock Asset Catalog so that we can "find" our material asset.
    class MaterialDataAssetTest_MockCatalog
        : public AssetCatalog
        , public AssetCatalogRequestBus::Handler
    {
    private:
        AZ::Uuid randomUuid = AZ::Uuid::CreateRandom();
        AZStd::vector<AssetId> m_mockAssetIds;

    public:
        AZ_CLASS_ALLOCATOR(MaterialDataAssetTest_MockCatalog, AZ::SystemAllocator, 0);

        MaterialDataAssetTest_MockCatalog()
        {
            AssetCatalogRequestBus::Handler::BusConnect();
        }

        ~MaterialDataAssetTest_MockCatalog()
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
            auto idIterator = AZStd::find(m_mockAssetIds.begin(), m_mockAssetIds.end(), id);
            if (idIterator != m_mockAssetIds.end())
            {
                // Only return the right type if we've requested an ID that we've generated.
                result.m_assetType = AZ::AzTypeInfo<MaterialDataAsset>::Uuid();
                result.m_assetId = id;
            }
            return result;
        }
        //////////////////////////////////////////////////////////////////////////

        AssetStreamInfo GetStreamInfoForLoad(const AssetId& id, const AssetType& type) override
        {
            EXPECT_TRUE(type == AzTypeInfo<MaterialDataAsset>::Uuid());
            AssetStreamInfo info;
            info.m_dataOffset = 0;
            info.m_streamFlags = IO::OpenMode::ModeRead;
            info.m_dataLen = 0;

            auto idIterator = AZStd::find(m_mockAssetIds.begin(), m_mockAssetIds.end(), id);
            if (idIterator != m_mockAssetIds.end())
            {
                // Only fill in mock material data if we've requested an ID that we've generated.
                info.m_streamName = "MockMaterialAssetName";
                info.m_dataLen = 123456;
                // It's important to set this flag so that we end up in the "char* assetPath" version of LoadAsset in MaterialAssetHandler.
                // (Materials are expected to get loaded via CryPak)
                info.m_isCustomStreamType = true;
            }

            return info;
        }
    };

    class MaterialDataAssetTest
        : public AllocatorsFixture
    {
    public:
        void SetUp() override
        {
            AllocatorsFixture::SetUp();

            AllocatorInstance<PoolAllocator>::Create();
            AllocatorInstance<ThreadPoolAllocator>::Create();

            // Save off any prior settings for the environments we're going to mock
            m_priorEnv = gEnv;
            m_priorFileIO = AZ::IO::FileIOBase::GetInstance();

            // Mock out enough systems that we can pretend to load a material.
            m_env = AZStd::make_unique<MockEnvironment>();
            m_env->m_stubEnv.pSystem = &m_env->m_system;
            gEnv = &m_env->m_stubEnv;

            // for FileIO, you must set the instance to null before changing it.
            // this is a way to tell the singleton system that you mean to replace a singleton and its
            // not a mistake.
            AZ::IO::FileIOBase::SetInstance(nullptr);
            AZ::IO::FileIOBase::SetInstance(&m_env->m_fileIOMock);
            AZ::IO::MockFileIOBase::InstallDefaultReturns(m_env->m_fileIOMock);

            // Create asset manager and asset database
            AssetManager::Descriptor desc;
            AssetManager::Create(desc);
            AssetManager::Instance().RegisterHandler(aznew MaterialAssetHandler(), AzTypeInfo<MaterialDataAsset>::Uuid());
            m_catalog.reset(aznew MaterialDataAssetTest_MockCatalog());
            AssetManager::Instance().RegisterCatalog(m_catalog.get(), AzTypeInfo<MaterialDataAsset>::Uuid());
        }

        void TearDown() override
        {
            m_catalog->DisableCatalog();
            m_catalog.reset();
            AssetManager::Destroy();

            AZ::IO::FileIOBase::SetInstance(nullptr);
            AZ::IO::FileIOBase::SetInstance(m_priorFileIO);

            m_env.reset();

            // restore state.
            gEnv = m_priorEnv;


            AllocatorInstance<PoolAllocator>::Destroy();
            AllocatorInstance<ThreadPoolAllocator>::Destroy();

            AllocatorsFixture::TearDown();
        }

        AZStd::unique_ptr<MaterialDataAssetTest_MockCatalog> m_catalog;

        struct MockEnvironment
        {
            SSystemGlobalEnvironment m_stubEnv;
            NiceMock<SystemMock> m_system;
            NiceMock<I3DEngineMock> m_3dEngineMock;
            NiceMock<IMaterialManagerMock> m_materialManagerMock;
            NiceMock<AZ::IO::MockFileIOBase> m_fileIOMock;
        };

        AZStd::unique_ptr<MockEnvironment> m_env;

        // This needs to exist longer than the lifetime of AssetManager so that our smart_ptr doesn't dereference an invalid internal pointer.
        NiceMock<IMaterialMock> m_mockMaterial;

        // Used to save off global settings that we're mocking out during the test.
        SSystemGlobalEnvironment* m_priorEnv = nullptr;
        ISystem* m_priorSystem = nullptr;
        AZ::IO::FileIOBase* m_priorFileIO = nullptr;
    };

    // Registration test - verify that MaterialAssetHandler successfully registered for the MaterialDataAsset type
    TEST_F(MaterialDataAssetTest, AssetTypeIsRegistered)
    {
        AZStd::vector<AssetType> assetTypes;
        AssetManager::Instance().GetHandledAssetTypes(m_catalog.get(), assetTypes);
        EXPECT_TRUE(assetTypes.size() == 1);
        EXPECT_TRUE(assetTypes[0] == AzTypeInfo<MaterialDataAsset>::Uuid());
    }

    // Basic instantiation test - make sure we can create an empty MaterialDataAsset via the MaterialAssetHandler.
    // The asset should be valid, but the IMaterial pointer should be null.
    TEST_F(MaterialDataAssetTest, CreateEmptyAsset)
    {
        Asset<MaterialDataAsset> materialAsset = AssetManager::Instance().CreateAsset<MaterialDataAsset>(m_catalog->GenerateMockAssetId());
        EXPECT_TRUE(materialAsset.Get());
        EXPECT_TRUE(materialAsset.GetAs<MaterialDataAsset>()->m_Material == nullptr);

        AssetInfo assetInfo;
        AssetCatalogRequestBus::BroadcastResult(assetInfo, &AssetCatalogRequestBus::Events::GetAssetInfoById, materialAsset.GetId());
        EXPECT_TRUE(assetInfo.m_assetId.IsValid());
        EXPECT_TRUE(assetInfo.m_assetType == AzTypeInfo<MaterialDataAsset>::Uuid());
    }

    // The whole point of the MaterialDataAsset is that it loads a material asset and instantiates it via the MaterialManager before
    // returning as a successful asset load.  This unit test attempts to "load" a material (via mocked methods), and validates that
    // the asset actually has an IMaterial instance that was provided through our mocked LoadMaterial() call.  (We'll assume that
    // the MaterialManager itself has been tested elsewehere)
    TEST_F(MaterialDataAssetTest, LoadMockMaterial)
    {
        _smart_ptr<IMaterial> dummyMaterialPtr(&m_mockMaterial);

        // Mock out the calls to "read" the material from disk
        EXPECT_CALL(m_env->m_fileIOMock, Seek(_, _, _)).WillRepeatedly(Return(AZ::IO::Result(AZ::IO::ResultCode::Success)));
        EXPECT_CALL(m_env->m_fileIOMock, Read(_, _, _, _, _)).WillRepeatedly(Return(AZ::IO::Result(AZ::IO::ResultCode::Success)));

        // Redirect GetISystem()->GetI3DEngine()->GetMaterialManager()->LoadMaterial() to return a dummy pointer that we can validate.
        EXPECT_CALL(m_env->m_system, GetI3DEngine()).WillRepeatedly(Return(&(m_env->m_3dEngineMock)));
        EXPECT_CALL(m_env->m_3dEngineMock, GetMaterialManager()).WillRepeatedly(Return(&(m_env->m_materialManagerMock)));
        EXPECT_CALL(m_env->m_materialManagerMock, LoadMaterial(_, _, _, _)).WillRepeatedly(Return(dummyMaterialPtr));

        // Create and "load" our material asset.
        const bool queueLoad = true;
        Asset<MaterialDataAsset> materialAsset;
        materialAsset.SetAutoLoadBehavior(AssetLoadBehavior::PreLoad);
        materialAsset.Create(m_catalog->GenerateMockAssetId(), queueLoad);

        while (materialAsset.GetStatus() == AssetData::AssetStatus::Loading)
        {
            AssetManager::Instance().DispatchEvents();
            AZStd::this_thread::yield();
        }

        // Verify that the asset believes it loaded successfully.
        ASSERT_TRUE(materialAsset && ((materialAsset.GetStatus() == AssetData::AssetStatus::ReadyPreNotify) || (materialAsset.GetStatus() == AssetData::AssetStatus::Ready)));
        // Verify that the asset actually got an instance of our mocked IMaterial.
        EXPECT_TRUE(materialAsset.GetAs<MaterialDataAsset>()->m_Material == dummyMaterialPtr);
    }

}