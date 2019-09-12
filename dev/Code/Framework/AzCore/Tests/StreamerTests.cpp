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
#include <FileIOBaseTestTypes.h>
#include <AzCore/IO/CompressionBus.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Streamer.h>
#include <AzCore/IO/Streamer/StreamerConfiguration.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    namespace IO
    {
        namespace Utils
        {
            //! Create a test file that stores 4 byte integers starting at 0 and incrementing.
            //! @filename The name of the file to write to.
            //! @filesize The size the new file needs to be in bytes. The stored values will continue till fileSize / 4.
            //! @paddingSize The amount of data to insert before and after the file. In total paddingSize / 4 integers
            //!              will be added. The prefix will be marked with "0xdeadbeef" and the postfix with "0xd15ea5ed".
            static void CreateTestFile(const AZStd::string& name, size_t fileSize, size_t paddingSize)
            {
                static const size_t bufferByteSize = 1024 * 1024;
                static const size_t bufferSize = bufferByteSize / sizeof(u32);
                u32* buffer = new u32[bufferSize];
                
                AZ_Assert(paddingSize < bufferByteSize, "Padding can't currently be larger than %i bytes.", bufferByteSize);
                size_t paddingCount = paddingSize / sizeof(u32);

                FileIOStream stream(name.c_str(), OpenMode::ModeWrite | OpenMode::ModeBinary);
                
                // Write pre-padding
                for (size_t i = 0; i < paddingCount; ++i)
                {
                    buffer[i] = 0xdeadbeef;
                }
                stream.Write(paddingSize, buffer);

                // Write content
                u32 startIndex = 0;
                while (fileSize > bufferByteSize)
                {
                    for (u32 i = 0; i < bufferSize; ++i)
                    {
                        buffer[i] = startIndex + i;
                    }
                    startIndex += bufferSize;

                    stream.Write(bufferByteSize, buffer);
                    fileSize -= bufferByteSize;
                }
                for (u32 i = 0; i < bufferSize; ++i)
                {
                    buffer[i] = startIndex + i;
                }
                stream.Write(fileSize, buffer);

                // Write post-padding
                for (size_t i = 0; i < paddingCount; ++i)
                {
                    buffer[i] = 0xd15ea5ed;
                }
                stream.Write(paddingSize, buffer);

                delete[] buffer;
            }
        }

        struct DedicatedCache_Uncompressed {};
        struct GlobalCache_Uncompressed {};
        struct DedicatedCache_Compressed {};
        struct GlobalCache_Compressed {};

        enum class PadArchive : bool
        {
            Yes,
            No
        };

        class MockFileBase
        {
        public:
            virtual ~MockFileBase() = default;

            virtual void CreateTestFile(AZStd::string filename, size_t fileSize, PadArchive padding) = 0;
            virtual const AZStd::string& GetFileName() const = 0;
        };

        class MockUncompressedFile
            : public MockFileBase
        {
        public:
            ~MockUncompressedFile() override
            {
                if (m_hasFile)
                {
                    FileIOBase::GetInstance()->DestroyPath(m_filename.c_str());
                }
            }

            void CreateTestFile(AZStd::string filename, size_t fileSize, PadArchive) override
            {
                m_fileSize = fileSize;
                m_filename = AZStd::move(filename);
                Utils::CreateTestFile(m_filename, m_fileSize, 0);
                m_hasFile = true;
            }

            const AZStd::string& GetFileName() const override
            {
                return m_filename;
            }

        private:
            AZStd::string m_filename;
            size_t m_fileSize = 0;
            bool m_hasFile = false;
        };

        class MockCompressedFile
            : public MockFileBase
            , public CompressionBus::Handler
        {
        public:
            static const uint32_t s_tag = static_cast<uint32_t>('T') << 24 | static_cast<uint32_t>('E') << 16 | static_cast<uint32_t>('S') << 8 | static_cast<uint32_t>('T');
            static const uint32_t s_paddingSize = 512; // Use this amount of bytes before and after a generated file as padding.

            ~MockCompressedFile() override
            {
                if (m_hasFile)
                {
                    BusDisconnect();
                    FileIOBase::GetInstance()->DestroyPath(m_filename.c_str());
                }
            }

            void CreateTestFile(AZStd::string filename, size_t fileSize, PadArchive padding) override
            {
                m_fileSize = fileSize;
                m_filename = AZStd::move(filename);
                m_hasPadding = (padding == PadArchive::Yes);
                uint32_t paddingSize = s_paddingSize;
                Utils::CreateTestFile(m_filename, m_fileSize / 2, m_hasPadding ? paddingSize : 0 );
                
                m_hasFile = true;

                BusConnect();
            }

            const AZStd::string& GetFileName() const override
            {
                return m_filename;
            }

            void Decompress(const AZ::IO::CompressionInfo& info, const void* compressed, size_t compressedSize,
                void* uncompressed, size_t uncompressedSize)
            {
                const uint32_t tag = s_tag;
                ASSERT_EQ(info.m_compressionTag.m_code, tag);
                ASSERT_EQ(info.m_compressedSize, m_fileSize / 2);
                ASSERT_TRUE(info.m_isCompressed);
                uint32_t paddingSize = s_paddingSize;
                ASSERT_EQ(info.m_offset, m_hasPadding ? paddingSize : 0);
                ASSERT_EQ(info.m_uncompressedSize, m_fileSize);

                // Check the input
                ASSERT_EQ(compressedSize, m_fileSize / 2);
                const u32* values = reinterpret_cast<const u32*>(compressed);
                const size_t numValues = compressedSize / sizeof(u32);
                for (size_t i = 0; i < numValues; ++i)
                {
                    EXPECT_EQ(values[i], i);
                }

                // Create the fake uncompressed data.
                ASSERT_EQ(uncompressedSize, m_fileSize);
                u32* output = reinterpret_cast<u32*>(uncompressed);
                size_t outputSize = uncompressedSize / sizeof(u32);
                for (size_t i = 0; i < outputSize; ++i)
                {
                    output[i] = static_cast<u32>(i);
                }
            }

            //@{ CompressionBus Handler implementation.
            void FindCompressionInfo(bool& found, AZ::IO::CompressionInfo& info, const AZStd::string_view filename) override
            {
                if (m_hasFile && m_filename == filename)
                {
                    found = true;
                    ASSERT_TRUE(info.m_archiveFilename.InitFromRelativePath(m_filename.c_str()));
                    info.m_compressedSize = m_fileSize / 2;
                    const uint32_t tag = s_tag;
                    info.m_compressionTag.m_code = tag;
                    info.m_isCompressed = true;
                    uint32_t paddingSize = s_paddingSize;
                    info.m_offset = m_hasPadding ? paddingSize : 0;
                    info.m_uncompressedSize = m_fileSize;

                    info.m_decompressor = 
                        [this](const AZ::IO::CompressionInfo& info, const void* compressed, 
                        size_t compressedSize, void* uncompressed, size_t uncompressedSize) -> bool
                    {
                        Decompress(info, compressed, compressedSize, uncompressed, uncompressedSize);
                        return true;
                    };
                }
            }
            //@}

        private:
            AZStd::string m_filename;
            size_t m_fileSize = 0;
            bool m_hasFile = false;
            bool m_hasPadding = false;
        };

        class StreamerTestBase
            : public UnitTest::AllocatorsTestFixture
        {
        public:
            void SetUp() override
            {
                AllocatorsTestFixture::SetUp();
                AZ::AllocatorInstance<AZ::PoolAllocator>::Create();
                AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();
                
                m_prevFileIO = FileIOBase::GetInstance();
                FileIOBase::SetInstance(&m_fileIO);

                Streamer::Descriptor desc;
                IO::Streamer::Create(desc);
            }

            void TearDown() override
            {
                Streamer::Destroy();

                for (size_t i = 0; i < m_testFileCount; ++i)
                {
                    AZStd::string name = AZStd::string::format("TestFile_%i.test", i);
                    FileIOBase::GetInstance()->DestroyPath(name.c_str());
                }

                FileIOBase::SetInstance(m_prevFileIO);

                AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
                AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();
                AllocatorsTestFixture::TearDown();
            }

            //! Requests are typically completed by Streamer before it updates it's internal bookkeeping. 
            //! If a test depends on getting status information such as if cache files have been cleared
            //! then call WaitForScheduler to give Steamers scheduler some time to update it's internal status.
            void WaitForScheduler()
            {
                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(250));
            }

        protected:
            virtual AZStd::unique_ptr<MockFileBase> CreateMockFile() = 0;
            virtual bool IsUsingArchive() const = 0;
            virtual bool CreateDedicatedCache() const = 0;

            //! Create a test file that stores 4 byte integers starting at 0 and incrementing.
            //! @filesize The size the new file needs to be in bytes. The stored values will continue till fileSize / 4.
            //! @return The name of the test file.
            AZStd::unique_ptr<MockFileBase> CreateTestFile(size_t fileSize, PadArchive padding)
            {
                AZStd::string name = AZStd::string::format("TestFile_%i.test", m_testFileCount++);
                AZStd::unique_ptr<MockFileBase> result = CreateMockFile();
                result->CreateTestFile(name, fileSize, padding);
                if (CreateDedicatedCache())
                {
                    AZ::IO::Streamer::Instance().CreateDedicatedCache(name.c_str());
                }
                return result;
            }

            void VerifyTestFile(const void* buffer, size_t fileSize, size_t offset = 0)
            {
                size_t count = fileSize / sizeof(u32);
                size_t numOffset = offset / sizeof(u32);
                const u32* data = reinterpret_cast<const u32*>(buffer);
                for (size_t i = 0; i < count; ++i)
                {
                    EXPECT_EQ(data[i], i + numOffset);
                }
            }

            void AssertTestFile(const void* buffer, size_t fileSize, size_t offset = 0)
            {
                size_t count = fileSize / sizeof(u32);
                size_t numOffset = offset / sizeof(u32);
                const u32* data = reinterpret_cast<const u32*>(buffer);
                for (size_t i = 0; i < count; ++i)
                {
                    ASSERT_EQ(data[i], i + numOffset);
                }
            }

            UnitTest::TestFileIOBase m_fileIO;
            FileIOBase* m_prevFileIO;
            size_t m_testFileCount = 0;
        };

        template<typename TestTag>
        class StreamerTest : public StreamerTestBase
        {
        protected:
            bool IsUsingArchive() const override
            {
                AZ_Assert(false, "Not correctly specialized.");
                return false;
            }

            bool CreateDedicatedCache() const override
            {
                AZ_Assert(false, "Not correctly specialized.");
                return false;
            }

            AZStd::unique_ptr<MockFileBase> CreateMockFile() override
            {
                AZ_Assert(false, "Not correctly specialized.");
                return nullptr;
            }
        };

        template<>
        class StreamerTest<DedicatedCache_Uncompressed> : public StreamerTestBase
        {
        protected:
            bool IsUsingArchive() const override { return false; }
            bool CreateDedicatedCache() const override { return true; }
            AZStd::unique_ptr<MockFileBase> CreateMockFile() override
            {
                return AZStd::make_unique<MockUncompressedFile>();
            }
        };

        template<>
        class StreamerTest<GlobalCache_Uncompressed> : public StreamerTestBase
        {
        protected:
            bool IsUsingArchive() const override { return false; }
            bool CreateDedicatedCache() const override { return false; }
            AZStd::unique_ptr<MockFileBase> CreateMockFile() override
            {
                return AZStd::make_unique<MockUncompressedFile>();
            }
        };

        template<>
        class StreamerTest<DedicatedCache_Compressed> : public StreamerTestBase
        {
        protected:
            bool IsUsingArchive() const override { return true; }
            bool CreateDedicatedCache() const override { return true; }
            AZStd::unique_ptr<MockFileBase> CreateMockFile() override
            {
                return AZStd::make_unique<MockCompressedFile>();
            }
        };

        template<>
        class StreamerTest<GlobalCache_Compressed> : public StreamerTestBase
        {
        protected:
            bool IsUsingArchive() const override { return true; }
            bool CreateDedicatedCache() const override { return false; }
            AZStd::unique_ptr<MockFileBase> CreateMockFile() override
            {
                return AZStd::make_unique<MockCompressedFile>();
            }
        };

        TYPED_TEST_CASE_P(StreamerTest);

        // Read a file that's smaller than the cache.
        TYPED_TEST_P(StreamerTest, Read_ReadSmallFileEntirely_FileFullyRead)
        {
            static const size_t fileSize = 50 * 1024; // 50kb file.
            auto testFile = this->CreateTestFile(fileSize, PadArchive::No);

            char buffer[fileSize];
            SizeType readSize = Streamer::Instance().Read(testFile->GetFileName().c_str(), 0, fileSize, buffer, 
                ExecuteWhenIdle, Request::PriorityType::DR_PRIORITY_NORMAL, nullptr, "UnitTest");
            ASSERT_EQ(readSize, fileSize);
            
            this->VerifyTestFile(buffer, fileSize);
        }

        // Read a large file that will need to be broken into chunks.
        TYPED_TEST_P(StreamerTest, Read_ReadLargeFileEntirely_FileFullyRead)
        {
            static const size_t fileSize = 10 * 1024 * 1024; // 10mb file.
            auto testFile = this->CreateTestFile(fileSize, PadArchive::No);

            char* buffer = new char[fileSize];
            SizeType readSize = Streamer::Instance().Read(testFile->GetFileName().c_str(), 0, fileSize, buffer,
                ExecuteWhenIdle, Request::PriorityType::DR_PRIORITY_NORMAL, nullptr, "UnitTest");
            ASSERT_EQ(readSize, fileSize);
            this->VerifyTestFile(buffer, fileSize);
            
            delete[] buffer;
        }

        /* NOTE: Test temporarily disabled until problem with a specific test machine is resolved.
        // Reads multiple small pieces to make sure that the cache is hit, seeded and copied properly.
        TYPED_TEST_P(StreamerTest, Read_ReadMultiplePieces_AllReadRequestWereSuccessful)
        {
            static const size_t fileSize = 2 * 1024 * 1024; // 2MB file.
            // Deliberately not taking a multiple of the file size so at least one read will have a partial cache hit.
            static const size_t bufferSize = 480;
            static const size_t readBlock = bufferSize * sizeof(u32);

            auto testFile = this->CreateTestFile(fileSize, PadArchive::No);

            u32 buffer[bufferSize];
            size_t valueStart = 0;
            size_t block = 0;
            size_t fileRemainder = fileSize;
            for (block = 0; block < fileSize; block += readBlock)
            {
                size_t blockSize = AZStd::min(readBlock, fileRemainder);
                SizeType readSize = Streamer::Instance().Read(testFile->GetFileName().c_str(), block, blockSize, buffer,
                    ExecuteWhenIdle, Request::PriorityType::DR_PRIORITY_NORMAL, nullptr, "UnitTest");
                ASSERT_EQ(readSize, blockSize);
                this->AssertTestFile(buffer, readSize, block);

                valueStart += bufferSize;
                fileRemainder -= readSize;
            }
        }*/

        // Queue a request on a suspended device, then resume to see if gets picked up again.
        TYPED_TEST_P(StreamerTest, SuspendProcesisng_SuspendWhileFileIsQueued_FileIsNotReadUntilProcessingIsRestarted)
        {
            static const size_t fileSize = 50 * 1024; // 50kb file.
            auto testFile = this->CreateTestFile(fileSize, PadArchive::No);

            char buffer[fileSize];
            AZStd::shared_ptr<Request> request = Streamer::Instance().CreateAsyncRead(testFile->GetFileName().c_str(), 0, fileSize, buffer, nullptr,
                ExecuteWhenIdle, Request::PriorityType::DR_PRIORITY_NORMAL, nullptr, "UnitTest");
            Streamer::Instance().SuspendAllDeviceProcessing();
            Streamer::Instance().QueueRequest(request);

            // Sleep for a short while to make sure the test doesn't accidentally outrun the Streamer.
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(500));
            EXPECT_EQ(Request::StateType::ST_PENDING, request->m_state);

            Streamer::Instance().ResumeAllDeviceProcessing();
            // Wait for a maximum of a few seconds for the request to complete. If it doesn't, the suspend is most likely stuck and the test should fail.
            auto start = AZStd::chrono::system_clock::now();
            while (AZStd::chrono::system_clock::now() - start < AZStd::chrono::seconds(5))
            {
                if (request->m_state == Request::StateType::ST_COMPLETED)
                {
                    break;
                }
                else
                {
                    AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(250));
                }
            }

            EXPECT_EQ(Request::StateType::ST_COMPLETED, request->m_state);
        }

        REGISTER_TYPED_TEST_CASE_P(StreamerTest,
            Read_ReadSmallFileEntirely_FileFullyRead,
            Read_ReadLargeFileEntirely_FileFullyRead,
            // NOTE: Test temporarily disabled until problem with a specific test machine is resolved.
            // Read_ReadMultiplePieces_AllReadRequestWereSuccessful,
            SuspendProcesisng_SuspendWhileFileIsQueued_FileIsNotReadUntilProcessingIsRestarted);

        typedef ::testing::Types<GlobalCache_Uncompressed, DedicatedCache_Uncompressed, GlobalCache_Compressed, DedicatedCache_Compressed> StreamerTestCases;

        INSTANTIATE_TYPED_TEST_CASE_P(StreamerTests, StreamerTest, StreamerTestCases);

    } // namespace IO
} // namespace AZ
