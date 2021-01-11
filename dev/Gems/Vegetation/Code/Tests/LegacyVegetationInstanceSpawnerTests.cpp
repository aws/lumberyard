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
#include "Vegetation_precompiled.h"

#include "VegetationTest.h"
#include "VegetationMocks.h"

#include <Mocks/ISystemMock.h>
#include <Mocks/I3DEngineMock.h>
#include <Mocks/IMergedMeshesManagerMock.h>

#include <AzCore/Component/Entity.h>
#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Memory/PoolAllocator.h>

#include <Vegetation/EmptyInstanceSpawner.h>
#include <Vegetation/LegacyVegetationInstanceSpawner.h>
#include <Vegetation/Ebuses/DescriptorNotificationBus.h>

namespace UnitTest
{
    class MockRenderNode : public IRenderNode
    {
        public:
            void ReleaseNode(bool) override { }
            EERType GetRenderNodeType() override { return eERType_NotRenderNode; }
            const char* GetEntityClassName(void) const override { return "Mock"; }
            const char* GetName(void) const override { return "Mock"; }
            Vec3 GetPos(bool) const override { return Vec3(0.0f); }
            void Render(const SRendParams&, const SRenderingPassInfo& passInfo) override {}
            void SetMaterial(_smart_ptr<IMaterial> pMat) override {}
            _smart_ptr<IMaterial> GetMaterial(Vec3* pHitPos = NULL) override { return nullptr; }
            _smart_ptr<IMaterial> GetMaterialOverride() override { return nullptr; }
            float GetMaxViewDist() override { return 0.0f; }
            void GetMemoryUsage(ICrySizer* pSizer) const override {}
            const AABB GetBBox() const override { return AABB(Vec3(0.0f), Vec3(0.0f)); }
            void SetBBox(const AABB& WSBBox) override { }
            void FillBBox(AABB& aabb) override {}
            void OffsetPosition(const Vec3& delta) override {}
    };

    // Mock VegetationSystemComponent is needed to reflect only the LegacyVegetationInstanceSpawner.
    class MockLegacyVegetationInstanceVegetationSystemComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(MockLegacyVegetationInstanceVegetationSystemComponent, "{DEE0C740-40E1-49F7-84DB-4603FBAD8A87}", AZ::Component);

        void Activate() override {}
        void Deactivate() override {}

        static void Reflect(AZ::ReflectContext* reflect)
        {
            Vegetation::InstanceSpawner::Reflect(reflect);
            Vegetation::LegacyVegetationInstanceSpawner::Reflect(reflect);

            // We also register EmptyInstanceSpawner so that we can test the Descriptor "type changed" methods
            // appropriately.  Since the default for Descriptor is to create a LegacyVegetationInstanceSpawner,
            // we need to change it to something else and back again to truly test the "type changed" method.
            Vegetation::EmptyInstanceSpawner::Reflect(reflect);
        }
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("VegetationSystemService", 0xa2322728));
        }
    };

    // To test Legacy Vegetation spawning, we need to mock up enough of the asset management system and the mesh
    // asset handling to pretend like we're loading/unloading meshes successfully.
    class LegacyVegetationInstanceSpawnerTests
        : public VegetationComponentTests
        , public Vegetation::DescriptorNotificationBus::Handler
        , public StatInstGroupEventBus::Handler
        , public AZ::Data::AssetCatalogRequestBus::Handler
        , public AZ::Data::AssetHandler
        , public AZ::Data::AssetCatalog
    {
    public:
        void RegisterComponentDescriptors() override
        {
            m_app.RegisterComponentDescriptor(MockLegacyVegetationInstanceVegetationSystemComponent::CreateDescriptor());
        }

        void SetUp() override
        {
            VegetationComponentTests::SetUp();

            // capture prior state.
            m_savedEnv = gEnv;

            // override state with our needed test mocks
            m_data = AZStd::make_unique<DataMembers>();
            gEnv = &(m_data->m_mockGEnv);
            m_data->m_mockGEnv.p3DEngine = &(m_data->m_mock3DEngine);
            m_data->m_mockGEnv.pSystem = &(m_data->m_mockSystem);

            // Reroute all the mocked calls as necessary
            EXPECT_CALL(m_data->m_mockSystem, GetI3DEngine()).WillRepeatedly(testing::Return(&(m_data->m_mock3DEngine)));
            EXPECT_CALL(m_data->m_mock3DEngine, GetIMergedMeshesManager()).WillRepeatedly(testing::Return(&(m_data->m_mockMergedMeshesManager)));
            EXPECT_CALL(m_data->m_mock3DEngine, SetStatInstGroup(testing::_, testing::_, testing::_)).WillRepeatedly(testing::Return(true));
            EXPECT_CALL(m_data->m_mockMergedMeshesManager, AddDynamicInstance(testing::_, testing::_, testing::_)).WillRepeatedly(testing::Return(&(m_data->m_mockRenderNode)));

            // Create a real Asset Mananger, and point to ourselves as the handler for MeshAsset.
            AZ::AllocatorInstance<AZ::PoolAllocator>::Create();
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();
            AZ::Data::AssetManager::Descriptor descriptor;
            descriptor.m_maxWorkerThreads = 1;
            AZ::Data::AssetManager::Create(descriptor);
            AZ::Data::AssetManager::Instance().RegisterHandler(this, AZ::AzTypeInfo<LmbrCentral::MeshAsset>::Uuid());
            AZ::Data::AssetManager::Instance().RegisterCatalog(this, AZ::AzTypeInfo<LmbrCentral::MeshAsset>::Uuid());

            // Intercept messages for finding assets by name and creating/destroying slices.
            AZ::Data::AssetCatalogRequestBus::Handler::BusConnect();

            // Intercept messages for generating / releasing stat group IDs
            StatInstGroupEventBus::Handler::BusConnect();
        }

        void TearDown() override
        {
            StatInstGroupEventBus::Handler::BusDisconnect();

            AZ::Data::AssetCatalogRequestBus::Handler::BusDisconnect();

            AZ::Data::AssetManager::Instance().UnregisterCatalog(this);
            AZ::Data::AssetManager::Instance().UnregisterHandler(this);

            AZ::Data::AssetManager::Destroy();
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
            AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();

            // Remove the other mocked systems
            m_data.reset();

            // restore the global state that we modified during the test.
            gEnv = m_savedEnv;

            VegetationComponentTests::TearDown();
        }

        // Helper methods:

        // Set up a mock asset with the given name and id and direct the instance spawner to use it.
        void CreateAndSetMockAsset(Vegetation::LegacyVegetationInstanceSpawner& instanceSpawner, AZ::Data::AssetId assetId, AZStd::string assetPath)
        {
            // Save these off for use from our mock AssetCatalogRequestBus
            m_assetId = assetId;
            m_assetPath = assetPath;

            // Mark our mock asset with the correct id and call it ready.
            reinterpret_cast<MockAssetData*>(&m_meshAsset)->SetId(m_assetId);

            Vegetation::DescriptorNotificationBus::Handler::BusConnect(&instanceSpawner);

            // Tell the spawner to use this asset.  This will trigger a LoadAssets() call inside of it.
            instanceSpawner.SetMeshAssetPath(m_assetPath);

            // Our instance spawner should now have a valid asset reference,
            // but not be loaded or spawnable yet.
            EXPECT_FALSE(instanceSpawner.HasEmptyAssetReferences());
            EXPECT_FALSE(instanceSpawner.IsLoaded());
            EXPECT_FALSE(instanceSpawner.IsSpawnable());

            // Since this is going through the real AssetManager, there's a delay while a separate
            // job thread executes and actually loads our mock mesh asset.
            // If our asset hasn't loaded successfully after 5 seconds, it's unlikely to succeed.
            // This choice of delay should be *reasonably* safe because it's all CPU-based processing,
            // no actual I/O occurs as a part of the test.
            constexpr int sleepMs = 10;
            constexpr int totalWaitTimeMs = 5000;
            int numRetries = totalWaitTimeMs / sleepMs;
            while ((m_numOnLoadedCalls < 1) && (numRetries >= 0))
            {
                AZ::Data::AssetManager::Instance().DispatchEvents();
                AZ::SystemTickBus::Broadcast(&AZ::SystemTickBus::Events::OnSystemTick);
                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(sleepMs));
                numRetries--;
            }

            ASSERT_TRUE(m_numOnLoadedCalls == 1);
            EXPECT_TRUE(instanceSpawner.IsLoaded());
            EXPECT_TRUE(instanceSpawner.IsSpawnable());

            Vegetation::DescriptorNotificationBus::Handler::BusDisconnect();
        }

        // AssetHandler
        // Minimalist mocks to look like a Mesh has been created/loaded/destroyed successfully
        AZ::Data::AssetPtr CreateAsset(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type) override
        {
            auto assetPtr = new LmbrCentral::MeshAsset();
            assetPtr->m_statObj = new MockStatObj();
            return assetPtr;
        }
        void DestroyAsset(AZ::Data::AssetPtr ptr) override { delete ptr; }

        void GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes) override
        {
            assetTypes.push_back(AZ::AzTypeInfo<LmbrCentral::MeshAsset>::Uuid());
        }
        bool LoadAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, const char* assetPath, const AZ::Data::AssetFilterCB& assetLoadFilterCB)
        {
            return true;
        }

        // DescriptorNotificationBus
        // Keep track of whether or not the Spawner successfully loaded the asset and notified listeners
        void OnDescriptorAssetsLoaded() override { m_numOnLoadedCalls++; }

        // StatInstGroupEventBus
        // Minimalist mocks to generate a stat group id for our mock vegetation object
        StatInstGroupId GenerateStatInstGroupId() override { return 1; }
        void ReleaseStatInstGroupId(StatInstGroupId statInstGroupId) override {}
        void ReleaseStatInstGroupIdSet(const AZStd::unordered_set<StatInstGroupId>& statInstGroupIdSet) override {}
        void ReserveStatInstGroupIdRange(StatInstGroupId from, StatInstGroupId to) override {}

        // AssetCatalogRequestBus
        // Minimalist mocks to provide our desired asset path or asset id
        AZStd::string GetAssetPathById(const AZ::Data::AssetId& /*id*/) override { return m_assetPath; }
        AZ::Data::AssetId GetAssetIdByPath(const char* /*path*/, const AZ::Data::AssetType& /*typeToRegister*/, bool /*autoRegisterIfNotFound*/) override { return m_assetId; }
        AZ::Data::AssetInfo GetAssetInfoById(const AZ::Data::AssetId& /*id*/) override
        {
            AZ::Data::AssetInfo assetInfo;
            assetInfo.m_assetId = m_assetId;
            assetInfo.m_assetType = AZ::AzTypeInfo<LmbrCentral::MeshAsset>::Uuid();
            assetInfo.m_relativePath = m_assetPath;
            return assetInfo;
        }

        // AssetCatalog
        // Minimalist mock to pretend like we've loaded a Mesh asset
        AZ::Data::AssetStreamInfo GetStreamInfoForLoad(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type) override
        {
            EXPECT_TRUE(type == AZ::AzTypeInfo<LmbrCentral::MeshAsset>::Uuid());
            AZ::Data::AssetStreamInfo info;
            info.m_dataOffset = 0;
            info.m_streamFlags = AZ::IO::OpenMode::ModeRead;
            info.m_streamName = m_assetPath;
            info.m_dataLen = 0;
            // By setting this to true, we call our custom LoadAssetData() above instead of using actual File IO
            info.m_isCustomStreamType = true;

            return info;
        }


        AZStd::string m_assetPath;
        AZ::Data::AssetId m_assetId;
        LmbrCentral::MeshAsset m_meshAsset;
        int m_numOnLoadedCalls = 0;

    private:
        struct DataMembers
        {
            SSystemGlobalEnvironment m_mockGEnv;
            ::testing::NiceMock<I3DEngineMock> m_mock3DEngine;
            ::testing::NiceMock<SystemMock> m_mockSystem;
            ::testing::NiceMock<MergedMeshesManagerMock> m_mockMergedMeshesManager;
            MockRenderNode m_mockRenderNode;
        };

        AZStd::unique_ptr<DataMembers> m_data;
        SSystemGlobalEnvironment* m_savedEnv = nullptr;
    };

    TEST_F(LegacyVegetationInstanceSpawnerTests, BasicInitializationTest)
    {
        // Basic test to make sure we can construct / destroy without errors.

        Vegetation::LegacyVegetationInstanceSpawner instanceSpawner;
    }

    TEST_F(LegacyVegetationInstanceSpawnerTests, DefaultSpawnersAreEqual)
    {
        // Two different instances of the default LegacyVegetationInstanceSpawner should
        // be considered data-equivalent.

        Vegetation::LegacyVegetationInstanceSpawner instanceSpawner1;
        Vegetation::LegacyVegetationInstanceSpawner instanceSpawner2;

        EXPECT_TRUE(instanceSpawner1 == instanceSpawner2);
    }

    // [LY-118267] Any test using CreateAndSetMockAsset can intermittently fail, so disabling them for now.
    TEST_F(LegacyVegetationInstanceSpawnerTests, DISABLED_DifferentSpawnersAreNotEqual)
    {
        // Two spawners with different data should *not* be data-equivalent.

        Vegetation::LegacyVegetationInstanceSpawner instanceSpawner1;
        Vegetation::LegacyVegetationInstanceSpawner instanceSpawner2;

        // Give the second instance spawner a non-default asset reference.
        CreateAndSetMockAsset(instanceSpawner2, AZ::Uuid::CreateRandom(), "test");

        // The test is written this way because only the == operator is overloaded.
        EXPECT_TRUE(!(instanceSpawner1 == instanceSpawner2));
    }

    // [LY-118267] Any test using CreateAndSetMockAsset can intermittently fail, so disabling them for now.
    TEST_F(LegacyVegetationInstanceSpawnerTests, DISABLED_LoadAndUnloadAssets)
    {
        // The spawner should successfully load/unload assets without errors.

        Vegetation::LegacyVegetationInstanceSpawner instanceSpawner;

        // Our instance spawner should be empty before we set the assets.
        EXPECT_TRUE(instanceSpawner.HasEmptyAssetReferences());

        // This will create and load an asset.
        CreateAndSetMockAsset(instanceSpawner, AZ::Uuid::CreateRandom(), "test");

        // Unload, then it should stop being loaded and spawnable.
        Vegetation::DescriptorNotificationBus::Handler::BusConnect(&instanceSpawner);
        instanceSpawner.UnloadAssets();
        EXPECT_FALSE(instanceSpawner.IsLoaded());
        EXPECT_FALSE(instanceSpawner.IsSpawnable());
        Vegetation::DescriptorNotificationBus::Handler::BusDisconnect();
    }

    // [LY-118267] This test intermittently fails (potentially only with uber builds) with the following backrtrace:
    //             dev\code\cryengine\crycommon\Cry_Vector3.h(124): error: this->IsValid()
    //  Temporarily disabling this test until the root cause can be found.
    TEST_F(LegacyVegetationInstanceSpawnerTests, DISABLED_CreateAndDestroyInstance)
    {
        // The spawner should successfully create and destroy an instance without errors.

        Vegetation::LegacyVegetationInstanceSpawner instanceSpawner;
        Vegetation::DescriptorNotificationBus::Handler::BusConnect(&instanceSpawner);

        CreateAndSetMockAsset(instanceSpawner, AZ::Uuid::CreateRandom(), "test");

        instanceSpawner.OnRegisterUniqueDescriptor();

        Vegetation::InstanceData instanceData;
        Vegetation::InstancePtr instance = instanceSpawner.CreateInstance(instanceData);
        EXPECT_TRUE(instance);
        instanceSpawner.DestroyInstance(0, instance);

        instanceSpawner.OnReleaseUniqueDescriptor();

        instanceSpawner.UnloadAssets();

        Vegetation::DescriptorNotificationBus::Handler::BusDisconnect();
    }

    TEST_F(LegacyVegetationInstanceSpawnerTests, SpawnerRegisteredWithDescriptor)
    {
        // Validate that the Descriptor successfully gets LegacyVegetationInstanceSpawner registered with it,
        // as long as InstanceSpawner and LegacyVegetationInstanceSpawner have been reflected.

        MockLegacyVegetationInstanceVegetationSystemComponent* component = nullptr;
        auto entity = CreateEntity(&component);

        Vegetation::Descriptor descriptor;
        descriptor.RefreshSpawnerTypeList();
        auto spawnerTypes = descriptor.GetSpawnerTypeList();

        // Our unit test harness reflects both Empty and Legacy Vegetation spawners.
        // They should come back in a deterministic alpha-sorted order.
        EXPECT_TRUE(spawnerTypes.size() == 2);
        EXPECT_TRUE(spawnerTypes[0].first == Vegetation::EmptyInstanceSpawner::RTTI_Type());
        EXPECT_TRUE(spawnerTypes[1].first == Vegetation::LegacyVegetationInstanceSpawner::RTTI_Type());
    }

    TEST_F(LegacyVegetationInstanceSpawnerTests, DescriptorCreatesCorrectSpawner)
    {
        // Validate that the Descriptor successfully creates a new LegacyVegetationInstanceSpawner as the default
        // and when we change the spawner type on the Descriptor.

        MockLegacyVegetationInstanceVegetationSystemComponent* component = nullptr;
        auto entity = CreateEntity(&component);

        // We expect the Descriptor to start off with a Legacy Vegetation spawner, so verify it created one initially.
        Vegetation::Descriptor descriptor;
        EXPECT_TRUE(azrtti_typeid(*(descriptor.GetInstanceSpawner())) == Vegetation::LegacyVegetationInstanceSpawner::RTTI_Type());
        descriptor.RefreshSpawnerTypeList();

        // Change it to Empty spawner to "cleanse the palette"
        descriptor.m_spawnerType = Vegetation::EmptyInstanceSpawner::RTTI_Type();
        descriptor.SpawnerTypeChanged();
        EXPECT_TRUE(azrtti_typeid(*(descriptor.GetInstanceSpawner())) == Vegetation::EmptyInstanceSpawner::RTTI_Type());

        // Change it back to Legacy Vegetation to verify that changing the type successfully creates one.
        descriptor.m_spawnerType = Vegetation::LegacyVegetationInstanceSpawner::RTTI_Type();
        descriptor.SpawnerTypeChanged();
        EXPECT_TRUE(azrtti_typeid(*(descriptor.GetInstanceSpawner())) == Vegetation::LegacyVegetationInstanceSpawner::RTTI_Type());
    }
}
