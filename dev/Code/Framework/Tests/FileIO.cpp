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

#include <Tests/TestTypes.h>

#include <AzCore/IO/FileIO.h>
#include <AzCore/std/string/string.h>
#include <AzCore/PlatformIncl.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/IO/FileOperations.h>
#include <time.h>

#ifdef AZ_PLATFORM_WINDOWS
#include <sys/stat.h>
#include <io.h>
#endif // AZ_PLATFORM_WINDOWS

using namespace AZ;
using namespace AZ::IO;
using namespace AZ::Debug;

namespace PathUtil
{
    AZStd::string AddSlash(const AZStd::string& path)
    {
        if (path.empty() || path[path.length() - 1] == '/')
        {
            return path;
        }
        if (path[path.length() - 1] == '\\')
        {
            return path.substr(0, path.length() - 1) + "/";
        }
        return path + "/";
    }
}

namespace UnitTest
{
    class NameMatchesFilterTest
        : public AllocatorsFixture
    {
    public:
        void run()
        {
            AZ_TEST_ASSERT(NameMatchesFilter("hello", "hello") == true);
            AZ_TEST_ASSERT(NameMatchesFilter("hello", "he?l?") == true);
            AZ_TEST_ASSERT(NameMatchesFilter("hello", "he???") == true);
            AZ_TEST_ASSERT(NameMatchesFilter("hello", "he*") == true);
            AZ_TEST_ASSERT(NameMatchesFilter("hello", "he*o") == true);

            AZ_TEST_ASSERT(NameMatchesFilter("hello", "?*?o") == true);
            AZ_TEST_ASSERT(NameMatchesFilter("hello", "h?*?") == true);
            AZ_TEST_ASSERT(NameMatchesFilter("hello", "h?*?o") == true);
            AZ_TEST_ASSERT(NameMatchesFilter("hello", "h?*?o?") == false);
            AZ_TEST_ASSERT(NameMatchesFilter("hello", "h***o*") == true);
            AZ_TEST_ASSERT(NameMatchesFilter("something", "some??") == false);

            AZ_TEST_ASSERT(NameMatchesFilter("hello", "?????*") == true);
            AZ_TEST_ASSERT(NameMatchesFilter("hello", "????*") == true);

            AZ_TEST_ASSERT(NameMatchesFilter("hello", "h??*") == true);
            AZ_TEST_ASSERT(NameMatchesFilter("hello", "??L*") == true);

            AZ_TEST_ASSERT(NameMatchesFilter("anything", "*.*") == true);
            AZ_TEST_ASSERT(NameMatchesFilter("any.thing", "*") == true);
            AZ_TEST_ASSERT(NameMatchesFilter("anything", "") == true);
            AZ_TEST_ASSERT(NameMatchesFilter("anything", nullptr) == true);
            AZ_TEST_ASSERT(NameMatchesFilter("system.pak", "*.pak") == true);
            AZ_TEST_ASSERT(NameMatchesFilter("system.pakx", "*.pak") == false);
            AZ_TEST_ASSERT(NameMatchesFilter("system.pa", "*.pak") == false);
            AZ_TEST_ASSERT(NameMatchesFilter("system.pak.3", "*.pak.*") == true);
            AZ_TEST_ASSERT(NameMatchesFilter("system.pa.pak", "*.pak") == true);
            AZ_TEST_ASSERT(NameMatchesFilter("log1234.log", "log????.log") == true);
            AZ_TEST_ASSERT(NameMatchesFilter("log1234.log", "log?????.log") == false);
            AZ_TEST_ASSERT(NameMatchesFilter("log151234.log", "log*.log") == true);
            AZ_TEST_ASSERT(NameMatchesFilter(".pak", "*.pak") == true);
            AZ_TEST_ASSERT(NameMatchesFilter("", "*.pak") == false);
            AZ_TEST_ASSERT(NameMatchesFilter(nullptr, nullptr) == false);
            AZ_TEST_ASSERT(NameMatchesFilter(nullptr, "*.pak") == false);
            AZ_TEST_ASSERT(NameMatchesFilter("anything", nullptr) == true);
            AZ_TEST_ASSERT(NameMatchesFilter("test.test", "????.????") == true);
            AZ_TEST_ASSERT(NameMatchesFilter("testatest", "????.????") == false);
        }
    };

    TEST_F(NameMatchesFilterTest, Test)
    {
        run();
    }

    /**
     * FileIOStream test
     */
    class FileIOStreamTest
        : public AllocatorsFixture
    {
    public:
        AZ::IO::LocalFileIO m_fileIO;
        AZ::IO::FileIOBase* m_prevFileIO;

        FileIOStreamTest()
        {
        }

        void SetUp() override
        {
            AllocatorsFixture::SetUp();
            m_prevFileIO = AZ::IO::FileIOBase::GetInstance();
            AZ::IO::FileIOBase::SetInstance(&m_fileIO);
        }

        ~FileIOStreamTest()
        {
        }

        void TearDown() override
        {
            AZ::IO::FileIOBase::SetInstance(m_prevFileIO);
            AllocatorsFixture::TearDown();
        }

        void run()
        {
            FileIOStream stream("./fileiotest.txt", AZ::IO::OpenMode::ModeWrite);
            AZ_TEST_ASSERT(stream.IsOpen());
            char output[256];
            azsnprintf(output, sizeof(output), "magic string");
            AZ_TEST_ASSERT(strlen(output) + 1 == stream.Write(strlen(output) + 1, output));
            stream.Close();

            stream.Open("./fileiotest.txt", AZ::IO::OpenMode::ModeRead);
            AZ_TEST_ASSERT(stream.IsOpen());
            AZ_TEST_ASSERT(strlen(output) + 1 == stream.Read(strlen(output) + 1, output));
            AZ_TEST_ASSERT(strcmp(output, "magic string") == 0);
            stream.Close();
        }
    };

    TEST_F(FileIOStreamTest, Test)
    {
        run();
    }

    namespace LocalFileIOTest
    {
        class FolderFixture
            : public AllocatorsFixture
        {
        public:
            AZStd::string m_root;
            AZStd::string folderName;
            AZStd::string deepFolder;
            AZStd::string extraFolder;

            AZStd::string fileRoot;
            AZStd::string file01Name;
            AZStd::string file02Name;
            AZStd::string file03Name;
            int m_randomFolderKey = 0;

            FolderFixture()
            {
            }


            void ChooseRandomFolder()
            {
                char currentDir[AZ_MAX_PATH_LEN];
#if AZ_TRAIT_USE_WINDOWS_FILE_API
                GetCurrentDirectoryA(AZ_MAX_PATH_LEN, currentDir);
#else
                getcwd(currentDir, AZ_MAX_PATH_LEN);
#endif

                folderName = currentDir;
                folderName.append("/temp");
                m_root = folderName;
                if (folderName.size() > 0)
                {
                    folderName = PathUtil::AddSlash(folderName);
                }

                AZStd::string tempName = AZStd::string::format("tmp%08x", m_randomFolderKey);
                folderName.append(tempName.c_str());
                folderName = PathUtil::AddSlash(folderName);
                AZStd::replace(folderName.begin(), folderName.end(), '\\', '/');

                // Make sure the drive letter is capitalized
                if (folderName.size() > 2)
                {
                    if (folderName[1] == ':')
                    {
                        folderName[0] = static_cast<char>(toupper(folderName[0]));
                    }
                }

                deepFolder = folderName;
                deepFolder.append("test");

                deepFolder = PathUtil::AddSlash(deepFolder);
                deepFolder.append("subdir");

                extraFolder = deepFolder;
                extraFolder = PathUtil::AddSlash(extraFolder);
                extraFolder.append("subdir2");

                // make a couple files there, and in the root:
                fileRoot = PathUtil::AddSlash(extraFolder);
            }

            void SetUp() override
            {
                AllocatorsFixture::SetUp();

                // lets use a random temp folder name
                srand(clock());
                m_randomFolderKey = rand();

                LocalFileIO local;
                do
                {
                    ChooseRandomFolder();
                    ++m_randomFolderKey;
                } while (local.IsDirectory(fileRoot.c_str()));

                file01Name = fileRoot + "file01.txt";
                file02Name = fileRoot + "file02.asdf";
                file03Name = fileRoot + "test123.wha";
            }
        
            void TearDown() override
            {
                if ((!folderName.empty())&&(strstr(folderName.c_str(), "/temp") != nullptr))
                {
                    // cleanup!
                    LocalFileIO local;
                    local.DestroyPath(folderName.c_str());
                }
                AllocatorsFixture::TearDown();
            }
            void CreateTestFiles()
            {
                LocalFileIO local;
                AZ_TEST_ASSERT(local.CreatePath(fileRoot.c_str()));
                AZ_TEST_ASSERT(local.IsDirectory(fileRoot.c_str()));
                for (const AZStd::string& filename : { file01Name, file02Name, file03Name })
                {
#ifdef AZ_COMPILER_MSVC
                    FILE* tempFile;
                    fopen_s(&tempFile, filename.c_str(), "wb");
#else
                    FILE* tempFile = fopen(filename.c_str(), "wb");
#endif
                    fwrite("this is just a test", 1, 19, tempFile);
                    fclose(tempFile);
                }
            }
        };

        class DirectoryTest
            : public FolderFixture
        {
        public:
            void run()
            {
                LocalFileIO local;

                AZ_TEST_ASSERT(!local.Exists(folderName.c_str()));

                AZStd::string longPathCreateTest = folderName;
                longPathCreateTest.append("one");
                longPathCreateTest = PathUtil::AddSlash(longPathCreateTest);
                longPathCreateTest.append("two");
                longPathCreateTest = PathUtil::AddSlash(longPathCreateTest);
                longPathCreateTest.append("three");

                AZ_TEST_ASSERT(!local.Exists(longPathCreateTest.c_str()));
                AZ_TEST_ASSERT(!local.IsDirectory(longPathCreateTest.c_str()));
                AZ_TEST_ASSERT(local.CreatePath(longPathCreateTest.c_str()));
                AZ_TEST_ASSERT(local.IsDirectory(longPathCreateTest.c_str()));

                AZ_TEST_ASSERT(!local.Exists(deepFolder.c_str()));
                AZ_TEST_ASSERT(!local.IsDirectory(deepFolder.c_str()));
                AZ_TEST_ASSERT(local.CreatePath(deepFolder.c_str()));
                AZ_TEST_ASSERT(local.IsDirectory(deepFolder.c_str()));

                AZ_TEST_ASSERT(local.Exists(deepFolder.c_str()));
                AZ_TEST_ASSERT(local.CreatePath(deepFolder.c_str()));
                AZ_TEST_ASSERT(local.Exists(deepFolder.c_str()));
            }
        };

        TEST_F(DirectoryTest, Test)
        {
            run();
        }

        class ReadWriteTest
            : public FolderFixture
        {
        public:
            void run()
            {
                LocalFileIO local;

                AZ_TEST_ASSERT(!local.Exists(fileRoot.c_str()));
                AZ_TEST_ASSERT(!local.IsDirectory(fileRoot.c_str()));
                AZ_TEST_ASSERT(local.CreatePath(fileRoot.c_str()));
                AZ_TEST_ASSERT(local.IsDirectory(fileRoot.c_str()));

                FILE* tempFile = nullptr;
                azfopen(&tempFile, file01Name.c_str(), "wb");

                fwrite("this is just a test", 1, 19, tempFile);
                fclose(tempFile);

                AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
                AZ_TEST_ASSERT(!local.Open("", AZ::IO::OpenMode::ModeWrite, fileHandle));
                AZ_TEST_ASSERT(fileHandle == AZ::IO::InvalidHandle);

                // test size without opening:
                AZ::u64 fs = 0;
                AZ_TEST_ASSERT(local.Size(file01Name.c_str(), fs));
                AZ_TEST_ASSERT(fs == 19);

                fileHandle = AZ::IO::InvalidHandle;

                AZ::u64 modTimeA = local.ModificationTime(file01Name.c_str());
                AZ_TEST_ASSERT(modTimeA != 0);

                // test invalid handle ops:
                AZ_TEST_ASSERT(!local.Seek(fileHandle, 0, AZ::IO::SeekType::SeekFromStart));
                AZ_TEST_ASSERT(!local.Close(fileHandle));
                AZ_TEST_ASSERT(!local.Eof(fileHandle));
                AZ_TEST_ASSERT(!local.Flush(fileHandle));
                AZ_TEST_ASSERT(!local.ModificationTime(fileHandle));
                AZ_TEST_ASSERT(!local.Read(fileHandle, 0, 0, false));
                AZ_TEST_ASSERT(!local.Tell(fileHandle, fs));

                AZ_TEST_ASSERT(!local.Exists((file01Name + "notexist").c_str()));

                AZ_TEST_ASSERT(local.Exists(file01Name.c_str()));
                AZ_TEST_ASSERT(!local.IsReadOnly(file01Name.c_str()));
                AZ_TEST_ASSERT(!local.IsDirectory(file01Name.c_str()));

                // test reads and seeks.
                AZ_TEST_ASSERT(local.Open(file01Name.c_str(), AZ::IO::OpenMode::ModeRead, fileHandle));
                AZ_TEST_ASSERT(fileHandle != AZ::IO::InvalidHandle);

                // use this again later...
                AZ::u64 modTimeB = local.ModificationTime(fileHandle);
                AZ_TEST_ASSERT(modTimeB != 0);

                static const size_t testStringLen = 256;
                char testString[testStringLen] = { 0 };

                // test size on open handle:
                fs = 0;
                AZ_TEST_ASSERT(local.Size(fileHandle, fs));
                AZ_TEST_ASSERT(fs == 19);

                // test size without opening, after its already open:
                fs = 0;
                AZ_TEST_ASSERT(local.Size(file01Name.c_str(), fs));
                AZ_TEST_ASSERT(fs == 19);

                AZ::u64 offs = 0;
                AZ_TEST_ASSERT(local.Tell(fileHandle, offs));
                AZ_TEST_ASSERT(offs == 0);
                AZ_TEST_ASSERT(local.Seek(fileHandle, 5, AZ::IO::SeekType::SeekFromStart));
                AZ_TEST_ASSERT(!local.Eof(fileHandle));
                AZ::u64 actualBytesRead = 0;
                // situation
                // this is just a test
                //      ^-------------
                // 15 chars
                AZ_TEST_ASSERT(local.Tell(fileHandle, offs));
                AZ_TEST_ASSERT(offs == 5);
                AZ_TEST_ASSERT(!local.Eof(fileHandle));
                AZ_TEST_ASSERT(local.Read(fileHandle, testString, testStringLen, false, &actualBytesRead));
                AZ_TEST_ASSERT(actualBytesRead == 14);
                AZ_TEST_ASSERT(strncmp(testString, "is just a test", 14) == 0);
                AZ_TEST_ASSERT(local.Eof(fileHandle));

                // this is just a test
                //                    ^
                AZ_TEST_ASSERT(local.Seek(fileHandle, -5, AZ::IO::SeekType::SeekFromCurrent));
                // this is just a test
                //               ^----
                AZ_TEST_ASSERT(local.Tell(fileHandle, offs));
                AZ_TEST_ASSERT(offs == 14);
                AZ_TEST_ASSERT(!local.Eof(fileHandle));
                AZ_TEST_ASSERT(local.Read(fileHandle, testString, testStringLen, false, &actualBytesRead));
                AZ_TEST_ASSERT(actualBytesRead == 5);
                AZ_TEST_ASSERT(strncmp(testString, " test", 5) == 0);
                AZ_TEST_ASSERT(local.Eof(fileHandle));
                // this is just a test
                //                    ^
                AZ_TEST_ASSERT(local.Seek(fileHandle, -6, AZ::IO::SeekType::SeekFromEnd));
                // this is just a test
                //              ^---
                AZ_TEST_ASSERT(local.Tell(fileHandle, offs));
                AZ_TEST_ASSERT(offs == 13);
                AZ_TEST_ASSERT(!local.Eof(fileHandle));
                AZ_TEST_ASSERT(local.Read(fileHandle, testString, 4, true, &actualBytesRead));
                AZ_TEST_ASSERT(actualBytesRead == 4);
                AZ_TEST_ASSERT(strncmp(testString, "a te", 4) == 0);
                AZ_TEST_ASSERT(local.Tell(fileHandle, offs));
                AZ_TEST_ASSERT(offs == 17);
                AZ_TEST_ASSERT(!local.Eof(fileHandle));

                // fail when not enough bytes:
                AZ_TEST_ASSERT(!local.Read(fileHandle, testString, testStringLen, true, &actualBytesRead));
                AZ_TEST_ASSERT(local.Eof(fileHandle));
                AZ_TEST_ASSERT(local.Close(fileHandle));
                fileHandle = AZ::IO::InvalidHandle;
            }
        };

        TEST_F(ReadWriteTest, Test)
        {
            run();
        }

        class PermissionsTest
            : public FolderFixture
        {
        public:
            void run()
            {
                LocalFileIO local;

                CreateTestFiles();

#if !defined(AZ_PLATFORM_ANDROID) // CHMOD never works, ever, on android due to security constraints on internal storage.  You'd need root.

#ifdef AZ_PLATFORM_WINDOWS
                _chmod(file01Name.c_str(), _S_IREAD);
#else
                chmod(file01Name.c_str(), S_IRUSR | S_IRGRP | S_IROTH);
#endif // windows

                AZ_TEST_ASSERT(local.IsReadOnly(file01Name.c_str()));

#ifdef AZ_PLATFORM_WINDOWS
                _chmod(file01Name.c_str(), _S_IREAD | _S_IWRITE);
#else
                chmod(file01Name.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
#endif // windows

#endif // CHMOD not working on android

                AZ_TEST_ASSERT(!local.IsReadOnly(file01Name.c_str()));
            }
        };

        TEST_F(PermissionsTest, Test)
        {
            run();
        }

        class CopyMoveTests
            : public FolderFixture
        {
        public:
            void run()
            {
                LocalFileIO local;
                
                AZ_TEST_ASSERT(local.CreatePath(fileRoot.c_str()));
                AZ_TEST_ASSERT(local.IsDirectory(fileRoot.c_str()));
                {
#ifdef AZ_COMPILER_MSVC
                    FILE* tempFile;
                    fopen_s(&tempFile, file01Name.c_str(), "wb");
#else
                    FILE* tempFile = fopen(file01Name.c_str(), "wb");
#endif
                    fwrite("this is just a test", 1, 19, tempFile);
                    fclose(tempFile);
                }

                // make sure attributes are copied (such as modtime) even if they're copied:
                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(1500));
                AZ_TEST_ASSERT(local.Copy(file01Name.c_str(), file02Name.c_str()));
                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(1500));
                AZ_TEST_ASSERT(local.Copy(file01Name.c_str(), file03Name.c_str()));

                AZ_TEST_ASSERT(local.Exists(file01Name.c_str()));
                AZ_TEST_ASSERT(local.Exists(file02Name.c_str()));
                AZ_TEST_ASSERT(local.Exists(file03Name.c_str()));
                AZ_TEST_ASSERT(!local.DestroyPath(file01Name.c_str())); // you may not destroy files.
                AZ_TEST_ASSERT(!local.DestroyPath(file02Name.c_str()));
                AZ_TEST_ASSERT(!local.DestroyPath(file03Name.c_str()));
                AZ_TEST_ASSERT(local.Exists(file01Name.c_str()));
                AZ_TEST_ASSERT(local.Exists(file02Name.c_str()));
                AZ_TEST_ASSERT(local.Exists(file03Name.c_str()));

                AZ::u64 f1s = 0;
                AZ::u64 f2s = 0;
                AZ::u64 f3s = 0;
                AZ_TEST_ASSERT(local.Size(file01Name.c_str(), f1s));
                AZ_TEST_ASSERT(local.Size(file02Name.c_str(), f2s));
                AZ_TEST_ASSERT(local.Size(file03Name.c_str(), f3s));
                AZ_TEST_ASSERT(f1s == f2s);
                AZ_TEST_ASSERT(f1s == f3s);
            }
        };

        TEST_F(CopyMoveTests, Test)
        {
            run();
        }

        class ModTimeTest
            : public FolderFixture
        {
        public:
            void run()
            {
                AZ::IO::LocalFileIO local;

                CreateTestFiles();

                AZ::u64 modTimeC = 0;
                AZ::u64 modTimeD = 0;
                modTimeC = local.ModificationTime(file02Name.c_str());
                modTimeD = local.ModificationTime(file03Name.c_str());

                // make sure modtimes are in ascending order (at least)
                AZ_TEST_ASSERT(modTimeD >= modTimeC);

                // now touch some of the files.   This is also how we test append mode, and write mode.
                AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
                AZ_TEST_ASSERT(local.Open(file02Name.c_str(), AZ::IO::OpenMode::ModeAppend | AZ::IO::OpenMode::ModeBinary, fileHandle));
                AZ_TEST_ASSERT(fileHandle != AZ::IO::InvalidHandle);
                AZ_TEST_ASSERT(local.Write(fileHandle, "more", 4));
                AZ_TEST_ASSERT(local.Close(fileHandle));

                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(1500));
                // No-append-mode
                AZ_TEST_ASSERT(local.Open(file03Name.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeBinary, fileHandle));
                AZ_TEST_ASSERT(fileHandle != AZ::IO::InvalidHandle);
                AZ_TEST_ASSERT(local.Write(fileHandle, "more", 4));
                AZ_TEST_ASSERT(local.Close(fileHandle));

                modTimeC = local.ModificationTime(file02Name.c_str());
                modTimeD = local.ModificationTime(file03Name.c_str());

                AZ_TEST_ASSERT(modTimeD > modTimeC);

                AZ::u64 f1s = 0;
                AZ::u64 f2s = 0;
                AZ::u64 f3s = 0;
                AZ_TEST_ASSERT(local.Size(file01Name.c_str(), f1s));
                AZ_TEST_ASSERT(local.Size(file02Name.c_str(), f2s));
                AZ_TEST_ASSERT(local.Size(file03Name.c_str(), f3s));
                AZ_TEST_ASSERT(f2s == f1s + 4);
                AZ_TEST_ASSERT(f3s == 4);
            }
        };

        TEST_F(ModTimeTest, Test)
        {
            run();
        }

        class FindFilesTest
            : public FolderFixture
        {
        public:
            void run()
            {
                AZ::IO::LocalFileIO local;

                CreateTestFiles();

                AZStd::vector<AZStd::string> resultFiles;
                bool foundOK = local.FindFiles(fileRoot.c_str(), "*.*",
                        [&](const char* filePath) -> bool
                        {
                            resultFiles.push_back(filePath);
                            return false; // early out!
                        });

                AZ_TEST_ASSERT(foundOK);
                AZ_TEST_ASSERT(resultFiles.size() == 1);

                resultFiles.clear();

                foundOK = local.FindFiles(fileRoot.c_str(), "*.*",
                        [&](const char* filePath) -> bool
                        {
                            resultFiles.push_back(filePath);
                            return true; // continue iterating
                        });

                AZ_TEST_ASSERT(foundOK);
                AZ_TEST_ASSERT(resultFiles.size() == 3);

                // note: following tests accumulate more files without clearing resultfiles.
                foundOK = local.FindFiles(fileRoot.c_str(), "*.txt",
                        [&](const char* filePath) -> bool
                        {
                            resultFiles.push_back(filePath);
                            return true; // continue iterating
                        });

                AZ_TEST_ASSERT(foundOK);
                AZ_TEST_ASSERT(resultFiles.size() == 4);

                foundOK = local.FindFiles(fileRoot.c_str(), "file*.asdf",
                        [&](const char* filePath) -> bool
                        {
                            resultFiles.push_back(filePath);
                            return true; // continue iterating
                        });

                AZ_TEST_ASSERT(foundOK);
                AZ_TEST_ASSERT(resultFiles.size() == 5);

                foundOK = local.FindFiles(fileRoot.c_str(), "asaf.asdf",
                        [&](const char* filePath) -> bool
                        {
                            resultFiles.push_back(filePath);
                            return true; // continue iterating
                        });

                AZ_TEST_ASSERT(foundOK);
                AZ_TEST_ASSERT(resultFiles.size() == 5);

                resultFiles.clear();

                // test to make sure directories show up:
                foundOK = local.FindFiles(deepFolder.c_str(), "*.*",
                        [&](const char* filePath) -> bool
                        {
                            resultFiles.push_back(filePath);
                            return true; // continue iterating
                        });

                // canonicalize the name in the same way that find does.
                //AZStd::replace() extraFolder.replace('\\', '/');  FIXME PPATEL

                AZ_TEST_ASSERT(foundOK);
                AZ_TEST_ASSERT(resultFiles.size() == 1);
                AZ_TEST_ASSERT(resultFiles[0] == extraFolder);
                resultFiles.clear();
                foundOK = local.FindFiles("o:137787621!@#$%^&&**())_+[])_", "asaf.asdf",
                        [&](const char* filePath) -> bool
                        {
                            resultFiles.push_back(filePath);
                            return true; // continue iterating
                        });

                AZ_TEST_ASSERT(!foundOK);
                AZ_TEST_ASSERT(resultFiles.size() == 0);

                AZStd::string file04Name = fileRoot + "test.wha";
                // test rename
                AZ_TEST_ASSERT(local.Rename(file03Name.c_str(), file04Name.c_str()));
                AZ_TEST_ASSERT(!local.Rename(file03Name.c_str(), file04Name.c_str()));
                AZ_TEST_ASSERT(local.Rename(file04Name.c_str(), file04Name.c_str())); // this is valid and ok
                AZ_TEST_ASSERT(local.Exists(file04Name.c_str()));
                AZ_TEST_ASSERT(!local.Exists(file03Name.c_str()));
                AZ_TEST_ASSERT(!local.IsDirectory(file04Name.c_str()));

                AZ::u64 f3s = 0;
                AZ_TEST_ASSERT(local.Size(file04Name.c_str(), f3s));
                AZ_TEST_ASSERT(f3s == 19);

                // deep destroy directory:
                AZ_TEST_ASSERT(local.DestroyPath(folderName.c_str()));
                AZ_TEST_ASSERT(!local.Exists(folderName.c_str()));
            }
        };

        TEST_F(FindFilesTest, Test)
        {
            run();
        }

        class AliasTest
            : public FolderFixture
        {
        public:
            void run()
            {
                AZ::IO::LocalFileIO local;

                // test aliases
                local.SetAlias("@test@", folderName.c_str());
                const char* testDest1 = local.GetAlias("@test@");
                AZ_TEST_ASSERT(testDest1 != nullptr);
                const char* testDest2 = local.GetAlias("@NOPE@");
                AZ_TEST_ASSERT(testDest2 == nullptr);
                testDest1 = local.GetAlias("@test@"); // try with different case
                AZ_TEST_ASSERT(testDest1 != nullptr);

                // test resolving
                const char* aliasTestPath = "@test@\\some\\path\\somefile.txt";
                char aliasResolvedPath[MAX_PATH];
                bool resolveDidWork = local.ResolvePath(aliasTestPath, aliasResolvedPath, MAX_PATH);
                AZ_TEST_ASSERT(resolveDidWork);
                AZStd::string expectedResolvedPath = folderName + "some/path/somefile.txt";
                AZ_TEST_ASSERT(aliasResolvedPath == expectedResolvedPath);

                // Test that sending in a too small output path fails,
                // if the output buffer is smaller than the string being resolved
                size_t SMALLER_THAN_PATH_BEING_RESOLVED = strlen(aliasTestPath) - 1;
                AZ_TEST_START_ASSERTTEST;
                resolveDidWork = local.ResolvePath(aliasTestPath, aliasResolvedPath, SMALLER_THAN_PATH_BEING_RESOLVED);
                AZ_TEST_STOP_ASSERTTEST(1);
                AZ_TEST_ASSERT(!resolveDidWork);

                // Test that sending in a too small output path fails,
                // if the output buffer is too small to hold the resolved path
                size_t SMALLER_THAN_FINAL_RESOLVED_PATH = expectedResolvedPath.length() - 1;
                AZ_TEST_START_ASSERTTEST;
                resolveDidWork = local.ResolvePath(aliasTestPath, aliasResolvedPath, SMALLER_THAN_FINAL_RESOLVED_PATH);
                AZ_TEST_STOP_ASSERTTEST(1);
                AZ_TEST_ASSERT(!resolveDidWork);

                // test clearing an alias
                local.ClearAlias("@test@");
                testDest1 = local.GetAlias("@test@");
                AZ_TEST_ASSERT(testDest1 == nullptr);
            }
        };

        TEST_F(AliasTest, Test)
        {
            run();
        }

        class SmartMoveTests
            : public FolderFixture
        {
        public:
            void run()
            {
                LocalFileIO localFileIO;
                AZ::IO::FileIOBase::SetInstance(&localFileIO);
                AZStd::string path;
                AzFramework::StringFunc::Path::GetFullPath(file01Name.c_str(), path);
                AZ_TEST_ASSERT(localFileIO.CreatePath(path.c_str()));
                AzFramework::StringFunc::Path::GetFullPath(file02Name.c_str(), path);
                AZ_TEST_ASSERT(localFileIO.CreatePath(path.c_str()));

                AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
                localFileIO.Open(file01Name.c_str(), OpenMode::ModeWrite | OpenMode::ModeText, fileHandle);
                localFileIO.Write(fileHandle, "DummyFile", 9);
                localFileIO.Close(fileHandle);

                AZ::IO::HandleType fileHandle1 = AZ::IO::InvalidHandle;
                localFileIO.Open(file02Name.c_str(), OpenMode::ModeWrite | OpenMode::ModeText, fileHandle1);
                localFileIO.Write(fileHandle1, "TestFile", 8);
                localFileIO.Close(fileHandle1);

                fileHandle1 = AZ::IO::InvalidHandle;
                localFileIO.Open(file02Name.c_str(), OpenMode::ModeRead | OpenMode::ModeText, fileHandle1);
                static const size_t testStringLen = 256;
                char testString[testStringLen] = { 0 };
                localFileIO.Read(fileHandle1, testString, testStringLen);
                localFileIO.Close(fileHandle1);
                AZ_TEST_ASSERT(strncmp(testString, "TestFile", 8) == 0);

                // try swapping files when none of the files are in use
                AZ_TEST_ASSERT(AZ::IO::SmartMove(file01Name.c_str(), file02Name.c_str()));

                fileHandle1 = AZ::IO::InvalidHandle;
                localFileIO.Open(file02Name.c_str(), OpenMode::ModeRead | OpenMode::ModeText, fileHandle1);
                testString[0] = '\0';
                localFileIO.Read(fileHandle1, testString, testStringLen);
                localFileIO.Close(fileHandle1);
                AZ_TEST_ASSERT(strncmp(testString, "DummyFile", 9) == 0);

                //try swapping files when source file is not present, this should fail
                AZ_TEST_ASSERT(!AZ::IO::SmartMove(file01Name.c_str(), file02Name.c_str()));

                fileHandle = AZ::IO::InvalidHandle;
                localFileIO.Open(file01Name.c_str(), OpenMode::ModeWrite | OpenMode::ModeText, fileHandle);
                localFileIO.Write(fileHandle, "TestFile", 8);
                localFileIO.Close(fileHandle);

#if defined(AZ_PLATFORM_WINDOWS)
                fileHandle1 = AZ::IO::InvalidHandle;
                localFileIO.Open(file02Name.c_str(), OpenMode::ModeRead | OpenMode::ModeText, fileHandle1);
                testString[0] = '\0';
                localFileIO.Read(fileHandle1, testString, testStringLen);

                // try swapping files when the destination file is open for read only, 
                // since window is unable to move files that are open for read, this will fail.
                AZ_TEST_ASSERT(!AZ::IO::SmartMove(file01Name.c_str(), file02Name.c_str()));
                localFileIO.Close(fileHandle1);
#endif
                fileHandle = AZ::IO::InvalidHandle;
                localFileIO.Open(file01Name.c_str(), OpenMode::ModeRead | OpenMode::ModeText, fileHandle);

                // try swapping files when the source file is open for read only
                AZ_TEST_ASSERT(AZ::IO::SmartMove(file01Name.c_str(), file02Name.c_str()));
                localFileIO.Close(fileHandle);

                fileHandle1 = AZ::IO::InvalidHandle;
                localFileIO.Open(file02Name.c_str(), OpenMode::ModeRead | OpenMode::ModeText, fileHandle1);
                testString[0] = '\0';
                localFileIO.Read(fileHandle1, testString, testStringLen);
                AZ_TEST_ASSERT(strncmp(testString, "TestFile", 8) == 0);
                localFileIO.Close(fileHandle1);

                localFileIO.Remove(file01Name.c_str());
                localFileIO.Remove(file02Name.c_str());
                localFileIO.DestroyPath(m_root.c_str());

                AZ::IO::FileIOBase::SetInstance(nullptr);
            }
        };

        TEST_F(SmartMoveTests, Test)
        {
            run();
        }
    }
}   // namespace UnitTest
