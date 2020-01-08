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
#include <AzCore/IO/Streamer/FileRequest.h>
#include <AzCore/IO/Streamer/FullFileDecompressor.h>
#include <AzCore/IO/Streamer/StreamerContext.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include "StreamStackEntryMock.h"

namespace AZ
{
    namespace IO
    {
        class FullDecompressorTest
            : public UnitTest::AllocatorsFixture
        {
        public:
            enum CompressionState
            {
                Uncompressed,
                Compressed,
                Corrupted
            };

            enum ReadResult
            {
                Success,
                Failed,
                Canceled
            };

            void SetUp() override
            {
                SetupAllocator();

                AllocatorInstance<PoolAllocator>::Create();
                AllocatorInstance<ThreadPoolAllocator>::Create();
            }

            void TearDown() override
            {
                m_decompressor.reset();
                m_mock.reset();

                m_decompressor = nullptr;
                m_mock = nullptr;

                delete[] m_buffer;
                m_buffer = nullptr;

                delete m_context;
                m_context = nullptr;

                AllocatorInstance<ThreadPoolAllocator>::Destroy();
                AllocatorInstance<PoolAllocator>::Destroy(); 

                TeardownAllocator();
            }

            void SetupEnvironment()
            {
                m_buffer = new u32[m_fakeFileLength >> 2];

                m_mock = AZStd::make_shared<StreamStackEntryMock>();
                m_decompressor = AZStd::make_shared<FullFileDecompressor>(1, 1);

                m_context = new StreamerContext();
                m_decompressor->SetContext(*m_context);
                m_decompressor->SetNext(m_mock);
            }

            void MockReadCalls(ReadResult mockResult)
            {
                using ::testing::_;
                using ::testing::Return;

                EXPECT_CALL(*m_mock, ExecuteRequests())
                    .WillOnce(Return(true))
                    .WillRepeatedly(Return(false));
                EXPECT_CALL(*m_mock, PrepareRequest(_));
                    
                ON_CALL(*m_mock, GetFileSize(_, _))
                    .WillByDefault(Invoke(this, &FullDecompressorTest::GetFileSize));
                switch (mockResult)
                {
                case ReadResult::Success:
                    ON_CALL(*m_mock, PrepareRequest(_))
                        .WillByDefault(Invoke(this, &FullDecompressorTest::PrepareReadRequest));
                    break;
                case ReadResult::Failed:
                    ON_CALL(*m_mock, PrepareRequest(_))
                        .WillByDefault(Invoke(this, &FullDecompressorTest::PrepareFailedReadRequest));
                    break;
                case ReadResult::Canceled:
                    ON_CALL(*m_mock, PrepareRequest(_))
                        .WillByDefault(Invoke(this, &FullDecompressorTest::PrepareCanceledReadRequest));
                    break;
                default:
                    AZ_Assert(false, "Unexpected mock result type.");
                }
            }

            bool GetFileSize(u64& result, const RequestPath&)
            {
                result = m_fakeFileLength;
                return true;
            }

            void PrepareReadRequest(FileRequest* request)
            {
                ASSERT_EQ(FileRequest::Operation::Read, request->GetOperationType());

                FileRequest::ReadData& data = request->GetReadData();
                u64 size = data.m_size >> 2;
                u32* buffer = reinterpret_cast<u32*>(data.m_output);
                for (u64 i = 0; i < size; ++i)
                {
                    buffer[i] = static_cast<u32>(data.m_offset + (i << 2));
                }
                request->SetStatus(FileRequest::Status::Completed);
                m_context->MarkRequestAsCompleted(request);
            }

            void PrepareFailedReadRequest(FileRequest* request)
            {
                request->SetStatus(FileRequest::Status::Failed);
                m_context->MarkRequestAsCompleted(request);
            }

            void PrepareCanceledReadRequest(FileRequest* request)
            {
                request->SetStatus(FileRequest::Status::Canceled);
                m_context->MarkRequestAsCompleted(request);
            }

            static bool Decompressor(const CompressionInfo&, const void* compressed, size_t compressedSize, void* uncompressed, size_t uncompressedBufferSize)
            {
                AZ_Assert(compressedSize == uncompressedBufferSize, "Fake decompression algorithm only supports copying data.");
                memcpy(uncompressed, compressed, compressedSize);
                return true;
            }

            static bool CorruptedDecompressor(const CompressionInfo&, const void*, size_t, void*, size_t)
            {
                return false;
            }

            void ProcessCompressedRead(u64 offset, u64 size, CompressionState compressionState, FileRequest::Status expectedResult)
            {
                CompressionInfo compressionInfo;
                compressionInfo.m_compressedSize = m_fakeFileLength;
                compressionInfo.m_isCompressed = (compressionState == CompressionState::Compressed || compressionState == CompressionState::Corrupted);
                compressionInfo.m_offset = 0;
                compressionInfo.m_uncompressedSize = m_fakeFileLength;
                if (compressionState == CompressionState::Corrupted)
                {
                    compressionInfo.m_decompressor = &FullDecompressorTest::CorruptedDecompressor;
                }
                else
                {
                    compressionInfo.m_decompressor = &FullDecompressorTest::Decompressor;
                }

                FileRequest* request = m_context->GetNewExternalRequest();
                request->CreateCompressedRead(nullptr, nullptr, AZStd::move(compressionInfo), m_buffer, offset, size);

                AZStd::vector<FileRequest*> dummyRequests;
                m_decompressor->PrepareRequest(request);
                while (m_decompressor->ExecuteRequests() || m_decompressor->GetNumRunningJobs() > 0)
                {
                    m_context->FinalizeCompletedRequests(dummyRequests);
                }

                EXPECT_EQ(expectedResult, request->GetStatus());

                m_context->RecycleRequest(request);
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

            u32* m_buffer;
            StreamerContext* m_context;
            AZStd::shared_ptr<FullFileDecompressor> m_decompressor;
            AZStd::shared_ptr<StreamStackEntryMock> m_mock;
            u64 m_fakeFileLength{ 1 * 1024 * 1024 };
        };

        TEST_F(FullDecompressorTest, DecompressedRead_FullReadAndDecompressData_SuccessfullyReadData)
        {
            SetupEnvironment();
            MockReadCalls(ReadResult::Success);
            ProcessCompressedRead(0, m_fakeFileLength, CompressionState::Compressed, FileRequest::Status::Completed);
            VerifyReadBuffer(0, m_fakeFileLength);
        }

        TEST_F(FullDecompressorTest, DecompressedRead_PartialReadAndDecompressData_SuccessfullyReadData)
        {
            SetupEnvironment();
            MockReadCalls(ReadResult::Success);
            ProcessCompressedRead(256, m_fakeFileLength-512, CompressionState::Compressed, FileRequest::Status::Completed);
            VerifyReadBuffer(256, m_fakeFileLength-512);
        }

        TEST_F(FullDecompressorTest, DecompressedRead_FullReadFromArchive_SuccessfullyReadData)
        {
            SetupEnvironment();
            MockReadCalls(ReadResult::Success);
            ProcessCompressedRead(0, m_fakeFileLength, CompressionState::Uncompressed, FileRequest::Status::Completed);
            VerifyReadBuffer(0, m_fakeFileLength);
        }

        TEST_F(FullDecompressorTest, DecompressedRead_PartialReadFromArchive_SuccessfullyReadData)
        {
            SetupEnvironment();
            MockReadCalls(ReadResult::Success);
            ProcessCompressedRead(256, m_fakeFileLength - 512, CompressionState::Uncompressed, FileRequest::Status::Completed);
            VerifyReadBuffer(256, m_fakeFileLength - 512);
        }

        TEST_F(FullDecompressorTest, DecompressedRead_FailedRead_FailureIsDetectedAndReported)
        {
            SetupEnvironment();
            MockReadCalls(ReadResult::Failed);
            ProcessCompressedRead(0, m_fakeFileLength, CompressionState::Compressed, FileRequest::Status::Failed);
        }

        TEST_F(FullDecompressorTest, DecompressedRead_CanceledRead_CancelIsDetectedAndReported)
        {
            SetupEnvironment();
            MockReadCalls(ReadResult::Canceled);
            ProcessCompressedRead(0, m_fakeFileLength, CompressionState::Compressed, FileRequest::Status::Canceled);
        }

        TEST_F(FullDecompressorTest, DecompressedRead_CorruptedArchiveRead_RequestIsCompletedWithFailedState)
        {
            SetupEnvironment();
            MockReadCalls(ReadResult::Success);
            ProcessCompressedRead(0, m_fakeFileLength, CompressionState::Corrupted, FileRequest::Status::Failed);
        }
    } // namespace IO
} // namespace AZ
