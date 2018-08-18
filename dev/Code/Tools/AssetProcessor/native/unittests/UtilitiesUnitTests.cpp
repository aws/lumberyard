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
#include "UtilitiesUnitTests.h"

#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>
#include "native/utilities/assetUtils.h"
#include "native/utilities/ByteArrayStream.h"
#include <AzCore/std/parallel/thread.h>
#include <AzFramework/Asset/AssetSystemComponent.h>
#include <AzFramework/Components/BootstrapReaderComponent.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include "native/assetprocessor.h"

#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QDir>
#include <QThread>
#include <QByteArray>

#if defined WIN32
#include <windows.h>
#else
#include <QFileInfo>
#include <QFile>
#endif

using namespace UnitTestUtils;
using namespace AssetUtilities;
using namespace AssetProcessor;

namespace AssetProcessor
{
    const char* const TEST_BOOTSTRAP_DATA =
        "sys_game_folder = TestProject                                                      \r\n\
        assets = pc                                                                         \r\n\
        -- ip and port of the asset processor.Only if you need to change defaults           \r\n\
        -- remote_ip = 127.0.0.1                                                            \r\n\
        windows_remote_ip = 127.0.0.7                                                       \r\n\
        remote_port = 45645                                                                 \r\n\
        assetProcessor_branch_token = 0xDD814240";

    // simple utility class to make sure threads join and don't cause asserts
    // if the unit test exits early.
    class AutoThreadJoiner final
    {
        public:
        explicit AutoThreadJoiner(AZStd::thread* ownershipTransferThread)
        {
            m_threadToOwn = ownershipTransferThread;
        }

        ~AutoThreadJoiner()
        {
            if (m_threadToOwn)
            {
                m_threadToOwn->join();
                delete m_threadToOwn;
            }
        }

        AZStd::thread* m_threadToOwn;
    };
}

REGISTER_UNIT_TEST(UtilitiesUnitTests)

void UtilitiesUnitTests::StartTest()
{
    using namespace AzToolsFramework::AssetDatabase;

    // do not change case
    // do not chop extension
    // do not make full path
    UNIT_TEST_EXPECT_TRUE(NormalizeFilePath("a/b\\c\\d/E.txt") == "a/b/c/d/E.txt");

    // do not erase full path
#if defined(AZ_PLATFORM_WINDOWS)
    UNIT_TEST_EXPECT_TRUE(NormalizeFilePath("c:\\a/b\\c\\d/E.txt") == "C:/a/b/c/d/E.txt");
#else
    UNIT_TEST_EXPECT_TRUE(NormalizeFilePath("c:\\a/b\\c\\d/E.txt") == "c:/a/b/c/d/E.txt");
#endif // defined(AZ_PLATFORM_WINDOWS)


    // same tests but for directories:
#if defined(AZ_PLATFORM_WINDOWS)
    UNIT_TEST_EXPECT_TRUE(NormalizeDirectoryPath("c:\\a/b\\c\\d") == "C:/a/b/c/d");
#else
    UNIT_TEST_EXPECT_TRUE(NormalizeDirectoryPath("c:\\a/b\\c\\d") == "c:/a/b/c/d");
#endif // defined(AZ_PLATFORM_WINDOWS)

    UNIT_TEST_EXPECT_TRUE(NormalizeDirectoryPath("a/b\\c\\d") == "a/b/c/d");

    // directories automatically chop slashes:
#if defined(AZ_PLATFORM_WINDOWS)
    UNIT_TEST_EXPECT_TRUE(NormalizeDirectoryPath("c:\\a/b\\c\\d\\") == "C:/a/b/c/d");
    UNIT_TEST_EXPECT_TRUE(NormalizeDirectoryPath("c:\\a/b\\c\\d//") == "C:/a/b/c/d");
#else
    UNIT_TEST_EXPECT_TRUE(NormalizeDirectoryPath("c:\\a/b\\c\\d\\") == "c:/a/b/c/d");
    UNIT_TEST_EXPECT_TRUE(NormalizeDirectoryPath("c:\\a/b\\c\\d//") == "c:/a/b/c/d");
#endif // defined(AZ_PLATFORM_WINDOWS)

    QTemporaryDir tempdir;
    QDir dir(tempdir.path());
    QString fileName (dir.filePath("test.txt"));
    CreateDummyFile(fileName);
#if defined WIN32
    DWORD fileAttributes = GetFileAttributesA(fileName.toUtf8());

    if (!(fileAttributes & FILE_ATTRIBUTE_READONLY))
    {
        //make the file Readonly
        if (!SetFileAttributesA(fileName.toUtf8(), fileAttributes | (FILE_ATTRIBUTE_READONLY)))
        {
            AZ_TracePrintf(AssetProcessor::DebugChannel, "Unable to change file attributes for the file: %s.\n", fileName.toUtf8().data());
        }
    }
#else
    QFileInfo fileInfo(fileName);
    if (fileInfo.permission(QFile::WriteUser))
    {
        //remove write user flag
        QFile::Permissions permissions = QFile::permissions(fileName) & ~(QFile::WriteUser);
        if (!QFile::setPermissions(fileName, permissions))
        {
            AZ_TracePrintf(AssetProcessor::DebugChannel, "Unable to change file attributes for the file: %s.\n", fileName.toUtf8().data());
        }
    }
#endif
    UNIT_TEST_EXPECT_TRUE(AssetUtilities::MakeFileWritable(fileName));

    // ------------- Test NormalizeAndRemoveAlias --------------

    UNIT_TEST_EXPECT_TRUE(AssetUtilities::NormalizeAndRemoveAlias("@test@\\my\\file.txt") == QString("my/file.txt"));
    UNIT_TEST_EXPECT_TRUE(AssetUtilities::NormalizeAndRemoveAlias("@test@my\\file.txt") == QString("my/file.txt"));
    UNIT_TEST_EXPECT_TRUE(AssetUtilities::NormalizeAndRemoveAlias("@TeSt@my\\file.txt") == QString("my/file.txt")); // case sensitivity test!


    //-----------------------Test CopyFileWithTimeout---------------------

    QString outputFileName(dir.filePath("test1.txt"));
    
    QFile inputFile(fileName);
    inputFile.open(QFile::WriteOnly);
    QFile outputFile(outputFileName);
    outputFile.open(QFile::WriteOnly);

#if defined(AZ_PLATFORM_WINDOWS)
    // this test is intentionally disabled on other platforms
    // because in general on other platforms its actually possible to delete and move
    // files out of the way even if they are currently opened for writing by a different
    // handle.
    //Trying to copy when the output file is open for reading should fail.
    {
        UnitTestUtils::AssertAbsorber absorb;
        UNIT_TEST_EXPECT_FALSE(CopyFileWithTimeout(fileName, outputFileName, 1));
        UNIT_TEST_EXPECT_TRUE(absorb.m_numWarningsAbsorbed == 2); // 2 for each fail
        //Trying to move when the output file is open for reading
        UNIT_TEST_EXPECT_FALSE(MoveFileWithTimeout(fileName, outputFileName, 1));
        UNIT_TEST_EXPECT_TRUE(absorb.m_numWarningsAbsorbed == 4);
    }
#endif // AZ_PLATFORM_WINDOWS ONLY

    inputFile.close();
    outputFile.close();

    //Trying to copy when the output file is not open
    UNIT_TEST_EXPECT_TRUE(CopyFileWithTimeout(fileName, outputFileName, 1));
    UNIT_TEST_EXPECT_TRUE(CopyFileWithTimeout(fileName, outputFileName, -1));//invalid timeout time
    // Trying to move when the output file is not open
    UNIT_TEST_EXPECT_TRUE(MoveFileWithTimeout(fileName, outputFileName, 1));
    UNIT_TEST_EXPECT_TRUE(MoveFileWithTimeout(outputFileName, fileName, 1));

    AZStd::atomic_bool setupDone{ false };
    AssetProcessor::AutoThreadJoiner joiner(new AZStd::thread(
        [&]()
        {
            //opening file
            outputFile.open(QFile::WriteOnly);
            setupDone = true;
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(1000));
            //closing file
            outputFile.close();
        }));

    while (!setupDone.load())
    {
        QThread::msleep(1);
    }

    UNIT_TEST_EXPECT_TRUE(outputFile.isOpen());

    //Trying to copy when the output file is open,but will close before the timeout inputted
    {
        UnitTestUtils::AssertAbsorber absorb;
        UNIT_TEST_EXPECT_TRUE(CopyFileWithTimeout(fileName, outputFileName, 3));
#if defined(AZ_PLATFORM_WINDOWS)
        // only windows has an issue with moving files out that are in use.
        // other platforms do so without issue.
        UNIT_TEST_EXPECT_TRUE(absorb.m_numWarningsAbsorbed > 0);
#endif // windows platform.
    }
  
    // ------------- Test CheckCanLock --------------
    {
        QTemporaryDir lockTestTempDir;
        QDir lockTestDir(lockTestTempDir.path());
        QString lockTestFileName(lockTestDir.filePath("lockTest.txt"));

        UNIT_TEST_EXPECT_FALSE(AssetUtilities::CheckCanLock(lockTestFileName));

        CreateDummyFile(lockTestFileName);
        UNIT_TEST_EXPECT_TRUE(AssetUtilities::CheckCanLock(lockTestFileName));

#if defined(AZ_PLATFORM_WINDOWS)
        // on windows, opening a file for reading locks it
        // but on other platforms, this is not the case.
        QFile lockTestFile(lockTestFileName);
        lockTestFile.open(QFile::ReadOnly);
#else // AZ_PLATFORM_WINDOWS
        int handle = open(lockTestFileName.toUtf8().constData(), O_RDONLY | O_EXLOCK | O_NONBLOCK);       
#endif
        UNIT_TEST_EXPECT_FALSE(AssetUtilities::CheckCanLock(lockTestFileName));
#if defined(AZ_PLATFORM_WINDOWS)
        lockTestFile.close();
#else
        if (handle != -1)
        {
            close(handle);
        }
#endif // windows/other platforms ifdef 
    }

    // ----------------- TEST BOOTSTRAP SCANNER ----------------
    {
        QTemporaryDir tempDir;
        QDir tempPath(dir.path());
        CreateDummyFile(tempPath.absoluteFilePath("bootstrap.cfg"), QString("sys_game_folder=correct --- this is a a comment \nasakhdsakjhdsakjshdkjh"));
        UNIT_TEST_EXPECT_TRUE(ComputeGameName(dir.path(), true) == "correct");
        CreateDummyFile(tempPath.absoluteFilePath("bootstrap.cfg"), QString("--sys_game_folder=notgood\n\t\t  \tsys_game_folder = correct -- this is a comment\nsome more file stuff"));
        UNIT_TEST_EXPECT_TRUE(ComputeGameName(dir.path(), true) == "correct");
        CreateDummyFile(tempPath.absoluteFilePath("bootstrap.cfg"), QString("\t    \t   \t sys_game_folder=correct --- this is a a comment \nasakhdsakjhdsakjshdkjh"));
        UNIT_TEST_EXPECT_TRUE(ComputeGameName(dir.path(), true) == "correct");
        CreateDummyFile(tempPath.absoluteFilePath("bootstrap.cfg"), QString("-- sys_game_folder=bad --- this is a a comment \n-- sys_game_folder=alsobad\nsys_game_folder=correct"));
        UNIT_TEST_EXPECT_TRUE(ComputeGameName(dir.path(), true) == "correct");
        CreateDummyFile(tempPath.absoluteFilePath("bootstrap.cfg"), QString("-- sys_game_folder=bad --- this is a a comment \n-- sys_game_folder=alsobad\nsys_game_folder\t=\tcorrect"));
        UNIT_TEST_EXPECT_TRUE(ComputeGameName(dir.path(), true) == "correct");
    }

    // --------------- TEST BYTEARRAYSTREAM

    {
        AssetProcessor::ByteArrayStream stream;
        UNIT_TEST_EXPECT_TRUE(stream.CanSeek());
        UNIT_TEST_EXPECT_TRUE(stream.IsOpen());
        UNIT_TEST_EXPECT_TRUE(stream.GetLength() == 0);
        UNIT_TEST_EXPECT_TRUE(stream.GetCurPos() == 0);
        char tempReadBuffer[24];
        azstrcpy(tempReadBuffer, 24, "This is a Test String");
        UNIT_TEST_EXPECT_TRUE(stream.Read(100, tempReadBuffer) == 0);

        // reserving does not alter the length.
        stream.Reserve(128);
        UNIT_TEST_EXPECT_TRUE(stream.GetLength() == 0);
        UNIT_TEST_EXPECT_TRUE(stream.Write(7, tempReadBuffer) == 7);
        UNIT_TEST_EXPECT_TRUE(stream.GetCurPos() == 7);
        UNIT_TEST_EXPECT_TRUE(stream.GetLength() == 7);
        UNIT_TEST_EXPECT_TRUE(memcmp(stream.GetArray().constData(), tempReadBuffer, 7) == 0);
        UNIT_TEST_EXPECT_TRUE(stream.Write(7, tempReadBuffer) == 7);
        UNIT_TEST_EXPECT_TRUE(stream.GetLength() == 14);
        UNIT_TEST_EXPECT_TRUE(stream.GetCurPos() == 14);
        UNIT_TEST_EXPECT_TRUE(memcmp(stream.GetArray().constData(), "This isThis is", 14) == 0);


        stream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN); // write at  begin, without overrunning
        UNIT_TEST_EXPECT_TRUE(stream.GetCurPos() == 0);
        UNIT_TEST_EXPECT_TRUE(stream.Write(4, "that") == 4);
        UNIT_TEST_EXPECT_TRUE(stream.GetLength() == 14);
        UNIT_TEST_EXPECT_TRUE(stream.GetCurPos() == 4);
        UNIT_TEST_EXPECT_TRUE(memcmp(stream.GetArray().constData(), "that isThis is", 14) == 0);

        stream.Seek(2, AZ::IO::GenericStream::ST_SEEK_CUR); // write in middle, without overrunning
        UNIT_TEST_EXPECT_TRUE(stream.GetCurPos() == 6);
        UNIT_TEST_EXPECT_TRUE(stream.Write(4, "1234") == 4);
        UNIT_TEST_EXPECT_TRUE(stream.GetLength() == 14);
        UNIT_TEST_EXPECT_TRUE(stream.GetCurPos() == 10);
        UNIT_TEST_EXPECT_TRUE(memcmp(stream.GetArray().constData(), "that i1234s is", 14) == 0);

        stream.Seek(-6, AZ::IO::GenericStream::ST_SEEK_END); // write in end, negative offset, without overrunning
        UNIT_TEST_EXPECT_TRUE(stream.GetCurPos() == 8);
        UNIT_TEST_EXPECT_TRUE(stream.Write(4, "5555") == 4);
        UNIT_TEST_EXPECT_TRUE(stream.GetCurPos() == 12);
        UNIT_TEST_EXPECT_TRUE(stream.GetLength() == 14);
        UNIT_TEST_EXPECT_TRUE(memcmp(stream.GetArray().constData(), "that i125555is", 14) == 0);

        stream.Seek(2, AZ::IO::GenericStream::ST_SEEK_BEGIN); // write at begin offset, with overrun:
        UNIT_TEST_EXPECT_TRUE(stream.GetCurPos() == 2);
        UNIT_TEST_EXPECT_TRUE(stream.GetLength() == 14);
        UNIT_TEST_EXPECT_TRUE(stream.Write(14, "xxxxxxxxxxxxxx") == 14);
        UNIT_TEST_EXPECT_TRUE(stream.GetLength() == 16);
        UNIT_TEST_EXPECT_TRUE(stream.GetCurPos() == 16);
        UNIT_TEST_EXPECT_TRUE(memcmp(stream.GetArray().constData(), "thxxxxxxxxxxxxxx", 16) == 0);

        stream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        stream.Seek(14, AZ::IO::GenericStream::ST_SEEK_CUR); // write in middle, with overrunning:
        UNIT_TEST_EXPECT_TRUE(stream.GetCurPos() == 14);
        UNIT_TEST_EXPECT_TRUE(stream.GetLength() == 16);
        UNIT_TEST_EXPECT_TRUE(stream.Write(4, "yyyy") == 4);
        UNIT_TEST_EXPECT_TRUE(stream.GetCurPos() == 18);
        UNIT_TEST_EXPECT_TRUE(stream.GetLength() == 18);
        UNIT_TEST_EXPECT_TRUE(memcmp(stream.GetArray().constData(), "thxxxxxxxxxxxxyyyy", 18) == 0);

        stream.Seek(-2, AZ::IO::GenericStream::ST_SEEK_END); // write in end, negative offset, with overrunning
        UNIT_TEST_EXPECT_TRUE(stream.GetCurPos() == 16);
        UNIT_TEST_EXPECT_TRUE(stream.GetLength() == 18);
        UNIT_TEST_EXPECT_TRUE(stream.Write(4, "ZZZZ") == 4);
        UNIT_TEST_EXPECT_TRUE(stream.GetCurPos() == 20);
        UNIT_TEST_EXPECT_TRUE(stream.GetLength() == 20);
        UNIT_TEST_EXPECT_TRUE(memcmp(stream.GetArray().constData(), "thxxxxxxxxxxxxyyZZZZ", 20) == 0);

        // read test.
        stream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        UNIT_TEST_EXPECT_TRUE(stream.Read(20, tempReadBuffer) == 20);
        UNIT_TEST_EXPECT_TRUE(memcmp(tempReadBuffer, "thxxxxxxxxxxxxyyZZZZ", 20) == 0);
        UNIT_TEST_EXPECT_TRUE(stream.Read(20, tempReadBuffer) == 0); // because its already at end.
        UNIT_TEST_EXPECT_TRUE(memcmp(tempReadBuffer, "thxxxxxxxxxxxxyyZZZZ", 20) == 0); // it should not have disturbed the buffer.
        stream.Seek(2, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        UNIT_TEST_EXPECT_TRUE(stream.Read(20, tempReadBuffer) == 18);
        UNIT_TEST_EXPECT_TRUE(memcmp(tempReadBuffer, "xxxxxxxxxxxxyyZZZZZZ", 20) == 0); // it should not have disturbed the buffer bits that it was not asked to touch.
    }

    // --------------- TEST FilePatternMatcher
    {
        const char* wildcardMatch[] = {
            "*.cfg",
            "*.txt",
            "abf*.llm"
            "sdf.c*",
            "a.bcd"
        };
        {
            AssetBuilderSDK::FilePatternMatcher extensionWildcardTest(AssetBuilderSDK::AssetBuilderPattern("*.cfg", AssetBuilderSDK::AssetBuilderPattern::Wildcard));
            UNIT_TEST_EXPECT_TRUE(extensionWildcardTest.MatchesPath(AZStd::string("foo.cfg")));
            UNIT_TEST_EXPECT_TRUE(extensionWildcardTest.MatchesPath(AZStd::string("abcd/foo.cfg")));
            UNIT_TEST_EXPECT_FALSE(extensionWildcardTest.MatchesPath(AZStd::string("abcd/foo.cfd")));
        }

        {
            AssetBuilderSDK::FilePatternMatcher prefixWildcardTest(AssetBuilderSDK::AssetBuilderPattern("abf*.llm", AssetBuilderSDK::AssetBuilderPattern::Wildcard));
            UNIT_TEST_EXPECT_TRUE(prefixWildcardTest.MatchesPath(AZStd::string("abf.llm")));
            UNIT_TEST_EXPECT_TRUE(prefixWildcardTest.MatchesPath(AZStd::string("abf12345.llm")));
            UNIT_TEST_EXPECT_FALSE(prefixWildcardTest.MatchesPath(AZStd::string("foo/abf12345.llm")));
            UNIT_TEST_EXPECT_FALSE(prefixWildcardTest.MatchesPath(AZStd::string("foo/abf12345.lls")));
            UNIT_TEST_EXPECT_FALSE(prefixWildcardTest.MatchesPath(AZStd::string("foo/ab2345.llm")));
        }

        {
            AssetBuilderSDK::FilePatternMatcher extensionPrefixWildcardTest(AssetBuilderSDK::AssetBuilderPattern("sdf.c*", AssetBuilderSDK::AssetBuilderPattern::Wildcard));
            UNIT_TEST_EXPECT_TRUE(extensionPrefixWildcardTest.MatchesPath(AZStd::string("sdf.cpp")));
            UNIT_TEST_EXPECT_TRUE(extensionPrefixWildcardTest.MatchesPath(AZStd::string("sdf.cxx")));
            UNIT_TEST_EXPECT_TRUE(extensionPrefixWildcardTest.MatchesPath(AZStd::string("sdf.c")));
            UNIT_TEST_EXPECT_FALSE(extensionPrefixWildcardTest.MatchesPath(AZStd::string("abcd/sdf.cpp")));
            UNIT_TEST_EXPECT_FALSE(extensionPrefixWildcardTest.MatchesPath(AZStd::string("s:\\asd/abcd/sdf.cpp")));
            UNIT_TEST_EXPECT_FALSE(extensionPrefixWildcardTest.MatchesPath(AZStd::string("sdc.c")));
            UNIT_TEST_EXPECT_FALSE(extensionPrefixWildcardTest.MatchesPath(AZStd::string("sdf.hxx")));
            UNIT_TEST_EXPECT_FALSE(extensionPrefixWildcardTest.MatchesPath(AZStd::string("s:\\asd/abcd/sdf.hxx")));
        }

        {
            AssetBuilderSDK::FilePatternMatcher prefixExtensionPrefixWildcardTest(AssetBuilderSDK::AssetBuilderPattern("s*.c*", AssetBuilderSDK::AssetBuilderPattern::Wildcard));
            UNIT_TEST_EXPECT_TRUE(prefixExtensionPrefixWildcardTest.MatchesPath(AZStd::string("sdf.cpp")));
            UNIT_TEST_EXPECT_TRUE(prefixExtensionPrefixWildcardTest.MatchesPath(AZStd::string("sdf.cxx")));
            UNIT_TEST_EXPECT_FALSE(prefixExtensionPrefixWildcardTest.MatchesPath(AZStd::string("abcd/sdf.cpp")));
            UNIT_TEST_EXPECT_FALSE(prefixExtensionPrefixWildcardTest.MatchesPath(AZStd::string("c:\\asd/abcd/sdf.cpp")));
            UNIT_TEST_EXPECT_FALSE(prefixExtensionPrefixWildcardTest.MatchesPath(AZStd::string("sdf.hxx")));
            UNIT_TEST_EXPECT_FALSE(prefixExtensionPrefixWildcardTest.MatchesPath(AZStd::string("s:\\asd/abcd/sdf.hxx")));
        }

        {
            AssetBuilderSDK::FilePatternMatcher fixedNameTest(AssetBuilderSDK::AssetBuilderPattern("a.bcd", AssetBuilderSDK::AssetBuilderPattern::Wildcard));
            UNIT_TEST_EXPECT_TRUE(fixedNameTest.MatchesPath(AZStd::string("a.bcd")));
            UNIT_TEST_EXPECT_FALSE(fixedNameTest.MatchesPath(AZStd::string("foo\\a.bcd")));
            UNIT_TEST_EXPECT_FALSE(fixedNameTest.MatchesPath(AZStd::string("foo/a.bcd")));
            UNIT_TEST_EXPECT_FALSE(fixedNameTest.MatchesPath(AZStd::string("c:/foo/a.bcd")));
            UNIT_TEST_EXPECT_FALSE(fixedNameTest.MatchesPath(AZStd::string("c:\\foo/a.bcd")));
            UNIT_TEST_EXPECT_FALSE(fixedNameTest.MatchesPath(AZStd::string("sdf.hxx")));
        }

        {
            AssetBuilderSDK::FilePatternMatcher midMatchExtensionPrefixTest(AssetBuilderSDK::AssetBuilderPattern("s*f.c*", AssetBuilderSDK::AssetBuilderPattern::Wildcard));
            UNIT_TEST_EXPECT_TRUE(midMatchExtensionPrefixTest.MatchesPath(AZStd::string("sdf.cpp")));
            UNIT_TEST_EXPECT_TRUE(midMatchExtensionPrefixTest.MatchesPath(AZStd::string("sef.cxx")));
            UNIT_TEST_EXPECT_TRUE(midMatchExtensionPrefixTest.MatchesPath(AZStd::string("sf.c")));
            UNIT_TEST_EXPECT_FALSE(midMatchExtensionPrefixTest.MatchesPath(AZStd::string("c:\\asd/abcd/sdf.cpp")));
            UNIT_TEST_EXPECT_FALSE(midMatchExtensionPrefixTest.MatchesPath(AZStd::string("abcd/sdf.cpp")));
            UNIT_TEST_EXPECT_FALSE(midMatchExtensionPrefixTest.MatchesPath(AZStd::string("sdc.c")));
            UNIT_TEST_EXPECT_FALSE(midMatchExtensionPrefixTest.MatchesPath(AZStd::string("sdf.hxx")));
            UNIT_TEST_EXPECT_FALSE(midMatchExtensionPrefixTest.MatchesPath(AZStd::string("s:\\asd/abcd/sdf.hxx")));
        }

        {
            AssetBuilderSDK::FilePatternMatcher subFolderExtensionWildcardTest(AssetBuilderSDK::AssetBuilderPattern("abcd/*.cfg", AssetBuilderSDK::AssetBuilderPattern::Wildcard));
            UNIT_TEST_EXPECT_TRUE(subFolderExtensionWildcardTest.MatchesPath(AZStd::string("abcd/sdf.cfg")));
            UNIT_TEST_EXPECT_FALSE(subFolderExtensionWildcardTest.MatchesPath(AZStd::string("c://abcd/sdf.cfg")));
            UNIT_TEST_EXPECT_FALSE(subFolderExtensionWildcardTest.MatchesPath(AZStd::string("sdf.cfg")));
            UNIT_TEST_EXPECT_FALSE(subFolderExtensionWildcardTest.MatchesPath(AZStd::string("abcs/sdf.cfg")));
            UNIT_TEST_EXPECT_FALSE(subFolderExtensionWildcardTest.MatchesPath(AZStd::string("abcd/sdf.cfx")));
        }

        {
            AssetBuilderSDK::FilePatternMatcher subFolderPatternTest(AssetBuilderSDK::AssetBuilderPattern(".*\\/savebackup\\/.*", AssetBuilderSDK::AssetBuilderPattern::Regex));
            UNIT_TEST_EXPECT_TRUE(subFolderPatternTest.MatchesPath(AZStd::string("abcd/savebackup/sdf.cfg")));
            UNIT_TEST_EXPECT_FALSE(subFolderPatternTest.MatchesPath(AZStd::string("abcd/savebackup")));
            UNIT_TEST_EXPECT_FALSE(subFolderPatternTest.MatchesPath(AZStd::string("savebackup/sdf.cfg")));
            UNIT_TEST_EXPECT_FALSE(subFolderPatternTest.MatchesPath(AZStd::string("c://abcd/sdf.cfg")));
            UNIT_TEST_EXPECT_FALSE(subFolderPatternTest.MatchesPath(AZStd::string("sdf.cfg")));
            UNIT_TEST_EXPECT_FALSE(subFolderPatternTest.MatchesPath(AZStd::string("abcs/sdf.cfg")));
            UNIT_TEST_EXPECT_FALSE(subFolderPatternTest.MatchesPath(AZStd::string("abcd/sdf.cfx")));
        }

        {
            AssetBuilderSDK::FilePatternMatcher subFolderPatternTest(AssetBuilderSDK::AssetBuilderPattern(".*\\/Presets\\/GeomCache\\/.*", AssetBuilderSDK::AssetBuilderPattern::Regex));
            UNIT_TEST_EXPECT_TRUE(subFolderPatternTest.MatchesPath(AZStd::string("something/Presets/GeomCache/sdf.cfg")));
            UNIT_TEST_EXPECT_FALSE(subFolderPatternTest.MatchesPath(AZStd::string("Presets/GeomCache/sdf.cfg"))); // should not match because it demands that there is a slash
            UNIT_TEST_EXPECT_FALSE(subFolderPatternTest.MatchesPath(AZStd::string("abcd/savebackup")));
            UNIT_TEST_EXPECT_FALSE(subFolderPatternTest.MatchesPath(AZStd::string("savebackup/sdf.cfg")));
            UNIT_TEST_EXPECT_FALSE(subFolderPatternTest.MatchesPath(AZStd::string("c://abcd/sdf.cfg")));
            UNIT_TEST_EXPECT_FALSE(subFolderPatternTest.MatchesPath(AZStd::string("sdf.cfg")));
            UNIT_TEST_EXPECT_FALSE(subFolderPatternTest.MatchesPath(AZStd::string("abcs/sdf.cfg")));
            UNIT_TEST_EXPECT_FALSE(subFolderPatternTest.MatchesPath(AZStd::string("abcd/sdf.cfx")));
        }
    }

    {
        AZ::Entity* entity;
        EBUS_EVENT_RESULT(entity, AZ::ComponentApplicationBus, FindEntity, AZ::SystemEntityId);
        if (entity)
        {
            if (auto bootstrapComponent = entity->FindComponent<AzFramework::BootstrapReaderComponent>())
            {
                // Reset bootstrap file contents
                bootstrapComponent->OverrideFileContents(AssetProcessor::TEST_BOOTSTRAP_DATA);
            }

            if (auto assetSystemComponent = entity->FindComponent<AzFramework::AssetSystem::AssetSystemComponent>())
            {
                // Unload and reload settings, and test
                assetSystemComponent->Deactivate();
                assetSystemComponent->Activate();

                UNIT_TEST_EXPECT_TRUE(assetSystemComponent->AssetProcessorPlatform().compare("pc") == 0);
                UNIT_TEST_EXPECT_TRUE(assetSystemComponent->AssetProcessorBranchToken().compare("0xDD814240") == 0);
                UNIT_TEST_EXPECT_TRUE(assetSystemComponent->AssetProcessorPort() == 45645);
                UNIT_TEST_EXPECT_TRUE(assetSystemComponent->AssetProcessorConnectionAddress().compare("127.0.0.7") == 0);
            }
        }
    }
    Q_EMIT UnitTestPassed();
}


