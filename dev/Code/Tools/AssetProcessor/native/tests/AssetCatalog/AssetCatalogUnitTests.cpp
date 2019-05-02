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
#include <AzCore/base.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzTest/AzTest.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/AssetDatabaseBus.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Asset/AssetProcessorMessages.h>

#include <QCoreApplication>

#include <native/unittests/UnitTestRunner.h> // for UnitTestUtils like CreateDummyFile / AssertAbsorber.
#include <native/resourcecompiler/RCBuilder.h> // for defines like BUILDER_ID_RC

#include <native/AssetManager/AssetCatalog.h>

namespace AssetProcessor
{
    using namespace AZ;
    using namespace AZ::Data;
    using namespace testing;
    using ::testing::NiceMock;
    using namespace UnitTestUtils;
    using namespace AzFramework::AssetSystem;
    using namespace AzToolsFramework::AssetSystem;
    using namespace AzToolsFramework::AssetDatabase;

    class MockDatabaseLocationListener : public AzToolsFramework::AssetDatabase::AssetDatabaseRequests::Bus::Handler
    {
    public:
        MOCK_METHOD1(GetAssetDatabaseLocation, bool(AZStd::string&));
    };

    class AssetCatalogForUnitTest : public AssetProcessor::AssetCatalog
    {
    public:
        AssetCatalogForUnitTest(QObject* parent, AssetProcessor::PlatformConfiguration* platformConfiguration)
            : AssetCatalog(parent, platformConfiguration) {}
        
        // prevent automatic save on shutdown, no point in doing that in unit test mode, just wastes time.
        virtual ~AssetCatalogForUnitTest()
        {
            ClearDirtyFlag();
        }
        void ClearDirtyFlag()
        {
            m_catalogIsDirty = false;
        }
    protected:

    };

    class AssetCatalogTest
        : public ::testing::Test
    {
    protected:
        
        // store all data we create here so that it can be destroyed on shutdown before we remove allocators
        struct DataMembers
        {
            NiceMock<MockDatabaseLocationListener> m_databaseLocationListener;
            QTemporaryDir m_temporaryDir;
            QDir m_temporarySourceDir;
            QDir m_priorAssetRoot;
            AssetDatabaseConnection m_dbConn;
            UnitTestUtils::ScopedDir m_scopedDir;
            PlatformConfiguration m_config;
            AZStd::unique_ptr<AssetCatalogForUnitTest> m_assetCatalog;
            QDir m_cacheRootDir; // where the 'cache' folder lives.
            QString m_gameName;
            AssertAbsorber m_absorber;
            AZStd::string m_databaseLocation;
            QCoreApplication coreApp;
            int argc = 0;
            DataMembers() : coreApp(argc, nullptr)
            {
                
            }
        };

        DataMembers* m_data = nullptr;
        AZStd::unique_ptr<AZ::ComponentApplication> m_app; // the app is created seperately so that we can control its lifetime.

        void SetUp() override
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Create();

            m_app.reset(aznew AZ::ComponentApplication());
            AZ::ComponentApplication::Descriptor desc;
            desc.m_useExistingAllocator = true;
            m_app->Create(desc);

            m_data = azcreate(DataMembers, ());
            
            AssetUtilities::ComputeAssetRoot(m_data->m_priorAssetRoot);
            AssetUtilities::ResetAssetRoot();

            // the canonicalization of the path here is to get around the fact that on some platforms
            // the "temporary" folder location could be junctioned into some other folder and getting "QDir::current()"
            // and other similar functions may actually return a different string but still be referring to the same folder   
            m_data->m_temporarySourceDir = QDir(m_data->m_temporaryDir.path());
            QString canonicalTempDirPath = AssetUtilities::NormalizeDirectoryPath(m_data->m_temporarySourceDir.canonicalPath());
            m_data->m_temporarySourceDir = QDir(canonicalTempDirPath);
            m_data->m_scopedDir.Setup(m_data->m_temporarySourceDir.path());
            CreateDummyFile(m_data->m_temporarySourceDir.absoluteFilePath("bootstrap.cfg"), QString("sys_game_folder=SamplesProject\n"));
            m_data->m_gameName = AssetUtilities::ComputeGameName(); // uses the above file.

            AssetUtilities::ResetAssetRoot();
            QDir newRoot; // throwaway dummy var - we just want to invoke the below function
            AssetUtilities::ComputeAssetRoot(newRoot, &m_data->m_temporarySourceDir);

            AssetUtilities::ComputeProjectCacheRoot(m_data->m_cacheRootDir);
            QString normalizedCacheRoot = AssetUtilities::NormalizeDirectoryPath(m_data->m_cacheRootDir.absolutePath());
            m_data->m_cacheRootDir = QDir(normalizedCacheRoot);

            // create the files we'll use for this test:
            QSet<QString> expectedFiles;
            // set up some interesting files:
            expectedFiles << m_data->m_temporarySourceDir.absoluteFilePath("rootfile2.txt");
            expectedFiles << m_data->m_temporarySourceDir.absoluteFilePath("subfolder1/rootfile1.txt"); // note:  Must override the actual root file
            expectedFiles << m_data->m_temporarySourceDir.absoluteFilePath("subfolder1/basefile.txt");
            expectedFiles << m_data->m_temporarySourceDir.absoluteFilePath("subfolder2/basefile.txt");
            expectedFiles << m_data->m_temporarySourceDir.absoluteFilePath("subfolder2/aaa/basefile.txt");
            expectedFiles << m_data->m_temporarySourceDir.absoluteFilePath("subfolder2/aaa/bbb/basefile.txt");
            expectedFiles << m_data->m_temporarySourceDir.absoluteFilePath("subfolder2/aaa/bbb/ccc/basefile.txt");
            expectedFiles << m_data->m_temporarySourceDir.absoluteFilePath("subfolder2/aaa/bbb/ccc/ddd/basefile.txt");
            expectedFiles << m_data->m_temporarySourceDir.absoluteFilePath("subfolder3/BaseFile.txt"); // note the case upper here
            expectedFiles << m_data->m_temporarySourceDir.absoluteFilePath("subfolder8/a/b/c/test.txt");

            // subfolder3 is not recursive so none of these should show up in any scan or override check
            expectedFiles << m_data->m_temporarySourceDir.absoluteFilePath("subfolder3/aaa/basefile.txt");
            expectedFiles << m_data->m_temporarySourceDir.absoluteFilePath("subfolder3/aaa/bbb/basefile.txt");
            expectedFiles << m_data->m_temporarySourceDir.absoluteFilePath("subfolder3/aaa/bbb/ccc/basefile.txt");

            expectedFiles << m_data->m_temporarySourceDir.absoluteFilePath("subfolder3/uniquefile.txt"); // only exists in subfolder3
            expectedFiles << m_data->m_temporarySourceDir.absoluteFilePath("subfolder3/uniquefile.ignore"); // only exists in subfolder3

            expectedFiles << m_data->m_temporarySourceDir.absoluteFilePath("subfolder3/rootfile3.txt"); // must override rootfile3 in root
            expectedFiles << m_data->m_temporarySourceDir.absoluteFilePath("rootfile1.txt");
            expectedFiles << m_data->m_temporarySourceDir.absoluteFilePath("rootfile3.txt");
            expectedFiles << m_data->m_temporarySourceDir.absoluteFilePath("unrecognised.file"); // a file that should not be recognised
            expectedFiles << m_data->m_temporarySourceDir.absoluteFilePath("unrecognised2.file"); // a file that should not be recognised
            expectedFiles << m_data->m_temporarySourceDir.absoluteFilePath("subfolder1/test/test.format"); // a file that should be recognised
            expectedFiles << m_data->m_temporarySourceDir.absoluteFilePath("test.format"); // a file that should NOT be recognised
            expectedFiles << m_data->m_temporarySourceDir.absoluteFilePath("subfolder3/somefile.xxx");
            expectedFiles << m_data->m_temporarySourceDir.absoluteFilePath("subfolder3/savebackup/test.txt");//file that should be excluded
            expectedFiles << m_data->m_temporarySourceDir.absoluteFilePath("subfolder3/somerandomfile.random");

            for (const QString& expect : expectedFiles)
            {
                CreateDummyFile(expect);
            }

            // in other unit tests we may open the database called ":memory:" to use an in-memory database instead of one on disk.
            // in this test, however, we use a real database, because the catalog shares it and opens its own connection to it.
            // ":memory:" databases are one-instance-only, and even if another connection is opened to ":memory:" it would
            // not share with others created using ":memory:" and get a unique database instead.
            m_data->m_databaseLocation = m_data->m_temporarySourceDir.absoluteFilePath("test_database.sqlite").toUtf8().constData();

            ON_CALL(m_data->m_databaseLocationListener, GetAssetDatabaseLocation(_))
                .WillByDefault(
                    DoAll( // set the 0th argument ref (string) to the database location and return true.
                        SetArgReferee<0>(m_data->m_databaseLocation.c_str()),
                        Return(true)));

            m_data->m_databaseLocationListener.BusConnect();
            m_data->m_dbConn.OpenDatabase();
            
            BuildConfig(m_data->m_temporarySourceDir, &(m_data->m_dbConn), m_data->m_config);
            m_data->m_assetCatalog.reset(new AssetCatalogForUnitTest(nullptr, &(m_data->m_config)));
        }

        void TearDown() override
        {
            // if you EXPECT warnings/asserts/errors you need to check in your test, and you need to also
            // reset it before returning from your test.
            EXPECT_EQ(m_data->m_absorber.m_numAssertsAbsorbed, 0);
            EXPECT_EQ(m_data->m_absorber.m_numErrorsAbsorbed, 0);
            EXPECT_EQ(m_data->m_absorber.m_numWarningsAbsorbed, 0);
            AssetUtilities::ResetAssetRoot();

            azdestroy(m_data);
            m_app->Destroy();
            m_app.reset();
            AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
        }

        // -- utility functions to create default state data --

        // Adds a scan folder to the config and to the database
        void AddScanFolder(const ScanFolderInfo& scanFolderInfo, PlatformConfiguration& config, AssetDatabaseConnection* dbConn)
        {
            config.AddScanFolder(scanFolderInfo);
            ScanFolderDatabaseEntry newScanFolder(
                scanFolderInfo.ScanPath().toStdString().c_str(),
                scanFolderInfo.GetDisplayName().toStdString().c_str(),
                scanFolderInfo.GetPortableKey().toStdString().c_str(),
                scanFolderInfo.GetOutputPrefix().toStdString().c_str(),
                scanFolderInfo.IsRoot());
            dbConn->SetScanFolder(newScanFolder);
        }

        // build some default configs.
        void BuildConfig(const QDir& tempPath, AssetDatabaseConnection* dbConn, PlatformConfiguration& config)
        {
            config.EnablePlatform({ "pc" ,{ "desktop", "renderer" } }, true);
            config.EnablePlatform({ "es3" ,{ "mobile", "renderer" } }, true);
            config.EnablePlatform({ "fandango" ,{ "console", "renderer" } }, false);
            AZStd::vector<AssetBuilderSDK::PlatformInfo> platforms;
            config.PopulatePlatformsForScanFolder(platforms);
            //                                               PATH         DisplayName    PortKey      outputfolder root    recurse  platforms     order
            AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder4"), "subfolder4", "subfolder4", "", false, false, platforms, -6), config, dbConn); // subfolder 4 overrides subfolder3
            AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder3"), "subfolder3", "subfolder3", "", false, false, platforms, -5), config, dbConn); // subfolder 3 overrides subfolder2
            AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder2"), "subfolder2", "subfolder2", "", false, true, platforms, -2), config, dbConn); // subfolder 2 overrides subfolder1
            AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder1"), "subfolder1", "subfolder1", "", false, true, platforms, -1), config, dbConn); // subfolder1 overrides root
            AddScanFolder(ScanFolderInfo(tempPath.absolutePath(), "temp", "tempfolder", "", true, false, platforms, 0), config, dbConn); // add the root

            config.AddMetaDataType("exportsettings", QString());

            AZ::Uuid buildIDRcLegacy;
            BUILDER_ID_RC.GetUuid(buildIDRcLegacy);

            AssetRecognizer rec;
            AssetPlatformSpec specpc;
            AssetPlatformSpec speces3;

            speces3.m_extraRCParams = "somerandomparam";
            rec.m_name = "random files";
            rec.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.random", AssetBuilderSDK::AssetBuilderPattern::Wildcard);
            rec.m_platformSpecs.insert("pc", specpc);
            config.AddRecognizer(rec);

            specpc.m_extraRCParams = ""; // blank must work
            speces3.m_extraRCParams = "testextraparams";

            const char* builderTxt1Name = "txt files";
            rec.m_name = builderTxt1Name;
            rec.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.txt", AssetBuilderSDK::AssetBuilderPattern::Wildcard);
            rec.m_platformSpecs.insert("pc", specpc);
            rec.m_platformSpecs.insert("es3", speces3);

            config.AddRecognizer(rec);

            // Ignore recognizer
            AssetPlatformSpec ignore_spec;
            ignore_spec.m_extraRCParams = "skip";
            AssetRecognizer ignore_rec;
            ignore_rec.m_name = "ignore files";
            ignore_rec.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.ignore", AssetBuilderSDK::AssetBuilderPattern::Wildcard);
            ignore_rec.m_platformSpecs.insert("pc", specpc);
            ignore_rec.m_platformSpecs.insert("es3", ignore_spec);
            config.AddRecognizer(ignore_rec);

            ExcludeAssetRecognizer excludeRecogniser;
            excludeRecogniser.m_name = "backup";
            excludeRecogniser.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher(".*\\/savebackup\\/.*", AssetBuilderSDK::AssetBuilderPattern::Regex);
            config.AddExcludeRecognizer(excludeRecogniser);
        }

        // Adds a source file and job entry to the database, jobId is output
        bool AddSourceAndJob(const char* scanFolder, const char* sourceRelPath, AssetDatabaseConnection* dbConn, AZ::s64& jobId, AZ::Uuid assetId = AZ::Uuid::CreateRandom())
        {
            ScanFolderDatabaseEntry scanFolderEntry;
            bool result = dbConn->GetScanFolderByPortableKey(scanFolder, scanFolderEntry);

            if (!result)
            {
                return false;
            }

            SourceDatabaseEntry sourceEntry(scanFolderEntry.m_scanFolderID, sourceRelPath, assetId, "fingerprint1");
            dbConn->SetSource(sourceEntry);

            JobDatabaseEntry jobEntry(sourceEntry.m_sourceID, "test", 1234, "pc", assetId, AzToolsFramework::AssetSystem::JobStatus::Completed, 12345);
            dbConn->SetJob(jobEntry);

            jobId = jobEntry.m_jobID;
            return true;
        };

        // Calls the GetRelativeProductPathFromFullSourceOrProductPath function and checks the return results, returning true if it matches both of the expected results
        bool TestGetRelativeProductPath(const QString fileToCheck, bool expectedToFind, AZStd::initializer_list<const char*> expectedPaths)
        {
            bool relPathfound = false;
            AZStd::string relPath;
            AZStd::string fullPath(fileToCheck.toStdString().c_str());

            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(relPathfound, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetRelativeProductPathFromFullSourceOrProductPath, fullPath, relPath);

            if (relPathfound != expectedToFind)
            {
                return false;
            }

            for (auto& path : expectedPaths)
            {
                if (relPath == path)
                {
                    return true;
                }
            }

            return false;
        }

        // Calls the GetFullSourcePathFromRelativeProductPath function and checks the return results, returning true if it matches both of the expected results
        bool TestGetFullSourcePath(const QString& fileToCheck, const QDir& tempPath, bool expectToFind, const char* expectedPath)
        {
            bool fullPathfound = false;
            AZStd::string fullPath;
            AZStd::string relPath(fileToCheck.toStdString().c_str());

            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(fullPathfound, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetFullSourcePathFromRelativeProductPath, relPath, fullPath);

            if (fullPathfound != expectToFind)
            {
                return false;
            }

            QString output(fullPath.c_str());
            output.remove(0, tempPath.path().length() + 1); //adding one for the native separator

            return (output == expectedPath);
        }
    };

    class AssetCatalogTestWithProducts
        : public AssetCatalogTest
    {
    public:
        void SetUp() override
        {
            AssetCatalogTest::SetUp();

            // Add a source file with 4 products
            AZ::s64 jobId;
            bool result = AddSourceAndJob("subfolder3", "BaseFile.txt", &(m_data->m_dbConn), jobId);
            EXPECT_TRUE(result);

            AZ::u32 productSubId = 0;
            for (auto& relativeProductPath : { "subfolder3/basefilez.arc2", "subfolder3/basefileaz.azm2", "subfolder3/basefile.arc2", "subfolder3/basefile.azm2" })
            {
                ProductDatabaseEntry newProduct(jobId, productSubId++, m_data->m_cacheRootDir.relativeFilePath(relativeProductPath).toStdString().c_str(), AZ::Data::AssetType::CreateRandom());
                m_data->m_dbConn.SetProduct(newProduct);
            }
        }
    };

    TEST_F(AssetCatalogTest, EmptyConstructors_Sanity)
    {
        // make sure constructors do not crash or misbehave when given empty names
        QString fileToCheck = "";

        // empty requests should generate an assert since it is a programming error to call this API with bad data.
        // however, the app should not crash even if the assert is absorbed.
        GetRelativeProductPathFromFullSourceOrProductPathRequest request(fileToCheck.toUtf8().constData());
        ASSERT_EQ(m_data->m_absorber.m_numAssertsAbsorbed, 1);
        GetFullSourcePathFromRelativeProductPathRequest sourceRequest(fileToCheck.toUtf8().constData());
        ASSERT_EQ(m_data->m_absorber.m_numAssertsAbsorbed, 2);
        // reset the absorber before we leave this assert-test, so that it doesn't cause failure of the test itself
        m_data->m_absorber.Clear();
            
        ASSERT_TRUE(TestGetRelativeProductPath("", false, { "" }));
        ASSERT_TRUE(TestGetFullSourcePath("", m_data->m_temporarySourceDir, false, ""));
    }

    TEST_F(AssetCatalogTestWithProducts, GetRelativePath_GivenRootPath_ReturnsFailure)
    {
        // Failure case
#if defined(AZ_PLATFORM_WINDOWS)
        QString fileToCheck = "d:\\test.txt";
#else
        QString fileToCheck = "/test.txt"; // rooted
#endif
        ASSERT_TRUE(TestGetRelativeProductPath(fileToCheck, false, { fileToCheck.toStdString().c_str() }));
    }

    TEST_F(AssetCatalogTestWithProducts, RelativePath)
    {
        // feed it a relative path with a TAB in the front :)
        QString fileToCheck = "\test.txt";
        ASSERT_TRUE(TestGetRelativeProductPath(fileToCheck, true, { "\test.txt" }));
    }

    TEST_F(AssetCatalogTestWithProducts, GetRelativeProductPathFromFullSourceOrProductPath_WithGameName_ReturnsFileInGameFolder)
    {
        // feed it a product path with gamename and a platform name, returns it without gamename
        QString fileToCheck = m_data->m_cacheRootDir.filePath("pc/" + m_data->m_gameName + "/aaa/basefile.txt");
        ASSERT_TRUE(TestGetRelativeProductPath(fileToCheck, true, { "aaa/basefile.txt" }));
    }

    TEST_F(AssetCatalogTestWithProducts, GetRelativeProductPathFromFullSourceOrProductPath_WithoutGameName_ReturnsFileInRootFolder)
    {
        // feed it a product path without gamename, just the file name since its supposed to be a root file
        QString fileToCheck = m_data->m_cacheRootDir.filePath("pc/basefile.txt");
        ASSERT_TRUE(TestGetRelativeProductPath(fileToCheck, true, { "basefile.txt" }));
    }

    TEST_F(AssetCatalogTestWithProducts, GetRelativeProductPathFromFullSourceOrProductPath_BadCasingInPlatform_ReturnsRelativePath)
    {
        // feed it a product path with gamename but poor casing (test 1:  the pc platform is not matching case)
        QString fileToCheck = m_data->m_cacheRootDir.filePath("Pc/" + m_data->m_gameName + "/aaa/basefile.txt");
        ASSERT_TRUE(TestGetRelativeProductPath(fileToCheck, true, { "aaa/basefile.txt" }));
    }

    TEST_F(AssetCatalogTestWithProducts, GetRelativeProductPathFromFullSourceOrProductPath_BadCasingInGameName_ReturnsRelativePath)
    {
        //feed it a product path with gamename but poor casing (test 2:  the gameName is not matching case)
        QString fileToCheck = m_data->m_cacheRootDir.filePath("pc/" + m_data->m_gameName.toUpper() + "/aaa/basefile.txt");
        ASSERT_TRUE(TestGetRelativeProductPath(fileToCheck, true, { "aaa/basefile.txt" }));
    }

    TEST_F(AssetCatalogTestWithProducts, GetRelativeProductPathFromFullSourceOrProductPath_FolderName_ReturnsFolderNameOnly)
    {
        // feed it a product path that resolves to a directory name instead of a file.  GameName is 'incorrect' (upper)
        QString fileToCheck = m_data->m_cacheRootDir.filePath("pc/" + m_data->m_gameName.toUpper() + "/aaa");
        ASSERT_TRUE(TestGetRelativeProductPath(fileToCheck, true, { "aaa" }));
    }

    TEST_F(AssetCatalogTestWithProducts, GetRelativeProductPathFromFullSourceOrProductPath_FolderNameExtraSlash_ReturnsFolderNameOnlyNoExtraSlash)
    {
        //  make sure it doesn't keep any trailing slashes.  GameName is 'incorrect' (upper)
        QString fileToCheck = m_data->m_cacheRootDir.filePath("pc/" + m_data->m_gameName.toUpper() + "/aaa/"); // extra trailing slash
        ASSERT_TRUE(TestGetRelativeProductPath(fileToCheck, true, { "aaa" })); // the API should never result in a trailing slash
    }

    TEST_F(AssetCatalogTestWithProducts, GetRelativeProductPathFromFullSourceOrProductPath_FolderNameExtraWrongWaySlash_ReturnsFolderNameOnlyNoExtraWrongSlash)
    {
        //  make sure it doesn't keep any trailing slashes.  GameName is 'incorrect' (upper)
        QString fileToCheck = m_data->m_cacheRootDir.filePath("pc/" + m_data->m_gameName.toUpper() + "/aaa\\"); // extra trailing wrongway slash
        ASSERT_TRUE(TestGetRelativeProductPath(fileToCheck, true, { "aaa" })); // the API should never result in a trailing slash
    }

    TEST_F(AssetCatalogTestWithProducts, GetRelativeProductPathFromFullSourceOrProductPath_RelativeDirectoryNameWhichDoesNotExist_ReturnsFolderNameOnly)
    {
        QString fileToCheck = m_data->m_cacheRootDir.filePath("pc/" + m_data->m_gameName.toLower() + "/nonexistantfolder"); // extra trailing wrongway slash
        ASSERT_TRUE(TestGetRelativeProductPath(fileToCheck, true, { "nonexistantfolder" })); 
    }

    TEST_F(AssetCatalogTestWithProducts, GetRelativeProductPathFromFullSourceOrProductPath_RelativeDirectoryNameWhichDoesNotExistWithExtraSlash_ReturnsFolderNameOnly)
    {
        QString fileToCheck = m_data->m_cacheRootDir.filePath("pc/" + m_data->m_gameName.toLower() + "/nonexistantfolder/"); // extra trailing wrongway slash
        ASSERT_TRUE(TestGetRelativeProductPath(fileToCheck, true, { "nonexistantfolder" }));
    }

    TEST_F(AssetCatalogTestWithProducts, GetRelativeProductPathFromFullSourceOrProductPath_RelativeDirectoryNameWhichDoesNotExistWithExtraWrongWaySlash_ReturnsFolderNameOnly)
    {
        QString fileToCheck = m_data->m_cacheRootDir.filePath("pc\\" + m_data->m_gameName.toLower() + "\\nonexistantfolder\\"); // extra trailing wrongway slash
        ASSERT_TRUE(TestGetRelativeProductPath(fileToCheck, true, { "nonexistantfolder" }));
    }

    TEST_F(AssetCatalogTestWithProducts, GetRelativeProductPathFromFullSourceOrProductPath_RelativePathToSourceFile_ReturnsProductFilePath)
    {
        QString fileToCheck = m_data->m_temporarySourceDir.absoluteFilePath("subfolder3/BaseFile.txt");
        ASSERT_TRUE(TestGetRelativeProductPath(fileToCheck, true, { "basefilez.arc2", "basefileaz.azm2", "basefile.arc2", "basefile.azm2" }));
    }

    TEST_F(AssetCatalogTestWithProducts, GetRelativeProductPathFromFullSourceOrProductPath_RelativePathToSourceFile_BadCasing_ReturnsProductFilePath)
    {
        // note that the casing of the source file is not correct.  It must still work.
        QString fileToCheck = m_data->m_temporarySourceDir.absoluteFilePath("subfolder2/aaa/basefile.txt");
        ASSERT_TRUE(TestGetRelativeProductPath(fileToCheck, true, { "aaa/basefile.txt" }));
    }

    class AssetCatalogTest_GetFullSourcePath
        : public AssetCatalogTest
    {
    public:
        void SetUp() override
        {
            AssetCatalogTest::SetUp();

            // ----- Test the ProcessGetFullAssetPath function on product files
            {
                QStringList pcouts;
                pcouts.push_back(m_data->m_cacheRootDir.filePath(QString("pc/") + m_data->m_gameName + "/subfolder3/randomfileoutput.random"));
                pcouts.push_back(m_data->m_cacheRootDir.filePath(QString("pc/") + m_data->m_gameName + "/subfolder3/randomfileoutput.random1"));
                pcouts.push_back(m_data->m_cacheRootDir.filePath(QString("pc/") + m_data->m_gameName + "/subfolder3/randomfileoutput.random2"));

                AZ::s64 jobId;
                ASSERT_TRUE(AddSourceAndJob("subfolder3", "somerandomfile.random", &(m_data->m_dbConn), jobId));

                AZ::u32 productSubID = 0;
                for (auto& product : pcouts)
                {
                    ProductDatabaseEntry newProduct(jobId, productSubID++, m_data->m_cacheRootDir.relativeFilePath(product).toStdString().c_str(), AZ::Data::AssetType::CreateRandom());
                    ASSERT_TRUE(m_data->m_dbConn.SetProduct(newProduct));
                }
            }
        }
    };

    TEST_F(AssetCatalogTest_GetFullSourcePath, NormalUsage_ReturnsAbsolutePathToSource)
    {
        // feed it an relative product, and expect a full, absolute source file path in return.
        QString fileToCheck = "subfolder3/randomfileoutput.random1";
        EXPECT_TRUE(TestGetFullSourcePath(fileToCheck, m_data->m_temporarySourceDir, true, "subfolder3/somerandomfile.random"));
    }

    TEST_F(AssetCatalogTest_GetFullSourcePath, SecondProduct_ReturnsAbsolutePathToSource)
    {
        // feed it another relative product from the same source file, expect the same source.
        QString fileToCheck = "subfolder3/randomfileoutput.random2";
        EXPECT_TRUE(TestGetFullSourcePath(fileToCheck, m_data->m_temporarySourceDir, true, "subfolder3/somerandomfile.random"));
    }

    TEST_F(AssetCatalogTest_GetFullSourcePath, IncorrectSeperators_ReturnsAbsolutePathToSource)
    {
        //feed it the same relative product with different separators
        QString fileToCheck = "subfolder3\\randomfileoutput.random2";
        EXPECT_TRUE(TestGetFullSourcePath(fileToCheck, m_data->m_temporarySourceDir, true, "subfolder3/somerandomfile.random"));
    }

    TEST_F(AssetCatalogTest_GetFullSourcePath, SourcePath_ReturnsSourcePath)
    {
        // feed it a full path to a source file, we expect that path back
        QString fileToCheck = m_data->m_temporarySourceDir.filePath("somefolder/somefile.txt");
        EXPECT_TRUE(TestGetFullSourcePath(fileToCheck, m_data->m_temporarySourceDir, true, "somefolder/somefile.txt"));
    }

    TEST_F(AssetCatalogTest_GetFullSourcePath, AliasedCachePath_ReturnsAbsolutePathToSource)
    {
        //feed it a path with alias and asset id
        QString fileToCheck = "@assets@/subfolder3/randomfileoutput.random1";
        EXPECT_TRUE(TestGetFullSourcePath(fileToCheck, m_data->m_temporarySourceDir, true, "subfolder3/somerandomfile.random"));
    }

    TEST_F(AssetCatalogTest_GetFullSourcePath, InvalidAlias_ReturnsAbsolutePathToSource)
    {
        //feed it a path with some random alias and asset id
        QString fileToCheck = "@somerandomalias@/subfolder3/randomfileoutput.random1";
        EXPECT_TRUE(TestGetFullSourcePath(fileToCheck, m_data->m_temporarySourceDir, true, "subfolder3/somerandomfile.random"));
    }
    
    TEST_F(AssetCatalogTest_GetFullSourcePath, InvalidAliasMissingSeperator_ReturnsAbsolutePathToSource)
    {
        //feed it a path with some random alias and asset id but no separator
        QString fileToCheck = "@somerandomalias@subfolder3/randomfileoutput.random1";
        EXPECT_TRUE(TestGetFullSourcePath(fileToCheck, m_data->m_temporarySourceDir, true, "subfolder3/somerandomfile.random"));
    }
    
    TEST_F(AssetCatalogTest_GetFullSourcePath, InvalidSourcePathContainingCacheAlias_ReturnsAbsolutePathToSource)
    {
        //feed it a path with alias and input name
        QString fileToCheck = "@assets@/somerandomfile.random";
        EXPECT_TRUE(TestGetFullSourcePath(fileToCheck, m_data->m_temporarySourceDir, true, "subfolder3/somerandomfile.random"));
    }
    
    TEST_F(AssetCatalogTest_GetFullSourcePath, AbsolutePathToCache_ReturnsAbsolutePathToSource)
    {
        //feed it an absolute path with cacheroot
        QString fileToCheck = m_data->m_cacheRootDir.filePath("pc/" + m_data->m_gameName.toLower() + "/subfolder3/randomfileoutput.random1");
        EXPECT_TRUE(TestGetFullSourcePath(fileToCheck, m_data->m_temporarySourceDir, true, "subfolder3/somerandomfile.random"));
    }
    
    TEST_F(AssetCatalogTest_GetFullSourcePath, ProductNameIncludingPlatformAndGameName_ReturnsAbsolutePathToSource)
    {
        //feed it a productName directly
        QString fileToCheck = "pc/" + m_data->m_gameName + "/subfolder3/randomfileoutput.random1";
        EXPECT_TRUE(TestGetFullSourcePath(fileToCheck, m_data->m_temporarySourceDir, true, "subfolder3/somerandomfile.random"));
    }

    class AssetCatalogTest_AssetInfo
        : public AssetCatalogTest
    {
    public:
       
        struct AssetCatalogTest_AssetInfo_DataMembers
        {
            AssetId m_assetA = AssetId(Uuid::CreateRandom(), 0);
            Uuid m_assetALegacyUuid = Uuid::CreateRandom();
            AssetType m_assetAType = AssetType::CreateRandom();
            AZStd::string m_assetAFileFilter  = "*.source";
            AZStd::string m_subfolder1AbsolutePath;
            AZStd::string m_assetASourceRelPath = "assetA.source";
            AZStd::string m_assetAProductRelPath = "assetA.product";
            AZStd::string m_assetAFullPath;
            AZStd::string m_assetAProductFullPath;
            AZStd::string m_assetTestString    = "Its the Asset A";
            AZStd::string m_productTestString  = "Its a product A";
        };

        AssetCatalogTest_AssetInfo_DataMembers* m_customDataMembers = nullptr;

        void SetUp() override
        {
            AssetCatalogTest::SetUp();
            m_customDataMembers = azcreate(AssetCatalogTest_AssetInfo_DataMembers, ());
            m_customDataMembers->m_subfolder1AbsolutePath = m_data->m_temporarySourceDir.absoluteFilePath("subfolder1").toStdString().c_str();

            AzFramework::StringFunc::Path::Join(m_customDataMembers->m_subfolder1AbsolutePath.c_str(), m_customDataMembers->m_assetASourceRelPath.c_str(), m_customDataMembers->m_assetAFullPath);
            CreateDummyFile(QString::fromUtf8(m_customDataMembers->m_assetAFullPath.c_str()), m_customDataMembers->m_assetTestString.c_str());
            
            AzFramework::StringFunc::Path::Join(m_data->m_cacheRootDir.absolutePath().toUtf8().constData(), m_customDataMembers->m_assetAProductRelPath.c_str(), m_customDataMembers->m_assetAProductFullPath);
            CreateDummyFile(QString::fromUtf8(m_customDataMembers->m_assetAProductFullPath.c_str()), m_customDataMembers->m_productTestString.c_str());
        }

        bool GetAssetInfoById(bool expectedResult, AZStd::string expectedRelPath, AZStd::string expectedRootPath, AssetType assetType)
        {
            bool result = false;
            AssetInfo assetInfo;
            AZStd::string rootPath;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(result, &AssetSystemRequest::GetAssetInfoById, m_customDataMembers->m_assetA, assetType, assetInfo, rootPath);

            if (result != expectedResult)
            {
                return false;
            }

            if (expectedResult)
            {
                return (assetInfo.m_assetId == m_customDataMembers->m_assetA)
                    && (assetInfo.m_assetType == m_customDataMembers->m_assetAType)
                    && (assetInfo.m_relativePath == expectedRelPath)
                    && (assetInfo.m_sizeBytes == m_customDataMembers->m_assetTestString.size())
                    && (rootPath == expectedRootPath);
            }

            return true;
        };

        bool GetAssetInfoByIdPair(bool expectedResult, AZStd::string expectedRelPath, AZStd::string expectedRootPath)
        {
            // First test without providing the assetType
            bool result = GetAssetInfoById(expectedResult, expectedRelPath, expectedRootPath, AssetType::CreateNull());

            // If successful, test again, this time providing the assetType
            if (result)
            {
                result = GetAssetInfoById(expectedResult, expectedRelPath, expectedRootPath, m_customDataMembers->m_assetAType);
            }

            return result;
        };

        bool GetSourceInfoBySourcePath(
            bool expectedResult,
            const AZStd::string& sourcePath,
            const AZ::Uuid& expectedUuid,
            const AZStd::string& expectedRelPath,
            const AZStd::string& expectedRootPath,
            const AZ::Data::AssetType& expectedAssetType = AZ::Data::s_invalidAssetType) // sources will have an asset type if registered using AzToolsFramework::ToolsAssetSystemRequests::RegisterSourceAssetType
        {
            bool result = false;
            AssetInfo assetInfo;
            AZStd::string rootPath;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(result, &AssetSystemRequest::GetSourceInfoBySourcePath, sourcePath.c_str(), assetInfo, rootPath);

            if (result != expectedResult)
            {
                return false;
            }

            if (expectedResult)
            {
                return (assetInfo.m_assetId == expectedUuid)
                    && (assetInfo.m_assetType == expectedAssetType)
                    && (assetInfo.m_relativePath == expectedRelPath)
                    && (assetInfo.m_sizeBytes == m_customDataMembers->m_assetTestString.size())
                    && (rootPath == expectedRootPath);
            }

            return true;
        };

        void TearDown() override 
        {
            azdestroy(m_customDataMembers);
            AssetCatalogTest::TearDown();
        }

    };

    TEST_F(AssetCatalogTest_AssetInfo, Sanity_InvalidCalls)
    {
        //Test 1: Asset not in database
        EXPECT_TRUE(GetAssetInfoByIdPair(false, "", ""));
        EXPECT_TRUE(GetSourceInfoBySourcePath(false, "", AZ::Uuid::CreateNull(), "", ""));
    }

    TEST_F(AssetCatalogTest_AssetInfo, FindAssetNotRegisteredAsSource_FindsProduct)
    {
        // Setup: Add asset to database
        AZ::s64 jobId;
        EXPECT_TRUE(AddSourceAndJob("subfolder1", m_customDataMembers->m_assetASourceRelPath.c_str(), &(m_data->m_dbConn), jobId, m_customDataMembers->m_assetA.m_guid));
        ProductDatabaseEntry newProductEntry(jobId, 0, m_customDataMembers->m_assetAProductRelPath.c_str(), m_customDataMembers->m_assetAType);
        m_data->m_dbConn.SetProduct(newProductEntry);

        // Test 2: Asset in database, not registered as source asset
        // note that when asking for products, a performance improvement causes the catalog to use its REGISTRY
        // rather than the database to ask for products, so to set this up the registry must be present and must have the asset registered within it
        AzFramework::AssetSystem::AssetNotificationMessage message(m_customDataMembers->m_assetAProductRelPath.c_str(), AssetNotificationMessage::AssetChanged, m_customDataMembers->m_assetAType);
        message.m_sizeBytes = m_customDataMembers->m_productTestString.size();
        message.m_assetId = AZ::Data::AssetId(m_customDataMembers->m_assetA.m_guid, 0);
        m_data->m_assetCatalog->OnAssetMessage("pc", message);

        // also of note:  When looking up products, you don't get a root path since they are all in the cache.
        // its important here that we specifically get an empty root path.
        EXPECT_TRUE(GetAssetInfoByIdPair(true, m_customDataMembers->m_assetAProductRelPath, ""));

        // this call has to work with both full and relative path.
        EXPECT_TRUE(GetSourceInfoBySourcePath(true, m_customDataMembers->m_assetASourceRelPath.c_str(), m_customDataMembers->m_assetA.m_guid, m_customDataMembers->m_assetASourceRelPath.c_str(), m_customDataMembers->m_subfolder1AbsolutePath.c_str()));
        EXPECT_TRUE(GetSourceInfoBySourcePath(true, m_customDataMembers->m_assetAFullPath.c_str(), m_customDataMembers->m_assetA.m_guid, m_customDataMembers->m_assetASourceRelPath.c_str(), m_customDataMembers->m_subfolder1AbsolutePath.c_str()));
    }

    TEST_F(AssetCatalogTest_AssetInfo, FindAssetInBuildQueue_FindsSource)
    {
        // Setup:  Add a source to queue.
        m_data->m_assetCatalog->OnSourceQueued(m_customDataMembers->m_assetA.m_guid, m_customDataMembers->m_assetALegacyUuid, m_customDataMembers->m_subfolder1AbsolutePath.c_str(), m_customDataMembers->m_assetASourceRelPath.c_str());

        // TEST: Asset in queue, not registered as source asset
        EXPECT_TRUE(GetAssetInfoByIdPair(false, "", ""));

        // this call should STILL work even after the above call to "OnSourceQueued" since its explicitly asking for the source details.
        EXPECT_TRUE(GetSourceInfoBySourcePath(true, m_customDataMembers->m_assetASourceRelPath.c_str(), m_customDataMembers->m_assetA.m_guid, m_customDataMembers->m_assetASourceRelPath.c_str(), m_customDataMembers->m_subfolder1AbsolutePath.c_str()));
        EXPECT_TRUE(GetSourceInfoBySourcePath(true, m_customDataMembers->m_assetAFullPath.c_str(), m_customDataMembers->m_assetA.m_guid, m_customDataMembers->m_assetASourceRelPath.c_str(), m_customDataMembers->m_subfolder1AbsolutePath.c_str()));
    }

    TEST_F(AssetCatalogTest_AssetInfo, FindAssetInBuildQueue_RegisteredAsSourceType_StillFindsSource)
    {
        // Setup:  Add a source to queue.
        m_data->m_assetCatalog->OnSourceQueued(m_customDataMembers->m_assetA.m_guid, m_customDataMembers->m_assetALegacyUuid, m_customDataMembers->m_subfolder1AbsolutePath.c_str(), m_customDataMembers->m_assetASourceRelPath.c_str());

        // Register as source type
        AzToolsFramework::ToolsAssetSystemBus::Broadcast(&AzToolsFramework::ToolsAssetSystemRequests::RegisterSourceAssetType, m_customDataMembers->m_assetAType, m_customDataMembers->m_assetAFileFilter.c_str());

        // Test: Asset in queue, registered as source asset
        EXPECT_TRUE(GetAssetInfoByIdPair(true, m_customDataMembers->m_assetASourceRelPath, m_customDataMembers->m_subfolder1AbsolutePath));

        // these calls are identical to the two in the prior test, but should continue to work even though we have registered the asset type as a source asset type.
        EXPECT_TRUE(GetSourceInfoBySourcePath(true, m_customDataMembers->m_assetASourceRelPath.c_str(), m_customDataMembers->m_assetA.m_guid, m_customDataMembers->m_assetASourceRelPath.c_str(), m_customDataMembers->m_subfolder1AbsolutePath.c_str(), m_customDataMembers->m_assetAType));
        EXPECT_TRUE(GetSourceInfoBySourcePath(true, m_customDataMembers->m_assetAFullPath.c_str(), m_customDataMembers->m_assetA.m_guid, m_customDataMembers->m_assetASourceRelPath.c_str(), m_customDataMembers->m_subfolder1AbsolutePath.c_str(), m_customDataMembers->m_assetAType));
    }

    TEST_F(AssetCatalogTest_AssetInfo, FindSource_FinishedProcessing_RegisteredAsSource_FindsSource)
    {
        // Register as source type
        AzToolsFramework::ToolsAssetSystemBus::Broadcast(&AzToolsFramework::ToolsAssetSystemRequests::RegisterSourceAssetType, m_customDataMembers->m_assetAType, m_customDataMembers->m_assetAFileFilter.c_str());

        // Setup:  Add a source to queue, then notify its finished and add it to the database (simulates a full pipeline)
        AZ::s64 jobId;
        EXPECT_TRUE(AddSourceAndJob("subfolder1", m_customDataMembers->m_assetASourceRelPath.c_str(), &(m_data->m_dbConn), jobId, m_customDataMembers->m_assetA.m_guid));
        m_data->m_assetCatalog->OnSourceQueued(m_customDataMembers->m_assetA.m_guid, m_customDataMembers->m_assetALegacyUuid, m_customDataMembers->m_subfolder1AbsolutePath.c_str(), m_customDataMembers->m_assetASourceRelPath.c_str());
        m_data->m_assetCatalog->OnSourceFinished(m_customDataMembers->m_assetA.m_guid, m_customDataMembers->m_assetALegacyUuid);
        ProductDatabaseEntry assetAEntry(jobId, 0, m_customDataMembers->m_assetAProductRelPath.c_str(), m_customDataMembers->m_assetAType);
        m_data->m_dbConn.SetProduct(assetAEntry);

        // TEST: Asset in database, registered as source asset
        EXPECT_TRUE(GetAssetInfoByIdPair(true, m_customDataMembers->m_assetASourceRelPath, m_customDataMembers->m_subfolder1AbsolutePath));

        // at this point the details about the asset in question is no longer in memory, only the database.  However, these calls should continue find the
        // information, because the system is supposed check both the database AND the in-memory queue in the to find the info being requested.
        EXPECT_TRUE(GetSourceInfoBySourcePath(true, m_customDataMembers->m_assetASourceRelPath.c_str(), m_customDataMembers->m_assetA.m_guid, m_customDataMembers->m_assetASourceRelPath.c_str(), m_customDataMembers->m_subfolder1AbsolutePath.c_str(), m_customDataMembers->m_assetAType));
        EXPECT_TRUE(GetSourceInfoBySourcePath(true, m_customDataMembers->m_assetAFullPath.c_str(), m_customDataMembers->m_assetA.m_guid, m_customDataMembers->m_assetASourceRelPath.c_str(), m_customDataMembers->m_subfolder1AbsolutePath.c_str(), m_customDataMembers->m_assetAType));
    }

} // namespace AssetProcessor




