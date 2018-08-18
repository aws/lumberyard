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

#include "AssetProcessorManagerTest.h"
#include "native/utilities/PlatformConfiguration.h"
#include "native/utilities/assetUtils.h"
#include "native/AssetManager/assetScanFolderInfo.h"
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Asset/AssetProcessorMessages.h>
#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>

#include <QTime>
#include <QCoreApplication>

AssetProcessorManager_Test::AssetProcessorManager_Test(AssetProcessor::PlatformConfiguration* config, QObject* parent /*= 0*/)
    :AssetProcessor::AssetProcessorManager(config, parent)
{
}

AssetProcessorManager_Test::~AssetProcessorManager_Test()
{
}

bool AssetProcessorManager_Test::CheckJobKeyToJobRunKeyMap(AZStd::string jobKey)
{
    return (m_jobKeyToJobRunKeyMap.find(jobKey) != m_jobKeyToJobRunKeyMap.end());
}

bool AssetProcessorManager_Test::CheckSourceUUIDToSourceNameMap(AZ::Uuid sourceUUID)
{
    return (m_sourceUUIDToSourceNameMap.find(sourceUUID) != m_sourceUUIDToSourceNameMap.end());
}

AssetProcessorManagerTest::AssetProcessorManagerTest()
    : m_argc(0)
    , m_argv(0)
{

    m_qApp.reset(new QCoreApplication(m_argc, m_argv));
    qRegisterMetaType<AssetProcessor::JobEntry>("JobEntry");
    qRegisterMetaType<AssetBuilderSDK::ProcessJobResponse>("ProcessJobResponse");
}

bool AssetProcessorManagerTest::BlockUntilIdle(int millisecondsMax)
{
    QTime limit;
    limit.start();
    while ((!m_isIdling) && (limit.elapsed() < millisecondsMax))
    {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    }

    // and then once more, so that any queued events as a result of the above finish.
    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);

    return m_isIdling;
}

void AssetProcessorManagerTest::SetUp()
{
    using namespace AssetProcessor;
    AssetProcessorTest::SetUp();

    m_config.reset(new AssetProcessor::PlatformConfiguration());
    m_mockApplicationManager.reset(new AssetProcessor::MockApplicationManager());
    
    AssetUtilities::ResetAssetRoot();

    m_scopeDir.Setup(m_tempDir.path());
    QDir tempPath(m_tempDir.path());

    UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("bootstrap.cfg"), QString("sys_game_folder=SamplesProject\n"));

    m_gameName = AssetUtilities::ComputeGameName(tempPath.absolutePath(), true);

    AssetUtilities::ResetAssetRoot();
    QDir newRoot;
    AssetUtilities::ComputeEngineRoot(newRoot, &tempPath);

    QDir cacheRoot;
    AssetUtilities::ComputeProjectCacheRoot(cacheRoot);
    QString normalizedCacheRoot = AssetUtilities::NormalizeDirectoryPath(cacheRoot.absolutePath());

    QString normalizedDirPathCheck = AssetUtilities::NormalizeDirectoryPath(QDir::current().absoluteFilePath("Cache/" + m_gameName));
    m_normalizedCacheRootDir.setPath(normalizedCacheRoot);

    UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("subfolder1/assetProcessorManagerTest.txt"));

    m_config->EnablePlatform({ "pc", { "host", "renderer", "desktop" } }, true);

    m_config->AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder1"), "subfolder1", "subfolder1", "", false, true, m_config->GetEnabledPlatforms() ));

    // add a scan folder that has an output prefix:
    m_config->AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder2"), "subfolder2", "subfolder2", "redirected", false, true, m_config->GetEnabledPlatforms()));

    AssetRecognizer rec;
    AssetPlatformSpec specpc;

    rec.m_name = "txt files";
    rec.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.txt", AssetBuilderSDK::AssetBuilderPattern::Wildcard);
    rec.m_platformSpecs.insert("pc", specpc);
    rec.m_supportsCreateJobs = false;
    m_mockApplicationManager->RegisterAssetRecognizerAsBuilder(rec);
    m_mockApplicationManager->BusConnect();

    m_assetProcessorManager.reset(new AssetProcessorManager_Test(m_config.get()));
    m_assertAbsorber.Clear();

    m_isIdling = false;

    m_idleConnection = QObject::connect(m_assetProcessorManager.get(), &AssetProcessor::AssetProcessorManager::AssetProcessorManagerIdleState, [this](bool newState)
    {
        m_isIdling = newState;
    });
}

void AssetProcessorManagerTest::TearDown()
{
    QObject::disconnect(m_idleConnection);
    m_mockApplicationManager->BusDisconnect();
    m_mockApplicationManager->UnRegisterAllBuilders();

    AssetUtilities::ResetAssetRoot();
    m_assetProcessorManager.reset();
    m_mockApplicationManager.reset();
    m_config.reset();
    m_qApp.reset();

    AssetProcessor::AssetProcessorTest::TearDown();
}

TEST_F(AssetProcessorManagerTest, UnitTestForGettingJobInfoBySourceUUIDSuccess)
{
    // Here we first mark a job for an asset complete and than fetch jobs info using the job log api to verify
    // Next we mark another job for that same asset as queued, and we again fetch jobs info from the api to verify,

    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;

    QDir tempPath(m_tempDir.path());

    QString relFileName("assetProcessorManagerTest.txt");
    QString absPath(tempPath.absoluteFilePath("subfolder1/assetProcessorManagerTest.txt"));
    QString watchFolder = tempPath.absoluteFilePath("subfolder1");

    JobEntry entry;
    entry.m_watchFolderPath = watchFolder;
    entry.m_databaseSourceName = entry.m_pathRelativeToWatchFolder = relFileName;
    entry.m_jobKey = "txt";
    entry.m_platformInfo = { "pc", {"host", "renderer", "desktop"} };
    entry.m_jobRunKey = 1;

    UnitTestUtils::CreateDummyFile(m_normalizedCacheRootDir.absoluteFilePath("outputfile.txt"));

    AssetBuilderSDK::ProcessJobResponse jobResponse;
    jobResponse.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
    jobResponse.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(m_normalizedCacheRootDir.absoluteFilePath("outputfile.txt").toUtf8().data()));

    QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, entry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, jobResponse));

    // let events bubble through:
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    QCoreApplication::processEvents(QEventLoop::AllEvents);

    AZ::Uuid uuid = AssetUtilities::CreateSafeSourceUUIDFromName(relFileName.toUtf8().data());
    AssetJobsInfoRequest request;
    request.m_assetId = AZ::Data::AssetId(uuid, 0);
    request.m_escalateJobs = false;
    AssetJobsInfoResponse response;
    m_assetProcessorManager->ProcessGetAssetJobsInfoRequest(request, response);

    EXPECT_TRUE(response.m_isSuccess);
    EXPECT_EQ(1, response.m_jobList.size());
    ASSERT_GT(response.m_jobList.size(), 0); // Assert on this to exit early if needed, otherwise indexing m_jobList later will crash.
    EXPECT_EQ(JobStatus::Completed, response.m_jobList[0].m_status);
    EXPECT_STRCASEEQ(relFileName.toUtf8().data(), response.m_jobList[0].m_sourceFile.c_str());


    m_assetProcessorManager->OnJobStatusChanged(entry, JobStatus::Queued);

    response.m_isSuccess = false;
    response.m_jobList.clear();

    m_assetProcessorManager->ProcessGetAssetJobsInfoRequest(request, response);
    EXPECT_TRUE(response.m_isSuccess);
    EXPECT_EQ(1, response.m_jobList.size());
    ASSERT_GT(response.m_jobList.size(), 0); // Assert on this to exit early if needed, otherwise indexing m_jobList later will crash.

    EXPECT_EQ(JobStatus::Queued, response.m_jobList[0].m_status);
    EXPECT_STRCASEEQ(relFileName.toUtf8().data(), response.m_jobList[0].m_sourceFile.c_str());
    EXPECT_STRCASEEQ(tempPath.filePath("subfolder1").toUtf8().data(), response.m_jobList[0].m_watchFolder.c_str());

    ASSERT_EQ(m_assertAbsorber.m_numWarningsAbsorbed, 0);
    ASSERT_EQ(m_assertAbsorber.m_numErrorsAbsorbed, 0);
    ASSERT_EQ(m_assertAbsorber.m_numAssertsAbsorbed, 0);
}

TEST_F(AssetProcessorManagerTest, UnitTestForGettingJobInfoBySourceUUIDFailure)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;

    QString relFileName("assetProcessorManagerTestFailed.txt");

    AZ::Uuid uuid = AssetUtilities::CreateSafeSourceUUIDFromName(relFileName.toUtf8().data());
    AssetJobsInfoRequest request;
    request.m_assetId = AZ::Data::AssetId(uuid, 0);
    request.m_escalateJobs = false;
    AssetJobsInfoResponse response;
    m_assetProcessorManager->ProcessGetAssetJobsInfoRequest(request, response);

    ASSERT_TRUE(response.m_isSuccess == false); //expected result should be false because AP does not know about this asset
    ASSERT_TRUE(response.m_jobList.size() == 0);
}

TEST_F(AssetProcessorManagerTest, UnitTestForCancelledJob)
{
    using namespace AzToolsFramework::AssetSystem;
    using namespace AssetProcessor;

    QDir tempPath(m_tempDir.path());
    QString relFileName("assetProcessorManagerTest.txt");
    QString absPath(tempPath.absoluteFilePath("subfolder1/assetProcessorManagerTest.txt"));
    JobEntry entry;

    entry.m_watchFolderPath = tempPath.absolutePath();
    entry.m_databaseSourceName = entry.m_pathRelativeToWatchFolder = relFileName;

    entry.m_jobKey = "txt";
    entry.m_platformInfo = { "pc", {"host", "renderer", "desktop"} };
    entry.m_jobRunKey = 1;

    AZ::Uuid sourceUUID = AssetUtilities::CreateSafeSourceUUIDFromName(entry.m_databaseSourceName.toUtf8().data());
    bool sourceFound = false;

    //Checking the response of the APM when we cancel a job in progress
    m_assetProcessorManager->OnJobStatusChanged(entry, JobStatus::Queued);
    m_assetProcessorManager->OnJobStatusChanged(entry, JobStatus::InProgress);
    ASSERT_TRUE(m_assetProcessorManager->CheckJobKeyToJobRunKeyMap(entry.m_jobKey.toUtf8().data()));
    ASSERT_TRUE(m_assetProcessorManager->CheckSourceUUIDToSourceNameMap(sourceUUID));
    m_assetProcessorManager->AssetCancelled(entry);
    ASSERT_FALSE(m_assetProcessorManager->CheckJobKeyToJobRunKeyMap(entry.m_jobKey.toUtf8().data()));
    ASSERT_FALSE(m_assetProcessorManager->CheckSourceUUIDToSourceNameMap(sourceUUID));
    ASSERT_TRUE(m_assetProcessorManager->GetDatabaseConnection()->QuerySourceBySourceGuid(sourceUUID, [&](AzToolsFramework::AssetDatabase::SourceDatabaseEntry& source)
    {
        sourceFound = true;
        return false;
    }));

    ASSERT_FALSE(sourceFound);
}

TEST_F(AssetProcessorManagerTest, BuilderSDK_API_CreateJobs_HasValidParameters_WithNoOutputFolder)
{
    QDir tempPath(m_tempDir.path());
    // here we push a file change through APM and make sure that "CreateJobs" has correct parameters, with no output redirection
    QString absPath(tempPath.absoluteFilePath("subfolder1/test_text.txt"));
    UnitTestUtils::CreateDummyFile(absPath);

    m_mockApplicationManager->ResetMockBuilderCreateJobCalls();

    m_isIdling = false;
    QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, absPath));
    
    // wait for AP to become idle.
    ASSERT_TRUE(BlockUntilIdle(5000));

    ASSERT_EQ(m_mockApplicationManager->GetMockBuilderCreateJobCalls(), 1);

    AZStd::shared_ptr<AssetProcessor::InternalMockBuilder> builderTxtBuilder;
    ASSERT_TRUE(m_mockApplicationManager->GetBuilderByID("txt files", builderTxtBuilder));

    const AssetBuilderSDK::CreateJobsRequest &req = builderTxtBuilder->GetLastCreateJobRequest();

    EXPECT_STREQ(req.m_watchFolder.c_str(), tempPath.absoluteFilePath("subfolder1").toUtf8().constData());
    EXPECT_STREQ(req.m_sourceFile.c_str(), "test_text.txt"); // only the name should be there, no output prefix.

    EXPECT_NE(req.m_sourceFileUUID, AZ::Uuid::CreateNull());
    EXPECT_TRUE(req.HasPlatform("pc"));
    EXPECT_TRUE(req.HasPlatformWithTag("desktop"));
}


TEST_F(AssetProcessorManagerTest, BuilderSDK_API_CreateJobs_HasValidParameters_WithOutputRedirectedFolder)
{
    QDir tempPath(m_tempDir.path());
    // here we push a file change through APM and make sure that "CreateJobs" has correct parameters, with no output redirection
    QString absPath(tempPath.absoluteFilePath("subfolder2/test_text.txt"));
    UnitTestUtils::CreateDummyFile(absPath);

    m_mockApplicationManager->ResetMockBuilderCreateJobCalls();
    
    m_isIdling = false;
    QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, absPath));
    
    ASSERT_TRUE(BlockUntilIdle(5000));

    ASSERT_EQ(m_mockApplicationManager->GetMockBuilderCreateJobCalls(), 1);

    AZStd::shared_ptr<AssetProcessor::InternalMockBuilder> builderTxtBuilder;
    ASSERT_TRUE(m_mockApplicationManager->GetBuilderByID("txt files", builderTxtBuilder));

    const AssetBuilderSDK::CreateJobsRequest &req = builderTxtBuilder->GetLastCreateJobRequest();

    // this test looks identical to the above test, but the important piece of information here is that
    // subfolder2 has its output redirected in the cache
    // this test makes sure that the CreateJobs API is completely unaffected by that and none of the internal database stuff
    // is reflected by the API.
    EXPECT_STREQ(req.m_watchFolder.c_str(), tempPath.absoluteFilePath("subfolder2").toUtf8().constData());
    EXPECT_STREQ(req.m_sourceFile.c_str(), "test_text.txt"); // only the name should be there, no output prefix.

    EXPECT_NE(req.m_sourceFileUUID, AZ::Uuid::CreateNull());
    EXPECT_TRUE(req.HasPlatform("pc"));
    EXPECT_TRUE(req.HasPlatformWithTag("desktop"));
}

// this tests exists to make sure a bug does not regress.
// when the bug was active, dependencies would be stored in the database incorrectly when different products emitted different dependencies.
// specifically, any dependency emitted by any product of a given source would show up as a dependency of ALL products for that source.
TEST_F(AssetProcessorManagerTest, AssetProcessedImpl_DifferentProductDependenciesPerProduct_SavesCorrectlyToDatabase)
{
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;
    /// --------------------- SETUP PHASE - make an asset exist in the database -------------------

    // Create the source file.
    QDir tempPath(m_tempDir.path());
    QString absPath(tempPath.absoluteFilePath("subfolder1/test_text.txt"));
    UnitTestUtils::CreateDummyFile(absPath);

    // prepare to capture the job details as the APM inspects the file.
    JobDetails capturedDetails;
    auto connection = QObject::connect(m_assetProcessorManager.get(), &AssetProcessorManager::AssetToProcess, [&capturedDetails](JobDetails jobDetails)
    {
        capturedDetails = jobDetails;
    });

    // tell the APM about the file:
    m_isIdling = false;
    QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, absPath));
    ASSERT_TRUE(BlockUntilIdle(5000));

    ASSERT_FALSE(capturedDetails.m_autoFail);

    QObject::disconnect(connection);

    // we should have gotten at least one request to actually process that job:
    ASSERT_STREQ(capturedDetails.m_jobEntry.GetAbsoluteSourcePath().toUtf8().constData(), absPath.toUtf8().constData());

    // now simulate the job being done and actually returning a full job finished details which includes dependencies:
    ProcessJobResponse response;
    response.m_resultCode = ProcessJobResult_Success;

    QString destTestPath1 = QDir(capturedDetails.m_destinationPath).absoluteFilePath("test1.txt");
    QString destTestPath2 = QDir(capturedDetails.m_destinationPath).absoluteFilePath("test2.txt");

    UnitTestUtils::CreateDummyFile(destTestPath1, "this is the first output");
    UnitTestUtils::CreateDummyFile(destTestPath2, "this is the second output");

    JobProduct productA(destTestPath1.toUtf8().constData(), AZ::Uuid::CreateRandom(), 1);
    JobProduct productB(destTestPath2.toUtf8().constData(), AZ::Uuid::CreateRandom(), 2);
    AZ::Data::AssetId expectedIdOfProductA(capturedDetails.m_jobEntry.m_sourceFileUUID, productA.m_productSubID);
    AZ::Data::AssetId expectedIdOfProductB(capturedDetails.m_jobEntry.m_sourceFileUUID, productB.m_productSubID);

    productA.m_dependencies.push_back(ProductDependency(expectedIdOfProductB, 5));
    productB.m_dependencies.push_back(ProductDependency(expectedIdOfProductA, 6));
    response.m_outputProducts.push_back(productA);
    response.m_outputProducts.push_back(productB);

    // tell the APM that the asset has been processed and allow it to bubble through its event queue:
    m_isIdling = false;
    m_assetProcessorManager->AssetProcessed(capturedDetails.m_jobEntry, response);
    ASSERT_TRUE(BlockUntilIdle(5000));
    // note that there exists different tests (in the AssetStateDatabase tests) to directly test the actual database store/get for this
    // the purpose of this test is to just make sure that the Asset Processor Manager actually understood the job dependencies
    // and correctly stored the results into the dependency table.

    //-------------------------------- EVALUATION PHASE -------------------------
    // at this point, the AP will have filed the asset away in its database and we can now validate that it actually 
    // did it correctly.
    // We expect to see two dependencies in the dependency table, each with the correct dependency, no duplicates, no lost data.
    AssetDatabaseConnection* sharedConnection = m_assetProcessorManager->m_stateData.get();

    AZStd::unordered_map<AZ::Data::AssetId, AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry> capturedTableEntries;

    ASSERT_TRUE(sharedConnection);
    AZStd::size_t countFound = 0;
    bool queryresult = sharedConnection->QueryProductDependenciesTable(
        [&capturedTableEntries, &countFound](AZ::Data::AssetId& asset, AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry& entry)
    {
        ++countFound;
        capturedTableEntries[asset] = entry;
        return true;
    });

    // this also asserts uniqueness.
    ASSERT_EQ(countFound, 2);
    ASSERT_EQ(capturedTableEntries.size(), countFound); // if they were not unique asset IDs, they would have collapsed on top of each other.
    
    // make sure both assetIds are present:
    ASSERT_NE(capturedTableEntries.find(expectedIdOfProductA), capturedTableEntries.end());
    ASSERT_NE(capturedTableEntries.find(expectedIdOfProductB), capturedTableEntries.end());

    // make sure both refer to the other and nothing else.
    EXPECT_EQ(capturedTableEntries[expectedIdOfProductA].m_dependencySourceGuid, expectedIdOfProductB.m_guid);
    EXPECT_EQ(capturedTableEntries[expectedIdOfProductA].m_dependencySubID, expectedIdOfProductB.m_subId);
    EXPECT_EQ(capturedTableEntries[expectedIdOfProductA].m_dependencyFlags, 5);

    EXPECT_EQ(capturedTableEntries[expectedIdOfProductB].m_dependencySourceGuid, expectedIdOfProductA.m_guid);
    EXPECT_EQ(capturedTableEntries[expectedIdOfProductB].m_dependencySubID, expectedIdOfProductA.m_subId);
    EXPECT_EQ(capturedTableEntries[expectedIdOfProductB].m_dependencyFlags, 6);
}


// this test exists to make sure a bug does not regress.
// when the bug was active, source files with multiple products would cause the asset processor to repeatedly process them
// due to a timing problem.  Specifically, if the products were not successfully moved to the output directory quickly enough
// it would assume something was wrong, and re-trigger the job, which cancelled the already-in-flight job currently busy copying
// the product files to the cache to finalize it.
TEST_F(AssetProcessorManagerTest, AssessDeletedFile_OnJobInFlight_IsIgnored)
{
    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    // constants to adjust:
    const int numOutputsToSimulate = 100;

    // --------------------- SETUP PHASE - make an asset exist in the database as if the job is complete -------------------
    // The asset needs multiple job products.

    // Create the source file.
    QDir tempPath(m_tempDir.path());
    QString absPath(tempPath.absoluteFilePath("subfolder1/test_text.txt"));
    UnitTestUtils::CreateDummyFile(absPath);

    // prepare to capture the job details as the APM inspects the file.
    JobDetails capturedDetails;
    auto connection = QObject::connect(m_assetProcessorManager.get(), &AssetProcessorManager::AssetToProcess, [&capturedDetails](JobDetails jobDetails)
    {
        capturedDetails = jobDetails;
    });

    // tell the APM about the file:
    m_isIdling = false;
    QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, absPath));
    ASSERT_TRUE(BlockUntilIdle(5000));

    QObject::disconnect(connection);

    // we should have gotten at least one request to actually process that job:
    ASSERT_STREQ(capturedDetails.m_jobEntry.GetAbsoluteSourcePath().toUtf8().constData(), absPath.toUtf8().constData());

    // now simulate the job being done and actually returning a full job finished details which includes dependencies:
    ProcessJobResponse response;
    response.m_resultCode = ProcessJobResult_Success;
    for (int outputIdx = 0; outputIdx < numOutputsToSimulate; ++outputIdx)
    {
        QString fileNameToGenerate = QString("test%1.txt").arg(outputIdx);
        QString filePathToGenerate = QDir(capturedDetails.m_destinationPath).absoluteFilePath(fileNameToGenerate);

        UnitTestUtils::CreateDummyFile(filePathToGenerate, "an output");
        JobProduct product(filePathToGenerate.toUtf8().constData(), AZ::Uuid::CreateRandom(), static_cast<AZ::u32>(outputIdx));
        response.m_outputProducts.push_back(product);
    }

    // tell the APM that the asset has been processed and allow it to bubble through its event queue:
    m_isIdling = false;
    m_assetProcessorManager->AssetProcessed(capturedDetails.m_jobEntry, response);
    ASSERT_TRUE(BlockUntilIdle(5000));
   
    // at this point, everything should be up to date and ready for the test - there should be one source in the database
    // with numOutputsToSimulate products.
    // now, we simulate a job running to process the asset again, by modifying the timestamp on the file to be at least one second later.
    // this is becuase on some operating systems, the resolution of file time stamps is at least one second.
    AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(1001));
    UnitTestUtils::CreateDummyFile(absPath, "Completely different file data");

    // with the source file changed, tell it to process it again:
    // prepare to capture the job details as the APM inspects the file.
    connection = QObject::connect(m_assetProcessorManager.get(), &AssetProcessorManager::AssetToProcess, [&capturedDetails](JobDetails jobDetails)
    {
        capturedDetails = jobDetails;
    });

    // tell the APM about the file:
    m_isIdling = false;
    QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, absPath));
    ASSERT_TRUE(BlockUntilIdle(5000));

    QObject::disconnect(connection);
    // we should have gotten at least one request to actually process that job:
    ASSERT_STREQ(capturedDetails.m_jobEntry.GetAbsoluteSourcePath().toUtf8().constData(), absPath.toUtf8().constData());
    ASSERT_FALSE(capturedDetails.m_autoFail);
    ASSERT_FALSE(capturedDetails.m_autoSucceed);
    ASSERT_FALSE(capturedDetails.m_destinationPath.isEmpty());
    // ----------------------------- TEST BEGINS HERE -----------------------------
    // simulte a very slow computer processing the file one output at a time and feeding file change notifies:
    
    // FROM THIS POINT ON we should see no new job create / cancellation or anything since we're just going to be messing with the cache.
    bool gotUnexpectedAssetToProcess = false;
    connection = QObject::connect(m_assetProcessorManager.get(), &AssetProcessorManager::AssetToProcess, [&gotUnexpectedAssetToProcess](JobDetails /*jobDetails*/)
    {
        gotUnexpectedAssetToProcess = true;
    });

    // this function tells APM about a file and waits for it to idle, if waitForIdle is true.
    // basically, it simulates the file watcher firing on events from the cache since file watcher events
    // come in on the queue at any time a file changes, sourced from a different thread.
    auto notifyAPM = [this, &gotUnexpectedAssetToProcess](const char* functionToCall, QString filePath, bool waitForIdle)
    {
        if (waitForIdle)
        {
            m_isIdling = false;
        }
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), functionToCall, Qt::QueuedConnection, Q_ARG(QString, QString(filePath)));
        if (waitForIdle)
        {
            ASSERT_TRUE(BlockUntilIdle(5000));
        }

        ASSERT_FALSE(gotUnexpectedAssetToProcess);
    };

    response = AssetBuilderSDK::ProcessJobResponse();
    response.m_resultCode = ProcessJobResult_Success;
    for (int outputIdx = 0; outputIdx < numOutputsToSimulate; ++outputIdx)
    {
        // every second one, we dont wait at all and let it rapidly process, to preturb the timing.
        bool shouldBlockAndWaitThisTime = outputIdx % 2 == 0;

        QString fileNameToGenerate = QString("test%1.txt").arg(outputIdx);
        QString filePathToGenerate = QDir(capturedDetails.m_destinationPath).absoluteFilePath(fileNameToGenerate);
       
        JobProduct product(filePathToGenerate.toUtf8().constData(), AZ::Uuid::CreateRandom(), static_cast<AZ::u32>(outputIdx));
        response.m_outputProducts.push_back(product);

        AssetProcessor::ProcessingJobInfoBus::Broadcast(&AssetProcessor::ProcessingJobInfoBus::Events::BeginIgnoringCacheFileDelete, filePathToGenerate.toUtf8().data());

        AZ::IO::SystemFile::Delete(filePathToGenerate.toUtf8().constData());

        // simulate the file watcher showing the deletion occuring:
        notifyAPM("AssessDeletedFile", filePathToGenerate, shouldBlockAndWaitThisTime);
        UnitTestUtils::CreateDummyFile(filePathToGenerate, "an output");
        
        // let the APM go for a significant amount of time so that it simulates a slow thread copying a large file with lots of events about it pouring in.
        for (int repeatLoop = 0; repeatLoop < 100; ++repeatLoop)
        {
            QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessDeletedFile", Qt::QueuedConnection, Q_ARG(QString, QString(filePathToGenerate)));
            QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents, 1);
            ASSERT_FALSE(gotUnexpectedAssetToProcess);
        }

        // also toss it a "cache modified" call to make sure that this does not spawn further jobs
        // note that assessing modified files in the cache should not result in it spawning jobs or even becoming unidle since it
        // actually ignores modified files in the cache.
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, QString(filePathToGenerate)));
        QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents, 1);
        ASSERT_FALSE(gotUnexpectedAssetToProcess);
        
        // now tell it to stop ignoring the cache delete and let it do the next one.
        EBUS_EVENT(AssetProcessor::ProcessingJobInfoBus, StopIgnoringCacheFileDelete, filePathToGenerate.toUtf8().data(), false);

        // simulate a "late" deletion notify coming from the file monitor that it outside the "ignore delete" section.  This should STILL not generate additional
        // deletion notifies as it should ignore these if the file in fact actually there when it gets around to checking it
        notifyAPM("AssessDeletedFile", filePathToGenerate, shouldBlockAndWaitThisTime);
    }

    // tell the APM that the asset has been processed and allow it to bubble through its event queue:
    m_isIdling = false;
    m_assetProcessorManager->AssetProcessed(capturedDetails.m_jobEntry, response);
    ASSERT_TRUE(BlockUntilIdle(5000));
    ASSERT_FALSE(gotUnexpectedAssetToProcess);

    QObject::disconnect(connection);
}
