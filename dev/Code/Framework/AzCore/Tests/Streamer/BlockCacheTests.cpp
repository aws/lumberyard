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

#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>
#include <AzCore/IO/Streamer/BlockCache.h>
#include <AzCore/IO/Streamer/FileRequest.h>
#include <AzCore/IO/Streamer/StreamerContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include "StreamStackEntryMock.h"

namespace AZ
{
    namespace IO
    {
        class BlockCacheTest
            : public UnitTest::AllocatorsFixture
        {
        public:
            void SetUp() override
            {
                SetupAllocator();

                AZ::AllocatorInstance<AZ::PoolAllocator>::Create();
                AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();

                m_path.InitFromAbsolutePath("Test");
                m_context = new StreamerContext();
            }

            void TearDown() override
            {
                delete[] m_buffer;
                m_buffer = nullptr;

                m_cache = nullptr;
                m_mock = nullptr;

                delete m_context;
                m_context = nullptr;

                AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
                AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();

                TeardownAllocator();
            }

            void CreateTestEnvironmentImplementation(bool onlyEpilogWrites)
            {
                using ::testing::_;

                m_cache = AZStd::make_shared<BlockCache>(m_cacheSize, m_blockSize, onlyEpilogWrites);
                m_mock = AZStd::make_shared<StreamStackEntryMock>();
                m_cache->SetNext(m_mock);
                EXPECT_CALL(*m_mock, SetContext(_)).Times(1);
                m_cache->SetContext(*m_context);

                m_buffer = new u32[m_readBufferLength >> 2];
            }

            void RedirectReadCalls()
            {
                using ::testing::_;
                using ::testing::Return;

                EXPECT_CALL(*m_mock, GetFileSize(_, _)).Times(1);
                EXPECT_CALL(*m_mock, ExecuteRequests())
                    .WillOnce(Return(true))
                    .WillRepeatedly(Return(false));
                ON_CALL(*m_mock, GetFileSize(_, _))
                    .WillByDefault(Invoke(this, &BlockCacheTest::GetFileSize));
                ON_CALL(*m_mock, PrepareRequest(_))
                    .WillByDefault(Invoke(this, &BlockCacheTest::PrepareReadRequest));
            }

            bool GetFileSize(u64& result, const RequestPath&)
            {
                result = m_fakeFileLength;
                return true;
            }

            void PrepareReadRequest(FileRequest* request)
            {
                FileRequest::ReadData& data = request->GetReadData();
                u64 size = data.m_size >> 2;
                u32* buffer = reinterpret_cast<u32*>(data.m_output);
                for (u64 i = 0; i < size; ++i)
                {
                    buffer[i] = static_cast<u32>(data.m_offset + (i << 2));
                }
                ReadFile(data.m_output, *data.m_path, data.m_offset, data.m_size);
                request->SetStatus(FileRequest::Status::Completed);
                m_context->MarkRequestAsCompleted(request);
            }

            void RedirectCanceledReadCalls()
            {
                using ::testing::_;
                using ::testing::Return;

                EXPECT_CALL(*m_mock, GetFileSize(_, _)).Times(1);
                EXPECT_CALL(*m_mock, ExecuteRequests())
                    .WillOnce(Return(true))
                    .WillRepeatedly(Return(false));
                ON_CALL(*m_mock, GetFileSize(_, _))
                    .WillByDefault(Invoke(this, &BlockCacheTest::GetFileSize));
                ON_CALL(*m_mock, PrepareRequest(_))
                    .WillByDefault(Invoke(this, &BlockCacheTest::PrepareCanceledReadRequest));;
            }

            void PrepareCanceledReadRequest(FileRequest* request)
            {
                FileRequest::ReadData& data = request->GetReadData();
                ReadFile(data.m_output, *data.m_path, data.m_offset, data.m_size);
                request->SetStatus(FileRequest::Status::Canceled);
                m_context->MarkRequestAsCompleted(request);
            }

            void VerifyReadBuffer(u64 offset, u64 size)
            {
                size = size >> 2;
                for (u64 i = 0; i < size; ++i)
                {
                    // Using assert here because in case of a problem EXPECT would 
                    // cause a large amount of log noise.
                    ASSERT_EQ(m_buffer[i], offset + (i << 2));
                }
            }

        protected:
            // To make testing easier, this utility mock unpacks the read requests.
            MOCK_METHOD4(ReadFile, bool(void*, const RequestPath&, u64, u64));

            void ProcessRead(void* output, const RequestPath& path, u64 offset, u64 size, FileRequest::Status expectedResult)
            {
                FileRequest* request = m_context->GetNewExternalRequest();
                request->CreateRead(nullptr, nullptr, output, path, offset, size);
                
                AZStd::vector<FileRequest*> dummyRequests;
                m_cache->PrepareRequest(request);
                while (m_cache->ExecuteRequests())
                {
                    m_context->FinalizeCompletedRequests(dummyRequests);
                }

                EXPECT_EQ(expectedResult, request->GetStatus());

                m_context->RecycleRequest(request);
            }

            StreamerContext* m_context;
            AZStd::shared_ptr<BlockCache> m_cache;
            AZStd::shared_ptr<StreamStackEntryMock> m_mock;
            RequestPath m_path;

            u32* m_buffer{ nullptr };
            u64 m_cacheSize{ 5 * 1024 * 1024 };
            u32 m_blockSize{ 64 * 1024 };
            u64 m_fakeFileLength{ 5 * m_blockSize };
            u64 m_readBufferLength{ 10 * 1024 * 1024 };
        };

        /////////////////////////////////////////////////////////////
        // Prolog and epilog enabled
        /////////////////////////////////////////////////////////////
        class BlockCacheWithPrologAndEpilogTest
            : public BlockCacheTest
        {
        public:
            void CreateTestEnvironment()
            {
                CreateTestEnvironmentImplementation(false);
            }
        };

        // File    |------------------------------------------------|
        // Request |------------------------------------------------|
        // Cache   [   x    ][    x   ][   x    ][   x    ][   x    ]
        TEST_F(BlockCacheWithPrologAndEpilogTest, ReadFile_EntireFileRead_FileReadInASingleRead)
        {
            using ::testing::_;

            CreateTestEnvironment();
            RedirectReadCalls();

            EXPECT_CALL(*m_mock, PrepareRequest(_)).Times(1);
            EXPECT_CALL(*this, ReadFile(_, _, 0, m_fakeFileLength));
            
            ProcessRead(m_buffer, m_path, 0, m_fakeFileLength, FileRequest::Status::Completed);
            VerifyReadBuffer(0, m_fakeFileLength);
        }

        // File    |----------------------------------------------|
        // Request |----------------------------------------------|
        // Cache   [   x    ][    x   ][   x    ][   x    ][   x    ]
        TEST_F(BlockCacheWithPrologAndEpilogTest, ReadFile_EntireUnalignedFileRead_FileReadInASingleRead)
        {
            using ::testing::_;

            m_fakeFileLength -= 512;
            CreateTestEnvironment();
            RedirectReadCalls();

            EXPECT_CALL(*m_mock, PrepareRequest(_)).Times(1);
            EXPECT_CALL(*this, ReadFile(_, _, 0, m_fakeFileLength));

            ProcessRead(m_buffer, m_path, 0, m_fakeFileLength, FileRequest::Status::Completed);
            VerifyReadBuffer(0, m_fakeFileLength);
        }

        // File    |------------------------------------------------|
        // Request      |-------------------------------------------|
        // Cache   [   v    ][    x   ][   x    ][   x    ][   x    ]
        TEST_F(BlockCacheWithPrologAndEpilogTest, ReadFile_PrologCacheLargeRequest_FileReadInTwoReads)
        {
            using ::testing::_;

            CreateTestEnvironment();
            RedirectReadCalls();

            EXPECT_CALL(*m_mock, PrepareRequest(_)).Times(2);
            EXPECT_CALL(*this, ReadFile(_, _, 0, m_blockSize));
            EXPECT_CALL(*this, ReadFile(_, _, m_blockSize, m_fakeFileLength - m_blockSize));

            ProcessRead(m_buffer, m_path, 256, m_fakeFileLength - 256, FileRequest::Status::Completed);
            VerifyReadBuffer(256, m_fakeFileLength - 256);
        }

        // File    |------------------------------------------------|
        // Request |-------------------------------------------|
        // Cache   [   x    ][    x   ][   x    ][   x    ][   v    ]
        TEST_F(BlockCacheWithPrologAndEpilogTest, ReadFile_EpilogCacheLargeRequest_FileReadInTwoReads)
        {
            using ::testing::_;

            CreateTestEnvironment();
            RedirectReadCalls();

            EXPECT_CALL(*m_mock, PrepareRequest(_)).Times(2);
            EXPECT_CALL(*this, ReadFile(_, _, 0, m_fakeFileLength - m_blockSize));
            EXPECT_CALL(*this, ReadFile(_, _, m_fakeFileLength - m_blockSize, m_blockSize));

            ProcessRead(m_buffer, m_path, 0, m_fakeFileLength - 256, FileRequest::Status::Completed);
            VerifyReadBuffer(0, m_fakeFileLength - 256);
        }

        // File    |----------------------------------------------|
        // Request |-------------------------------------------|
        // Cache   [   x    ][    x   ][   x    ][   x    ][   v    ]
        TEST_F(BlockCacheWithPrologAndEpilogTest, ReadFile_UnalignedEpilogCacheLargeRequest_FileReadInTwoReads)
        {
            using ::testing::_;

            m_fakeFileLength -= 512;
            CreateTestEnvironment();
            RedirectReadCalls();

            EXPECT_CALL(*m_mock, PrepareRequest(_)).Times(2);
            EXPECT_CALL(*this, ReadFile(_, _, 0, 4 * m_blockSize));
            EXPECT_CALL(*this, ReadFile(_, _, 4 * m_blockSize, m_fakeFileLength - 4 * m_blockSize));

            ProcessRead(m_buffer, m_path, 0, m_fakeFileLength - 256, FileRequest::Status::Completed);
            VerifyReadBuffer(0, m_fakeFileLength - 256);
        }

        // File    |------------------------------------------------|
        // Request     |---------------------------------------|
        // Cache   [   v    ][    x   ][   x    ][   x    ][   v    ]
        TEST_F(BlockCacheWithPrologAndEpilogTest, ReadFile_PrologAndEpilogCacheLargeRequest_FileReadInThreeReads)
        {
            using ::testing::_;

            CreateTestEnvironment();
            RedirectReadCalls();

            EXPECT_CALL(*m_mock, PrepareRequest(_)).Times(3);
            EXPECT_CALL(*this, ReadFile(_, _, 0, m_blockSize));
            EXPECT_CALL(*this, ReadFile(_, _, m_blockSize, m_fakeFileLength - 2 * m_blockSize));
            EXPECT_CALL(*this, ReadFile(_, _, m_fakeFileLength - m_blockSize, m_blockSize));

            ProcessRead(m_buffer, m_path, 256, m_fakeFileLength - 512, FileRequest::Status::Completed);
            VerifyReadBuffer(256, m_fakeFileLength - 512);
        }

        // File    |------------------------------------------------|
        // Request |--------|
        // Cache   [   x    ][    x   ][   x    ][   x    ][   x    ]
        TEST_F(BlockCacheWithPrologAndEpilogTest, ReadFile_ExactBlockRead_FileReadInASingleRead)
        {
            using ::testing::_;

            CreateTestEnvironment();
            RedirectReadCalls();

            EXPECT_CALL(*m_mock, PrepareRequest(_)).Times(1);
            EXPECT_CALL(*this, ReadFile(_, _, 0, m_blockSize));

            ProcessRead(m_buffer, m_path, 0, m_blockSize, FileRequest::Status::Completed);
            VerifyReadBuffer(0, m_blockSize);
        }

        // File    |------------------------------------------------|
        // Request |------|
        // Cache   [   v    ][    x   ][   x    ][   x    ][   x    ]
        TEST_F(BlockCacheWithPrologAndEpilogTest, ReadFile_SmallRead_FileReadInASingleRead)
        {
            using ::testing::_;

            CreateTestEnvironment();
            RedirectReadCalls();

            EXPECT_CALL(*m_mock, PrepareRequest(_)).Times(1);
            EXPECT_CALL(*this, ReadFile(_, _, 0, m_blockSize));

            ProcessRead(m_buffer, m_path, 0, m_blockSize - 256, FileRequest::Status::Completed);
            VerifyReadBuffer(0, m_blockSize - 256);
        }

        // File    |-----|
        // Request |---|
        // Cache   [   v    ][    x   ][   x    ][   x    ][   x    ]
        TEST_F(BlockCacheWithPrologAndEpilogTest, ReadFile_SmallReadUnalignedFile_FileReadInASingleRead)
        {
            using ::testing::_;

            m_fakeFileLength = m_blockSize - 128;
            CreateTestEnvironment();
            RedirectReadCalls();

            EXPECT_CALL(*m_mock, PrepareRequest(_)).Times(1);
            EXPECT_CALL(*this, ReadFile(_, _, 0, m_fakeFileLength));

            ProcessRead(m_buffer, m_path, 0, m_fakeFileLength - 256, FileRequest::Status::Completed);
            VerifyReadBuffer(0, m_fakeFileLength - 256);
        }

        // File    |------------------------------------------------|
        // Request   |---|
        // Cache   [   v    ][    x   ][   x    ][   x    ][   x    ]
        TEST_F(BlockCacheWithPrologAndEpilogTest, ReadFile_SmallReadWithOffset_FileReadInASingleRead)
        {
            using ::testing::_;

            CreateTestEnvironment();
            RedirectReadCalls();

            EXPECT_CALL(*m_mock, PrepareRequest(_)).Times(1);
            EXPECT_CALL(*this, ReadFile(_, _, 0, m_blockSize));

            ProcessRead(m_buffer, m_path, 256, m_blockSize - 512, FileRequest::Status::Completed);
            VerifyReadBuffer(256, m_blockSize - 512);
        }

        // File    |-----|
        // Request  |---|
        // Cache   [   v    ][    x   ][   x    ][   x    ][   x    ]
        TEST_F(BlockCacheWithPrologAndEpilogTest, ReadFile_SmallReadWithOffsetInUnaligedFile_FileReadInASingleRead)
        {
            using ::testing::_;

            m_fakeFileLength = m_blockSize - 128;
            CreateTestEnvironment();
            RedirectReadCalls();

            EXPECT_CALL(*m_mock, PrepareRequest(_)).Times(1);
            EXPECT_CALL(*this, ReadFile(_, _, 0, m_fakeFileLength));

            ProcessRead(m_buffer, m_path, 256, m_blockSize - 512, FileRequest::Status::Completed);
            VerifyReadBuffer(256, m_blockSize - 512);
        }

        // File    |-----|
        // Request  |----|
        // Cache   [   v    ][    x   ][   x    ][   x    ][   x    ]
        TEST_F(BlockCacheWithPrologAndEpilogTest, ReadFile_SmallReadTillEndWithOffsetInUnaligedFile_FileReadInASingleRead)
        {
            using ::testing::_;

            m_fakeFileLength = m_blockSize - 128;
            CreateTestEnvironment();
            RedirectReadCalls();

            EXPECT_CALL(*m_mock, PrepareRequest(_)).Times(1);
            EXPECT_CALL(*this, ReadFile(_, _, 0, m_fakeFileLength));

            ProcessRead(m_buffer, m_path, 256, m_fakeFileLength - 256, FileRequest::Status::Completed);
            VerifyReadBuffer(256, m_fakeFileLength - 256);
        }

        // File    |------------------------------------------------|
        // Request              |---|
        // Cache   [   x    ][    v   ][   x    ][   x    ][   x    ]
        TEST_F(BlockCacheWithPrologAndEpilogTest, ReadFile_SmallReadWithOffsetIntoNextBlock_FileReadInASingleRead)
        {
            using ::testing::_;

            CreateTestEnvironment();
            RedirectReadCalls();

            EXPECT_CALL(*m_mock, PrepareRequest(_)).Times(1);
            EXPECT_CALL(*this, ReadFile(_, _, m_blockSize, m_blockSize));

            ProcessRead(m_buffer, m_path, m_blockSize + 256, m_blockSize - 512, FileRequest::Status::Completed);
            VerifyReadBuffer(m_blockSize + 256, m_blockSize - 512);
        }

        // File    |------------------------------------------------|
        // Request                                           |------|
        // Cache   [   x    ][    x   ][   x    ][   x    ][   v    ]
        TEST_F(BlockCacheWithPrologAndEpilogTest, ReadFile_SmallReadWithOffsetIntoLastBlock_FileReadInASingleRead)
        {
            using ::testing::_;

            CreateTestEnvironment();
            RedirectReadCalls();

            EXPECT_CALL(*m_mock, PrepareRequest(_)).Times(1);
            EXPECT_CALL(*this, ReadFile(_, _, 4 * m_blockSize, m_blockSize));

            ProcessRead(m_buffer, m_path, 4 * m_blockSize + 256, m_blockSize - 256, FileRequest::Status::Completed);
            VerifyReadBuffer(4 * m_blockSize + 256, m_blockSize - 256);
        }

        // File     |----------------------------------------------|
        // Request0     |---------------------------------------|
        // Request1                                           |----|
        // Cache    [   v    ][    x   ][   x    ][   x    ][   v    ]
        // This test queues up multiple read that overlap so one in-flight cache block is used in two requests.
        TEST_F(BlockCacheWithPrologAndEpilogTest, ReadFile_MultipleReadsOverlapping_BothFilesAreCorrectlyRead)
        {
            using ::testing::_;
            using ::testing::Return;

            CreateTestEnvironment();            
            
            EXPECT_CALL(*m_mock, GetFileSize(_, _)).Times(2);
            EXPECT_CALL(*m_mock, ExecuteRequests())
                .WillOnce(Return(true))
                .WillRepeatedly(Return(false));
            ON_CALL(*m_mock, GetFileSize(_, _))
                .WillByDefault(Invoke(this, &BlockCacheTest::GetFileSize));
            ON_CALL(*m_mock, PrepareRequest(_))
                .WillByDefault(Invoke(this, &BlockCacheTest::PrepareReadRequest));;

            EXPECT_CALL(*m_mock, PrepareRequest(_)).Times(3);
            EXPECT_CALL(*this, ReadFile(_, _, 0, m_blockSize));
            EXPECT_CALL(*this, ReadFile(_, _, m_blockSize, m_fakeFileLength - 2 * m_blockSize));
            EXPECT_CALL(*this, ReadFile(_, _, m_fakeFileLength - m_blockSize, m_blockSize));

            constexpr size_t secondReadSize = 512;
            u32 buffer1[secondReadSize / sizeof(u32)];

            FileRequest* request0 = m_context->GetNewExternalRequest();
            FileRequest* request1 = m_context->GetNewExternalRequest();
            request0->CreateRead(nullptr, nullptr, m_buffer, m_path, 256, m_fakeFileLength - 512);
            request1->CreateRead(nullptr, nullptr, buffer1, m_path, m_fakeFileLength-768, secondReadSize);

            AZStd::vector<FileRequest*> dummyRequests;
            m_cache->PrepareRequest(request0);
            m_cache->PrepareRequest(request1);
            while (m_cache->ExecuteRequests())
            {
                m_context->FinalizeCompletedRequests(dummyRequests);
            }

            EXPECT_EQ(FileRequest::Status::Completed, request0->GetStatus());
            EXPECT_EQ(FileRequest::Status::Completed, request1->GetStatus());

            m_context->RecycleRequest(request1);
            m_context->RecycleRequest(request0);

            VerifyReadBuffer(256, m_fakeFileLength - 512);
            for (u64 i = 0; i < secondReadSize / sizeof(u32); ++i)
            {
                // Using assert here because in case of a problem EXPECT would 
                // cause a large amount of log noise.
                ASSERT_EQ(buffer1[i], m_fakeFileLength - 768 + (i << 2));
            }
        }



        /////////////////////////////////////////////////////////////
        // Prolog and epilog can read, but only the epilog can write.
        /////////////////////////////////////////////////////////////
        class BlockCacheWithOnlyEpilogTest
            : public BlockCacheTest
        {
        public:
            void CreateTestEnvironment()
            {
                CreateTestEnvironmentImplementation(true);
            }
        };

        // File    |------------------------------------------------|
        // Request |------------------------------------------------|
        // Cache   [   x    ][    x   ][   x    ][   x    ][   x    ]
        TEST_F(BlockCacheWithOnlyEpilogTest, ReadFile_EntireFileRead_FileReadInASingleRead)
        {
            using ::testing::_;

            CreateTestEnvironment();
            RedirectReadCalls();

            EXPECT_CALL(*m_mock, PrepareRequest(_)).Times(1);
            EXPECT_CALL(*this, ReadFile(_, _, 0, m_fakeFileLength));

            ProcessRead(m_buffer, m_path, 0, m_fakeFileLength, FileRequest::Status::Completed);
            VerifyReadBuffer(0, m_fakeFileLength);
        }

        // File    |----------------------------------------------|
        // Request |----------------------------------------------|
        // Cache   [   x    ][    x   ][   x    ][   x    ][   x    ]
        TEST_F(BlockCacheWithOnlyEpilogTest, ReadFile_EntireUnalignedFileRead_FileReadInASingleRead)
        {
            using ::testing::_;

            m_fakeFileLength -= 512;
            CreateTestEnvironment();
            RedirectReadCalls();

            EXPECT_CALL(*m_mock, PrepareRequest(_)).Times(1);
            EXPECT_CALL(*this, ReadFile(_, _, 0, m_fakeFileLength));

            ProcessRead(m_buffer, m_path, 0, m_fakeFileLength, FileRequest::Status::Completed);
            VerifyReadBuffer(0, m_fakeFileLength);
        }

        // File    |------------------------------------------------|
        // Request      |-------------------------------------------|
        // Cache   [   v    ][    x   ][   x    ][   x    ][   x    ]
        TEST_F(BlockCacheWithOnlyEpilogTest, ReadFile_PrologCacheLargeRequest_FileReadInTwoReads)
        {
            using ::testing::_;

            CreateTestEnvironment();
            RedirectReadCalls();

            EXPECT_CALL(*m_mock, PrepareRequest(_)).Times(1);
            EXPECT_CALL(*this, ReadFile(_, _, 256, m_fakeFileLength - 256));
            
            ProcessRead(m_buffer, m_path, 256, m_fakeFileLength - 256, FileRequest::Status::Completed);
            VerifyReadBuffer(256, m_fakeFileLength - 256);
        }

        // File    |------------------------------------------------|
        // Request |-------------------------------------------|
        // Cache   [   x    ][    x   ][   x    ][   x    ][   v    ]
        TEST_F(BlockCacheWithOnlyEpilogTest, ReadFile_EpilogCacheLargeRequest_FileReadInTwoReads)
        {
            using ::testing::_;

            CreateTestEnvironment();
            RedirectReadCalls();

            EXPECT_CALL(*m_mock, PrepareRequest(_)).Times(2);
            EXPECT_CALL(*this, ReadFile(_, _, 0, m_fakeFileLength - m_blockSize));
            EXPECT_CALL(*this, ReadFile(_, _, m_fakeFileLength - m_blockSize, m_blockSize));

            ProcessRead(m_buffer, m_path, 0, m_fakeFileLength - 256, FileRequest::Status::Completed);
            VerifyReadBuffer(0, m_fakeFileLength - 256);
        }

        // File    |----------------------------------------------|
        // Request |-------------------------------------------|
        // Cache   [   x    ][    x   ][   x    ][   x    ][   v    ]
        TEST_F(BlockCacheWithOnlyEpilogTest, ReadFile_UnalignedEpilogCacheLargeRequest_FileReadInTwoReads)
        {
            using ::testing::_;

            m_fakeFileLength -= 512;
            CreateTestEnvironment();
            RedirectReadCalls();

            EXPECT_CALL(*m_mock, PrepareRequest(_)).Times(2);
            EXPECT_CALL(*this, ReadFile(_, _, 0, 4 * m_blockSize));
            EXPECT_CALL(*this, ReadFile(_, _, 4 * m_blockSize, m_fakeFileLength - 4 * m_blockSize));

            ProcessRead(m_buffer, m_path, 0, m_fakeFileLength - 256, FileRequest::Status::Completed);
            VerifyReadBuffer(0, m_fakeFileLength - 256);
        }

        // File    |------------------------------------------------|
        // Request     |---------------------------------------|
        // Cache   [   v    ][    x   ][   x    ][   x    ][   v    ]
        TEST_F(BlockCacheWithOnlyEpilogTest, ReadFile_PrologAndEpilogCacheLargeRequest_FileReadInThreeReads)
        {
            using ::testing::_;

            CreateTestEnvironment();
            RedirectReadCalls();

            EXPECT_CALL(*m_mock, PrepareRequest(_)).Times(2);
            EXPECT_CALL(*this, ReadFile(_, _, 256, m_fakeFileLength - m_blockSize - 256));
            EXPECT_CALL(*this, ReadFile(_, _, m_fakeFileLength - m_blockSize, m_blockSize));

            ProcessRead(m_buffer, m_path, 256, m_fakeFileLength - 512, FileRequest::Status::Completed);
            VerifyReadBuffer(256, m_fakeFileLength - 512);
        }

        // File    |------------------------------------------------|
        // Request |--------|
        // Cache   [   x    ][    x   ][   x    ][   x    ][   x    ]
        TEST_F(BlockCacheWithOnlyEpilogTest, ReadFile_ExactBlockRead_FileReadInASingleRead)
        {
            using ::testing::_;

            CreateTestEnvironment();
            RedirectReadCalls();

            EXPECT_CALL(*m_mock, PrepareRequest(_)).Times(1);
            EXPECT_CALL(*this, ReadFile(_, _, 0, m_blockSize));

            ProcessRead(m_buffer, m_path, 0, m_blockSize, FileRequest::Status::Completed);
            VerifyReadBuffer(0, m_blockSize);
        }

        // File    |------------------------------------------------|
        // Request |------|
        // Cache   [   v    ][    x   ][   x    ][   x    ][   x    ]
        TEST_F(BlockCacheWithOnlyEpilogTest, ReadFile_SmallRead_FileReadInASingleRead)
        {
            using ::testing::_;

            CreateTestEnvironment();
            RedirectReadCalls();

            EXPECT_CALL(*m_mock, PrepareRequest(_)).Times(1);
            EXPECT_CALL(*this, ReadFile(_, _, 0, m_blockSize));

            ProcessRead(m_buffer, m_path, 0, m_blockSize - 256, FileRequest::Status::Completed);
            VerifyReadBuffer(0, m_blockSize - 256);
        }

        // File    |-----|
        // Request |---|
        // Cache   [   v    ][    x   ][   x    ][   x    ][   x    ]
        TEST_F(BlockCacheWithOnlyEpilogTest, ReadFile_SmallReadUnalignedFile_FileReadInASingleRead)
        {
            using ::testing::_;

            m_fakeFileLength = m_blockSize - 128;
            CreateTestEnvironment();
            RedirectReadCalls();

            EXPECT_CALL(*m_mock, PrepareRequest(_)).Times(1);
            EXPECT_CALL(*this, ReadFile(_, _, 0, m_fakeFileLength));

            ProcessRead(m_buffer, m_path, 0, m_fakeFileLength - 256, FileRequest::Status::Completed);
            VerifyReadBuffer(0, m_fakeFileLength - 256);
        }

        // File    |------------------------------------------------|
        // Request   |---|
        // Cache   [   v    ][    x   ][   x    ][   x    ][   x    ]
        TEST_F(BlockCacheWithOnlyEpilogTest, ReadFile_SmallReadWithOffset_FileReadInASingleRead)
        {
            using ::testing::_;

            CreateTestEnvironment();
            RedirectReadCalls();

            EXPECT_CALL(*m_mock, PrepareRequest(_)).Times(1);
            EXPECT_CALL(*this, ReadFile(_, _, 0, m_blockSize));

            ProcessRead(m_buffer, m_path, 256, m_blockSize - 512, FileRequest::Status::Completed);
            VerifyReadBuffer(256, m_blockSize - 512);
        }

        // File    |-----|
        // Request  |---|
        // Cache   [   v    ][    x   ][   x    ][   x    ][   x    ]
        TEST_F(BlockCacheWithOnlyEpilogTest, ReadFile_SmallReadWithOffsetInUnaligedFile_FileReadInASingleRead)
        {
            using ::testing::_;

            m_fakeFileLength = m_blockSize - 128;
            CreateTestEnvironment();
            RedirectReadCalls();

            EXPECT_CALL(*m_mock, PrepareRequest(_)).Times(1);
            EXPECT_CALL(*this, ReadFile(_, _, 0, m_fakeFileLength));

            ProcessRead(m_buffer, m_path, 256, m_blockSize - 512, FileRequest::Status::Completed);
            VerifyReadBuffer(256, m_blockSize - 512);
        }

        // File    |-----|
        // Request  |----|
        // Cache   [   v    ][    x   ][   x    ][   x    ][   x    ]
        TEST_F(BlockCacheWithOnlyEpilogTest, ReadFile_SmallReadTillEndWithOffsetInUnaligedFile_FileReadInASingleRead)
        {
            using ::testing::_;

            m_fakeFileLength = m_blockSize - 128;
            CreateTestEnvironment();
            RedirectReadCalls();

            EXPECT_CALL(*m_mock, PrepareRequest(_)).Times(1);
            EXPECT_CALL(*this, ReadFile(_, _, 0, m_fakeFileLength));

            ProcessRead(m_buffer, m_path, 256, m_fakeFileLength - 256, FileRequest::Status::Completed);
            VerifyReadBuffer(256, m_fakeFileLength - 256);
        }

        // File    |------------------------------------------------|
        // Request              |---|
        // Cache   [   x    ][    v   ][   x    ][   x    ][   x    ]
        TEST_F(BlockCacheWithOnlyEpilogTest, ReadFile_SmallReadWithOffsetIntoNextBlock_FileReadInASingleRead)
        {
            using ::testing::_;

            CreateTestEnvironment();
            RedirectReadCalls();

            EXPECT_CALL(*m_mock, PrepareRequest(_)).Times(1);
            EXPECT_CALL(*this, ReadFile(_, _, m_blockSize, m_blockSize));

            ProcessRead(m_buffer, m_path, m_blockSize + 256, m_blockSize - 512, FileRequest::Status::Completed);
            VerifyReadBuffer(m_blockSize + 256, m_blockSize - 512);
        }

        // File    |------------------------------------------------|
        // Request                                           |------|
        // Cache   [   x    ][    x   ][   x    ][   x    ][   v    ]
        TEST_F(BlockCacheWithOnlyEpilogTest, ReadFile_SmallReadWithOffsetIntoLastBlock_FileReadInASingleRead)
        {
            using ::testing::_;

            CreateTestEnvironment();
            RedirectReadCalls();

            EXPECT_CALL(*m_mock, PrepareRequest(_)).Times(1);
            EXPECT_CALL(*this, ReadFile(_, _, 4 * m_blockSize, m_blockSize));

            ProcessRead(m_buffer, m_path, 4 * m_blockSize + 256, m_blockSize - 256, FileRequest::Status::Completed);
            VerifyReadBuffer(4 * m_blockSize + 256, m_blockSize - 256);
        }

        // File     |----------------------------------------------|
        // Request0     |---------------------------------------|
        // Request1                                           |----|
        // Cache    [   v    ][    x   ][   x    ][   x    ][   v    ]
        // This test queues up multiple read that overlap so one in-flight cache block is used in two requests.
        TEST_F(BlockCacheWithOnlyEpilogTest, ReadFile_MultipleReadsOverlapping_BothFilesAreCorrectlyRead)
        {
            using ::testing::_;
            using ::testing::Return;

            CreateTestEnvironment();

            EXPECT_CALL(*m_mock, GetFileSize(_, _)).Times(2);
            EXPECT_CALL(*m_mock, ExecuteRequests())
                .WillOnce(Return(true))
                .WillRepeatedly(Return(false));
            ON_CALL(*m_mock, GetFileSize(_, _))
                .WillByDefault(Invoke(this, &BlockCacheTest::GetFileSize));
            ON_CALL(*m_mock, PrepareRequest(_))
                .WillByDefault(Invoke(this, &BlockCacheTest::PrepareReadRequest));

            EXPECT_CALL(*m_mock, PrepareRequest(_)).Times(2);
            EXPECT_CALL(*this, ReadFile(_, _, 256, m_fakeFileLength - m_blockSize - 256));
            EXPECT_CALL(*this, ReadFile(_, _, m_fakeFileLength - m_blockSize, m_blockSize));


            u32 buffer1[512 / 4];

            FileRequest* request0 = m_context->GetNewExternalRequest();
            FileRequest* request1 = m_context->GetNewExternalRequest();
            request0->CreateRead(nullptr, nullptr, m_buffer, m_path, 256, m_fakeFileLength - 512);
            request1->CreateRead(nullptr, nullptr, buffer1, m_path, m_fakeFileLength - 768, 512);

            AZStd::vector<FileRequest*> dummyRequests;
            m_cache->PrepareRequest(request0);
            m_cache->PrepareRequest(request1);
            while (m_cache->ExecuteRequests())
            {
                m_context->FinalizeCompletedRequests(dummyRequests);
            }

            EXPECT_EQ(FileRequest::Status::Completed, request0->GetStatus());
            EXPECT_EQ(FileRequest::Status::Completed, request1->GetStatus());

            m_context->RecycleRequest(request1);
            m_context->RecycleRequest(request0);

            VerifyReadBuffer(256, m_fakeFileLength - 512);
            for (u64 i = 0; i < 512 / 4; ++i)
            {
                // Using assert here because in case of a problem EXPECT would 
                // cause a large amount of log noise.
                ASSERT_EQ(buffer1[i], m_fakeFileLength - 768 + (i << 2));
            }
        }



        /////////////////////////////////////////////////////////////
        // Generic block cache test.
        /////////////////////////////////////////////////////////////
        class BlockCacheGenericTest
            : public BlockCacheTest
        {
        public:
            void CreateTestEnvironment()
            {
                CreateTestEnvironmentImplementation(false);
            }
        };

        TEST_F(BlockCacheGenericTest, Cancel_QueueReadAndCancel_SubRequestPushCanceledThroughCache)
        {
            using ::testing::_;

            CreateTestEnvironment();
            RedirectCanceledReadCalls();

            EXPECT_CALL(*m_mock, PrepareRequest(_)).Times(3);
            EXPECT_CALL(*this, ReadFile(_, _, 0, m_blockSize));
            EXPECT_CALL(*this, ReadFile(_, _, m_blockSize, m_fakeFileLength - 2 * m_blockSize));
            EXPECT_CALL(*this, ReadFile(_, _, m_fakeFileLength - m_blockSize, m_blockSize));

            ProcessRead(m_buffer, m_path, 256, m_fakeFileLength - 512, FileRequest::Status::Canceled);
        }
    } // namespace IO
} // namespace AZ
