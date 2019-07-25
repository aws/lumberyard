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
#include "StdAfx.h"

#include <AzTest/AzTest.h>
#include <CryPak.h>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/IO/SystemFile.h> // for max path decl
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/parallel/semaphore.h>
#include <CryPakFileIO.h>
#include <AzCore/std/functional.h> // for function<> in the find files callback.
#include <AzFramework/IO/LocalFileIO.h>

namespace CryPakUnitTests
{

#if defined(WIN32) || defined(WIN64)

    // Note:  none of the below is really a unit test, its all basic feature tests
    // for critical functionality
    
    // for fuzzing test, how much work to do?
    static int s_numTrialsToPerform = 200500; // as much we can do in about a second.

    class Integ_CryPakUnitTests
        : public ::testing::Test
    {
    protected:
        bool IsPackValid(const char* path)
        {
            ICryPak* pak = gEnv->pCryPak;
            if (!pak)
            {
                return false;
            }

            if (!pak->OpenPack(path, ICryPak::FLAGS_PATH_REAL))
            {
                return false;
            }

            pak->ClosePack(path);
            return true;
        }

        template <class Function>
        void RunConcurrentUnitTest(AZ::u32 numIterations, AZ::u32 numThreads, Function testFunction)
        {
            AZStd::semaphore wait;
            AZStd::atomic_int runningThreads;
            AZStd::atomic_int successCount;

            for (AZ::u32 itr = 0; itr < numIterations; ++itr)
            {
                runningThreads = 0;
                successCount = 0;

                for (AZ::u32 threadIdx = 0; threadIdx < numThreads; ++threadIdx)
                {
                    ++runningThreads;
                    AZStd::thread testThread(
                        [testFunction, itr, threadIdx, &wait, &runningThreads, &successCount]()
                        {
                            // Add some variability to thread timing by every other thread sleeping some
                            if (threadIdx % 2)
                            {
                                CrySleep(1 + (itr + threadIdx) % 3);
                            }

                            bool success = testFunction();
                            if (success)
                            {
                                ++successCount;
                            }
                            if (--runningThreads == 0)
                            {
                                wait.release();
                            }
                        }
                        );
                    testThread.detach();
                }

                wait.acquire();
                EXPECT_TRUE(!runningThreads);
                EXPECT_TRUE(successCount == numThreads);
            }
        }

        void TestFGetCachedFileData(const char* testFilePath, size_t dataLen, const char* testData)
        {
            ICryPak* pak = gEnv->pCryPak;
            EXPECT_TRUE(pak != nullptr);

            // Note: CryEngine never releases boot CBootProfiler, so these numbers stay artificially low until that is addressesed
            const int numThreadedIterations = 1;
            const int numTestThreads = 5;

            {
                // Canary tests first
                AZ::IO::HandleType fileHandle = pak->FOpen(testFilePath, "rb", 0);

                EXPECT_TRUE(fileHandle != AZ::IO::InvalidHandle);

                size_t fileSize = 0;
                char* pFileBuffer = (char*)gEnv->pCryPak->FGetCachedFileData(fileHandle, fileSize);
                EXPECT_TRUE(pFileBuffer != nullptr);
                EXPECT_TRUE(fileSize == dataLen);
                EXPECT_TRUE(memcmp(pFileBuffer, testData, dataLen) == 0);

                // 2nd call to FGetCAchedFileData, same file handle
                fileSize = 0;
                char* pFileBuffer2 = (char*)gEnv->pCryPak->FGetCachedFileData(fileHandle, fileSize);
                EXPECT_TRUE(pFileBuffer2 != nullptr);
                EXPECT_TRUE(pFileBuffer == pFileBuffer2);
                EXPECT_TRUE(fileSize == dataLen);

                // open already open file and call FGetCAchedFileData
                fileSize = 0;
                {
                    AZ::IO::HandleType fileHandle2 = pak->FOpen(testFilePath, "rb", 0);
                    char* pFileBuffer3 = (char*)gEnv->pCryPak->FGetCachedFileData(fileHandle2, fileSize);
                    EXPECT_TRUE(pFileBuffer3 != nullptr);
                    EXPECT_TRUE(fileSize == dataLen);
                    EXPECT_TRUE(memcmp(pFileBuffer3, testData, dataLen) == 0);
                    pak->FClose(fileHandle2);
                }

                // Multithreaded test #1 reading from the same file handle in parallel
                RunConcurrentUnitTest(numThreadedIterations, numTestThreads,
                    [fileHandle, pFileBuffer, dataLen]()
                    {
                        size_t fileSize = 0;
                        char* pFileBufferThread = (char*)gEnv->pCryPak->FGetCachedFileData(fileHandle, fileSize);
                        bool success = pFileBufferThread != nullptr;
                        success = success && (pFileBuffer == pFileBufferThread);
                        success = success && (fileSize == dataLen);
                        return success;
                    }
                    );

                pak->FClose(fileHandle);
            }

            // Multithreaded Test #2 reading from the same file concurrently
            RunConcurrentUnitTest(numThreadedIterations, numTestThreads,
                [testFilePath, dataLen, testData]()
                {
                    bool success = false;
                    AZ::IO::HandleType threadFileHandle = gEnv->pCryPak->FOpen(testFilePath, "rb", 0);
                    if (threadFileHandle != AZ::IO::InvalidHandle)
                    {
                        size_t fileSize = 0;
                        char* pFileBufferThread = (char*)gEnv->pCryPak->FGetCachedFileData(threadFileHandle, fileSize);
                        success = pFileBufferThread != nullptr;
                        success = success && (fileSize == dataLen);
                        success = success && (memcmp(pFileBufferThread, testData, dataLen) == 0);
                    }
                    gEnv->pCryPak->FClose(threadFileHandle);
                    return success;
                }
                );
        }

        void PerformOneIterationOfCryPakTest(ICryArchive::EPakFlags openFlags, ICryArchive::ECompressionMethods compressionMethod, ICryArchive::ECompressionLevels level, int stepSize = 777, int numSteps = 7, int iterations = 3)
        {
            // this also coincidentally tests to make sure packs inside aliases work.
            string testPakPath = "@cache@/paktest.pak";

            ICryPak* pak = gEnv->pCryPak;

            EXPECT_TRUE(pak != nullptr);

            // delete test files in case they already exist
            pak->ClosePack(testPakPath.c_str());
            remove(testPakPath.c_str());

            // ------------ BASIC TEST:  Create and read Empty Archive ------------
            ICryArchive* pArchive = pak->OpenArchive(testPakPath.c_str(), nullptr, ICryArchive::FLAGS_CREATE_NEW);
            EXPECT_TRUE(pArchive != nullptr);

            delete pArchive;
            EXPECT_TRUE(IsPackValid(testPakPath.c_str()));

            // ------------ BASIC TEST:  Create archive full of standard sizes (including 0)   ----------------
            pArchive = pak->OpenArchive(testPakPath.c_str(), nullptr, ICryArchive::FLAGS_CREATE_NEW);
            EXPECT_TRUE(pArchive != nullptr);

            // the strategy here is to find errors related to file sizes, alignment, overwrites
            // so the first test will just repeatedly write files into the pack file with varying lengths (in odd number increments from a couple KB down to 0, including 0)
            std::vector<unsigned char> orderedData;
            int maxSize = numSteps * stepSize;

            std::vector<unsigned char> checkSums;
            checkSums.resize(maxSize);
            for (int pos = 0; pos < maxSize; ++pos)
            {
                checkSums[pos] = static_cast<unsigned char>(pos % 256);
            }

            for (int j = 0; j < iterations; ++j)
            {
                for (int currentSize = maxSize; currentSize >= 0; currentSize -= stepSize)
                {
                    char fnBuffer[AZ_MAX_PATH_LEN];

                    sprintf_s(fnBuffer, AZ_MAX_PATH_LEN, "file-%i-%i.dat", static_cast<int>(currentSize), j);
                    EXPECT_TRUE(pArchive->UpdateFile(fnBuffer, checkSums.data(), currentSize, compressionMethod, level) == 0);
                }
            }

            delete pArchive;
            EXPECT_TRUE(IsPackValid(testPakPath.c_str()));


            // --------------------------------------------- read it back and verify
            pArchive = pak->OpenArchive(testPakPath.c_str(), nullptr, openFlags);
            EXPECT_TRUE(pArchive != nullptr);

            for (int j = 0; j < iterations; ++j)
            {
                for (int currentSize = maxSize; currentSize >= 0; currentSize -= stepSize)
                {
                    char fnBuffer[AZ_MAX_PATH_LEN];
                    sprintf_s(fnBuffer, AZ_MAX_PATH_LEN, "file-%i-%i.dat", static_cast<int>(currentSize), j);
                    ICryArchive::Handle hand = pArchive->FindFile(fnBuffer);
                    EXPECT_TRUE(hand != nullptr);
                    EXPECT_TRUE(pArchive->GetFileSize(hand) == currentSize);
                    EXPECT_TRUE(pArchive->ReadFile(hand, checkSums.data()) == 0);
                    for (int pos = 0; pos < currentSize; ++pos)
                    {
                        EXPECT_TRUE(checkSums[pos] == (pos % 256));
                    }
                }
            }

            delete pArchive;
            EXPECT_TRUE(IsPackValid(testPakPath.c_str()));

            // ---------------- MORE COMPLICATED TEST which involves overwriting elements ----------------
            pArchive = pak->OpenArchive(testPakPath.c_str());
            EXPECT_TRUE(pArchive != nullptr);

            // overwrite the first and last iterations with files that are half their original size.
            for (int j = 0; j < iterations; ++j)
            {
                for (int currentSize = maxSize; currentSize >= 0; currentSize -= stepSize)
                {
                    char fnBuffer[AZ_MAX_PATH_LEN];

                    int newSize = currentSize; // more will become zero
                    if (j != 1)
                    {
                        newSize = newSize / 2; // the second iteration overwrites files with exactly the same size.
                    }

                    sprintf_s(fnBuffer, AZ_MAX_PATH_LEN, "file-%i-%i.dat", static_cast<int>(currentSize), j);

                    // before we overwrite it, ensure that the element is correctly resized:
                    ICryArchive::Handle hand = pArchive->FindFile(fnBuffer);
                    EXPECT_TRUE(hand != nullptr);
                    EXPECT_TRUE(pArchive->GetFileSize(hand) == currentSize);
                    EXPECT_TRUE(pArchive->ReadFile(hand, checkSums.data()) == 0);
                    for (int pos = 0; pos < currentSize; ++pos)
                    {
                        EXPECT_TRUE(checkSums[pos] == (pos % 256));
                    }

                    // now overwrite it:
                    EXPECT_TRUE(pArchive->UpdateFile(fnBuffer, checkSums.data(), newSize, compressionMethod, level) == 0);

                    // after overwriting it ensure that the pack contains the updated info:
                    hand = pArchive->FindFile(fnBuffer);
                    EXPECT_TRUE(hand != nullptr);
                    EXPECT_TRUE(pArchive->GetFileSize(hand) == newSize);
                    EXPECT_TRUE(pArchive->ReadFile(hand, checkSums.data()) == 0);
                    for (int pos = 0; pos < newSize; ++pos)
                    {
                        EXPECT_TRUE(checkSums[pos] == (pos % 256));
                    }
                }
            }
            delete pArchive;
            EXPECT_TRUE(IsPackValid(testPakPath.c_str()));

            // -------------------------------------------------------------------------------------------
            // read it back and verify
            pArchive = pak->OpenArchive(testPakPath.c_str(), nullptr, openFlags);
            EXPECT_TRUE(pArchive != nullptr);

            for (int j = 0; j < iterations; ++j)
            {
                for (int currentSize = maxSize; currentSize >= 0; currentSize -= stepSize)
                {
                    char fnBuffer[AZ_MAX_PATH_LEN];
                    sprintf_s(fnBuffer, AZ_MAX_PATH_LEN, "file-%i-%i.dat", static_cast<int>(currentSize), j);

                    int newSize = currentSize; // more will become zero
                    if (j != 1)
                    {
                        newSize = newSize / 2; // the middle iteration overwrites files with exactly the same size.
                    }

                    ICryArchive::Handle hand = pArchive->FindFile(fnBuffer);
                    EXPECT_TRUE(hand != nullptr);
                    EXPECT_TRUE(pArchive->ReadFile(hand, checkSums.data()) == 0);
                    EXPECT_TRUE(pArchive->GetFileSize(hand) == newSize);

                    for (int pos = 0; pos < newSize; ++pos)
                    {
                        EXPECT_TRUE(checkSums[pos] == (pos % 256));
                    }
                }
            }

            delete pArchive;
            EXPECT_TRUE(IsPackValid(testPakPath.c_str()));

            // ---------- scattered test --------------
            // in this next test, we're going to update only some elements, to make sure it reads existing data okay
            // we want to make at least one element shrink and one element grow, adjacent to other files
            // this will include files that become zero size, and also includes new files that were not there before

            // first, reset the pack to the original state:
            pArchive = pak->OpenArchive(testPakPath.c_str(), nullptr, ICryArchive::FLAGS_CREATE_NEW);
            EXPECT_TRUE(pArchive != nullptr);

            for (int j = 0; j < iterations; ++j)
            {
                for (int currentSize = maxSize; currentSize >= 0; currentSize -= stepSize)
                {
                    char fnBuffer[AZ_MAX_PATH_LEN];

                    sprintf_s(fnBuffer, AZ_MAX_PATH_LEN, "file-%i-%i.dat", static_cast<int>(currentSize), j);
                    EXPECT_TRUE(pArchive->UpdateFile(fnBuffer, checkSums.data(), currentSize, compressionMethod, level) == 0);
                }
            }

            delete pArchive;
            EXPECT_TRUE(IsPackValid(testPakPath.c_str()));

            pArchive = pak->OpenArchive(testPakPath.c_str());
            EXPECT_TRUE(pArchive != nullptr);
            // replace a scattering of the files:

            int writeCount = 0;
            for (int j = 0; j < iterations + 1; ++j) // note:  an extra iteration to generate new files
            {
                char fnBuffer[AZ_MAX_PATH_LEN];
                for (int currentSize = maxSize; currentSize >= 0; currentSize -= stepSize)
                {
                    sprintf_s(fnBuffer, AZ_MAX_PATH_LEN, "file-%i-%i.dat", static_cast<int>(currentSize), j);

                    ++writeCount;
                    if (writeCount % 4 == 0)
                    {
                        if (j != iterations) // the last one wont be there
                        {
                            // don't do anything for every fourth file, but we do make sure its there:
                            ICryArchive::Handle hand = pArchive->FindFile(fnBuffer);
                            EXPECT_TRUE(hand != nullptr);
                            EXPECT_TRUE(pArchive->GetFileSize(hand) == currentSize);
                            EXPECT_TRUE(pArchive->ReadFile(hand, checkSums.data()) == 0);
                        }

                        continue;
                    }

                    int newSize = currentSize;

                    if (writeCount % 4 == 1)
                    {
                        newSize = newSize * 2;
                    }
                    else if (writeCount % 4 == 2)
                    {
                        newSize = newSize / 2;
                    }
                    else if (writeCount % 4 == 3)
                    {
                        newSize = 0;
                    }

                    if (newSize > maxSize)
                    {
                        newSize = maxSize; // don't blow our buffer!
                    }



                    // overwrite it:
                    EXPECT_TRUE(pArchive->UpdateFile(fnBuffer, checkSums.data(), newSize, compressionMethod, level) == 0);

                    // after overwriting it ensure that the pack contains the updated info:
                    ICryArchive::Handle hand = pArchive->FindFile(fnBuffer);
                    EXPECT_TRUE(hand != nullptr);
                    EXPECT_TRUE(pArchive->GetFileSize(hand) == newSize);
                    EXPECT_TRUE(pArchive->ReadFile(hand, checkSums.data()) == 0);
                    for (int pos = 0; pos < newSize; ++pos)
                    {
                        EXPECT_TRUE(checkSums[pos] == (pos % 256));
                    }
                }
            }
            delete pArchive;
            EXPECT_TRUE(IsPackValid(testPakPath.c_str()));

            // -------------------------------------------------------------------------------------------
            // read it back and verify
            pArchive = pak->OpenArchive(testPakPath.c_str(), nullptr, openFlags);
            EXPECT_TRUE(pArchive != nullptr);

            writeCount = 0;
            for (int j = 0; j < iterations + 1; ++j) // make sure the extra iteration is there.
            {
                char fnBuffer[AZ_MAX_PATH_LEN];
                for (int currentSize = maxSize; currentSize >= 0; currentSize -= stepSize)
                {
                    ++writeCount;

                    int newSize = currentSize;

                    if (writeCount % 4 == 1)
                    {
                        newSize = newSize * 2;
                    }
                    else if (writeCount % 4 == 2)
                    {
                        newSize = newSize / 2;
                    }
                    else if (writeCount % 4 == 3)
                    {
                        newSize = 0;
                    }
                    else if (writeCount % 4 == 0)
                    {
                        if (j == iterations) // the last one wont be there
                        {
                            continue;
                        }
                    }

                    if (newSize > maxSize)
                    {
                        newSize = maxSize; // don't blow our buffer!
                    }



                    sprintf_s(fnBuffer, AZ_MAX_PATH_LEN, "file-%i-%i.dat", static_cast<int>(currentSize), j);

                    // check it:
                    ICryArchive::Handle hand = pArchive->FindFile(fnBuffer);
                    EXPECT_TRUE(hand != nullptr);
                    EXPECT_TRUE(pArchive->GetFileSize(hand) == newSize);
                    EXPECT_TRUE(pArchive->ReadFile(hand, checkSums.data()) == 0);
                    for (int pos = 0; pos < newSize; ++pos)
                    {
                        EXPECT_TRUE(checkSums[pos] == (pos % 256));
                    }
                }
            }
            delete pArchive;
            EXPECT_TRUE(IsPackValid(testPakPath.c_str()));
        }
    };

    TEST_F(Integ_CryPakUnitTests, TestCryPakFGetCachedFileData)
    {
        // Test setup - from PakFile
        const char* fileInPakFile = "levels\\mylevel\\levelinfo.xml";
        const char* dataString = "HELLO WORLD"; // other unit tests make sure writing and reading is working, so don't test that here
        const size_t dataLen = strlen(dataString);

        ICryPak* pak = gEnv->pCryPak;
        EXPECT_TRUE(pak != nullptr);

        {
            string testPakPath_withSubfolders = "@cache@/immediate.pak";
            string testPakPath_withMountPoint = "@cache@/levels/test/flatpak.pak";

            // delete test files in case they already exist
            pak->ClosePack(testPakPath_withSubfolders.c_str());
            remove(testPakPath_withSubfolders.c_str());
            remove(testPakPath_withMountPoint.c_str());
            gEnv->pFileIO->CreatePath("@cache@/levels/test");

            // setup test pak and file
            ICryArchive* pArchive = pak->OpenArchive(testPakPath_withSubfolders.c_str(), nullptr, ICryArchive::FLAGS_CREATE_NEW);
            EXPECT_TRUE(pArchive != nullptr);
            EXPECT_TRUE(pArchive->UpdateFile(fileInPakFile, (void*)dataString, dataLen, ICryArchive::METHOD_COMPRESS, ICryArchive::LEVEL_FASTEST) == 0);
            delete pArchive;
            EXPECT_TRUE(IsPackValid(testPakPath_withSubfolders.c_str()));

            EXPECT_TRUE(pak->OpenPack("@assets@", testPakPath_withSubfolders.c_str()));

            EXPECT_TRUE(pak->IsFileExist(fileInPakFile));
        }

        // ---- PakFile FGetCachedFileDataTests (these leverage PakFiles CachedFile mechanism for caching data ---
        TestFGetCachedFileData(fileInPakFile, dataLen, dataString);

        // ------setup loose file FGetCachedFileData tests -------------------------
        const char* testRootPath = "@log@/unittesttemp";
        const char* looseTestFilePath = "@log@/unittesttemp/realfileforunittest.txt";
        {
            using namespace AZ::IO;
            CryPakFileIO cpfio;
            ICryPak* pak = gEnv->pCryPak;
            EXPECT_TRUE(pak != nullptr);
            const char* dataString = "HELLO WORLD"; // other unit tests make sure writing and reading is working, so don't test that here
            size_t dataLen = strlen(dataString);

            // remove existing
            EXPECT_TRUE(cpfio.DestroyPath(testRootPath) == ResultCode::Success);

            // create test file
            EXPECT_TRUE(cpfio.CreatePath(testRootPath) == ResultCode::Success);

            HandleType normalFileHandle;
            EXPECT_TRUE(cpfio.Open(looseTestFilePath, OpenMode::ModeWrite | OpenMode::ModeBinary, normalFileHandle) == ResultCode::Success);

            AZ::u64 bytesWritten = 0;
            EXPECT_TRUE(cpfio.Write(normalFileHandle, dataString, dataLen, &bytesWritten) == ResultCode::Success);
            EXPECT_TRUE(bytesWritten == dataLen);

            EXPECT_TRUE(cpfio.Close(normalFileHandle) == ResultCode::Success);
            EXPECT_TRUE(cpfio.Exists(looseTestFilePath));
        }

        // ---- loose file FGetCachedFileDataTests (these leverage the CryPak RawDataCache) ---
        TestFGetCachedFileData(looseTestFilePath, dataLen, dataString);
    }

    // a bug was found that causes problems reading data from packs if they are immediately mounted after writing.
    // this unit test adjusts for that.
    TEST_F(Integ_CryPakUnitTests, TestCryPackImmediateReading)
    {
        // the strategy is to create a pak file similar to how the level system does
        // one which contains subfolders
        // and a file inside that subfolder
        // to be successful, it must be possible to write that pack, close it, then open it via crypak
        // and be able to IMMEDIATELY
        // * read the file in the subfolder
        // * enumerate the folders (including that subfolder) even though they are 'virtual', not real folders on physical media
        // * all of the above even though the mount point for the pak is @assets@ wheras the physical pack lives in @cache@
        // finally, we're going to repeat the above test but with files mounted with subfolders
        // so for example, the pack will contain levelinfo.xml at the root of it
        // but it will be mounted at a subfolder (levels/mylevel).
        // this must cause FindNext and Open to work for levels/mylevel/levelinfo.xml.

        string testPakPath_withSubfolders = "@cache@/immediate.pak";
        string testPakPath_withMountPoint = "@cache@/levels/test/flatpak.pak";
        ICryPak* pak = gEnv->pCryPak;
        EXPECT_TRUE(pak != nullptr);
        const char* dataString = "HELLO WORLD"; // other unit tests make sure writing and reading is working, so don't test that here
        size_t dataLen = strlen(dataString);


        // delete test files in case they already exist
        pak->ClosePack(testPakPath_withSubfolders.c_str());
        pak->ClosePack(testPakPath_withMountPoint.c_str());
        remove(testPakPath_withSubfolders.c_str());
        remove(testPakPath_withMountPoint.c_str());
        gEnv->pFileIO->CreatePath("@cache@/levels/test");


        ICryArchive* pArchive = pak->OpenArchive(testPakPath_withSubfolders.c_str(), nullptr, ICryArchive::FLAGS_CREATE_NEW);
        EXPECT_TRUE(pArchive != nullptr);
        EXPECT_TRUE(pArchive->UpdateFile("levels\\mylevel\\levelinfo.xml", (void*)dataString, dataLen, ICryArchive::METHOD_COMPRESS, ICryArchive::LEVEL_FASTEST) == 0);
        delete pArchive;
        EXPECT_TRUE(IsPackValid(testPakPath_withSubfolders.c_str()));

        EXPECT_TRUE(pak->OpenPack("@assets@", testPakPath_withSubfolders.c_str()));
        // ---- BARRAGE OF TESTS
        EXPECT_TRUE(pak->IsFileExist("levels\\mylevel\\levelinfo.xml"));
        EXPECT_TRUE(pak->IsFileExist("levels//mylevel//levelinfo.xml"));

        bool found_mylevel_folder = false;
        struct _finddata_t fileinfo;
        intptr_t handle = pak->FindFirst("levels\\*.*", &fileinfo);

        EXPECT_TRUE(handle != -1);
        if (handle != -1)
        {
            do
            {
                if (fileinfo.attrib & _A_SUBDIR)
                {
                    if (azstricmp(fileinfo.name, "mylevel") == 0)
                    {
                        found_mylevel_folder = true;
                    }
                }
                else
                {
                    EXPECT_TRUE(azstricmp(fileinfo.name, "levelinfo.xml") != 0); // you may not find files inside the pak in this folder.
                }
            } while (pak->FindNext(handle, &fileinfo) >= 0);

            pak->FindClose(handle);
        }

        EXPECT_TRUE(found_mylevel_folder);

        bool found_mylevel_file = false;

        handle = pak->FindFirst("levels\\mylevel\\*.*", &fileinfo);

        EXPECT_TRUE(handle != -1);
        if (handle != -1)
        {
            do
            {
                if ((fileinfo.attrib & _A_SUBDIR) == 0)
                {
                    if (azstricmp(fileinfo.name, "levelinfo.xml") == 0)
                    {
                        found_mylevel_file = true;
                    }
                }
                else
                {
                    EXPECT_TRUE(azstricmp(fileinfo.name, "mylevel") != 0); // you may not find the level subfolder here since we're in the subfolder already
                    EXPECT_TRUE(azstricmp(fileinfo.name, "levels\\mylevel") != 0);
                    EXPECT_TRUE(azstricmp(fileinfo.name, "levels//mylevel") != 0);
                }
            } while (pak->FindNext(handle, &fileinfo) >= 0);

            pak->FindClose(handle);
        }

        EXPECT_TRUE(found_mylevel_file);

        // now test clean-up
        pak->ClosePack(testPakPath_withSubfolders.c_str());
        remove(testPakPath_withSubfolders.c_str());

        EXPECT_TRUE(!pak->IsFileExist("levels\\mylevel\\levelinfo.xml"));
        EXPECT_TRUE(!pak->IsFileExist("levels//mylevel//levelinfo.xml"));

        handle = pak->FindFirst("levels\\*.*", &fileinfo);

        found_mylevel_folder = false;

        EXPECT_TRUE(handle != -1);
        if (handle != -1)
        {
            do
            {
                if (fileinfo.attrib & _A_SUBDIR)
                {
                    if (azstricmp(fileinfo.name, "mylevel") == 0)
                    {
                        found_mylevel_folder = true;
                    }
                }
            } while (pak->FindNext(handle, &fileinfo) >= 0);

            pak->FindClose(handle);
        }

        EXPECT_TRUE(!found_mylevel_folder);


        // ----------- SECOND TEST.  File in levels/mylevel/ showing up as searchable.
        // note that the actual file's folder has nothing to do with the mount point.

        pArchive = pak->OpenArchive(testPakPath_withMountPoint.c_str(), nullptr, ICryArchive::FLAGS_CREATE_NEW);
        EXPECT_TRUE(pArchive != nullptr);
        EXPECT_TRUE(pArchive->UpdateFile("levelinfo.xml", (void*)dataString, dataLen, ICryArchive::METHOD_COMPRESS, ICryArchive::LEVEL_FASTEST) == 0);
        delete pArchive;
        EXPECT_TRUE(IsPackValid(testPakPath_withMountPoint.c_str()));

        EXPECT_TRUE(pak->OpenPack("@assets@\\uniquename\\mylevel2", testPakPath_withMountPoint.c_str()));

        // ---- BARRAGE OF TESTS
        EXPECT_TRUE(pak->IsFileExist("uniquename\\mylevel2\\levelinfo.xml"));
        EXPECT_TRUE(pak->IsFileExist("uniquename//mylevel2//levelinfo.xml"));

        EXPECT_TRUE(!pak->IsFileExist("uniquename\\mylevel\\levelinfo.xml"));
        EXPECT_TRUE(!pak->IsFileExist("uniquename//mylevel//levelinfo.xml"));
        EXPECT_TRUE(!pak->IsFileExist("uniquename\\test\\levelinfo.xml"));
        EXPECT_TRUE(!pak->IsFileExist("uniquename//test//levelinfo.xml"));

        found_mylevel_folder = false;
        handle = pak->FindFirst("uniquename\\*.*", &fileinfo);

        EXPECT_TRUE(handle != -1);
        if (handle != -1)
        {
            do
            {
                if (fileinfo.attrib & _A_SUBDIR)
                {
                    if (azstricmp(fileinfo.name, "mylevel2") == 0)
                    {
                        found_mylevel_folder = true;
                    }
                }
            } while (pak->FindNext(handle, &fileinfo) >= 0);

            pak->FindClose(handle);
        }

        EXPECT_TRUE(found_mylevel_folder);

        found_mylevel_file = false;

        handle = pak->FindFirst("uniquename\\mylevel2\\*.*", &fileinfo);

        EXPECT_TRUE(handle != -1);
        if (handle != -1)
        {
            do
            {
                if ((fileinfo.attrib & _A_SUBDIR) == 0)
                {
                    if (azstricmp(fileinfo.name, "levelinfo.xml") == 0)
                    {
                        found_mylevel_file = true;
                    }
                }
            } while (pak->FindNext(handle, &fileinfo) >= 0);

            pak->FindClose(handle);
        }

        EXPECT_TRUE(found_mylevel_file);

        pak->ClosePack(testPakPath_withMountPoint.c_str());

        // --- test to make sure that when you iterate only the first component is found, so bury it deep and ask for the root
        EXPECT_TRUE(pak->OpenPack("@assets@\\uniquename\\mylevel2\\mylevel3\\mylevel4", testPakPath_withMountPoint.c_str()));

        found_mylevel_folder = false;
        handle = pak->FindFirst("uniquename\\*.*", &fileinfo);

        int numFound = 0;
        EXPECT_TRUE(handle != -1);
        if (handle != -1)
        {
            do
            {
                if (fileinfo.attrib & _A_SUBDIR)
                {
                    ++numFound;
                    if (azstricmp(fileinfo.name, "mylevel2") == 0)
                    {
                        found_mylevel_folder = true;
                    }
                }
            } while (pak->FindNext(handle, &fileinfo) >= 0);

            pak->FindClose(handle);
        }

        EXPECT_TRUE(numFound == 1);
        EXPECT_TRUE(found_mylevel_folder);

        numFound = 0;
        found_mylevel_folder = 0;

        // now make sure no red herrings appear
        // for example, if I have a pack mounted at "@assets@\\uniquename\\mylevel2\\mylevel3\\mylevel4"
        // and I ask for "@assets@\\somethingelse" I should get no hits
        // in addition if I ask for "@assets@\\uniquename\\mylevel3" I should also get no hits
        handle = pak->FindFirst("somethingelse\\*.*", &fileinfo);
        EXPECT_TRUE(handle == -1);

        handle = pak->FindFirst("uniquename\\mylevel3*.*", &fileinfo);
        EXPECT_TRUE(handle == -1);

        pak->ClosePack(testPakPath_withMountPoint.c_str());
        remove(testPakPath_withMountPoint.c_str());
    }

    // test that CryPakFILEIO class works as expected
    TEST_F(Integ_CryPakUnitTests, TestCryPakViaFileIO)
    {
        // strategy:
        // create a loose file and a packed file
        // mount the pack
        // make sure that the pack and loose file both appear when the PaKFileIO interface is used.
        using namespace AZ::IO;
        CryPakFileIO cpfio;
        string genericCryPakFileName = "@cache@/testcrypakio.pak";
        ICryPak* pak = gEnv->pCryPak;
        EXPECT_TRUE(pak != nullptr);
        const char* dataString = "HELLO WORLD"; // other unit tests make sure writing and reading is working, so don't test that here
        size_t dataLen = strlen(dataString);
        AZStd::vector<char> dataBuffer;
        dataBuffer.resize(dataLen);

        // delete test files in case they already exist
        pak->ClosePack(genericCryPakFileName.c_str());
        remove(genericCryPakFileName.c_str());

        // create generic file

        HandleType normalFileHandle;
        AZ::u64 bytesWritten = 0;
        EXPECT_TRUE(cpfio.DestroyPath("@log@/unittesttemp") == ResultCode::Success);
        EXPECT_TRUE(!cpfio.Exists("@log@/unittesttemp"));
        EXPECT_TRUE(!cpfio.IsDirectory("@log@/unittesttemp"));
        EXPECT_TRUE(cpfio.CreatePath("@log@/unittesttemp") == ResultCode::Success);
        EXPECT_TRUE(cpfio.Exists("@log@/unittesttemp"));
        EXPECT_TRUE(cpfio.IsDirectory("@log@/unittesttemp"));
        EXPECT_TRUE(cpfio.Open("@log@/unittesttemp/realfileforunittest.xml", OpenMode::ModeWrite | OpenMode::ModeBinary, normalFileHandle) == ResultCode::Success);
        EXPECT_TRUE(cpfio.Write(normalFileHandle, dataString, dataLen, &bytesWritten) == ResultCode::Success);
        EXPECT_TRUE(bytesWritten == dataLen);
        EXPECT_TRUE(cpfio.Close(normalFileHandle) == ResultCode::Success);
        normalFileHandle = InvalidHandle;
        EXPECT_TRUE(cpfio.Exists("@log@/unittesttemp/realfileforunittest.xml"));

        ICryArchive* pArchive = pak->OpenArchive(genericCryPakFileName.c_str(), nullptr, ICryArchive::FLAGS_CREATE_NEW);
        EXPECT_TRUE(pArchive != nullptr);
        EXPECT_TRUE(pArchive->UpdateFile("testfile.xml", (void*)dataString, dataLen, ICryArchive::METHOD_COMPRESS, ICryArchive::LEVEL_FASTEST) == 0);
        delete pArchive;
        EXPECT_TRUE(IsPackValid(genericCryPakFileName.c_str()));

        EXPECT_TRUE(pak->OpenPack("@assets@", genericCryPakFileName.c_str()));

        // ---- BARRAGE OF TESTS
        EXPECT_TRUE(cpfio.Exists("testfile.xml"));
        EXPECT_TRUE(cpfio.Exists("@assets@/testfile.xml"));  // this should be hte same file
        EXPECT_TRUE(!cpfio.Exists("@log@/testfile.xml"));
        EXPECT_TRUE(!cpfio.Exists("@cache@/testfile.xml"));
        EXPECT_TRUE(cpfio.Exists("@log@/unittesttemp/realfileforunittest.xml"));

        // --- Coverage test ----
        AZ::u64 fileSize = 0;
        AZ::u64 bytesRead = 0;
        AZ::u64 currentOffset = 0;
        char fileNameBuffer[AZ_MAX_PATH_LEN] = { 0 };
        EXPECT_TRUE(cpfio.Size("testfile.xml", fileSize) == ResultCode::Success);
        EXPECT_TRUE(fileSize == dataLen);
        EXPECT_TRUE(cpfio.Open("testfile.xml", OpenMode::ModeRead | OpenMode::ModeBinary, normalFileHandle) == ResultCode::Success);
        EXPECT_TRUE(normalFileHandle != InvalidHandle);
        EXPECT_TRUE(cpfio.Size(normalFileHandle, fileSize) == ResultCode::Success);
        EXPECT_TRUE(fileSize == dataLen);
        EXPECT_TRUE(cpfio.Read(normalFileHandle, dataBuffer.data(), 2, true, &bytesRead) == ResultCode::Success);
        EXPECT_TRUE(bytesRead == 2);
        EXPECT_TRUE(dataBuffer[0] == 'H');
        EXPECT_TRUE(dataBuffer[1] == 'E');
        EXPECT_TRUE(cpfio.Tell(normalFileHandle, currentOffset) == ResultCode::Success);
        EXPECT_TRUE(currentOffset == 2);
        EXPECT_TRUE(cpfio.Seek(normalFileHandle, 2, SeekType::SeekFromCurrent) == ResultCode::Success);
        EXPECT_TRUE(cpfio.Tell(normalFileHandle, currentOffset) == ResultCode::Success);
        EXPECT_TRUE(currentOffset == 4);
        EXPECT_TRUE(!cpfio.Eof(normalFileHandle));
        EXPECT_TRUE(cpfio.Seek(normalFileHandle, 2, SeekType::SeekFromStart) == ResultCode::Success);
        EXPECT_TRUE(cpfio.Tell(normalFileHandle, currentOffset) == ResultCode::Success);
        EXPECT_TRUE(currentOffset == 2);
        EXPECT_TRUE(cpfio.Seek(normalFileHandle, -2, SeekType::SeekFromEnd) == ResultCode::Success);
        EXPECT_TRUE(cpfio.Tell(normalFileHandle, currentOffset) == ResultCode::Success);
        EXPECT_TRUE(currentOffset == dataLen - 2);
        EXPECT_TRUE(cpfio.Read(normalFileHandle, dataBuffer.data(), 4, true, &bytesRead) != ResultCode::Success);
        EXPECT_TRUE(bytesRead == 2);
        EXPECT_TRUE(cpfio.Eof(normalFileHandle));
        EXPECT_TRUE(cpfio.GetFilename(normalFileHandle, fileNameBuffer, AZ_MAX_PATH_LEN));
        EXPECT_TRUE(azstricmp(fileNameBuffer, "testfile.xml") == 0);
        // just coverage-call this function:
        EXPECT_TRUE(cpfio.ModificationTime(normalFileHandle) == gEnv->pCryPak->GetModificationTime(normalFileHandle));
        EXPECT_TRUE(cpfio.ModificationTime("testfile.xml") == gEnv->pCryPak->GetModificationTime(normalFileHandle));
        EXPECT_TRUE(cpfio.ModificationTime("testfile.xml") != 0);
        EXPECT_TRUE(cpfio.ModificationTime("@log@/unittesttemp/realfileforunittest.xml") != 0);

        EXPECT_TRUE(cpfio.Close(normalFileHandle) == ResultCode::Success);

        EXPECT_TRUE(!cpfio.IsDirectory("testfile.xml"));
        EXPECT_TRUE(cpfio.IsDirectory("@assets@"));
        EXPECT_TRUE(cpfio.IsReadOnly("testfile.xml"));
        EXPECT_TRUE(cpfio.IsReadOnly("@assets@/testfile.xml"));
        EXPECT_TRUE(!cpfio.IsReadOnly("@log@/unittesttemp/realfileforunittest.xml"));


        // copy file from inside pak:
        EXPECT_TRUE(cpfio.Copy("testfile.xml", "@log@/unittesttemp/copiedfile.xml") == ResultCode::Success);
        EXPECT_TRUE(cpfio.Exists("@log@/unittesttemp/copiedfile.xml"));

        // make sure copy is ok

        EXPECT_TRUE(cpfio.Open("@log@/unittesttemp/copiedfile.xml", OpenMode::ModeRead | OpenMode::ModeBinary, normalFileHandle) == ResultCode::Success);
        EXPECT_TRUE(cpfio.Size(normalFileHandle, fileSize) == ResultCode::Success);
        EXPECT_TRUE(fileSize == dataLen);
        EXPECT_TRUE(cpfio.Read(normalFileHandle, dataBuffer.data(), dataLen + 10, false, &bytesRead) == ResultCode::Success); // allowed to read less
        EXPECT_TRUE(bytesRead == dataLen);
        EXPECT_TRUE(memcmp(dataBuffer.data(), dataString, dataLen) == 0);
        EXPECT_TRUE(cpfio.Close(normalFileHandle) == ResultCode::Success);

        // make sure file does not exist, since copy will NOT overwrite:
        cpfio.Remove("@log@/unittesttemp/copiedfile2.xml");

        EXPECT_TRUE(cpfio.Rename("@log@/unittesttemp/copiedfile.xml", "@log@/unittesttemp/copiedfile2.xml") == ResultCode::Success);
        EXPECT_TRUE(!cpfio.Exists("@log@/unittesttemp/copiedfile.xml"));
        EXPECT_TRUE(cpfio.Exists("@log@/unittesttemp/copiedfile2.xml"));

        // find files test.

        bool foundIt = false;
        // note that this file exists only in the pak.  
        cpfio.FindFiles("@assets@", "*.xml", [&foundIt](const char* foundName)
            {
                // according to the contract stated in the FileIO.h file, we expect full paths. (Aliases are full paths)
                if (azstricmp(foundName, "@assets@/testfile.xml") == 0)
                {
                    foundIt = true;
                    return false;
                }
                return true;
            });

        EXPECT_TRUE(foundIt);


        EXPECT_TRUE(cpfio.Remove("@assets@/testfile.xml") != ResultCode::Success); // may not delete pak files
    
        // make sure it works with and without alias:
        EXPECT_TRUE(cpfio.Exists("@assets@/testfile.xml"));
        EXPECT_TRUE(cpfio.Exists("testfile.xml"));

        EXPECT_TRUE(cpfio.Exists("@log@/unittesttemp/realfileforunittest.xml"));
        EXPECT_TRUE(cpfio.Remove("@log@/unittesttemp/realfileforunittest.xml") == ResultCode::Success);
        EXPECT_TRUE(!cpfio.Exists("@log@/unittesttemp/realfileforunittest.xml"));

        // now test clean-up
        pak->ClosePack(genericCryPakFileName.c_str());
        remove(genericCryPakFileName.c_str());

        EXPECT_TRUE(cpfio.DestroyPath("@log@/unittesttemp") == ResultCode::Success);
        EXPECT_TRUE(!cpfio.Exists("@log@/unittesttemp/realfileforunittest.xml"));
        EXPECT_TRUE(!cpfio.Exists("@log@/unittesttemp"));
        EXPECT_TRUE(!pak->IsFileExist("testfile.xml"));
    }

    TEST_F(Integ_CryPakUnitTests, TestCryPakModTime)
    {
        // repeat the following test multiple times, since timing (seconds) can affect it and it involves time!
        for (int iteration = 0; iteration < 10; ++iteration)
        {
            CrySleep(172); // offset time by a nice non expected number

            // helper paths and strings
            string gameFolder = gEnv->pFileIO->GetAlias("@cache@");

            string testFile = "unittest.bin";
            string testFilePath = gameFolder + "\\" + testFile;
            string testPak = "unittest.pak";
            string testPakPath = gameFolder + "\\" + testPak;
            string zipCmd = "-zip=" + testPakPath;

            // delete test files in case they already exist
            remove(testFilePath);
            gEnv->pCryPak->ClosePack(testPakPath);
            remove(testPakPath);

            // create a test file
            char data[] = "unittest";
            FILE* f = nullptr;
            azfopen(&f, testFilePath, "wb");
            EXPECT_TRUE(f != nullptr);                                              // file successfully opened for writing
            EXPECT_TRUE(fwrite(data, sizeof(char), sizeof(data), f) == sizeof(data)); // file written to successfully
            EXPECT_TRUE(fclose(f) == 0);                                 // file closed successfully

            AZ::IO::HandleType fDisk = gEnv->pCryPak->FOpen(testFilePath, "rb");
            EXPECT_TRUE(fDisk > 0);                                          // opened file on disk successfully
            uint64 modTimeDisk = gEnv->pCryPak->GetModificationTime(fDisk);       // high res mod time extracted from file on disk
            EXPECT_TRUE(gEnv->pCryPak->FClose(fDisk) == 0);              // file closed successfully

            // create a low res copy of disk file's mod time
            uint64 absDiff, maxDiff = 20000000ul;
            uint16 dosDate, dosTime;
            FILETIME ft;
            LARGE_INTEGER lt;

            ft.dwHighDateTime = modTimeDisk >> 32;
            ft.dwLowDateTime = modTimeDisk & 0xFFFFFFFF;
            EXPECT_TRUE(FileTimeToDosDateTime(&ft, &dosDate, &dosTime) != FALSE); // converted to DOSTIME successfully
            ft.dwHighDateTime = 0;
            ft.dwLowDateTime = 0;
            EXPECT_TRUE(DosDateTimeToFileTime(dosDate, dosTime, &ft) != FALSE);   // converted to FILETIME successfully
            lt.HighPart = ft.dwHighDateTime;
            lt.LowPart = ft.dwLowDateTime;
            uint64 modTimeDiskLowRes = lt.QuadPart;

            absDiff = modTimeDiskLowRes >= modTimeDisk ? modTimeDiskLowRes - modTimeDisk : modTimeDisk - modTimeDiskLowRes;
            EXPECT_TRUE(absDiff <= maxDiff);                             // FILETIME (high res) and DOSTIME (low res) should be at most 2 seconds apart

            gEnv->pResourceCompilerHelper->CallResourceCompiler(testFilePath, zipCmd);
            EXPECT_TRUE(remove(testFilePath) == 0);                      // test file on disk deleted successfully

            EXPECT_TRUE(gEnv->pCryPak->OpenPack(testPakPath));           // opened pak successfully

            AZ::IO::HandleType fPak = gEnv->pCryPak->FOpen(testFilePath, "rb");
            EXPECT_TRUE(fPak > 0);                                           // file (in pak) opened correctly
            uint64 modTimePak = gEnv->pCryPak->GetModificationTime(fPak);         // low res mod time extracted from file in pak
            EXPECT_TRUE(gEnv->pCryPak->FClose(fPak) == 0);               // file closed successfully

            EXPECT_TRUE(gEnv->pCryPak->ClosePack(testPakPath));          // closed pak successfully
            EXPECT_TRUE(remove(testPakPath) == 0);                       // test pak file deleted successfully

            absDiff = modTimePak >= modTimeDisk ? modTimePak - modTimeDisk : modTimeDisk - modTimePak;
            // compare mod times.  They are allowed to be up to 2 seconds apart but no more
            EXPECT_TRUE(absDiff <= maxDiff);                             // FILETIME (disk) and DOSTIME (pak) should be at most 2 seconds apart
            // note:  Do not directly compare the disk time and pack time, the resolution drops the last digit off in some cases in pak
            // it only has a 2 second resolution.  you may compare to make sure that the pak time is WITHIN 2 seconds (as above) but not equal.

            // we depend on the fact that crypak is rounding up, instead of down
            EXPECT_TRUE(modTimePak >= modTimeDisk);
        }
    }

    TEST_F(Integ_CryPakUnitTests, TestCryPakPacking_Compressed_Better)
    {
        PerformOneIterationOfCryPakTest(ICryArchive::FLAGS_READ_ONLY, ICryArchive::METHOD_COMPRESS, ICryArchive::LEVEL_BETTER);
    }

    TEST_F(Integ_CryPakUnitTests, TestCryPakPacking_ReadOnlyOptimized_Compressed_Better)
    {
        PerformOneIterationOfCryPakTest(ICryArchive::FLAGS_OPTIMIZED_READ_ONLY, ICryArchive::METHOD_COMPRESS, ICryArchive::LEVEL_BETTER);
    }

    TEST_F(Integ_CryPakUnitTests, TestCryPakPacking_ReadWrite_Compressed_Better)
    {
        PerformOneIterationOfCryPakTest((ICryArchive::EPakFlags)0, ICryArchive::METHOD_COMPRESS, ICryArchive::LEVEL_BETTER);
    }

    TEST_F(Integ_CryPakUnitTests, TestCryPakPacking_Compressed_VariousLevels)
    {
        PerformOneIterationOfCryPakTest(ICryArchive::FLAGS_READ_ONLY, ICryArchive::METHOD_COMPRESS, ICryArchive::LEVEL_FASTEST);
        PerformOneIterationOfCryPakTest(ICryArchive::FLAGS_READ_ONLY, ICryArchive::METHOD_COMPRESS, ICryArchive::LEVEL_FASTER);
        PerformOneIterationOfCryPakTest(ICryArchive::FLAGS_READ_ONLY, ICryArchive::METHOD_COMPRESS, ICryArchive::LEVEL_NORMAL);
        PerformOneIterationOfCryPakTest(ICryArchive::FLAGS_READ_ONLY, ICryArchive::METHOD_COMPRESS, ICryArchive::LEVEL_BETTER);
        PerformOneIterationOfCryPakTest(ICryArchive::FLAGS_READ_ONLY, ICryArchive::METHOD_COMPRESS, ICryArchive::LEVEL_BEST);
        PerformOneIterationOfCryPakTest(ICryArchive::FLAGS_OPTIMIZED_READ_ONLY, ICryArchive::METHOD_COMPRESS, ICryArchive::LEVEL_FASTEST);
        PerformOneIterationOfCryPakTest(ICryArchive::FLAGS_OPTIMIZED_READ_ONLY, ICryArchive::METHOD_COMPRESS, ICryArchive::LEVEL_FASTER);
        PerformOneIterationOfCryPakTest(ICryArchive::FLAGS_OPTIMIZED_READ_ONLY, ICryArchive::METHOD_COMPRESS, ICryArchive::LEVEL_NORMAL);
        PerformOneIterationOfCryPakTest(ICryArchive::FLAGS_OPTIMIZED_READ_ONLY, ICryArchive::METHOD_COMPRESS, ICryArchive::LEVEL_BETTER);
        PerformOneIterationOfCryPakTest(ICryArchive::FLAGS_OPTIMIZED_READ_ONLY, ICryArchive::METHOD_COMPRESS, ICryArchive::LEVEL_BEST);
        PerformOneIterationOfCryPakTest((ICryArchive::EPakFlags)0, ICryArchive::METHOD_COMPRESS, ICryArchive::LEVEL_FASTEST);
        PerformOneIterationOfCryPakTest((ICryArchive::EPakFlags)0, ICryArchive::METHOD_COMPRESS, ICryArchive::LEVEL_FASTER);
        PerformOneIterationOfCryPakTest((ICryArchive::EPakFlags)0, ICryArchive::METHOD_COMPRESS, ICryArchive::LEVEL_NORMAL);
        PerformOneIterationOfCryPakTest((ICryArchive::EPakFlags)0, ICryArchive::METHOD_COMPRESS, ICryArchive::LEVEL_BETTER);
        PerformOneIterationOfCryPakTest((ICryArchive::EPakFlags)0, ICryArchive::METHOD_COMPRESS, ICryArchive::LEVEL_BEST);
    }

    TEST_F(Integ_CryPakUnitTests, TestCryPakPacking_Uncompressed)
    {
        PerformOneIterationOfCryPakTest(ICryArchive::FLAGS_OPTIMIZED_READ_ONLY, ICryArchive::METHOD_STORE, ICryArchive::LEVEL_BETTER);
        PerformOneIterationOfCryPakTest(ICryArchive::FLAGS_READ_ONLY, ICryArchive::METHOD_STORE, ICryArchive::LEVEL_BETTER);
        PerformOneIterationOfCryPakTest((ICryArchive::EPakFlags)0, ICryArchive::METHOD_STORE, ICryArchive::LEVEL_BETTER);
    }

    TEST_F(Integ_CryPakUnitTests, TestCryPakPacking_BiggerFiles)
    {
        // each of these totals many megabytes...
        PerformOneIterationOfCryPakTest(ICryArchive::FLAGS_READ_ONLY, ICryArchive::METHOD_COMPRESS, ICryArchive::LEVEL_BEST, 5555, 55, 5);
        PerformOneIterationOfCryPakTest(ICryArchive::FLAGS_OPTIMIZED_READ_ONLY, ICryArchive::METHOD_COMPRESS, ICryArchive::LEVEL_BEST, 5555, 77, 5);
        PerformOneIterationOfCryPakTest((ICryArchive::EPakFlags)0, ICryArchive::METHOD_COMPRESS, ICryArchive::LEVEL_BEST, 5555, 55, 5);
    }

    TEST_F(Integ_CryPakUnitTests, TestCryPakFolderAliases)
    {
        // test whether aliasing works as expected.  We'll create a pak in the cache, but we'll map it to a bunch of folders
        string testPakPath = "@cache@/paktest.pak";

        char realNameBuf[AZ_MAX_PATH_LEN] = { 0 };
        EXPECT_TRUE(gEnv->pFileIO->ResolvePath(testPakPath.c_str(), realNameBuf, AZ_MAX_PATH_LEN));

        ICryPak* pak = gEnv->pCryPak;

        EXPECT_TRUE(pak != nullptr);

        // delete test files in case they already exist
        pak->ClosePack(testPakPath.c_str());
        remove(testPakPath.c_str());

        // ------------ BASIC TEST:  Create and read Empty Archive ------------
        ICryArchive* pArchive = pak->OpenArchive(testPakPath.c_str(), nullptr, ICryArchive::FLAGS_CREATE_NEW);
        EXPECT_TRUE(pArchive != nullptr);

        char fillBuffer[32] = "Test";
        EXPECT_TRUE(pArchive->UpdateFile("foundit.dat", const_cast<char*>("test"), 4, ICryArchive::METHOD_COMPRESS, ICryArchive::LEVEL_BEST) == 0);

        delete pArchive;
        EXPECT_TRUE(IsPackValid(testPakPath.c_str()));

        EXPECT_TRUE(pak->OpenPack("@cache@", realNameBuf));
        EXPECT_TRUE(pak->IsFileExist("@cache@/foundit.dat"));
        EXPECT_TRUE(!pak->IsFileExist("@cache@/foundit.dat", ICryPak::eFileLocation_OnDisk));
        EXPECT_TRUE(!pak->IsFileExist("@cache@/notfoundit.dat"));
        EXPECT_TRUE(pak->ClosePack(realNameBuf));

        // change its actual location:
        EXPECT_TRUE(pak->OpenPack("@assets@", realNameBuf));
        EXPECT_TRUE(pak->IsFileExist("@assets@/foundit.dat"));
        EXPECT_TRUE(!pak->IsFileExist("@cache@/foundit.dat")); // do not find it in the previous location!
        EXPECT_TRUE(!pak->IsFileExist("@assets@/foundit.dat", ICryPak::eFileLocation_OnDisk));
        EXPECT_TRUE(!pak->IsFileExist("@assets@/notfoundit.dat"));
        EXPECT_TRUE(pak->ClosePack(realNameBuf));

        // try sub-folders
        EXPECT_TRUE(pak->OpenPack("@assets@/mystuff", realNameBuf));
        EXPECT_TRUE(pak->IsFileExist("@assets@/mystuff/foundit.dat"));
        EXPECT_TRUE(!pak->IsFileExist("@assets@/foundit.dat")); // do not find it in the previous locations!
        EXPECT_TRUE(!pak->IsFileExist("@cache@/foundit.dat")); // do not find it in the previous locations!
        EXPECT_TRUE(!pak->IsFileExist("@assets@/foundit.dat", ICryPak::eFileLocation_OnDisk));
        EXPECT_TRUE(!pak->IsFileExist("@assets@/mystuff/foundit.dat", ICryPak::eFileLocation_OnDisk));
        EXPECT_TRUE(!pak->IsFileExist("@assets@/notfoundit.dat")); // non-existent file
        EXPECT_TRUE(!pak->IsFileExist("@assets@/mystuff/notfoundit.dat")); // non-existent file
        EXPECT_TRUE(pak->ClosePack(realNameBuf));
    }

    TEST_F(Integ_CryPakUnitTests, IResourceList_Add_NullFileName_ReturnsDot)
    {
        // note that there are three different resource lists: { ICryPak::RFOM_EngineStartup, ICryPak::RFOM_Level, ICryPak::RFOM_NextLevel }
        // but the first two are the same exact type (CResourceList) and the third one is a special one that can only be deserialized from file and needs
        // to be tested separately.

        IResourceList* reslist = gEnv->pCryPak->GetResourceList(ICryPak::RFOM_EngineStartup);
        ASSERT_TRUE(reslist);

        reslist->Clear();
        reslist->Add(nullptr);
        EXPECT_STREQ(reslist->GetFirst(), ".");
        reslist->Clear();
    }

    TEST_F(Integ_CryPakUnitTests, IResourceList_Add_EmptyFileName_DoesNotCrash)
    {
        IResourceList* reslist = gEnv->pCryPak->GetResourceList(ICryPak::RFOM_EngineStartup);
        ASSERT_TRUE(reslist);

        reslist->Clear();
        reslist->Add("");
        EXPECT_STREQ(reslist->GetFirst(), "");
        reslist->Clear();
    }

    TEST_F(Integ_CryPakUnitTests, IResourceList_Add_RegularFileName_NormalizesAppropriately)
    {
        IResourceList* reslist = gEnv->pCryPak->GetResourceList(ICryPak::RFOM_EngineStartup);
        ASSERT_TRUE(reslist);

        reslist->Clear();
        reslist->Add("blah\\blah/AbCDE");

        // it normalizes the string, so the slashes flip and everything is lowercased.
        EXPECT_STREQ(reslist->GetFirst(), "blah/blah/abcde");
        reslist->Clear();
    }

    TEST_F(Integ_CryPakUnitTests, IResourceList_Add_ReallyShortFileName_NormalizesAppropriately)
    {
        IResourceList* reslist = gEnv->pCryPak->GetResourceList(ICryPak::RFOM_EngineStartup);
        ASSERT_TRUE(reslist);

        reslist->Clear();
        reslist->Add("A");

        // it normalizes the string, so the slashes flip and everything is lowercased.
        EXPECT_STREQ(reslist->GetFirst(), "a");
        reslist->Clear();
    }

    TEST_F(Integ_CryPakUnitTests, IResourceList_Add_AbsolutePath_RemovesAndReplacesWithAlias)
    {
        IResourceList* reslist = gEnv->pCryPak->GetResourceList(ICryPak::RFOM_EngineStartup);
        ASSERT_TRUE(reslist);

        AZ::IO::FileIOBase* ioBase = AZ::IO::FileIOBase::GetInstance();
        ASSERT_TRUE(ioBase);

        const char *assetsPath = ioBase->GetAlias("@assets@");
        ASSERT_TRUE(assetsPath);

        AZStd::string stringToAdd = AZStd::string::format("%s/textures/test.dds", assetsPath);

        reslist->Clear();
        reslist->Add(stringToAdd.c_str());

        // it normalizes the string, so the slashes flip and everything is lowercased.
        EXPECT_STREQ(reslist->GetFirst(), "@assets@/textures/test.dds");
        reslist->Clear();
    }

    TEST_F(Integ_CryPakUnitTests, IResourceList_Add_FuzzTest_DoesNotCrash)
    {
        IResourceList* reslist = gEnv->pCryPak->GetResourceList(ICryPak::RFOM_EngineStartup);
        ASSERT_TRUE(reslist);

        AZStd::string randomJunkName;
    
        randomJunkName.resize(128, '\0');

        for (int trialNumber = 0; trialNumber < s_numTrialsToPerform; ++trialNumber)
        {
            for (int randomChar = 0; randomChar < randomJunkName.size(); ++randomChar)
            {
                // note that this is intentionally allowing null characters to generate.
                // note that this also puts characters AFTER the null, if a null appears in the mddle.
                // so that if there are off by one errors they could include cruft afterwards.
                
                if (randomChar > trialNumber % randomJunkName.size())
                {
                    // choose this point for the nulls to begin.  It makes sure we test every size of string.
                    randomJunkName[randomChar] = 0;
                }
                else
                {
                    randomJunkName[randomChar] = (char)(rand() % 256); // this will trigger invalid UTF8 decoding too
                }
            }

            reslist->Clear();
            reslist->Add(randomJunkName.c_str());
            // something should have been added
            ASSERT_TRUE(reslist->GetFirst());
            // only one thing should have been added.
            ASSERT_FALSE(reslist->GetNext());
            // we dont actually care what it actually resolves to, we're just fuzzing for a crash!
            reslist->Clear();
        }
        
    }

#endif //defined(WIN32) || defined(WIN64)

    // Unit tests for CCryPak::BeautifyPath
    TEST(CryPakUnitTests, BeautifyPath_AliasedPathMakeLowerTrue_PathIsToLoweredSlashesReplaced)
    {
        char nativeSlash = CCryPak::g_cNativeSlash;

        AZStd::string aliasedPath = "@alias@/someDir/SomeFile.EXT";
        CCryPak::BeautifyPath(aliasedPath.data(), true);

        char expectedPath[AZ_MAX_PATH_LEN];
        azsnprintf(expectedPath, AZ_MAX_PATH_LEN, "@alias@%csomedir%csomefile.ext", nativeSlash, nativeSlash);

        EXPECT_STREQ(aliasedPath.data(), expectedPath);
    }

    TEST(CryPakUnitTests, BeautifyPath_AliasedPathMakeLowerFalse_PathNotToLoweredSlashesReplaced)
    {
        char nativeSlash = CCryPak::g_cNativeSlash;

        AZStd::string falseMakeLowerPath = "@alias@/someDir/SomeFile.EXT";
        CCryPak::BeautifyPath(falseMakeLowerPath.data(), false);

        char expectedPath[AZ_MAX_PATH_LEN];
        azsnprintf(expectedPath, AZ_MAX_PATH_LEN, "@alias@%csomeDir%cSomeFile.EXT", nativeSlash, nativeSlash);

        EXPECT_STREQ(falseMakeLowerPath.data(), expectedPath);
    }

    TEST(CryPakUnitTests, BeautifyPath_UnaliasedPath_PathIsToLoweredSlashesReplaced)
    {
        char nativeSlash = CCryPak::g_cNativeSlash;

        AZStd::string unaliasedPath = "someDir/SomeFile.EXT";
        CCryPak::BeautifyPath(unaliasedPath.data(), true);

        char expectedPath[AZ_MAX_PATH_LEN];
        azsnprintf(expectedPath, AZ_MAX_PATH_LEN, "somedir%csomefile.ext", nativeSlash);

        EXPECT_STREQ(unaliasedPath.data(), expectedPath);
    }

    TEST(CryPakUnitTests, BeautifyPath_CaseSensitiveAlias_AliasUnalteredButPathToLowered)
    {
        char nativeSlash = CCryPak::g_cNativeSlash;

        AZStd::string caseSensitiveAliasPath = "@ALiAs@/someDir/SomeFile.EXT";
        CCryPak::BeautifyPath(caseSensitiveAliasPath.data(), true);

        char expectedPath[AZ_MAX_PATH_LEN];
        azsnprintf(expectedPath, AZ_MAX_PATH_LEN, "@ALiAs@%csomedir%csomefile.ext", nativeSlash, nativeSlash);

        EXPECT_STREQ(caseSensitiveAliasPath.data(), expectedPath);
    }

    TEST(CryPakUnitTests, BeautifyPath_AbsolutePathMakeLowerTrue_LoweredAndSlashesSwitched)
    {
        char nativeSlash = CCryPak::g_cNativeSlash;
#if defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_APPLE_OSX)
        AZStd::string caseSensitiveAliasPath = "//absolutePath\\someDir\\SomeFile.EXT";
        CCryPak::BeautifyPath(caseSensitiveAliasPath.data(), true);

        constexpr const char* expectedPath = "/absolutepath" CRY_NATIVE_PATH_SEPSTR "somedir" CRY_NATIVE_PATH_SEPSTR "somefile.ext";
#else // WINDOWS
        AZStd::string caseSensitiveAliasPath = "C://absolutePath///////////////////////////////someDir/SomeFile.EXT";
        CCryPak::BeautifyPath(caseSensitiveAliasPath.data(), true);

        constexpr const char* expectedPath = "c:" CRY_NATIVE_PATH_SEPSTR "absolutepath" CRY_NATIVE_PATH_SEPSTR "somedir" CRY_NATIVE_PATH_SEPSTR "somefile.ext";
#endif // defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_APPLE_OSX)

        EXPECT_STREQ(caseSensitiveAliasPath.data(), expectedPath);
    }

    TEST(CryPakUnitTests, BeautifyPath_EmptyString_ReturnsNullTerminatedEmptyString)
    {
        char nativeSlash = CCryPak::g_cNativeSlash;

        AZStd::string emptyPath("");
        CCryPak::BeautifyPath(emptyPath.data(), true);
        
        EXPECT_STREQ(emptyPath.data(), "\0");
    }

    /*
    // Commenting this test out since CryPak tests do not support AZ_TEST_START_ASSERTTEST
    TEST(CryPakUnitTests, BeautifyPath_NullCharPtr_AssertsPathIsNullptr)
    {
        char nativeSlash = CCryPak::g_cNativeSlash;

        char* nullPath = nullptr;

        CCryPak::BeautifyPath(nullPath, true);
    }
    */

    class CryPakUnitTestsWithAllocators
        : public ::testing::Test
    {
    protected:

        void SetUp() override
        {
            AZ::AllocatorInstance<AZ::LegacyAllocator>::Create();
            AZ::AllocatorInstance<CryStringAllocator>::Create();
            m_localFileIO = aznew AZ::IO::LocalFileIO();
            AZ::IO::FileIOBase::SetDirectInstance(m_localFileIO);
            m_localFileIO->SetAlias(m_firstAlias.c_str(), m_firstAliasPath.c_str());
            m_localFileIO->SetAlias(m_secondAlias.c_str(), m_secondAliasPath.c_str());
        }

        void TearDown() override
        {
            AZ::IO::FileIOBase::SetDirectInstance(nullptr);
            delete m_localFileIO;
            m_localFileIO = nullptr;
            AZ::AllocatorInstance<CryStringAllocator>::Destroy();
            AZ::AllocatorInstance<AZ::LegacyAllocator>::Destroy();
        }

        AZ::IO::FileIOBase* m_localFileIO = nullptr;
        AZStd::string m_firstAlias = "@devassets@";
        AZStd::string m_firstAliasPath = "devassets_absolutepath";
        AZStd::string m_secondAlias = "@assets@";
        AZStd::string m_secondAliasPath = "assets_absolutepath";
    };

    // ConvertAbsolutePathToAliasedPath tests are built to verify existing behavior doesn't change.
    // Tt's a legacy function and the actual intended behavior is unknown, so these are black box unit tests.
    TEST_F(CryPakUnitTestsWithAllocators, ConvertAbsolutePathToAliasedPath_NullString_ReturnsSuccess)
    {
        AZ::Outcome<string, AZStd::string> conversionResult = CryPakInternal::ConvertAbsolutePathToAliasedPath(nullptr);
        EXPECT_TRUE(conversionResult.IsSuccess());
        // ConvertAbsolutePathToAliasedPath returns string(sourcePath) if sourcePath is nullptr.
        string fromNull(nullptr);
        EXPECT_EQ(conversionResult.GetValue().compare(fromNull),0);
    }

    TEST_F(CryPakUnitTestsWithAllocators, ConvertAbsolutePathToAliasedPath_NoAliasInSource_ReturnsSource)
    {
        AZStd::string sourceString("NoAlias");
        AZ::Outcome<string, AZStd::string> conversionResult = CryPakInternal::ConvertAbsolutePathToAliasedPath(sourceString.c_str());
        EXPECT_TRUE(conversionResult.IsSuccess());
        // ConvertAbsolutePathToAliasedPath returns sourceString if there is no alias in the source.
        EXPECT_EQ(conversionResult.GetValue().compare(sourceString.c_str()), 0);
    }

    TEST_F(CryPakUnitTestsWithAllocators, ConvertAbsolutePathToAliasedPath_NullAliasToLookFor_ReturnsSource)
    {
        AZStd::string sourceString("NoAlias");
        AZ::Outcome<string, AZStd::string> conversionResult = 
            CryPakInternal::ConvertAbsolutePathToAliasedPath(sourceString.c_str(), nullptr);
        EXPECT_TRUE(conversionResult.IsSuccess());
        EXPECT_EQ(conversionResult.GetValue().compare(sourceString.c_str()), 0);
    }

    TEST_F(CryPakUnitTestsWithAllocators, ConvertAbsolutePathToAliasedPath_NullAliasToReplaceWith_ReturnsSource)
    {
        AZStd::string sourceString("NoAlias");
        AZ::Outcome<string, AZStd::string> conversionResult =
            CryPakInternal::ConvertAbsolutePathToAliasedPath(sourceString.c_str(), "@SomeAlias", nullptr);
        EXPECT_TRUE(conversionResult.IsSuccess());
        EXPECT_EQ(conversionResult.GetValue().compare(sourceString.c_str()), 0);
    }

    TEST_F(CryPakUnitTestsWithAllocators, ConvertAbsolutePathToAliasedPath_NullAliases_ReturnsSource)
    {
        AZStd::string sourceString("NoAlias");
        AZ::Outcome<string, AZStd::string> conversionResult = 
            CryPakInternal::ConvertAbsolutePathToAliasedPath(sourceString.c_str(), nullptr, nullptr);
        EXPECT_TRUE(conversionResult.IsSuccess());
        EXPECT_EQ(conversionResult.GetValue().compare(sourceString.c_str()), 0);
    }

    TEST_F(CryPakUnitTestsWithAllocators, ConvertAbsolutePathToAliasedPath_AbsPathInSource_ReturnsReplacedAlias)
    {
        // ConvertAbsolutePathToAliasedPath only replaces data if GetDirectInstance is valid.
        EXPECT_TRUE(AZ::IO::FileIOBase::GetDirectInstance() != nullptr);
        
        const char* fullPath = AZ::IO::FileIOBase::GetDirectInstance()->GetAlias(m_firstAlias.c_str());
        AZStd::string sourceString = AZStd::string::format("%sSomeStringWithAlias", fullPath);
        AZStd::string expectedResult = AZStd::string::format("%s" CRY_NATIVE_PATH_SEPSTR "SomeStringWithAlias", m_secondAlias.c_str());

        AZ::Outcome<string, AZStd::string> conversionResult = CryPakInternal::ConvertAbsolutePathToAliasedPath(
            sourceString.c_str(), 
            m_firstAlias.c_str(),  // find any instance of FirstAlias in sourceString
            m_secondAlias.c_str());  // replace it with SecondAlias
        EXPECT_TRUE(conversionResult.IsSuccess());
        EXPECT_STREQ(conversionResult.GetValue().c_str(), expectedResult.c_str());
    }

    TEST_F(CryPakUnitTestsWithAllocators, ConvertAbsolutePathToAliasedPath_AliasInSource_ReturnsReplacedAlias)
    {
        // ConvertAbsolutePathToAliasedPath only replaces data if GetDirectInstance is valid.
        EXPECT_TRUE(AZ::IO::FileIOBase::GetDirectInstance() != nullptr);

        AZStd::string sourceString = AZStd::string::format("%sSomeStringWithAlias", m_firstAlias.c_str());
        AZStd::string expectedResult = AZStd::string::format("%s" CRY_NATIVE_PATH_SEPSTR "SomeStringWithAlias", m_secondAlias.c_str());
        
        AZ::Outcome<string, AZStd::string> conversionResult = CryPakInternal::ConvertAbsolutePathToAliasedPath(
            sourceString.c_str(), 
            m_firstAlias.c_str(),  // find any instance of FirstAlias in sourceString
            m_secondAlias.c_str());  // replace it with SecondAlias
        EXPECT_TRUE(conversionResult.IsSuccess());
        EXPECT_STREQ(conversionResult.GetValue().c_str(), expectedResult.c_str());
    }

    TEST_F(CryPakUnitTestsWithAllocators, ConvertAbsolutePathToAliasedPath_AbsPathInSource_DOSSlashInSource_ReturnsReplacedAlias)
    {
        // ConvertAbsolutePathToAliasedPath only replaces data if GetDirectInstance is valid.
        EXPECT_TRUE(AZ::IO::FileIOBase::GetDirectInstance() != nullptr);
        
        const char* fullPath = AZ::IO::FileIOBase::GetDirectInstance()->GetAlias(m_firstAlias.c_str());
        AZStd::string sourceString = AZStd::string::format("%s" DOS_PATH_SEP_STR "SomeStringWithAlias", fullPath);
        AZStd::string expectedResult = AZStd::string::format("%s" CRY_NATIVE_PATH_SEPSTR "SomeStringWithAlias", m_secondAlias.c_str());
        AZ::Outcome<string, AZStd::string> conversionResult = CryPakInternal::ConvertAbsolutePathToAliasedPath(
            sourceString.c_str(), 
            m_firstAlias.c_str(),  // find any instance of FirstAlias in sourceString
            m_secondAlias.c_str());  // replace it with SecondAlias
        EXPECT_TRUE(conversionResult.IsSuccess());
        EXPECT_STREQ(conversionResult.GetValue().c_str(), expectedResult.c_str());
    }

    TEST_F(CryPakUnitTestsWithAllocators, ConvertAbsolutePathToAliasedPath_AbsPathInSource_UNIXSlashInSource_ReturnsReplacedAlias)
    {
        // ConvertAbsolutePathToAliasedPath only replaces data if GetDirectInstance is valid.
        EXPECT_TRUE(AZ::IO::FileIOBase::GetDirectInstance() != nullptr);

        const char* fullPath = AZ::IO::FileIOBase::GetDirectInstance()->GetAlias(m_firstAlias.c_str());
        AZStd::string sourceString = AZStd::string::format("%s" UNIX_PATH_SEP_STR "SomeStringWithAlias", fullPath);
        AZStd::string expectedResult = AZStd::string::format("%s" CRY_NATIVE_PATH_SEPSTR "SomeStringWithAlias", m_secondAlias.c_str());
        AZ::Outcome<string, AZStd::string> conversionResult = CryPakInternal::ConvertAbsolutePathToAliasedPath(
            sourceString.c_str(), 
            m_firstAlias.c_str(),  // find any instance of FirstAlias in sourceString
            m_secondAlias.c_str());  // replace it with SecondAlias
        EXPECT_TRUE(conversionResult.IsSuccess());
        EXPECT_STREQ(conversionResult.GetValue().c_str(), expectedResult.c_str());
    }

    TEST_F(CryPakUnitTestsWithAllocators, ConvertAbsolutePathToAliasedPath_AliasInSource_DOSSlashInSource_ReturnsReplacedAlias)
    {
        // ConvertAbsolutePathToAliasedPath only replaces data if GetDirectInstance is valid.
        EXPECT_TRUE(AZ::IO::FileIOBase::GetDirectInstance() != nullptr);

        AZStd::string sourceString = AZStd::string::format("%s" DOS_PATH_SEP_STR "SomeStringWithAlias", m_firstAlias.c_str());
        AZStd::string expectedResult = AZStd::string::format("%s" CRY_NATIVE_PATH_SEPSTR "SomeStringWithAlias", m_secondAlias.c_str());

        // sourceString is now (firstAlias)SomeStringWithAlias

        AZ::Outcome<string, AZStd::string> conversionResult = CryPakInternal::ConvertAbsolutePathToAliasedPath(
            sourceString.c_str(), 
            m_firstAlias.c_str(),  // find any instance of FirstAlias in sourceString
            m_secondAlias.c_str());  // replace it with SecondAlias
        EXPECT_TRUE(conversionResult.IsSuccess());
        EXPECT_STREQ(conversionResult.GetValue().c_str(), expectedResult.c_str());
    }

    TEST_F(CryPakUnitTestsWithAllocators, ConvertAbsolutePathToAliasedPath_AliasInSource_UNIXSlashInSource_ReturnsReplacedAlias)
    {
        // ConvertAbsolutePathToAliasedPath only replaces data if GetDirectInstance is valid.
        EXPECT_TRUE(AZ::IO::FileIOBase::GetDirectInstance() != nullptr);

        AZStd::string sourceString = AZStd::string::format("%s" UNIX_PATH_SEP_STR "SomeStringWithAlias", m_firstAlias.c_str());
        AZStd::string expectedResult = AZStd::string::format("%s" CRY_NATIVE_PATH_SEPSTR "SomeStringWithAlias", m_secondAlias.c_str());

        // sourceString is now (firstAlias)SomeStringWithAlias

        AZ::Outcome<string, AZStd::string> conversionResult = CryPakInternal::ConvertAbsolutePathToAliasedPath(
            sourceString.c_str(), 
            m_firstAlias.c_str(),  // find any instance of FirstAlias in sourceString
            m_secondAlias.c_str());  // replace it with SecondAlias
        EXPECT_TRUE(conversionResult.IsSuccess());
        EXPECT_STREQ(conversionResult.GetValue().c_str(), expectedResult.c_str());
    }

    TEST_F(CryPakUnitTestsWithAllocators, ConvertAbsolutePathToAliasedPath_SourceLongerThanMaxPath_ReturnsFailure)
    {
        const int longPathArraySize = AZ_MAX_PATH_LEN + 2;
        char longPath[longPathArraySize];
        memset(longPath, 'a', sizeof(char)*longPathArraySize);
        longPath[longPathArraySize-1] = '\0';
        AZ::Outcome<string, AZStd::string> conversionResult = CryPakInternal::ConvertAbsolutePathToAliasedPath(longPath);
        EXPECT_FALSE(conversionResult.IsSuccess());
    }
}
