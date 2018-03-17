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

    m_qApp = new QCoreApplication(m_argc, m_argv);
    qRegisterMetaType<AssetProcessor::JobEntry>("JobEntry");
    qRegisterMetaType<AssetBuilderSDK::ProcessJobResponse>("ProcessJobResponse");
}

void AssetProcessorManagerTest::SetUp()
{
    using namespace AssetProcessor;
    AssetProcessorTest::SetUp();

    m_config = new AssetProcessor::PlatformConfiguration();

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

    m_config->AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder1"), "subfolder1", "subfolder1", "", false, true));

    m_config->EnablePlatform({ "pc", {"host", "renderer"} }, true);

    m_assetProcessorManager = new AssetProcessorManager_Test(m_config);
    m_assertAbsorber.Clear();
}

void AssetProcessorManagerTest::TearDown()
{
    AssetUtilities::ResetAssetRoot();
    delete m_assetProcessorManager;
    delete m_config;
    delete m_qApp;
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

    JobEntry entry;
    entry.m_relativePathToFile = relFileName;
    entry.m_absolutePathToFile = absPath;
    entry.m_jobKey = "txt";
    entry.m_platformInfo = { "pc", {"host", "renderer", "desktop"} };
    entry.m_jobRunKey = 1;

    UnitTestUtils::CreateDummyFile(m_normalizedCacheRootDir.absoluteFilePath("outputfile.txt"));

    AssetBuilderSDK::ProcessJobResponse jobResponse;
    jobResponse.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
    jobResponse.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(m_normalizedCacheRootDir.absoluteFilePath("outputfile.txt").toUtf8().data()));

    QMetaObject::invokeMethod(m_assetProcessorManager, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, entry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, jobResponse));

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
    entry.m_relativePathToFile = relFileName;
    entry.m_absolutePathToFile = absPath;
    entry.m_jobKey = "txt";
    entry.m_platformInfo = { "pc", {"host", "renderer", "desktop"} };
    entry.m_jobRunKey = 1;

    AZ::Uuid sourceUUID = AssetUtilities::CreateSafeSourceUUIDFromName(entry.m_relativePathToFile.toUtf8().data());
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

