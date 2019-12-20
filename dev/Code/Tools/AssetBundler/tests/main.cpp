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

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/API/BootstrapReaderBus.h>
#include <AzFramework/FileFunc/FileFunc.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/Asset/AssetBundler.h>
#include <AzFramework/Platform/PlatformDefaults.h>

#include <source/utils/utils.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <tests/main.h>

namespace AssetBundler
{
    class AssetBundlerBatchUtilsTest
        : public UnitTest::ScopedAllocatorSetupFixture
    {
    };

    TEST_F(AssetBundlerBatchUtilsTest, SplitFilename_MacFile_OutputBaseNameAndPlatform)
    {
        AZStd::string filePath = "assetInfoFile_osx_gl.xml";
        AZStd::string baseFilename;
        AZStd::string platformIdentifier;

        AzToolsFramework::SplitFilename(filePath, baseFilename, platformIdentifier);

        ASSERT_EQ(baseFilename, "assetInfoFile");
        ASSERT_EQ(platformIdentifier, "osx_gl");
    }

    TEST_F(AssetBundlerBatchUtilsTest, SplitFilename_PcFile_OutputBaseNameAndPlatform)
    {
        AZStd::string filePath = "assetInfoFile_pc.xml";
        AZStd::string baseFilename;
        AZStd::string platformIdentifier;

        AzToolsFramework::SplitFilename(filePath, baseFilename, platformIdentifier);

        ASSERT_EQ(baseFilename, "assetInfoFile");
        ASSERT_EQ(platformIdentifier, "pc");
    }

    TEST_F(AssetBundlerBatchUtilsTest, SplitFilename_MacFileWithUnderScoreInFileName_OutputBaseNameAndPlatform)
    {
        AZStd::string filePath = "assetInfoFile_test_osx_gl.xml";
        AZStd::string baseFilename;
        AZStd::string platformIdentifier;

        AzToolsFramework::SplitFilename(filePath, baseFilename, platformIdentifier);

        ASSERT_EQ(baseFilename, "assetInfoFile_test");
        ASSERT_EQ(platformIdentifier, "osx_gl");
    }

    TEST_F(AssetBundlerBatchUtilsTest, SplitFilename_PcFileWithUnderScoreInFileName_OutputBaseNameAndPlatform)
    {
        AZStd::string filePath = "assetInfoFile_test_pc.xml";
        AZStd::string baseFilename;
        AZStd::string platformIdentifier;

        AzToolsFramework::SplitFilename(filePath, baseFilename, platformIdentifier);

        ASSERT_EQ(baseFilename, "assetInfoFile_test");
        ASSERT_EQ(platformIdentifier, "pc");
    }

    const char RelativeTestFolder[] = "Code/Tools/AssetBundler/tests";
    const char GemsFolder[] = "Gems";
    const char EngineFolder[] = "Engine";
    const char PlatformsFolder[] = "Platforms";


    class AssetBundlerGemsUtilTest
        : public UnitTest::ScopedAllocatorSetupFixture
    {
    public:
        void SetUp() override
        {
            UnitTest::ScopedAllocatorSetupFixture::SetUp();
            m_data = AZStd::make_unique<StaticData>();
            m_data->m_application.reset(aznew AzToolsFramework::ToolsApplication());
            m_data->m_application.get()->Start(AzFramework::Application::Descriptor());

            const char* engineRoot = nullptr;
            AzFramework::ApplicationRequests::Bus::BroadcastResult(engineRoot, &AzFramework::ApplicationRequests::GetEngineRoot);
            if (!engineRoot)
            {
                GTEST_FATAL_FAILURE_(AZStd::string::format("Unable to locate engine root.\n").c_str());
            }

            AzFramework::StringFunc::Path::Join(engineRoot, RelativeTestFolder, m_data->m_testEngineRoot);

            m_data->m_localFileIO = aznew AZ::IO::LocalFileIO();
            m_data->m_priorFileIO = AZ::IO::FileIOBase::GetInstance();
            // we need to set it to nullptr first because otherwise the 
            // underneath code assumes that we might be leaking the previous instance
            AZ::IO::FileIOBase::SetInstance(nullptr);
            AZ::IO::FileIOBase::SetInstance(m_data->m_localFileIO);
           
            AddGemData(m_data->m_testEngineRoot.c_str(), "GemA");
            AddGemData(m_data->m_testEngineRoot.c_str(), "GemB");

            AZStd::string absoluteEngineSeedFilePath;
            AzFramework::StringFunc::Path::ConstructFull(m_data->m_testEngineRoot.c_str(), EngineFolder, "SeedAssetList", AzToolsFramework::AssetSeedManager::GetSeedFileExtension(), absoluteEngineSeedFilePath, true);
            m_data->m_gemSeedFilePairList.emplace_back(AZStd::make_pair(absoluteEngineSeedFilePath, true));

            AddGemData(m_data->m_testEngineRoot.c_str(), "GemC", false);
        }
        void TearDown() override
        {
            AZ::IO::FileIOBase::SetInstance(nullptr);
            delete m_data->m_localFileIO;
            AZ::IO::FileIOBase::SetInstance(m_data->m_priorFileIO);

            m_data->m_gemInfoList.set_capacity(0);
            m_data->m_gemSeedFilePairList.set_capacity(0);
            m_data->m_application.get()->Stop();
            m_data->m_application.reset();
            UnitTest::ScopedAllocatorSetupFixture::TearDown();
        }

        void AddGemData(const char* engineRoot, const char* gemName, bool seedFileExists = true)
        {
            AZStd::string relativeGemPath;
            AzFramework::StringFunc::Path::Join(GemsFolder, gemName, relativeGemPath);

            AZStd::string absoluteGemPath;
            AzFramework::StringFunc::Path::Join(engineRoot, relativeGemPath.c_str(), absoluteGemPath);

            AZStd::string absoluteGemSeedFilePath;
            AzFramework::StringFunc::Path::ConstructFull(absoluteGemPath.c_str(), "seedList", AzToolsFramework::AssetSeedManager::GetSeedFileExtension(), absoluteGemSeedFilePath, true);

            m_data->m_gemSeedFilePairList.emplace_back(AZStd::make_pair(absoluteGemSeedFilePath, seedFileExists));

            m_data->m_gemInfoList.emplace_back(AzToolsFramework::AssetUtils::GemInfo(gemName, relativeGemPath, absoluteGemPath, AZ::Uuid::CreateRandom().ToString<AZStd::string>().c_str(), false, false));
            
            AZStd::string platformsDirectory;
            AzFramework::StringFunc::Path::Join(absoluteGemPath.c_str(), PlatformsFolder, platformsDirectory);
            if (m_data->m_localFileIO->Exists(platformsDirectory.c_str()))
            {
                m_data->m_localFileIO->FindFiles(platformsDirectory.c_str(),
                    AZStd::string::format("*.%s", AzToolsFramework::AssetSeedManager::GetSeedFileExtension()).c_str(),
                    [&](const char* fileName)
                    {
                        AZStd::string normalizedFilePath = fileName;
                        AzFramework::StringFunc::Path::Normalize(normalizedFilePath);
                        m_data->m_gemSeedFilePairList.emplace_back(AZStd::make_pair(normalizedFilePath, seedFileExists));
                        return true;
                    });
            }

            AZStd::string iosDirectory;
            AzFramework::StringFunc::Path::Join(platformsDirectory.c_str(), AzFramework::PlatformIOS, iosDirectory);
            if (m_data->m_localFileIO->Exists(iosDirectory.c_str()))
            {
                bool recurse = true;
                AZ::Outcome<AZStd::list<AZStd::string>, AZStd::string> result = AzFramework::FileFunc::FindFileList(iosDirectory,
                    AZStd::string::format("*.%s", AzToolsFramework::AssetSeedManager::GetSeedFileExtension()).c_str(), recurse);

                if (result.IsSuccess())
                {
                    AZStd::list<AZStd::string> seedFiles = result.TakeValue();
                    for(AZStd::string& seedFile : seedFiles)
                    {
                        AZStd::string normalizedFilePath = seedFile;
                        AzFramework::StringFunc::Path::Normalize(normalizedFilePath);
                        m_data->m_gemSeedFilePairList.emplace_back(AZStd::make_pair(normalizedFilePath, seedFileExists));
                    }
                }
            }
        }

        struct StaticData
        {
            AZStd::vector<AzToolsFramework::AssetUtils::GemInfo> m_gemInfoList;
            AZStd::vector<AZStd::pair<AZStd::string, bool>> m_gemSeedFilePairList;
            AZStd::unique_ptr<AzToolsFramework::ToolsApplication> m_application = {};
            AZ::IO::FileIOBase* m_priorFileIO = nullptr;
            AZ::IO::FileIOBase* m_localFileIO = nullptr;
            AZStd::string m_testEngineRoot;
        };

        AZStd::unique_ptr<StaticData> m_data;
    };

    TEST_F(AssetBundlerGemsUtilTest, GetDefaultSeedFiles_AllSeedFiles_Found)
    {
        AZStd::vector<AZStd::string> defaultSeedList = AssetBundler::GetDefaultSeedListFiles(m_data->m_testEngineRoot.c_str(), m_data->m_gemInfoList, AzFramework::PlatformFlags::Platform_PC);
        ASSERT_EQ(defaultSeedList.size(), 4); //adding one for the engine seed file

        int gemAIndex = 0;
        int gemBIndex = 1;
        int gemBSharedFileIndex = 2;
        int engineIndex = 4;

        // Validate whether both GemA and GemB seed file are present  
        EXPECT_EQ(defaultSeedList[gemAIndex], m_data->m_gemSeedFilePairList[gemAIndex].first);
        EXPECT_EQ(defaultSeedList[gemBIndex], m_data->m_gemSeedFilePairList[gemBIndex].first);
        EXPECT_EQ(defaultSeedList[gemBSharedFileIndex], m_data->m_gemSeedFilePairList[gemBSharedFileIndex].first);

        // Validate that the engine seed file is present
        EXPECT_EQ(defaultSeedList.back(), m_data->m_gemSeedFilePairList[engineIndex].first);
    }

    TEST_F(AssetBundlerGemsUtilTest, GetDefaultSeedFilesForMultiplePlatforms_AllSeedFiles_Found)
    {
        AZStd::vector<AZStd::string> defaultSeedList = AssetBundler::GetDefaultSeedListFiles(m_data->m_testEngineRoot.c_str(), m_data->m_gemInfoList, AzFramework::PlatformFlags::Platform_PC | AzFramework::PlatformFlags::Platform_IOS);
        ASSERT_EQ(defaultSeedList.size(), 5); //adding one for the engine seed file

        int gemAIndex = 0;
        int gemBIndex = 1;
        int gemBSharedFileIndex = 2;
        int gemBIosFileIndex = 3;
        int engineIndex = 4;

        // Validate whether both GemA and GemB seed file are present  
        EXPECT_EQ(defaultSeedList[gemAIndex], m_data->m_gemSeedFilePairList[gemAIndex].first);
        EXPECT_EQ(defaultSeedList[gemBIndex], m_data->m_gemSeedFilePairList[gemBIndex].first);
        EXPECT_EQ(defaultSeedList[gemBSharedFileIndex], m_data->m_gemSeedFilePairList[gemBSharedFileIndex].first);
        EXPECT_EQ(defaultSeedList[gemBIosFileIndex], m_data->m_gemSeedFilePairList[gemBIosFileIndex].first);

        // Validate that the engine seed file is present
        EXPECT_EQ(defaultSeedList.back(), m_data->m_gemSeedFilePairList[engineIndex].first);
    }


    TEST_F(AssetBundlerGemsUtilTest, IsSeedFileValid_Ok)
    {
         for (const auto& pair : m_data->m_gemSeedFilePairList)
        {
            bool result = IsGemSeedFilePathValid(m_data->m_testEngineRoot.c_str(), pair.first, m_data->m_gemInfoList, AzFramework::PlatformFlags::Platform_PC | AzFramework::PlatformFlags::Platform_IOS);
            EXPECT_EQ(result,pair.second);
        }
    }

    const char TestProject[] = "TestProject";
    const char TestProjectLowerCase[] = "testproject";
#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
    const char TestRoot[] = "D:\\Dummy\\Test\\dev\\";
#else
    const char TestRoot[] = "/Dummy/Test/dev/";
#endif

    class MockApplication
        : public AzFramework::ApplicationRequests::Bus::Handler
    {
    public:
        MockApplication()
        {
            AzFramework::ApplicationRequests::Bus::Handler::BusConnect();
        }
        ~MockApplication()
        {
            AzFramework::ApplicationRequests::Bus::Handler::BusDisconnect();
        }
        // AzFramework::ApplicationRequests::Bus::Handler interface
        void NormalizePath(AZStd::string& /*path*/) override {};
        void NormalizePathKeepCase(AZStd::string& /*path*/) override {};
        void CalculateBranchTokenForAppRoot(AZStd::string& /*token*/) const override {};
        const char* GetEngineRoot() const { return TestRoot; }
    };

    class MockBootstrapReader
        : public AzFramework::BootstrapReaderRequestBus::Handler
    {
    public:
        MockBootstrapReader()
        {
            AzFramework::BootstrapReaderRequestBus::Handler::BusConnect();
        }
        ~MockBootstrapReader()
        {
            AzFramework::BootstrapReaderRequestBus::Handler::BusDisconnect();
        }
        // AzFramework::BootstrapReaderRequestBus interface
        bool SearchConfigurationForKey(const AZStd::string& key, bool checkPlatform, AZStd::string& value) override
        {
            if (key == "sys_game_folder")
            {
                value = TestProject;
            }

            return true;
        }
    };


    class AssetBundlerPathUtilTest
        : public UnitTest::ScopedAllocatorSetupFixture
    {
    public:

        void SetUp() override
        {
            UnitTest::ScopedAllocatorSetupFixture::SetUp();
            m_data = AZStd::make_unique<StaticData>();
        }

        void TearDown() override
        {
            m_data.reset();
            UnitTest::ScopedAllocatorSetupFixture::TearDown();
        }

        struct StaticData
        {
            MockApplication m_mockApplication;
            MockBootstrapReader m_mockBootstrapReader;
        };

        AZStd::unique_ptr<StaticData> m_data;
    };


    TEST_F(AssetBundlerPathUtilTest, ComputeAssetAliasAndGameName_AssetCatalogPathNotProvided_Valid)
    {
        AZStd::string platformIdentifier = "pc";
        AZStd::string assetCatalogFile;
        AZStd::string assetAlias;
        AZStd::string gameName;
        EXPECT_TRUE(ComputeAssetAliasAndGameName(platformIdentifier, assetCatalogFile, assetAlias, gameName).IsSuccess());
        EXPECT_TRUE(gameName == TestProject);

        AZStd::string assetPath;
        bool success = AzFramework::StringFunc::Path::ConstructFull(TestRoot, "Cache", assetPath) &&
            AzFramework::StringFunc::Path::ConstructFull(assetPath.c_str(), TestProject, assetPath) &&
            AzFramework::StringFunc::Path::ConstructFull(assetPath.c_str(), platformIdentifier.c_str(), assetPath) &&
            AzFramework::StringFunc::Path::ConstructFull(assetPath.c_str(), TestProjectLowerCase, assetPath);
        EXPECT_EQ(assetAlias, assetPath);
    }

    TEST_F(AssetBundlerPathUtilTest, ComputeAssetAliasAndGameName_AssetCatalogPathProvided_Valid)
    {
        AZStd::string platformIdentifier = "pc";
#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
        AZStd::string assetCatalogFile = "D:\\Dummy\\Test\\dev\\Cache\\TestProject1\\pc\\testproject1\\assetcatalog.xml";
#else
        AZStd::string assetCatalogFile = "/Dummy/Test/dev/Cache/TestProject1/pc/testproject1/assetcatalog.xml";
#endif
        AZStd::string assetAlias;
        AZStd::string gameName;
        EXPECT_TRUE(ComputeAssetAliasAndGameName(platformIdentifier, assetCatalogFile, assetAlias, gameName).IsSuccess());
        EXPECT_EQ(gameName, "TestProject1");

        AZStd::string assetPath;
        bool success = AzFramework::StringFunc::Path::ConstructFull(TestRoot, "Cache", assetPath) &&
            AzFramework::StringFunc::Path::ConstructFull(assetPath.c_str(), "TestProject1", assetPath) &&
            AzFramework::StringFunc::Path::ConstructFull(assetPath.c_str(), platformIdentifier.c_str(), assetPath) &&
            AzFramework::StringFunc::Path::ConstructFull(assetPath.c_str(), "testproject1", assetPath);
        EXPECT_EQ(assetAlias, assetPath);
    }

    TEST_F(AssetBundlerPathUtilTest, ComputeAssetAliasAndGameName_GameNameMismatch_Failure)
    {
        AZStd::string platformIdentifier = "pc";
#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
        AZStd::string assetCatalogFile = "D:\\Dummy\\Cache\\TestProject1\\pc\\testproject1\\assetcatalog.xml";
#else
        AZStd::string assetCatalogFile = "/Dummy/Cache/TestProject1/pc/testproject1/assetcatalog.xml";
#endif
        AZStd::string assetAlias;
        AZStd::string gameName="SomeOtherGamename";
        EXPECT_FALSE(ComputeAssetAliasAndGameName(platformIdentifier, assetCatalogFile, assetAlias, gameName).IsSuccess());
    }
}
