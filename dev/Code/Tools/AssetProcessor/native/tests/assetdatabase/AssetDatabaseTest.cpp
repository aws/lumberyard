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

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzToolsFramework/API/AssetDatabaseBus.h>

#include <QString>
#include <QFile>
#include <QDir>
#include <QTemporaryDir>

#include <native/tests/AssetProcessorTest.h>
#include <native/AssetDatabase/AssetDatabase.h>

namespace UnitTests
{
    using namespace testing;
    using ::testing::NiceMock;
    using namespace AssetProcessor;
    
    using AzToolsFramework::AssetDatabase::ProductDatabaseEntry;
    using AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry;
    using AzToolsFramework::AssetDatabase::SourceDatabaseEntry;
    using AzToolsFramework::AssetDatabase::JobDatabaseEntry;
    using AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer;
    using AzToolsFramework::AssetDatabase::AssetDatabaseConnection;
    
    class MockDatabaseLocationListener : public AzToolsFramework::AssetDatabase::AssetDatabaseRequests::Bus::Handler
    {
    public:
        MOCK_METHOD1(GetAssetDatabaseLocation, bool(AZStd::string&));
    };

    class AssetDatabaseTest : public AssetProcessorTest
    {
    public:
        void SetUp() override
        {
            AssetProcessorTest::SetUp();
            m_data.reset(new StaticData());
            m_data->m_databaseLocation = ":memory:"; // this special string causes SQLITE to open the database in memory and not touch disk at all.
            m_data->m_databaseLocationListener.BusConnect();
            
            ON_CALL(m_data->m_databaseLocationListener, GetAssetDatabaseLocation(_))
                .WillByDefault( 
                    DoAll( // set the 0th argument ref (string) to the database location and return true.
                        SetArgReferee<0>(":memory:"), 
                        Return(true)));

            // Initialize the database:
            m_data->m_connection.ClearData(); // this is expected to reset/clear/reopen
        }

        void TearDown() override
        {
            m_data->m_databaseLocationListener.BusDisconnect();
            m_data.reset();
            AssetProcessorTest::TearDown();
        }

        // COVERAGE TEST
        // For each of these coverage tests we'll start with the same kind of database, one with
        // SCAN FOLDER:          rootportkey
        //        SOURCE:            somefile.tif
        //             JOB:              "some job key"  runkey: 1   "pc"  SUCCEEDED
        //                 Product:         "someproduct1.dds"  subid: 1
        //                 Product:         "someproduct2.dds"  subid: 2
        //        SOURCE:            otherfile.tif
        //             JOB:              "some other job key"   runkey: 2  "osx" FAILED
        //                 Product:         "someproduct3.dds"  subid: 3
        //                 Product:         "someproduct4.dds"  subid: 4
        void CreateCoverageTestData()
        {
            m_data->m_scanFolder = { "c:/lumberyard/dev", "dev", "rootportkey", "" };
            ASSERT_TRUE(m_data->m_connection.SetScanFolder(m_data->m_scanFolder));
            
            m_data->m_sourceFile1 = { m_data->m_scanFolder.m_scanFolderID, "somefile.tif", AZ::Uuid::CreateRandom() };
            m_data->m_sourceFile2 = { m_data->m_scanFolder.m_scanFolderID, "otherfile.tif", AZ::Uuid::CreateRandom() };
            ASSERT_TRUE(m_data->m_connection.SetSource(m_data->m_sourceFile1));
            ASSERT_TRUE(m_data->m_connection.SetSource(m_data->m_sourceFile2));
            
            m_data->m_job1 = { m_data->m_sourceFile1.m_sourceID, "some job key", 123, "pc", AZ::Uuid::CreateRandom(), AzToolsFramework::AssetSystem::JobStatus::Completed, 1 };
            m_data->m_job2 = { m_data->m_sourceFile2.m_sourceID, "some other job key", 345, "osx", AZ::Uuid::CreateRandom(), AzToolsFramework::AssetSystem::JobStatus::Failed, 2 };
            ASSERT_TRUE(m_data->m_connection.SetJob(m_data->m_job1));
            ASSERT_TRUE(m_data->m_connection.SetJob(m_data->m_job2));

            m_data->m_product1 = { m_data->m_job1.m_jobID, 1, "someproduct1.dds", AZ::Data::AssetType::CreateRandom() };
            m_data->m_product2 = { m_data->m_job1.m_jobID, 2, "someproduct2.dds", AZ::Data::AssetType::CreateRandom() };
            m_data->m_product3 = { m_data->m_job2.m_jobID, 3, "someproduct3.dds", AZ::Data::AssetType::CreateRandom() };
            m_data->m_product4 = { m_data->m_job2.m_jobID, 4, "someproduct4.dds", AZ::Data::AssetType::CreateRandom() };

            ASSERT_TRUE(m_data->m_connection.SetProduct(m_data->m_product1));
            ASSERT_TRUE(m_data->m_connection.SetProduct(m_data->m_product2));
            ASSERT_TRUE(m_data->m_connection.SetProduct(m_data->m_product3));
            ASSERT_TRUE(m_data->m_connection.SetProduct(m_data->m_product4));
        }

    protected:

        struct StaticData
        {
            // these variables are created during SetUp() and destroyed during TearDown() and thus are always available during tests using this fixture:
            AZStd::string m_databaseLocation;
            NiceMock<MockDatabaseLocationListener> m_databaseLocationListener;
            AssetProcessor::AssetDatabaseConnection m_connection;

            // The following database entry variables are initialized only when you call coverage test data CreateCoverageTestData().
            // Tests which don't need or want a pre-made database should not call CreateCoverageTestData() but note that in that case
            // these entries will be empty and their identifiers will be -1.
            ScanFolderDatabaseEntry m_scanFolder;
            SourceDatabaseEntry m_sourceFile1;
            SourceDatabaseEntry m_sourceFile2;
            JobDatabaseEntry m_job1;
            JobDatabaseEntry m_job2;
            ProductDatabaseEntry m_product1;
            ProductDatabaseEntry m_product2;
            ProductDatabaseEntry m_product3;
            ProductDatabaseEntry m_product4;
            
        };

        // we store the above data in a unique_ptr so that its memory can be cleared during TearDown() in one call, before we destroy the memory
        // allocator, reducing the chance of missing or forgetting to destroy one in the future.
        AZStd::unique_ptr<StaticData> m_data;
    };

    TEST_F(AssetDatabaseTest, GetProducts_WithEmptyDatabase_Fails_ReturnsNoProducts)
    {
        ProductDatabaseEntryContainer products;
        EXPECT_FALSE(m_data->m_connection.GetProducts(products));
        EXPECT_EQ(products.size(), 0);
    }

    TEST_F(AssetDatabaseTest, GetProductByProductID_NotFound_Fails_ReturnsNoProducts)
    {
        AzToolsFramework::AssetDatabase::ProductDatabaseEntry product;
        EXPECT_FALSE(m_data->m_connection.GetProductByProductID(3443, product));
        EXPECT_EQ(product, AzToolsFramework::AssetDatabase::ProductDatabaseEntry());
    }

    TEST_F(AssetDatabaseTest, GetProductsLikeProductName_NotFound_Fails_ReturnsNoProducts)
    {
        ProductDatabaseEntryContainer products;
        EXPECT_FALSE(m_data->m_connection.GetProductsLikeProductName("none", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::Raw, products));
        EXPECT_EQ(products.size(), 0);
        
        EXPECT_FALSE(m_data->m_connection.GetProductsLikeProductName("none", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::StartsWith, products));
        EXPECT_EQ(products.size(), 0);
        
        EXPECT_FALSE(m_data->m_connection.GetProductsLikeProductName("none", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::EndsWith, products));
        EXPECT_EQ(products.size(), 0);
        
        EXPECT_FALSE(m_data->m_connection.GetProductsLikeProductName("none", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::Matches, products));
        EXPECT_EQ(products.size(), 0);
    }

    TEST_F(AssetDatabaseTest, GetProductsBySourceID_NotFound_Fails_ReturnsNoProducts)
    {
        ProductDatabaseEntryContainer products;
        EXPECT_FALSE(m_data->m_connection.GetProductsBySourceID(25654, products));
        EXPECT_EQ(products.size(), 0);
    }

    TEST_F(AssetDatabaseTest, SetProduct_InvalidProductID_Fails)
    {
        // trying to "overwrite" a product that does not exist should fail and emit error.
        AZ::Data::AssetType validAssetType1 = AZ::Data::AssetType::CreateRandom();
        ProductDatabaseEntry product { 123213, 234234, 1, "SomeProduct1.dds", validAssetType1 };

        m_errorAbsorber->Clear();
        EXPECT_FALSE(m_data->m_connection.SetProduct(product));
        EXPECT_GT(m_errorAbsorber->m_numErrorsAbsorbed, 0);
        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
        
        // make sure it didn't actually touch the db as a side effect:
        ProductDatabaseEntryContainer products;
        EXPECT_FALSE(m_data->m_connection.GetProducts(products));
        EXPECT_EQ(products.size(), 0);
    }

    TEST_F(AssetDatabaseTest, SetProduct_InvalidJobPK_Fails)
    {
        AZ::Data::AssetType validAssetType1 = AZ::Data::AssetType::CreateRandom();

        // -1 means insert a new product, but the JobPK is an enforced FK constraint, so this should fail since there
        // won't be a Job with the PK of 234234.

        ProductDatabaseEntry product{ -1, 234234, 1, "SomeProduct1.dds", validAssetType1 };

        m_errorAbsorber->Clear();
        EXPECT_FALSE(m_data->m_connection.SetProduct(product));
        EXPECT_GT(m_errorAbsorber->m_numErrorsAbsorbed, 0);
        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this

        // make sure it didn't actually touch the db as a side effect:
        ProductDatabaseEntryContainer products;
        EXPECT_FALSE(m_data->m_connection.GetProducts(products));
        EXPECT_EQ(products.size(), 0);
    }

    // if we give it a valid command and a -1 product, we expect it to succeed without assert or warning
    // and we expect it to tell us (by filling in the entry) what the new PK is.
    TEST_F(AssetDatabaseTest, SetProduct_AutoPK_Succeeds)
    {
        AZ::Data::AssetType validAssetType1 = AZ::Data::AssetType::CreateRandom();

        // to add a product legitimately you have to have a full chain of primary keys, chain is:
        // ScanFolder --> Source --> job --> product.
        // we'll create all of those first (except product) before starting the product test.

        //add a scanfolder.  None of this has to exist in real disk, this is a db test only.
        ScanFolderDatabaseEntry scanFolder {"c:/lumberyard/dev", "dev", "rootportkey", ""};
        EXPECT_TRUE(m_data->m_connection.SetScanFolder(scanFolder));
        ASSERT_NE(scanFolder.m_scanFolderID, -1); 

        SourceDatabaseEntry sourceEntry {scanFolder.m_scanFolderID, "somefile.tif", AZ::Uuid::CreateRandom() };
        EXPECT_TRUE(m_data->m_connection.SetSource(sourceEntry));
        ASSERT_NE(sourceEntry.m_sourceID, -1);

        JobDatabaseEntry jobEntry{ sourceEntry.m_sourceID, "some job key", 123, "pc", AZ::Uuid::CreateRandom(), AzToolsFramework::AssetSystem::JobStatus::Completed, 1 };
        EXPECT_TRUE(m_data->m_connection.SetJob(jobEntry));
        ASSERT_NE(jobEntry.m_jobID, -1);

        // --- set up complete --- perform the test!

        ProductDatabaseEntry product{ -1, jobEntry.m_jobID, 1, "SomeProduct1.dds", validAssetType1 };

        m_errorAbsorber->Clear();
        EXPECT_TRUE(m_data->m_connection.SetProduct(product));
        ASSERT_NE(product.m_productID, -1);

        EXPECT_EQ(m_errorAbsorber->m_numErrorsAbsorbed, 0);
        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this

        // read it back from the DB and make sure its identical to what was written

        ProductDatabaseEntry productFromDB;
        EXPECT_TRUE(m_data->m_connection.GetProductByProductID(product.m_productID, productFromDB));
        ASSERT_EQ(product, productFromDB);
    }


    // update an existing job by giving it a specific PK of a known existing item.
    TEST_F(AssetDatabaseTest, SetProduct_SpecificPK_Succeeds_DifferentSubID)
    {
        AZ::Data::AssetType validAssetType1 = AZ::Data::AssetType::CreateRandom();

        // to add a product legitimately you have to have a full chain of primary keys, chain is:
        // ScanFolder --> Source --> job --> product.
        // we'll create all of those first (except product) before starting the product test.
        ScanFolderDatabaseEntry scanFolder{ "c:/lumberyard/dev", "dev", "rootportkey", "" };
        ASSERT_TRUE(m_data->m_connection.SetScanFolder(scanFolder));
        
        SourceDatabaseEntry sourceEntry{ scanFolder.m_scanFolderID, "somefile.tif", AZ::Uuid::CreateRandom() };
        ASSERT_TRUE(m_data->m_connection.SetSource(sourceEntry));

        // two different job entries.
        JobDatabaseEntry jobEntry{ sourceEntry.m_sourceID, "some job key", 123, "pc", AZ::Uuid::CreateRandom(), AzToolsFramework::AssetSystem::JobStatus::Completed, 1 };
        JobDatabaseEntry jobEntry2{ sourceEntry.m_sourceID, "some job key 2", 345, "pc", AZ::Uuid::CreateRandom(), AzToolsFramework::AssetSystem::JobStatus::Completed, 2 };
        ASSERT_TRUE(m_data->m_connection.SetJob(jobEntry));
        ASSERT_TRUE(m_data->m_connection.SetJob(jobEntry2));

        ProductDatabaseEntry product{ -1, jobEntry.m_jobID, 1, "SomeProduct1.dds", validAssetType1 };
        ASSERT_TRUE(m_data->m_connection.SetProduct(product));
        
        // --- set up complete --- perform the test!
        // update all the fields of that product and then write it to the db.
        ProductDatabaseEntry newProductData = product; // copy first
        // now change all the fields:
        newProductData.m_assetType = AZ::Uuid::CreateRandom();
        newProductData.m_productName = "different name.dds";
        newProductData.m_subID = 2;
        newProductData.m_jobPK = jobEntry2.m_jobID; // move it to the other job, too!

        // update the product
        EXPECT_TRUE(m_data->m_connection.SetProduct(newProductData));
        ASSERT_NE(newProductData.m_productID, -1);

        // it should not have entered a new product but instead overwritten the old one.
        ASSERT_EQ(product.m_productID, newProductData.m_productID);

        // read it back from DB and verify:
        ProductDatabaseEntry productFromDB;
        EXPECT_TRUE(m_data->m_connection.GetProductByProductID(newProductData.m_productID, productFromDB));
        ASSERT_EQ(newProductData, productFromDB);

        ProductDatabaseEntryContainer products;
        EXPECT_TRUE(m_data->m_connection.GetProducts(products));
        EXPECT_EQ(products.size(), 1);
    }

    // update an existing job by giving it a subID and JobID which is enough to uniquely identify a product (since products may not have the same subid from the same job).
    // this is actually a very common case (same job id, same subID)
    TEST_F(AssetDatabaseTest, SetProduct_SpecificPK_Succeeds_SameSubID_SameJobID)
    {
        ScanFolderDatabaseEntry scanFolder{ "c:/lumberyard/dev", "dev", "rootportkey", "" };
        ASSERT_TRUE(m_data->m_connection.SetScanFolder(scanFolder));
        SourceDatabaseEntry sourceEntry{ scanFolder.m_scanFolderID, "somefile.tif", AZ::Uuid::CreateRandom() };
        ASSERT_TRUE(m_data->m_connection.SetSource(sourceEntry));
        JobDatabaseEntry jobEntry{ sourceEntry.m_sourceID, "some job key", 123, "pc", AZ::Uuid::CreateRandom(), AzToolsFramework::AssetSystem::JobStatus::Completed, 1 };
        ASSERT_TRUE(m_data->m_connection.SetJob(jobEntry));
        ProductDatabaseEntry product{ -1, jobEntry.m_jobID, 1, "SomeProduct1.dds", AZ::Data::AssetType::CreateRandom() };
        ASSERT_TRUE(m_data->m_connection.SetProduct(product));

        // --- set up complete --- perform the test!
        // update all the fields of that product and then write it to the db.
        ProductDatabaseEntry newProductData = product; // copy first
                                                       // now change all the fields:
        newProductData.m_assetType = AZ::Uuid::CreateRandom();
        newProductData.m_productName = "different name.dds";
        newProductData.m_productID = -1; // wipe out the product ID, so that we can make sure it returns it.
        // we don't change the subID here or the job ID.

        // update the product
        EXPECT_TRUE(m_data->m_connection.SetProduct(newProductData));
        ASSERT_NE(newProductData.m_productID, -1);

        // it should not have entered a new product but instead overwritten the old one.
        EXPECT_EQ(product.m_productID, newProductData.m_productID);

        // read it back from DB and verify:
        ProductDatabaseEntry productFromDB;
        EXPECT_TRUE(m_data->m_connection.GetProductByProductID(newProductData.m_productID, productFromDB));
        EXPECT_EQ(newProductData, productFromDB);

        ProductDatabaseEntryContainer products;
        EXPECT_TRUE(m_data->m_connection.GetProducts(products));
        EXPECT_EQ(products.size(), 1);
    }

    TEST_F(AssetDatabaseTest, GetProductsByJobID_InvalidID_NotFound_ReturnsFalse)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        EXPECT_FALSE(m_data->m_connection.GetProductsByJobID(-1, resultProducts));
        EXPECT_EQ(resultProducts.size(), 0);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsByJobID_Valid_ReturnsTrue_FindsProducts)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        EXPECT_TRUE(m_data->m_connection.GetProductsByJobID(m_data->m_job1.m_jobID, resultProducts));
        EXPECT_EQ(resultProducts.size(), 2); // should have found the first two products.

        // since there is no ordering, we just have to find both of them:
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());
        
        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductByJobIDSubId_InvalidID_NotFound_ReturnsFalse)
    {
        CreateCoverageTestData();

        ProductDatabaseEntry resultProduct;

        EXPECT_FALSE(m_data->m_connection.GetProductByJobIDSubId(m_data->m_job1.m_jobID, -1, resultProduct));
        EXPECT_FALSE(m_data->m_connection.GetProductByJobIDSubId(-1, m_data->m_product1.m_subID, resultProduct));
        EXPECT_FALSE(m_data->m_connection.GetProductByJobIDSubId(-1, -1, resultProduct));
        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductByJobIDSubId_ValidID_Found_ReturnsTrue)
    {
        CreateCoverageTestData();

        ProductDatabaseEntry resultProduct;

        EXPECT_TRUE(m_data->m_connection.GetProductByJobIDSubId(m_data->m_job1.m_jobID, m_data->m_product1.m_subID, resultProduct));
        EXPECT_EQ(resultProduct, m_data->m_product1);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }
 
    // --------------------------------------------------------------------------------------------------------------------
    // ------------------------------------------ GetProductsByProductName ------------------------------------------------
    // --------------------------------------------------------------------------------------------------------------------

    TEST_F(AssetDatabaseTest, GetProductsByProductName_EmptyString_NoResults)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;
        EXPECT_FALSE(m_data->m_connection.GetProductsByProductName(QString(), resultProducts));
        EXPECT_EQ(resultProducts.size(), 0);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsByProductName_NotFound_NoResult)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;
        EXPECT_FALSE(m_data->m_connection.GetProductsByProductName("akdsuhuksahdsak", resultProducts));
        EXPECT_EQ(resultProducts.size(), 0);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsByProductName_CorrectName_ReturnsCorrectResult)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;
        EXPECT_TRUE(m_data->m_connection.GetProductsByProductName("someproduct4.dds", resultProducts));
        EXPECT_EQ(resultProducts.size(), 1);
        EXPECT_EQ(resultProducts[0], m_data->m_product4);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }
    
    // tests all of the filters (beside name) to make sure they all function as expected.
    TEST_F(AssetDatabaseTest, GetProductsByProductName_FilterTest_BuilderGUID)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;
        
        // give it a random builder guid.  This should make it not match any products:
        EXPECT_FALSE(m_data->m_connection.GetProductsByProductName("someproduct4.dds", resultProducts, AZ::Uuid::CreateRandom()));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it a correct builder guid but the wrong builder.  Job2's builder actually built product4.
        EXPECT_FALSE(m_data->m_connection.GetProductsByProductName("someproduct4.dds", resultProducts, m_data->m_job1.m_builderGuid));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it correct data
        EXPECT_TRUE(m_data->m_connection.GetProductsByProductName("someproduct4.dds", resultProducts, m_data->m_job2.m_builderGuid));
        EXPECT_EQ(resultProducts.size(), 1);
        EXPECT_EQ(resultProducts[0], m_data->m_product4);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsByProductName_FilterTest_JobKey)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        // give it a random job key that is not going to match the existing job keys. This should make it not match any products:
        EXPECT_FALSE(m_data->m_connection.GetProductsByProductName("someproduct4.dds", resultProducts, AZ::Uuid::CreateNull(), "no matcher"));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it a correct job key but not one that output that product.
        EXPECT_FALSE(m_data->m_connection.GetProductsByProductName("someproduct4.dds", resultProducts, AZ::Uuid::CreateNull(), QString(m_data->m_job1.m_jobKey.c_str())));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it correct data
        EXPECT_TRUE(m_data->m_connection.GetProductsByProductName("someproduct4.dds", resultProducts, AZ::Uuid::CreateNull(), QString(m_data->m_job2.m_jobKey.c_str())));
        EXPECT_EQ(resultProducts.size(), 1);
        EXPECT_EQ(resultProducts[0], m_data->m_product4);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsByProductName_FilterTest_Platform)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        // give it a platform that is not going to match the existing job platforms.  This should make it not match any products:
        EXPECT_FALSE(m_data->m_connection.GetProductsByProductName("someproduct4.dds", resultProducts, AZ::Uuid::CreateNull(), QString(), "badplatform"));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it a correct platform but not one that output that product.
        EXPECT_FALSE(m_data->m_connection.GetProductsByProductName("someproduct4.dds", resultProducts, AZ::Uuid::CreateNull(), QString(), "pc"));  // its actually osx
        EXPECT_EQ(resultProducts.size(), 0);

        // give it correct data
        EXPECT_TRUE(m_data->m_connection.GetProductsByProductName("someproduct4.dds", resultProducts, AZ::Uuid::CreateNull(), QString(), "osx"));
        EXPECT_EQ(resultProducts.size(), 1);
        EXPECT_EQ(resultProducts[0], m_data->m_product4);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsByProductName_FilterTest_Status)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        // give it a correct status but not one that output that product.
        EXPECT_FALSE(m_data->m_connection.GetProductsByProductName("someproduct4.dds", resultProducts, AZ::Uuid::CreateNull(), QString(), QString(), AzToolsFramework::AssetSystem::JobStatus::Completed));  // its actually failed
        EXPECT_EQ(resultProducts.size(), 0);

        // give it correct data
        EXPECT_TRUE(m_data->m_connection.GetProductsByProductName("someproduct4.dds", resultProducts, AZ::Uuid::CreateNull(), QString(), QString(), AzToolsFramework::AssetSystem::JobStatus::Failed));
        EXPECT_EQ(resultProducts.size(), 1);
        EXPECT_EQ(resultProducts[0], m_data->m_product4);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    // --------------------------------------------------------------------------------------------------------------------
    // ------------------------------------------ GetProductsLikeProductName ----------------------------------------------
    // --------------------------------------------------------------------------------------------------------------------

    TEST_F(AssetDatabaseTest, GetProductsLikeProductName_EmptyString_NoResults)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;
        EXPECT_FALSE(m_data->m_connection.GetProductsLikeProductName(QString(), AssetDatabaseConnection::StartsWith, resultProducts));
        EXPECT_EQ(resultProducts.size(), 0);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsLikeProductName_NotFound_NoResult)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;
        EXPECT_FALSE(m_data->m_connection.GetProductsLikeProductName("akdsuhuksahdsak", AssetDatabaseConnection::StartsWith, resultProducts));
        EXPECT_EQ(resultProducts.size(), 0);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsLikeProductName_CorrectName_StartsWith_ReturnsCorrectResult)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;
        EXPECT_TRUE(m_data->m_connection.GetProductsLikeProductName("someproduct4", AssetDatabaseConnection::StartsWith, resultProducts));
        EXPECT_EQ(resultProducts.size(), 1);
        EXPECT_EQ(resultProducts[0], m_data->m_product4);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsLikeProductName_CorrectName_EndsWith_ReturnsCorrectResult)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;
        EXPECT_TRUE(m_data->m_connection.GetProductsLikeProductName("product4.dds", AssetDatabaseConnection::EndsWith, resultProducts));
        EXPECT_EQ(resultProducts.size(), 1);
        EXPECT_EQ(resultProducts[0], m_data->m_product4);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsLikeProductName_CorrectName_Matches_ReturnsCorrectResult)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;
        EXPECT_TRUE(m_data->m_connection.GetProductsLikeProductName("product4", AssetDatabaseConnection::Matches, resultProducts));
        EXPECT_EQ(resultProducts.size(), 1);
        EXPECT_EQ(resultProducts[0], m_data->m_product4);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsLikeProductName_CorrectName_StartsWith_ReturnsMany)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;
        
        // a very broad search that matches all products.
        EXPECT_TRUE(m_data->m_connection.GetProductsLikeProductName("someproduct", AssetDatabaseConnection::StartsWith, resultProducts));
        EXPECT_EQ(resultProducts.size(), 4);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    // tests all of the filters (beside name) to make sure they all function as expected.
    TEST_F(AssetDatabaseTest, GetProductsLikeProductName_FilterTest_BuilderGUID)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        // give it a random builder guid.  This should make it not match any products:
        EXPECT_FALSE(m_data->m_connection.GetProductsLikeProductName("someproduct4", AssetDatabaseConnection::StartsWith, resultProducts, AZ::Uuid::CreateRandom()));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it a correct builder guid but the wrong builder.  Job2's builder actually built product4.
        EXPECT_FALSE(m_data->m_connection.GetProductsLikeProductName("someproduct4", AssetDatabaseConnection::StartsWith, resultProducts, m_data->m_job1.m_builderGuid));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it correct data
        EXPECT_TRUE(m_data->m_connection.GetProductsLikeProductName("someproduct4", AssetDatabaseConnection::StartsWith, resultProducts, m_data->m_job2.m_builderGuid));
        EXPECT_EQ(resultProducts.size(), 1);
        EXPECT_EQ(resultProducts[0], m_data->m_product4);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsLikeProductName_FilterTest_JobKey)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        // give it a random job key that is not going to match the existing job keys. This should make it not match any products:
        EXPECT_FALSE(m_data->m_connection.GetProductsLikeProductName("someproduct4", AssetDatabaseConnection::StartsWith,  resultProducts, AZ::Uuid::CreateNull(), "no matcher"));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it a correct job key but not one that output that product.
        EXPECT_FALSE(m_data->m_connection.GetProductsLikeProductName("someproduct4", AssetDatabaseConnection::StartsWith, resultProducts, AZ::Uuid::CreateNull(), QString(m_data->m_job1.m_jobKey.c_str())));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it correct data
        EXPECT_TRUE(m_data->m_connection.GetProductsLikeProductName("someproduct4", AssetDatabaseConnection::StartsWith, resultProducts, AZ::Uuid::CreateNull(), QString(m_data->m_job2.m_jobKey.c_str())));
        EXPECT_EQ(resultProducts.size(), 1);
        EXPECT_EQ(resultProducts[0], m_data->m_product4);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsLikeProductName_FilterTest_Platform)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        // give it a platform that is not going to match the existing job platforms.  This should make it not match any products:
        EXPECT_FALSE(m_data->m_connection.GetProductsLikeProductName("someproduct4", AssetDatabaseConnection::StartsWith, resultProducts, AZ::Uuid::CreateNull(), QString(), "badplatform"));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it a correct platform but not one that output that product.
        EXPECT_FALSE(m_data->m_connection.GetProductsLikeProductName("someproduct4", AssetDatabaseConnection::StartsWith, resultProducts, AZ::Uuid::CreateNull(), QString(), "pc"));  // its actually osx
        EXPECT_EQ(resultProducts.size(), 0);

        // give it correct data
        EXPECT_TRUE(m_data->m_connection.GetProductsLikeProductName("someproduct4", AssetDatabaseConnection::StartsWith, resultProducts, AZ::Uuid::CreateNull(), QString(), "osx"));
        EXPECT_EQ(resultProducts.size(), 1);
        EXPECT_EQ(resultProducts[0], m_data->m_product4);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsLikeProductName_FilterTest_Status)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        // give it a correct status but not one that output that product.
        EXPECT_FALSE(m_data->m_connection.GetProductsLikeProductName("someproduct4", AssetDatabaseConnection::StartsWith, resultProducts, AZ::Uuid::CreateNull(), QString(), QString(), AzToolsFramework::AssetSystem::JobStatus::Completed));  // its actually failed
        EXPECT_EQ(resultProducts.size(), 0);

        // give it correct data
        EXPECT_TRUE(m_data->m_connection.GetProductsLikeProductName("someproduct4", AssetDatabaseConnection::StartsWith, resultProducts, AZ::Uuid::CreateNull(), QString(), QString(), AzToolsFramework::AssetSystem::JobStatus::Failed));
        EXPECT_EQ(resultProducts.size(), 1);
        EXPECT_EQ(resultProducts[0], m_data->m_product4);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    // --------------------------------------------------------------------------------------------------------------------
    // --------------------------------------------- GetProductsBySourceID ------------------------------------------------
    // --------------------------------------------------------------------------------------------------------------------

    TEST_F(AssetDatabaseTest, GetProductsBySourceID_NotFound_NoResult)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;
        EXPECT_FALSE(m_data->m_connection.GetProductsBySourceID(-1, resultProducts));
        EXPECT_EQ(resultProducts.size(), 0);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsBySourceID_CorrectID_ReturnsCorrectResult)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;
        EXPECT_TRUE(m_data->m_connection.GetProductsBySourceID(m_data->m_sourceFile1.m_sourceID, resultProducts));
        EXPECT_EQ(resultProducts.size(), 2);
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());
        
        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    // tests all of the filters (beside name) to make sure they all function as expected.
    TEST_F(AssetDatabaseTest, GetProductsBySourceID_FilterTest_BuilderGUID)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        // give it a random builder guid.  This should make it not match any products:
        EXPECT_FALSE(m_data->m_connection.GetProductsBySourceID(m_data->m_sourceFile1.m_sourceID, resultProducts, AZ::Uuid::CreateRandom()));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it a valid data, but the wrong one.  Note that job2 was the one that built the other files, not the sourcefile1.
        EXPECT_FALSE(m_data->m_connection.GetProductsBySourceID(m_data->m_sourceFile1.m_sourceID, resultProducts, m_data->m_job2.m_builderGuid));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it correct data
        EXPECT_TRUE(m_data->m_connection.GetProductsBySourceID(m_data->m_sourceFile1.m_sourceID, resultProducts, m_data->m_job1.m_builderGuid));
        EXPECT_EQ(resultProducts.size(), 2);
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsBySourceID_FilterTest_JobKey)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        // give it invalid data that won't match anything
        EXPECT_FALSE(m_data->m_connection.GetProductsBySourceID(m_data->m_sourceFile1.m_sourceID, resultProducts, AZ::Uuid::CreateNull(), "no matcher"));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it a valid data, but the wrong one:
        EXPECT_FALSE(m_data->m_connection.GetProductsBySourceID(m_data->m_sourceFile1.m_sourceID, resultProducts, AZ::Uuid::CreateNull(), QString(m_data->m_job2.m_jobKey.c_str())));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it correct data
        EXPECT_TRUE(m_data->m_connection.GetProductsBySourceID(m_data->m_sourceFile1.m_sourceID, resultProducts, AZ::Uuid::CreateNull(), QString(m_data->m_job1.m_jobKey.c_str())));
        EXPECT_EQ(resultProducts.size(), 2);
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsBySourceID_FilterTest_Platform)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        // give it invalid data that won't match anything
        EXPECT_FALSE(m_data->m_connection.GetProductsBySourceID(m_data->m_sourceFile1.m_sourceID, resultProducts, AZ::Uuid::CreateNull(), QString(), "badplatform"));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it a valid data, but the wrong one:
        EXPECT_FALSE(m_data->m_connection.GetProductsBySourceID(m_data->m_sourceFile1.m_sourceID, resultProducts, AZ::Uuid::CreateNull(), QString(), "osx"));  // its actually pc
        EXPECT_EQ(resultProducts.size(), 0);

        // give it correct data
        EXPECT_TRUE(m_data->m_connection.GetProductsBySourceID(m_data->m_sourceFile1.m_sourceID, resultProducts, AZ::Uuid::CreateNull(), QString(), "pc"));
        EXPECT_EQ(resultProducts.size(), 2);
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsBySourceID_FilterTest_Status)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        // give it a valid data, but the wrong one:
        EXPECT_FALSE(m_data->m_connection.GetProductsBySourceID(m_data->m_sourceFile1.m_sourceID, resultProducts, AZ::Uuid::CreateNull(), QString(), QString(), AzToolsFramework::AssetSystem::JobStatus::Failed));  // its actually completed
        EXPECT_EQ(resultProducts.size(), 0);

        // give it correct data
        EXPECT_TRUE(m_data->m_connection.GetProductsBySourceID(m_data->m_sourceFile1.m_sourceID, resultProducts, AZ::Uuid::CreateNull(), QString(), QString(), AzToolsFramework::AssetSystem::JobStatus::Completed));
        EXPECT_EQ(resultProducts.size(), 2);
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    // --------------------------------------------------------------------------------------------------------------------
    // -------------------------------------------- GetProductsBySourceName -----------------------------------------------
    // --------------------------------------------------------------------------------------------------------------------

    TEST_F(AssetDatabaseTest, GetProductsBySourceName_EmptyString_NoResult)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;
        EXPECT_FALSE(m_data->m_connection.GetProductsBySourceName(QString(), resultProducts));
        EXPECT_EQ(resultProducts.size(), 0);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsBySourceName_NotFound_NoResult)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;
        EXPECT_FALSE(m_data->m_connection.GetProductsBySourceName("blahrga", resultProducts));
        EXPECT_EQ(resultProducts.size(), 0);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsBySourceName_CorrectName_ReturnsCorrectResult)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;
        EXPECT_TRUE(m_data->m_connection.GetProductsBySourceName("somefile.tif", resultProducts));  // this is source1, which results in product1 and product2 via job1
        EXPECT_EQ(resultProducts.size(), 2);
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    // tests all of the filters (beside name) to make sure they all function as expected.
    TEST_F(AssetDatabaseTest, GetProductsBySourceName_FilterTest_BuilderGUID)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        // give it a random builder guid.  This should make it not match any products:
        EXPECT_FALSE(m_data->m_connection.GetProductsBySourceName("somefile.tif", resultProducts, AZ::Uuid::CreateRandom()));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it a valid data, but the wrong one.  Note that job2 was the one that built the other files, not the sourcefile1.
        EXPECT_FALSE(m_data->m_connection.GetProductsBySourceName("somefile.tif", resultProducts, m_data->m_job2.m_builderGuid));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it correct data
        EXPECT_TRUE(m_data->m_connection.GetProductsBySourceName("somefile.tif", resultProducts, m_data->m_job1.m_builderGuid));
        EXPECT_EQ(resultProducts.size(), 2);
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsBySourceName_FilterTest_JobKey)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        // give it invalid data that won't match anything
        EXPECT_FALSE(m_data->m_connection.GetProductsBySourceName("somefile.tif", resultProducts, AZ::Uuid::CreateNull(), "no matcher"));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it a valid data, but the wrong one:
        EXPECT_FALSE(m_data->m_connection.GetProductsBySourceName("somefile.tif", resultProducts, AZ::Uuid::CreateNull(), QString(m_data->m_job2.m_jobKey.c_str())));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it correct data
        EXPECT_TRUE(m_data->m_connection.GetProductsBySourceName("somefile.tif", resultProducts, AZ::Uuid::CreateNull(), QString(m_data->m_job1.m_jobKey.c_str())));
        EXPECT_EQ(resultProducts.size(), 2);
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsBySourceName_FilterTest_Platform)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        // give it invalid data that won't match anything
        EXPECT_FALSE(m_data->m_connection.GetProductsBySourceName("somefile.tif", resultProducts, AZ::Uuid::CreateNull(), QString(), "badplatform"));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it a valid data, but the wrong one:
        EXPECT_FALSE(m_data->m_connection.GetProductsBySourceName("somefile.tif", resultProducts, AZ::Uuid::CreateNull(), QString(), "osx"));  // its actually pc
        EXPECT_EQ(resultProducts.size(), 0);

        // give it correct data
        EXPECT_TRUE(m_data->m_connection.GetProductsBySourceName("somefile.tif", resultProducts, AZ::Uuid::CreateNull(), QString(), "pc"));
        EXPECT_EQ(resultProducts.size(), 2);
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsBySourceName_FilterTest_Status)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        // give it a valid data, but the wrong one:
        EXPECT_FALSE(m_data->m_connection.GetProductsBySourceName("somefile.tif", resultProducts, AZ::Uuid::CreateNull(), QString(), QString(), AzToolsFramework::AssetSystem::JobStatus::Failed));  // its actually completed
        EXPECT_EQ(resultProducts.size(), 0);

        // give it correct data
        EXPECT_TRUE(m_data->m_connection.GetProductsBySourceName("somefile.tif", resultProducts, AZ::Uuid::CreateNull(), QString(), QString(), AzToolsFramework::AssetSystem::JobStatus::Completed));
        EXPECT_EQ(resultProducts.size(), 2);
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    // --------------------------------------------------------------------------------------------------------------------
    // -------------------------------------------- GetProductsLikeSourceName ---------------------------------------------
    // --------------------------------------------------------------------------------------------------------------------

    TEST_F(AssetDatabaseTest, GetProductsLikeSourceName_EmptyString_NoResult)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;
        EXPECT_FALSE(m_data->m_connection.GetProductsLikeSourceName(QString(), AzToolsFramework::AssetDatabase::AssetDatabaseConnection::Raw, resultProducts));
        EXPECT_EQ(resultProducts.size(), 0);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsLikeSourceName_NotFound_NoResult)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;
        EXPECT_FALSE(m_data->m_connection.GetProductsLikeSourceName("blahrga", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::Raw, resultProducts));
        EXPECT_EQ(resultProducts.size(), 0);

        // this matches the end of a legit string, but we are using startswith, so it should NOT MATCH
        EXPECT_FALSE(m_data->m_connection.GetProductsLikeSourceName("file.tif", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::StartsWith, resultProducts));
        EXPECT_EQ(resultProducts.size(), 0);

        // this matches the startswith, but should NOT MATCH, becuase its asking for things that end with it.
        EXPECT_FALSE(m_data->m_connection.GetProductsLikeSourceName("somefile", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::EndsWith, resultProducts));
        EXPECT_EQ(resultProducts.size(), 0);

        // make sure invalid tokens do not crash it or something
        EXPECT_FALSE(m_data->m_connection.GetProductsLikeSourceName("%%%%%blahrga%%%%%", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::Matches, resultProducts));
        EXPECT_EQ(resultProducts.size(), 0);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsLikeSourceName_StartsWith_CorrectName_ReturnsCorrectResult)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;
        EXPECT_TRUE(m_data->m_connection.GetProductsLikeSourceName("somefile", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::StartsWith, resultProducts));  // this is source1, which results in product1 and product2 via job1
        EXPECT_EQ(resultProducts.size(), 2);
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }


    TEST_F(AssetDatabaseTest, GetProductsLikeSourceName_EndsWith_CorrectName_ReturnsCorrectResult)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;
        EXPECT_TRUE(m_data->m_connection.GetProductsLikeSourceName("omefile.tif", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::EndsWith, resultProducts));  // this is source1, which results in product1 and product2 via job1
        EXPECT_EQ(resultProducts.size(), 2);
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsLikeSourceName_Matches_CorrectName_ReturnsCorrectResult)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;
        EXPECT_TRUE(m_data->m_connection.GetProductsLikeSourceName("omefile", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::Matches, resultProducts));  // this is source1, which results in product1 and product2 via job1
        EXPECT_EQ(resultProducts.size(), 2);
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    // tests all of the filters (beside name) to make sure they all function as expected.
    TEST_F(AssetDatabaseTest, GetProductsLikeSourceName_FilterTest_BuilderGUID)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        // give it a random builder guid.  This should make it not match any products:
        EXPECT_FALSE(m_data->m_connection.GetProductsLikeSourceName("somefile", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::StartsWith, resultProducts, AZ::Uuid::CreateRandom()));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it a valid data, but the wrong one.  Note that job2 was the one that built the other files, not the sourcefile1.
        EXPECT_FALSE(m_data->m_connection.GetProductsLikeSourceName("somefile", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::StartsWith, resultProducts, m_data->m_job2.m_builderGuid));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it correct data
        EXPECT_TRUE(m_data->m_connection.GetProductsLikeSourceName("somefile", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::StartsWith, resultProducts, m_data->m_job1.m_builderGuid));
        EXPECT_EQ(resultProducts.size(), 2);
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsLikeSourceName_FilterTest_JobKey)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        // give it invalid data that won't match anything
        EXPECT_FALSE(m_data->m_connection.GetProductsLikeSourceName("somefile", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::StartsWith, resultProducts, AZ::Uuid::CreateNull(), "no matcher"));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it a valid data, but the wrong one:
        EXPECT_FALSE(m_data->m_connection.GetProductsLikeSourceName("somefile", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::StartsWith, resultProducts, AZ::Uuid::CreateNull(), QString(m_data->m_job2.m_jobKey.c_str())));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it correct data
        EXPECT_TRUE(m_data->m_connection.GetProductsLikeSourceName("somefile", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::StartsWith, resultProducts, AZ::Uuid::CreateNull(), QString(m_data->m_job1.m_jobKey.c_str())));
        EXPECT_EQ(resultProducts.size(), 2);
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsLikeSourceName_FilterTest_Platform)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        // give it invalid data that won't match anything
        EXPECT_FALSE(m_data->m_connection.GetProductsLikeSourceName("somefile", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::StartsWith, resultProducts, AZ::Uuid::CreateNull(), QString(), "badplatform"));
        EXPECT_EQ(resultProducts.size(), 0);

        // give it a valid data, but the wrong one:
        EXPECT_FALSE(m_data->m_connection.GetProductsLikeSourceName("somefile", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::StartsWith, resultProducts, AZ::Uuid::CreateNull(), QString(), "osx"));  // its actually pc
        EXPECT_EQ(resultProducts.size(), 0);

        // give it correct data
        EXPECT_TRUE(m_data->m_connection.GetProductsLikeSourceName("somefile", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::StartsWith, resultProducts, AZ::Uuid::CreateNull(), QString(), "pc"));
        EXPECT_EQ(resultProducts.size(), 2);
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, GetProductsLikeSourceName_FilterTest_Status)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        // give it a valid data, but the wrong one:
        EXPECT_FALSE(m_data->m_connection.GetProductsLikeSourceName("somefile", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::StartsWith, resultProducts, AZ::Uuid::CreateNull(), QString(), QString(), AzToolsFramework::AssetSystem::JobStatus::Failed));  // its actually completed
        EXPECT_EQ(resultProducts.size(), 0);

        // give it correct data
        EXPECT_TRUE(m_data->m_connection.GetProductsLikeSourceName("somefile", AzToolsFramework::AssetDatabase::AssetDatabaseConnection::StartsWith, resultProducts, AZ::Uuid::CreateNull(), QString(), QString(), AzToolsFramework::AssetSystem::JobStatus::Completed));
        EXPECT_EQ(resultProducts.size(), 2);
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    // --------------------------------------------------------------------------------------------------------------------
    // -------------------------------------------------- SetProducts  ----------------------------------------------------
    // --------------------------------------------------------------------------------------------------------------------
    TEST_F(AssetDatabaseTest, SetProducts_EmptyList_Fails)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer requestProducts;
        EXPECT_FALSE(m_data->m_connection.SetProducts(requestProducts));
        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, SetProducts_UpdatesProductIDs)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer requestProducts;

        ProductDatabaseEntryContainer resultProducts;
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t priorProductCount = resultProducts.size();

        requestProducts.push_back({ m_data->m_job1.m_jobID, 5, "someproduct5.dds", AZ::Data::AssetType::CreateRandom() });
        requestProducts.push_back({ m_data->m_job1.m_jobID, 6, "someproduct6.dds", AZ::Data::AssetType::CreateRandom() });
        EXPECT_TRUE(m_data->m_connection.SetProducts(requestProducts));

        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t newProductCount = resultProducts.size();

        EXPECT_NE(requestProducts[0].m_productID, -1);
        EXPECT_NE(requestProducts[1].m_productID, -1);
        
        EXPECT_EQ(newProductCount, priorProductCount + 2);
        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    // --------------------------------------------------------------------------------------------------------------------
    // ---------------------------------------------- RemoveProduct(s)  ---------------------------------------------------
    // --------------------------------------------------------------------------------------------------------------------
    TEST_F(AssetDatabaseTest, RemoveProduct_InvalidID_Fails_DoesNotCorruptDB)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t priorProductCount = resultProducts.size();
        
        EXPECT_FALSE(m_data->m_connection.RemoveProduct(-1));
       
        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t newProductCount = resultProducts.size();

        EXPECT_EQ(newProductCount, priorProductCount);
    }

    TEST_F(AssetDatabaseTest, RemoveProducts_EmptyList_Fails_DoesNotCorruptDB)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t priorProductCount = resultProducts.size();

        resultProducts.clear();
        EXPECT_FALSE(m_data->m_connection.RemoveProducts(resultProducts));

        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t newProductCount = resultProducts.size();

        EXPECT_EQ(newProductCount, priorProductCount);
    }

    TEST_F(AssetDatabaseTest, RemoveProducts_InvalidProductIDs_Fails_DoesNotCorruptDB)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t priorProductCount = resultProducts.size();

        resultProducts.clear();
        resultProducts.push_back({ -1, m_data->m_job1.m_jobID, 5, "someproduct5.dds", AZ::Data::AssetType::CreateRandom() });
        resultProducts.push_back({ -2, m_data->m_job1.m_jobID, 6, "someproduct5.dds", AZ::Data::AssetType::CreateRandom() });
        resultProducts.push_back({ -3, m_data->m_job1.m_jobID, 7, "someproduct5.dds", AZ::Data::AssetType::CreateRandom() });

        EXPECT_FALSE(m_data->m_connection.RemoveProducts(resultProducts));

        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t newProductCount = resultProducts.size();

        EXPECT_EQ(newProductCount, priorProductCount);
    }

    TEST_F(AssetDatabaseTest, RemoveProduct_CorrectProduct_OnlyRemovesThatProduct)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t priorProductCount = resultProducts.size();

        EXPECT_TRUE(m_data->m_connection.RemoveProduct(m_data->m_product1.m_productID));

        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t newProductCount = resultProducts.size();

        EXPECT_EQ(newProductCount, priorProductCount - 1);

        // make sure they're all there except that one.
        EXPECT_EQ(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product3), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product4), resultProducts.end());
    }

    TEST_F(AssetDatabaseTest, RemoveProducts_CorrectProduct_OnlyRemovesThoseProducts)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t priorProductCount = resultProducts.size();

        resultProducts.clear();
        resultProducts.push_back(m_data->m_product1);
        resultProducts.push_back(m_data->m_product3);

        EXPECT_TRUE(m_data->m_connection.RemoveProducts(resultProducts));

        // its also supposed to clear their ids:
        EXPECT_EQ(resultProducts[0].m_productID, -1);
        EXPECT_EQ(resultProducts[1].m_productID, -1);

        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t newProductCount = resultProducts.size();

        EXPECT_EQ(newProductCount, priorProductCount - 2);

        // make sure they're all there except those two - (1 and 3) which should be 'eq' to end().
        EXPECT_EQ(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());
        EXPECT_EQ(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product3), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product4), resultProducts.end());
    }


    // --------------------------------------------------------------------------------------------------------------------
    // ---------------------------------------------- RemoveProductsByJobID  ---------------------------------------------------
    // --------------------------------------------------------------------------------------------------------------------
    TEST_F(AssetDatabaseTest, RemoveProductsByJobID_InvalidID_Fails_DoesNotCorruptDB)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t priorProductCount = resultProducts.size();

        EXPECT_FALSE(m_data->m_connection.RemoveProductsByJobID(-1));

        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t newProductCount = resultProducts.size();

        EXPECT_EQ(newProductCount, priorProductCount);
    }

    TEST_F(AssetDatabaseTest, RemoveProductsByJobID_ValidID_OnlyRemovesTheMatchingProducts)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;

        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t priorProductCount = resultProducts.size();

        EXPECT_TRUE(m_data->m_connection.RemoveProductsByJobID(m_data->m_job1.m_jobID));

        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t newProductCount = resultProducts.size();

        EXPECT_EQ(newProductCount, priorProductCount - 2);

        // both products that belong to the first job should be gone.
        EXPECT_EQ(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_EQ(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product3), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product4), resultProducts.end());
    }


    // --------------------------------------------------------------------------------------------------------------------
    // ------------------------------------------ RemoveProductsBySourceID ------------------------------------------------
    // --------------------------------------------------------------------------------------------------------------------

    TEST_F(AssetDatabaseTest, RemoveProductsBySourceID_InvalidSourceID_NoResults)
    {
        CreateCoverageTestData();
        ProductDatabaseEntryContainer resultProducts;

        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t priorProductCount = resultProducts.size();

        EXPECT_FALSE(m_data->m_connection.RemoveProductsBySourceID(-1));
        
        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t newProductCount = resultProducts.size();
        EXPECT_EQ(newProductCount, priorProductCount);

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, RemoveProductsBySourceID_Valid_OnlyRemovesTheCorrectOnes)
    {
        CreateCoverageTestData();
        ProductDatabaseEntryContainer resultProducts;

        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t priorProductCount = resultProducts.size();

        EXPECT_TRUE(m_data->m_connection.RemoveProductsBySourceID(m_data->m_sourceFile1.m_sourceID));

        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t newProductCount = resultProducts.size();
        EXPECT_EQ(newProductCount, priorProductCount - 2);

        // both products that belong to the first source should be gone - but only those!
        EXPECT_EQ(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_EQ(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product3), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product4), resultProducts.end());

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    // tests all of the filters (beside name) to make sure they all function as expected.
    TEST_F(AssetDatabaseTest, RemoveProductsBySourceID_FilterTest_BuilderGUID)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t priorProductCount = resultProducts.size();

        // give it a non-matching builder UUID - it should not delete anything despite the product sourceId being correct.
        EXPECT_FALSE(m_data->m_connection.RemoveProductsBySourceID(m_data->m_sourceFile1.m_sourceID, AZ::Uuid::CreateRandom()));
        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t newProductCount = resultProducts.size();
        EXPECT_EQ(newProductCount, priorProductCount); 

        // give it a correct data but the wrong builder (a valid, but wrong one)
        EXPECT_FALSE(m_data->m_connection.RemoveProductsBySourceID(m_data->m_sourceFile1.m_sourceID, m_data->m_job2.m_builderGuid));
        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        newProductCount = resultProducts.size();
        EXPECT_EQ(newProductCount, priorProductCount);

        // give it correct data, it should delete the first two products:
        EXPECT_TRUE(m_data->m_connection.RemoveProductsBySourceID(m_data->m_sourceFile1.m_sourceID, m_data->m_job1.m_builderGuid));
        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        newProductCount = resultProducts.size();
        EXPECT_EQ(newProductCount, priorProductCount - 2);

        // both products that belong to the first source should be gone - but only those!
        EXPECT_EQ(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_EQ(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product3), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product4), resultProducts.end());

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, RemoveProductsBySourceID_FilterTest_JobKey)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t priorProductCount = resultProducts.size();

        // give it a non-matching job key - it should not delete anything despite the product sourceId being correct.
        EXPECT_FALSE(m_data->m_connection.RemoveProductsBySourceID(m_data->m_sourceFile1.m_sourceID, AZ::Uuid::CreateNull(), "random key that wont match"));
        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t newProductCount = resultProducts.size();
        EXPECT_EQ(newProductCount, priorProductCount);

        // give it a correct data but the wrong builder (a valid, but wrong one)
        EXPECT_FALSE(m_data->m_connection.RemoveProductsBySourceID(m_data->m_sourceFile1.m_sourceID, AZ::Uuid::CreateNull(), m_data->m_job2.m_jobKey.c_str())); // job2 is not the one that did sourcefile1
        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        newProductCount = resultProducts.size();
        EXPECT_EQ(newProductCount, priorProductCount);

        // give it correct data, it should delete the first two products:
        EXPECT_TRUE(m_data->m_connection.RemoveProductsBySourceID(m_data->m_sourceFile1.m_sourceID, AZ::Uuid::CreateNull(), m_data->m_job1.m_jobKey.c_str()));
        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        newProductCount = resultProducts.size();
        EXPECT_EQ(newProductCount, priorProductCount - 2);

        // both products that belong to the first source should be gone - but only those!
        EXPECT_EQ(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_EQ(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product3), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product4), resultProducts.end());

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, RemoveProductsBySourceID_FilterTest_Platform)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t priorProductCount = resultProducts.size();

        // give it a non-matching job key - it should not delete anything despite the product sourceId being correct.
        EXPECT_FALSE(m_data->m_connection.RemoveProductsBySourceID(m_data->m_sourceFile1.m_sourceID, AZ::Uuid::CreateNull(), QString(), "no such platform"));
        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t newProductCount = resultProducts.size();
        EXPECT_EQ(newProductCount, priorProductCount);

        // give it a correct data but the wrong builder (a valid, but wrong one)
        EXPECT_FALSE(m_data->m_connection.RemoveProductsBySourceID(m_data->m_sourceFile1.m_sourceID, AZ::Uuid::CreateNull(), QString(), "osx")); // its actually PC
        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        newProductCount = resultProducts.size();
        EXPECT_EQ(newProductCount, priorProductCount);

        // give it correct data, it should delete the first two products:
        EXPECT_TRUE(m_data->m_connection.RemoveProductsBySourceID(m_data->m_sourceFile1.m_sourceID, AZ::Uuid::CreateNull(), QString(), "pc"));
        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        newProductCount = resultProducts.size();
        EXPECT_EQ(newProductCount, priorProductCount - 2);

        // both products that belong to the first source should be gone - but only those!
        EXPECT_EQ(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_EQ(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product3), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product4), resultProducts.end());

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }

    TEST_F(AssetDatabaseTest, RemoveProductsBySourceID_FilterTest_Status)
    {
        CreateCoverageTestData();

        ProductDatabaseEntryContainer resultProducts;
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t priorProductCount = resultProducts.size();

        // give it a correct status but not one that output that product.
        EXPECT_FALSE(m_data->m_connection.RemoveProductsBySourceID(m_data->m_sourceFile1.m_sourceID, AZ::Uuid::CreateNull(), QString(), QString(), AzToolsFramework::AssetSystem::JobStatus::Failed));

        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        size_t newProductCount = resultProducts.size();
        EXPECT_EQ(newProductCount, priorProductCount);

        // give it correct data
        EXPECT_TRUE(m_data->m_connection.RemoveProductsBySourceID(m_data->m_sourceFile1.m_sourceID, AZ::Uuid::CreateNull(), QString(), QString(), AzToolsFramework::AssetSystem::JobStatus::Completed));

        resultProducts.clear();
        EXPECT_TRUE(m_data->m_connection.GetProducts(resultProducts));
        newProductCount = resultProducts.size();
        EXPECT_EQ(newProductCount, priorProductCount - 2);

        // both products that belong to the first source should be gone - but only those!
        EXPECT_EQ(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product1), resultProducts.end());
        EXPECT_EQ(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product2), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product3), resultProducts.end());
        EXPECT_NE(AZStd::find(resultProducts.begin(), resultProducts.end(), m_data->m_product4), resultProducts.end());

        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0); // not allowed to assert on this
    }
} // end namespace UnitTests
