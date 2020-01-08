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

#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Asset/AssetCatalog.h>
#include <AzFramework/Asset/AssetProcessorMessages.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/std/containers/vector.h>
#include <AzFramework/Application/Application.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include "AZTestShared/Utils/Utils.h"

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

    class AssetCatalogDeltaTest :
        public ::testing::Test
    {
    public:
        const char* path1 = "Asset1Path";
        const char* path2 = "Asset2Path";
        const char* path3 = "Asset3Path";
        const char* path4 = "Asset4Path";
        const char* path5 = "Asset5Path";
        const char* path6 = "Asset6Path";

        char sourceCatalogPath1[AZ_MAX_PATH_LEN];
        char sourceCatalogPath2[AZ_MAX_PATH_LEN];
        char sourceCatalogPath3[AZ_MAX_PATH_LEN];

        char baseCatalogPath[AZ_MAX_PATH_LEN];
        char deltaCatalogPath[AZ_MAX_PATH_LEN];
        char deltaCatalogPath2[AZ_MAX_PATH_LEN];
        char deltaCatalogPath3[AZ_MAX_PATH_LEN];

        AZStd::vector<AZStd::string> deltaCatalogFiles;
        AZStd::vector<AZStd::string> deltaCatalogFiles2;
        AZStd::vector<AZStd::string> deltaCatalogFiles3;

        AZStd::shared_ptr<AzFramework::AssetRegistry> baseCatalog;
        AZStd::shared_ptr<AzFramework::AssetRegistry> deltaCatalog;
        AZStd::shared_ptr<AzFramework::AssetRegistry> deltaCatalog2;
        AZStd::shared_ptr<AzFramework::AssetRegistry> deltaCatalog3;

        AZStd::unique_ptr<AzFramework::Application> m_app;

        void SetUp() override
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Create();

            m_app.reset(aznew AzFramework::Application());
            AZ::ComponentApplication::Descriptor desc;
            desc.m_useExistingAllocator = true;
            m_app->Start(desc);

            asset1 = AssetId(AZ::Uuid::CreateRandom(), 0);
            asset2 = AssetId(AZ::Uuid::CreateRandom(), 0);
            asset3 = AssetId(AZ::Uuid::CreateRandom(), 0);
            asset4 = AssetId(AZ::Uuid::CreateRandom(), 0);
            asset5 = AssetId(AZ::Uuid::CreateRandom(), 0);

            azstrcpy(sourceCatalogPath1, AZ_MAX_PATH_LEN,(GetTestFolderPath() + "AssetCatalogSource1.xml").c_str());
            azstrcpy(sourceCatalogPath2, AZ_MAX_PATH_LEN,(GetTestFolderPath() + "AssetCatalogSource2.xml").c_str());
            azstrcpy(sourceCatalogPath3, AZ_MAX_PATH_LEN,(GetTestFolderPath() + "AssetCatalogSource3.xml").c_str());

            azstrcpy(baseCatalogPath, AZ_MAX_PATH_LEN, (GetTestFolderPath() + "AssetCatalogBase.xml").c_str());
            azstrcpy(deltaCatalogPath, AZ_MAX_PATH_LEN, (GetTestFolderPath() + "AssetCatalogDelta.xml").c_str());
            azstrcpy(deltaCatalogPath2, AZ_MAX_PATH_LEN, (GetTestFolderPath() + "AssetCatalogDelta2.xml").c_str());
            azstrcpy(deltaCatalogPath3, AZ_MAX_PATH_LEN, (GetTestFolderPath() + "AssetCatalogDelta3.xml").c_str());


            AZ::Data::AssetInfo info1, info2, info3, info4, info5, info6;

            info1.m_relativePath = path1;
            info2.m_relativePath = path2;
            info3.m_relativePath = path3;
            info4.m_relativePath = path4;
            info5.m_relativePath = path5;
            info6.m_relativePath = path6;

            

            AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::RegisterAsset, asset1, info1);
            AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::RegisterAsset, asset2, info2);
            // baseCatalog - asset1 path1, asset2 path2
            AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::SaveCatalog, baseCatalogPath);


            AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::RegisterAsset, asset1, info3);
            AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::RegisterAsset, asset4, info4);
            AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::StartMonitoringAssets);
            {
                AzFramework::AssetSystem::AssetNotificationMessage message(path3, AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged, AZ::Uuid::CreateRandom());
                message.m_assetId = asset1;
                message.m_dependencies.push_back(AZ::Data::ProductDependency(asset2, 0));
                AzFramework::AssetSystemBus::Broadcast(&AzFramework::AssetSystemBus::Events::AssetChanged, message);
            }
            AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::StopMonitoringAssets);
            // sourcecatalog1 - asset1 path3 (depends on asset 2), asset2 path2, asset4 path4
            AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::SaveCatalog, sourceCatalogPath1);
            AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::UnregisterAsset, asset2);
            // deltacatalog - asset1 path3 (depends on asset 2), asset4 path4
            deltaCatalogFiles.push_back(path3);
            deltaCatalogFiles.push_back(path4);

            // deltacatalog - asset1 path3, asset4 path4
            AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::SaveCatalog, deltaCatalogPath);

            AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::RegisterAsset, asset2, info2);
            AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::RegisterAsset, asset5, info5);
            AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::StartMonitoringAssets);
            {
                AzFramework::AssetSystem::AssetNotificationMessage message(path5, AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged, AZ::Uuid::CreateRandom());
                message.m_assetId = asset5;
                message.m_dependencies.push_back(AZ::Data::ProductDependency(asset2, 0));
                AzFramework::AssetSystemBus::Broadcast(&AzFramework::AssetSystemBus::Events::AssetChanged, message);
            }
            AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::StopMonitoringAssets);
            // sourcecatalog2 - asset1 path3 (depends on asset 2), asset2 path2, asset4 path4, asset5 path5 (depends on asset 2)
            AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::SaveCatalog, sourceCatalogPath2);
            AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::UnregisterAsset, asset1);
            AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::UnregisterAsset, asset2);
            AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::UnregisterAsset, asset4);
            // deltacatalog2 - asset5 path5 (depends on asset 2)
            deltaCatalogFiles2.push_back(path5);
            AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::SaveCatalog, deltaCatalogPath2);
            

            AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::RegisterAsset, asset1, info6);
            AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::RegisterAsset, asset2, info2);
            AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::RegisterAsset, asset5, info4);
            AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::StartMonitoringAssets);
            {
                AzFramework::AssetSystem::AssetNotificationMessage message(path4, AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged, AZ::Uuid::CreateRandom());
                message.m_assetId = asset5;
                message.m_dependencies.push_back(AZ::Data::ProductDependency(asset2, 0));
                AzFramework::AssetSystemBus::Broadcast(&AzFramework::AssetSystemBus::Events::AssetChanged, message);
            }
            AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::StopMonitoringAssets);
            //sourcecatalog3 - asset1 path6, asset2 path2, asset5 path4 (depends on asset 2)
            AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::SaveCatalog, sourceCatalogPath3);
            AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::UnregisterAsset, asset2);
            // deltacatalog3 - asset1 path6 asset5 path4 (depends on asset 2)
            deltaCatalogFiles3.push_back(path6);
            deltaCatalogFiles3.push_back(path4);
            AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::SaveCatalog, deltaCatalogPath3);
            AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::UnregisterAsset, asset1);
            AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::UnregisterAsset, asset5);

            baseCatalog = AzFramework::AssetCatalog::LoadCatalogFromFile(baseCatalogPath);
            deltaCatalog = AzFramework::AssetCatalog::LoadCatalogFromFile(deltaCatalogPath);
            deltaCatalog2 = AzFramework::AssetCatalog::LoadCatalogFromFile(deltaCatalogPath2);
            deltaCatalog3 = AzFramework::AssetCatalog::LoadCatalogFromFile(deltaCatalogPath3);
        }

        void TearDown() override
        {
            deltaCatalogFiles.set_capacity(0);
            deltaCatalogFiles2.set_capacity(0);
            deltaCatalogFiles3.set_capacity(0);
            baseCatalog.reset();
            deltaCatalog.reset();
            deltaCatalog2.reset();
            deltaCatalog3.reset();
            m_app->Stop();
            m_app.reset();
            AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
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

        void CheckNoDependencies(AssetId assetId)
        {
            AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> result = AZ::Failure<AZStd::string>("No response");
            AssetCatalogRequestBus::BroadcastResult(result, &AssetCatalogRequestBus::Events::GetDirectProductDependencies, assetId);
            EXPECT_FALSE(result.IsSuccess());
        }

    };

    TEST_F(AssetCatalogDeltaTest, DeltaCatalogTest)
    {
        AZStd::string assetPath;
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset1);
        EXPECT_EQ(assetPath, "");

        AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::ClearCatalog);
        AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::LoadCatalog, baseCatalogPath);

        // Topmost should have precedence

        // baseCatalog path1 path2
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset1);
        EXPECT_EQ(assetPath, path1);
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset2);
        EXPECT_EQ(assetPath, path2);
        CheckNoDependencies(asset1);
        CheckNoDependencies(asset2);

        AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::AddDeltaCatalog, deltaCatalog);

        // deltacatalog path3                path4
        /// baseCatalog path1  path2
        ///             asset1 asset2 asset3 asset4 asset5
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset1);
        EXPECT_EQ(assetPath, path3);
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset2);
        EXPECT_EQ(assetPath, path2);
        CheckDirectDependencies(asset1, { asset2 });
        CheckNoDependencies(asset2);
        CheckNoDependencies(asset4);

        AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::AddDeltaCatalog, deltaCatalog2);

        // deltacatalog2                             path5
        //  deltacatalog path3                path4
        ///  baseCatalog path1  path2
        ///              asset1 asset2 asset3 asset4 asset5
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset1);
        EXPECT_EQ(assetPath, path3);
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset2);
        EXPECT_EQ(assetPath, path2);
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset5);
        EXPECT_EQ(assetPath, path5);
        CheckDirectDependencies(asset1, { asset2 });
        CheckNoDependencies(asset2);
        CheckNoDependencies(asset4);
        CheckDirectDependencies(asset5, { asset2 });

        AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::RemoveDeltaCatalog, deltaCatalog);

        // deltacatalog2                             path5
        ///  baseCatalog path1  path2
        ///              asset1 asset2 asset3 asset4 asset5
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset1);
        EXPECT_EQ(assetPath, path1);
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset3);
        EXPECT_EQ(assetPath, "");
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset2);
        EXPECT_EQ(assetPath, path2);
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset5);
        EXPECT_EQ(assetPath, path5);
        CheckNoDependencies(asset1);
        CheckNoDependencies(asset2);
        CheckDirectDependencies(asset5, { asset2 });

        AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::AddDeltaCatalog, deltaCatalog);

        //  deltacatalog path3                path4
        // deltacatalog2                             path5
        ///  baseCatalog path1  path2
        ///              asset1 asset2 asset3 asset4 asset5
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset1);
        EXPECT_EQ(assetPath, path3);
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset2);
        EXPECT_EQ(assetPath, path2);
        CheckDirectDependencies(asset1, { asset2 });
        CheckNoDependencies(asset2);
        CheckNoDependencies(asset4);
        CheckDirectDependencies(asset5, { asset2 });

        AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::InsertDeltaCatalog, deltaCatalog3, 0);

        //  deltacatalog path3                path4
        // deltacatalog2                             path5
        // deltacatalog3 path6                       path4
        ///  baseCatalog path1  path2
        ///              asset1 asset2 asset3 asset4 asset5
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset1);
        EXPECT_EQ(assetPath, path3);
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset5);
        EXPECT_EQ(assetPath, path5);
        CheckDirectDependencies(asset1, { asset2 });
        CheckNoDependencies(asset2);
        CheckNoDependencies(asset4);
        CheckDirectDependencies(asset5, { asset2 });

        AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::RemoveDeltaCatalog, deltaCatalog3);

        //  deltacatalog path3                path4
        // deltacatalog2                             path5
        ///  baseCatalog path1  path2
        ///              asset1 asset2 asset3 asset4 asset5
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset1);
        EXPECT_EQ(assetPath, path3);
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset5);
        EXPECT_EQ(assetPath, path5);
        CheckDirectDependencies(asset1, { asset2 });
        CheckNoDependencies(asset2);
        CheckNoDependencies(asset4);
        CheckDirectDependencies(asset5, { asset2 });

        AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::InsertDeltaCatalog, deltaCatalog3, 1);

        //  deltacatalog path3                path4
        // deltacatalog3 path6                       path4
        // deltacatalog2                             path5
        ///  baseCatalog path1  path2
        ///              asset1 asset2 asset3 asset4 asset5
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset1);
        EXPECT_EQ(assetPath, path3);
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset5);
        EXPECT_EQ(assetPath, path4);
        CheckDirectDependencies(asset1, { asset2 });
        CheckNoDependencies(asset2);
        CheckNoDependencies(asset4);
        CheckDirectDependencies(asset5, { asset2 });

        AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::RemoveDeltaCatalog, baseCatalog);
        AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::InsertDeltaCatalog, baseCatalog, 3);

        ///  baseCatalog path1  path2
        //  deltacatalog path3                path4
        // deltacatalog3 path6                       path4
        // deltacatalog2                             path5
        ///              asset1 asset2 asset3 asset4 asset5
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset1);
        EXPECT_EQ(assetPath, path1);
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset2);
        EXPECT_EQ(assetPath, path2);
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset3);
        EXPECT_EQ(assetPath, "");
        CheckNoDependencies(asset1);
        CheckNoDependencies(asset2);
        CheckNoDependencies(asset4);
        CheckDirectDependencies(asset5, { asset2 });
    }

    TEST_F(AssetCatalogDeltaTest, DeltaCatalogTest_AddDeltaCatalogNext_Success)
    {
        AZStd::string assetPath;
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset1);
        EXPECT_EQ(assetPath, "");

        AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::ClearCatalog);
        AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::LoadCatalog, baseCatalogPath);

        // Topmost should have precedence

        // baseCatalog path1 path2
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset1);
        EXPECT_EQ(assetPath, path1);
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset2);
        EXPECT_EQ(assetPath, path2);
        CheckNoDependencies(asset1);
        CheckNoDependencies(asset2);

        AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::AddDeltaCatalog, deltaCatalog);

        // deltacatalog path3                path4
        /// baseCatalog path1  path2
        ///             asset1 asset2 asset3 asset4 asset5
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset1);
        EXPECT_EQ(assetPath, path3);
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset2);
        EXPECT_EQ(assetPath, path2);
        CheckDirectDependencies(asset1, { asset2 });
        CheckNoDependencies(asset2);
        CheckNoDependencies(asset4);

        AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::AddDeltaCatalog, deltaCatalog2);

        // deltacatalog2                             path5
        //  deltacatalog path3                path4
        ///  baseCatalog path1  path2
        ///              asset1 asset2 asset3 asset4 asset5
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset1);
        EXPECT_EQ(assetPath, path3);
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset2);
        EXPECT_EQ(assetPath, path2);
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset5);
        EXPECT_EQ(assetPath, path5);
        CheckDirectDependencies(asset1, { asset2 });
        CheckNoDependencies(asset2);
        CheckNoDependencies(asset4);
        CheckDirectDependencies(asset5, { asset2 });

        AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::RemoveDeltaCatalog, deltaCatalog);

        // deltacatalog2                             path5
        ///  baseCatalog path1  path2
        ///              asset1 asset2 asset3 asset4 asset5
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset1);
        EXPECT_EQ(assetPath, path1);
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset3);
        EXPECT_EQ(assetPath, "");
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset2);
        EXPECT_EQ(assetPath, path2);
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset5);
        EXPECT_EQ(assetPath, path5);
        CheckNoDependencies(asset1);
        CheckNoDependencies(asset2);
        CheckDirectDependencies(asset5, { asset2 });

        AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::AddDeltaCatalog, deltaCatalog);

        //  deltacatalog path3                path4
        // deltacatalog2                             path5
        ///  baseCatalog path1  path2
        ///              asset1 asset2 asset3 asset4 asset5
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset1);
        EXPECT_EQ(assetPath, path3);
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset2);
        EXPECT_EQ(assetPath, path2);
        CheckDirectDependencies(asset1, { asset2 });
        CheckNoDependencies(asset2);
        CheckNoDependencies(asset4);
        CheckDirectDependencies(asset5, { asset2 });

        AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::InsertDeltaCatalogBefore, deltaCatalog3, deltaCatalog2);

        //  deltacatalog path3                path4
        // deltacatalog2                             path5
        // deltacatalog3 path6                       path4
        ///  baseCatalog path1  path2
        ///              asset1 asset2 asset3 asset4 asset5
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset1);
        EXPECT_EQ(assetPath, path3);
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset5);
        EXPECT_EQ(assetPath, path5);
        CheckDirectDependencies(asset1, { asset2 });
        CheckNoDependencies(asset2);
        CheckNoDependencies(asset4);
        CheckDirectDependencies(asset5, { asset2 });

        AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::RemoveDeltaCatalog, deltaCatalog3);

        //  deltacatalog path3                path4
        // deltacatalog2                             path5
        ///  baseCatalog path1  path2
        ///              asset1 asset2 asset3 asset4 asset5
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset1);
        EXPECT_EQ(assetPath, path3);
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset5);
        EXPECT_EQ(assetPath, path5);
        CheckDirectDependencies(asset1, { asset2 });
        CheckNoDependencies(asset2);
        CheckNoDependencies(asset4);
        CheckDirectDependencies(asset5, { asset2 });

        AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::InsertDeltaCatalogBefore, deltaCatalog3, deltaCatalog);

        //  deltacatalog path3                path4
        // deltacatalog3 path6                       path4
        // deltacatalog2                             path5
        ///  baseCatalog path1  path2
        ///              asset1 asset2 asset3 asset4 asset5
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset1);
        EXPECT_EQ(assetPath, path3);
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset5);
        EXPECT_EQ(assetPath, path4);
        CheckDirectDependencies(asset1, { asset2 });
        CheckNoDependencies(asset2);
        CheckNoDependencies(asset4);
        CheckDirectDependencies(asset5, { asset2 });

        AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::RemoveDeltaCatalog, baseCatalog);
        AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::AddDeltaCatalog, baseCatalog);

        ///  baseCatalog path1  path2
        //  deltacatalog path3                path4
        // deltacatalog3 path6                       path4
        // deltacatalog2                             path5
        ///              asset1 asset2 asset3 asset4 asset5
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset1);
        EXPECT_EQ(assetPath, path1);
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset2);
        EXPECT_EQ(assetPath, path2);
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset3);
        EXPECT_EQ(assetPath, "");
        CheckNoDependencies(asset1);
        CheckNoDependencies(asset2);
        CheckNoDependencies(asset4);
        CheckDirectDependencies(asset5, { asset2 });
    }
    TEST_F(AssetCatalogDeltaTest, DeltaCatalogCreationTest)
    {
        bool result = false;
        AZStd::string assetPath;        

        // load source catalog 1
        AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::ClearCatalog);
        AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::LoadCatalog, sourceCatalogPath1);
        // generate delta catalog 1
        AZStd::string testDeltaCatalogPath1 = GetTestFolderPath() + "TestAssetCatalogDelta.xml";
        AssetCatalogRequestBus::BroadcastResult(result, &AssetCatalogRequestBus::Events::CreateDeltaCatalog, deltaCatalogFiles, testDeltaCatalogPath1);
        EXPECT_TRUE(result);

        AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::ClearCatalog);
        AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::LoadCatalog, testDeltaCatalogPath1.c_str());
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset1);
        EXPECT_EQ(assetPath, path3);
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset4);
        EXPECT_EQ(assetPath, path4);
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset2);
        EXPECT_EQ(assetPath, "");
        // check against depenency info
        CheckDirectDependencies(asset1, { asset2 });


        // load source catalog 2
        AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::ClearCatalog);
        AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::LoadCatalog, sourceCatalogPath2);
        // generate delta catalog 3
        AZStd::string testDeltaCatalogPath2 = GetTestFolderPath() + "TestAssetCatalogDelta2.xml";
        AssetCatalogRequestBus::BroadcastResult(result, &AssetCatalogRequestBus::Events::CreateDeltaCatalog, deltaCatalogFiles2, testDeltaCatalogPath2);
        EXPECT_TRUE(result);

        AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::ClearCatalog);
        AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::LoadCatalog, testDeltaCatalogPath2.c_str());
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset5);
        EXPECT_EQ(assetPath, path5);
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset2);
        EXPECT_EQ(assetPath, "");
        // check against dependency info
        CheckDirectDependencies(asset5, { asset2 });


        // load source catalog 3
        AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::ClearCatalog);
        AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::LoadCatalog, sourceCatalogPath3);
        // generate delta catalog 3
        AZStd::string testDeltaCatalogPath3 = GetTestFolderPath() + "TestAssetCatalogDelta3.xml";
        AssetCatalogRequestBus::BroadcastResult(result, &AssetCatalogRequestBus::Events::CreateDeltaCatalog, deltaCatalogFiles3, testDeltaCatalogPath3);
        EXPECT_TRUE(result);

        AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::ClearCatalog);
        AssetCatalogRequestBus::Broadcast(&AssetCatalogRequestBus::Events::LoadCatalog, testDeltaCatalogPath3.c_str());
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset1);
        EXPECT_EQ(assetPath, path6);
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset5);
        EXPECT_EQ(assetPath, path4);
        AssetCatalogRequestBus::BroadcastResult(assetPath, &AssetCatalogRequestBus::Events::GetAssetPathById, asset2);
        EXPECT_EQ(assetPath, "");
        // check against dependency info
        CheckDirectDependencies(asset5, { asset2 });
        CheckNoDependencies(asset1);
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
            m_assetRoot = UnitTest::GetTestFolderPath();
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
        AzFramework::StringFunc::Path::Join(m_assetRoot.c_str(), "AssetA.txt", absPath, true);
        EXPECT_EQ(m_firstAssetId, m_assetCatalog->GetAssetIdByPath(absPath.c_str(), AZ::Data::s_invalidAssetType, false));
    }

    TEST_F(AssetCatalogAPITest, GetAssetPathById_AbsolutePathWithSubFolder_Valid)
    {
        AZStd::string absPath;
        AzFramework::StringFunc::Path::Join(m_assetRoot.c_str(), "Foo/AssetA.txt", absPath, true);
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

    TEST_F(AssetCatalogAPITest, GetRegisteredAssetPaths_TwoAssetsRegistered_RetrieveAssetPaths)
    {
        AZStd::vector<AZStd::string> assetPaths = m_assetCatalog->GetRegisteredAssetPaths();
        EXPECT_EQ(assetPaths.size(), 2);
        ASSERT_THAT(assetPaths, testing::ElementsAre(
            "AssetA.txt",
            "Foo/AssetA.txt"));
    }
}
