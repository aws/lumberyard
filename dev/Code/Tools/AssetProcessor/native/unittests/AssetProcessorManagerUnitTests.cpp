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
#if defined(UNIT_TEST)

#include <AzCore/base.h>

#if defined AZ_PLATFORM_WINDOWS
// a compiler bug appears to cause this file to take a really, really long time to optimize (many minutes).
// its used for unit testing only.
#pragma optimize("", off)
#endif

#include "AssetProcessorManagerUnitTests.h"

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/IO/FileIO.h>
#include <AzFramework/IO/LocalFileIO.h>

#include <AzToolsFramework/Asset/AssetProcessorMessages.h>

#include "MockApplicationManager.h"
#include "native/AssetManager/assetProcessorManager.h"
#include "native/utilities/PlatformConfiguration.h"
#include "native/utilities/AssetBuilderInfo.h"
#include "native/AssetManager/assetScanFolderInfo.h"
#include "native/utilities/AssetUtils.h"
#include "native/FileWatcher/FileWatcherAPI.h"
#include "native/FileWatcher/FileWatcher.h"
#include "native/unittests/MockConnectionHandler.h"
#include "native/resourcecompiler/RCBuilder.h"
#include "native/assetprocessor.h"


#include <QTemporaryDir>
#include <QString>
#include <QCoreApplication>
#include <QSet>
#include <QList>
#include <QTime>
#include <QThread>
#include <QPair>

namespace AssetProcessor
{
    using namespace UnitTestUtils;
    using namespace AzFramework::AssetSystem;
    using namespace AzToolsFramework::AssetSystem;
    using namespace AzToolsFramework::AssetDatabase;

    class AssetProcessorManager_Test
        : public AssetProcessorManager
    {
    public:
        explicit AssetProcessorManager_Test(PlatformConfiguration* config, QObject* parent = 0)
            : AssetProcessorManager(config, parent)
        {}

    public:
        using GetRelativeProductPathFromFullSourceOrProductPathRequest = AzFramework::AssetSystem::GetRelativeProductPathFromFullSourceOrProductPathRequest;
        using GetRelativeProductPathFromFullSourceOrProductPathResponse = AzFramework::AssetSystem::GetRelativeProductPathFromFullSourceOrProductPathResponse;
        using GetFullSourcePathFromRelativeProductPathRequest = AzFramework::AssetSystem::GetFullSourcePathFromRelativeProductPathRequest;
        using GetFullSourcePathFromRelativeProductPathResponse = AzFramework::AssetSystem::GetFullSourcePathFromRelativeProductPathResponse;
        const ScanFolderInfo* UNITTEST_GetScanFolderForFile(const char* absoluteFile);
        bool UNITTEST_SetSource(SourceDatabaseEntry& source);
        bool UNITTEST_SetSourceFileDependency(SourceFileDependencyEntry& sourceFileDependency);
        void UNITTEST_ClearSourceFileDependencyInternalMaps();
        QStringList UNITTEST_CheckSourceFileDependency(QString source);
        void UNITTEST_ProcessCreateJobsResponse(AssetBuilderSDK::CreateJobsResponse& createJobsResponse, const AssetBuilderSDK::CreateJobsRequest& createJobsRequest);
        void UNITTEST_UpdateSourceFileDependencyInfo();
        void UNITTEST_UpdateSourceFileDependencyDatabase();
        void UNITTEST_AnalyzeJobDetail(JobToProcessEntry& jobEntry);
    };

    const ScanFolderInfo* AssetProcessorManager_Test::UNITTEST_GetScanFolderForFile(const char* absoluteFile)
    {
        return m_platformConfig->GetScanFolderForFile(absoluteFile);
    }

    bool AssetProcessorManager_Test::UNITTEST_SetSource(SourceDatabaseEntry& source)
    {
        return m_stateData->SetSource(source);
    }

    bool AssetProcessorManager_Test::UNITTEST_SetSourceFileDependency(SourceFileDependencyEntry& sourceFileDependency)
    {
        return m_stateData->SetSourceFileDependency(sourceFileDependency);
    }

    QStringList AssetProcessorManager_Test::UNITTEST_CheckSourceFileDependency(QString source)
    {
        return CheckSourceFileDependency(source);
    }

    void AssetProcessorManager_Test::UNITTEST_ProcessCreateJobsResponse(AssetBuilderSDK::CreateJobsResponse& createJobsResponse, const AssetBuilderSDK::CreateJobsRequest& createJobsRequest)
    {
        ProcessCreateJobsResponse(createJobsResponse, createJobsRequest);
    }

    void AssetProcessorManager_Test::UNITTEST_ClearSourceFileDependencyInternalMaps()
    {
        m_dependsOnSourceToSourceMap.clear();
        m_dependsOnSourceUuidToSourceMap.clear();
        m_sourceFileDependencyInfoMap.clear();
    }

    void AssetProcessorManager_Test::UNITTEST_UpdateSourceFileDependencyInfo()
    {
        UpdateSourceFileDependencyInfo();
    }

    void AssetProcessorManager_Test::UNITTEST_UpdateSourceFileDependencyDatabase()
    {
        UpdateSourceFileDependencyDatabase();
    }
    void AssetProcessorManager_Test::UNITTEST_AnalyzeJobDetail(JobToProcessEntry& jobEntry)
    {
        AnalyzeJobDetail(jobEntry);
    }

    REGISTER_UNIT_TEST(AssetProcessorManagerUnitTests)
    REGISTER_UNIT_TEST(AssetProcessorManagerUnitTests_ScanFolders)
    REGISTER_UNIT_TEST(AssetProcessorManagerUnitTests_JobKeys)
    REGISTER_UNIT_TEST(AssetProcessorManagerUnitTests_SourceFileDependencies)
    REGISTER_UNIT_TEST(AssetProcessorManagerUnitTests_CheckOutputFolders)

    namespace
    {

        /// This functions sorts the processed result list by platform name
        /// if platform is same than it sorts by job description
        void sortAssetToProcessResultList(QList<JobDetails>& processResults)
        {
            //Sort the processResults based on platforms
            qSort(processResults.begin(), processResults.end(),
                [](const JobDetails& first, const JobDetails& second)
                {
                    if (first.m_jobEntry.m_platformInfo.m_identifier == second.m_jobEntry.m_platformInfo.m_identifier)
                    {
                        return first.m_jobEntry.m_jobKey.toLower() < second.m_jobEntry.m_jobKey.toLower();
                    }

                    return first.m_jobEntry.m_platformInfo.m_identifier < second.m_jobEntry.m_platformInfo.m_identifier;
                });

            //AZ_TracePrintf("test", "-------------------------\n");
        }

        void ComputeFingerprints(unsigned int& fingerprintForPC, unsigned int& fingerprintForES3, PlatformConfiguration& config, QString filePath, QString relPath)
        {
            QString extraInfoForPC;
            QString extraInfoForES3;
            RecognizerPointerContainer output;
            config.GetMatchingRecognizers(filePath, output);
            for (const AssetRecognizer* assetRecogniser : output)
            {
                extraInfoForPC.append(assetRecogniser->m_platformSpecs["pc"].m_extraRCParams);
                extraInfoForES3.append(assetRecogniser->m_platformSpecs["es3"].m_extraRCParams);
                extraInfoForPC.append(assetRecogniser->m_version);
                extraInfoForES3.append(assetRecogniser->m_version);
            }

            //Calculating fingerprints for the file for pc and es3 platforms
            AZ::Uuid sourceId = AZ::Uuid("{2206A6E0-FDBC-45DE-B6FE-C2FC63020BD5}");
            JobEntry jobEntryPC(filePath, relPath, 0, { "pc", {"desktop", "renderer"} }, "", 0, 1, sourceId);
            JobEntry jobEntryES3(filePath, relPath, 0, { "es3", {"mobile", "renderer"} }, "", 0, 2, sourceId);

            JobDetails jobDetailsPC;
            jobDetailsPC.m_extraInformationForFingerprinting = extraInfoForPC;
            jobDetailsPC.m_jobEntry = jobEntryPC;
            JobDetails jobDetailsES3;
            jobDetailsES3.m_extraInformationForFingerprinting = extraInfoForES3;
            jobDetailsES3.m_jobEntry = jobEntryES3;
            fingerprintForPC = AssetUtilities::GenerateFingerprint(jobDetailsPC);
            fingerprintForES3 = AssetUtilities::GenerateFingerprint(jobDetailsES3);
        }
    }

    void AssetProcessorManagerUnitTests::StartTest()
    {
        // the asset processor manager is generally sitting on top of many other systems.
        // we have tested those systems individually in other unit tests, but we need to create
        // a simulated environment to test the manager itself.
        // for the manager, the only things we care about is that it emits the correct signals
        // when the appropriate stimulus is given and that state is appropriately updated
        // we want to make no modifications to the real asset database or any other file
        // except the temp folder while this goes on.

        // attach a file monitor to ensure this occurs.

        MockApplicationManager  mockAppManager;
        mockAppManager.BusConnect();

        QDir oldRoot;
        AssetUtilities::ComputeAssetRoot(oldRoot);
        AssetUtilities::ResetAssetRoot();
        QStringList collectedChanges;
        FileWatcher fileWatcher;
        FolderWatchCallbackEx folderWatch(oldRoot.absolutePath(), QString(), true);
        fileWatcher.AddFolderWatch(&folderWatch);
        connect(&folderWatch, &FolderWatchCallbackEx::fileChange,
            [&collectedChanges](FileChangeInfo info)
            {
                //do not add log files and folders
                QFileInfo fileInfo(info.m_filePath);
                if (!QRegExp(".*.log", Qt::CaseInsensitive, QRegExp::RegExp).exactMatch(info.m_filePath) && !fileInfo.isDir())
                {
                    collectedChanges.append(info.m_filePath);
                }
            });

        QTemporaryDir dir;
        UnitTestUtils::ScopedDir changeDir(dir.path());
        QDir tempPath(dir.path());
        NetworkRequestID requestId(1, 1);

        fileWatcher.AddFolderWatch(&folderWatch);
        fileWatcher.StartWatching();

        CreateDummyFile(tempPath.absoluteFilePath("bootstrap.cfg"), QString("sys_game_folder=SamplesProject\n"));

        // system is already actually initialized, along with gEnv, so this will always return that game name.
        QString gameName = AssetUtilities::ComputeGameName();

        // update the engine root
        AssetUtilities::ResetAssetRoot();
        QDir newRoot;
        AssetUtilities::ComputeAssetRoot(newRoot, &tempPath);

        // create a dummy file in the cache folder, so the folder structure gets created
        QDir projectCacheRoot;
        AssetUtilities::ComputeProjectCacheRoot(projectCacheRoot);
        CreateDummyFile(projectCacheRoot.absoluteFilePath("placeholder.txt"));

        UNIT_TEST_EXPECT_FALSE(gameName.isEmpty());
        // should create cache folder in the root, and read everything from there.

        QSet<QString> expectedFiles;
        // set up some interesting files:
        expectedFiles << tempPath.absoluteFilePath("rootfile2.txt");
        expectedFiles << tempPath.absoluteFilePath("subfolder1/rootfile1.txt"); // note:  Must override the actual root file
        expectedFiles << tempPath.absoluteFilePath("subfolder1/basefile.txt");
        expectedFiles << tempPath.absoluteFilePath("subfolder2/basefile.txt");
        expectedFiles << tempPath.absoluteFilePath("subfolder2/aaa/basefile.txt");
        expectedFiles << tempPath.absoluteFilePath("subfolder2/aaa/bbb/basefile.txt");
        expectedFiles << tempPath.absoluteFilePath("subfolder2/aaa/bbb/ccc/basefile.txt");
        expectedFiles << tempPath.absoluteFilePath("subfolder2/aaa/bbb/ccc/ddd/basefile.txt");
        expectedFiles << tempPath.absoluteFilePath("subfolder3/BaseFile.txt"); // note the case upper here
        expectedFiles << tempPath.absoluteFilePath("subfolder8/a/b/c/test.txt");

        // subfolder3 is not recursive so none of these should show up in any scan or override check
        expectedFiles << tempPath.absoluteFilePath("subfolder3/aaa/basefile.txt");
        expectedFiles << tempPath.absoluteFilePath("subfolder3/aaa/bbb/basefile.txt");
        expectedFiles << tempPath.absoluteFilePath("subfolder3/aaa/bbb/ccc/basefile.txt");

        expectedFiles << tempPath.absoluteFilePath("subfolder3/uniquefile.txt"); // only exists in subfolder3
        expectedFiles << tempPath.absoluteFilePath("subfolder3/uniquefile.ignore"); // only exists in subfolder3

        expectedFiles << tempPath.absoluteFilePath("subfolder3/rootfile3.txt"); // must override rootfile3 in root
        expectedFiles << tempPath.absoluteFilePath("rootfile1.txt");
        expectedFiles << tempPath.absoluteFilePath("rootfile3.txt");
        expectedFiles << tempPath.absoluteFilePath("unrecognised.file"); // a file that should not be recognised
        expectedFiles << tempPath.absoluteFilePath("unrecognised2.file"); // a file that should not be recognised
        expectedFiles << tempPath.absoluteFilePath("subfolder1/test/test.format"); // a file that should be recognised
        expectedFiles << tempPath.absoluteFilePath("test.format"); // a file that should NOT be recognised
        expectedFiles << tempPath.absoluteFilePath("subfolder3/somefile.xxx");
        expectedFiles << tempPath.absoluteFilePath("subfolder3/savebackup/test.txt");//file that should be excluded
        expectedFiles << tempPath.absoluteFilePath("subfolder3/somerandomfile.random");
        expectedFiles << tempPath.absoluteFilePath("subfolder2/folder/ship.tiff");

        // these will be used for the "check lock" tests
        expectedFiles << tempPath.absoluteFilePath("subfolder4/needsLock.tiff");
        expectedFiles << tempPath.absoluteFilePath("subfolder4/noLockNeeded.txt");

        // this will be used for the "rename a folder" test.
        expectedFiles << tempPath.absoluteFilePath("subfolder1/rename_this/somefile1.txt");
        expectedFiles << tempPath.absoluteFilePath("subfolder1/rename_this/somefolder/somefile2.txt");

        // this will be used for the "rename a folder" test with deep folders that don't contain files:
        expectedFiles << tempPath.absoluteFilePath("subfolder1/rename_this_secondly/somefolder/somefile2.txt");

        // this will be used for the "delete a SOURCE file" test.
        expectedFiles << tempPath.absoluteFilePath("subfolder1/to_be_deleted/some_deleted_file.txt");

        // this will be used for the "fewer products than last time" test.
        expectedFiles << tempPath.absoluteFilePath("subfolder1/fewer_products/test.txt");

        for (const QString& expect : expectedFiles)
        {
            UNIT_TEST_EXPECT_TRUE(CreateDummyFile(expect));
            AZ_TracePrintf(AssetProcessor::DebugChannel, "Created file %s with msecs %llu\n", expect.toUtf8().constData(),
                QFileInfo(expect).lastModified().toMSecsSinceEpoch());

#if defined(AZ_PLATFORM_WINDOWS)
            QThread::msleep(35); // give at least some milliseconds so that the files never share the same timestamp exactly
#else
            // on platforms such as mac, the file time resolution is only a second :(
            QThread::msleep(1001);
#endif
        }


        PlatformConfiguration config;
        config.EnablePlatform({ "pc",{ "desktop", "renderer" } }, true);
        config.EnablePlatform({ "es3",{ "mobile", "renderer" } }, true);
        config.EnablePlatform({ "fandago",{ "console", "renderer" } }, false);
        AZStd::vector<AssetBuilderSDK::PlatformInfo> platforms;
        config.PopulatePlatformsForScanFolder(platforms);
        //                                               PATH                 DisplayName  PortKey       outputfolder  root recurse  platforms   order
        config.AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder4"), "subfolder4", "subfolder4", "",          false, false, platforms, -6)); // subfolder 4 overrides subfolder3
        config.AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder3"), "subfolder3", "subfolder3", "",          false, false, platforms,-5)); // subfolder 3 overrides subfolder2
        config.AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder2"), "subfolder2", "subfolder2", "",          false, true,  platforms, -2)); // subfolder 2 overrides subfolder1
        config.AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder1"), "subfolder1", "subfolder1", "",          false, true,  platforms, -1)); // subfolder1 overrides root
        config.AddScanFolder(ScanFolderInfo(tempPath.absolutePath(),         "temp",       "tempfolder", "",          true, false,  platforms, 0)); // add the root


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
        UNIT_TEST_EXPECT_TRUE(mockAppManager.RegisterAssetRecognizerAsBuilder(rec));

        specpc.m_extraRCParams = ""; // blank must work
        speces3.m_extraRCParams = "testextraparams";

        const char* builderTxt1Name = "txt files";
        rec.m_name = builderTxt1Name;
        rec.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.txt", AssetBuilderSDK::AssetBuilderPattern::Wildcard);
        rec.m_platformSpecs.insert("pc", specpc);
        rec.m_platformSpecs.insert("es3", speces3);

        config.AddRecognizer(rec);

        mockAppManager.RegisterAssetRecognizerAsBuilder(rec);

        // test dual-recognisers - two recognisers for the same pattern.
        const char* builderTxt2Name = "txt files 2 (builder2)";
        rec.m_name = builderTxt2Name;
        config.AddRecognizer(rec);
        mockAppManager.RegisterAssetRecognizerAsBuilder(rec);

        rec.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher(".*\\/test\\/.*\\.format", AssetBuilderSDK::AssetBuilderPattern::Regex);
        rec.m_name = "format files that live in a folder called test";
        config.AddRecognizer(rec);
        mockAppManager.RegisterAssetRecognizerAsBuilder(rec);

        // tiff file recognizer
        rec.m_name = "tiff files";
        rec.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.tiff", AssetBuilderSDK::AssetBuilderPattern::Wildcard);
        rec.m_platformSpecs.clear();
        rec.m_platformSpecs.insert("pc", specpc);
        rec.m_testLockSource = true;
        config.AddRecognizer(rec);
        mockAppManager.RegisterAssetRecognizerAsBuilder(rec);

        rec.m_platformSpecs.clear();
        rec.m_testLockSource = false;

        specpc.m_extraRCParams = "pcparams";
        speces3.m_extraRCParams = "es3params";

        rec.m_name = "xxx files";
        rec.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.xxx", AssetBuilderSDK::AssetBuilderPattern::Wildcard);
        rec.m_platformSpecs.insert("pc", specpc);
        rec.m_platformSpecs.insert("es3", speces3);
        config.AddRecognizer(rec);
        mockAppManager.RegisterAssetRecognizerAsBuilder(rec);

        // two recognizers for the same pattern.
        rec.m_name = "xxx files 2 (builder2)";
        specpc.m_extraRCParams = "pcparams2";
        speces3.m_extraRCParams = "es3params2";
        rec.m_platformSpecs.insert("pc", specpc);
        rec.m_platformSpecs.insert("es3", speces3);
        config.AddRecognizer(rec);
        mockAppManager.RegisterAssetRecognizerAsBuilder(rec);

        // Ignore recognizer
        AssetPlatformSpec ignore_spec;
        ignore_spec.m_extraRCParams = "skip";
        AssetRecognizer ignore_rec;
        ignore_rec.m_name = "ignore files";
        ignore_rec.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.ignore", AssetBuilderSDK::AssetBuilderPattern::Wildcard);
        ignore_rec.m_platformSpecs.insert("pc", specpc);
        ignore_rec.m_platformSpecs.insert("es3", ignore_spec);
        config.AddRecognizer(ignore_rec);
        mockAppManager.RegisterAssetRecognizerAsBuilder(ignore_rec);
        

        ExcludeAssetRecognizer excludeRecogniser;
        excludeRecogniser.m_name = "backup";
        excludeRecogniser.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher(".*\\/savebackup\\/.*", AssetBuilderSDK::AssetBuilderPattern::Regex);
        config.AddExcludeRecognizer(excludeRecogniser);

        AssetProcessorManager_Test apm(&config);  // note, this will 'push' the scan folders in to the db.
        QDir cacheRoot;
        UNIT_TEST_EXPECT_TRUE(AssetUtilities::ComputeProjectCacheRoot(cacheRoot));
        QString normalizedCacheRoot = AssetUtilities::NormalizeDirectoryPath(cacheRoot.absolutePath());

        // make sure it picked up the one in the current folder

        QString normalizedDirPathCheck = AssetUtilities::NormalizeDirectoryPath(QDir::current().absoluteFilePath("Cache/" + gameName));
        UNIT_TEST_EXPECT_TRUE(normalizedCacheRoot == normalizedDirPathCheck);
        QDir normalizedCacheRootDir(normalizedCacheRoot);

        QList<JobDetails> processResults;
        QList<QPair<QString, QString> > changedInputResults;
        QList<QPair<QString, AzFramework::AssetSystem::AssetNotificationMessage> > assetMessages;

        bool idling = false;
        unsigned int CRC_Of_Change_Message =  AssetUtilities::ComputeCRC32Lowercase("AssetProcessorManager::assetChanged");

        connect(&apm, &AssetProcessorManager::AssetToProcess,
            this, [&processResults](JobDetails details)
            {
                //AZ_TracePrintf("Results", "ProcessResult: %s - %s - %s - %s - %u\n", file.toUtf8().constData(), platform.toUtf8().constData(), jobDescription.toUtf8().constData(), destination.toUtf8().constData(), originalFingerprint);
                processResults.push_back(AZStd::move(details));
            });

        connect(&apm, &AssetProcessorManager::AssetMessage,
            this, [&assetMessages](QString platform, AzFramework::AssetSystem::AssetNotificationMessage message)
            {
                assetMessages.push_back(qMakePair(platform, message));
            });

        connect(&apm, &AssetProcessorManager::InputAssetProcessed,
            this, [&changedInputResults](QString relativePath, QString platform)
            {
                changedInputResults.push_back(QPair<QString, QString>(relativePath, platform));
            });

        connect(&apm, &AssetProcessorManager::AssetProcessorManagerIdleState,
            this, [&idling](bool state)
            {
                idling = state;
            }
            );

        AssetProcessor::MockConnectionHandler connection;
        connection.BusConnect(1);
        QList<QPair<unsigned int, QByteArray> > payloadList;
        connection.m_callback = [&](unsigned int type, unsigned int serial, const QByteArray payload)
            {
                payloadList.append(qMakePair(type, payload));
            };

        // run the tests.
        // first, feed it things which it SHOULD ignore and should NOT generate any tasks:

        // the following is a file which does exist but should not be processed as it is in a non-watched folder (not recursive)
        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString,  tempPath.absoluteFilePath("subfolder3/aaa/basefile.txt")));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));
        UNIT_TEST_EXPECT_TRUE(processResults.isEmpty());
        UNIT_TEST_EXPECT_TRUE(changedInputResults.isEmpty());
        UNIT_TEST_EXPECT_TRUE(assetMessages.isEmpty());

        // an imaginary non-existent file should also fail even if it matches filters:
        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString,  tempPath.absoluteFilePath("subfolder3/basefileaaaaa.txt")));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));
        UNIT_TEST_EXPECT_TRUE(processResults.isEmpty());
        UNIT_TEST_EXPECT_TRUE(changedInputResults.isEmpty());
        UNIT_TEST_EXPECT_TRUE(assetMessages.isEmpty());

        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString,  tempPath.absoluteFilePath("basefileaaaaa.txt")));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));
        UNIT_TEST_EXPECT_TRUE(processResults.isEmpty());
        UNIT_TEST_EXPECT_TRUE(changedInputResults.isEmpty());
        UNIT_TEST_EXPECT_TRUE(assetMessages.isEmpty());

        // block until no more events trickle in:
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        processResults.clear();

        QString inputIgnoreFilePath = AssetUtilities::NormalizeFilePath(tempPath.absoluteFilePath("subfolder3/uniquefile.ignore"));

        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, inputIgnoreFilePath));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        // block until no more events trickle in:
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        sortAssetToProcessResultList(processResults);

        UNIT_TEST_EXPECT_TRUE(processResults.size() == 1); // 1, since we have one recognizer for .ignore, but the 'es3' platform is marked as skip
        UNIT_TEST_EXPECT_TRUE((processResults[0].m_jobEntry.m_platformInfo.m_identifier == "pc"));


        // block until no more events trickle in:
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        processResults.clear();

        // give it a file that should actually cause the generation of a task:

        QString inputFilePath = AssetUtilities::NormalizeFilePath(tempPath.absoluteFilePath("subfolder3/uniquefile.txt"));

        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, inputFilePath));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        // block until no more events trickle in:
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        sortAssetToProcessResultList(processResults);

        UNIT_TEST_EXPECT_TRUE(processResults.size() == 4); // 2 each for pc and es3,since we have two recognizer for .txt file
        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_platformInfo.m_identifier == processResults[1].m_jobEntry.m_platformInfo.m_identifier);
        UNIT_TEST_EXPECT_TRUE(processResults[2].m_jobEntry.m_platformInfo.m_identifier == processResults[3].m_jobEntry.m_platformInfo.m_identifier);
        UNIT_TEST_EXPECT_TRUE((processResults[0].m_jobEntry.m_platformInfo.m_identifier == "es3"));
        UNIT_TEST_EXPECT_TRUE((processResults[1].m_jobEntry.m_platformInfo.m_identifier == "es3"));
        UNIT_TEST_EXPECT_TRUE((processResults[2].m_jobEntry.m_platformInfo.m_identifier == "pc"));
        UNIT_TEST_EXPECT_TRUE((processResults[3].m_jobEntry.m_platformInfo.m_identifier == "pc"));


        QList<int> es3JobsIndex;
        QList<int> pcJobsIndex;
        for (int checkIdx = 0; checkIdx < 4; ++checkIdx)
        {
            UNIT_TEST_EXPECT_TRUE(processResults[checkIdx].m_jobEntry.m_computedFingerprint != 0);
            UNIT_TEST_EXPECT_TRUE(processResults[checkIdx].m_jobEntry.m_jobRunKey != 0);
            UNIT_TEST_EXPECT_TRUE(processResults[checkIdx].m_jobEntry.m_absolutePathToFile == inputFilePath);
            UNIT_TEST_EXPECT_TRUE(processResults[checkIdx].m_jobEntry.m_relativePathToFile == "uniquefile.txt");
            QString platformFolder = cacheRoot.filePath(QString::fromUtf8(processResults[checkIdx].m_jobEntry.m_platformInfo.m_identifier.c_str()) + "/" + gameName.toLower());
            platformFolder = AssetUtilities::NormalizeDirectoryPath(platformFolder);
            UNIT_TEST_EXPECT_TRUE(processResults[checkIdx].m_destinationPath.startsWith(platformFolder));
            UNIT_TEST_EXPECT_TRUE(processResults[checkIdx].m_jobEntry.m_computedFingerprint != 0);

            QMetaObject::invokeMethod(&apm, "OnJobStatusChanged", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[checkIdx].m_jobEntry), Q_ARG(JobStatus, JobStatus::Queued));

            QCoreApplication::processEvents(QEventLoop::AllEvents);

            // create log files, so that we can test the correct retrieval

            // we create all of them except for #1
            if (checkIdx != 1)
            {
                JobInfo info;
                info.m_jobRunKey = processResults[checkIdx].m_jobEntry.m_jobRunKey;
                info.m_builderGuid = processResults[checkIdx].m_jobEntry.m_builderGuid;
                info.m_jobKey = processResults[checkIdx].m_jobEntry.m_jobKey.toUtf8().data();
                info.m_platform = processResults[checkIdx].m_jobEntry.m_platformInfo.m_identifier.c_str();
                info.m_sourceFile = processResults[checkIdx].m_jobEntry.m_relativePathToFile.toUtf8().data();

                AZStd::string logFolder = AZStd::string::format("%s/%s", AssetUtilities::ComputeJobLogFolder().c_str(), AssetUtilities::ComputeJobLogFileName(info).c_str());
                AZ::IO::HandleType logHandle;
                AZ::IO::LocalFileIO::GetInstance()->CreatePath(AssetUtilities::ComputeJobLogFolder().c_str());
                UNIT_TEST_EXPECT_TRUE(AZ::IO::LocalFileIO::GetInstance()->Open(logFolder.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeBinary, logHandle));
                AZStd::string logLine = AZStd::string::format("Log stored for job run key %lli\n", processResults[checkIdx].m_jobEntry.m_jobRunKey);
                AZ::IO::LocalFileIO::GetInstance()->Write(logHandle, logLine.c_str(), logLine.size());
                AZ::IO::LocalFileIO::GetInstance()->Close(logHandle);
            }
        }



        // ----------------------- test job info requests, while we have some assets in flight ---------------------------

        // by this time, querying for the status of those jobs should be possible since the "OnJobStatusChanged" event should have bubbled through
        {
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            AssetJobsInfoRequest requestInfo;
            AssetJobsInfoResponse jobResponse;

            requestInfo.m_searchTerm = inputFilePath.toUtf8().constData();

            {
                // send our request:
                payloadList.clear();
                connection.m_sent = false;
                apm.ProcessGetAssetJobsInfoRequest(requestId, &requestInfo);


                // wait for it to process:
                QCoreApplication::processEvents(QEventLoop::AllEvents);

                UNIT_TEST_EXPECT_TRUE(connection.m_sent); // we expect a response to have arrived.

                unsigned int jobinformationResultIndex = -1;
                for (int index = 0; index < payloadList.size(); index++)
                {
                    unsigned int type = payloadList.at(index).first;
                    if (type == requestInfo.GetMessageType())
                    {
                        jobinformationResultIndex = index;
                    }
                }
                UNIT_TEST_EXPECT_FALSE(jobinformationResultIndex == -1);
                UNIT_TEST_EXPECT_TRUE(AZ::Utils::LoadObjectFromBufferInPlace(payloadList.at(jobinformationResultIndex).second.data(), payloadList.at(jobinformationResultIndex).second.size(), jobResponse));
            }

            UNIT_TEST_EXPECT_TRUE(jobResponse.m_isSuccess);
            UNIT_TEST_EXPECT_TRUE(jobResponse.m_jobList.size() == processResults.size());

            // make sure each job corresponds to one in the process results list (but note that the order is not important).
            for (int oldJobIdx = azlossy_cast<int>(jobResponse.m_jobList.size()) - 1; oldJobIdx >= 0; --oldJobIdx)
            {
                bool foundIt = false;
                const JobInfo& jobInfo = jobResponse.m_jobList[oldJobIdx];

                // validate EVERY field
                UNIT_TEST_EXPECT_TRUE(jobInfo.m_status == JobStatus::Queued);
                UNIT_TEST_EXPECT_FALSE(jobInfo.m_sourceFile.empty());
                UNIT_TEST_EXPECT_FALSE(jobInfo.m_platform.empty());
                UNIT_TEST_EXPECT_FALSE(jobInfo.m_jobKey.empty());
                UNIT_TEST_EXPECT_FALSE(jobInfo.m_builderGuid.IsNull());
                UNIT_TEST_EXPECT_TRUE(jobInfo.m_jobRunKey != 0);

                for (const JobDetails& details : processResults)
                {
                    if ((QString::compare(jobInfo.m_sourceFile.c_str(), details.m_jobEntry.m_relativePathToFile, Qt::CaseInsensitive) == 0) &&
                        (QString::compare(jobInfo.m_platform.c_str(), details.m_jobEntry.m_platformInfo.m_identifier.c_str(), Qt::CaseInsensitive) == 0) &&
                        (QString::compare(jobInfo.m_jobKey.c_str(), details.m_jobEntry.m_jobKey, Qt::CaseInsensitive) == 0) &&
                        (jobInfo.m_builderGuid == details.m_jobEntry.m_builderGuid) &&
                        (jobInfo.m_jobRunKey == details.m_jobEntry.m_jobRunKey) &&
                        (jobInfo.GetHash() == details.m_jobEntry.GetHash()))
                    {
                        foundIt = true;
                        break;
                    }
                }
                UNIT_TEST_EXPECT_TRUE(foundIt);
            }
        }

        // ------------- JOB LOG TEST -------------------
        for (int checkIdx = 0; checkIdx < 4; ++checkIdx)
        {
            const JobDetails& details = processResults[checkIdx];
            // create log files, so that we can test the correct retrieval

            // we create all of them except for #1
            if (checkIdx != 1)
            {
                AZStd::string logFolder = AZStd::string::format("%s/%s", AssetUtilities::ComputeJobLogFolder().c_str(), AssetUtilities::ComputeJobLogFileName(details.m_jobEntry).c_str());
                AZ::IO::HandleType logHandle;
                AZ::IO::LocalFileIO::GetInstance()->CreatePath(AssetUtilities::ComputeJobLogFolder().c_str());
                UNIT_TEST_EXPECT_TRUE(AZ::IO::LocalFileIO::GetInstance()->Open(logFolder.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeBinary, logHandle));
                AZStd::string logLine = AZStd::string::format("Log stored for job %lli\n", processResults[checkIdx].m_jobEntry.GetHash());
                AZ::IO::LocalFileIO::GetInstance()->Write(logHandle, logLine.c_str(), logLine.size());
                AZ::IO::LocalFileIO::GetInstance()->Close(logHandle);
            }
        }

        for (int checkIdx = 0; checkIdx < 4; ++checkIdx)
        {
            const JobDetails& details = processResults[checkIdx];

            // request job logs.
            AssetJobLogRequest requestLog;
            AssetJobLogResponse requestResponse;
            requestLog.m_jobRunKey = details.m_jobEntry.m_jobRunKey;
            {
                // send our request:
                payloadList.clear();
                connection.m_sent = false;
                apm.ProcessGetAssetJobLogRequest(requestId, &requestLog);

                // wait for it to process:
                QCoreApplication::processEvents(QEventLoop::AllEvents);

                UNIT_TEST_EXPECT_TRUE(connection.m_sent); // we expect a response to have arrived.

                unsigned int jobLogResponseIndex = -1;
                for (int index = 0; index < payloadList.size(); index++)
                {
                    unsigned int type = payloadList.at(index).first;
                    if (type == requestLog.GetMessageType())
                    {
                        jobLogResponseIndex = index;
                    }
                }
                UNIT_TEST_EXPECT_FALSE(jobLogResponseIndex == -1);
                UNIT_TEST_EXPECT_TRUE(AZ::Utils::LoadObjectFromBufferInPlace(payloadList.at(jobLogResponseIndex).second.data(), payloadList.at(jobLogResponseIndex).second.size(), requestResponse));


                if (checkIdx != 1)
                {
                    UNIT_TEST_EXPECT_TRUE(requestResponse.m_isSuccess);
                    UNIT_TEST_EXPECT_FALSE(requestResponse.m_jobLog.empty());
                    AZStd::string checkString = AZStd::string::format("Log stored for job %lli\n", processResults[checkIdx].m_jobEntry.GetHash());
                    UNIT_TEST_EXPECT_TRUE(requestResponse.m_jobLog.find(checkString.c_str()) != AZStd::string::npos);
                }
                else
                {
                    // the [1] index was not written so it should be failed and empty
                    UNIT_TEST_EXPECT_FALSE(requestResponse.m_isSuccess);
                }
            }
        }

        // now indicate the job has started.
        for (const JobDetails& details : processResults)
        {
            apm.OnJobStatusChanged(details.m_jobEntry, JobStatus::InProgress);
        }
        QCoreApplication::processEvents(QEventLoop::AllEvents);


        // ----------------------- test job info requests, while we have some assets in flight ---------------------------

        // by this time, querying for the status of those jobs should be possible since the "OnJobStatusChanged" event should have bubbled through
        // and this time, it should be "in progress"
        {
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            AssetJobsInfoRequest requestInfo;

            requestInfo.m_searchTerm = inputFilePath.toUtf8().constData();

            {
                // send our request:
                payloadList.clear();
                connection.m_sent = false;
                apm.ProcessGetAssetJobsInfoRequest(requestId, &requestInfo);
            }

            // wait for it to process:
            QCoreApplication::processEvents(QEventLoop::AllEvents);

            UNIT_TEST_EXPECT_TRUE(connection.m_sent); // we expect a response to have arrived.

            unsigned int jobinformationResultIndex = -1;
            for (int index = 0; index < payloadList.size(); index++)
            {
                unsigned int type = payloadList.at(index).first;
                if (type == requestInfo.GetMessageType())
                {
                    jobinformationResultIndex = index;
                }
            }
            UNIT_TEST_EXPECT_FALSE(jobinformationResultIndex == -1);

            AssetJobsInfoResponse jobResponse;
            UNIT_TEST_EXPECT_TRUE(AZ::Utils::LoadObjectFromBufferInPlace(payloadList.at(jobinformationResultIndex).second.data(), payloadList.at(jobinformationResultIndex).second.size(), jobResponse));

            UNIT_TEST_EXPECT_TRUE(jobResponse.m_isSuccess);
            UNIT_TEST_EXPECT_TRUE(jobResponse.m_jobList.size() == processResults.size());

            // make sure each job corresponds to one in the process results list (but note that the order is not important).
            for (int oldJobIdx = azlossy_cast<int>(jobResponse.m_jobList.size()) - 1; oldJobIdx >= 0; --oldJobIdx)
            {
                bool foundIt = false;
                const JobInfo& jobInfo = jobResponse.m_jobList[oldJobIdx];

                // validate EVERY field
                UNIT_TEST_EXPECT_TRUE(jobInfo.m_status == JobStatus::InProgress);
                UNIT_TEST_EXPECT_FALSE(jobInfo.m_sourceFile.empty());
                UNIT_TEST_EXPECT_FALSE(jobInfo.m_platform.empty());
                UNIT_TEST_EXPECT_FALSE(jobInfo.m_jobKey.empty());
                UNIT_TEST_EXPECT_FALSE(jobInfo.m_builderGuid.IsNull());

                for (const JobDetails& details : processResults)
                {
                    if ((QString::compare(jobInfo.m_sourceFile.c_str(), details.m_jobEntry.m_relativePathToFile, Qt::CaseInsensitive) == 0) &&
                        (QString::compare(jobInfo.m_platform.c_str(), details.m_jobEntry.m_platformInfo.m_identifier.c_str(), Qt::CaseInsensitive) == 0) &&
                        (QString::compare(jobInfo.m_jobKey.c_str(), details.m_jobEntry.m_jobKey, Qt::CaseInsensitive) == 0) &&
                        (jobInfo.m_builderGuid == details.m_jobEntry.m_builderGuid) &&
                        (jobInfo.GetHash() == details.m_jobEntry.GetHash()))
                    {
                        foundIt = true;
                        break;
                    }
                }
                UNIT_TEST_EXPECT_TRUE(foundIt);
            }
        }

        // ---------- test successes ----------


        QStringList es3outs;
        es3outs.push_back(cacheRoot.filePath(QString("es3/") + gameName + "/basefile.arc1"));
        es3outs.push_back(cacheRoot.filePath(QString("es3/") + gameName + "/basefile.arc2"));

        // feed it the messages its waiting for (create the files)
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(es3outs[0], "products."));
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(es3outs[1], "products."))

        //Invoke Asset Processed for es3 platform , txt files job description
        AssetBuilderSDK::ProcessJobResponse response;
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(es3outs[0].toUtf8().constData()));
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(es3outs[1].toUtf8().constData()));
        
        // make sure legacy SubIds get stored in the DB and in asset response messages.
        // also make sure they don't get filed for the wrong asset.
        response.m_outputProducts[0].m_legacySubIDs.push_back(1234);
        response.m_outputProducts[0].m_legacySubIDs.push_back(5678);
        response.m_outputProducts[1].m_legacySubIDs.push_back(2222);

        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[0].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        // let events bubble through:
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        UNIT_TEST_EXPECT_TRUE(assetMessages.size() == 2);

        UNIT_TEST_EXPECT_TRUE(changedInputResults.size() == 1);

        // always RELATIVE, always with the product name.
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].first == "es3");
        UNIT_TEST_EXPECT_TRUE(assetMessages[1].first == "es3");
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].second.m_data == "basefile.arc1");
        UNIT_TEST_EXPECT_TRUE(assetMessages[1].second.m_data == "basefile.arc2");
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].second.m_type == AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged);
        UNIT_TEST_EXPECT_TRUE(assetMessages[1].second.m_type == AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged);
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].second.m_sizeBytes != 0);
        UNIT_TEST_EXPECT_TRUE(assetMessages[1].second.m_sizeBytes != 0);
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].second.m_assetId.IsValid());
        UNIT_TEST_EXPECT_TRUE(assetMessages[1].second.m_assetId.IsValid());
        UNIT_TEST_EXPECT_TRUE(!assetMessages[0].second.m_legacyAssetIds.empty());
        UNIT_TEST_EXPECT_TRUE(!assetMessages[1].second.m_legacyAssetIds.empty());
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].second.m_legacyAssetIds[0].IsValid());
        UNIT_TEST_EXPECT_TRUE(assetMessages[1].second.m_legacyAssetIds[0].IsValid());
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].second.m_legacyAssetIds[0] != assetMessages[0].second.m_assetId);
        UNIT_TEST_EXPECT_TRUE(assetMessages[1].second.m_legacyAssetIds[0] != assetMessages[1].second.m_assetId);

        UNIT_TEST_EXPECT_TRUE(assetMessages[0].second.m_legacyAssetIds.size() == 3);
        UNIT_TEST_EXPECT_TRUE(assetMessages[1].second.m_legacyAssetIds.size() == 2);

        UNIT_TEST_EXPECT_TRUE(assetMessages[0].second.m_legacyAssetIds[1].m_subId == 1234);
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].second.m_legacyAssetIds[2].m_subId == 5678);
        UNIT_TEST_EXPECT_TRUE(assetMessages[1].second.m_legacyAssetIds[1].m_subId == 2222);

        UNIT_TEST_EXPECT_TRUE(AssetUtilities::NormalizeFilePath(changedInputResults[0].first) == AssetUtilities::NormalizeFilePath(inputFilePath));

        // ----------------------- test job info requests, when some assets are done.
        {
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            AssetJobsInfoRequest requestInfo;
            bool escalated = false;
            int numEscalated = 0;

            requestInfo.m_escalateJobs = true;
            requestInfo.m_searchTerm = inputFilePath.toUtf8().constData();
            auto connectionMade = QObject::connect(&apm, &AssetProcessorManager::EscalateJobs, [&escalated, &numEscalated](AssetProcessor::JobIdEscalationList jobList)

                    {
                        escalated = true;
                        numEscalated = jobList.size();
                    });

            // send our request:
            payloadList.clear();
            connection.m_sent = false;
            apm.ProcessGetAssetJobsInfoRequest(requestId, &requestInfo);

            // wait for it to process:
            QCoreApplication::processEvents(QEventLoop::AllEvents);

            QObject::disconnect(connectionMade);

            UNIT_TEST_EXPECT_TRUE(connection.m_sent); // we expect a response to have arrived.
            UNIT_TEST_EXPECT_TRUE(escalated);
            UNIT_TEST_EXPECT_TRUE(numEscalated > 0);

            unsigned int jobinformationResultIndex = -1;
            for (int index = 0; index < payloadList.size(); index++)
            {
                unsigned int type = payloadList.at(index).first;
                if (type == requestInfo.GetMessageType())
                {
                    jobinformationResultIndex = index;
                }
            }
            UNIT_TEST_EXPECT_FALSE(jobinformationResultIndex == -1);

            AssetJobsInfoResponse jobResponse;
            UNIT_TEST_EXPECT_TRUE(AZ::Utils::LoadObjectFromBufferInPlace(payloadList.at(jobinformationResultIndex).second.data(), payloadList.at(jobinformationResultIndex).second.size(), jobResponse));

            UNIT_TEST_EXPECT_TRUE(jobResponse.m_isSuccess);
            UNIT_TEST_EXPECT_TRUE(jobResponse.m_jobList.size() == processResults.size());

            // make sure each job corresponds to one in the process results list (but note that the order is not important).
            for (int oldJobIdx = azlossy_cast<int>(jobResponse.m_jobList.size()) - 1; oldJobIdx >= 0; --oldJobIdx)
            {
                bool foundIt = false;
                const JobInfo& jobInfo = jobResponse.m_jobList[oldJobIdx];

                // validate EVERY field
                UNIT_TEST_EXPECT_FALSE(jobInfo.m_sourceFile.empty());
                UNIT_TEST_EXPECT_FALSE(jobInfo.m_platform.empty());
                UNIT_TEST_EXPECT_FALSE(jobInfo.m_jobKey.empty());
                UNIT_TEST_EXPECT_FALSE(jobInfo.m_builderGuid.IsNull());

                for (int detailsIdx = 0; detailsIdx < processResults.size(); ++detailsIdx)
                {
                    const JobDetails& details = processResults[detailsIdx];

                    if ((QString::compare(jobInfo.m_sourceFile.c_str(), details.m_jobEntry.m_relativePathToFile, Qt::CaseInsensitive) == 0) &&
                        (QString::compare(jobInfo.m_platform.c_str(), details.m_jobEntry.m_platformInfo.m_identifier.c_str(), Qt::CaseInsensitive) == 0) &&
                        (QString::compare(jobInfo.m_jobKey.c_str(), details.m_jobEntry.m_jobKey, Qt::CaseInsensitive) == 0) &&
                        (jobInfo.m_builderGuid == details.m_jobEntry.m_builderGuid) &&
                        (jobInfo.GetHash() == details.m_jobEntry.GetHash()))
                    {
                        foundIt = true;

                        if (detailsIdx == 0) // we only said that the first job was done
                        {
                            UNIT_TEST_EXPECT_TRUE(jobInfo.m_status == JobStatus::Completed);
                        }
                        else
                        {
                            UNIT_TEST_EXPECT_TRUE(jobInfo.m_status == JobStatus::InProgress);
                        }

                        break;
                    }
                }
                UNIT_TEST_EXPECT_TRUE(foundIt);
            }
        }

        changedInputResults.clear();
        assetMessages.clear();

        es3outs.clear();
        es3outs.push_back(cacheRoot.filePath(QString("es3/") + gameName + "/basefile.azm"));
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(es3outs[0], "products."));

        //Invoke Asset Processed for es3 platform , txt files2 job description
        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(es3outs[0].toUtf8().constData()));

        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[1].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        // let events bubble through:
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        UNIT_TEST_EXPECT_TRUE(assetMessages.size() == 1);
        UNIT_TEST_EXPECT_TRUE(changedInputResults.size() == 1);

        // always RELATIVE, always with the product name.
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].first == "es3");
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].second.m_data == "basefile.azm");

        changedInputResults.clear();
        assetMessages.clear();

        QStringList pcouts;
        pcouts.push_back(cacheRoot.filePath(QString("pc/") + gameName + "/basefile.arc1"));
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(pcouts[0], "products."));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcouts[0].toUtf8().constData()));

        //Invoke Asset Processed for pc platform , txt files job description
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[2].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        // let events bubble through:
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        UNIT_TEST_EXPECT_TRUE(assetMessages.size() == 1);
        UNIT_TEST_EXPECT_TRUE(changedInputResults.size() == 1);

        // always RELATIVE, always with the product name.
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].first == "pc");
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].second.m_data == "basefile.arc1");


        UNIT_TEST_EXPECT_TRUE(AssetUtilities::NormalizeFilePath(changedInputResults[0].first) == AssetUtilities::NormalizeFilePath(inputFilePath));

        changedInputResults.clear();
        assetMessages.clear();

        pcouts.clear();
        pcouts.push_back(cacheRoot.filePath(QString("pc/") + gameName + "/basefile.azm"));
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(pcouts[0], "products."));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcouts[0].toUtf8().constData()));

        //Invoke Asset Processed for pc platform , txt files 2 job description
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[3].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        // let events bubble through:
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        UNIT_TEST_EXPECT_TRUE(assetMessages.size() == 1);
        UNIT_TEST_EXPECT_TRUE(changedInputResults.size() == 1);

        // always RELATIVE, always with the product name.
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].first == "pc");
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].second.m_data == "basefile.azm");

        UNIT_TEST_EXPECT_TRUE(AssetUtilities::NormalizeFilePath(changedInputResults[0].first) == AssetUtilities::NormalizeFilePath(inputFilePath));

        // all four should now be complete:
        // ----------------------- test job info requests, now that all are done ---------------------------

        // by this time, querying for the status of those jobs should be possible since the "OnJobStatusChanged" event should have bubbled through
        // and this time, it should be "in progress"
        {
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            AssetJobsInfoRequest requestInfo;

            requestInfo.m_searchTerm = inputFilePath.toUtf8().constData();

            {
                // send our request:
                payloadList.clear();
                connection.m_sent = false;
                apm.ProcessGetAssetJobsInfoRequest(requestId, &requestInfo);
            }

            // wait for it to process:
            QCoreApplication::processEvents(QEventLoop::AllEvents);

            UNIT_TEST_EXPECT_TRUE(connection.m_sent); // we expect a response to have arrived.

            unsigned int jobinformationResultIndex = -1;
            for (int index = 0; index < payloadList.size(); index++)
            {
                unsigned int type = payloadList.at(index).first;
                if (type == requestInfo.GetMessageType())
                {
                    jobinformationResultIndex = index;
                }
            }
            UNIT_TEST_EXPECT_FALSE(jobinformationResultIndex == -1);

            AssetJobsInfoResponse jobResponse;
            UNIT_TEST_EXPECT_TRUE(AZ::Utils::LoadObjectFromBufferInPlace(payloadList.at(jobinformationResultIndex).second.data(), payloadList.at(jobinformationResultIndex).second.size(), jobResponse));

            UNIT_TEST_EXPECT_TRUE(jobResponse.m_isSuccess);
            UNIT_TEST_EXPECT_TRUE(jobResponse.m_jobList.size() == processResults.size());

            // make sure each job corresponds to one in the process results list (but note that the order is not important).
            for (int oldJobIdx = azlossy_cast<int>(jobResponse.m_jobList.size()) - 1; oldJobIdx >= 0; --oldJobIdx)
            {
                bool foundIt = false;
                const JobInfo& jobInfo = jobResponse.m_jobList[oldJobIdx];

                // validate EVERY field
                UNIT_TEST_EXPECT_TRUE(jobInfo.m_status == JobStatus::Completed);
                UNIT_TEST_EXPECT_FALSE(jobInfo.m_sourceFile.empty());
                UNIT_TEST_EXPECT_FALSE(jobInfo.m_platform.empty());
                UNIT_TEST_EXPECT_FALSE(jobInfo.m_jobKey.empty());
                UNIT_TEST_EXPECT_FALSE(jobInfo.m_builderGuid.IsNull());

                for (const JobDetails& details : processResults)
                {
                    if ((QString::compare(jobInfo.m_sourceFile.c_str(), details.m_jobEntry.m_relativePathToFile, Qt::CaseInsensitive) == 0) &&
                        (QString::compare(jobInfo.m_platform.c_str(), details.m_jobEntry.m_platformInfo.m_identifier.c_str(), Qt::CaseInsensitive) == 0) &&
                        (QString::compare(jobInfo.m_jobKey.c_str(), details.m_jobEntry.m_jobKey, Qt::CaseInsensitive) == 0) &&
                        (jobInfo.m_builderGuid == details.m_jobEntry.m_builderGuid) &&
                        (jobInfo.GetHash() == details.m_jobEntry.GetHash()))
                    {
                        foundIt = true;
                        break;
                    }
                }
                UNIT_TEST_EXPECT_TRUE(foundIt);
            }
        }

        changedInputResults.clear();
        assetMessages.clear();
        processResults.clear();

        // feed it the exact same file again.
        // this should result in NO ADDITIONAL processes since nothing has changed.
        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, inputFilePath));
        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        UNIT_TEST_EXPECT_TRUE(processResults.isEmpty());
        UNIT_TEST_EXPECT_TRUE(changedInputResults.isEmpty());
        UNIT_TEST_EXPECT_TRUE(assetMessages.isEmpty());

        // delete one of the products and tell it that it changed
        // it should reprocess that file, for that platform only:

        payloadList.clear();
        connection.m_sent = false;

        AssetNotificationMessage assetNotifMessage;
        SourceFileNotificationMessage sourceFileChangedMessage;

        // this should result in NO ADDITIONAL processes since nothing has changed.
        UNIT_TEST_EXPECT_TRUE(QFile::remove(pcouts[0]));
        QMetaObject::invokeMethod(&apm, "AssessDeletedFile", Qt::QueuedConnection, Q_ARG(QString, pcouts[0]));
        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        UNIT_TEST_EXPECT_TRUE(connection.m_sent);
        UNIT_TEST_EXPECT_TRUE(payloadList.size() == 1);// We should receive two messages one assetRemoved message and the other SourceAssetChanged message
        unsigned int messageLoadCount = 0;
        for (auto payload : payloadList)
        {
            if (payload.first == SourceFileNotificationMessage::MessageType())
            {
                UNIT_TEST_EXPECT_TRUE(AZ::Utils::LoadObjectFromBufferInPlace(payload.second.data(), payload.second.size(), sourceFileChangedMessage));
                UNIT_TEST_EXPECT_TRUE(sourceFileChangedMessage.m_type == SourceFileNotificationMessage::FileChanged);
                ++messageLoadCount;
            }
        }

        UNIT_TEST_EXPECT_TRUE(messageLoadCount == payloadList.size()); // ensure we found both messages
        UNIT_TEST_EXPECT_TRUE(pcouts[0].contains(QString(assetNotifMessage.m_data.c_str()), Qt::CaseInsensitive));

        QDir scanFolder(sourceFileChangedMessage.m_scanFolder.c_str());
        QString pathToCheck(scanFolder.filePath(sourceFileChangedMessage.m_relativeSourcePath.c_str()));
        UNIT_TEST_EXPECT_TRUE(QString::compare(inputFilePath, pathToCheck, Qt::CaseInsensitive) == 0);

        // should have asked to launch only the PC process because the other assets are already done for the other plat
        UNIT_TEST_EXPECT_TRUE(processResults.size() == 1);
        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_platformInfo.m_identifier == "pc");
        UNIT_TEST_EXPECT_TRUE(AssetUtilities::NormalizeFilePath(processResults[0].m_jobEntry.m_absolutePathToFile) == AssetUtilities::NormalizeFilePath(inputFilePath));

        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(pcouts[0], "products2"));
        // tell it were done again!

        changedInputResults.clear();
        assetMessages.clear();

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcouts[0].toUtf8().constData()));

        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[0].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        // let events bubble through:
        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        UNIT_TEST_EXPECT_TRUE(assetMessages.size() == 1);
        UNIT_TEST_EXPECT_TRUE(changedInputResults.size() == 1);

        // always RELATIVE, always with the product name.
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].second.m_data == "basefile.azm");
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].first == "pc");
        UNIT_TEST_EXPECT_TRUE(AssetUtilities::NormalizeFilePath(changedInputResults[0].first) == AssetUtilities::NormalizeFilePath(inputFilePath));

        changedInputResults.clear();
        assetMessages.clear();
        processResults.clear();

        connection.m_sent = false;
        payloadList.clear();

        // modify the input file, then
        // feed it the exact same file again.
        // it should spawn BOTH compilers:
        UNIT_TEST_EXPECT_TRUE(QFile::remove(inputFilePath));
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(inputFilePath, "new!"));
        AZ_TracePrintf(AssetProcessor::DebugChannel, "-------------------------------------------\n");

        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, inputFilePath));
        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        UNIT_TEST_EXPECT_TRUE(connection.m_sent);
        UNIT_TEST_EXPECT_TRUE(payloadList.size() == 1);// We should always receive only one of these messages
        UNIT_TEST_EXPECT_TRUE(AZ::Utils::LoadObjectFromBufferInPlace(payloadList.at(0).second.data(), payloadList.at(0).second.size(), sourceFileChangedMessage));
        scanFolder = QDir(sourceFileChangedMessage.m_scanFolder.c_str());
        pathToCheck = scanFolder.filePath(sourceFileChangedMessage.m_relativeSourcePath.c_str());
        UNIT_TEST_EXPECT_TRUE(QString::compare(inputFilePath, pathToCheck, Qt::CaseInsensitive) == 0);

        sortAssetToProcessResultList(processResults);

        // --------- same result as above ----------
        UNIT_TEST_EXPECT_TRUE(processResults.size() == 4); // 2 each for pc and es3,since we have two recognizer for .txt file
        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_platformInfo.m_identifier == processResults[1].m_jobEntry.m_platformInfo.m_identifier);
        UNIT_TEST_EXPECT_TRUE(processResults[2].m_jobEntry.m_platformInfo.m_identifier == processResults[3].m_jobEntry.m_platformInfo.m_identifier);
        UNIT_TEST_EXPECT_TRUE((processResults[0].m_jobEntry.m_platformInfo.m_identifier == "es3"));
        UNIT_TEST_EXPECT_TRUE((processResults[1].m_jobEntry.m_platformInfo.m_identifier == "es3"));
        UNIT_TEST_EXPECT_TRUE((processResults[2].m_jobEntry.m_platformInfo.m_identifier == "pc"));
        UNIT_TEST_EXPECT_TRUE((processResults[3].m_jobEntry.m_platformInfo.m_identifier == "pc"));
        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_computedFingerprint != 0);
        UNIT_TEST_EXPECT_TRUE(processResults[1].m_jobEntry.m_computedFingerprint != 0);

        for (int checkIdx = 0; checkIdx < 4; ++checkIdx)
        {
            QString processFile1 = processResults[checkIdx].m_jobEntry.m_absolutePathToFile;
            UNIT_TEST_EXPECT_TRUE(processFile1 == inputFilePath);
            QString platformFolder = cacheRoot.filePath(QString::fromUtf8(processResults[checkIdx].m_jobEntry.m_platformInfo.m_identifier.c_str()) + "/" + gameName.toLower());
            platformFolder = AssetUtilities::NormalizeDirectoryPath(platformFolder);
            processFile1 = processResults[checkIdx].m_destinationPath;
            UNIT_TEST_EXPECT_TRUE(processFile1.startsWith(platformFolder));
            UNIT_TEST_EXPECT_TRUE(processResults[checkIdx].m_jobEntry.m_computedFingerprint != 0);
        }

        // this time make different products:

        QStringList oldes3outs;
        QStringList oldpcouts;
        oldes3outs = es3outs;
        oldpcouts.append(pcouts);
        QStringList es3outs2;
        QStringList pcouts2;
        es3outs.clear();
        pcouts.clear();
        es3outs.push_back(cacheRoot.filePath(QString("es3/") + gameName + "/basefilea.arc1"));
        es3outs2.push_back(cacheRoot.filePath(QString("es3/") + gameName + "/basefilea.azm"));
        // note that the ES3 outs have changed
        // but the pc outs are still the same.
        pcouts.push_back(cacheRoot.filePath(QString("pc/") + gameName + "/basefile.arc1"));
        pcouts2.push_back(cacheRoot.filePath(QString("pc/") + gameName + "/basefile.azm"));

        // feed it the messages its waiting for (create the files)
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(es3outs[0], "newfile."));
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(pcouts[0], "newfile."));
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(es3outs2[0], "newfile."));
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(pcouts2[0], "newfile."));

        QCoreApplication::processEvents(QEventLoop::AllEvents | QEventLoop::WaitForMoreEvents, 50);

        changedInputResults.clear();
        assetMessages.clear();

        // send all the done messages simultaneously:

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(es3outs[0].toUtf8().constData()));
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[0].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(es3outs2[0].toUtf8().constData()));
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[1].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcouts[0].toUtf8().constData()));
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[2].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcouts2[0].toUtf8().constData()));
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[3].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        // let events bubble through:
        QCoreApplication::processEvents(QEventLoop::AllEvents | QEventLoop::WaitForMoreEvents, 50);

        UNIT_TEST_EXPECT_TRUE(changedInputResults.size() == 4);
        UNIT_TEST_EXPECT_TRUE(assetMessages.size() == 7);

        // what we expect to happen here is that it tells us that 3 files were removed, and 4 files were changed.
        // The files removed should be the ones we did not emit this time
        // note that order isn't guarinteed but an example output it this

        // [0] Removed: ES3, basefile.arc1
        // [1] Removed: ES3, basefile.arc2
        // [2] Changed: ES3, basefilea.arc1 (added)

        // [3] Removed: ES3, basefile.azm
        // [4] Changed: ES3, basefilea.azm (added)

        // [5] changed: PC, basefile.arc1 (changed)
        // [6] changed: PC, basefile.azm (changed)

        for (auto element : assetMessages)
        {
            if (element.second.m_data == "basefile.arc1")
            {
                if (element.first == "pc")
                {
                    UNIT_TEST_EXPECT_TRUE(element.second.m_type == AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged);
                }
                else
                {
                    UNIT_TEST_EXPECT_TRUE(element.second.m_type == AzFramework::AssetSystem::AssetNotificationMessage::AssetRemoved);
                }
            }

            if (element.second.m_data == "basefilea.arc1")
            {
                UNIT_TEST_EXPECT_TRUE(element.second.m_type == AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged);
                UNIT_TEST_EXPECT_TRUE(element.first == "es3");
            }

            if (element.second.m_data == "basefile.arc2")
            {
                UNIT_TEST_EXPECT_TRUE(element.second.m_type == AzFramework::AssetSystem::AssetNotificationMessage::AssetRemoved);
                UNIT_TEST_EXPECT_TRUE(element.first == "es3");
            }
        }

        // original products must no longer exist since it should have found and deleted them!
        for (QString outFile: oldes3outs)
        {
            UNIT_TEST_EXPECT_FALSE(QFile::exists(outFile));
        }

        // the old pc products should still exist because they were emitted this time around.
        for (QString outFile: oldpcouts)
        {
            UNIT_TEST_EXPECT_TRUE(QFile::exists(outFile));
        }

        changedInputResults.clear();
        assetMessages.clear();
        processResults.clear();

        // add a fingerprint file thats next to the original file
        // feed it the exportsettings file again.
        // it should spawn BOTH compilers again.
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(inputFilePath + ".exportsettings", "new!"));
        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, inputFilePath + ".exportsettings"));
        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));
        sortAssetToProcessResultList(processResults);

        // --------- same result as above ----------
        UNIT_TEST_EXPECT_TRUE(processResults.size() == 4); // pc and es3
        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_platformInfo.m_identifier == processResults[1].m_jobEntry.m_platformInfo.m_identifier);
        UNIT_TEST_EXPECT_TRUE(processResults[2].m_jobEntry.m_platformInfo.m_identifier == processResults[3].m_jobEntry.m_platformInfo.m_identifier);
        UNIT_TEST_EXPECT_TRUE((processResults[0].m_jobEntry.m_platformInfo.m_identifier == "es3"));
        UNIT_TEST_EXPECT_TRUE((processResults[1].m_jobEntry.m_platformInfo.m_identifier == "es3"));
        UNIT_TEST_EXPECT_TRUE((processResults[2].m_jobEntry.m_platformInfo.m_identifier == "pc"));
        UNIT_TEST_EXPECT_TRUE((processResults[3].m_jobEntry.m_platformInfo.m_identifier == "pc"));
        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_computedFingerprint != 0);

        // send all the done messages simultaneously:
        for (int checkIdx = 0; checkIdx < 4; ++checkIdx)
        {
            QString processFile1 = processResults[checkIdx].m_jobEntry.m_absolutePathToFile;
            UNIT_TEST_EXPECT_TRUE(processFile1 == inputFilePath);
            QString platformFolder = cacheRoot.filePath(QString::fromUtf8(processResults[checkIdx].m_jobEntry.m_platformInfo.m_identifier.c_str()) + "/" + gameName.toLower());
            platformFolder = AssetUtilities::NormalizeDirectoryPath(platformFolder);
            processFile1 = processResults[checkIdx].m_destinationPath;
            UNIT_TEST_EXPECT_TRUE(processFile1.startsWith(platformFolder));
            UNIT_TEST_EXPECT_TRUE(processResults[checkIdx].m_jobEntry.m_computedFingerprint != 0);
        }


        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(es3outs[0].toUtf8().constData()));
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[0].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(es3outs2[0].toUtf8().constData()));
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[1].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcouts[0].toUtf8().constData()));
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[2].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcouts2[0].toUtf8().constData()));
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[3].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        // let events bubble through:
        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        // --- delete the input asset and make sure it cleans up all products.

        changedInputResults.clear();
        assetMessages.clear();
        processResults.clear();


        // first, delete the fingerprint file, this should result in normal reprocess:
        QFile::remove(inputFilePath + ".exportsettings");
        QMetaObject::invokeMethod(&apm, "AssessDeletedFile", Qt::QueuedConnection, Q_ARG(QString, inputFilePath + ".exportsettings"));
        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        sortAssetToProcessResultList(processResults);

        // --------- same result as above ----------
        UNIT_TEST_EXPECT_TRUE(processResults.size() == 4); // 2 each for pc and es3,since we have two recognizer for .txt file
        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_platformInfo.m_identifier == processResults[1].m_jobEntry.m_platformInfo.m_identifier);
        UNIT_TEST_EXPECT_TRUE(processResults[2].m_jobEntry.m_platformInfo.m_identifier == processResults[3].m_jobEntry.m_platformInfo.m_identifier);
        UNIT_TEST_EXPECT_TRUE((processResults[0].m_jobEntry.m_platformInfo.m_identifier == "es3"));
        UNIT_TEST_EXPECT_TRUE((processResults[1].m_jobEntry.m_platformInfo.m_identifier == "es3"));
        UNIT_TEST_EXPECT_TRUE((processResults[2].m_jobEntry.m_platformInfo.m_identifier == "pc"));
        UNIT_TEST_EXPECT_TRUE((processResults[3].m_jobEntry.m_platformInfo.m_identifier == "pc"));
        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_computedFingerprint != 0);
        UNIT_TEST_EXPECT_TRUE(processResults[1].m_jobEntry.m_computedFingerprint != 0);


        // send all the done messages simultaneously:

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(es3outs[0].toUtf8().constData()));
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[0].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(es3outs2[0].toUtf8().constData()));
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[1].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcouts[0].toUtf8().constData()));
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[2].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcouts2[0].toUtf8().constData()));
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[3].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        // let events bubble through:
        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        // deleting the fingerprint file should not have erased the products
        UNIT_TEST_EXPECT_TRUE(QFile::exists(pcouts[0]));
        UNIT_TEST_EXPECT_TRUE(QFile::exists(es3outs[0]));
        UNIT_TEST_EXPECT_TRUE(QFile::exists(pcouts2[0]));
        UNIT_TEST_EXPECT_TRUE(QFile::exists(es3outs2[0]));

        changedInputResults.clear();
        assetMessages.clear();
        processResults.clear();

        connection.m_sent = false;
        payloadList.clear();

        // delete the original input.
        QFile::remove(inputFilePath);

        SourceFileNotificationMessage sourceFileRemovedMessage;
        QMetaObject::invokeMethod(&apm, "AssessDeletedFile", Qt::QueuedConnection, Q_ARG(QString, inputFilePath));
        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));
        UNIT_TEST_EXPECT_TRUE(payloadList.size() == 1);

        messageLoadCount = 0;
        for (auto payload : payloadList)
        {
            if (payload.first == SourceFileNotificationMessage::MessageType())
            {
                UNIT_TEST_EXPECT_TRUE(AZ::Utils::LoadObjectFromBufferInPlace(payload.second.data(), payload.second.size(), sourceFileRemovedMessage));
                UNIT_TEST_EXPECT_TRUE(sourceFileRemovedMessage.m_type == SourceFileNotificationMessage::FileRemoved);
                ++messageLoadCount;
            }
        }

        UNIT_TEST_EXPECT_TRUE(connection.m_sent);
        UNIT_TEST_EXPECT_TRUE(messageLoadCount == payloadList.size()); // make sure all messages are accounted for
        scanFolder = QDir(sourceFileRemovedMessage.m_scanFolder.c_str());
        pathToCheck = scanFolder.filePath(sourceFileRemovedMessage.m_relativeSourcePath.c_str());
        UNIT_TEST_EXPECT_TRUE(QString::compare(inputFilePath, pathToCheck, Qt::CaseInsensitive) == 0);

        // nothing to process, but products should be gone!
        UNIT_TEST_EXPECT_TRUE(processResults.isEmpty());
        UNIT_TEST_EXPECT_TRUE(changedInputResults.isEmpty());

        // should have gotten four "removed" messages for its products:
        UNIT_TEST_EXPECT_TRUE(assetMessages.size() == 4);

        for (auto element : assetMessages)
        {
            UNIT_TEST_EXPECT_TRUE(element.second.m_type == AzFramework::AssetSystem::AssetNotificationMessage::AssetRemoved);
        }

        UNIT_TEST_EXPECT_FALSE(QFile::exists(pcouts[0]));
        UNIT_TEST_EXPECT_FALSE(QFile::exists(es3outs[0]));
        UNIT_TEST_EXPECT_FALSE(QFile::exists(pcouts2[0]));
        UNIT_TEST_EXPECT_FALSE(QFile::exists(es3outs2[0]));

        changedInputResults.clear();
        assetMessages.clear();
        processResults.clear();

        // test: if an asset fails, it should recompile it next time, and not report success

        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(inputFilePath, "new2"));
        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, inputFilePath));
        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        sortAssetToProcessResultList(processResults);

        // --------- same result as above ----------
        UNIT_TEST_EXPECT_TRUE(processResults.size() == 4); // 2 each for pc and es3,since we have two recognizer for .txt file
        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_platformInfo.m_identifier == processResults[1].m_jobEntry.m_platformInfo.m_identifier);
        UNIT_TEST_EXPECT_TRUE(processResults[2].m_jobEntry.m_platformInfo.m_identifier == processResults[3].m_jobEntry.m_platformInfo.m_identifier);
        UNIT_TEST_EXPECT_TRUE((processResults[0].m_jobEntry.m_platformInfo.m_identifier == "es3"));
        UNIT_TEST_EXPECT_TRUE((processResults[1].m_jobEntry.m_platformInfo.m_identifier == "es3"));
        UNIT_TEST_EXPECT_TRUE((processResults[2].m_jobEntry.m_platformInfo.m_identifier == "pc"));
        UNIT_TEST_EXPECT_TRUE((processResults[3].m_jobEntry.m_platformInfo.m_identifier == "pc"));
        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_computedFingerprint != 0);

        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(es3outs[0], "newfile."));
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(es3outs2[0], "newfile."));
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(pcouts2[0], "newfile."));

        // send both done messages simultaneously!
        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(es3outs[0].toUtf8().constData()));
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[0].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(es3outs2[0].toUtf8().constData()));
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[1].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        // send one failure only for PC :
        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcouts[0].toUtf8().constData()));
        QMetaObject::invokeMethod(&apm, "AssetFailed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[2].m_jobEntry));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcouts2[0].toUtf8().constData()));
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[3].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        // let events bubble through:
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        // ----------------------- test job info requests, some assets have failed (specifically, the [2] index job entry
        {
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            AssetJobsInfoRequest requestInfo;

            requestInfo.m_searchTerm = inputFilePath.toUtf8().constData();

            {
                // send our request:
                payloadList.clear();
                connection.m_sent = false;
                apm.ProcessGetAssetJobsInfoRequest(requestId, &requestInfo);
            }

            // wait for it to process:
            QCoreApplication::processEvents(QEventLoop::AllEvents);

            UNIT_TEST_EXPECT_TRUE(connection.m_sent); // we expect a response to have arrived.

            unsigned int jobinformationResultIndex = -1;
            for (int index = 0; index < payloadList.size(); index++)
            {
                unsigned int type = payloadList.at(index).first;
                if (type == requestInfo.GetMessageType())
                {
                    jobinformationResultIndex = index;
                }
            }
            UNIT_TEST_EXPECT_FALSE(jobinformationResultIndex == -1);

            AssetJobsInfoResponse jobResponse;
            UNIT_TEST_EXPECT_TRUE(AZ::Utils::LoadObjectFromBufferInPlace(payloadList.at(jobinformationResultIndex).second.data(), payloadList.at(jobinformationResultIndex).second.size(), jobResponse));

            UNIT_TEST_EXPECT_TRUE(jobResponse.m_isSuccess);
            UNIT_TEST_EXPECT_TRUE(jobResponse.m_jobList.size() == processResults.size());

            // make sure each job corresponds to one in the process results list (but note that the order is not important).
            for (int oldJobIdx = azlossy_cast<int>(jobResponse.m_jobList.size()) - 1; oldJobIdx >= 0; --oldJobIdx)
            {
                bool foundIt = false;
                const JobInfo& jobInfo = jobResponse.m_jobList[oldJobIdx];

                // validate EVERY field
                UNIT_TEST_EXPECT_FALSE(jobInfo.m_sourceFile.empty());
                UNIT_TEST_EXPECT_FALSE(jobInfo.m_platform.empty());
                UNIT_TEST_EXPECT_FALSE(jobInfo.m_jobKey.empty());
                UNIT_TEST_EXPECT_FALSE(jobInfo.m_builderGuid.IsNull());

                for (int detailsIdx = 0; detailsIdx < processResults.size(); ++detailsIdx)
                {
                    const JobDetails& details = processResults[detailsIdx];

                    if ((QString::compare(jobInfo.m_sourceFile.c_str(), details.m_jobEntry.m_relativePathToFile, Qt::CaseInsensitive) == 0) &&
                        (QString::compare(jobInfo.m_platform.c_str(), details.m_jobEntry.m_platformInfo.m_identifier.c_str(), Qt::CaseInsensitive) == 0) &&
                        (QString::compare(jobInfo.m_jobKey.c_str(), details.m_jobEntry.m_jobKey, Qt::CaseInsensitive) == 0) &&
                        (jobInfo.m_builderGuid == details.m_jobEntry.m_builderGuid) &&
                        (jobInfo.GetHash() == details.m_jobEntry.GetHash()))
                    {
                        foundIt = true;

                        if (detailsIdx == 2) // we only said that the index [2] job was dead
                        {
                            UNIT_TEST_EXPECT_TRUE(jobInfo.m_status == JobStatus::Failed);
                        }
                        else
                        {
                            UNIT_TEST_EXPECT_TRUE(jobInfo.m_status == JobStatus::Completed);
                        }

                        break;
                    }
                }
                UNIT_TEST_EXPECT_TRUE(foundIt);
            }
        }

        // we should have get three success:
        UNIT_TEST_EXPECT_TRUE(changedInputResults.size() == 3);
        UNIT_TEST_EXPECT_TRUE(assetMessages.size() == 3);
        UNIT_TEST_EXPECT_TRUE(payloadList.size() == 1);

        // which should be for the ES3:
        UNIT_TEST_EXPECT_TRUE(AssetUtilities::NormalizeFilePath(changedInputResults[0].first) == inputFilePath);

        // always RELATIVE, always with the product name.
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].second.m_data == "basefilea.arc1" || assetMessages[0].second.m_data == "basefilea.azm");
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].first == "es3");

        for (auto& payload : payloadList)
        {
            if (payload.first == SourceFileNotificationMessage::MessageType())
            {
                UNIT_TEST_EXPECT_TRUE(AZ::Utils::LoadObjectFromBufferInPlace(payload.second.data(), payload.second.size(), sourceFileRemovedMessage));
                UNIT_TEST_EXPECT_TRUE(sourceFileRemovedMessage.m_type == SourceFileNotificationMessage::FileRemoved);
            }
        }

        scanFolder = QDir(sourceFileRemovedMessage.m_scanFolder.c_str());
        pathToCheck = scanFolder.filePath(sourceFileRemovedMessage.m_relativeSourcePath.c_str());
        UNIT_TEST_EXPECT_TRUE(QString::compare(inputFilePath, pathToCheck, Qt::CaseInsensitive) == 0);

        // now if we notify again, only the pc should process:
        changedInputResults.clear();
        assetMessages.clear();
        processResults.clear();
        payloadList.clear();

        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, inputFilePath));
        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        // --------- same result as above ----------
        UNIT_TEST_EXPECT_TRUE(processResults.size() == 1); // pc only
        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_platformInfo.m_identifier == "pc");

        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(pcouts[0], "new1"));

        // send one failure only for PC :

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcouts[0].toUtf8().constData()));
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[0].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        // let events bubble through:
        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        // we should have got only one success:
        UNIT_TEST_EXPECT_TRUE(changedInputResults.size() == 1);
        UNIT_TEST_EXPECT_TRUE(assetMessages.size() == 1);

        // always RELATIVE, always with the product name.
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].second.m_data == "basefile.arc1");
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].first == "pc");

        changedInputResults.clear();
        assetMessages.clear();
        processResults.clear();


        //----------This file is used for testing ProcessGetFullAssetPath function //
        inputFilePath = AssetUtilities::NormalizeFilePath(tempPath.absoluteFilePath("subfolder3/somerandomfile.random"));
        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, inputFilePath));
        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        sortAssetToProcessResultList(processResults);

        // --------- same result as above ----------
        UNIT_TEST_EXPECT_TRUE(processResults.size() == 1); // 1 for pc
        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_platformInfo.m_identifier == "pc");

        pcouts.clear();

        pcouts.push_back(cacheRoot.filePath(QString("pc/") + gameName + "/subfolder3/randomfileoutput.random"));
        pcouts.push_back(cacheRoot.filePath(QString("pc/") + gameName + "/subfolder3/randomfileoutput.random1"));
        pcouts.push_back(cacheRoot.filePath(QString("pc/") + gameName + "/subfolder3/randomfileoutput.random2"));
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(pcouts[0], "products."));
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(pcouts[1], "products."));
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(pcouts[2], "products."));

        //Invoke Asset Processed for pc platform , txt files job description
        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcouts[0].toUtf8().constData()));
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcouts[1].toUtf8().constData()));
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcouts[2].toUtf8().constData()));
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[0].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        // let events bubble through:
        QCoreApplication::processEvents(QEventLoop::AllEvents | QEventLoop::WaitForMoreEvents, 1000);

        UNIT_TEST_EXPECT_TRUE(assetMessages.size() == 3);
        UNIT_TEST_EXPECT_TRUE(changedInputResults.size() == 1);

        changedInputResults.clear();
        assetMessages.clear();
        processResults.clear();


        // -------------- override test -----------------
        // set up by letting it compile basefile.txt from 3:
        inputFilePath = AssetUtilities::NormalizeFilePath(tempPath.absoluteFilePath("subfolder3/BaseFile.txt"));
        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, inputFilePath));
        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        sortAssetToProcessResultList(processResults);

        // --------- same result as above ----------
        UNIT_TEST_EXPECT_TRUE(processResults.size() == 4); // 2 each for pc and es3,since we have two recognizer for .txt file
        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_platformInfo.m_identifier == processResults[1].m_jobEntry.m_platformInfo.m_identifier);
        UNIT_TEST_EXPECT_TRUE(processResults[2].m_jobEntry.m_platformInfo.m_identifier == processResults[3].m_jobEntry.m_platformInfo.m_identifier);
        UNIT_TEST_EXPECT_TRUE((processResults[0].m_jobEntry.m_platformInfo.m_identifier == "es3"));
        UNIT_TEST_EXPECT_TRUE((processResults[1].m_jobEntry.m_platformInfo.m_identifier == "es3"));
        UNIT_TEST_EXPECT_TRUE((processResults[2].m_jobEntry.m_platformInfo.m_identifier == "pc"));
        UNIT_TEST_EXPECT_TRUE((processResults[3].m_jobEntry.m_platformInfo.m_identifier == "pc"));
        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_computedFingerprint != 0);

        es3outs.clear();
        es3outs2.clear();
        pcouts.clear();
        pcouts2.clear();
        es3outs.push_back(cacheRoot.filePath(QString("es3/") + gameName + "/basefilez.arc2"));
        es3outs2.push_back(cacheRoot.filePath(QString("es3/") + gameName + "/basefileaz.azm2"));
        // note that the ES3 outs have changed
        // but the pc outs are still the same.
        pcouts.push_back(cacheRoot.filePath(QString("pc/") + gameName + "/basefile.arc2"));
        pcouts2.push_back(cacheRoot.filePath(QString("pc/") + gameName + "/basefile.azm2"));
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(es3outs[0], "newfile."));
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(pcouts[0], "newfile."));
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(es3outs2[0], "newfile."));
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(pcouts2[0], "newfile."));
        changedInputResults.clear();
        assetMessages.clear();

        // send all the done messages simultaneously:
        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(es3outs[0].toUtf8().constData()));
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[0].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(es3outs2[0].toUtf8().constData()));
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[1].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcouts[0].toUtf8().constData()));
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[2].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcouts2[0].toUtf8().constData()));
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[3].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        // let events bubble through:
        QCoreApplication::processEvents(QEventLoop::AllEvents | QEventLoop::WaitForMoreEvents, 1000);

        // we should have got only one success:
        UNIT_TEST_EXPECT_TRUE(changedInputResults.size() == 4);
        UNIT_TEST_EXPECT_TRUE(assetMessages.size() == 4);
        for (auto element : assetMessages)
        {
            // because the source asset had UPPER CASE in it, we should have multiple legacy IDs
            UNIT_TEST_EXPECT_TRUE(element.second.m_legacyAssetIds.size() == 2);
        }

        // ------------- setup complete, now do the test...
        // now feed it a file that has been overridden by a more important later file
        inputFilePath = AssetUtilities::NormalizeFilePath(tempPath.absoluteFilePath("subfolder1/basefile.txt"));

        changedInputResults.clear();
        assetMessages.clear();
        processResults.clear();

        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, inputFilePath));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));
        UNIT_TEST_EXPECT_TRUE(processResults.isEmpty());
        UNIT_TEST_EXPECT_TRUE(changedInputResults.isEmpty());
        UNIT_TEST_EXPECT_TRUE(assetMessages.isEmpty());

        // since it was overridden, nothing should occur.
        //AZ_TracePrintf("Asset Processor", "Preparing the assessDeletedFiles invocation...\n");

        // delete the highest priority override file and ensure that it generates tasks
        // for the next highest priority!  Basically, deleting this file should "reveal" the file underneath it in the other subfolder
        QString deletedFile = tempPath.absoluteFilePath("subfolder3/BaseFile.txt");
        QString expectedReplacementInputFile = AssetUtilities::NormalizeFilePath(tempPath.absoluteFilePath("subfolder2/basefile.txt"));

        UNIT_TEST_EXPECT_TRUE(QFile::remove(deletedFile));
        // sometimes the above deletion actually takes a moment to trickle, for some reason, and it doesn't actually get that the file was erased.
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        UNIT_TEST_EXPECT_FALSE(QFile::exists(deletedFile));

        QMetaObject::invokeMethod(&apm, "AssessDeletedFile", Qt::QueuedConnection, Q_ARG(QString,  deletedFile));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        sortAssetToProcessResultList(processResults);

        // --------- same result as above ----------
        UNIT_TEST_EXPECT_TRUE(processResults.size() == 4); // 2 each for pc and es3,since we have two recognizer for .txt file
        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_platformInfo.m_identifier == processResults[1].m_jobEntry.m_platformInfo.m_identifier);
        UNIT_TEST_EXPECT_TRUE(processResults[2].m_jobEntry.m_platformInfo.m_identifier == processResults[3].m_jobEntry.m_platformInfo.m_identifier);
        UNIT_TEST_EXPECT_TRUE((processResults[0].m_jobEntry.m_platformInfo.m_identifier == "es3"));
        UNIT_TEST_EXPECT_TRUE((processResults[1].m_jobEntry.m_platformInfo.m_identifier == "es3"));
        UNIT_TEST_EXPECT_TRUE((processResults[2].m_jobEntry.m_platformInfo.m_identifier == "pc"));
        UNIT_TEST_EXPECT_TRUE((processResults[3].m_jobEntry.m_platformInfo.m_identifier == "pc"));
        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_computedFingerprint != 0);

        for (int checkIdx = 0; checkIdx < 4; ++checkIdx)
        {
            QString processFile1 = processResults[checkIdx].m_jobEntry.m_absolutePathToFile;
            UNIT_TEST_EXPECT_TRUE(processFile1 == expectedReplacementInputFile);
            QString platformFolder = cacheRoot.filePath(QString::fromUtf8(processResults[checkIdx].m_jobEntry.m_platformInfo.m_identifier.c_str()) + "/" + gameName.toLower());
            platformFolder = AssetUtilities::NormalizeDirectoryPath(platformFolder);
            processFile1 = processResults[checkIdx].m_destinationPath;
            UNIT_TEST_EXPECT_TRUE(processFile1.startsWith(platformFolder));
            UNIT_TEST_EXPECT_TRUE(processResults[checkIdx].m_jobEntry.m_computedFingerprint != 0);
        }

        if (!collectedChanges.isEmpty())
        {
            for (const QString& invalid : collectedChanges)
            {
                AZ_TracePrintf(AssetProcessor::DebugChannel, "Invalid change made: %s.\n", invalid.toUtf8().constData());
            }
            Q_EMIT UnitTestFailed("Changes were made to the real file system, this is not allowed during this test.");
            return;
        }

        AssetStatus status = AssetStatus_Unknown;

        QString filePath = tempPath.absoluteFilePath("subfolder3/somefile.xxx");
        inputFilePath = AssetUtilities::NormalizeFilePath(filePath);

        unsigned int fingerprintForPC = 0;
        unsigned int fingerprintForES3 = 0;

        ComputeFingerprints(fingerprintForPC, fingerprintForES3, config, filePath, inputFilePath);

        processResults.clear();
        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, inputFilePath));
        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        sortAssetToProcessResultList(processResults);

        UNIT_TEST_EXPECT_TRUE(processResults.size() == 4); // // 2 each for pc and es3,since we have two recognizer for .xxx file
        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_platformInfo.m_identifier == processResults[1].m_jobEntry.m_platformInfo.m_identifier);
        UNIT_TEST_EXPECT_TRUE(processResults[2].m_jobEntry.m_platformInfo.m_identifier == processResults[3].m_jobEntry.m_platformInfo.m_identifier);
        UNIT_TEST_EXPECT_TRUE((processResults[0].m_jobEntry.m_platformInfo.m_identifier == "es3"));
        UNIT_TEST_EXPECT_TRUE((processResults[1].m_jobEntry.m_platformInfo.m_identifier == "es3"));
        UNIT_TEST_EXPECT_TRUE((processResults[2].m_jobEntry.m_platformInfo.m_identifier == "pc"));
        UNIT_TEST_EXPECT_TRUE((processResults[3].m_jobEntry.m_platformInfo.m_identifier == "pc"));

        config.RemoveRecognizer("xxx files 2 (builder2)");
        UNIT_TEST_EXPECT_TRUE(mockAppManager.UnRegisterAssetRecognizerAsBuilder("xxx files 2 (builder2)"));

        //Changing specs for pc
        specpc.m_extraRCParams = "new pcparams";
        rec.m_platformSpecs.insert("pc", specpc);

        config.AddRecognizer(rec);
        mockAppManager.RegisterAssetRecognizerAsBuilder(rec);

        processResults.clear();
        inputFilePath = AssetUtilities::NormalizeFilePath(filePath);
        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, inputFilePath));
        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        // we never actually submitted any fingerprints or indicated success, so the same number of jobs should occur as before
        sortAssetToProcessResultList(processResults);

        UNIT_TEST_EXPECT_TRUE(processResults.size() == 4); // // 2 each for pc and es3,since we have two recognizer for .xxx file
        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_platformInfo.m_identifier == processResults[1].m_jobEntry.m_platformInfo.m_identifier);
        UNIT_TEST_EXPECT_TRUE(processResults[2].m_jobEntry.m_platformInfo.m_identifier == processResults[3].m_jobEntry.m_platformInfo.m_identifier);
        UNIT_TEST_EXPECT_TRUE((processResults[0].m_jobEntry.m_platformInfo.m_identifier == "es3"));
        UNIT_TEST_EXPECT_TRUE((processResults[1].m_jobEntry.m_platformInfo.m_identifier == "es3"));
        UNIT_TEST_EXPECT_TRUE((processResults[2].m_jobEntry.m_platformInfo.m_identifier == "pc"));
        UNIT_TEST_EXPECT_TRUE((processResults[3].m_jobEntry.m_platformInfo.m_identifier == "pc"));

        // tell it that all those assets are now successfully done:
        for (auto processResult : processResults)
        {
            QString outputFile = normalizedCacheRootDir.absoluteFilePath(processResult.m_destinationPath + "/doesn'tmatter.dds" + processResult.m_jobEntry.m_jobKey);
            CreateDummyFile(outputFile);
            AssetBuilderSDK::ProcessJobResponse response;
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
            response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(outputFile.toUtf8().constData()));
            apm.AssetProcessed(processResult.m_jobEntry, response);
        }

        // now re-perform the same test, this time only the pc ones should re-appear.
        // this should happen because we're changing the extra params, which should be part of the fingerprint
        // if this unit test fails, check to make sure that the extra params are being ingested into the fingerprint computation functions
        // and also make sure that the jobs that are for the remaining es3 platform don't change.

        // store the UUID so that we can insert the new one with the same UUID
        AZStd::shared_ptr<InternalMockBuilder> builderTxt2Builder;
        UNIT_TEST_EXPECT_TRUE(mockAppManager.GetBuilderByID("xxx files 2 (builder2)", builderTxt2Builder));

        AZ::Uuid builderUuid;
        UNIT_TEST_EXPECT_TRUE(mockAppManager.GetBuildUUIDFromName("xxx files 2 (builder2)", builderUuid));

        builderTxt2Builder.reset();

        config.RemoveRecognizer("xxx files 2 (builder2)");
        mockAppManager.UnRegisterAssetRecognizerAsBuilder("xxx files 2 (builder2)");
        //Changing specs for pc
        specpc.m_extraRCParams = "new pcparams---"; // make sure the xtra params are different.
        rec.m_platformSpecs.remove("pc");
        rec.m_platformSpecs.insert("pc", specpc);

        config.AddRecognizer(rec);
        mockAppManager.RegisterAssetRecognizerAsBuilder(rec);

        processResults.clear();

        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, inputFilePath));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        sortAssetToProcessResultList(processResults);

        UNIT_TEST_EXPECT_TRUE(processResults.size() == 1); // only 1 for pc
        UNIT_TEST_EXPECT_TRUE((processResults[0].m_jobEntry.m_platformInfo.m_identifier == "pc"));

        // ---------------------

        unsigned int newfingerprintForPC = 0;
        unsigned int newfingerprintForES3 = 0;

        ComputeFingerprints(newfingerprintForPC, newfingerprintForES3, config, filePath, inputFilePath);

        UNIT_TEST_EXPECT_TRUE(newfingerprintForPC != fingerprintForPC);//Fingerprints should be different
        UNIT_TEST_EXPECT_TRUE(newfingerprintForES3 == fingerprintForES3);//Fingerprints are same

        config.RemoveRecognizer("xxx files 2 (builder2)");
        mockAppManager.UnRegisterAssetRecognizerAsBuilder("xxx files 2 (builder2)");

        //Changing version
        rec.m_version = "1.0";
        config.AddRecognizer(rec);
        mockAppManager.RegisterAssetRecognizerAsBuilder(rec);

        processResults.clear();

        inputFilePath = AssetUtilities::NormalizeFilePath(filePath);
        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, inputFilePath));
        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));
        UNIT_TEST_EXPECT_TRUE(processResults.size() == 2); // pc and es3
        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_platformInfo.m_identifier != processResults[1].m_jobEntry.m_platformInfo.m_identifier);
        UNIT_TEST_EXPECT_TRUE((processResults[0].m_jobEntry.m_platformInfo.m_identifier == "pc") || (processResults[0].m_jobEntry.m_platformInfo.m_identifier == "es3"));
        UNIT_TEST_EXPECT_TRUE((processResults[1].m_jobEntry.m_platformInfo.m_identifier == "pc") || (processResults[1].m_jobEntry.m_platformInfo.m_identifier == "es3"));

        unsigned int newfingerprintForPCAfterVersionChange = 0;
        unsigned int newfingerprintForES3AfterVersionChange = 0;

        ComputeFingerprints(newfingerprintForPCAfterVersionChange, newfingerprintForES3AfterVersionChange, config, filePath, inputFilePath);

        UNIT_TEST_EXPECT_TRUE((newfingerprintForPCAfterVersionChange != fingerprintForPC) || (newfingerprintForPCAfterVersionChange != newfingerprintForPC));//Fingerprints should be different
        UNIT_TEST_EXPECT_TRUE((newfingerprintForES3AfterVersionChange != fingerprintForES3) || (newfingerprintForES3AfterVersionChange != newfingerprintForES3));//Fingerprints should be different

        //------Test for Files which are excluded
        processResults.clear();
        inputFilePath = AssetUtilities::NormalizeFilePath(tempPath.absoluteFilePath("subfolder3/savebackup/test.txt"));
        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, inputFilePath));
        UNIT_TEST_EXPECT_FALSE(BlockUntil(idling, 3000)); //Processing a file that will be excluded should not cause assetprocessor manager to emit the onBecameIdle signal because its state should not change
        UNIT_TEST_EXPECT_TRUE(processResults.size() == 0);

        // ------------- Test querying asset status -------------------
        {
            filePath = tempPath.absoluteFilePath("subfolder2/folder/ship.tiff");
            inputFilePath = AssetUtilities::NormalizeFilePath(filePath);
            
            QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, inputFilePath));
            UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

            for (const JobDetails& processResult : processResults)
            {
                QString outputFile = normalizedCacheRootDir.absoluteFilePath(processResult.m_destinationPath + "/ship_nrm.dds");
                
                CreateDummyFile(outputFile);
                
                AssetBuilderSDK::ProcessJobResponse response;
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
                response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(outputFile.toUtf8().constData()));
                
                apm.AssetProcessed(processResult.m_jobEntry, response);
            }

            // let events bubble through:
            QCoreApplication::processEvents(QEventLoop::AllEvents | QEventLoop::WaitForMoreEvents, 1000);

            bool foundIt = false;

            connect(&apm, &AssetProcessorManager::SendAssetExistsResponse,
                this, [&foundIt](NetworkRequestID requestId, bool result)
            {
                foundIt = result;
            });

            auto runTestCase = [this, &foundIt, &apm, &requestId](const char* testCase, bool expectedValue, bool& result)
            {
                result = foundIt = false;

                apm.OnRequestAssetExists(requestId, "pc", testCase);
                UNIT_TEST_EXPECT_FALSE(foundIt == expectedValue); // Macro will call return on failure

                result = true;
            };

            const char* successCases[] = 
            {
                "ship.tiff", // source
                "ship", // source no extension
                "ship_nrm.dds", // product
                "ship_nrm", // product no extension
            };

            // Test source without path, should all fail
            for (const auto& testCase : successCases)
            {
                foundIt = false;

                apm.OnRequestAssetExists(requestId, "pc", testCase);
                UNIT_TEST_EXPECT_FALSE(foundIt);
            }

            // Test source with the path included
            for (const auto& testCase : successCases)
            {
                foundIt = false;
                AZStd::string withPath = AZStd::string("folder/") + testCase;
                
                apm.OnRequestAssetExists(requestId, "pc", withPath.c_str());
                UNIT_TEST_EXPECT_TRUE(foundIt);
            }

            const char* failCases[] =
            {
                "folder/ships.tiff",
                "otherfolder/ship.tiff",
                "otherfolder/ship_nrm.dds",
                "folder/ship_random.other/random",
                "folder/ship.dds", // source wrong extension
                "folder/ship_nrm.tiff", // product wrong extension
                "folder/ship_color.dds", // product that doesn't exist
            };

            for (const auto& testCase : failCases)
            {
                foundIt = false;

                apm.OnRequestAssetExists(requestId, "pc", testCase);
                UNIT_TEST_EXPECT_FALSE(foundIt);
            }
        }

        // -----------------------------------------------------------------------------------------------
        // -------------------------------------- FOLDER RENAMING TEST -----------------------------------
        // -----------------------------------------------------------------------------------------------
        // Test: Rename a source folder

        // test renaming an entire folder!

        QString fileToMove1 = tempPath.absoluteFilePath("subfolder1/rename_this/somefile1.txt");
        QString fileToMove2 = tempPath.absoluteFilePath("subfolder1/rename_this/somefolder/somefile2.txt");

        config.RemoveRecognizer(builderTxt2Name); // don't need this anymore.
        mockAppManager.UnRegisterAssetRecognizerAsBuilder(builderTxt2Name);

        processResults.clear();
        // put the two files on the map:
        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, fileToMove1));
        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, fileToMove2));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));
        sortAssetToProcessResultList(processResults);

        UNIT_TEST_EXPECT_TRUE(processResults.size() == 4); // 2 fils on 2 platforms

        for (int index = 0; index < processResults.size(); ++index)
        {
            QFileInfo fi(processResults[index].m_jobEntry.m_absolutePathToFile);
            QString pcout = QDir(processResults[index].m_destinationPath).absoluteFilePath(fi.fileName());
            UNIT_TEST_EXPECT_TRUE(CreateDummyFile(pcout, "products."));

            response.m_outputProducts.clear();
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
            response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcout.toUtf8().constData()));
            QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[index].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));
        }


        // let events bubble through:
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

        // setup complete.  now RENAME that folder.

        QDir renamer;
        UNIT_TEST_EXPECT_TRUE(renamer.rename(tempPath.absoluteFilePath("subfolder1/rename_this"), tempPath.absoluteFilePath("subfolder1/done_renaming")));

        // renames appear as a delete then add of that folder:
        processResults.clear();
        assetMessages.clear();
        QMetaObject::invokeMethod(&apm, "AssessDeletedFile", Qt::QueuedConnection, Q_ARG(QString, tempPath.absoluteFilePath("subfolder1/rename_this")));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        sortAssetToProcessResultList(processResults);

        UNIT_TEST_EXPECT_TRUE(processResults.size() == 0); // nothing to process

        // we are aware that 4 products went missing (es3 and pc versions of the 2 files since we renamed the SOURCE folder)
        UNIT_TEST_EXPECT_TRUE(assetMessages.size() == 4);
        for (auto element : assetMessages)
        {
            UNIT_TEST_EXPECT_TRUE(element.second.m_type == AzFramework::AssetSystem::AssetNotificationMessage::AssetRemoved);
        }

        processResults.clear();
        QMetaObject::invokeMethod(&apm, "AssessAddedFile", Qt::QueuedConnection, Q_ARG(QString, tempPath.absoluteFilePath("subfolder1/done_renaming")));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        sortAssetToProcessResultList(processResults);

        UNIT_TEST_EXPECT_TRUE(processResults.size() == 4); // 2 files on 2 platforms

        // -----------------------------------------------------------------------------------------------
        // -------------------------------------- FOLDER RENAMING TEST -----------------------------------
        // -----------------------------------------------------------------------------------------------
        // Test: Rename a cache folder

        QStringList outputsCreated;

        for (int index = 0; index < processResults.size(); ++index)
        {
            QFileInfo fi(processResults[index].m_jobEntry.m_absolutePathToFile);
            QString pcout = QDir(processResults[index].m_destinationPath).absoluteFilePath(fi.fileName());
            UNIT_TEST_EXPECT_TRUE(CreateDummyFile(pcout, "products."));

            response.m_outputProducts.clear();
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
            response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcout.toUtf8().constData()));
            QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[index].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));
        }

        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

        // it now believes that there are a whole bunch of assets in subfolder1/done_renaming and they resulted in
        // a whole bunch of files to have been created in the asset cache, listed in processresults, and they exist in outputscreated...
        // rename the output folder:

        QString originalCacheFolderName = normalizedCacheRootDir.absoluteFilePath("pc") + "/" + gameName + "/done_renaming";
        QString newCacheFolderName = normalizedCacheRootDir.absoluteFilePath("pc") + "/" + gameName + "/renamed_again";

        UNIT_TEST_EXPECT_TRUE(renamer.rename(originalCacheFolderName, newCacheFolderName));

        // tell it that the products moved:
        processResults.clear();
        assetMessages.clear();
        QMetaObject::invokeMethod(&apm, "AssessDeletedFile", Qt::QueuedConnection, Q_ARG(QString, originalCacheFolderName));
        QMetaObject::invokeMethod(&apm, "AssessAddedFile", Qt::QueuedConnection, Q_ARG(QString, newCacheFolderName));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        sortAssetToProcessResultList(processResults);

        // at this point, we should NOT get 2 removed products - we should only get those messages later
        // once the processing queue actually processes these assets - not prematurely as it discovers them missing.
        UNIT_TEST_EXPECT_TRUE(assetMessages.size() == 0);


        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_platformInfo.m_identifier == "pc");
        UNIT_TEST_EXPECT_TRUE(processResults[1].m_jobEntry.m_platformInfo.m_identifier == "pc");

        // -----------------------------------------------------------------------------------------------
        // -------------------------------------- FOLDER RENAMING TEST -----------------------------------
        // -----------------------------------------------------------------------------------------------
        // Test: Rename folders that did not have files in them (but had child files, this was a bug at a point)

        fileToMove1 = tempPath.absoluteFilePath("subfolder1/rename_this_secondly/somefolder/somefile2.txt");

        processResults.clear();
        // put the two files on the map:
        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, fileToMove1));


        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));
        sortAssetToProcessResultList(processResults);

        UNIT_TEST_EXPECT_TRUE(processResults.size() == 2); // 1 file on 2 platforms

        for (int index = 0; index < processResults.size(); ++index)
        {
            QFileInfo fi(processResults[index].m_jobEntry.m_absolutePathToFile);
            QString pcout = QDir(processResults[index].m_destinationPath).absoluteFilePath(fi.fileName());
            UNIT_TEST_EXPECT_TRUE(CreateDummyFile(pcout, "products."));

            response.m_outputProducts.clear();
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
            response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcout.toUtf8().constData()));
            QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[index].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));
        }

        // let events bubble through:
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

        // setup complete.  now RENAME that folder.

        originalCacheFolderName = normalizedCacheRootDir.absoluteFilePath("pc") + "/" + gameName + "/rename_this_secondly";
        newCacheFolderName = normalizedCacheRootDir.absoluteFilePath("pc") + "/" + gameName + "/done_renaming_again";

        UNIT_TEST_EXPECT_TRUE(renamer.rename(originalCacheFolderName, newCacheFolderName));

        // tell it that the products moved:
        processResults.clear();
        assetMessages.clear();
        QMetaObject::invokeMethod(&apm, "AssessDeletedFile", Qt::QueuedConnection, Q_ARG(QString, originalCacheFolderName));
        QMetaObject::invokeMethod(&apm, "AssessAddedFile", Qt::QueuedConnection, Q_ARG(QString, newCacheFolderName));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        sortAssetToProcessResultList(processResults);

        UNIT_TEST_EXPECT_TRUE(assetMessages.size() == 0); // we don't prematurely emit "AssetRemoved" until we actually finish process.
        UNIT_TEST_EXPECT_TRUE(processResults.size() == 1); // ONLY the PC files need to be re-processed because only those were renamed.
        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_platformInfo.m_identifier == "pc");

        // --------------------------------------------------------------------------------------------------
        // ------------------------------ TEST DELETED SOURCE RESULTING IN DELETED PRODUCTS -----------------
        // --------------------------------------------------------------------------------------------------

        // first, set up a whole pipeline to create, notify, and consume the file:
        fileToMove1 = tempPath.absoluteFilePath("subfolder1/to_be_deleted/some_deleted_file.txt");

        processResults.clear();
        // put the two files on the map:
        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, fileToMove1));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));
        sortAssetToProcessResultList(processResults);

        UNIT_TEST_EXPECT_TRUE(processResults.size() == 2); // 1 file on 2 platforms

        QStringList createdDummyFiles;

        for (int index = 0; index < processResults.size(); ++index)
        {
            QFileInfo fi(processResults[index].m_jobEntry.m_absolutePathToFile);
            QString pcout = QDir(processResults[index].m_destinationPath).absoluteFilePath(fi.fileName());
            UNIT_TEST_EXPECT_TRUE(CreateDummyFile(pcout, "products."));

            response.m_outputProducts.clear();
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
            response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcout.toUtf8().constData()));
            QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[index].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));
        }

        // let events bubble through:
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

        processResults.clear();
        assetMessages.clear();

        // setup complete.  now delete the source file:
        UNIT_TEST_EXPECT_TRUE(renamer.remove(fileToMove1));
        QMetaObject::invokeMethod(&apm, "AssessDeletedFile", Qt::QueuedConnection, Q_ARG(QString, fileToMove1));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));
        sortAssetToProcessResultList(processResults);
        UNIT_TEST_EXPECT_TRUE(assetMessages.size() == 2); // all products must be removed
        UNIT_TEST_EXPECT_TRUE(processResults.size() == 0); // nothing should process

        for (int index = 0; index < createdDummyFiles.size(); ++index)
        {
            QFileInfo fi(createdDummyFiles[index]);
            UNIT_TEST_EXPECT_FALSE(fi.exists());
            // in fact, the directory must also no longer exist in the cache:
            UNIT_TEST_EXPECT_FALSE(fi.dir().exists());
        }

        // --------------------------------------------------------------------------------------------------
        // - TEST SOURCE FILE REPROCESSING RESULTING IN FEWER PRODUCTS NEXT TIME ----------------------------
        // (it needs to delete the products and it needs to notify listeners about it)
        // --------------------------------------------------------------------------------------------------

        // first, set up a whole pipeline to create, notify, and consume the file:
        fileToMove1 = tempPath.absoluteFilePath("subfolder1/fewer_products/test.txt");

        processResults.clear();
        // put the two files on the map:
        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, fileToMove1));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));
        sortAssetToProcessResultList(processResults);

        UNIT_TEST_EXPECT_TRUE(processResults.size() == 2); // 1 file on 2 platforms

        createdDummyFiles.clear(); // keep track of the files which we expect to be gone next time

        for (int index = 0; index < processResults.size(); ++index)
        {
            response.m_outputProducts.clear();
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;

            // this time, ouput 2 files for each job instead of just one:
            QFileInfo fi(processResults[index].m_jobEntry.m_absolutePathToFile);

            response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(QDir(processResults[index].m_destinationPath).absoluteFilePath(fi.fileName() + ".0.txt").toUtf8().constData()));
            response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(QDir(processResults[index].m_destinationPath).absoluteFilePath(fi.fileName() + ".1.txt").toUtf8().constData()));

            createdDummyFiles.push_back(response.m_outputProducts[0].m_productFileName.c_str()); // we're only gong to delete this one out of the two, which is why we don't push the other one.

            UNIT_TEST_EXPECT_TRUE(CreateDummyFile(response.m_outputProducts[0].m_productFileName.c_str(), "product 0"));
            UNIT_TEST_EXPECT_TRUE(CreateDummyFile(response.m_outputProducts[1].m_productFileName.c_str(), "product 1"));

            QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[index].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));
        }

        // let events bubble through:
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

        // at this point, we have a cache with the four files (2 for each platform)
        // we're going to resubmit the job with different data
        UNIT_TEST_EXPECT_TRUE(renamer.remove(fileToMove1));
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(fileToMove1, "fresh data!"));

        processResults.clear();

        // tell file changed:
        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, fileToMove1));
        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        sortAssetToProcessResultList(processResults);

        UNIT_TEST_EXPECT_TRUE(processResults.size() == 2); // 1 file on 2 platforms

        assetMessages.clear();

        for (int index = 0; index < processResults.size(); ++index)
        {
            response.m_outputProducts.clear();
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;

            // this time, ouput only one file for each job instead of just one:
            QFileInfo fi(processResults[index].m_jobEntry.m_absolutePathToFile);

            response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(QDir(processResults[index].m_destinationPath).absoluteFilePath(fi.fileName() + ".1.txt").toUtf8().constData()));
            UNIT_TEST_EXPECT_TRUE(CreateDummyFile(response.m_outputProducts[0].m_productFileName.c_str(), "product 1 changed"));

            QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[index].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));
        }

        // let events bubble through:
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

        sortAssetToProcessResultList(processResults);

        // we should have gotten 2 product removed, 2 product changed, total of 4 asset messages

        UNIT_TEST_EXPECT_TRUE(assetMessages.size() == 4);
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].second.m_assetId != AZ::Data::AssetId());
        UNIT_TEST_EXPECT_TRUE(assetMessages[1].second.m_assetId != AZ::Data::AssetId());
        UNIT_TEST_EXPECT_TRUE(assetMessages[2].second.m_assetId != AZ::Data::AssetId());
        UNIT_TEST_EXPECT_TRUE(assetMessages[3].second.m_assetId != AZ::Data::AssetId());

        UNIT_TEST_EXPECT_TRUE(assetMessages[0].first == "es3");
        UNIT_TEST_EXPECT_TRUE(assetMessages[1].first == "es3");
        UNIT_TEST_EXPECT_TRUE(assetMessages[2].first == "pc");
        UNIT_TEST_EXPECT_TRUE(assetMessages[3].first == "pc");

        UNIT_TEST_EXPECT_TRUE(assetMessages[0].second.m_data == "fewer_products/test.txt.0.txt");
        UNIT_TEST_EXPECT_TRUE(assetMessages[1].second.m_data == "fewer_products/test.txt.1.txt");
        UNIT_TEST_EXPECT_TRUE(assetMessages[2].second.m_data == "fewer_products/test.txt.0.txt");
        UNIT_TEST_EXPECT_TRUE(assetMessages[3].second.m_data == "fewer_products/test.txt.1.txt");

        UNIT_TEST_EXPECT_TRUE(assetMessages[0].second.m_type == AzFramework::AssetSystem::AssetNotificationMessage::AssetRemoved);
        UNIT_TEST_EXPECT_TRUE(assetMessages[1].second.m_type == AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged);
        UNIT_TEST_EXPECT_TRUE(assetMessages[2].second.m_type == AzFramework::AssetSystem::AssetNotificationMessage::AssetRemoved);
        UNIT_TEST_EXPECT_TRUE(assetMessages[3].second.m_type == AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged);


        // and finally, the actual removed products should be gone from the HDD:
        for (int index = 0; index < createdDummyFiles.size(); ++index)
        {
            QFileInfo fi(createdDummyFiles[index]);
            UNIT_TEST_EXPECT_FALSE(fi.exists());
            // the directory must still exist because there were other files in there (no accidental deletions!)
            UNIT_TEST_EXPECT_TRUE(fi.dir().exists());
        }

        // -----------------------------------------------------------------------------------------------
        // ------------------- ASSETBUILDER TEST---------------------------------------------------
        //------------------------------------------------------------------------------------------------

        mockAppManager.ResetMatchingBuildersInfoFunctionCalls();
        mockAppManager.ResetMockBuilderCreateJobCalls();
        mockAppManager.UnRegisterAllBuilders();

        AssetRecognizer abt_rec1;
        AssetPlatformSpec abt_speces3;
        abt_rec1.m_name = "UnitTestTextBuilder1";
        abt_rec1.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.txt", AssetBuilderSDK::AssetBuilderPattern::Wildcard);
        //abt_rec1.m_regexp.setPatternSyntax(QRegExp::Wildcard);
        //abt_rec1.m_regexp.setPattern("*.txt");
        abt_rec1.m_platformSpecs.insert("es3", speces3);
        mockAppManager.RegisterAssetRecognizerAsBuilder(abt_rec1);

        AssetRecognizer abt_rec2;
        AssetPlatformSpec abt_specpc;
        abt_rec2.m_name = "UnitTestTextBuilder2";
        abt_rec2.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.txt", AssetBuilderSDK::AssetBuilderPattern::Wildcard);
        //abt_rec2.m_regexp.setPatternSyntax(QRegExp::Wildcard);
        //abt_rec2.m_regexp.setPattern("*.txt");
        abt_rec2.m_platformSpecs.insert("pc", specpc);
        mockAppManager.RegisterAssetRecognizerAsBuilder(abt_rec2);

        processResults.clear();

        inputFilePath = AssetUtilities::NormalizeFilePath(tempPath.absoluteFilePath("subfolder3/uniquefile.txt"));

        // Pass the txt file through the asset pipeline
        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, inputFilePath));
        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));
        UNIT_TEST_EXPECT_TRUE(mockAppManager.GetMatchingBuildersInfoFunctionCalls() == 1);
        UNIT_TEST_EXPECT_TRUE(mockAppManager.GetMockBuilderCreateJobCalls() == 2);  // Since we have two text builder registered

        AssetProcessor::BuilderInfoList builderInfoList;
        mockAppManager.GetMatchingBuildersInfo(AZStd::string(inputFilePath.toUtf8().constData()), builderInfoList);
        auto builderInfoListCount = builderInfoList.size();
        UNIT_TEST_EXPECT_TRUE(builderInfoListCount == 2);

        for (auto& buildInfo : builderInfoList)
        {
            AZStd::shared_ptr<InternalMockBuilder> builder;
            UNIT_TEST_EXPECT_TRUE(mockAppManager.GetBuilderByID(buildInfo.m_name, builder));

            UNIT_TEST_EXPECT_TRUE(builder->GetCreateJobCalls() == 1);
            
            // note, uuid does not include watch folder name.  This is a quick test to make sure that the source file UUID actually makes it into the CreateJobRequest.
            // the ProcessJobRequest is populated frmo the CreateJobRequest.
            UNIT_TEST_EXPECT_TRUE(builder->GetLastCreateJobRequest().m_sourceFileUUID == AssetUtilities::CreateSafeSourceUUIDFromName("uniquefile.txt"));
            QString watchedFolder(AssetUtilities::NormalizeFilePath(builder->GetLastCreateJobRequest().m_watchFolder.c_str()));
            QString expectedWatchedFolder(tempPath.absoluteFilePath("subfolder3"));
            UNIT_TEST_EXPECT_TRUE(QString::compare(watchedFolder, expectedWatchedFolder, Qt::CaseInsensitive) == 0); // verify watchfolder

            QString filename(AssetUtilities::NormalizeFilePath(builder->GetLastCreateJobRequest().m_sourceFile.c_str()));
            QString expectFileName("uniquefile.txt");
            UNIT_TEST_EXPECT_TRUE(QString::compare(filename, expectFileName, Qt::CaseInsensitive) == 0); // verify filename
            builder->ResetCounters();
        }

        sortAssetToProcessResultList(processResults);

        UNIT_TEST_EXPECT_TRUE(processResults.size() == 2); // 1 for pc and es3
        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_platformInfo.m_identifier == "es3");
        UNIT_TEST_EXPECT_TRUE(processResults[1].m_jobEntry.m_platformInfo.m_identifier == "pc");
        UNIT_TEST_EXPECT_TRUE(QString::compare(processResults[0].m_jobEntry.m_absolutePathToFile, inputFilePath, Qt::CaseInsensitive) == 0);
        UNIT_TEST_EXPECT_TRUE(QString::compare(processResults[1].m_jobEntry.m_absolutePathToFile, inputFilePath, Qt::CaseInsensitive) == 0);
        UNIT_TEST_EXPECT_TRUE(QString::compare(QString(processResults[0].m_jobEntry.m_jobKey), QString(abt_rec1.m_name)) == 0);
        UNIT_TEST_EXPECT_TRUE(QString::compare(QString(processResults[1].m_jobEntry.m_jobKey), QString(abt_rec2.m_name)) == 0);
        Q_EMIT UnitTestPassed();
    }

    void AssetProcessorManagerUnitTests_JobKeys::StartTest()
    {
        // Test Strategy
        // Tell the mock builder to create two jobs for the same source file and platform but having different job keys.
        // Feed the source file to the asset pipeline and ensure we get two jobs to be processed.
        // Register products for those jobs in the asset database.
        // Delete all products for one of those jobs and feed the source file to the asset pipeline, ensure that we get only one job to be processed.
        // Tell the mock builder to create one job now for the same source file and platform.
        // Feed the source file to the asset pipeline and ensure that we do not get any new jobs to be processed and also ensure that all the products of the missing jobs are deleted from disk.
        // Tell the mock builder to create two jobs again for the same source file and platform but having different job keys.
        // Feed the source file to the asset pipeline and ensure that we do get a new job to be process this time.

        // attach a file monitor to ensure this occurs.
        MockAssetBuilderInfoHandler mockAssetBuilderInfoHandler;

        QDir oldRoot;
        AssetUtilities::ComputeAssetRoot(oldRoot);
        AssetUtilities::ResetAssetRoot();

        QTemporaryDir dir;
        UnitTestUtils::ScopedDir changeDir(dir.path());
        QDir tempPath(dir.path());

        CreateDummyFile(tempPath.absoluteFilePath("bootstrap.cfg"), QString("sys_game_folder=SamplesProject\n"));

        // system is already actually initialized, along with gEnv, so this will always return that game name.
        QString gameName = AssetUtilities::ComputeGameName();

        // update the engine root
        AssetUtilities::ResetAssetRoot();
        QDir newRoot;
        AssetUtilities::ComputeAssetRoot(newRoot, &tempPath);

        UNIT_TEST_EXPECT_FALSE(gameName.isEmpty());

        PlatformConfiguration config;
        config.EnablePlatform({ "pc" ,{ "desktop", "renderer" } }, true);
        AZStd::vector<AssetBuilderSDK::PlatformInfo> platforms;
        config.PopulatePlatformsForScanFolder(platforms);
        //                                               PATH                 DisplayName  PortKey       outputfolder  root recurse order
        config.AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder1"), "subfolder1", "subfolder1", "", false, true, platforms,-1)); // subfolder1 overrides root
        config.AddScanFolder(ScanFolderInfo(tempPath.absolutePath(), "temp", "tempfolder", "", true, false, platforms, 0)); // add the root

        AssetProcessorManager_Test apm(&config);

        QDir cacheRoot;
        UNIT_TEST_EXPECT_TRUE(AssetUtilities::ComputeProjectCacheRoot(cacheRoot));

        QList<JobDetails> processResults;
        QList<QPair<QString, QString> > changedInputResults;
        QList<QPair<QString, AzFramework::AssetSystem::AssetNotificationMessage> > assetMessages;

        bool idling = false;

        connect(&apm, &AssetProcessorManager::AssetToProcess,
            this, [&processResults](JobDetails details)
        {
            processResults.push_back(AZStd::move(details));
        });

        connect(&apm, &AssetProcessorManager::AssetMessage,
            this, [&assetMessages](QString platform, AzFramework::AssetSystem::AssetNotificationMessage message)
        {
            assetMessages.push_back(qMakePair(platform, message));
        });

        connect(&apm, &AssetProcessorManager::InputAssetProcessed,
            this, [&changedInputResults](QString relativePath, QString platform)
        {
            changedInputResults.push_back(QPair<QString, QString>(relativePath, platform));
        });

        connect(&apm, &AssetProcessorManager::AssetProcessorManagerIdleState,
            this, [&idling](bool state)
        {
            idling = state;
        }
        );

        QString sourceFile = tempPath.absoluteFilePath("subfolder1/basefile.foo");
        CreateDummyFile(sourceFile);

        mockAssetBuilderInfoHandler.m_numberOfJobsToCreate = 2; //Create two jobs for this file

        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, sourceFile));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        // block until no more events trickle in:
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        sortAssetToProcessResultList(processResults);

        UNIT_TEST_EXPECT_TRUE(processResults.size() == 2);
        for (int idx = 0; idx < processResults.size(); idx++)
        {
            UNIT_TEST_EXPECT_TRUE((processResults[idx].m_jobEntry.m_platformInfo.m_identifier == "pc"));
            UNIT_TEST_EXPECT_TRUE(processResults[idx].m_jobEntry.m_relativePathToFile.startsWith("basefile.foo"));
        }
        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_jobKey.compare(processResults[1].m_jobEntry.m_jobKey) != 0);

        QStringList pcouts;
        pcouts.push_back(cacheRoot.filePath(QString("pc/") + gameName + "/basefile.arc1"));
        pcouts.push_back(cacheRoot.filePath(QString("pc/") + gameName + "/basefile.arc2"));

        // Create the product files for the first job 
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(pcouts[0], "product1"));
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(pcouts[1], "product2"));


        // Invoke Asset Processed for pc platform for the first job
        AssetBuilderSDK::ProcessJobResponse response;
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcouts[0].toUtf8().constData()));
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcouts[1].toUtf8().constData()));

        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[0].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        // let events bubble through:
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        UNIT_TEST_EXPECT_TRUE(assetMessages.size() == 2);

        UNIT_TEST_EXPECT_TRUE(changedInputResults.size() == 1);

        UNIT_TEST_EXPECT_TRUE(assetMessages[0].first == "pc");
        UNIT_TEST_EXPECT_TRUE(assetMessages[1].first == "pc");
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].second.m_data == "basefile.arc1");
        UNIT_TEST_EXPECT_TRUE(assetMessages[1].second.m_data == "basefile.arc2");
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].second.m_type == AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged);
        UNIT_TEST_EXPECT_TRUE(assetMessages[1].second.m_type == AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged);

        UNIT_TEST_EXPECT_TRUE(AssetUtilities::NormalizeFilePath(changedInputResults[0].first) == AssetUtilities::NormalizeFilePath(sourceFile));

        pcouts.clear();
        pcouts.push_back(cacheRoot.filePath(QString("pc/") + gameName + "/basefile.arc3"));
        // Create the product files for the second job 
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(pcouts[0], "product1"));

        // Invoke Asset Processed for pc platform for the second job
        response.m_outputProducts.clear();
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcouts[0].toUtf8().constData()));
        assetMessages.clear();
        changedInputResults.clear();
        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[1].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        // let events bubble through:
        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        UNIT_TEST_EXPECT_TRUE(assetMessages.size() == 1);

        UNIT_TEST_EXPECT_TRUE(changedInputResults.size() == 1);

        UNIT_TEST_EXPECT_TRUE(assetMessages[0].first == "pc");
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].second.m_data == "basefile.arc3");
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].second.m_type == AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged);

        UNIT_TEST_EXPECT_TRUE(AssetUtilities::NormalizeFilePath(changedInputResults[0].first) == AssetUtilities::NormalizeFilePath(sourceFile));

        //Delete the product of the second job
        UNIT_TEST_EXPECT_TRUE(QFile::remove(pcouts[0]));

        processResults.clear();

        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, sourceFile));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        // block until no more events trickle in:
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        sortAssetToProcessResultList(processResults);

        UNIT_TEST_EXPECT_TRUE(processResults.size() == 1); // We should only have one job to process here
        for (int idx = 0; idx < processResults.size(); idx++)
        {
            UNIT_TEST_EXPECT_TRUE((processResults[idx].m_jobEntry.m_platformInfo.m_identifier == "pc"));
            UNIT_TEST_EXPECT_TRUE(processResults[idx].m_jobEntry.m_relativePathToFile.startsWith("basefile.foo"));
        }

        mockAssetBuilderInfoHandler.m_numberOfJobsToCreate = 1; //Create one job for this file this time

        processResults.clear();

        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, sourceFile));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        // block until no more events trickle in:
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        UNIT_TEST_EXPECT_TRUE(processResults.size() == 0); // We should not have any job to process here

        // products of the second job should not exists any longer
        for (QString outFile : pcouts)
        {
            UNIT_TEST_EXPECT_FALSE(QFile::exists(pcouts[0]));
        }

        mockAssetBuilderInfoHandler.m_numberOfJobsToCreate = 2; //Again create two jobs for this file, this should result in one additional job

        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, sourceFile));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        // block until no more events trickle in:
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        UNIT_TEST_EXPECT_TRUE(processResults.size() == 1); // We should see a job to process here
        for (int idx = 0; idx < processResults.size(); idx++)
        {
            UNIT_TEST_EXPECT_TRUE((processResults[idx].m_jobEntry.m_platformInfo.m_identifier == "pc"));
            UNIT_TEST_EXPECT_TRUE(processResults[idx].m_jobEntry.m_relativePathToFile.startsWith("basefile.foo"));
        }

        Q_EMIT UnitTestPassed();
    }

    void AssetProcessorManagerUnitTests_ScanFolders::StartTest()
    {
        using namespace AzToolsFramework::AssetDatabase;

        MockApplicationManager  mockAppManager;
        mockAppManager.BusConnect();

        QDir oldRoot;
        AssetUtilities::ComputeAssetRoot(oldRoot);
        AssetUtilities::ResetAssetRoot();

        QTemporaryDir dir;
        UnitTestUtils::ScopedDir changeDir(dir.path());
        QDir tempPath(dir.path());

        CreateDummyFile(tempPath.absoluteFilePath("bootstrap.cfg"), QString("sys_game_folder=SamplesProject\n"));

        // system is already actually initialized, along with gEnv, so this will always return that game name.
        QString gameName = AssetUtilities::ComputeGameName();

        // update the engine root
        AssetUtilities::ResetAssetRoot();
        QDir newRoot;
        AssetUtilities::ComputeAssetRoot(newRoot, &tempPath);

        UNIT_TEST_EXPECT_FALSE(gameName.isEmpty());
        // should create cache folder in the root, and read everything from there.

        PlatformConfiguration config;
        AZStd::vector<AssetBuilderSDK::PlatformInfo> platforms;
        config.PopulatePlatformsForScanFolder(platforms);
        //                                               PATH                 DisplayName  PortKey       outputfolder  root recurse platforms order
        config.AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder4"), "subfolder4", "subfolder4", "", false, false, platforms, -6)); // subfolder 4 overrides subfolder3
        config.AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder3"), "subfolder3", "subfolder3", "", false, false, platforms, -5)); // subfolder 3 overrides subfolder2
        config.AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder2"), "subfolder2", "subfolder2", "", false, true, platforms, -2)); // subfolder 2 overrides subfolder1
        config.AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder1"), "subfolder1", "subfolder1", "", false, true, platforms, -1)); // subfolder1 overrides root
        config.AddScanFolder(ScanFolderInfo(tempPath.absolutePath(), "temp", "tempfolder", "", true, false, platforms, 0)); // add the root

        {
            // create this, which will write those scan folders into the db as-is
            AssetProcessorManager_Test apm(&config);
        }

        ScanFolderDatabaseEntryContainer entryContainer;
        auto puller = [&entryContainer](ScanFolderDatabaseEntry& entry)
            {
                entryContainer.push_back(entry);
                return true;
            };

        {
            AssetDatabaseConnection connection;
            UNIT_TEST_EXPECT_TRUE(connection.OpenDatabase());
            // make sure we find the scan folders.
            int numFound = 0;
            entryContainer.clear();
            connection.QueryScanFoldersTable(puller);
            UNIT_TEST_EXPECT_TRUE(config.GetScanFolderCount() == entryContainer.size());
            // make sure they are all present and have port key:
            for (int idx = 0; idx < config.GetScanFolderCount(); ++idx)
            {
                AssetProcessor::ScanFolderInfo& scanFolderInConfig = config.GetScanFolderAt(idx);
                auto found = AZStd::find_if(entryContainer.begin(), entryContainer.end(), [&scanFolderInConfig](const ScanFolderDatabaseEntry& target)
                        {
                            return (
                                (target.m_scanFolderID == scanFolderInConfig.ScanFolderID()) &&
                                (scanFolderInConfig.GetPortableKey() == target.m_portableKey.c_str()) &&
                                (scanFolderInConfig.ScanPath() == target.m_scanFolder.c_str()) &&
                                (scanFolderInConfig.GetDisplayName() == target.m_displayName.c_str())
                                );
                        }
                        );

                UNIT_TEST_EXPECT_TRUE(found != entryContainer.end());
            }
        }

        // now make a different config with different scan folders but with some of the same portable keys but new paths.
        PlatformConfiguration config2;
        AZStd::vector<AssetBuilderSDK::PlatformInfo> platforms2;
        config2.PopulatePlatformsForScanFolder(platforms2);
        //                                               PATH                 DisplayName  PortKey       outputfolder  root recurse platforms order
        // case 1:  same absolute path, but the same portable key - should use same ID as before.
        config2.AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder4"), "subfolder4", "subfolder4", "", false, false, platforms2, -6)); // subfolder 4 overrides subfolder3

        // case 2:  A new absolute path, but same portable key - should use same id as before
        config2.AddScanFolder(ScanFolderInfo(tempPath.filePath("newfolder3"), "subfolder3", "subfolder3", "", false, false, platforms2, -5)); // subfolder 3 overrides subfolder2

        // case 3:  same absolute path, new portable key - should use a new ID
        config2.AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder1"), "subfolder3", "newfolder3", "", false, false, platforms2, -5)); // subfolder 3 overrides subfolder2

        // case 4:  subfolder2 is missing - it should be gone.

        {
            // create this, which will write those scan folders into the db as-is
            AssetProcessorManager_Test apm(&config2);
            apm.CheckMissingFiles();
        }

        {
            AssetDatabaseConnection connection;
            UNIT_TEST_EXPECT_TRUE(connection.OpenDatabase());
            // make sure we find the scan folders.
            int numFound = 0;
            entryContainer.clear();
            connection.QueryScanFoldersTable(puller);
            UNIT_TEST_EXPECT_TRUE(config2.GetScanFolderCount() == entryContainer.size());

            // make sure they are all present and have port key:
            for (int idx = 0; idx < config2.GetScanFolderCount(); ++idx)
            {
                AssetProcessor::ScanFolderInfo& scanFolderInConfig = config2.GetScanFolderAt(idx);
                auto found = AZStd::find_if(entryContainer.begin(), entryContainer.end(), [&scanFolderInConfig](const ScanFolderDatabaseEntry& target)
                        {
                            return (
                                (target.m_scanFolderID == scanFolderInConfig.ScanFolderID()) &&
                                (scanFolderInConfig.GetPortableKey() == target.m_portableKey.c_str()) &&
                                (scanFolderInConfig.ScanPath() == target.m_scanFolder.c_str()) &&
                                (scanFolderInConfig.GetDisplayName() == target.m_displayName.c_str())
                                );
                        }
                        );

                UNIT_TEST_EXPECT_TRUE(found != entryContainer.end());
            }
        }

        const AssetProcessor::ScanFolderInfo* subfolder4InConfig1 = nullptr;
        const AssetProcessor::ScanFolderInfo* subfolder4InConfig2 = nullptr;

        const AssetProcessor::ScanFolderInfo* subfolder3InConfig1 = nullptr;
        const AssetProcessor::ScanFolderInfo* subfolder3InConfig2 = nullptr;

        AZStd::unordered_set<AZ::s64> idsInConfig1;

        for (int idx = 0; idx < config.GetScanFolderCount(); ++idx)
        {
            AssetProcessor::ScanFolderInfo& scanFolderInConfig = config.GetScanFolderAt(idx);
            idsInConfig1.insert(scanFolderInConfig.ScanFolderID());

            if (scanFolderInConfig.GetPortableKey() == "subfolder4")
            {
                subfolder4InConfig1 = &scanFolderInConfig;
            }

            if (scanFolderInConfig.GetPortableKey() == "subfolder3")
            {
                subfolder3InConfig1 = &scanFolderInConfig;
            }
        }

        for (int idx = 0; idx < config2.GetScanFolderCount(); ++idx)
        {
            AssetProcessor::ScanFolderInfo& scanFolderInConfig = config.GetScanFolderAt(idx);

            if (scanFolderInConfig.GetPortableKey() == "subfolder4")
            {
                subfolder4InConfig2 = &scanFolderInConfig;
            }

            if (scanFolderInConfig.GetPortableKey() == "subfolder3")
            {
                subfolder3InConfig2 = &scanFolderInConfig;
            }

            if (scanFolderInConfig.GetPortableKey() == "newfolder3")
            {
                // it must be a new ID, so it can't reuse any ids.
                UNIT_TEST_EXPECT_TRUE(idsInConfig1.find(scanFolderInConfig.ScanFolderID()) == idsInConfig1.end()); // must not be found
            }
        }

        UNIT_TEST_EXPECT_TRUE(subfolder3InConfig2);
        UNIT_TEST_EXPECT_TRUE(subfolder3InConfig1);

        UNIT_TEST_EXPECT_TRUE(subfolder4InConfig2);
        UNIT_TEST_EXPECT_TRUE(subfolder4InConfig1);

        // the above scan folders should not have changed id
        UNIT_TEST_EXPECT_TRUE(subfolder3InConfig1->ScanFolderID() == subfolder3InConfig2->ScanFolderID());
        UNIT_TEST_EXPECT_TRUE(subfolder4InConfig1->ScanFolderID() == subfolder4InConfig2->ScanFolderID());

        Q_EMIT UnitTestPassed();
    }

    void AssetProcessorManagerUnitTests_SourceFileDependencies::StartTest()
    {
        using namespace AzToolsFramework::AssetDatabase;

        MockApplicationManager  mockAppManager;
        mockAppManager.BusConnect();

        QDir oldRoot;
        AssetUtilities::ComputeAssetRoot(oldRoot);
        AssetUtilities::ResetAssetRoot();

        QTemporaryDir dir;
        UnitTestUtils::ScopedDir changeDir(dir.path());
        QDir tempPath(dir.path());

        CreateDummyFile(tempPath.absoluteFilePath("bootstrap.cfg"), QString("sys_game_folder=SamplesProject\n"));

        // system is already actually initialized, along with gEnv, so this will always return that game name.
        QString gameName = AssetUtilities::ComputeGameName();

        // update the engine root
        AssetUtilities::ResetAssetRoot();
        QDir newRoot;
        AssetUtilities::ComputeAssetRoot(newRoot, &tempPath);

        UNIT_TEST_EXPECT_FALSE(gameName.isEmpty());
        // should create cache folder in the root, and read everything from there.

        PlatformConfiguration config;
        AZStd::vector<AssetBuilderSDK::PlatformInfo> platforms;
        config.PopulatePlatformsForScanFolder(platforms);
        //                                               PATH                 DisplayName  PortKey       outputfolder  root recurse platforms order
        config.AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder1"), "subfolder1", "subfolder1", "", false, true, platforms, -1)); // subfolder1 overrides root
        config.AddScanFolder(ScanFolderInfo(tempPath.absolutePath(), "temp", "tempfolder", "", true, false, platforms, 0)); // add the root

        
        AssetProcessorManager_Test apm(&config);
        AZ::Uuid builderUuid = AZ::Uuid::CreateRandom();
        AZ::Uuid builder2Uuid = AZ::Uuid::CreateRandom();
        //Adding two entries in the source dependency table
        SourceFileDependencyEntry firstEntry(builderUuid, "FileA.txt", "subfolder2/FileB.txt"); //FileA depends on FileB
        apm.UNITTEST_SetSourceFileDependency(firstEntry);
        SourceFileDependencyEntry secondEntry(builderUuid, "FileC.txt", "subfolder2/FileB.txt"); //FileC depends on FileB
        apm.UNITTEST_SetSourceFileDependency(secondEntry);

        QString sourceFileAPath = tempPath.absoluteFilePath("subfolder1/FileA.txt");
        QString sourceFileCPath = tempPath.absoluteFilePath("subfolder1/FileC.txt");
        QString sourceFileBPath = tempPath.absoluteFilePath("subfolder1/subfolder2/FileB.txt");
        QString sourceFileDPath = tempPath.absoluteFilePath("subfolder1/subFolder2/FileD.txt");
        QString sourceFileEPath = tempPath.absoluteFilePath("subfolder1/FileE.txt");
        QString sourceFileFPath = tempPath.absoluteFilePath("subfolder1/FileF.txt");
        QString sourceFileGPath = tempPath.absoluteFilePath("subfolder1/FileG.txt");
        QString sourceFileHPath = tempPath.absoluteFilePath("subfolder1/FileH.txt");
        CreateDummyFile(sourceFileBPath, QString(""));
        CreateDummyFile(sourceFileDPath, QString(""));
        CreateDummyFile(sourceFileAPath, QString(""));
        CreateDummyFile(sourceFileCPath, QString(""));
        CreateDummyFile(sourceFileEPath, QString(""));
        CreateDummyFile(sourceFileFPath, QString(""));
        CreateDummyFile(sourceFileGPath, QString(""));
        CreateDummyFile(sourceFileHPath, QString(""));

        //Adding FileB.txt in the database as a source file, it will enable us to search this source file by Uuid
        const ScanFolderInfo* scanFolderInfo = apm.UNITTEST_GetScanFolderForFile(sourceFileBPath.toUtf8().constData());
        AZ::Uuid fileBUuid = AssetUtilities::CreateSafeSourceUUIDFromName("subfolder2/FileB.txt");
        AZ::Uuid fileDUuid = AssetUtilities::CreateSafeSourceUUIDFromName("subfolder2/FileD.txt");
        AzToolsFramework::AssetDatabase::SourceDatabaseEntry sourceDatabaseEntry(-1, scanFolderInfo->ScanFolderID(), "subfolder2/FileB.txt", fileBUuid);
        apm.UNITTEST_SetSource(sourceDatabaseEntry); 
        QStringList sourceFiles = apm.UNITTEST_CheckSourceFileDependency(sourceFileBPath);
        UNIT_TEST_EXPECT_TRUE(sourceFiles.size() == 2); // Both the entries are found from the database
        UNIT_TEST_EXPECT_TRUE(sourceFiles[0].compare("FileA.txt", Qt::CaseInsensitive) == 0 || sourceFiles[0].compare("FileC.txt", Qt::CaseInsensitive) == 0);
        UNIT_TEST_EXPECT_TRUE(sourceFiles[1].compare("FileA.txt", Qt::CaseInsensitive) == 0 || sourceFiles[1].compare("FileC.txt", Qt::CaseInsensitive) == 0);

        AZStd::vector<AssetBuilderSDK::PlatformInfo> PCInfos = { { "pc", { "desktop", "renderer" } } };

        {
            // here we are just providing the source file dependency path which is not relative to any watch folders in sourceFileDependency
            sourceFiles.clear();
            AssetBuilderSDK::CreateJobsRequest jobRequest(builderUuid, "subfolder2/FileD.txt", tempPath.filePath("subfolder1").toUtf8().data(), PCInfos, fileDUuid);
            AssetBuilderSDK::CreateJobsResponse jobResponse;
            AssetBuilderSDK::SourceFileDependency sourceFileDependency;
            sourceFileDependency.m_sourceFileDependencyPath = "FileB.txt"; //path is not relative to the watch folder
            jobResponse.m_sourceFileDependencyList.push_back(sourceFileDependency);
            apm.UNITTEST_ProcessCreateJobsResponse(jobResponse, jobRequest);
            sourceFiles = apm.UNITTEST_CheckSourceFileDependency(sourceFileBPath);

            UNIT_TEST_EXPECT_TRUE(sourceFiles.size() == 3); // 2 entries from the database and 1 entry from APM internal maps
            UNIT_TEST_EXPECT_TRUE(sourceFiles.contains("FileA.txt", Qt::CaseInsensitive));
            UNIT_TEST_EXPECT_TRUE(sourceFiles.contains("FileC.txt", Qt::CaseInsensitive));
            UNIT_TEST_EXPECT_TRUE(sourceFiles.contains("subfolder2/FileD.txt", Qt::CaseInsensitive));
        }
        
        apm.UNITTEST_ClearSourceFileDependencyInternalMaps();

        {
            // here we are just providing the source file dependency path which is relative to one of the watch folders in sourceFileDependency
            sourceFiles.clear();
            AssetBuilderSDK::CreateJobsRequest jobRequest(builderUuid, "subfolder2/FileD.txt", tempPath.filePath("subfolder1").toUtf8().data(), PCInfos, fileDUuid);
            AssetBuilderSDK::CreateJobsResponse jobResponse;
            AssetBuilderSDK::SourceFileDependency sourceFileDependency;
            sourceFileDependency.m_sourceFileDependencyPath = "subfolder2/FileB.txt"; //path is relative to the watch folder
            jobResponse.m_sourceFileDependencyList.push_back(sourceFileDependency);
            apm.UNITTEST_ProcessCreateJobsResponse(jobResponse, jobRequest);
            sourceFiles = apm.UNITTEST_CheckSourceFileDependency(sourceFileBPath);

            UNIT_TEST_EXPECT_TRUE(sourceFiles.size() == 3); // 2 entries from the database and 1 entry from APM internal maps
            UNIT_TEST_EXPECT_TRUE(sourceFiles.contains("FileA.txt", Qt::CaseInsensitive));
            UNIT_TEST_EXPECT_TRUE(sourceFiles.contains("FileC.txt", Qt::CaseInsensitive));
            UNIT_TEST_EXPECT_TRUE(sourceFiles.contains("subfolder2/FileD.txt", Qt::CaseInsensitive));
        }

        apm.UNITTEST_ClearSourceFileDependencyInternalMaps();
        
        {
            // here we are providing source uuid instead of a file path in sourceFileDependency
            sourceFiles.clear();
            AssetBuilderSDK::CreateJobsRequest jobRequest(builderUuid, "subfolder2/FileD.txt", tempPath.filePath("subfolder1").toUtf8().data(), PCInfos, fileDUuid);
            AssetBuilderSDK::CreateJobsResponse jobResponse;
            AssetBuilderSDK::SourceFileDependency sourceFileDependency;
            sourceFileDependency.m_sourceFileDependencyUUID = fileBUuid; //file uuid instead of file path
            jobResponse.m_sourceFileDependencyList.push_back(sourceFileDependency);
            apm.UNITTEST_ProcessCreateJobsResponse(jobResponse, jobRequest);
            sourceFiles = apm.UNITTEST_CheckSourceFileDependency(sourceFileBPath);
            UNIT_TEST_EXPECT_TRUE(sourceFiles.size() == 3); // 2 entries from the database and 1 entry from APM internal maps
            UNIT_TEST_EXPECT_TRUE(sourceFiles.contains("FileA.txt", Qt::CaseInsensitive));
            UNIT_TEST_EXPECT_TRUE(sourceFiles.contains("FileC.txt", Qt::CaseInsensitive));
            UNIT_TEST_EXPECT_TRUE(sourceFiles.contains("subfolder2/FileD.txt", Qt::CaseInsensitive));
        }

        apm.UNITTEST_ClearSourceFileDependencyInternalMaps();

        {
            // here we are providing an invalid path in sourceFileDependency
            sourceFiles.clear();
            AssetBuilderSDK::CreateJobsRequest jobRequest(builderUuid, "subfolder2/FileD.txt", tempPath.filePath("subfolder1").toUtf8().data(), PCInfos, fileDUuid);
            AssetBuilderSDK::CreateJobsResponse jobResponse;
            AssetBuilderSDK::SourceFileDependency sourceFileDependency;
            sourceFileDependency.m_sourceFileDependencyPath = "subfolder1/FileB.txt"; 
            jobResponse.m_sourceFileDependencyList.push_back(sourceFileDependency);
            apm.UNITTEST_ProcessCreateJobsResponse(jobResponse, jobRequest);
            sourceFiles = apm.UNITTEST_CheckSourceFileDependency(sourceFileBPath);
            UNIT_TEST_EXPECT_TRUE(sourceFiles.size() == 2); // Both the entries from the database, invalid entry is ignored
            UNIT_TEST_EXPECT_TRUE(sourceFiles.contains("FileA.txt", Qt::CaseInsensitive));
            UNIT_TEST_EXPECT_TRUE(sourceFiles.contains("FileC.txt", Qt::CaseInsensitive));
        }

        apm.UNITTEST_ClearSourceFileDependencyInternalMaps();

        {
            // here we are providing an absolute path in sourceFileDependency
            sourceFiles.clear();
            AssetBuilderSDK::CreateJobsRequest jobRequest(builderUuid, "subfolder2/FileD.txt", tempPath.filePath("subfolder1").toUtf8().data(), PCInfos, fileDUuid);
            AssetBuilderSDK::CreateJobsResponse jobResponse;
            AssetBuilderSDK::SourceFileDependency sourceFileDependency;
            sourceFileDependency.m_sourceFileDependencyPath = sourceFileBPath.toUtf8().data();
            jobResponse.m_sourceFileDependencyList.push_back(sourceFileDependency);
            apm.UNITTEST_ProcessCreateJobsResponse(jobResponse, jobRequest);
            sourceFiles = apm.UNITTEST_CheckSourceFileDependency(sourceFileBPath);
            UNIT_TEST_EXPECT_TRUE(sourceFiles.size() == 3); // 2 entries from the database and 1 entry from APM internal maps
            UNIT_TEST_EXPECT_TRUE(sourceFiles.contains("FileA.txt", Qt::CaseInsensitive));
            UNIT_TEST_EXPECT_TRUE(sourceFiles.contains("FileC.txt", Qt::CaseInsensitive));
            UNIT_TEST_EXPECT_TRUE(sourceFiles.contains("subfolder2/FileD.txt", Qt::CaseInsensitive));
        }

        apm.UNITTEST_ClearSourceFileDependencyInternalMaps();
        AZ::Uuid fileEUuid = AssetUtilities::CreateSafeSourceUUIDFromName("FileE.txt");

        {
            // here we are emitting a source file dependency on a file which AP only becomes aware of later on 
            sourceFiles.clear();
            AssetBuilderSDK::CreateJobsRequest jobRequest(builderUuid, "subfolder2/FileD.txt", tempPath.filePath("subfolder1").toUtf8().data(), PCInfos, fileDUuid);
            AssetBuilderSDK::CreateJobsResponse jobResponse;
            AssetBuilderSDK::SourceFileDependency sourceFileDependency;
            sourceFileDependency.m_sourceFileDependencyUUID = fileEUuid;//FileE is not present in the database and AP has never seen any such file
            jobResponse.m_sourceFileDependencyList.push_back(sourceFileDependency);
            apm.UNITTEST_ProcessCreateJobsResponse(jobResponse, jobRequest);

            //This will happen when a file is forwarded to the APM by the filewatcher
            sourceFiles = apm.UNITTEST_CheckSourceFileDependency(sourceFileEPath);

            UNIT_TEST_EXPECT_TRUE(sourceFiles.size() == 1); // entry is found in APM internal maps
            UNIT_TEST_EXPECT_TRUE(sourceFiles[0].compare("subfolder2/FileD.txt", Qt::CaseInsensitive) == 0);
        }

        apm.UNITTEST_ClearSourceFileDependencyInternalMaps();

        {
            // here we are processing FileA and since FileA has registered subfolder2/FileB as its source file dependency, we should find it
            sourceFiles.clear();
            AssetProcessorManager::JobToProcessEntry entry;
            entry.m_sourceFileInfo.m_relativePath = "FileA.txt";
            entry.m_sourceFileInfo.m_scanFolder = scanFolderInfo;
            AssetProcessor::JobDetails jobDetails;
            jobDetails.m_watchFolder = scanFolderInfo->ScanPath();
            jobDetails.m_jobEntry.m_builderGuid = builderUuid;
            jobDetails.m_jobEntry.m_absolutePathToFile = sourceFileAPath;
            jobDetails.m_jobEntry.m_relativePathToFile = "FileA.txt";
            entry.m_jobsToAnalyze.push_back(jobDetails);
            apm.UNITTEST_UpdateSourceFileDependencyInfo();
            apm.UNITTEST_UpdateSourceFileDependencyDatabase();
            apm.UNITTEST_AnalyzeJobDetail(entry);
            AZStd::vector<SourceFileDependencyInternal>& sourceFileDependencyList = entry.m_jobsToAnalyze[0].m_sourceFileDependencyList;
            UNIT_TEST_EXPECT_TRUE(sourceFileDependencyList.size() == 1);
            UNIT_TEST_EXPECT_TRUE(sourceFileDependencyList[0].m_relativeSourceFileDependencyPath.compare("subfolder2/FileB.txt") == 0);
        }

        apm.UNITTEST_ClearSourceFileDependencyInternalMaps();

        //Adding one more entry in the source dependency table, now FileA depends on both FileB and FileE
        SourceFileDependencyEntry thirdEntry(builderUuid, "FileA.txt", "FileE.txt");
        apm.UNITTEST_SetSourceFileDependency(thirdEntry);
        apm.UNITTEST_ClearSourceFileDependencyInternalMaps();

        {
            // here we are again processing FileA and since FileA has registered an additional source file dependency, we should find all
            AssetProcessorManager::JobToProcessEntry entry;
            entry.m_sourceFileInfo.m_relativePath = "FileA.txt";
            entry.m_sourceFileInfo.m_scanFolder = scanFolderInfo;
            AssetProcessor::JobDetails jobDetails;
            jobDetails.m_watchFolder = scanFolderInfo->ScanPath();
            jobDetails.m_jobEntry.m_builderGuid = builderUuid;
            jobDetails.m_jobEntry.m_absolutePathToFile = sourceFileAPath;
            jobDetails.m_jobEntry.m_relativePathToFile = "FileA.txt";
            entry.m_jobsToAnalyze.push_back(jobDetails);
            apm.UNITTEST_UpdateSourceFileDependencyInfo();
            apm.UNITTEST_UpdateSourceFileDependencyDatabase();
            apm.UNITTEST_AnalyzeJobDetail(entry);
            AZStd::vector<SourceFileDependencyInternal>& sourceFileDependencyList = entry.m_jobsToAnalyze[0].m_sourceFileDependencyList;
            UNIT_TEST_EXPECT_TRUE(sourceFileDependencyList.size() == 2);
            UNIT_TEST_EXPECT_TRUE(sourceFileDependencyList[0].m_relativeSourceFileDependencyPath != sourceFileDependencyList[1].m_relativeSourceFileDependencyPath);
            UNIT_TEST_EXPECT_TRUE(sourceFileDependencyList[0].m_relativeSourceFileDependencyPath.compare("subfolder2/FileB.txt") == 0 || 
                                  sourceFileDependencyList[0].m_relativeSourceFileDependencyPath.compare("FileE.txt") == 0);
            UNIT_TEST_EXPECT_TRUE(sourceFileDependencyList[1].m_relativeSourceFileDependencyPath.compare("subfolder2/FileB.txt") == 0 ||
                                  sourceFileDependencyList[1].m_relativeSourceFileDependencyPath.compare("FileE.txt") == 0);
        }

        apm.UNITTEST_ClearSourceFileDependencyInternalMaps();

        {
            // here we are again processing FileA
            // since FileA depends on FileB.txt and FileE. Here we are adding another dependency to FileE i.e FileE depends on FileD 
            // therefore when we process FileA we should see only the following dependencies FileB, FileE and FileD 
            sourceFiles.clear();
            AssetBuilderSDK::CreateJobsRequest jobRequest(builderUuid, "FileE.txt", tempPath.filePath("subfolder1").toUtf8().data(), PCInfos, fileEUuid);
            AssetBuilderSDK::CreateJobsResponse jobResponse;
            AssetBuilderSDK::SourceFileDependency sourceFileDependency;
            sourceFileDependency.m_sourceFileDependencyPath = "subfolder2/FileD.txt";
            jobResponse.m_sourceFileDependencyList.push_back(sourceFileDependency);
            apm.UNITTEST_ProcessCreateJobsResponse(jobResponse, jobRequest);

            AssetProcessorManager::JobToProcessEntry entry;
            entry.m_sourceFileInfo.m_relativePath = "FileA.txt";
            entry.m_sourceFileInfo.m_scanFolder = scanFolderInfo;
            AssetProcessor::JobDetails jobDetails;
            jobDetails.m_watchFolder = scanFolderInfo->ScanPath();
            jobDetails.m_jobEntry.m_builderGuid = builderUuid;
            jobDetails.m_jobEntry.m_absolutePathToFile = sourceFileAPath;
            jobDetails.m_jobEntry.m_relativePathToFile = "FileA.txt";
            entry.m_jobsToAnalyze.push_back(jobDetails);
            apm.UNITTEST_UpdateSourceFileDependencyInfo();
            apm.UNITTEST_UpdateSourceFileDependencyDatabase();
            apm.UNITTEST_AnalyzeJobDetail(entry);
            AZStd::vector<SourceFileDependencyInternal>& sourceFileDependencyList = entry.m_jobsToAnalyze[0].m_sourceFileDependencyList;
            UNIT_TEST_EXPECT_TRUE(sourceFileDependencyList.size() == 3);
            UNIT_TEST_EXPECT_TRUE(sourceFileDependencyList[0].m_relativeSourceFileDependencyPath != sourceFileDependencyList[1].m_relativeSourceFileDependencyPath);
            UNIT_TEST_EXPECT_TRUE(sourceFileDependencyList[1].m_relativeSourceFileDependencyPath != sourceFileDependencyList[2].m_relativeSourceFileDependencyPath);
            UNIT_TEST_EXPECT_TRUE(sourceFileDependencyList[0].m_relativeSourceFileDependencyPath != sourceFileDependencyList[2].m_relativeSourceFileDependencyPath);
            UNIT_TEST_EXPECT_TRUE(sourceFileDependencyList[0].m_relativeSourceFileDependencyPath.compare("subfolder2/FileB.txt") == 0 || 
                                  sourceFileDependencyList[0].m_relativeSourceFileDependencyPath.compare("FileE.txt") == 0 || 
                                  sourceFileDependencyList[0].m_relativeSourceFileDependencyPath.compare("subfolder2/FileD.txt") == 0);
            UNIT_TEST_EXPECT_TRUE(sourceFileDependencyList[1].m_relativeSourceFileDependencyPath.compare("subfolder2/FileB.txt") == 0 ||
                                  sourceFileDependencyList[1].m_relativeSourceFileDependencyPath.compare("FileE.txt") == 0 ||
                                  sourceFileDependencyList[1].m_relativeSourceFileDependencyPath.compare("subfolder2/FileD.txt") == 0);
            UNIT_TEST_EXPECT_TRUE(sourceFileDependencyList[2].m_relativeSourceFileDependencyPath.compare("subfolder2/FileB.txt") == 0 ||
                                  sourceFileDependencyList[2].m_relativeSourceFileDependencyPath.compare("FileE.txt") == 0 ||
                                  sourceFileDependencyList[2].m_relativeSourceFileDependencyPath.compare("subfolder2/FileD.txt") == 0);
        }

        apm.UNITTEST_ClearSourceFileDependencyInternalMaps();

        {
            // here we are again processing FileA
            // since FileA depends on FileB.txt and FileE.And FileE depends on FileD. We are adding another dependency to FileD i.e FileD depends on FileA.
            // This can result on circular dependency but when we process FileA we should see only the following dependencies FileB, FileE and FileD. 
            sourceFiles.clear();
            AssetBuilderSDK::CreateJobsRequest jobRequest(builderUuid, "subfolder2/FileD.txt", tempPath.filePath("subfolder1").toUtf8().data(), PCInfos, fileDUuid);
            AssetBuilderSDK::CreateJobsResponse jobResponse;
            AssetBuilderSDK::SourceFileDependency sourceFileDependency;
            sourceFileDependency.m_sourceFileDependencyPath = "FileA.txt";
            jobResponse.m_sourceFileDependencyList.push_back(sourceFileDependency);
            apm.UNITTEST_ProcessCreateJobsResponse(jobResponse, jobRequest);

            AssetProcessorManager::JobToProcessEntry entry;
            entry.m_sourceFileInfo.m_relativePath = "FileA.txt";
            entry.m_sourceFileInfo.m_scanFolder = scanFolderInfo;
            AssetProcessor::JobDetails jobDetails;
            jobDetails.m_watchFolder = scanFolderInfo->ScanPath();
            jobDetails.m_jobEntry.m_builderGuid = builderUuid;
            jobDetails.m_jobEntry.m_absolutePathToFile = sourceFileAPath;
            jobDetails.m_jobEntry.m_relativePathToFile = "FileA.txt";
            entry.m_jobsToAnalyze.push_back(jobDetails);
            apm.UNITTEST_UpdateSourceFileDependencyInfo();
            apm.UNITTEST_UpdateSourceFileDependencyDatabase();
            apm.UNITTEST_AnalyzeJobDetail(entry);
            AZStd::vector<SourceFileDependencyInternal>& sourceFileDependencyList = entry.m_jobsToAnalyze[0].m_sourceFileDependencyList;
            UNIT_TEST_EXPECT_TRUE(sourceFileDependencyList.size() == 3);
            UNIT_TEST_EXPECT_TRUE(sourceFileDependencyList[0].m_relativeSourceFileDependencyPath != sourceFileDependencyList[1].m_relativeSourceFileDependencyPath);
            UNIT_TEST_EXPECT_TRUE(sourceFileDependencyList[1].m_relativeSourceFileDependencyPath != sourceFileDependencyList[2].m_relativeSourceFileDependencyPath);
            UNIT_TEST_EXPECT_TRUE(sourceFileDependencyList[0].m_relativeSourceFileDependencyPath != sourceFileDependencyList[2].m_relativeSourceFileDependencyPath);
            UNIT_TEST_EXPECT_TRUE(sourceFileDependencyList[0].m_relativeSourceFileDependencyPath.compare("subfolder2/FileB.txt") == 0 || 
                                  sourceFileDependencyList[0].m_relativeSourceFileDependencyPath.compare("FileE.txt") == 0 || 
                                  sourceFileDependencyList[0].m_relativeSourceFileDependencyPath.compare("subfolder2/FileD.txt") == 0);
            UNIT_TEST_EXPECT_TRUE(sourceFileDependencyList[1].m_relativeSourceFileDependencyPath.compare("subfolder2/FileB.txt") == 0 ||
                                  sourceFileDependencyList[1].m_relativeSourceFileDependencyPath.compare("FileE.txt") == 0 ||
                                  sourceFileDependencyList[1].m_relativeSourceFileDependencyPath.compare("subfolder2/FileD.txt") == 0);
            UNIT_TEST_EXPECT_TRUE(sourceFileDependencyList[2].m_relativeSourceFileDependencyPath.compare("subfolder2/FileB.txt") == 0 ||
                                  sourceFileDependencyList[2].m_relativeSourceFileDependencyPath.compare("FileE.txt") == 0 ||
                                  sourceFileDependencyList[2].m_relativeSourceFileDependencyPath.compare("subfolder2/FileD.txt") == 0);
        }

        apm.UNITTEST_ClearSourceFileDependencyInternalMaps();

        auto checkDep = [](const AZStd::vector<SourceFileDependencyInternal>& sourceFileDependencyList, const AZStd::string& file) -> bool
        {
            for (const auto& entry : sourceFileDependencyList)
            {
                if (entry.m_relativeSourceFileDependencyPath.compare(file) == 0)
                {
                    return true;
                }
            }

            return false;
        };

        AZ::Uuid fileFUuid = AssetUtilities::CreateSafeSourceUUIDFromName("FileF.txt");

        {
            // Process a file that outputs 2 dependencies
            sourceFiles.clear();
            AssetBuilderSDK::CreateJobsRequest jobRequest(builderUuid, "FileF.txt", tempPath.filePath("subfolder1").toUtf8().data(), PCInfos, fileFUuid);
            AssetBuilderSDK::CreateJobsResponse jobResponse;
            AssetBuilderSDK::SourceFileDependency sourceFileDependency;
            sourceFileDependency.m_sourceFileDependencyPath = "FileG.txt";
            jobResponse.m_sourceFileDependencyList.push_back(sourceFileDependency);
            AssetBuilderSDK::SourceFileDependency sourceFileDependency2;
            sourceFileDependency2.m_sourceFileDependencyPath = "FileH.txt";
            jobResponse.m_sourceFileDependencyList.push_back(sourceFileDependency2);
            apm.UNITTEST_ProcessCreateJobsResponse(jobResponse, jobRequest);

            AssetProcessorManager::JobToProcessEntry entry;
            entry.m_sourceFileInfo.m_relativePath = "FileF.txt";
            entry.m_sourceFileInfo.m_scanFolder = scanFolderInfo;
            AssetProcessor::JobDetails jobDetails;
            jobDetails.m_watchFolder = scanFolderInfo->ScanPath();
            jobDetails.m_jobEntry.m_builderGuid = builderUuid;
            jobDetails.m_jobEntry.m_absolutePathToFile = sourceFileFPath;
            jobDetails.m_jobEntry.m_relativePathToFile = "FileF.txt";
            entry.m_jobsToAnalyze.push_back(jobDetails);
            apm.UNITTEST_UpdateSourceFileDependencyInfo();
            apm.UNITTEST_UpdateSourceFileDependencyDatabase();
            apm.UNITTEST_AnalyzeJobDetail(entry);
            AZStd::vector<SourceFileDependencyInternal>& sourceFileDependencyList = entry.m_jobsToAnalyze[0].m_sourceFileDependencyList;
            UNIT_TEST_EXPECT_TRUE(sourceFileDependencyList.size() == 2);
            UNIT_TEST_EXPECT_TRUE(checkDep(sourceFileDependencyList, "FileG.txt"));
            UNIT_TEST_EXPECT_TRUE(checkDep(sourceFileDependencyList, "FileH.txt"));
        }

        apm.UNITTEST_ClearSourceFileDependencyInternalMaps();

        {
            // Now process the same file again, but this time remove a dependency
            sourceFiles.clear();
            AssetBuilderSDK::CreateJobsRequest jobRequest(builderUuid, "FileF.txt", tempPath.filePath("subfolder1").toUtf8().data(), PCInfos, fileFUuid);
            AssetBuilderSDK::CreateJobsResponse jobResponse;
            AssetBuilderSDK::SourceFileDependency sourceFileDependency;
            sourceFileDependency.m_sourceFileDependencyPath = "FileG.txt";
            jobResponse.m_sourceFileDependencyList.push_back(sourceFileDependency);
            apm.UNITTEST_ProcessCreateJobsResponse(jobResponse, jobRequest);

            AssetProcessorManager::JobToProcessEntry entry;
            entry.m_sourceFileInfo.m_relativePath = "FileF.txt";
            entry.m_sourceFileInfo.m_scanFolder = scanFolderInfo;
            AssetProcessor::JobDetails jobDetails;
            jobDetails.m_watchFolder = scanFolderInfo->ScanPath();
            jobDetails.m_jobEntry.m_builderGuid = builderUuid;
            jobDetails.m_jobEntry.m_absolutePathToFile = sourceFileFPath;
            jobDetails.m_jobEntry.m_relativePathToFile = "FileF.txt";
            entry.m_jobsToAnalyze.push_back(jobDetails);
            apm.UNITTEST_UpdateSourceFileDependencyInfo();
            apm.UNITTEST_UpdateSourceFileDependencyDatabase();
            apm.UNITTEST_AnalyzeJobDetail(entry);
            AZStd::vector<SourceFileDependencyInternal>& sourceFileDependencyList = entry.m_jobsToAnalyze[0].m_sourceFileDependencyList;
            UNIT_TEST_EXPECT_TRUE(sourceFileDependencyList.size() == 1);
            UNIT_TEST_EXPECT_TRUE(checkDep(sourceFileDependencyList, "FileG.txt"));
        }

        apm.UNITTEST_ClearSourceFileDependencyInternalMaps();

        {
            // And again the same file, but with a different builder
            sourceFiles.clear();
            AssetBuilderSDK::CreateJobsRequest jobRequest(builder2Uuid, "FileF.txt", tempPath.filePath("subfolder1").toUtf8().data(), PCInfos, fileFUuid);
            AssetBuilderSDK::CreateJobsResponse jobResponse;
            AssetBuilderSDK::SourceFileDependency sourceFileDependency;
            sourceFileDependency.m_sourceFileDependencyPath = "FileH.txt";
            jobResponse.m_sourceFileDependencyList.push_back(sourceFileDependency);
            apm.UNITTEST_ProcessCreateJobsResponse(jobResponse, jobRequest);

            AssetProcessorManager::JobToProcessEntry entry;
            entry.m_sourceFileInfo.m_relativePath = "FileF.txt";
            entry.m_sourceFileInfo.m_scanFolder = scanFolderInfo;
            AssetProcessor::JobDetails jobDetails;
            jobDetails.m_watchFolder = scanFolderInfo->ScanPath();
            jobDetails.m_jobEntry.m_builderGuid = builder2Uuid;
            jobDetails.m_jobEntry.m_absolutePathToFile = sourceFileFPath;
            jobDetails.m_jobEntry.m_relativePathToFile = "FileF.txt";
            entry.m_jobsToAnalyze.push_back(jobDetails);
            apm.UNITTEST_UpdateSourceFileDependencyInfo();
            apm.UNITTEST_UpdateSourceFileDependencyDatabase();
            apm.UNITTEST_AnalyzeJobDetail(entry);
            AZStd::vector<SourceFileDependencyInternal>& sourceFileDependencyList = entry.m_jobsToAnalyze[0].m_sourceFileDependencyList;
            UNIT_TEST_EXPECT_TRUE(sourceFileDependencyList.size() == 1);
            UNIT_TEST_EXPECT_TRUE(checkDep(sourceFileDependencyList, "FileH.txt"));
        }

        {
            // Go back and make sure the original builder/file pair wasn't affected
            AssetProcessorManager::JobToProcessEntry entry;
            entry.m_sourceFileInfo.m_relativePath = "FileF.txt";
            entry.m_sourceFileInfo.m_scanFolder = scanFolderInfo;
            AssetProcessor::JobDetails jobDetails;
            jobDetails.m_watchFolder = scanFolderInfo->ScanPath();
            jobDetails.m_jobEntry.m_builderGuid = builderUuid;
            jobDetails.m_jobEntry.m_absolutePathToFile = sourceFileFPath;
            jobDetails.m_jobEntry.m_relativePathToFile = "FileF.txt";
            entry.m_jobsToAnalyze.push_back(jobDetails);
            apm.UNITTEST_UpdateSourceFileDependencyInfo();
            apm.UNITTEST_UpdateSourceFileDependencyDatabase();
            apm.UNITTEST_AnalyzeJobDetail(entry);
            AZStd::vector<SourceFileDependencyInternal>& sourceFileDependencyList = entry.m_jobsToAnalyze[0].m_sourceFileDependencyList;
            UNIT_TEST_EXPECT_TRUE(sourceFileDependencyList.size() == 1);
            UNIT_TEST_EXPECT_TRUE(checkDep(sourceFileDependencyList, "FileG.txt"));
        }

        Q_EMIT UnitTestPassed();
    }

    void AssetProcessorManagerUnitTests_CheckOutputFolders::StartTest()
    {
        MockAssetBuilderInfoHandler mockAssetBuilderInfoHandler;

        QDir oldRoot;
        AssetUtilities::ComputeAssetRoot(oldRoot);
        AssetUtilities::ResetAssetRoot();

        QTemporaryDir dir;
        UnitTestUtils::ScopedDir changeDir(dir.path());
        QDir tempPath(dir.path());

        CreateDummyFile(tempPath.absoluteFilePath("bootstrap.cfg"), QString("sys_game_folder=SamplesProject\n"));

        // system is already actually initialized, along with gEnv, so this will always return that game name.
        QString gameName = AssetUtilities::ComputeGameName();

        // update the engine root
        AssetUtilities::ResetAssetRoot();
        QDir newRoot;
        AssetUtilities::ComputeAssetRoot(newRoot, &tempPath);

        UNIT_TEST_EXPECT_FALSE(gameName.isEmpty());

        PlatformConfiguration config;
        config.EnablePlatform({ "pc",{ "desktop", "renderer" } }, true);
        AZStd::vector<AssetBuilderSDK::PlatformInfo> platforms;
        config.PopulatePlatformsForScanFolder(platforms);
        //                                               PATH                 DisplayName  PortKey       outputfolder  root recurse  platforms order

        // note: the crux of this test is that we ar redirecting output into the cache at a different location instead of default.
        // so our scan folder has a "redirected" folder.
        config.AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder1"), "subfolder1", "subfolder1", "redirected", false, true, platforms, -1)); 

        AssetProcessorManager_Test apm(&config);

        QDir cacheRoot;
        UNIT_TEST_EXPECT_TRUE(AssetUtilities::ComputeProjectCacheRoot(cacheRoot));

        QList<JobDetails> processResults;
        QList<QPair<QString, QString> > changedInputResults;
        QList<QPair<QString, AzFramework::AssetSystem::AssetNotificationMessage> > assetMessages;

        bool idling = false;

        connect(&apm, &AssetProcessorManager::AssetToProcess,
            this, [&processResults](JobDetails details)
        {
            processResults.push_back(AZStd::move(details));
        });

        connect(&apm, &AssetProcessorManager::AssetMessage,
            this, [&assetMessages](QString platform, AzFramework::AssetSystem::AssetNotificationMessage message)
        {
            assetMessages.push_back(qMakePair(platform, message));
        });

        connect(&apm, &AssetProcessorManager::InputAssetProcessed,
            this, [&changedInputResults](QString relativePath, QString platform)
        {
            changedInputResults.push_back(QPair<QString, QString>(relativePath, platform));
        });

        connect(&apm, &AssetProcessorManager::AssetProcessorManagerIdleState,
            this, [&idling](bool state)
        {
            idling = state;
        }
        );

        QString sourceFile = tempPath.absoluteFilePath("subfolder1/basefile.foo");
        CreateDummyFile(sourceFile);

        mockAssetBuilderInfoHandler.m_numberOfJobsToCreate = 1; //Create two jobs for this file

        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, sourceFile));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        // block until no more events trickle in:
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        UNIT_TEST_EXPECT_TRUE(processResults.size() == 1);
        UNIT_TEST_EXPECT_TRUE((processResults[0].m_jobEntry.m_platformInfo.m_identifier == "pc"));
        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_relativePathToFile.startsWith("redirected/basefile.foo"));

        QStringList pcouts;
        pcouts.push_back(cacheRoot.filePath(QString("pc/") + gameName + "/redirected/basefile.arc1"));

        // Create the product files for the first job 
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(pcouts[0], "product1"));

        // Invoke Asset Processed for pc platform for the first job
        AssetBuilderSDK::ProcessJobResponse response;
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcouts[0].toUtf8().constData()));

        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[0].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        // let events bubble through:
        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        UNIT_TEST_EXPECT_TRUE(assetMessages.size() == 1);
        UNIT_TEST_EXPECT_TRUE(changedInputResults.size() == 1);
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].first == "pc");
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].second.m_data == "redirected/basefile.arc1");
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].second.m_type == AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged);

        UNIT_TEST_EXPECT_TRUE(AssetUtilities::NormalizeFilePath(changedInputResults[0].first) == AssetUtilities::NormalizeFilePath(sourceFile));

        // ------------- TEST 1:   Modified source file

        processResults.clear();
        pcouts.clear();
        assetMessages.clear();
        changedInputResults.clear();

        // now, the test is set up.  we can now start poking that file and make sure we emit the appropriate messages.
        // first, lets modify the file and make sure we get the build request.
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(sourceFile, "new data!"));

        QMetaObject::invokeMethod(&apm, "AssessModifiedFile", Qt::QueuedConnection, Q_ARG(QString, sourceFile));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        // block until no more events trickle in:
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        UNIT_TEST_EXPECT_TRUE(processResults.size() == 1);
        UNIT_TEST_EXPECT_TRUE((processResults[0].m_jobEntry.m_platformInfo.m_identifier == "pc"));
        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_relativePathToFile.startsWith("redirected/basefile.foo"));
        
        pcouts.push_back(cacheRoot.filePath(QString("pc/") + gameName + "/redirected/basefile.arc1"));

        // Create the product files for the first job 
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(pcouts[0], "product1"));

        // Invoke Asset Processed for pc platform for the first job
        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcouts[0].toUtf8().constData()));

        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[0].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        // let events bubble through:
        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        UNIT_TEST_EXPECT_TRUE(assetMessages.size() == 1);
        UNIT_TEST_EXPECT_TRUE(changedInputResults.size() == 1);
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].first == "pc");
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].second.m_data == "redirected/basefile.arc1");
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].second.m_type == AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged);

        UNIT_TEST_EXPECT_TRUE(AssetUtilities::NormalizeFilePath(changedInputResults[0].first) == AssetUtilities::NormalizeFilePath(sourceFile));

        // ------------- TEST 2:   Deleted product

        processResults.clear();
        pcouts.clear();
        assetMessages.clear();
        changedInputResults.clear();

        QString deletedProductPath = cacheRoot.filePath(QString("pc/") + gameName + "/redirected/basefile.arc1");
        
        QFile::remove(deletedProductPath);

        QMetaObject::invokeMethod(&apm, "AssessDeletedFile", Qt::QueuedConnection, Q_ARG(QString, deletedProductPath));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        // block until no more events trickle in:
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        UNIT_TEST_EXPECT_TRUE(assetMessages.size() == 0); // asset removed is delayed until actual processing occurs.

        UNIT_TEST_EXPECT_TRUE(processResults.size() == 1);
        UNIT_TEST_EXPECT_TRUE((processResults[0].m_jobEntry.m_platformInfo.m_identifier == "pc"));
        UNIT_TEST_EXPECT_TRUE(processResults[0].m_jobEntry.m_relativePathToFile.startsWith("redirected/basefile.foo"));

        pcouts.push_back(cacheRoot.filePath(QString("pc/") + gameName + "/redirected/basefile.arc1"));

        // Create the product files for the first job 
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(pcouts[0], "product1"));

        // Invoke Asset Processed for pc platform for the first job
        response.m_outputProducts.clear();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct(pcouts[0].toUtf8().constData()));

        changedInputResults.clear();
        assetMessages.clear();

        QMetaObject::invokeMethod(&apm, "AssetProcessed", Qt::QueuedConnection, Q_ARG(JobEntry, processResults[0].m_jobEntry), Q_ARG(AssetBuilderSDK::ProcessJobResponse, response));

        // let events bubble through:
        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        UNIT_TEST_EXPECT_TRUE(assetMessages.size() == 1);
        UNIT_TEST_EXPECT_TRUE(changedInputResults.size() == 1);
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].first == "pc");
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].second.m_data == "redirected/basefile.arc1");
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].second.m_type == AzFramework::AssetSystem::AssetNotificationMessage::AssetChanged);

        UNIT_TEST_EXPECT_TRUE(AssetUtilities::NormalizeFilePath(changedInputResults[0].first) == AssetUtilities::NormalizeFilePath(sourceFile));

        // ------------- TEST 3:   Deleted source - the products should end up deleted!

        processResults.clear();
        pcouts.clear();
        assetMessages.clear();
        changedInputResults.clear();

        QString deletedSourcePath = sourceFile;
        QFile::remove(deletedSourcePath);
        QMetaObject::invokeMethod(&apm, "AssessDeletedFile", Qt::QueuedConnection, Q_ARG(QString, deletedSourcePath));

        UNIT_TEST_EXPECT_TRUE(BlockUntil(idling, 5000));

        // block until no more events trickle in:
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        UNIT_TEST_EXPECT_TRUE(assetMessages.size() == 1);
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].first == "pc");
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].second.m_data == "redirected/basefile.arc1");
        UNIT_TEST_EXPECT_TRUE(assetMessages[0].second.m_type == AzFramework::AssetSystem::AssetNotificationMessage::AssetRemoved);

        // also ensure file is actually gone!
        UNIT_TEST_EXPECT_TRUE(!QFile::exists(deletedProductPath));

        Q_EMIT UnitTestPassed();
    }

#include <native/unittests/AssetProcessorManagerUnitTests.moc>
} // namespace AssetProcessor

#endif



