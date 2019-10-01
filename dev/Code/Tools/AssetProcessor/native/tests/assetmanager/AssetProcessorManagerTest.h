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

#pragma once

#include <AzTest/AzTest.h>
#include <AzCore/std/parallel/atomic.h>
#include <qcoreapplication.h>
#include "native/tests/AssetProcessorTest.h"
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include "native/assetprocessor.h"
#include "native/unittests/UnitTestRunner.h"
#include "native/AssetManager/assetProcessorManager.h"
#include "native/utilities/PlatformConfiguration.h"
#include "native/unittests/MockApplicationManager.h"

#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <QTemporaryDir>
#include <QMetaObject>
#include <AzToolsFramework/API/AssetDatabaseBus.h>

class AssetProcessorManager_Test;

class MockDatabaseLocationListener : public AzToolsFramework::AssetDatabase::AssetDatabaseRequests::Bus::Handler
{
public:
    MOCK_METHOD1(GetAssetDatabaseLocation, bool(AZStd::string&));
};

class AssetProcessorManagerTest
    : public AssetProcessor::AssetProcessorTest
{
public:
    

    AssetProcessorManagerTest();
    virtual ~AssetProcessorManagerTest()
    {
    }

    // utility function.  Blocks and runs the QT event pump for up to millisecondsMax and will break out as soon as the APM is idle.
    bool BlockUntilIdle(int millisecondsMax);

protected:
    void SetUp() override;
    void TearDown() override;

    QTemporaryDir m_tempDir;

    AZStd::unique_ptr<AssetProcessorManager_Test> m_assetProcessorManager;
    AZStd::unique_ptr<AssetProcessor::MockApplicationManager> m_mockApplicationManager;
    AZStd::unique_ptr<AssetProcessor::PlatformConfiguration> m_config;
    UnitTestUtils::AssertAbsorber m_assertAbsorber; // absorb asserts/warnings/errors so that the unit test output is not cluttered
    QString m_gameName;
    QDir m_normalizedCacheRootDir;
    AZStd::atomic_bool m_isIdling;
    QMetaObject::Connection m_idleConnection;

    struct StaticData
    {
        AZStd::string m_databaseLocation;
        ::testing::NiceMock<MockDatabaseLocationListener> m_databaseLocationListener;
    };

    AZStd::unique_ptr<StaticData> m_data;
    
private:
    int         m_argc;
    char**      m_argv;
    AZStd::unique_ptr<UnitTestUtils::ScopedDir> m_scopeDir;

    AZStd::unique_ptr<QCoreApplication> m_qApp;    
};

struct PathDependencyTest
    : public AssetProcessorManagerTest
{
    void SetUp() override;

    struct TestAsset
    {
        TestAsset() = default;
        TestAsset(const char* name) : m_name(name) {}

        AZStd::string m_name;
        AZStd::vector<AZ::Data::AssetId> m_products;
    };

    void CaptureJobs(AZStd::vector<AssetProcessor::JobDetails>& jobDetails, const char* sourceFilePath);
    void ProcessAsset(TestAsset& asset, AZStd::initializer_list<AZStd::initializer_list<const char*>> outputAssets, const AssetBuilderSDK::ProductPathDependencySet& dependencies);
};

struct MultiplatformPathDependencyTest
    : public PathDependencyTest
{
    void SetUp() override;
};

struct MockBuilderInfoHandler
    : public AssetProcessor::AssetBuilderInfoBus::Handler
{
    ~MockBuilderInfoHandler();

    //! AssetProcessor::AssetBuilderInfoBus Interface
    void GetMatchingBuildersInfo(const AZStd::string& assetPath, AssetProcessor::BuilderInfoList& builderInfoList) override;
    void GetAllBuildersInfo(AssetProcessor::BuilderInfoList& builderInfoList) override;

    void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response);
    void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response);

    AssetBuilderSDK::AssetBuilderDesc CreateBuilderDesc(const QString& builderName, const QString& builderId, const AZStd::vector<AssetBuilderSDK::AssetBuilderPattern>& builderPatterns);

    AssetBuilderSDK::AssetBuilderDesc m_builderDesc;
    QString m_jobFingerprint;
    QString m_dependencyFilePath;
    int m_createJobsCount = 0;
};

struct ModtimeScanningTest
    : public AssetProcessorManagerTest
{
    void SetUp() override;
    void TearDown() override;

    void ProcessAssetJobs();
    void SimulateAssetScanner(QSet<AssetProcessor::AssetFileInfo> filePaths);
    QSet<AssetProcessor::AssetFileInfo> BuildFileSet();

    QString m_relativePathFromWatchFolder[2];
    QString m_absolutePath[2];
    QList<AssetProcessor::JobDetails> m_processResults;
    QList<QString> m_deletedSources;
    AZStd::shared_ptr<AssetProcessor::InternalMockBuilder> m_builderTxtBuilder;
    MockBuilderInfoHandler m_mockBuilderInfoHandler;
};

struct FingerprintTest
    : public AssetProcessorManagerTest
{
    void SetUp() override;
    void TearDown() override;

    void RunFingerprintTest(QString builderFingerprint, QString jobFingerprint, bool expectedResult);

    QString m_absolutePath;
    MockBuilderInfoHandler m_mockBuilderInfoHandler;
    AZStd::vector<AssetProcessor::JobDetails> m_jobResults;
};