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

#include <AzCore/UnitTest/TestTypes.h>
#include <AzToolsFramework/AssetCatalog/PlatformAddressedAssetCatalogManager.h>
#include <AzFramework/Asset/AssetRegistry.h>
#include <AzCore/IO/FileIO.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzTestShared/Utils/Utils.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzFramework/Platform/PlatformDefaults.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <QTemporaryDir>
#include <QDir>

namespace 
{
    static const int s_totalAssets = 12;
}

namespace UnitTest
{
    class PlatformAddressedAssetCatalogManagerTest
        : public AllocatorsFixture
    {
    public:

        AZStd::string GetTempFolder()
        {
            QTemporaryDir dir;
            QDir tempPath(dir.path());
            return tempPath.absolutePath().toUtf8().data();
        }

        void SetUp() override
        {
            using namespace AZ::Data;
            m_application = new AzToolsFramework::ToolsApplication();
            m_application->Start(AzFramework::Application::Descriptor());

            AZStd::string cacheFolder;
            AzFramework::StringFunc::Path::Join(GetTempFolder().c_str(), "testplatform", cacheFolder);
            AzFramework::StringFunc::Path::Join(cacheFolder.c_str(), "testproject", cacheFolder);
           
            AZ::IO::FileIOBase::GetInstance()->SetAlias("@assets@", cacheFolder.c_str());

            for (int platformNum = AzFramework::PlatformId::PC; platformNum < AzFramework::PlatformId::NumPlatforms; ++platformNum)
            {
                AZStd::string platformName{ AzFramework::PlatformHelper::GetPlatformName(static_cast<AzFramework::PlatformId>(platformNum)) };
                if (!platformName.length())
                {
                    // Do not test disabled platforms
                    continue;
                }
                AZStd::unique_ptr<AzFramework::AssetRegistry> assetRegistry = AZStd::make_unique<AzFramework::AssetRegistry>();
                for (int idx = 0; idx < s_totalAssets; idx++)
                {
                    m_assets[platformNum][idx] = AssetId(AZ::Uuid::CreateRandom(), 0);
                    AZ::Data::AssetInfo info;
                    info.m_relativePath = AZStd::string::format("%s%sAsset%d_%s.txt", cacheFolder.c_str(), AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING, idx, platformName.c_str());
                    info.m_assetId = m_assets[platformNum][idx];
                    assetRegistry->RegisterAsset(m_assets[platformNum][idx], info);
                    m_assetsPath[platformNum][idx] = info.m_relativePath;
                    if (m_fileStreams[platformNum][idx].Open(m_assetsPath[platformNum][idx].c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeBinary | AZ::IO::OpenMode::ModeCreatePath))
                    {
                        m_fileStreams[platformNum][idx].Write(info.m_relativePath.size(), info.m_relativePath.data());
                    }
                    else
                    {
                        GTEST_FATAL_FAILURE_(AZStd::string::format("Unable to create temporary file ( %s ) in PlatformAddressedAssetCatalogManagerTest unit tests.\n", m_assetsPath[platformNum][idx].c_str()).c_str());
                    }
                }

                bool useRequestBus = false;
                AzFramework::AssetCatalog assetCatalog(useRequestBus);
                AZStd::string catalogPath = AzToolsFramework::PlatformAddressedAssetCatalog::GetCatalogRegistryPathForPlatform(static_cast<AzFramework::PlatformId>(platformNum));

                if (!assetCatalog.SaveCatalog(catalogPath.c_str(), assetRegistry.get()))
                {
                    GTEST_FATAL_FAILURE_(AZStd::string::format("Unable to save the asset catalog file.\n").c_str());
                }
            }


            m_PlatformAddressedAssetCatalogManager = new AzToolsFramework::PlatformAddressedAssetCatalogManager();
        }

        void TearDown() override
        {
            AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            for (int platformNum = AzFramework::PlatformId::PC; platformNum < AzFramework::PlatformId::NumPlatforms; ++platformNum)
            {
                AZStd::string platformName{ AzFramework::PlatformHelper::GetPlatformName(static_cast<AzFramework::PlatformId>(platformNum)) };
                if (!platformName.length())
                {
                    // Do not test disabled platforms
                    continue;
                }
                AZStd::string catalogPath = AzToolsFramework::PlatformAddressedAssetCatalog::GetCatalogRegistryPathForPlatform(static_cast<AzFramework::PlatformId>(platformNum));

                if (fileIO->Exists(catalogPath.c_str()))
                {
                    fileIO->Remove(catalogPath.c_str());
                }
                // Deleting all the temporary files
                for (int idx = 0; idx < s_totalAssets; idx++)
                {
                    // we need to close the handle before we try to remove the file
                    m_fileStreams[platformNum][idx].Close();
                    if (fileIO->Exists(m_assetsPath[platformNum][idx].c_str()))
                    {
                        fileIO->Remove(m_assetsPath[platformNum][idx].c_str());
                    }
                }
            }

            delete m_localFileIO;
            m_localFileIO = nullptr;
            AZ::IO::FileIOBase::SetInstance(m_priorFileIO);
            delete m_PlatformAddressedAssetCatalogManager;
            m_application->Stop();
            delete m_application;
        }


        AzToolsFramework::PlatformAddressedAssetCatalogManager* m_PlatformAddressedAssetCatalogManager;
        AzToolsFramework::ToolsApplication* m_application;
        AZ::IO::FileIOBase* m_priorFileIO = nullptr;
        AZ::IO::FileIOBase* m_localFileIO = nullptr;
        AZ::IO::FileIOStream m_fileStreams[AzFramework::PlatformId::NumPlatforms][s_totalAssets];

        AZ::Data::AssetId m_assets[AzFramework::PlatformId::NumPlatforms][s_totalAssets];
        AZStd::string m_assetsPath[AzFramework::PlatformId::NumPlatforms][s_totalAssets];
    };

    TEST_F(PlatformAddressedAssetCatalogManagerTest, PlatformAddressedAssetCatalogManager_AllCatalogsLoaded_Success)
    {
        for (int platformNum = AzFramework::PlatformId::PC; platformNum < AzFramework::PlatformId::NumPlatforms; ++platformNum)
        {
            AZStd::string platformName{ AzFramework::PlatformHelper::GetPlatformName(static_cast<AzFramework::PlatformId>(platformNum)) };
            if (!platformName.length())
            {
                // Do not test disabled platforms
                continue;
            }
            for (int assetNum = 0; assetNum < s_totalAssets; ++assetNum)
            {

                AZ::Data::AssetInfo assetInfo;
                AzToolsFramework::AssetCatalog::PlatformAddressedAssetCatalogRequestBus::EventResult(assetInfo, static_cast<AzFramework::PlatformId>(platformNum), &AZ::Data::AssetCatalogRequests::GetAssetInfoById, m_assets[platformNum][assetNum]);

                EXPECT_EQ(m_assets[platformNum][assetNum], assetInfo.m_assetId);
            }
        }
    }

    TEST_F(PlatformAddressedAssetCatalogManagerTest, PlatformAddressedAssetCatalogManager_CatalogExistsChecks_Success)
    {

        EXPECT_EQ(AzToolsFramework::PlatformAddressedAssetCatalog::CatalogExists(AzFramework::PlatformId::ES3), true);
        AZStd::string es3CatalogPath = AzToolsFramework::PlatformAddressedAssetCatalog::GetCatalogRegistryPathForPlatform(AzFramework::PlatformId::ES3);
        if (AZ::IO::FileIOBase::GetInstance()->Exists(es3CatalogPath.c_str()))
        {
            AZ::IO::FileIOBase::GetInstance()->Remove(es3CatalogPath.c_str());
        }
        EXPECT_EQ(AzToolsFramework::PlatformAddressedAssetCatalog::CatalogExists(AzFramework::PlatformId::ES3), false);
    }
}

