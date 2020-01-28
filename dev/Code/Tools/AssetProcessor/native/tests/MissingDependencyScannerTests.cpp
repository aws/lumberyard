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

#include <native/tests/AssetProcessorTest.h>
#include <AzToolsFramework/API/AssetDatabaseBus.h>
#include <native/utilities/MissingDependencyScanner.h>
#include <AssetDatabase/AssetDatabase.h>

namespace AssetProcessor
{
    class MockDatabaseLocationListener : public AzToolsFramework::AssetDatabase::AssetDatabaseRequests::Bus::Handler
    {
    public:
        MOCK_METHOD1(GetAssetDatabaseLocation, bool(AZStd::string&));
    };

    class MissingDependencyScannerTest
        : public AssetProcessorTest
    {
    public:
        MissingDependencyScannerTest()
        {

        }

    protected:
        void SetUp() override
        {
            using namespace testing;
            using ::testing::NiceMock;

            AssetProcessorTest::SetUp();

            m_errorAbsorber = nullptr;

            m_data = AZStd::make_unique<StaticData>();

            QDir tempPath(m_data->m_tempDir.path());

            m_data->m_databaseLocationListener.BusConnect();

            m_data->m_databaseLocation = tempPath.absoluteFilePath("test_database.sqlite").toUtf8().constData();

            ON_CALL(m_data->m_databaseLocationListener, GetAssetDatabaseLocation(_))
                .WillByDefault(
                    DoAll( // set the 0th argument ref (string) to the database location and return true.
                        SetArgReferee<0>(m_data->m_databaseLocation),
                        Return(true)));

            m_data->m_dbConn.OpenDatabase();

            m_data->m_scopedDir.Setup(tempPath.absolutePath());
        }

        void TearDown() override
        {
            m_data = nullptr;

            AssetProcessorTest::TearDown();
        }

        struct StaticData
        {
            QTemporaryDir m_tempDir;
            AZStd::string m_databaseLocation;
            ::testing::NiceMock<MockDatabaseLocationListener> m_databaseLocationListener;
            AssetDatabaseConnection m_dbConn;
            MissingDependencyScanner m_scanner;
            UnitTestUtils::ScopedDir m_scopedDir; // Sets up FileIO instance
        };

        AZStd::unique_ptr<StaticData> m_data;
    };

    TEST_F(MissingDependencyScannerTest, ScanFile_FindsValidReferenceToProduct)
    {
        using namespace AzToolsFramework::AssetDatabase;

        QDir tempPath(m_data->m_tempDir.path());
        QString testFilePath = tempPath.absoluteFilePath("subfolder1/assetProcessorManagerTest.txt");

        ASSERT_TRUE(UnitTestUtils::CreateDummyFile(testFilePath, "tests/1.product"));

        auto& db = m_data->m_dbConn;
        AZ::Uuid actualTestGuid;

        {
            // Create the referenced product
            ScanFolderDatabaseEntry scanFolder;
            scanFolder.m_displayName = "Test";
            scanFolder.m_portableKey = "Test";
            scanFolder.m_scanFolder = tempPath.absoluteFilePath("subfolder1").toUtf8().constData();
            ASSERT_TRUE(db.SetScanFolder(scanFolder));

            SourceDatabaseEntry sourceEntry;
            sourceEntry.m_sourceName = "tests/1";
            sourceEntry.m_sourceGuid = AssetUtilities::CreateSafeSourceUUIDFromName(sourceEntry.m_sourceName.c_str());
            sourceEntry.m_scanFolderPK = 1;
            ASSERT_TRUE(db.SetSource(sourceEntry));

            // Save off the guid for testing at the end
            actualTestGuid = sourceEntry.m_sourceGuid;

            JobDatabaseEntry jobEntry;
            jobEntry.m_sourcePK = sourceEntry.m_sourceID;
            jobEntry.m_platform = "pc";
            jobEntry.m_jobRunKey = 1;
            ASSERT_TRUE(db.SetJob(jobEntry));

            ProductDatabaseEntry productEntry;
            productEntry.m_jobPK = jobEntry.m_jobID;
            productEntry.m_productName = "pc/test/tests/1.product";
            ASSERT_TRUE(db.SetProduct(productEntry));
        }

        // Create the product that references the product above.  This represents the dummy file we created up above
        SourceDatabaseEntry sourceEntry;
        sourceEntry.m_sourceName = "tests/2";
        sourceEntry.m_sourceGuid = AssetUtilities::CreateSafeSourceUUIDFromName(sourceEntry.m_sourceName.c_str());
        sourceEntry.m_scanFolderPK = 1;
        ASSERT_TRUE(db.SetSource(sourceEntry));

        JobDatabaseEntry jobEntry;
        jobEntry.m_sourcePK = sourceEntry.m_sourceID;
        jobEntry.m_platform = "pc";
        jobEntry.m_jobRunKey = 2;
        ASSERT_TRUE(db.SetJob(jobEntry));

        ProductDatabaseEntry productEntry;
        productEntry.m_jobPK = jobEntry.m_jobID;
        productEntry.m_productName = "pc/test/tests/2.product";
        ASSERT_TRUE(db.SetProduct(productEntry));

        AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer container;

        m_data->m_scanner.ScanFile(testFilePath.toUtf8().constData(), AssetProcessor::MissingDependencyScanner::DefaultMaxScanIteration, productEntry.m_productID, container, &db);

        MissingProductDependencyDatabaseEntryContainer missingDeps;
        ASSERT_TRUE(db.GetMissingProductDependenciesByProductId(productEntry.m_productID, missingDeps));

        ASSERT_EQ(missingDeps.size(), 1);
        ASSERT_EQ(missingDeps[0].m_productPK, productEntry.m_productID);
        ASSERT_EQ(missingDeps[0].m_dependencySourceGuid, actualTestGuid);
    }
}
