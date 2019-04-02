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

#include "native/tests/AssetProcessorTest.h"

#include "native/utilities/assetUtils.h"

#include <QDir>
#include <QFileInfo>
#include <QDateTime>

using namespace AssetUtilities;

class AssetUtilitiesTest
    : public AssetProcessor::AssetProcessorTest
{
};

TEST_F(AssetUtilitiesTest, NormlizeFilePath_NormalizedValidPathRelPath_Valid)
{
    QString result = NormalizeFilePath("a/b\\c\\d/E.txt");
    EXPECT_STREQ(result.toUtf8().constData(), "a/b/c/d/E.txt");
}

TEST_F(AssetUtilitiesTest, NormlizeFilePath_NormalizedValidPathFullPath_Valid)
{
    QString result = NormalizeFilePath("c:\\a/b\\c\\d/E.txt");
    // on windows, drive letters are normalized to full
#if defined(AZ_PLATFORM_WINDOWS)
    ASSERT_TRUE(result.compare("C:/a/b/c/d/E.txt", Qt::CaseSensitive) == 0);
#else
    // on other platforms, C: is a relative path to a file called 'c:')
    EXPECT_STREQ(result.toUtf8().constData(), "c:/a/b/c/d/E.txt");
#endif
}

TEST_F(AssetUtilitiesTest, NormlizeFilePath_NormalizedValidDirRelPath_Valid)
{
    QString result = NormalizeDirectoryPath("a/b\\c\\D");
    EXPECT_STREQ(result.toUtf8().constData(), "a/b/c/D");
}

TEST_F(AssetUtilitiesTest, NormlizeFilePath_NormalizedValidDirFullPath_Valid)
{
    QString result = NormalizeDirectoryPath("c:\\a/b\\C\\d\\");

    // on windows, drive letters are normalized to full
#if defined(AZ_PLATFORM_WINDOWS)
    EXPECT_STREQ(result.toUtf8().constData(), "C:/a/b/C/d");
#else
    EXPECT_STREQ(result.toUtf8().constData(), "c:/a/b/C/d");
#endif
}

TEST_F(AssetUtilitiesTest, ComputeCRC32Lowercase_IsCaseInsensitive)
{
    const char* upperCaseString = "HELLOworld";
    const char* lowerCaseString = "helloworld";

    EXPECT_EQ(AssetUtilities::ComputeCRC32Lowercase(lowerCaseString), AssetUtilities::ComputeCRC32Lowercase(upperCaseString));

    // also try the length-based one.
    EXPECT_EQ(AssetUtilities::ComputeCRC32Lowercase(lowerCaseString, size_t(5)), AssetUtilities::ComputeCRC32Lowercase(upperCaseString, size_t(5)));
}

TEST_F(AssetUtilitiesTest, ComputeCRC32_IsCaseSensitive)
{
    const char* upperCaseString = "HELLOworld";
    const char* lowerCaseString = "helloworld";

    EXPECT_NE(AssetUtilities::ComputeCRC32(lowerCaseString), AssetUtilities::ComputeCRC32(upperCaseString));

    // also try the length-based one.
    EXPECT_NE(AssetUtilities::ComputeCRC32(lowerCaseString, size_t(5)), AssetUtilities::ComputeCRC32(upperCaseString, size_t(5)));
}


TEST_F(AssetUtilitiesTest, UpdateToCorrectCase_MissingFile_ReturnsFalse)
{
    QTemporaryDir dir;
    QDir tempPath(dir.path());
    QString canonicalTempDirPath = AssetUtilities::NormalizeDirectoryPath(tempPath.canonicalPath());
    UnitTestUtils::ScopedDir changeDir(canonicalTempDirPath);
    tempPath = QDir(canonicalTempDirPath);

    QString fileName = "someFile.txt";
    EXPECT_FALSE(AssetUtilities::UpdateToCorrectCase(canonicalTempDirPath, fileName));
}

TEST_F(AssetUtilitiesTest, UpdateToCorrectCase_ExistingFile_ReturnsTrue_CorrectsCase)
{
    QTemporaryDir dir;
    QDir tempPath(dir.path());
    QString canonicalTempDirPath = AssetUtilities::NormalizeDirectoryPath(tempPath.canonicalPath());
    UnitTestUtils::ScopedDir changeDir(canonicalTempDirPath);
    tempPath = QDir(canonicalTempDirPath);

    QStringList thingsToTry;
    thingsToTry << "SomeFile.TxT";
    thingsToTry << "otherfile.txt";
    thingsToTry << "subfolder1/otherfile.txt";
    thingsToTry << "subfolder2\\otherfile.txt";
    thingsToTry << "subFolder3\\somefile.txt";
    thingsToTry << "subFolder4\\subfolder6\\somefile.txt";
    thingsToTry << "subFolder5\\subfolder7/someFile.txt";
    thingsToTry << "specialFileName[.txt";
    thingsToTry << "specialFileName].txt";
    thingsToTry << "specialFileName!.txt";
    thingsToTry << "specialFileName#.txt";
    thingsToTry << "specialFileName$.txt";
    thingsToTry << "specialFile%Name%.txt";
    thingsToTry << "specialFileName&.txt";
    thingsToTry << "specialFileName(.txt";
    thingsToTry << "specialFileName+.txt";
    thingsToTry << "specialFileName[9].txt";
    thingsToTry << "specialFileName[A-Za-z].txt"; // these should all be treated as literally the name of the file, not a regex!
    
    for (QString triedThing : thingsToTry)
    {
        QString lowercaseVersion = triedThing;
        EXPECT_TRUE(UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath(triedThing)));

        lowercaseVersion = lowercaseVersion.toLower();
        // each one should be found.   If it fails, we'll pipe out the name of the file it fails on for extra context.
        
        EXPECT_TRUE(AssetUtilities::UpdateToCorrectCase(canonicalTempDirPath, lowercaseVersion)) << "File being Examined: " << lowercaseVersion.toUtf8().constData();
        // each one should correct, and return a normalized path.
        EXPECT_STREQ(lowercaseVersion.toUtf8().constData(), AssetUtilities::NormalizeFilePath(triedThing).toUtf8().constData());
    }
}

TEST_F(AssetUtilitiesTest, GenerateFingerprint_BasicTest)
{
    QTemporaryDir dir;
    QDir tempPath(dir.path());
    QString canonicalTempDirPath = AssetUtilities::NormalizeDirectoryPath(tempPath.canonicalPath());
    UnitTestUtils::ScopedDir changeDir(canonicalTempDirPath);
    tempPath = QDir(canonicalTempDirPath);
    QString absoluteTestFilePath1 = tempPath.absoluteFilePath("basicfile.txt");
    QString absoluteTestFilePath2 = tempPath.absoluteFilePath("basicfile2.txt");
    EXPECT_TRUE(UnitTestUtils::CreateDummyFile(absoluteTestFilePath1, "contents"));
    EXPECT_TRUE(UnitTestUtils::CreateDummyFile(absoluteTestFilePath2, "contents"));

    AZStd::string fileEncoded1 = absoluteTestFilePath1.toUtf8().constData();
    AZStd::string fileEncoded2 = absoluteTestFilePath2.toUtf8().constData();

    AssetProcessor::JobDetails jobDetail;
    // it is expected that the only parts of jobDetails that matter are:
    // jobDetail.m_extraInformationForFingerprinting
    // jobDetail.m_fingerprintFiles
    // jobDetail.m_jobDependencyList

    jobDetail.m_extraInformationForFingerprinting = "extra info1";
    // the fingerprint should always be stable over repeated runs, even with minimal info:
    unsigned int result1 = AssetUtilities::GenerateFingerprint(jobDetail);
    unsigned int result2 = AssetUtilities::GenerateFingerprint(jobDetail);
    EXPECT_EQ(result1, result2);

    // the fingerprint should always be different when anything changes:
    jobDetail.m_extraInformationForFingerprinting = "extra info1";
    result1 = AssetUtilities::GenerateFingerprint(jobDetail);
    jobDetail.m_extraInformationForFingerprinting = "extra info2";
    result2 = AssetUtilities::GenerateFingerprint(jobDetail);
    EXPECT_NE(result1, result2);

    jobDetail.m_fingerprintFiles.insert(AZStd::make_pair(absoluteTestFilePath1.toUtf8().constData(), "basicfile.txt"));
    result1 = AssetUtilities::GenerateFingerprint(jobDetail);
    result2 = AssetUtilities::GenerateFingerprint(jobDetail);
    EXPECT_EQ(result1, result2);

    // mutating the dependency list should mutate the fingerprint, even if the extra info doesn't change.
    jobDetail.m_fingerprintFiles.insert(AZStd::make_pair(absoluteTestFilePath2.toUtf8().constData(), "basicfile2.txt"));
    result2 = AssetUtilities::GenerateFingerprint(jobDetail);
    EXPECT_NE(result1, result2);

    UnitTestUtils::SleepForMinimumFileSystemTime();

    // mutating the actual files should mutate the fingerprint, even if the file list doesn't change.
    // note that both files are in the file list, so changing just the one should result in a change in hash:
    EXPECT_TRUE(UnitTestUtils::CreateDummyFile(absoluteTestFilePath1, "contents new"));
    result1 = AssetUtilities::GenerateFingerprint(jobDetail);
    EXPECT_NE(result1, result2);
    
    // changing the other should also change the hash:
    EXPECT_TRUE(UnitTestUtils::CreateDummyFile(absoluteTestFilePath2, "contents new2"));
    result2 = AssetUtilities::GenerateFingerprint(jobDetail);
    EXPECT_NE(result1, result2);
}


// there was a case (when CRC32 was used) where a chain of files which had similar modtimes would
// cause the CRC32 algorithm to fail.  This test verifies that it is not the case with the current hash algorithm:
TEST_F(AssetUtilitiesTest, GenerateFingerprint_ChainCollisionTest)
{
    // strategy:  
    // Create 'filesToTest' number of dummy files each named basicFileN.txt, where N is their index.
    // Interleaving their mod time so that even numbered files have the same mod time as other even numbered files
    // and odd numbered files have the same mod time as other odd numbered files, but the evens and odds do not have
    // the same mod time as each other (basically create at least 2 groups of files in terms of unique mod times).

    // Once created, we will throw these files into the fingerprint function in groups.
    // the group will range from a single file, to up to filesToGroupTogether files, in a sliding window which
    // covers the entire filesToTest Range with wrap around.
    // So, for example if there are 4 filesToTest, and filesToGroupTogether is 3, you would see the following groups
    // sent to the fingerprinting function (in terms of file indices)
    // [0], [0, 1], [0, 1, 2]
    // [1], [1, 2], [1, 2, 3]
    // [2], [2, 3], [2, 3, 0]  // wraparound here
    // [3], [3, 0], [3, 0, 1]
    // note that the above grouping ensures that the same exact set of files is never sent to the fingerprinting
    // function, and thus all fingerprints generated should be unique.

    const int filesToTest = 200; // make it an even number, this is total files to create
    const int filesToGroupTogether = 25; 
    QTemporaryDir dir;
    QDir tempPath(dir.path());
    QString canonicalTempDirPath = AssetUtilities::NormalizeDirectoryPath(tempPath.canonicalPath());
    UnitTestUtils::ScopedDir changeDir(canonicalTempDirPath);
    tempPath = QDir(canonicalTempDirPath);

    // note that we create two sets of files, attempting to make duplicate modtimes, interleaved odd and even.
    // first, we make evens as fast as possible - the goal is to make modtimes be the same for as many of these
    // files as possible:
    for (int fileIdx = 0; fileIdx < filesToTest / 2; ++fileIdx)
    {
        QString absoluteFilePath = tempPath.absoluteFilePath(QString("basicFile%1.txt").arg(fileIdx * 2));
        EXPECT_TRUE(UnitTestUtils::CreateDummyFile(absoluteFilePath, "contents"));
    }
    
    // we wait for at a little bit of time to pass:
    UnitTestUtils::SleepForMinimumFileSystemTime();

    // now make odds as fast as possible.
    for (int fileIdx = 0; fileIdx < filesToTest / 2; ++fileIdx)
    {
        QString absoluteFilePath = tempPath.absoluteFilePath(QString("basicFile%1.txt").arg((fileIdx * 2) + 1));
        EXPECT_TRUE(UnitTestUtils::CreateDummyFile(absoluteFilePath, "contents"));
    }

    // its very likely that some of the above files have the same modtime.  But verify that its true:
    AZStd::unordered_map<AZ::s64, int> modTimes;  // map of [modtime] -> [how many files had that same modtime]
    
    for (int idx = 0; idx < filesToTest; ++idx)
    {
        QString relPath = QString("basicFile%1.txt").arg(idx);
        QString absoluteFilePath = tempPath.absoluteFilePath(relPath);

        QFileInfo fi(absoluteFilePath);
        EXPECT_TRUE(fi.exists());
        AZ::s64 msecs = fi.lastModified().toMSecsSinceEpoch();

        // count how many files have the same modtime:
        modTimes[msecs] = modTimes[msecs] + 1;
    }

    EXPECT_GT(modTimes.size(), 1); // at least two groups of different modtimes
    EXPECT_LT(modTimes.size(), filesToTest); // at least SOME duplicate modtimes.

    AZStd::unordered_set<AZ::u32> fingerprintsSoFar;

    // the strategy here is to use a sliding window of the above 200 files
    // as the window slices from 0 to 200, we build a list of n files starting
    // at that position, with wrap-around.
    for (int slidingWindow = 0; slidingWindow < filesToTest; ++slidingWindow)
    {
        for (int numFilesToAddToList = 1; numFilesToAddToList < filesToGroupTogether; ++numFilesToAddToList)
        {
            int startFileIndex = slidingWindow;
            int endFileIndex = slidingWindow + numFilesToAddToList;

            AssetProcessor::JobDetails jobDetail;
            for (int fileIdx = startFileIndex; fileIdx < endFileIndex; ++fileIdx)
            {
                QString relPath = QString("basicFile%1.txt").arg(fileIdx % filesToTest); // wrap-around
                QString absoluteFilePath = tempPath.absoluteFilePath(relPath);
                jobDetail.m_fingerprintFiles.insert(AZStd::make_pair(absoluteFilePath.toUtf8().constData(), relPath.toUtf8().constData()));
            }

            // pair_iter_bool should always be true, meaning it is a new fingerprint not seen before.
            auto inserter = fingerprintsSoFar.insert(AssetUtilities::GenerateFingerprint(jobDetail));
            EXPECT_TRUE(inserter.second);
        }
    }
}

TEST_F(AssetUtilitiesTest, GetFileFingerprint_BasicTest)
{
    QTemporaryDir dir;
    QDir tempPath(dir.path());
    QString canonicalTempDirPath = AssetUtilities::NormalizeDirectoryPath(tempPath.canonicalPath());
    UnitTestUtils::ScopedDir changeDir(canonicalTempDirPath);
    tempPath = QDir(canonicalTempDirPath);
    QString absoluteTestFilePath1 = tempPath.absoluteFilePath("basicfile.txt");
    QString absoluteTestFilePath2 = tempPath.absoluteFilePath("basicfile2.txt");

    EXPECT_TRUE(UnitTestUtils::CreateDummyFile(absoluteTestFilePath1, "contents"));
    UnitTestUtils::SleepForMinimumFileSystemTime();
    EXPECT_TRUE(UnitTestUtils::CreateDummyFile(absoluteTestFilePath2, "contents2"));

    AZStd::string fileEncoded1 = absoluteTestFilePath1.toUtf8().constData();
    AZStd::string fileEncoded2 = absoluteTestFilePath2.toUtf8().constData();

    // repeatedly hashing the same file should result in the same hash:
    EXPECT_STREQ(AssetUtilities::GetFileFingerprint(fileEncoded1, "").c_str(), AssetUtilities::GetFileFingerprint(fileEncoded1, "").c_str());
    EXPECT_STREQ(AssetUtilities::GetFileFingerprint(fileEncoded1, "Name").c_str(), AssetUtilities::GetFileFingerprint(fileEncoded1, "Name").c_str());
    EXPECT_STREQ(AssetUtilities::GetFileFingerprint(fileEncoded2, "").c_str(), AssetUtilities::GetFileFingerprint(fileEncoded2, "").c_str());
    EXPECT_STREQ(AssetUtilities::GetFileFingerprint(fileEncoded2, "Name").c_str(), AssetUtilities::GetFileFingerprint(fileEncoded2, "Name").c_str());

    // mutating the 'name' should mutate the fingerprint:
    EXPECT_STRNE(AssetUtilities::GetFileFingerprint(fileEncoded1, "").c_str(), AssetUtilities::GetFileFingerprint(fileEncoded1, "Name").c_str());

    // two different files should not hash to the same fingerprint:
    EXPECT_STRNE(AssetUtilities::GetFileFingerprint(fileEncoded1, "").c_str(), AssetUtilities::GetFileFingerprint(fileEncoded2, "").c_str());

    UnitTestUtils::SleepForMinimumFileSystemTime();

    AZStd::string oldFingerprint1 = AssetUtilities::GetFileFingerprint(fileEncoded1, "");
    AZStd::string oldFingerprint2 = AssetUtilities::GetFileFingerprint(fileEncoded2, "");
    AZStd::string oldFingerprint1a = AssetUtilities::GetFileFingerprint(fileEncoded1, "Name1");
    AZStd::string oldFingerprint2a = AssetUtilities::GetFileFingerprint(fileEncoded2, "Name2");

    EXPECT_TRUE(UnitTestUtils::CreateDummyFile(absoluteTestFilePath1, "contents1a"));
    EXPECT_TRUE(UnitTestUtils::CreateDummyFile(absoluteTestFilePath2, "contents2a"));

    EXPECT_STRNE(oldFingerprint1.c_str(), AssetUtilities::GetFileFingerprint(fileEncoded1, "").c_str());
    EXPECT_STRNE(oldFingerprint2.c_str(), AssetUtilities::GetFileFingerprint(fileEncoded2, "").c_str());
    EXPECT_STRNE(oldFingerprint1a.c_str(), AssetUtilities::GetFileFingerprint(fileEncoded1, "Name1").c_str());
    EXPECT_STRNE(oldFingerprint2a.c_str(), AssetUtilities::GetFileFingerprint(fileEncoded2, "Name2").c_str());
}


TEST_F(AssetUtilitiesTest, GetFileFingerprint_NonExistentFiles)
{
    AZStd::string nonExistentFile1 = AZ::Uuid::CreateRandom().ToString<AZStd::string>() + ".txt";
    ASSERT_FALSE(QFileInfo::exists(nonExistentFile1.c_str()));

    EXPECT_STRNE(AssetUtilities::GetFileFingerprint(nonExistentFile1, "").c_str(), AssetUtilities::GetFileFingerprint(nonExistentFile1, "Name").c_str());
    EXPECT_STREQ(AssetUtilities::GetFileFingerprint(nonExistentFile1, "Name").c_str(), AssetUtilities::GetFileFingerprint(nonExistentFile1, "Name").c_str());
}