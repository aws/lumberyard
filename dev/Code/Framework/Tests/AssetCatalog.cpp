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

#include <Tests/TestTypes.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/Math/Uuid.h>
#include <AzFramework/Asset/AssetCatalog.h>
#include <AzFramework/Asset/AssetProcessorMessages.h>
#include <AzFramework/Application/Application.h>
#include <AzFramework/StringFunc/StringFunc.h>

using namespace AZStd;
using namespace AZ::Data;

namespace UnitTest
{
    namespace
    {
        AssetId asset1;
        AssetId asset2;
        AssetId asset3;
        AssetId asset4;
        AssetId asset5;

        bool Search(const AZStd::vector<AZ::Data::ProductDependency>& dependencies, const AZ::Data::AssetId& assetId)
        {
            for (const auto& dependency : dependencies)
            {
                if (dependency.m_assetId == assetId)
                {
                    return true;
                }
            }

            return false;
        }
    }

    class AssetCatalogDependencyTest
        : public AllocatorsFixture
    {
        AzFramework::AssetCatalog* m_assetCatalog;

    public:
        void SetUp() override
        {
            AllocatorsFixture::SetUp();
            AZ::AllocatorInstance<AZ::PoolAllocator>::Create();
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();

            AZ::Data::AssetManager::Descriptor desc;
            AZ::Data::AssetManager::Create(desc);

            m_assetCatalog = aznew AzFramework::AssetCatalog();
            m_assetCatalog->StartMonitoringAssets();

            asset1 = AssetId(AZ::Uuid::CreateRandom(), 0);
            asset2 = AssetId(AZ::Uuid::CreateRandom(), 0);
            asset3 = AssetId(AZ::Uuid::CreateRandom(), 0);
            asset4 = AssetId(AZ::Uuid::CreateRandom(), 0);
            asset5 = AssetId(AZ::Uuid::CreateRandom(), 0);

            // asset1 -> asset2 -> asset3 -> asset5
            //                 --> asset4

            {
                AzFramework::AssetSystem::AssetNotificationMessage message("test", AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged, AZ::Uuid::CreateRandom());
                message.m_assetId = asset1;
                message.m_dependencies.emplace_back(asset2, 0);
                AzFramework::AssetSystemBus::Broadcast(&AzFramework::AssetSystemBus::Events::AssetChanged, message);
            }

            {
                AzFramework::AssetSystem::AssetNotificationMessage message("test", AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged, AZ::Uuid::CreateRandom());
                message.m_assetId = asset2;
                message.m_dependencies.emplace_back(asset3, 0);
                message.m_dependencies.emplace_back(asset4, 0);
                AzFramework::AssetSystemBus::Broadcast(&AzFramework::AssetSystemBus::Events::AssetChanged, message);
            }

            {
                AzFramework::AssetSystem::AssetNotificationMessage message("test", AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged, AZ::Uuid::CreateRandom());
                message.m_assetId = asset3;
                message.m_dependencies.emplace_back(asset5, 0);
                AzFramework::AssetSystemBus::Broadcast(&AzFramework::AssetSystemBus::Events::AssetChanged, message);
            }

            {
                AzFramework::AssetSystem::AssetNotificationMessage message("test", AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged, AZ::Uuid::CreateRandom());
                message.m_assetId = asset4;
                AzFramework::AssetSystemBus::Broadcast(&AzFramework::AssetSystemBus::Events::AssetChanged, message);
            }

            {
                AzFramework::AssetSystem::AssetNotificationMessage message("test", AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged, AZ::Uuid::CreateRandom());
                message.m_assetId = asset5;
                AzFramework::AssetSystemBus::Broadcast(&AzFramework::AssetSystemBus::Events::AssetChanged, message);
            }
        }

        void TearDown() override
        {
            delete m_assetCatalog;

            AzFramework::LegacyAssetEventBus::ClearQueuedEvents();

            AssetManager::Destroy();

            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
            AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();

            AllocatorsFixture::TearDown();
        }

        void CheckDirectDependencies(AssetId assetId, AZStd::initializer_list<AssetId> expectedDependencies)
        {
            AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> result = AZ::Failure<AZStd::string>("No response");
            AssetCatalogRequestBus::BroadcastResult(result, &AssetCatalogRequestBus::Events::GetDirectProductDependencies, assetId);

            EXPECT_TRUE(result.IsSuccess());

            auto& actualDependencies = result.GetValue();

            EXPECT_TRUE(actualDependencies.size() == actualDependencies.size());

            for (const auto& dependency : expectedDependencies)
            {
                EXPECT_TRUE(Search(actualDependencies, dependency));
            }
        }

        void CheckAllDependencies(AssetId assetId, AZStd::initializer_list<AssetId> expectedDependencies)
        {
            AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> result = AZ::Failure<AZStd::string>("No response");
            AssetCatalogRequestBus::BroadcastResult(result, &AssetCatalogRequestBus::Events::GetAllProductDependencies, assetId);

            EXPECT_TRUE(result.IsSuccess());

            auto& actualDependencies = result.GetValue();

            EXPECT_TRUE(actualDependencies.size() == actualDependencies.size());

            for (const auto& dependency : expectedDependencies)
            {
                EXPECT_TRUE(Search(actualDependencies, dependency));
            }
        }
    };

    TEST_F(AssetCatalogDependencyTest, directDependencies)
    {
        CheckDirectDependencies(asset1, { asset2 });
        CheckDirectDependencies(asset2, { asset3, asset4 });
        CheckDirectDependencies(asset3, { asset5 });
        CheckDirectDependencies(asset4, { });
        CheckDirectDependencies(asset5, { });
    }

    TEST_F(AssetCatalogDependencyTest, allDependencies)
    {
        CheckAllDependencies(asset1, { asset2, asset3, asset4, asset5 });
        CheckAllDependencies(asset2, { asset3, asset4, asset5 });
        CheckAllDependencies(asset3, { asset5 });
        CheckAllDependencies(asset4, {});
        CheckAllDependencies(asset5, {});
    }

    TEST_F(AssetCatalogDependencyTest, unregisterTest)
    {
        AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::UnregisterAsset, asset4);
        CheckDirectDependencies(asset2, { asset3 });
        CheckAllDependencies(asset1, { asset2, asset3, asset5 });
    }

    class AssetCatalogAPITest
        : public AllocatorsFixture
    {
    public:
        void SetUp() override
        {
            using namespace AZ::Data;
            AllocatorsFixture::SetUp();
            m_application = new AzFramework::Application();
            m_assetRoot = "Z:\\Dummy";
            m_application->NormalizePathKeepCase(m_assetRoot);
            m_application->SetAssetRoot(m_assetRoot.c_str());
            m_assetCatalog = aznew AzFramework::AssetCatalog();

            m_firstAssetId = AssetId(AZ::Uuid::CreateRandom(), 0);
            AssetInfo assetInfo;
            assetInfo.m_assetId = m_firstAssetId;
            assetInfo.m_relativePath = "AssetA.txt";
            assetInfo.m_sizeBytes = 1; //any non zero value
            m_assetCatalog->RegisterAsset(m_firstAssetId, assetInfo);

            m_secondAssetId = AssetId(AZ::Uuid::CreateRandom(), 0);
            assetInfo.m_assetId = m_secondAssetId;
            assetInfo.m_relativePath = "Foo/AssetA.txt";
            assetInfo.m_sizeBytes = 1; //any non zero value
            m_assetCatalog->RegisterAsset(m_secondAssetId, assetInfo);
        }

        void TearDown() override
        {
            delete m_assetCatalog;
            delete m_application;
            AllocatorsFixture::TearDown();
        }

        AzFramework::Application* m_application;
        AzFramework::AssetCatalog* m_assetCatalog;
        AZ::Data::AssetId m_firstAssetId;
        AZ::Data::AssetId m_secondAssetId;
        AZStd::string m_assetRoot;
    };


    TEST_F(AssetCatalogAPITest, GetAssetPathById_AbsolutePath_Valid)
    {
        AZStd::string absPath;
        AzFramework::StringFunc::Path::ConstructFull(m_assetRoot.c_str(), "AssetA.txt", absPath, true);
        EXPECT_EQ(m_firstAssetId, m_assetCatalog->GetAssetIdByPath(absPath.c_str(), AZ::Data::s_invalidAssetType, false));
    }

    TEST_F(AssetCatalogAPITest, GetAssetPathById_AbsolutePathWithSubFolder_Valid)
    {
        AZStd::string absPath;
        AzFramework::StringFunc::Path::ConstructFull(m_assetRoot.c_str(), "Foo/AssetA.txt", absPath, true);
        EXPECT_EQ(m_secondAssetId, m_assetCatalog->GetAssetIdByPath(absPath.c_str(), AZ::Data::s_invalidAssetType, false));
    }

    TEST_F(AssetCatalogAPITest, GetAssetPathById_RelPath_Valid)
    {
        EXPECT_EQ(m_firstAssetId, m_assetCatalog->GetAssetIdByPath("AssetA.txt", AZ::Data::s_invalidAssetType, false));
    }

    TEST_F(AssetCatalogAPITest, GetAssetPathById_RelPathWithSubFolders_Valid)
    {
        EXPECT_EQ(m_secondAssetId, m_assetCatalog->GetAssetIdByPath("Foo/AssetA.txt", AZ::Data::s_invalidAssetType, false));
    }

    TEST_F(AssetCatalogAPITest, GetAssetPathById_RelPathWithSeparators_Valid)
    {
        EXPECT_EQ(m_firstAssetId, m_assetCatalog->GetAssetIdByPath("//AssetA.txt", AZ::Data::s_invalidAssetType, false));
    }
}
