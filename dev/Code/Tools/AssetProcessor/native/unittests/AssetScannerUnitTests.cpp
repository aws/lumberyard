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

#ifdef UNIT_TEST

#include "AssetScannerUnitTests.h"
#include "native/AssetManager/assetScanner.h"
#include "native/utilities/PlatformConfiguration.h"
#include "native/utilities/assetUtils.h"
#include <QTemporaryDir>
#include <QFile>
#include <QDebug>
#include <QDir>
#include <QTextStream>
#include <QSet>
#include <QList>
#include <QCoreApplication>
#include <QTime>


using namespace UnitTestUtils;
using namespace AssetProcessor;

void AssetScannerUnitTest::StartTest()
{
    QTemporaryDir tempEngineRoot;
    QDir tempPath(tempEngineRoot.path());

    QSet<QString> expectedFiles;
    // set up some interesting files:
    expectedFiles << tempPath.absoluteFilePath("rootfile2.txt");
    expectedFiles << tempPath.absoluteFilePath("subfolder1/basefile.txt");
    expectedFiles << tempPath.absoluteFilePath("subfolder2/basefile.txt");
    expectedFiles << tempPath.absoluteFilePath("subfolder2/aaa/basefile.txt");
    expectedFiles << tempPath.absoluteFilePath("subfolder2/aaa/bbb/basefile.txt");
    expectedFiles << tempPath.absoluteFilePath("subfolder2/aaa/bbb/ccc/basefile.txt");
    expectedFiles << tempPath.absoluteFilePath("subfolder2/aaa/bbb/ccc/ddd/basefile.txt");
    expectedFiles << tempPath.absoluteFilePath("subfolder2/aaa/bbb/ccc/ddd/eee.fff.ggg/basefile.txt");//adding a folder name containing dots
    expectedFiles << tempPath.absoluteFilePath("subfolder2/aaa/bbb/ccc/ddd/eee.fff.ggg/basefile1.txt");
    expectedFiles << tempPath.absoluteFilePath("subfolder3/basefile.txt");
    expectedFiles << tempPath.absoluteFilePath("subfolder3/aaa/basefile.txt");
    expectedFiles << tempPath.absoluteFilePath("subfolder3/aaa/bbb/basefile.txt");
    expectedFiles << tempPath.absoluteFilePath("subfolder3/aaa/bbb/ccc/basefile.txt");
    expectedFiles << tempPath.absoluteFilePath("rootfile1.txt");

    for (const QString& expect : expectedFiles)
    {
        UNIT_TEST_EXPECT_TRUE(CreateDummyFile(expect));
    }

    // but we're going to not watch subfolder3 recursively, so... remove these files (even though they're on disk)
    // if this causes a failure it means that its ignoring the "do not recurse" flag:
    expectedFiles.remove(tempPath.absoluteFilePath("subfolder3/aaa/basefile.txt"));
    expectedFiles.remove(tempPath.absoluteFilePath("subfolder3/aaa/bbb/basefile.txt"));
    expectedFiles.remove(tempPath.absoluteFilePath("subfolder3/aaa/bbb/ccc/basefile.txt"));

    PlatformConfiguration config;
    AZStd::vector<AssetBuilderSDK::PlatformInfo> platforms;
    config.PopulatePlatformsForScanFolder(platforms);
    //                                               PATH               DisplayName  PortKey outputfolder  root recurse  platforms
    config.AddScanFolder(ScanFolderInfo(tempPath.absolutePath(),         "temp",       "ap1",   "",        true,  false, platforms));  // note:  "Recurse" set to false.
    config.AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder1"), "",           "ap2",   "",       false,  true,  platforms));
    config.AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder2"), "",           "ap3",   "",       false,  true,  platforms));
    config.AddScanFolder(ScanFolderInfo(tempPath.filePath("subfolder3"), "",           "ap4",   "",       false,  false, platforms)); // note:  "Recurse" set to false.
    AssetScanner scanner(&config);

    QList<QString> actuallyFound;

    bool doneScan = false;

    connect(&scanner, &AssetScanner::FileOfInterestFound, this, [&actuallyFound](QString file)
        {
            actuallyFound.append(file);
        }
        );

    connect(&scanner, &AssetScanner::AssetScanningStatusChanged, this, [&doneScan](AssetProcessor::AssetScanningStatus status)
        {
            if ((status == AssetProcessor::AssetScanningStatus::Completed) || (status == AssetProcessor::AssetScanningStatus::Stopped))
            {
                doneScan = true;
            }
        }
        );

    // this test makes sure that no files that should be missed were actually missed.
    // it makes sure that if a folder is added recursively, child files and folders are found
    // it makes sure that if a folder is added NON-recursively, child folder files are not found.

    scanner.StartScan();
    QTime nowTime;
    nowTime.start();
    while (!doneScan)
    {
        QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents, 100);

        if (nowTime.elapsed() > 10000)
        {
            break;
        }
    }

    UNIT_TEST_EXPECT_TRUE(doneScan);
    UNIT_TEST_EXPECT_TRUE(actuallyFound.count() == expectedFiles.count());

    for (const QString& search : actuallyFound)
    {
        UNIT_TEST_EXPECT_TRUE(expectedFiles.find(search) != expectedFiles.end());
    }

    Q_EMIT UnitTestPassed();
}

#include <native/unittests/AssetScannerUnitTests.moc>

REGISTER_UNIT_TEST(AssetScannerUnitTest)

#endif //UNIT_TEST
