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

#include <AzTest/AzTest.h>
#include <PhysX/SystemComponentBus.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/IO/SystemFile.h>

namespace PhysX
{
    using namespace AZ::Data;

    class MaterialLibraryTest_MockCatalog
        : public AZ::Data::AssetCatalog
        , public AZ::Data::AssetCatalogRequestBus::Handler
    {

    private:
        AZ::Uuid m_randomUuid = AZ::Uuid::CreateRandom();
        AZStd::vector<AssetId> m_mockAssetIds;

    public:
        AZ_CLASS_ALLOCATOR(MaterialLibraryTest_MockCatalog, AZ::SystemAllocator, 0);

        MaterialLibraryTest_MockCatalog()
        {
            AssetCatalogRequestBus::Handler::BusConnect();
        }

        ~MaterialLibraryTest_MockCatalog()
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
            result.m_assetType = AZ::AzTypeInfo<Physics::MaterialLibraryAsset>::Uuid();
            auto foundId = AZStd::find(m_mockAssetIds.begin(), m_mockAssetIds.end(), id);
            if (foundId != m_mockAssetIds.end())
            {
                result.m_assetId = *foundId;
            }

            return result;
        }
        //////////////////////////////////////////////////////////////////////////

        AssetStreamInfo GetStreamInfoForLoad(const AssetId& id, const AssetType& type) override
        {
            EXPECT_TRUE(type == AZ::AzTypeInfo<Physics::MaterialLibraryAsset>::Uuid());
            AssetStreamInfo info;
            info.m_dataOffset = 0;
            info.m_isCustomStreamType = false;
            info.m_streamFlags = AZ::IO::OpenMode::ModeRead;

            for (int i = 0; i < m_mockAssetIds.size(); ++i)
            {
                if (m_mockAssetIds[i] == id)
                {
                    info.m_streamName = AZStd::string::format("MaterialLibraryAssetName%d", i);
                }
            }

            if (!info.m_streamName.empty())
            {
                // this ensures tha parallel running unit tests do not overlap their files that they use.
                AZStd::string fullName = AZStd::string::format("%s-%s", m_randomUuid.ToString<AZStd::string>().c_str(), info.m_streamName.c_str());
                info.m_streamName = fullName;
                info.m_dataLen = static_cast<size_t>(AZ::IO::SystemFile::Length(info.m_streamName.c_str()));
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
            info.m_streamFlags = AZ::IO::OpenMode::ModeWrite;
            return info;
        }

        bool SaveAsset(Asset<Physics::MaterialLibraryAsset>& asset)
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

    class DISABLED_PhysXMaterialLibraryTest
        : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            m_catalog = AZStd::make_unique<MaterialLibraryTest_MockCatalog>();
            AssetManager::Instance().RegisterCatalog(m_catalog.get(), AZ::AzTypeInfo<Physics::MaterialLibraryAsset>::Uuid());
        }

        void TearDown() override
        {
            AssetManager::Instance().UnregisterCatalog(m_catalog.get());
        }

        AZStd::unique_ptr<MaterialLibraryTest_MockCatalog> m_catalog;
    };
    
    TEST_F(DISABLED_PhysXMaterialLibraryTest, DISABLED_DefaultMaterialLibrary_CorrectMaterialLibraryIsInferred)
    {
        PhysX::Configuration globalConfig;
        PhysX::ConfigurationRequestBus::BroadcastResult(globalConfig, &PhysX::ConfigurationRequests::GetConfiguration);

        AZ::Data::AssetId dummyAssetId = AZ::Data::AssetId(AZ::Uuid::CreateName("DummyLibrary.physmaterial"));
        AZ::Data::Asset<Physics::MaterialLibraryAsset> dummyMaterialLibAsset = AZ::Data::AssetManager::Instance().GetAsset<Physics::MaterialLibraryAsset>(dummyAssetId);
        globalConfig.m_materialLibrary = dummyMaterialLibAsset;
        PhysX::ConfigurationRequestBus::Broadcast(&PhysX::ConfigurationRequests::SetConfiguration, globalConfig);

        // We must have now a default material library setup
        ASSERT_TRUE(globalConfig.m_materialLibrary.GetId().IsValid());

        AZ::Data::AssetId otherDummyAssetId = AZ::Data::AssetId(AZ::Uuid::CreateName("OtherDummyLibrary.physmaterial"));
        AZ::Data::Asset<Physics::MaterialLibraryAsset> otherDummyMaterialLibAsset = AZ::Data::AssetManager::Instance().GetAsset<Physics::MaterialLibraryAsset>(otherDummyAssetId);

        // Set selection's material library to a different one than default material library
        Physics::MaterialSelection selectionTest;
        selectionTest.SetMaterialLibrary(otherDummyAssetId);

        ASSERT_TRUE(selectionTest.GetMaterialLibraryAssetId().IsValid());
        ASSERT_EQ(selectionTest.GetMaterialLibraryAssetId(), selectionTest.GetMaterialLibraryAssetId());
        ASSERT_NE(selectionTest.GetMaterialLibraryAssetId(), globalConfig.m_materialLibrary.GetId());

        // By reseting the selection, now it should infer to the default material library set in the global configuration
        selectionTest.ResetToDefaultMaterialLibrary();

        ASSERT_TRUE(selectionTest.GetMaterialLibraryAssetId().IsValid());
        ASSERT_EQ(selectionTest.GetMaterialLibraryAssetId(), globalConfig.m_materialLibrary.GetId());

        // Release material library so we exit gracefully
        globalConfig.m_materialLibrary = {};
        PhysX::ConfigurationRequestBus::Broadcast(&PhysX::ConfigurationRequests::SetConfiguration, globalConfig);
    }
}