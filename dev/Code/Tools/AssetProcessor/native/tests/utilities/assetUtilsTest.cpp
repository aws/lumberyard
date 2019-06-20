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
#include <native/utilities/AssetUtilEBusHelper.h>

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

TEST_F(AssetUtilitiesTest, GenerateFingerprint_Empty_Asserts)
{
    AssetProcessor::JobDetails jobDetail;
    AZ::u32 fingerprint = AssetUtilities::GenerateFingerprint(jobDetail);
    
    EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 1);
    m_errorAbsorber->Clear();
}

TEST_F(AssetUtilitiesTest, GenerateFingerprint_MissingFile_NotSameAsZeroByteFile)
{
    QTemporaryDir dir;
    QDir tempPath(dir.path());
    QString canonicalTempDirPath = AssetUtilities::NormalizeDirectoryPath(tempPath.canonicalPath());
    UnitTestUtils::ScopedDir changeDir(canonicalTempDirPath);
    tempPath = QDir(canonicalTempDirPath);
    QString absoluteTestFilePath1 = tempPath.absoluteFilePath("basicfile.txt");
    QString absoluteTestFilePath2 = tempPath.absoluteFilePath("basicfile2.txt");
    
    EXPECT_TRUE(UnitTestUtils::CreateDummyFile(absoluteTestFilePath1, "")); // empty file
    // note:  basicfile1 exists but is empty, whereas basicfile2, 3 are missing entirely.

    AssetProcessor::JobDetails jobDetail;
    jobDetail.m_fingerprintFiles.insert(AZStd::make_pair(tempPath.absoluteFilePath("basicfile.txt").toUtf8().constData(), "basicfile.txt"));
    AZ::u32 fingerprint1 = AssetUtilities::GenerateFingerprint(jobDetail);

    jobDetail.m_fingerprintFiles.clear();
    jobDetail.m_fingerprintFiles.insert(AZStd::make_pair(tempPath.absoluteFilePath("basicfile2.txt").toUtf8().constData(), "basicfile2.txt"));
    AZ::u32 fingerprint2 = AssetUtilities::GenerateFingerprint(jobDetail);

    EXPECT_NE(fingerprint1, fingerprint2);
}

TEST_F(AssetUtilitiesTest, GenerateFingerprint_MissingFile_NotSameAsOtherMissingFile)
{
    QTemporaryDir dir;
    QDir tempPath(dir.path());
    QString canonicalTempDirPath = AssetUtilities::NormalizeDirectoryPath(tempPath.canonicalPath());
    UnitTestUtils::ScopedDir changeDir(canonicalTempDirPath);
    tempPath = QDir(canonicalTempDirPath);
    QString absoluteTestFilePath1 = tempPath.absoluteFilePath("basicfile.txt");
    QString absoluteTestFilePath2 = tempPath.absoluteFilePath("basicfile2.txt");

    // we create no files on disk.

    AssetProcessor::JobDetails jobDetail;
    jobDetail.m_fingerprintFiles.insert(AZStd::make_pair(tempPath.absoluteFilePath("basicfile.txt").toUtf8().constData(), "basicfile.txt"));
    AZ::u32 fingerprint1 = AssetUtilities::GenerateFingerprint(jobDetail);

    jobDetail.m_fingerprintFiles.clear();
    jobDetail.m_fingerprintFiles.insert(AZStd::make_pair(tempPath.absoluteFilePath("basicfile2.txt").toUtf8().constData(), "basicfile2.txt"));
    AZ::u32 fingerprint2 = AssetUtilities::GenerateFingerprint(jobDetail);

    EXPECT_NE(fingerprint1, fingerprint2);
}

TEST_F(AssetUtilitiesTest, GenerateFingerprint_OneFile_Differs)
{
    // this test makes sure that changing each part of jobDetail relevant to fingerprints causes the resulting fingerprint to change.
    QTemporaryDir dir;
    QDir tempPath(dir.path());
    QString canonicalTempDirPath = AssetUtilities::NormalizeDirectoryPath(tempPath.canonicalPath());
    UnitTestUtils::ScopedDir changeDir(canonicalTempDirPath);
    tempPath = QDir(canonicalTempDirPath);
    QString absoluteTestFilePath1 = tempPath.absoluteFilePath("basicfile.txt");
    QString absoluteTestFilePath2 = tempPath.absoluteFilePath("basicfile2.txt");
    QString absoluteTestFilePath3 = tempPath.absoluteFilePath("basicfile3.txt");

    EXPECT_TRUE(UnitTestUtils::CreateDummyFile(absoluteTestFilePath1, "contents"));
    EXPECT_TRUE(UnitTestUtils::CreateDummyFile(absoluteTestFilePath2, "contents")); // same contents
    EXPECT_TRUE(UnitTestUtils::CreateDummyFile(absoluteTestFilePath3, "contents2")); // different contents

    AssetProcessor::JobDetails jobDetail;
    jobDetail.m_fingerprintFiles.insert(AZStd::make_pair(tempPath.absoluteFilePath("basicfile.txt").toUtf8().constData(), "basicfile.txt"));

    AZ::u32 fingerprint1 = AssetUtilities::GenerateFingerprint(jobDetail);
    
    jobDetail.m_fingerprintFiles.clear();
    jobDetail.m_fingerprintFiles.insert(AZStd::make_pair(tempPath.absoluteFilePath("basicfile2.txt").toUtf8().constData(), "basicfile2.txt"));
    AZ::u32 fingerprint2 = AssetUtilities::GenerateFingerprint(jobDetail);

    jobDetail.m_fingerprintFiles.clear();
    jobDetail.m_fingerprintFiles.insert(AZStd::make_pair(tempPath.absoluteFilePath("basicfile3.txt").toUtf8().constData(), "basicfile3.txt"));
    AZ::u32 fingerprint3 = AssetUtilities::GenerateFingerprint(jobDetail);

    jobDetail.m_fingerprintFiles.clear();
    jobDetail.m_fingerprintFiles.insert(AZStd::make_pair(tempPath.absoluteFilePath("basicfile.txt").toUtf8().constData(), "basicfile.txt"));
    EXPECT_EQ(AssetUtilities::GenerateFingerprint(jobDetail), fingerprint1);

    jobDetail.m_fingerprintFiles.clear();
    jobDetail.m_fingerprintFiles.insert(AZStd::make_pair(tempPath.absoluteFilePath("basicfile2.txt").toUtf8().constData(), "basicfile2.txt"));
    EXPECT_EQ(AssetUtilities::GenerateFingerprint(jobDetail), fingerprint2);

    jobDetail.m_fingerprintFiles.clear();
    jobDetail.m_fingerprintFiles.insert(AZStd::make_pair(tempPath.absoluteFilePath("basicfile3.txt").toUtf8().constData(), "basicfile3.txt"));
    EXPECT_EQ(AssetUtilities::GenerateFingerprint(jobDetail), fingerprint3);

    EXPECT_NE(fingerprint1, fingerprint2);
    EXPECT_NE(fingerprint2, fingerprint3);
    EXPECT_NE(fingerprint3, fingerprint1);
}

TEST_F(AssetUtilitiesTest, GenerateFingerprint_MultipleFile_Differs)
{
    // given multiple files, make sure that the fingerprint for multiple files differs from the one file (that each file is taken into account)
    QTemporaryDir dir;
    QDir tempPath(dir.path());
    QString canonicalTempDirPath = AssetUtilities::NormalizeDirectoryPath(tempPath.canonicalPath());
    UnitTestUtils::ScopedDir changeDir(canonicalTempDirPath);
    tempPath = QDir(canonicalTempDirPath);
    QString absoluteTestFilePath1 = tempPath.absoluteFilePath("basicfile.txt");
    QString absoluteTestFilePath2 = tempPath.absoluteFilePath("basicfile2.txt");
    QString absoluteTestFilePath3 = tempPath.absoluteFilePath("basicfile3.txt");

    EXPECT_TRUE(UnitTestUtils::CreateDummyFile(absoluteTestFilePath1, "contents"));
    EXPECT_TRUE(UnitTestUtils::CreateDummyFile(absoluteTestFilePath2, "contents")); // same contents
    EXPECT_TRUE(UnitTestUtils::CreateDummyFile(absoluteTestFilePath3, "contents2")); // different contents

    AssetProcessor::JobDetails jobDetail;
    jobDetail.m_fingerprintFiles.insert(AZStd::make_pair(tempPath.absoluteFilePath("basicfile.txt").toUtf8().constData(), "basicfile.txt"));
    AZ::u32 fingerprint1 = AssetUtilities::GenerateFingerprint(jobDetail);
    jobDetail.m_fingerprintFiles.insert(AZStd::make_pair(tempPath.absoluteFilePath("basicfile2.txt").toUtf8().constData(), "basicfile2.txt"));
    AZ::u32 fingerprint2 = AssetUtilities::GenerateFingerprint(jobDetail);
    jobDetail.m_fingerprintFiles.insert(AZStd::make_pair(tempPath.absoluteFilePath("basicfile3.txt").toUtf8().constData(), "basicfile3.txt"));
    AZ::u32 fingerprint3 = AssetUtilities::GenerateFingerprint(jobDetail);

    EXPECT_NE(fingerprint1, fingerprint2);
    EXPECT_NE(fingerprint2, fingerprint3);
    EXPECT_NE(fingerprint3, fingerprint1);

    jobDetail.m_fingerprintFiles.clear();
    jobDetail.m_fingerprintFiles.insert(AZStd::make_pair(tempPath.absoluteFilePath("basicfile.txt").toUtf8().constData(), "basicfile.txt"));
    jobDetail.m_fingerprintFiles.insert(AZStd::make_pair(tempPath.absoluteFilePath("basicfile2.txt").toUtf8().constData(), "basicfile2.txt"));
    fingerprint1 = AssetUtilities::GenerateFingerprint(jobDetail);
    
    jobDetail.m_fingerprintFiles.clear();
    jobDetail.m_fingerprintFiles.insert(AZStd::make_pair(tempPath.absoluteFilePath("basicfile2.txt").toUtf8().constData(), "basicfile2.txt"));
    jobDetail.m_fingerprintFiles.insert(AZStd::make_pair(tempPath.absoluteFilePath("basicfile3.txt").toUtf8().constData(), "basicfile3.txt"));
    fingerprint2 = AssetUtilities::GenerateFingerprint(jobDetail);

    jobDetail.m_fingerprintFiles.clear();
    jobDetail.m_fingerprintFiles.insert(AZStd::make_pair(tempPath.absoluteFilePath("basicfile1.txt").toUtf8().constData(), "basicfile1.txt"));
    jobDetail.m_fingerprintFiles.insert(AZStd::make_pair(tempPath.absoluteFilePath("basicfile3.txt").toUtf8().constData(), "basicfile3.txt"));
    fingerprint3 = AssetUtilities::GenerateFingerprint(jobDetail);

    EXPECT_NE(fingerprint1, fingerprint2);
    EXPECT_NE(fingerprint2, fingerprint3);
    EXPECT_NE(fingerprint3, fingerprint1);
}

namespace AssetUtilsTest
{
    class MockJobDependencyResponder : public AssetProcessor::ProcessingJobInfoBus::Handler
    {
    public:
        MOCK_METHOD1(GetJobFingerprint, AZ::u32(const AssetProcessor::JobIndentifier&));
    };
}

TEST_F(AssetUtilitiesTest, GenerateFingerprint_GivenJobDependencies_AffectsOutcome)
{
    using namespace testing;
    using ::testing::NiceMock;

    NiceMock<AssetUtilsTest::MockJobDependencyResponder> responder;

    responder.BusConnect();

    QTemporaryDir dir;
    QDir tempPath(dir.path());
    QString canonicalTempDirPath = AssetUtilities::NormalizeDirectoryPath(tempPath.canonicalPath());
    UnitTestUtils::ScopedDir changeDir(canonicalTempDirPath);
    tempPath = QDir(canonicalTempDirPath);
    QString absoluteTestFilePath1 = tempPath.absoluteFilePath("basicfile.txt");

    AssetProcessor::JobDetails jobDetail;
    jobDetail.m_fingerprintFiles.insert(AZStd::make_pair(tempPath.absoluteFilePath("basicfile.txt").toUtf8().constData(), "basicfile.txt"));
    AZ::u32 fingerprint1 = AssetUtilities::GenerateFingerprint(jobDetail);

    // add a job dependency - it should alter the fingerprint, even if the file does not exist.
    AssetBuilderSDK::JobDependency jobDep("thing", "pc", AssetBuilderSDK::JobDependencyType::Order, AssetBuilderSDK::SourceFileDependency("basicfile2.txt", AZ::Uuid::CreateNull()));
    AssetProcessor::JobDependencyInternal internalJobDep(jobDep);
    internalJobDep.m_builderUuidList.insert(AZ::Uuid::CreateRandom());
    jobDetail.m_jobDependencyList.push_back(internalJobDep);

    EXPECT_CALL(responder, GetJobFingerprint(_))
        .WillOnce( 
            Return(0x12341234));

    AZ::u32 fingerprint2 = AssetUtilities::GenerateFingerprint(jobDetail);

    // different job fingerprint -> different result
    EXPECT_CALL(responder, GetJobFingerprint(_))
        .WillOnce( 
            Return(0x11111111));

    AZ::u32 fingerprint3 = AssetUtilities::GenerateFingerprint(jobDetail);

    EXPECT_NE(fingerprint1, fingerprint2);
    EXPECT_NE(fingerprint2, fingerprint3);
    EXPECT_NE(fingerprint3, fingerprint1);
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