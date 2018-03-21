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

#include "TestTypes.h"
#include "FileIOBaseTestTypes.h"

#include <AzCore/std/functional.h>

#include <AzCore/IO/Streamer.h>
#include <AzCore/IO/StreamerUtil.h>
#include <AzCore/IO/FileIOEventBus.h>
#include <AzCore/IO/Device.h>
#include <AzCore/IO/CompressorZLib.h>
#include <AzCore/IO/VirtualStream.h>

#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/Memory/AllocationRecords.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/Casting/numeric_cast.h>

#include <AzCore/Math/Sfmt.h>

#include <AzCore/IO/CompressorStream.h>
#include <AzCore/XML/rapidxml.h>

using namespace AZ;
using namespace AZ::IO;
using namespace AZ::Debug;
using namespace AZStd;

#if   defined(AZ_PLATFORM_APPLE_IOS)
#   define AZ_ROOT_TEST_FOLDER  "/Documents/"
#elif defined(AZ_PLATFORM_APPLE_TV)
#   define AZ_ROOT_TEST_FOLDER "/Library/Caches/"
#elif defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_APPLE_OSX)
#   define AZ_ROOT_TEST_FOLDER  "./"
#elif defined(AZ_PLATFORM_ANDROID)
#   define AZ_ROOT_TEST_FOLDER  "/sdcard/"
#else
#   define AZ_ROOT_TEST_FOLDER  ""
#endif

namespace // anonymous
{
#if defined(AZ_PLATFORM_APPLE_IOS) || defined(AZ_PLATFORM_APPLE_TV)
    AZStd::string GetTestFolderPath()
    {
        return AZStd::string(getenv("HOME")) + AZ_ROOT_TEST_FOLDER;
    }
#else
    AZStd::string GetTestFolderPath()
    {
        return AZ_ROOT_TEST_FOLDER;
    }
#endif
} // anonymous namespace

// If you are running the performance tests, you need to set the
// following defines to whatever environment you want to test.
// The default settings below assume that there are 5 files
// at the root of the disc (in the xgd), the first 4 being 5MB big
// and the last one 5MB + enough spare data to be skipped.
// To run the test, setup your xgd as necessary, run the emulator in
// accurate mode and enable DVD emulation in the debug options.
//#define ENABLE_PERFORMANCE_TEST
#ifdef ENABLE_PERFORMANCE_TEST
#       define AZ_DVD_ROOT          "e:\\"
#       define AZ_NET_ROOT          "\\\\18pcheng2\\AZCORE_DATA\\"
#       define AZ_HDD_ROOT          ""
#   define LARGE_READ_TEST_FILE         AZ_DVD_ROOT "FILE1.IMG"  // File size should be at least TOTAL_READ_SIZE.
#   define SMALL_READ_TEST_FILE         AZ_DVD_ROOT "FILE2.IMG"  // File size should be at least TOTAL_READ_SIZE.
#   define INTERLEAVED_READ_TEST_FILE1  AZ_DVD_ROOT "FILE3.IMG"  // File size should be at least TOTAL_READ_SIZE.
#   define INTERLEAVED_READ_TEST_FILE2  AZ_DVD_ROOT "FILE4.IMG"  // File size should be at least TOTAL_READ_SIZE.
#   define UNALIGNED_READ_TEST_FILE     AZ_DVD_ROOT "FILE5.IMG"  // File size should be at least TOTAL_READ_SIZE + (TOTAL_READ_SIZE / READ_CHUNK_SIZE_1) * ECC_SIZE.
#   define READ_CHUNK_SIZE_1            4096
#   define READ_CHUNK_SIZE_2            1024
#   define ECC_SIZE                     (32 * 1024)
#   define TOTAL_READ_SIZE              (5 * 1024 * 1024)
#endif

namespace UnitTest
{

    /**
     *
     */
    class FileStreamTest
        : public AllocatorsFixture
        , public FileIOEventBus::Handler
    {
        int m_fileErrors = 0;
        static TestFileIOBase m_fileIO;
        static AZ::IO::FileIOBase* m_prevFileIO;
    public:

        void SetUp() override
        {
            AllocatorsFixture::SetUp();

            AZ::AllocatorInstance<AZ::PoolAllocator>::Create();
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();
            
            m_prevFileIO = FileIOBase::GetInstance();
            FileIOBase::SetInstance(&m_fileIO);

            BusConnect();
        }

        void TearDown() override
        {
            BusDisconnect();
            FileIOBase::SetInstance(m_prevFileIO);

            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
            AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();

            AllocatorsFixture::TearDown();
        }

        virtual void OnError(const SystemFile* file, const char* fileName, int errorCode)
        {
            (void)file;
            (void)fileName;
            (void)errorCode;
            AZ_TracePrintf("StreamerTest", "IO Error (%d)!\n", errorCode);
            m_fileErrors++;
        }

        void CycleOpenReadClose(const char** filenames, size_t n, size_t first, size_t id, size_t nIter)
        {
            (void)id;
            size_t idx = first;
            for (size_t i = 0; i < nIter; ++i)
            {
                VirtualStream* stream = GetStreamer()->RegisterFileStream(filenames[idx], OpenMode::ModeRead, false);
                if (stream)
                {
                    char byte;
                    GetStreamer()->Read(stream, 0, 1, &byte);
                    GetStreamer()->UnRegisterStream(stream);
                }
                else
                {
                    AZ_TracePrintf("FileStreamTest", "CycleOpenReadClose %d failed to open %s.\n", (int)id, filenames[idx]);
                }
                idx = (idx + 1) % n;
                //if (i % 5000 == 0) {
                //  AZ_TracePrintf("FileStreamTest", "CycleOpenReadClose %d completed %d runs.\n", (int)id, (int)i);
                //}
            }
            AZ_TracePrintf("FileStreamTest", "CycleOpenReadClose %d completed %d runs.\n", (int)id, (int)nIter);
        }

        void run()
        {
            Streamer::Descriptor desc;
            desc.m_maxNumOpenLimitedStream = 2; //< to test limited resource code
            AZStd::string testFolder = GetTestFolderPath();
            if (testFolder.length() > 0)
            {
                desc.m_fileMountPoint = testFolder.c_str();
            }
            Streamer::Create(desc);

            SyncRequestCallback doneCB;
            u32 toWrite = 0xbad0babe;

            SizeType bytesTransfered;
            Request::StateType operationState;

            AZStd::string fileName1 = AZStd::string::format("%s%itest.dat", GetTestFolderPath(), ::testing::UnitTest::GetInstance()->random_seed());;
            AZStd::string fileName2 = AZStd::string::format("%s%itest1.dat", GetTestFolderPath(), ::testing::UnitTest::GetInstance()->random_seed());;
            AZStd::string fileName3 = AZStd::string::format("%s%itest2.dat", GetTestFolderPath(), ::testing::UnitTest::GetInstance()->random_seed());;
            AZStd::string fileName4 = AZStd::string::format("%s%itest3.dat", GetTestFolderPath(), ::testing::UnitTest::GetInstance()->random_seed());;


            const char* testFileNames[] = {
                fileName1.c_str(),
                fileName2.c_str(),
                fileName3.c_str(),
                fileName4.c_str(),
            };
            const size_t nTestFiles = AZ_ARRAY_SIZE(testFileNames);


            VirtualStream* fileStream = GetStreamer()->RegisterFileStream(testFileNames[0], OpenMode::ModeWrite, false);
            AZ_TEST_ASSERT(m_fileErrors == 0);
            AZ_TEST_ASSERT(fileStream != NULL);

            bytesTransfered = GetStreamer()->Write(fileStream, &toWrite, sizeof(toWrite), Request::PriorityType::DR_PRIORITY_NORMAL, &operationState);
            AZ_TEST_ASSERT(m_fileErrors == 0);
            AZ_TEST_ASSERT(bytesTransfered == sizeof(toWrite));
            AZ_TEST_ASSERT(operationState == Request::StateType::ST_COMPLETED);

            u32 arrayWithData[100];
            for (unsigned int i = 0; i < AZ_ARRAY_SIZE(arrayWithData); ++i)
            {
                arrayWithData[i] = i << 24 | i << 16 | i << 8 | i;
                GetStreamer()->WriteAsync(fileStream, &arrayWithData[i], sizeof(u32), doneCB);
            }
            doneCB.Wait();
            AZ_TEST_ASSERT(doneCB.m_state == Request::StateType::ST_COMPLETED);
            AZ_TEST_ASSERT(fileStream->GetLength() == sizeof(u32) * (AZ_ARRAY_SIZE(arrayWithData) + 1));

            GetStreamer()->UnRegisterStream(fileStream);

            fileStream = GetStreamer()->RegisterFileStream(testFileNames[0], OpenMode::ModeRead, false);
            AZ_TEST_ASSERT(m_fileErrors == 0);
            AZ_TEST_ASSERT(fileStream != NULL);

            u32 toRead;
            bytesTransfered = GetStreamer()->Read(fileStream, 0, sizeof(toRead), &toRead, AZStd::chrono::microseconds::max(), Request::PriorityType::DR_PRIORITY_NORMAL, &operationState);
            AZ_TEST_ASSERT(m_fileErrors == 0);
            AZ_TEST_ASSERT(bytesTransfered == sizeof(toRead));
            AZ_TEST_ASSERT(operationState == Request::StateType::ST_COMPLETED);
            AZ_TEST_ASSERT(toRead == toWrite);

            GetStreamer()->UnRegisterStream(fileStream);

            //////////////////////////////////////////////////////////////////////////
            // Request sort test
            fileStream = GetStreamer()->RegisterFileStream(testFileNames[0], OpenMode::ModeRead, false);
            AZ_TEST_ASSERT(m_fileErrors == 0);
            AZ_TEST_ASSERT(fileStream != NULL);

            // 0 - read the start
            GetStreamer()->ReadAsync(fileStream, 0, sizeof(toRead), &toRead, doneCB);
            // 1 - load numbers 0 to 49
            GetStreamer()->ReadAsync(fileStream, 4, 50 * sizeof(u32), &arrayWithData[0], doneCB);
            // 2 - load number 50 to 85
            GetStreamer()->ReadAsync(fileStream, 4 + 50 * sizeof(u32), 35 * sizeof(u32), &arrayWithData[50], doneCB);
            // 3 - load 100
            GetStreamer()->ReadAsync(fileStream, 4 + 85 * sizeof(u32), 15 * sizeof(u32), &arrayWithData[85], doneCB);

            // data is too close for a seek to matter < 32 KB so the order should be based on the size (since the deadline is set no default)
            // so the order of execution should be 0,3,2,1 or 3,2,1,0 depending on first request being sent before we can sort.
            doneCB.Wait();

            for (unsigned int i = 0; i < AZ_ARRAY_SIZE(arrayWithData); ++i)
            {
                u32 testNum = i << 24 | i << 16 | i << 8 | i;
                AZ_TEST_ASSERT(arrayWithData[i] == testNum);
            }

            GetStreamer()->UnRegisterStream(fileStream);
            //////////////////////////////////////////////////////////////////////////


            // make sure when we limit the number of file stream, the streamer respect the number of file streams.
            VirtualStream* fileStream1 = GetStreamer()->RegisterFileStream(testFileNames[1], OpenMode::ModeWrite, true);
            AZ_TEST_ASSERT(m_fileErrors == 0);
            AZ_TEST_ASSERT(fileStream1 != NULL);
            // register stream "allowing" URL without actually having one (should be treated as a file)
            VirtualStream* fileStream2 = GetStreamer()->RegisterFileStream(testFileNames[2], OpenMode::ModeWrite, true);
            AZ_TEST_ASSERT(m_fileErrors == 0);
            AZ_TEST_ASSERT(fileStream2 != NULL);
            // register stream as a file. URL test is no longer needed as the Device uses a reference to a FileIOBase for path and alias resolution
            VirtualStream* fileStream3 = GetStreamer()->RegisterFileStream(testFileNames[3], OpenMode::ModeWrite, true);
            AZ_TEST_ASSERT(m_fileErrors == 0);
            AZ_TEST_ASSERT(fileStream3 != NULL);

            AZ_TEST_ASSERT(!fileStream1->IsOpen()); // Because m_maxNumOpenLimitedStream is 2 only the latest two registered streams should be opened
            // The limited flag is now stored next to the registered stream in the Device object and is no longer on the stream object itself
            AZ_TEST_ASSERT(fileStream2->IsOpen()); // file resources are not open till first use
            AZ_TEST_ASSERT(fileStream3->IsOpen()); // file resources are not open till first use

            bytesTransfered = GetStreamer()->Write(fileStream1, &toWrite, sizeof(toWrite),Request::PriorityType::DR_PRIORITY_NORMAL, &operationState);
            AZ_TEST_ASSERT(m_fileErrors == 0);
            AZ_TEST_ASSERT(bytesTransfered == sizeof(toWrite));
            AZ_TEST_ASSERT(operationState == Request::StateType::ST_COMPLETED);

            AZ_TEST_ASSERT(fileStream1->IsOpen()); // now the stream should be kept open

            bytesTransfered = GetStreamer()->Write(fileStream2, &toWrite, sizeof(toWrite),Request::PriorityType::DR_PRIORITY_NORMAL, &operationState);
            AZ_TEST_ASSERT(m_fileErrors == 0);
            AZ_TEST_ASSERT(bytesTransfered == sizeof(toWrite));
            AZ_TEST_ASSERT(operationState == Request::StateType::ST_COMPLETED);

            AZ_TEST_ASSERT(fileStream1->IsOpen()); // since we allow 2 open streams this should still be open
            AZ_TEST_ASSERT(fileStream2->IsOpen()); // now the stream should be kept open

            bytesTransfered = GetStreamer()->Write(fileStream3, &toWrite, sizeof(toWrite),Request::PriorityType::DR_PRIORITY_NORMAL, &operationState);
            AZ_TEST_ASSERT(m_fileErrors == 0);
            AZ_TEST_ASSERT(bytesTransfered == sizeof(toWrite));
            AZ_TEST_ASSERT(operationState == Request::StateType::ST_COMPLETED);

            AZ_TEST_ASSERT(!fileStream1->IsOpen());
            AZ_TEST_ASSERT(fileStream2->IsOpen());
            AZ_TEST_ASSERT(fileStream3->IsOpen());

            GetStreamer()->UnRegisterStream(fileStream1);
            GetStreamer()->UnRegisterStream(fileStream2);
            GetStreamer()->UnRegisterStream(fileStream3);


            //
            // concurrent open/close stress test
            //
            // one test thread
            const size_t numOperations = 50000;

            CycleOpenReadClose(testFileNames, nTestFiles, 0, 0, numOperations);

            // two test threads
            {
                const size_t maxThreads = 2;
                AZStd::thread   test_threads[maxThreads];
                AZStd::thread_desc td;
                for (size_t i = 0; i < maxThreads; ++i)
                {
                    test_threads[i] = AZStd::thread(AZStd::bind(&FileStreamTest::CycleOpenReadClose, this, testFileNames, 1, i % 1, i, numOperations / 10), &td);
                }
                for (size_t i = 0; i < maxThreads; ++i)
                {
                    test_threads[i].join();
                }
            }

            // six test threads
            {
                const size_t maxThreads = 6;
                AZStd::thread   test_threads[maxThreads];
                AZStd::thread_desc td;
                for (size_t i = 0; i < maxThreads; ++i)
                {
                    test_threads[i] = AZStd::thread(AZStd::bind(&FileStreamTest::CycleOpenReadClose, this, testFileNames, nTestFiles, i % nTestFiles, i, numOperations / 20), &td);
                }
                for (size_t i = 0; i < maxThreads; ++i)
                {
                    test_threads[i].join();
                }
            }

            Streamer::Destroy();
        }
    };

    TestFileIOBase FileStreamTest::m_fileIO;
    FileIOBase* FileStreamTest::m_prevFileIO = nullptr;

    TEST_F(FileStreamTest, Test)
    {
        run();
    }

#ifdef ENABLE_PERFORMANCE_TEST
    /**
     *
     */
    class PERF_FileStreamPerformanceTest
        : public AllocatorsFixture
        , public FileIOEventBus::Handler
    {
        int m_fileErrors = 0;
        static TestFileIOBase m_fileIO;
        static AZ::IO::FileIOBase* m_prevFileIO;
    public:
   
        void SetUp() override
        {
            AllocatorsFixture::SetUp();

            AZ::AllocatorInstance<AZ::PoolAllocator>::Create();
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();

            m_prevFileIO = FileIOBase::GetInstance();
            FileIOBase::SetInstance(&m_fileIO);

            BusConnect();
        }

        void TearDown() override
        {
            BusDisconnect();

            FileIOBase::SetInstance(m_prevFileIO);

            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
            AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();

            AllocatorsFixture::TearDown();
        }

        virtual void OnError(const SystemFile* file, const char* fileName, int errorCode)
        {
            (void)file;
            (void)fileName;
            (void)errorCode;
            AZ_TracePrintf("FileStreamPerformanceTest", "IO Error (%d)!\n", errorCode);
            m_fileErrors++;
        }

        void InterleavedRead(Streamer* streamer, VirtualStream* st, int chunk, int total)
        {
            (void)streamer; // whatever msvcc...this is used 5 lines below...
            // Interleaved Read
            Request::StateType operationState;
            int completed = 0;
            char* buf = (char*)azmalloc(chunk);
            streamer->Read(st, completed, chunk, buf, IO::ExecuteWhenIdle,Request::PriorityType::DR_PRIORITY_NORMAL, &operationState);
            while (completed < total)
            {
                int read = (int)streamer->Read(st, completed, chunk, buf, IO::ExecuteWhenIdle,Request::PriorityType::DR_PRIORITY_NORMAL, &operationState);
                completed += read;
            }
            azfree(buf);
        }

        void run()
        {
            // wait for system to stabilize
            AZ_TracePrintf("FileStreamPerformanceTest", "Waiting for system to stabilize...\n");
            AZStd::this_thread::sleep_for(chrono::seconds(10));

            Streamer::Descriptor desc;
            desc.m_fileMountPoint = NULL;
            Streamer::Create(desc);

            const int len = TOTAL_READ_SIZE;
            const int chunk1 = READ_CHUNK_SIZE_1;
            const int chunk2 = READ_CHUNK_SIZE_2;

            char* buf1 = (char*)azmalloc(len);
            char* buf2 = (char*)azmalloc(len);

            // Large Block Read
            {
                VirtualStream* st = GetStreamer()->RegisterFileStream(LARGE_READ_TEST_FILE, OpenMode::ModeRead, false);
                Request::StateType operationState;

                chrono::system_clock::time_point start = chrono::system_clock::now();
                int read = (int)GetStreamer()->Read(st, 0, len, buf1, IO::ExecuteWhenIdle, Request::PriorityType::DR_PRIORITY_NORMAL, &operationState);
                AZ_TEST_ASSERT(operationState == Request::StateType::ST_COMPLETED);
                AZ_TEST_ASSERT(read == len);
                GetStreamer()->UnRegisterStream(st);
                chrono::system_clock::time_point end = chrono::system_clock::now();
                AZ_TracePrintf("FileStreamPerformanceTest", "Full read test: %d bytes in %dms.\n", len, (int)chrono::milliseconds(end - start).count());
            }

            // Small Block Read
            {
                VirtualStream* st = GetStreamer()->RegisterFileStream(SMALL_READ_TEST_FILE, OpenMode::ModeRead, false);
                Request::StateType operationState;
                int completed = 0;

                chrono::system_clock::time_point start = chrono::system_clock::now();
                while (completed < len)
                {
                    int read = (int)GetStreamer()->Read(st, completed, chunk1, buf1 + completed, IO::ExecuteWhenIdle,Request::PriorityType::DR_PRIORITY_NORMAL, &operationState);
                    AZ_TEST_ASSERT(operationState == Request::StateType::ST_COMPLETED);
                    AZ_TEST_ASSERT(read == chunk1);
                    completed += read;
                }
                GetStreamer()->UnRegisterStream(st);
                chrono::system_clock::time_point end = chrono::system_clock::now();
                AZ_TracePrintf("FileStreamPerformanceTest", "Small chunk(%d) read test: %d bytes in %dms.\n", chunk1, len, (int)chrono::milliseconds(end - start).count());
            }

            // Interleaved Read
            {
                VirtualStream* st1 = GetStreamer()->RegisterFileStream(INTERLEAVED_READ_TEST_FILE1, OpenMode::ModeRead, false);
                VirtualStream* st2 = GetStreamer()->RegisterFileStream(INTERLEAVED_READ_TEST_FILE2, OpenMode::ModeRead, false);
                chrono::system_clock::time_point start = chrono::system_clock::now();
                AZStd::thread_desc td;
                td.m_name = "InterleavedReadThread1";
                AZStd::thread thread1 = AZStd::thread(AZStd::bind(&PERF_FileStreamPerformanceTest::InterleavedRead, this, GetStreamer(), st1, chunk1, len), &td);
                td.m_name = "InterleavedReadThread2";
                AZStd::thread thread2 = AZStd::thread(AZStd::bind(&PERF_FileStreamPerformanceTest::InterleavedRead, this, GetStreamer(), st2, chunk2, len), &td);
                thread1.join();
                thread2.join();
                chrono::system_clock::time_point end = chrono::system_clock::now();
                AZ_TracePrintf("FileStreamPerformanceTest", "Interleaved read test (%d, %d), (%d, %d): %dms.\n", len, chunk1, len, chunk2, (int)chrono::milliseconds(end - start).count());
                GetStreamer()->UnRegisterStream(st1);
                GetStreamer()->UnRegisterStream(st2);
            }

            // Seek/Read
            {
                VirtualStream* st = GetStreamer()->RegisterFileStream(UNALIGNED_READ_TEST_FILE, OpenMode::ModeRead, false);
                Request::StateType operationState;
                int completed = 0;
                int offset = ECC_SIZE / 2;

                chrono::system_clock::time_point start = chrono::system_clock::now();
                while (completed < len)
                {
                    int read = (int)GetStreamer()->Read(st, offset, chunk1, buf1 + completed, IO::ExecuteWhenIdle,Request::PriorityType::DR_PRIORITY_NORMAL, &operationState);
                    AZ_TEST_ASSERT(operationState == Request::StateType::ST_COMPLETED);
                    AZ_TEST_ASSERT(read == chunk1);
                    completed += read;
                    offset += ECC_SIZE;
                }
                GetStreamer()->UnRegisterStream(st);
                chrono::system_clock::time_point end = chrono::system_clock::now();
                AZ_TracePrintf("FileStreamPerformanceTest", "Unaligned read test: %d bytes in %dms.\n", len, (int)chrono::milliseconds(end - start).count());
            }

            azfree(buf1);
            azfree(buf2);

            Streamer::Destroy();
        }
    };

    TestFileIOBase PERF_FileStreamPerformanceTest::m_fileIO;
    AZ::IO::FileIOBase* PERF_FileStreamPerformanceTest::m_prevFileIO = nullptr;

    TEST_F(PERF_FileStreamPerformanceTest, Test)
    {
        run();
    }
#endif // ENABLE_PERFORMANCE_TEST

    /**
     * Testing the main device read/write speed.
     */
    class PERF_FileReadWriteSpeedTest
        : public AllocatorsFixture
        , public FileIOEventBus::Handler
    {
        int m_fileErrors = 0;
        AZStd::atomic_int m_numRequestsToProcess;
        static TestFileIOBase m_fileIO;
        static AZ::IO::FileIOBase* m_prevFileIO;
    public:
        PERF_FileReadWriteSpeedTest()
            : m_numRequestsToProcess(0)
        {}

        void SetUp() override
        {
            m_prevFileIO = FileIOBase::GetInstance();
            FileIOBase::SetInstance(&m_fileIO);

            AllocatorsFixture::SetUp();

            AZ::AllocatorInstance<AZ::PoolAllocator>::Create();
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();

            BusConnect();
        }

        void TearDown() override
        {
            BusDisconnect();

            FileIOBase::SetInstance(m_prevFileIO);

            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
            AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();

            AllocatorsFixture::TearDown();
        }

        virtual void OnError(const SystemFile* file, const char* fileName, int errorCode)
        {
            (void)file;
            (void)fileName;
            (void)errorCode;
            AZ_TracePrintf("FileReadWriteSpeedTest", "IO Error (%d)!\n", errorCode);
            m_fileErrors++;
        }

        void OnRequestComplete(AZ::IO::RequestHandle /*request handle*/, AZ::IO::SizeType /* num bytes read */, void* /*buffer*/, AZ::IO::Request::StateType /* request state */)
        {
            --m_numRequestsToProcess;
        }

        void run()
        {
            // wait for system to stabilize
            AZ_TracePrintf("FileStreamPerformanceTest", "Waiting for system to stabilize...\n");
            AZStd::this_thread::sleep_for(chrono::seconds(2));

            Streamer::Descriptor desc;
            desc.m_fileMountPoint = nullptr;
            Streamer::Create(desc);

            size_t fileSize = 100 * 1024 * 1024;
            float fileSizeMB = (float)fileSize / (1024.f * 1024.f);

            size_t bufferSize = 256 * 1024 /*fileSize*/;

            AZStd::string fileName = AZStd::string::format("%s%iBigFile.dat", GetTestFolderPath(), ::testing::UnitTest::GetInstance()->random_seed());;
            AZStd::string fileNameSystemFile = AZStd::string::format("%s%iBigFile1.dat", GetTestFolderPath(), ::testing::UnitTest::GetInstance()->random_seed());

            void* buf1 = azmalloc(bufferSize, 64 * 1024);
            void* buf2 = azmalloc(bufferSize, 64 * 1024);
            memset(buf1, 0, bufferSize);
            memset(buf2, 1, bufferSize);

            // Streamer Write a big file
            {
                chrono::system_clock::time_point start = chrono::system_clock::now();
                VirtualStream* st = GetStreamer()->RegisterFileStream(fileName.c_str(), IO::OpenMode::ModeWrite, false);
                int numRequestsToProcess = static_cast<int>(fileSize / bufferSize); // we assume fileSize is a multiple of bufferSize
                m_numRequestsToProcess = numRequestsToProcess;
                for (int i = 0; i < numRequestsToProcess; ++i)
                {
                    GetStreamer()->WriteAsync(st, (i % 2 == 0) ? buf1 : buf2, bufferSize, AZStd::bind(&PERF_FileReadWriteSpeedTest::OnRequestComplete, this, AZStd::placeholders::_1, AZStd::placeholders::_2, AZStd::placeholders::_3, AZStd::placeholders::_4));
                }
                while (m_numRequestsToProcess != 0)
                {
                    AZStd::this_thread::yield();
                }
                chrono::system_clock::time_point end = chrono::system_clock::now();
                float MBperSecond = fileSizeMB / AZStd::chrono::duration<float>(end - start).count();
                AZ::Debug::Trace::Instance().Printf("FileReadWriteSpeedTest", "Streamer Write %.2f MB at %.2f MB/s\n", fileSizeMB, MBperSecond);
                GetStreamer()->UnRegisterStream(st);
            }

            // Streamer Read a big file
            {
                chrono::system_clock::time_point start = chrono::system_clock::now();
                VirtualStream* st = GetStreamer()->RegisterFileStream(fileName.c_str());
                int numRequestsToProcess = static_cast<int>(fileSize / bufferSize); // we assume fileSize is a multiple of bufferSize
                size_t offset = 0;
                m_numRequestsToProcess = numRequestsToProcess;
                for (int i = 0; i < numRequestsToProcess; ++i)
                {
                    GetStreamer()->ReadAsync(st, offset, bufferSize, (i % 2 == 0) ? buf1 : buf2, AZStd::bind(&PERF_FileReadWriteSpeedTest::OnRequestComplete, this, AZStd::placeholders::_1, AZStd::placeholders::_2, AZStd::placeholders::_3, AZStd::placeholders::_4));
                    offset += bufferSize;
                }
                while (m_numRequestsToProcess != 0)
                {
                    AZStd::this_thread::yield();
                }
                chrono::system_clock::time_point end = chrono::system_clock::now();
                float MBperSecond = fileSizeMB / AZStd::chrono::duration<float>(end - start).count();
                AZ::Debug::Trace::Instance().Printf("FileReadWriteSpeedTest", "Streamer Read %.2f MB at %.2f MB/s\n", fileSizeMB, MBperSecond);
                GetStreamer()->UnRegisterStream(st);
            }

            // SystemFile Write a big file
            {
                chrono::system_clock::time_point start = chrono::system_clock::now();
                SystemFile file;
                file.Open(fileNameSystemFile.c_str(), SystemFile::SF_OPEN_CREATE | SystemFile::SF_OPEN_WRITE_ONLY /*, FILE_FLAG_NO_BUFFERING*/);
                int numRequestsToProcess = static_cast<int>(fileSize / bufferSize); // we assume fileSize is a multiple of bufferSize
                for (int i = 0; i < numRequestsToProcess; ++i)
                {
                    file.Write((i % 2 == 0) ? buf1 : buf2, bufferSize);
                }
                chrono::system_clock::time_point end = chrono::system_clock::now();
                float MBperSecond = fileSizeMB / AZStd::chrono::duration<float>(end - start).count();
                AZ::Debug::Trace::Instance().Printf("FileReadWriteSpeedTest", "SystemFile Write %.2f MB at %.2f MB/s\n", fileSizeMB, MBperSecond);
                file.Close();
            }

            // SystemFile Read a big file
            {
                chrono::system_clock::time_point start = chrono::system_clock::now();
                SystemFile file;
                file.Open(fileNameSystemFile.c_str(), SystemFile::SF_OPEN_READ_ONLY /*, FILE_FLAG_NO_BUFFERING*/);
                int numRequestsToProcess = static_cast<int>(fileSize / bufferSize); // we assume fileSize is a multiple of bufferSize
                for (int i = 0; i < numRequestsToProcess; ++i)
                {
                    file.Read(bufferSize, (i % 2 == 0) ? buf1 : buf2);
                }
                chrono::system_clock::time_point end = chrono::system_clock::now();
                float MBperSecond = fileSizeMB / AZStd::chrono::duration<float>(end - start).count();
                AZ::Debug::Trace::Instance().Printf("FileReadWriteSpeedTest", "SystemFile Read %.2f MB at %.2f MB/s\n", fileSizeMB, MBperSecond);
                file.Close();
            }

            azfree(buf1);
            azfree(buf2);

            Streamer::Destroy();
        }
    };

    TestFileIOBase PERF_FileReadWriteSpeedTest::m_fileIO;
    AZ::IO::FileIOBase* PERF_FileReadWriteSpeedTest::m_prevFileIO = nullptr;

#ifdef ENABLE_PERFORMANCE_TEST
    TEST_F(PERF_FileReadWriteSpeedTest, Test)
    {
        run();
    }
#endif // ENABLE_PERFORMANCE_TEST

    /**
     *
     */
    class GenericStreamTest
        : public AllocatorsFixture
    {
        // Hierarchical data serializer
        class DataSerializer
        {
        public:
            virtual ~DataSerializer() {}

            virtual bool Parse(GenericStream* stream) = 0;      // Entry point for loading
            virtual bool BeginSave(GenericStream* stream) = 0;  // Entry point for saving
            virtual bool EndSave() = 0;                         // Signal end of saving

            virtual bool Write(const char* name, const void* data, size_t dataSize) = 0;    // Write a binary byte array
            virtual bool Write(const char* name, const char* data) = 0;                         // Write a string

            virtual bool BeginTag(const char* name) = 0;    // Open a tag group during save
            virtual bool EndTag(const char* name) = 0;      // Close a tag group during save

        protected:
            // store callbacks to be used during parsing:
            // OnTagCB
            // OnDataCB
        };

        // Xml version of DataSerializer
        class XmlSerializer
            : public DataSerializer
        {
        public:
            virtual bool Parse(GenericStream* stream)
            {
                size_t len = static_cast<size_t>(stream->GetLength());
                char* xmlData = reinterpret_cast<char*>(azmalloc(len + 1, 0, AZ::SystemAllocator, "xmlSerializer"));
                len = static_cast<size_t>(stream->Read(len, xmlData));
                xmlData[len] = 0;
                m_xmlDoc.parse<rapidxml::parse_no_data_nodes>(xmlData);
                m_curNode = m_xmlDoc.first_node();
                azfree(xmlData);
                return true;
            }
            virtual bool BeginSave(GenericStream* stream)
            {
                (void)stream;
                return true;
            }
            virtual bool EndSave()
            {
                return true;
            }

            virtual bool Write(const char* name, const void* data, size_t dataSize)
            {
                (void)name;
                (void)data;
                (void)dataSize;
                AZ_Assert(false, "Unsupported");
                return false;
            }
            virtual bool Write(const char* name, const char* data)
            {
                (void)name;
                (void)data;
                return true;
            }

            virtual bool BeginTag(const char* name)
            {
                (void)name;
                return true;
            }
            virtual bool EndTag(const char* name)
            {
                (void)name;
                return true;
            }

        protected:
            rapidxml::xml_document<char>    m_xmlDoc;
            rapidxml::xml_node<char>*       m_curNode;
        };

        // Binary version of DataSerializer
        //class BinarySerializer : public DataSerializer {
        //};

    public:
        void SetUp() override
        {
            AllocatorsFixture::SetUp();

            AZ::AllocatorInstance<AZ::PoolAllocator>::Create();
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();

            m_prevFileIO = FileIOBase::GetInstance();
            FileIOBase::SetInstance(&m_fileIO);
        }

        void TearDown() override
        {
            FileIOBase::SetInstance(m_prevFileIO);

            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
            AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();

            AllocatorsFixture::TearDown();
        }

        void run()
        {
            IO::Streamer::Descriptor desc;
            AZStd:: string testFolder = GetTestFolderPath();
            if (testFolder.length() > 0)
            {
                desc.m_fileMountPoint = testFolder.c_str();
            }
            IO::Streamer::Create(desc);

            AZ_TracePrintf("", "\n");
            
            AZStd::string streamerStreamTestFile = AZStd::string::format("%s%iStreamerStreamTest.txt", GetTestFolderPath(), ::testing::UnitTest::GetInstance()->random_seed());
            AZStd::string systemFile = AZStd::string::format("%s%iSystemFileStreamTest.txt", GetTestFolderPath(), ::testing::UnitTest::GetInstance()->random_seed());

            {
                IO::VirtualStream* stream = IO::Streamer::Instance().RegisterFileStream(streamerStreamTestFile.c_str(), IO::OpenMode::ModeWrite, false);
                StreamerStream streamerStream(stream, false);
                AZ_TEST_ASSERT(streamerStream.IsOpen());
                streamerStream.Write(6, "Hello");
                IO::Streamer::Instance().UnRegisterStream(stream);

                SystemFile file;
                
                file.Open(systemFile.c_str(), SystemFile::SF_OPEN_CREATE | SystemFile::SF_OPEN_WRITE_ONLY);
                SystemFileStream fileStream(&file, false);
                AZ_TEST_ASSERT(fileStream.IsOpen());
                fileStream.Write(6, "Hello");
                file.Close();

                char memBuffer[16];
                MemoryStream memStream(memBuffer, 16, 0);
                AZ_TEST_ASSERT(memStream.IsOpen());
                memStream.Write(6, "Hello");
            }

            {
                char streamBuffer[16];
                IO::VirtualStream* stream = IO::Streamer::Instance().RegisterFileStream(streamerStreamTestFile.c_str(), OpenMode::ModeRead, false);
                StreamerStream streamerStream(stream, false);
                AZ_TEST_ASSERT(streamerStream.IsOpen());
                streamerStream.Read(6, streamBuffer);
                IO::Streamer::Instance().UnRegisterStream(stream);
                AZ_TEST_ASSERT(strcmp(streamBuffer, "Hello") == 0);

                char fileBuffer[16];
                SystemFile file;
                file.Open(systemFile.c_str(), SystemFile::SF_OPEN_READ_ONLY);
                SystemFileStream fileStream(&file, false);
                AZ_TEST_ASSERT(fileStream.IsOpen());
                fileStream.Read(6, fileBuffer);
                file.Close();
                AZ_TEST_ASSERT(strcmp(fileBuffer, "Hello") == 0);

                char memBuffer[16];
                MemoryStream memStream("Hello", 6);
                AZ_TEST_ASSERT(memStream.IsOpen());
                memStream.Read(6, memBuffer);
                AZ_TEST_ASSERT(strcmp(memBuffer, "Hello") == 0);
            }

            IO::Streamer::Destroy();
        }

    private:
        static TestFileIOBase m_fileIO;
        static AZ::IO::FileIOBase* m_prevFileIO;
    };

    TestFileIOBase GenericStreamTest::m_fileIO;
    AZ::IO::FileIOBase* GenericStreamTest::m_prevFileIO = nullptr;

    TEST_F(GenericStreamTest, Test)
    {
        run();
    }

#if !defined(AZCORE_EXCLUDE_ZLIB)

    /**
     * Compressed stream test.
     */
    class CompressedStreamTest
        : public AllocatorsFixture
    {
    public:
        void SetUp() override
        {
            AllocatorsFixture::SetUp();

            AZ::AllocatorInstance<AZ::PoolAllocator>::Create();
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();

            m_prevFileIO = FileIOBase::GetInstance();
            FileIOBase::SetInstance(&m_fileIO);

            IO::Streamer::Descriptor desc;
            AZStd::string testFolder = GetTestFolderPath();
            if (testFolder.length() > 0)
            {
                desc.m_fileMountPoint = testFolder.c_str();
            }
            IO::Streamer::Create(desc);
        }

        void TearDown() override
        {
            IO::Streamer::Destroy();

            FileIOBase::SetInstance(m_prevFileIO);

            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
            AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();

            AllocatorsFixture::TearDown();
        }

        void run()
        {
            AZStd::string randomFileName = AZStd::string::format("%s%iCompressedStream.azcs", GetTestFolderPath(), ::testing::UnitTest::GetInstance()->random_seed());
            const char* compressedFileName = randomFileName.c_str();

            // enable compression on the device (we can find device by name and/or by drive name, etc.)
            IO::Device* device = IO::Streamer::Instance().FindDevice(AZ_ROOT_TEST_FOLDER);
            AZ_TEST_ASSERT(device);  // we must have a valid device

            CompressorStream* compressorStream = new CompressorStream(compressedFileName, IO::OpenMode::ModeWrite);
            
            AZ_TEST_ASSERT(compressorStream);
            AZ_TEST_ASSERT(compressorStream->IsOpen());

            AZStd::unique_ptr<IO::VirtualStream> writeStream(AZStd::make_unique<IO::VirtualStream>(compressorStream, true));
            const char numIterations = 10;
            //////////////////////////////////////////////////////////////////////////
            // Compressed file with no extra seek points  write a compressed file (no extra sync points)
            bool registered = IO::Streamer::Instance().RegisterStream(writeStream.get(), IO::OpenMode::ModeWrite, false);
            AZ_TEST_ASSERT(writeStream);
            // setup compression parameters
            bool result = compressorStream->WriteCompressedHeader(CompressorZLib::TypeId());
            AZ_TEST_ASSERT(result);

            AZStd::vector<char> buffer;

            const int bufferSize = 10 * 1024;

            for (char i = 0; i < numIterations; ++i)
            {
                buffer.clear();
                buffer.resize(bufferSize, i);
                IO::Streamer::SizeType numWritten = IO::Streamer::Instance().Write(writeStream.get(), buffer.data(), buffer.size());
                AZ_TEST_ASSERT(numWritten == buffer.size());
            }

            AZ_TEST_ASSERT(compressorStream->GetUncompressedLength() == bufferSize * numIterations); // compares uncompressed length to the amount of bytes written
            AZ_TEST_ASSERT(compressorStream->GetCompressedLength() != compressorStream->GetUncompressedLength()); // it's possible on theory to be equal, but very unlikely
            AZ_TEST_ASSERT(compressorStream->GetWrappedStream()->GetLength() == compressorStream->GetCompressedLength()); //The underlying stream length should be actual amount data written

            IO::Streamer::Instance().UnRegisterStream(writeStream.get());

            // read a compressed file
            VirtualStream* readStream = IO::Streamer::Instance().RegisterFileStream(compressedFileName, OpenMode::ModeRead, false);
            AZ_TEST_ASSERT(readStream);
            AZ_TEST_ASSERT(readStream->IsCompressed());
            AZ_TEST_ASSERT(readStream->GetLength() != bufferSize * numIterations);

            // continuous reads
            for (char i = 0; i < numIterations; ++i)
            {
                buffer.clear();
                buffer.resize(bufferSize, 0);
                IO::Streamer::SizeType numRead = IO::Streamer::Instance().Read(readStream, i * buffer.size(), buffer.size(), buffer.data());
                AZ_TEST_ASSERT(numRead == buffer.size());
                for (size_t el = 0; el < buffer.size(); ++el)
                {
                    AZ_TEST_ASSERT(buffer[el] == i);
                }
            }

            // random reads
            for (char i = 0; i < numIterations; ++i)
            {
                int chunkToRead = rand() % numIterations;
                buffer.clear();
                buffer.resize(bufferSize, 0);
                IO::Streamer::SizeType numRead = IO::Streamer::Instance().Read(readStream, chunkToRead * buffer.size(), buffer.size(), buffer.data());
                AZ_TEST_ASSERT(numRead == buffer.size());
                for (size_t el = 0; el < buffer.size(); ++el)
                {
                    AZ_TEST_ASSERT(buffer[el] == chunkToRead);
                }
            }

            IO::Streamer::Instance().UnRegisterStream(readStream);

            //////////////////////////////////////////////////////////////////////////
            // Compressed file with extra seek points (for fast search)
            compressorStream = new CompressorStream(compressedFileName, IO::OpenMode::ModeWrite);
            AZ_TEST_ASSERT(compressorStream);
            AZ_TEST_ASSERT(compressorStream->IsOpen());

            writeStream.reset(new VirtualStream(compressorStream, true));
            registered = IO::Streamer::Instance().RegisterStream(writeStream.get(), IO::OpenMode::ModeWrite, false);
            AZ_TEST_ASSERT(registered);
            // setup compression parameters
            result = compressorStream->WriteCompressedHeader(CompressorZLib::TypeId());
            AZ_TEST_ASSERT(result);

            AZ::Sfmt sfmt;

            // write 100k random values
            const int numRandomValues = 100000;

            AZStd::vector<AZ::u32> randomBuffer;

            for (int i = 0; i < numIterations; ++i)
            {
                AZ::u32 sfmtCurrentSeed = 1234 * i;
                sfmt.Seed(&sfmtCurrentSeed, 1);

                randomBuffer.resize(numRandomValues, 0);
                sfmt.FillArray32(randomBuffer.data(), numRandomValues);

                IO::Streamer::SizeType numWritten = IO::Streamer::Instance().Write(writeStream.get(), randomBuffer.data(), randomBuffer.size() * sizeof(AZ::u32));
                AZ_TEST_ASSERT(numWritten == randomBuffer.size() * sizeof(AZ::u32));

                // insert a manual seek point
                result = compressorStream->WriteCompressedSeekPoint();
                AZ_TEST_ASSERT(result);
            }
            IO::Streamer::Instance().UnRegisterStream(writeStream.get());
            writeStream->Close();

            // read a compressed file
            readStream = IO::Streamer::Instance().RegisterFileStream(compressedFileName, IO::OpenMode::ModeRead, false);
            AZ_TEST_ASSERT(readStream);
            AZ_TEST_ASSERT(readStream->IsCompressed());
            AZ_TEST_ASSERT(readStream->GetLength() != numRandomValues * sizeof(AZ::u32) * numIterations);

            // continuous reads
            for (char i = 0; i < numIterations; ++i)
            {
                randomBuffer.clear();
                randomBuffer.resize(numRandomValues, 0);
                IO::Streamer::SizeType numRead = IO::Streamer::Instance().Read(readStream, i * randomBuffer.size() * sizeof(AZ::u32), randomBuffer.size() * sizeof(AZ::u32), randomBuffer.data());
                AZ_TEST_ASSERT(numRead == randomBuffer.size() * sizeof(AZ::u32));
                AZ::u32 sfmtCurrentSeed = 1234 * i;
                sfmt.Seed(&sfmtCurrentSeed, 1);
                for (size_t el = 0; el < numRandomValues; ++el)
                {
                    AZ_TEST_ASSERT(randomBuffer[el] == sfmt.Rand32());
                }
            }

            // random reads
            for (char i = 0; i < numIterations; ++i)
            {
                int chunkToRead = rand() % numIterations;
                randomBuffer.clear();
                randomBuffer.resize(numRandomValues, 0);
                IO::Streamer::SizeType numRead = IO::Streamer::Instance().Read(readStream, chunkToRead * randomBuffer.size() * sizeof(AZ::u32), randomBuffer.size() * sizeof(AZ::u32), randomBuffer.data());
                AZ_TEST_ASSERT(numRead == randomBuffer.size() * sizeof(AZ::u32));
                AZ::u32 sfmtCurrentSeed = 1234 * chunkToRead;
                sfmt.Seed(&sfmtCurrentSeed, 1);
                for (size_t el = 0; el < numRandomValues; ++el)
                {
                    AZ_TEST_ASSERT(randomBuffer[el] == sfmt.Rand32());
                }
            }

            IO::Streamer::Instance().UnRegisterStream(readStream);
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // Compressed file with extra auto seek points (for fast search)
            compressorStream = new CompressorStream(compressedFileName, IO::OpenMode::ModeWrite);
            writeStream.reset(new VirtualStream(compressorStream, true));
            registered = IO::Streamer::Instance().RegisterStream(writeStream.get(), IO::OpenMode::ModeWrite, false);
            AZ_TEST_ASSERT(writeStream);
            // setup compression parameters compression level high, autosync data every 75000 bytes
            result = compressorStream->WriteCompressedHeader(CompressorZLib::TypeId(), 10, 75000);
            AZ_TEST_ASSERT(result);

            // write 100k values
            const int numValues = 100000;  // make sure they are dividable by 10

            for (int i = 0; i < numIterations; ++i)
            {
                buffer.resize(numValues, 0);
                for (int el = 0; el < numValues; ++el)
                {
                    buffer[el] = (i + el) % numIterations;
                }

                // break the writes so we can insert more accurate autosync points.
                // we can change the Streamer/Compressor to be always accurate but this is not really desirable
                IO::Streamer::SizeType numWritten = 0;
                for (int iW = 0; iW < 10; ++iW)
                {
                    numWritten += IO::Streamer::Instance().Write(writeStream.get(), &buffer[(iW * numValues) / 10], buffer.size() / 10);
                }
                AZ_TEST_ASSERT(numWritten == buffer.size());
            }

            IO::Streamer::Instance().UnRegisterStream(writeStream.get());
            writeStream->Close();

            // read a compressed file
            readStream = IO::Streamer::Instance().RegisterFileStream(compressedFileName, IO::OpenMode::ModeRead, false);
            AZ_TEST_ASSERT(readStream);
            AZ_TEST_ASSERT(readStream->IsCompressed());
            AZ_TEST_ASSERT(readStream->GetLength() != numValues * numIterations);

            // continuous reads
            for (char i = 0; i < numIterations; ++i)
            {
                randomBuffer.resize(numValues, 0);
                IO::Streamer::SizeType numRead = IO::Streamer::Instance().Read(readStream, i * buffer.size(), buffer.size(), buffer.data());
                AZ_TEST_ASSERT(numRead == buffer.size());
                for (size_t el = 0; el < numValues; ++el)
                {
                    AZ_TEST_ASSERT(buffer[el] == static_cast<char>(((i + el) % (numIterations))));
                }
            }

            // random reads
            for (char i = 0; i < numIterations; ++i)
            {
                int chunkToRead = rand() % numIterations;
                randomBuffer.clear();
                randomBuffer.resize(numRandomValues, 0);
                IO::Streamer::SizeType numRead = IO::Streamer::Instance().Read(readStream, chunkToRead * buffer.size(), buffer.size(), buffer.data());
                AZ_TEST_ASSERT(numRead == buffer.size());
                for (size_t el = 0; el < numValues; ++el)
                {
                    AZ_TEST_ASSERT(buffer[el] == static_cast<char>(((chunkToRead + el) % numIterations)));
                }
            }

            IO::Streamer::Instance().UnRegisterStream(readStream);
            //////////////////////////////////////////////////////////////////////////
        }

        private:
            static TestFileIOBase m_fileIO;
            static AZ::IO::FileIOBase* m_prevFileIO;
    };

    TestFileIOBase CompressedStreamTest::m_fileIO;
    AZ::IO::FileIOBase* CompressedStreamTest::m_prevFileIO = nullptr;

    TEST_F(CompressedStreamTest, Test)
    {
        run();
    }

#endif // AZCORE_EXCLUDE_ZLIB
}

