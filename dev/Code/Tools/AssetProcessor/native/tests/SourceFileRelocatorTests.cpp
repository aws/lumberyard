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

#include <AssetManager/SourceFileRelocator.h>
#include <AzTest/AzTest.h>
#include "AssetProcessorTest.h"
#include "AzToolsFramework/API/AssetDatabaseBus.h"
#include "AssetDatabase/AssetDatabase.h"
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTests
{
    //using namespace testing;
    using testing::NiceMock;
    using namespace AssetProcessor;
    using namespace AzToolsFramework::AssetDatabase;

    class MockDatabaseLocationListener : public AssetDatabaseRequests::Bus::Handler
    {
    public:
        MOCK_METHOD1(GetAssetDatabaseLocation, bool(AZStd::string&));
    };

    class MockFileIO : public AZ::IO::FileIOBase
    {
    public:
        bool IsDirectory(const char* filePath) override
        {
            return !AzFramework::StringFunc::Path::HasExtension(filePath);
        }
        MOCK_METHOD3(Open, AZ::IO::Result (const char*, AZ::IO::OpenMode, AZ::IO::HandleType&));
        MOCK_METHOD1(Close, AZ::IO::Result (AZ::IO::HandleType));
        MOCK_METHOD2(Tell, AZ::IO::Result (AZ::IO::HandleType, AZ::u64&));
        MOCK_METHOD3(Seek, AZ::IO::Result (AZ::IO::HandleType, AZ::s64, AZ::IO::SeekType));
        MOCK_METHOD5(Read, AZ::IO::Result (AZ::IO::HandleType, void*, AZ::u64, bool, AZ::u64*));
        MOCK_METHOD4(Write, AZ::IO::Result (AZ::IO::HandleType, const void*, AZ::u64, AZ::u64*));
        MOCK_METHOD1(Flush, AZ::IO::Result (AZ::IO::HandleType));
        MOCK_METHOD1(Eof, bool (AZ::IO::HandleType));
        MOCK_METHOD1(ModificationTime, AZ::u64 (AZ::IO::HandleType));
        MOCK_METHOD1(ModificationTime, AZ::u64 (const char*));
        MOCK_METHOD2(Size, AZ::IO::Result (const char*, AZ::u64&));
        MOCK_METHOD2(Size, AZ::IO::Result (AZ::IO::HandleType, AZ::u64&));
        MOCK_METHOD1(Exists, bool (const char*));
        MOCK_METHOD1(IsReadOnly, bool (const char*));
        MOCK_METHOD1(CreatePath, AZ::IO::Result (const char*));
        MOCK_METHOD1(DestroyPath, AZ::IO::Result (const char*));
        MOCK_METHOD1(Remove, AZ::IO::Result (const char*));
        MOCK_METHOD2(Copy, AZ::IO::Result (const char*, const char*));
        MOCK_METHOD2(Rename, AZ::IO::Result (const char*, const char*));
        MOCK_METHOD3(FindFiles, AZ::IO::Result (const char*, const char*, FindFilesCallbackType));
        MOCK_METHOD2(SetAlias, void (const char*, const char*));
        MOCK_METHOD1(ClearAlias, void (const char*));
        MOCK_METHOD1(GetAlias, const char* (const char*));
        MOCK_CONST_METHOD2(ConvertToAlias, AZ::u64 (char*, AZ::u64));
        MOCK_METHOD3(ResolvePath, bool (const char*, char*, AZ::u64));
        MOCK_CONST_METHOD3(GetFilename, bool (AZ::IO::HandleType, char*, AZ::u64));
        MOCK_METHOD0(IsRemoteIOEnabled, bool ());
    };

    class SourceFileRelocatorTest
        : public UnitTest::ScopedAllocatorSetupFixture
        , public UnitTest::TraceBusRedirector
    {
    public:
        void SetUp() override
        {
            AZ::Debug::TraceMessageBus::Handler::BusConnect();

            m_data.reset(new StaticData());

            QDir tempPath(m_tempDir.path());

            m_data->m_connection.reset(new AssetProcessor::AssetDatabaseConnection());
            m_data->m_databaseLocation = tempPath.absoluteFilePath("test_database.sqlite").toUtf8().constData();
            m_data->m_databaseLocationListener.BusConnect();

            {
                using namespace testing;

                ON_CALL(m_data->m_databaseLocationListener, GetAssetDatabaseLocation(_))
                    .WillByDefault(
                        DoAll( // set the 0th argument ref (string) to the database location and return true.
                            SetArgReferee<0>(m_data->m_databaseLocation),
                            Return(true)));
            }
            // Initialize the database:
            m_data->m_connection->ClearData(); // this is expected to reset/clear/reopen

            m_data->m_platformConfig.EnablePlatform(AssetBuilderSDK::PlatformInfo("pc", { "desktop" }));

            AZStd::vector<AssetBuilderSDK::PlatformInfo> platforms;
            m_data->m_platformConfig.PopulatePlatformsForScanFolder(platforms);

            m_data->m_scanFolder1 = { tempPath.absoluteFilePath("dev").toUtf8().constData(), "dev", "devKey", ""};
            m_data->m_scanFolder2 = { tempPath.absoluteFilePath("folder").toUtf8().constData(), "folder", "folderKey", "prefix" };

            ASSERT_TRUE(UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("dev/subfolder1/somefile.tif")));
            ASSERT_TRUE(UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("dev/subfolder1/otherfile.tif")));
            ASSERT_TRUE(UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("folder/otherfile.tif")));
            ASSERT_TRUE(UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("folder/a/b/c/d/otherfile.tif")));
            ASSERT_TRUE(UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("dev/duplicate/file1.tif")));
            ASSERT_TRUE(UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("folder/duplicate/file1.tif")));
            ASSERT_TRUE(UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("dev/subfolder2/file.tif")));

            ASSERT_TRUE(m_data->m_connection->SetScanFolder(m_data->m_scanFolder1));
            ASSERT_TRUE(m_data->m_connection->SetScanFolder(m_data->m_scanFolder2));

            ScanFolderInfo scanFolder1(m_data->m_scanFolder1.m_scanFolder.c_str(),
                m_data->m_scanFolder1.m_displayName.c_str(),
                m_data->m_scanFolder1.m_portableKey.c_str(),
                m_data->m_scanFolder1.m_outputPrefix.c_str(),
                false, true, platforms, 0, m_data->m_scanFolder1.m_scanFolderID);
            m_data->m_platformConfig.AddScanFolder(scanFolder1);

            ScanFolderInfo scanFolder2(m_data->m_scanFolder2.m_scanFolder.c_str(),
                m_data->m_scanFolder2.m_displayName.c_str(),
                m_data->m_scanFolder2.m_portableKey.c_str(),
                m_data->m_scanFolder2.m_outputPrefix.c_str(),
                false, true, platforms, 0, m_data->m_scanFolder2.m_scanFolderID);
            m_data->m_platformConfig.AddScanFolder(scanFolder2);

            SourceDatabaseEntry sourceFile1 = { m_data->m_scanFolder1.m_scanFolderID, "subfolder1/somefile.tif", AZ::Uuid::CreateRandom(), "AnalysisFingerprint" };
            SourceDatabaseEntry sourceFile2 = { m_data->m_scanFolder1.m_scanFolderID, "subfolder1/otherfile.tif", AZ::Uuid::CreateRandom(), "AnalysisFingerprint" };
            SourceDatabaseEntry sourceFile3 = { m_data->m_scanFolder2.m_scanFolderID, "prefix/otherfile.tif", AZ::Uuid::CreateRandom(), "AnalysisFingerprint" };
            SourceDatabaseEntry sourceFile4 = { m_data->m_scanFolder2.m_scanFolderID, "prefix/a/b/c/d/otherfile.tif", AZ::Uuid::CreateRandom(), "AnalysisFingerprint" };
            SourceDatabaseEntry sourceFile5 = { m_data->m_scanFolder1.m_scanFolderID, "duplicate/file1.tif", AZ::Uuid::CreateRandom(), "AnalysisFingerprint" };
            SourceDatabaseEntry sourceFile6 = { m_data->m_scanFolder2.m_scanFolderID, "duplicate/file1.tif", AZ::Uuid::CreateRandom(), "AnalysisFingerprint" };
            SourceDatabaseEntry sourceFile7 = { m_data->m_scanFolder1.m_scanFolderID, "subfolder2/file.tif", AZ::Uuid::CreateRandom(), "AnalysisFingerprint" };
            SourceDatabaseEntry sourceFile8 = { m_data->m_scanFolder1.m_scanFolderID, "test.txt", AZ::Uuid::CreateRandom(), "AnalysisFingerprint" };
            ASSERT_TRUE(m_data->m_connection->SetSource(sourceFile1));
            ASSERT_TRUE(m_data->m_connection->SetSource(sourceFile2));
            ASSERT_TRUE(m_data->m_connection->SetSource(sourceFile3));
            ASSERT_TRUE(m_data->m_connection->SetSource(sourceFile4));
            ASSERT_TRUE(m_data->m_connection->SetSource(sourceFile5));
            ASSERT_TRUE(m_data->m_connection->SetSource(sourceFile6));
            ASSERT_TRUE(m_data->m_connection->SetSource(sourceFile7));
            ASSERT_TRUE(m_data->m_connection->SetSource(sourceFile8));

            SourceFileDependencyEntry dependency1 = { AZ::Uuid::CreateRandom(), "subfolder1/somefile.tif", "subfolder1/otherfile.tif", SourceFileDependencyEntry::TypeOfDependency::DEP_SourceToSource, false };
            SourceFileDependencyEntry dependency2 = { AZ::Uuid::CreateRandom(), "subfolder1/otherfile.tif", "prefix/otherfile.tif", SourceFileDependencyEntry::TypeOfDependency::DEP_JobToJob, false };
            ASSERT_TRUE(m_data->m_connection->SetSourceFileDependency(dependency1));
            ASSERT_TRUE(m_data->m_connection->SetSourceFileDependency(dependency2));

            JobDatabaseEntry job1 = { sourceFile1.m_sourceID, "JobKey", 12345, "pc", AZ::Uuid::CreateRandom(), AzToolsFramework::AssetSystem::JobStatus::Completed,
            1111 };
            JobDatabaseEntry job2 = { sourceFile2.m_sourceID, "JobKey", 2222, "pc", AZ::Uuid::CreateRandom(), AzToolsFramework::AssetSystem::JobStatus::Completed,
            1111 };
            JobDatabaseEntry job3 = { sourceFile3.m_sourceID, "JobKey", 4444, "pc", AZ::Uuid::CreateRandom(), AzToolsFramework::AssetSystem::JobStatus::Completed,
            1111 };
            ASSERT_TRUE(m_data->m_connection->SetJob(job1));
            ASSERT_TRUE(m_data->m_connection->SetJob(job2));
            ASSERT_TRUE(m_data->m_connection->SetJob(job3));

            const AZ::u32 productSubId = 1;
            ProductDatabaseEntry product1 = { job1.m_jobID, productSubId, "subfolder1/somefile.dds", AZ::Data::AssetType::CreateRandom() };
            ProductDatabaseEntry product2 = { job2.m_jobID, productSubId, "subfolder1/otherfile.dds", AZ::Data::AssetType::CreateRandom() };
            ProductDatabaseEntry product3 = { job3.m_jobID, productSubId, "blah.dds", AZ::Data::AssetType::CreateRandom() };
            ASSERT_TRUE(m_data->m_connection->SetProduct(product1));
            ASSERT_TRUE(m_data->m_connection->SetProduct(product2));
            ASSERT_TRUE(m_data->m_connection->SetProduct(product3));

            ProductDependencyDatabaseEntry productDependency1 = { product1.m_productID, sourceFile2.m_sourceGuid, productSubId, {}, "pc", false };
            ProductDependencyDatabaseEntry productDependency2 = { product2.m_productID, sourceFile3.m_sourceGuid, productSubId, {}, "pc", false };
            ProductDependencyDatabaseEntry productDependency3 = { product1.m_productID, sourceFile4.m_sourceGuid, productSubId, {}, "pc", false };
            ProductDependencyDatabaseEntry productDependency4 = { product2.m_productID, sourceFile4.m_sourceGuid, productSubId, {}, "pc", false };
            ProductDependencyDatabaseEntry productDependency5 = { product3.m_productID, sourceFile4.m_sourceGuid, productSubId, {}, "pc", true };

            ASSERT_TRUE(m_data->m_connection->SetProductDependency(productDependency1));
            ASSERT_TRUE(m_data->m_connection->SetProductDependency(productDependency2));
            ASSERT_TRUE(m_data->m_connection->SetProductDependency(productDependency3));
            ASSERT_TRUE(m_data->m_connection->SetProductDependency(productDependency4));
            ASSERT_TRUE(m_data->m_connection->SetProductDependency(productDependency5));

            AZ::IO::FileIOBase::SetInstance(nullptr); // The API requires the old instance to be destroyed first
            AZ::IO::FileIOBase::SetInstance(new AZ::IO::LocalFileIO());

            m_data->m_reporter = AZStd::make_unique<SourceFileRelocator>(m_data->m_connection, &m_data->m_platformConfig);
        }

        void TearDown() override
        {
            m_data->m_databaseLocationListener.BusDisconnect();
            m_data.reset();

            AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
        }

        void TestResultEntries(const SourceFileRelocationContainer& container, AZStd::initializer_list<AZStd::string> expectedEntryDatabaseNames) const
        {
            AZStd::vector<AZStd::string> actualEntryDatabaseNames;

            for(const auto& relocationInfo : container)
            {
                actualEntryDatabaseNames.push_back(relocationInfo.m_sourceEntry.m_sourceName);
            }

            ASSERT_THAT(actualEntryDatabaseNames, testing::UnorderedElementsAreArray(expectedEntryDatabaseNames));
        }

        void TestGetSourcesByPath(const AZStd::string& source, AZStd::initializer_list<AZStd::string> expectedEntryDatabaseNames, bool expectSuccess = true) const
        {
            SourceFileRelocationContainer relocationContainer;

            QDir tempPath(m_tempDir.path());

            const ScanFolderInfo* info;
            auto result = m_data->m_reporter->GetSourcesByPath(source, relocationContainer, info);
            ASSERT_EQ(result.IsSuccess(), expectSuccess) << result.GetError().c_str();

            if (expectSuccess)
            {
                TestResultEntries(relocationContainer, expectedEntryDatabaseNames);
            }
        }

        void TestComputeDestination(const ScanFolderDatabaseEntry& scanFolderEntry, AZStd::string_view sourceWithScanFolder, AZStd::string_view source, AZStd::string_view destination, const char* expectedPath, bool expectSuccess = true) const
        {
            SourceFileRelocationContainer entryContainer;

            QDir tempPath(m_tempDir.path());
            const ScanFolderInfo* info = nullptr;
            const ScanFolderInfo* destInfo = nullptr;

            ASSERT_TRUE(m_data->m_reporter->GetSourcesByPath(tempPath.absoluteFilePath(AZStd::string(sourceWithScanFolder).c_str()).toUtf8().constData(), entryContainer, info));

            auto result = m_data->m_reporter->ComputeDestination(entryContainer, m_data->m_platformConfig.GetScanFolderByPath(scanFolderEntry.m_scanFolder.c_str()), source, destination, destInfo);

            ASSERT_EQ(result.IsSuccess(), expectSuccess) << result.GetError().c_str();

            if (expectSuccess)
            {
                ASSERT_STREQ(entryContainer[0].m_newRelativePath.c_str(), expectedPath);
                ASSERT_NE(destInfo, nullptr);
            }
        }

        struct StaticData
        {
            // these variables are created during SetUp() and destroyed during TearDown() and thus are always available during tests using this fixture:
            AZStd::string m_databaseLocation;
            NiceMock<MockDatabaseLocationListener> m_databaseLocationListener;
            AZStd::shared_ptr<AssetProcessor::AssetDatabaseConnection> m_connection;
            PlatformConfiguration m_platformConfig;

            ScanFolderDatabaseEntry m_scanFolder1;
            ScanFolderDatabaseEntry m_scanFolder2;

            FileStatePassthrough m_fileStateCache;

            AZStd::unique_ptr<SourceFileRelocator> m_reporter;
        };

        QTemporaryDir m_tempDir;

        // we store the above data in a unique_ptr so that its memory can be cleared during TearDown() in one call, before we destroy the memory
        // allocator, reducing the chance of missing or forgetting to destroy one in the future.
        AZStd::unique_ptr<StaticData> m_data;
    };

    TEST_F(SourceFileRelocatorTest, GetSources_SingleFile_Succeeds)
    {
        TestGetSourcesByPath("subfolder1/somefile.tif", { "subfolder1/somefile.tif" });
    }

    TEST_F(SourceFileRelocatorTest, GetSources_Folder_Fails)
    {
        TestGetSourcesByPath("subfolder1", { }, false);
    }

    TEST_F(SourceFileRelocatorTest, GetSources_SingleFileWildcard1_Succeeds)
    {
        TestGetSourcesByPath("subfolder1/some*", { "subfolder1/somefile.tif" });
    }

    TEST_F(SourceFileRelocatorTest, GetSources_SingleFileWildcard2_Succeeds)
    {
        TestGetSourcesByPath("subfolder1/some*.tif", { "subfolder1/somefile.tif" });
    }

    TEST_F(SourceFileRelocatorTest, GetSources_SingleFileWildcard3_Succeeds)
    {
        TestGetSourcesByPath("?ther?ile.ti?", { "prefix/otherfile.tif" });
    }

    TEST_F(SourceFileRelocatorTest, GetSources_MultipleFilesWildcard1_Succeeds)
    {
        TestGetSourcesByPath("subfolder1/*file*", { "subfolder1/somefile.tif", "subfolder1/otherfile.tif" });
    }

    TEST_F(SourceFileRelocatorTest, GetSources_MultipleFilesWildcard2_Succeeds)
    {
        TestGetSourcesByPath("subfolder1/*", { "subfolder1/somefile.tif", "subfolder1/otherfile.tif" });
    }

    TEST_F(SourceFileRelocatorTest, GetSources_MultipleFilesWildcard_AbsolutePath_Succeeds)
    {
        QDir tempDir(m_tempDir.path());
        TestGetSourcesByPath(tempDir.absoluteFilePath("dev/subfolder1*").toUtf8().constData(), { "subfolder1/somefile.tif", "subfolder1/otherfile.tif" });
    }

    TEST_F(SourceFileRelocatorTest, GetSources_MultipleFoldersWildcard_Succeeds)
    {
        QDir tempDir(m_tempDir.path());
        TestGetSourcesByPath("subfolder*/*", { "subfolder1/somefile.tif", "subfolder1/otherfile.tif", "subfolder2/file.tif" });
    }

    TEST_F(SourceFileRelocatorTest, GetSources_ScanFolder1_Fails)
    {
        TestGetSourcesByPath("dev", { }, false);
    }

    TEST_F(SourceFileRelocatorTest, GetSources_ScanFolder2_Fails)
    {
        TestGetSourcesByPath("dev/", { }, false);
    }

    TEST_F(SourceFileRelocatorTest, GetSources_MultipleScanFolders_Fails)
    {
        TestGetSourcesByPath("*", { }, false);
    }

    TEST_F(SourceFileRelocatorTest, GetSources_PartialPath_SucceedsWithNoResults)
    {
        TestGetSourcesByPath("older/*", { }, true);
    }

    TEST_F(SourceFileRelocatorTest, GetSources_AmbiguousPath1_Fails)
    {
        TestGetSourcesByPath("duplicate/file1.tif", { }, false);
    }

    TEST_F(SourceFileRelocatorTest, GetSources_AmbiguousPathWildcard_Fails)
    {
        TestGetSourcesByPath("duplicate/*.tif", { }, false);
    }

    TEST_F(SourceFileRelocatorTest, GetSources_DuplicateFile_AbsolutePath_Succeeds)
    {
        QDir tempPath(m_tempDir.path());

        auto filePath = QDir(tempPath.absoluteFilePath(m_data->m_scanFolder1.m_scanFolder.c_str())).absoluteFilePath("duplicate/file1.tif");
        TestGetSourcesByPath(filePath.toUtf8().constData(), { "duplicate/file1.tif" }, true);
    }

    TEST_F(SourceFileRelocatorTest, HandleWildcard_RepeatCharacters1_Succeeds)
    {
        auto result = m_data->m_reporter->HandleWildcard("aaaaaaab", "a*b*", "a*bb*");

        ASSERT_TRUE(result.IsSuccess());
        ASSERT_STREQ(result.GetValue().c_str(), "aaaaaaabb");
    }

    TEST_F(SourceFileRelocatorTest, HandleWildcard_RepeatCharacters2_Succeeds)
    {
        auto result = m_data->m_reporter->HandleWildcard("aaaaaaaaaa", "a*a*", "a*b*");

        ASSERT_TRUE(result.IsSuccess());
        ASSERT_STREQ(result.GetValue().c_str(), "aaaaaaaaab");
    }

    TEST_F(SourceFileRelocatorTest, HandleWildcard_RepeatCharacters3_Succeeds)
    {
        auto result = m_data->m_reporter->HandleWildcard("aaabbbaaabbb", "a*a*", "a*c*");

        ASSERT_TRUE(result.IsSuccess());
        ASSERT_STREQ(result.GetValue().c_str(), "aaabbbaacbbb");
    }

    TEST_F(SourceFileRelocatorTest, HandleWildcard_RepeatCharacters4_Succeeds)
    {
        auto result = m_data->m_reporter->HandleWildcard("aabccbedd", "a*b*dd", "1*2*3");

        ASSERT_TRUE(result.IsSuccess());
        ASSERT_STREQ(result.GetValue().c_str(), "1abcc2e3");
    }

    TEST_F(SourceFileRelocatorTest, HandleWildcard_ZeroLengthMatch_Succeeds)
    {
        auto result = m_data->m_reporter->HandleWildcard("aabb", "aabb*", "1*");

        ASSERT_TRUE(result.IsSuccess());
        ASSERT_STREQ(result.GetValue().c_str(), "1");
    }

    TEST_F(SourceFileRelocatorTest, HandleWildcard_ZeroLengthMatch_MultipleWildcards_Succeeds)
    {
        auto result = m_data->m_reporter->HandleWildcard("abcdef", "a*b*e*", "1*2*3*");

        ASSERT_TRUE(result.IsSuccess());
        ASSERT_STREQ(result.GetValue().c_str(), "12cd3f");
    }

    TEST_F(SourceFileRelocatorTest, HandleWildcard_Complex_Succeeds)
    {
        auto result = m_data->m_reporter->HandleWildcard("subfolder1somefile.tif", "*o*some*.tif", "*1*2*3");

        ASSERT_TRUE(result.IsSuccess());
    }

    TEST_F(SourceFileRelocatorTest, HandleWildcard_TooComplex_Fails)
    {
        auto result = m_data->m_reporter->HandleWildcard("subfolder1/somefile.tif", "*o*some*.tif", "*1*2*3");

        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(SourceFileRelocatorTest, GatherDependencies_Succeeds)
    {
        SourceFileRelocationContainer entryContainer;

        QDir tempPath(m_tempDir.path());
        const ScanFolderInfo* info;

        ASSERT_TRUE(m_data->m_reporter->GetSourcesByPath(tempPath.absoluteFilePath(AZStd::string("folder/o*").c_str()).toUtf8().constData(), entryContainer, info));
        ASSERT_EQ(entryContainer.size(), 1);
        ASSERT_STREQ(entryContainer[0].m_sourceEntry.m_sourceName.c_str(), "prefix/otherfile.tif");

        m_data->m_reporter->PopulateDependencies(entryContainer);

        AZStd::vector<AZStd::string> databaseSourceNames;
        AZStd::vector<AZ::s64> databaseProductDependencyNames;

        for(const auto& relocationInfo : entryContainer)
        {
            for (const auto& dependencyEntry : relocationInfo.m_sourceDependencyEntries)
            {
                databaseSourceNames.push_back(dependencyEntry.m_source);
            }
        }

        for(const auto& relocationInfo : entryContainer)
        {
            for(const auto& productDependency : relocationInfo.m_productDependencyEntries)
            {
                databaseProductDependencyNames.push_back(productDependency.m_productPK);
            }
        }

        ASSERT_THAT(databaseSourceNames, testing::UnorderedElementsAreArray({ "subfolder1/otherfile.tif" }));
        ASSERT_THAT(databaseProductDependencyNames, testing::UnorderedElementsAreArray({ 2 }));
    }

    TEST_F(SourceFileRelocatorTest, ComputeDestination_MoveFolder_Succeeds)
    {
        TestComputeDestination(m_data->m_scanFolder2, "folder/o*.tif", "o*.tif", "newfolder/makeafolder/o*.tif", "newfolder/makeafolder/otherfile.tif");
    }

    TEST_F(SourceFileRelocatorTest, ComputeDestination_RenameFile_Succeeds)
    {
        TestComputeDestination(m_data->m_scanFolder2, "folder/otherfile.tif", "otherfile.tif", "anewfile.png", "anewfile.png");
    }

    TEST_F(SourceFileRelocatorTest, ComputeDestination_MoveFolderDeeper_Succeeds)
    {
        TestComputeDestination(m_data->m_scanFolder1, "dev/*o*/some*.tif", "*o*/some*.tif", "subfolder2/subfolder3/*o*/some*.tif", "subfolder2/subfolder3/subfolder1/somefile.tif");
    }

    TEST_F(SourceFileRelocatorTest, ComputeDestination_MoveFileUpAFolder_Succeeds)
    {
        TestComputeDestination(m_data->m_scanFolder1, "dev/subfolder1/somefile.tif", "subfolder1/somefile.tif", "somefile.tif", "somefile.tif");
    }

    TEST_F(SourceFileRelocatorTest, ComputeDestination_MoveFileUpAFolderAndRename_Succeeds)
    {
        TestComputeDestination(m_data->m_scanFolder1, "dev/subfolder1/somefile.tif", "subfolder1/somefile.tif", "somenewfile.tif", "somenewfile.tif");
    }

    TEST_F(SourceFileRelocatorTest, ComputeDestination_MoveFileUpPartial_Succeeds)
    {
        TestComputeDestination(m_data->m_scanFolder2, "folder/a/*", "a/b/c/*", "a/*", "a/d/otherfile.tif");
    }

    TEST_F(SourceFileRelocatorTest, ComputeDestination_AbsolutePathBoth_Succeeds)
    {
        QDir tempPath(m_tempDir.path());
        TestComputeDestination(m_data->m_scanFolder2, "folder/a/*",
            tempPath.absoluteFilePath("folder/a/b/*").toUtf8().constData(),
            tempPath.absoluteFilePath("folder/a/*").toUtf8().constData(),
            "a/c/d/otherfile.tif");
    }

    TEST_F(SourceFileRelocatorTest, ComputeDestination_AbsolutePathSource_Succeeds)
    {
        QDir tempPath(m_tempDir.path());
        TestComputeDestination(m_data->m_scanFolder2, "folder/a/*",
            tempPath.absoluteFilePath("folder/a/b/*").toUtf8().constData(),
            "a/*",
            "a/c/d/otherfile.tif");
    }

    TEST_F(SourceFileRelocatorTest, ComputeDestination_AbsolutePathDestination_Succeeds)
    {
        QDir tempPath(m_tempDir.path());
        TestComputeDestination(m_data->m_scanFolder2, "folder/a/*",
            "a/b/*",
            tempPath.absoluteFilePath("folder/a/*").toUtf8().constData(),
            "a/c/d/otherfile.tif");
    }

    TEST_F(SourceFileRelocatorTest, ComputeDestination_AbsolutePathRename_Succeeds)
    {
        QDir tempPath(m_tempDir.path());
        TestComputeDestination(m_data->m_scanFolder2, "folder/a/*",
            "a/b/c/d/otherfile.tif",
            tempPath.absoluteFilePath("folder/a/c/d/newlyNamed.png").toUtf8().constData(),
            "a/c/d/newlyNamed.png");
    }

    TEST_F(SourceFileRelocatorTest, ComputeDestination_MoveOutsideScanfolder_Fails)
    {
        TestComputeDestination(m_data->m_scanFolder2, "folder/a/*", "a/b/c/*", m_tempDir.path().toUtf8().constData(), "", false);
    }

    TEST_F(SourceFileRelocatorTest, ComputeDestination_PathNavigation_Fails)
    {
        TestComputeDestination(m_data->m_scanFolder2, "folder/a/*", "a/b/c/*", "../a*", "", false);
    }

    TEST_F(SourceFileRelocatorTest, ComputeDestination_WildcardAcrossDirectories_Fails)
    {
        TestComputeDestination(m_data->m_scanFolder2, "folder/a/b/c/*", "*/c/*", "*/d/*", "", false);
    }

    TEST_F(SourceFileRelocatorTest, ComputeDestination_MismatchedWildcardCount_Fails)
    {
        TestComputeDestination(m_data->m_scanFolder2, "folder/a/b/c/*", "a/b/*/*", "*/d", "", false);
    }

    TEST_F(SourceFileRelocatorTest, ComputeDestination_InvalidCharacters_Fails)
    {
        TestComputeDestination(m_data->m_scanFolder2, "folder/a/b/c/*", "a/b/c/*", "d/*?", "", false);
        TestComputeDestination(m_data->m_scanFolder2, "folder/a/b/c/*", "a/b/c/*", "d/*<", "", false);
        TestComputeDestination(m_data->m_scanFolder2, "folder/a/b/c/*", "a/b/c/*", "d/*>", "", false);
        TestComputeDestination(m_data->m_scanFolder2, "folder/a/b/c/*", "a/b/c/*", "d/*\"", "", false);
        TestComputeDestination(m_data->m_scanFolder2, "folder/a/b/c/*", "a/b/c/*", "d/*|", "", false);
    }

    TEST_F(SourceFileRelocatorTest, ComputeDestination_Directory_Succeeds)
    {
        TestComputeDestination(m_data->m_scanFolder1, "dev/subfolder1/somefile.tif", "subfolder1/somefile.tif", "subfolder2/", "subfolder2/somefile.tif");
        TestComputeDestination(m_data->m_scanFolder1, "dev/subfolder1/s*", "subfolder1/s*", "subfolder2/", "subfolder2/somefile.tif");
        TestComputeDestination(m_data->m_scanFolder1, "dev/test.txt", "test.txt", "subfolder2/", "subfolder2/test.txt");
    }

    TEST_F(SourceFileRelocatorTest, BuildReport_Succeeds)
    {
        SourceFileRelocationContainer entryContainer;

        QDir tempPath(m_tempDir.path());
        const ScanFolderInfo* info = nullptr;
        const ScanFolderInfo* destInfo = nullptr;
        FileUpdateTasks updateTasks;

        ASSERT_TRUE(m_data->m_reporter->GetSourcesByPath(tempPath.absoluteFilePath("folder/*").toUtf8().constData(), entryContainer, info));
        ASSERT_EQ(entryContainer.size(), 2);

        m_data->m_reporter->ComputeDestination(entryContainer, info, "*", "someOtherPlace/*", destInfo);
        m_data->m_reporter->PopulateDependencies(entryContainer);
        AZStd::string report = m_data->m_reporter->BuildReport(entryContainer, updateTasks, true);

        ASSERT_FALSE(report.empty());
    }

    TEST_F(SourceFileRelocatorTest, Move_Preview_Succeeds)
    {
        QDir tempPath(m_tempDir.path());
        auto result = m_data->m_reporter->Move(tempPath.absoluteFilePath((m_data->m_scanFolder1.m_scanFolder + "/subfolder*").c_str()).toUtf8().constData(), "someOtherPlace/*", true);

        ASSERT_TRUE(result.IsSuccess());
        ASSERT_EQ(result.GetValue().m_relocationContainer.size(), 3);
    }

    TEST_F(SourceFileRelocatorTest, TestInterface)
    {
        auto* interface = AZ::Interface<ISourceFileRelocation>::Get();

        ASSERT_NE(interface, nullptr);
    }

    TEST_F(SourceFileRelocatorTest, Move_Real_Succeeds)
    {
        QDir tempPath(m_tempDir.path());

        auto filePath = QDir(tempPath.absoluteFilePath(m_data->m_scanFolder1.m_scanFolder.c_str())).absoluteFilePath("duplicate/file1.tif");
        auto newFilePath = QDir(tempPath.absoluteFilePath(m_data->m_scanFolder1.m_scanFolder.c_str())).absoluteFilePath("someOtherPlace/file1.tif");

        ASSERT_TRUE(AZ::IO::FileIOBase::GetInstance()->Exists(filePath.toUtf8().constData()));

        auto result = m_data->m_reporter->Move(filePath.toUtf8().constData(), "someOtherPlace/file1.tif", false);

        ASSERT_TRUE(result.IsSuccess());
        ASSERT_FALSE(AZ::IO::FileIOBase::GetInstance()->Exists(filePath.toUtf8().constData()));
        ASSERT_TRUE(AZ::IO::FileIOBase::GetInstance()->Exists(newFilePath.toUtf8().constData()));
    }

    TEST_F(SourceFileRelocatorTest, Move_Real_ReadOnlyFile_Fails)
    {
        QDir tempPath(m_tempDir.path());

        auto filePath = QDir(tempPath.absoluteFilePath(m_data->m_scanFolder1.m_scanFolder.c_str())).absoluteFilePath("duplicate/file1.tif");
        auto newFilePath = QDir(tempPath.absoluteFilePath(m_data->m_scanFolder1.m_scanFolder.c_str())).absoluteFilePath("someOtherPlace/file1.tif");

        ASSERT_TRUE(AZ::IO::FileIOBase::GetInstance()->Exists(filePath.toUtf8().constData()));
        ASSERT_TRUE(AZ::IO::SystemFile::SetWritable(filePath.toUtf8().constData(), false));

        AZ_TEST_START_TRACE_SUPPRESSION;
        auto result = m_data->m_reporter->Move(filePath.toUtf8().constData(), "someOtherPlace/file1.tif", false);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);

        ASSERT_TRUE(result.IsSuccess());
        ASSERT_TRUE(AZ::IO::FileIOBase::GetInstance()->Exists(filePath.toUtf8().constData()));
        ASSERT_FALSE(AZ::IO::FileIOBase::GetInstance()->Exists(newFilePath.toUtf8().constData()));
    }

    TEST_F(SourceFileRelocatorTest, Move_Real_WithDependencies_Fails)
    {
        QDir tempPath(m_tempDir.path());
        auto result = m_data->m_reporter->Move("subfolder1/otherfile.tif", "someOtherPlace/otherfile.tif", false);

        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(SourceFileRelocatorTest, Move_Real_WithDependenciesUpdateReferences_Succeeds)
    {
        QDir tempPath(m_tempDir.path());
        auto result = m_data->m_reporter->Move("subfolder1/otherfile.tif", "someOtherPlace/otherfile.tif", false, false, true, true);

        ASSERT_TRUE(result.IsSuccess());
    }

    TEST_F(SourceFileRelocatorTest, Delete_Real_Succeeds)
    {
        QDir tempPath(m_tempDir.path());

        auto filePath = QDir(tempPath.absoluteFilePath(m_data->m_scanFolder1.m_scanFolder.c_str())).absoluteFilePath("duplicate/file1.tif");
        
        ASSERT_TRUE(AZ::IO::FileIOBase::GetInstance()->Exists(filePath.toUtf8().constData()));

        auto result = m_data->m_reporter->Delete(filePath.toUtf8().constData(), false);

        ASSERT_TRUE(result.IsSuccess());
        ASSERT_FALSE(AZ::IO::FileIOBase::GetInstance()->Exists(filePath.toUtf8().constData()));
    }

    TEST_F(SourceFileRelocatorTest, Delete_Real_WithDependencies_Fails)
    {
        QDir tempPath(m_tempDir.path());
        auto result = m_data->m_reporter->Delete("subfolder1/otherfile.tif", false);

        ASSERT_FALSE(result.IsSuccess());
    }
}
