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

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define TESTS_H_SECTION_1 1
#define TESTS_H_SECTION_2 2
#define TESTS_H_SECTION_3 3
#define TESTS_H_SECTION_4 4
#define TESTS_H_SECTION_5 5
#define TESTS_H_SECTION_6 6
#endif

#ifndef GM_UNITTEST_FIXTURE_H
#define GM_UNITTEST_FIXTURE_H

#include <GridMate/GridMate.h>
#include <GridMate/Session/Session.h>

#include <AzCore/UnitTest/UnitTest.h>

#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/Memory/AllocationRecords.h>

#include <AzCore/Driller/Driller.h>
#include <AzCore/Memory/MemoryDriller.h>

#include <GridMate/Carrier/Carrier.h>

#define GM_TEST_MEMORY_DRILLING 0

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION TESTS_H_SECTION_1
#include AZ_RESTRICTED_FILE(Tests_h, AZ_RESTRICTED_PLATFORM)
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION TESTS_H_SECTION_2
#include AZ_RESTRICTED_FILE(Tests_h, AZ_RESTRICTED_PLATFORM)
#endif

#if defined(AZ_PLATFORM_WINDOWS)
#include <WinSock2.h>
#endif

//////////////////////////////////////////////////////////////////////////
// Drillers
#include <AzCore/Driller/Driller.h>
#include <AzCore/IO/Streamer.h>

#define AZ_ROOT_TEST_FOLDER ""
//////////////////////////////////////////////////////////////////////////

namespace UnitTest
{
    struct TestCarrierDesc
        : GridMate::CarrierDesc
    {
        TestCarrierDesc() : CarrierDesc()
        {
            m_connectionTimeoutMS = 15000;
        }
    };

    class GridMateTestFixture
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION TESTS_H_SECTION_3
#include AZ_RESTRICTED_FILE(Tests_h, AZ_RESTRICTED_PLATFORM)
#endif
    {
    protected:
        GridMate::IGridMate* m_gridMate;
        AZ::Debug::DrillerSession* m_drillerSession;
        AZ::Debug::DrillerOutputFileStream* m_drillerStream;
        AZ::Debug::DrillerManager* m_drillerManager;
        void* m_allocatorBuffer;

    public:
        GridMateTestFixture(unsigned int memorySize = 100* 1024* 1024)
            : m_gridMate(nullptr)
            , m_drillerSession(nullptr)
            , m_drillerStream(nullptr)
            , m_drillerManager(nullptr)
            , m_allocatorBuffer(nullptr)
        {
            GridMate::GridMateDesc desc;
#if GM_TEST_MEMORY_DRILLING
            m_drillerManager = AZ::Debug::DrillerManager::Create();
            m_drillerManager->Register(aznew AZ::Debug::MemoryDriller);

            desc.m_allocatorDesc.m_allocationRecords = true;
#endif
            AZ::SystemAllocator::Descriptor sysAllocDesc;
            sysAllocDesc.m_heap.m_numMemoryBlocks = 1;
            sysAllocDesc.m_heap.m_memoryBlocksByteSize[0] = memorySize;
            m_allocatorBuffer = azmalloc(memorySize, sysAllocDesc.m_heap.m_memoryBlockAlignment, AZ::OSAllocator);
            sysAllocDesc.m_heap.m_memoryBlocks[0] = m_allocatorBuffer;
            AZ::AllocatorInstance<AZ::SystemAllocator>::Create(sysAllocDesc);
            desc.m_allocatorDesc.m_custom = &AZ::AllocatorInstance<AZ::SystemAllocator>::Get();

            // register/enable streamer for data IO operations
            AZ::IO::Streamer::Descriptor streamerDesc;
            if (strlen(AZ_ROOT_TEST_FOLDER) > 0)
            {
                streamerDesc.m_fileMountPoint = AZ_ROOT_TEST_FOLDER;
            }
            AZ::IO::Streamer::Create(streamerDesc);

            // enable compression on the device (we can find device by name and/or by drive name, etc.)
            //AZ::IO::Device* device = AZ::IO::Streamer::Instance().FindDevice(AZ_ROOT_TEST_FOLDER);
            //AZ_TEST_ASSERT(device); // we must have a valid device
            //bool result = device->RegisterCompressor(aznew AZ::IO::CompressorZLib());
            //AZ_TEST_ASSERT(result); // we should be able to register

            //desc.m_autoInitPlatformNetModules = false;
            m_gridMate = GridMateCreate(desc);
            AZ_TEST_ASSERT(m_gridMate != NULL);

            m_drillerSession = NULL;

            AZ::Debug::AllocationRecords* records = AZ::AllocatorInstance<GridMate::GridMateAllocator>::Get().GetRecords();
            if (records)
            {
                records->SetMode(AZ::Debug::AllocationRecords::RECORD_FULL);
            }

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION TESTS_H_SECTION_4
#include AZ_RESTRICTED_FILE(Tests_h, AZ_RESTRICTED_PLATFORM)
#endif
        }

        virtual ~GridMateTestFixture()
        {
            if (m_gridMate)
            {
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION TESTS_H_SECTION_5
#include AZ_RESTRICTED_FILE(Tests_h, AZ_RESTRICTED_PLATFORM)
#endif

                StopDrilling();

                GridMateDestroy(m_gridMate);
                m_gridMate = NULL;
            }

            // stop streamer
            AZ::IO::Streamer::Destroy();

            AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
            azfree(m_allocatorBuffer, AZ::OSAllocator);

            if (m_drillerManager)
            {
                AZ::Debug::DrillerManager::Destroy(m_drillerManager);
                m_drillerManager = nullptr;
            }
        }

        void StartDrilling(const char* name)
        {
            (void) name;
            // TODO FIX
            /*
            if( m_drillerSession != NULL ) return;

            //////////////////////////////////////////////////////////////////////////
            // Setup drillers
            AZ::Debug::DrillerManager& dm = AZ::Debug::DrillerManager::Instance();

            // create a list of driller we what to drill
            AZ::Debug::DrillerManager::DrillerListType dillersToDrill;
            AZ::Debug::DrillerManager::DrillerInfo di;
            for(int i = 0; i < dm.GetNumDrillerDesc(); ++i )
            {
            AZ::Debug::DrillerDescriptor* dd = dm.GetDrillerDesc(i);
            di.id = dd->GetId();            // set driller id
            dillersToDrill.push_back(di);
            }

            char fullFileName[256];
            azsnprintf(fullFileName,AZ_ARRAY_SIZE(fullFileName),"%s.gm",name);

            // open a driller output file stream
            AZ::IO::Stream* stream = AZ::IO::Streamer::Instance().RegisterFileStream(fullFileName, AZ::IO::Streamer::FS_WRITE_CREATE);
            //stream->WriteCompressed(IO::CompressorZLib::TypeId(),0,2);
            m_drillerStream.BeginWrite(stream);

            // start a driller session with the file stream and the list of drillers
            m_drillerSession = dm.Start(m_drillerStream,dillersToDrill);*/
        }

        void StopDrilling()
        {
            /*          if(m_drillerSession)
            {
            AZ::Debug::DrillerManager::Instance().Stop(m_drillerSession);
            m_drillerSession = NULL;
            }
            if( m_drillerStream.GetIOStream() != NULL )
            {
            AZ_Assert(m_drillerStream.GetIOStream()->IsWrite(),"The stream should be open for writing");
            m_drillerStream.EndWrite();
            }               */
        }

        void Update()
        {
            //          if(m_gridMate && m_drillerSession)
            //              AZ::Debug::DrillerManager::Instance().FrameUpdate();
        }

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION TESTS_H_SECTION_6
#include AZ_RESTRICTED_FILE(Tests_h, AZ_RESTRICTED_PLATFORM)
#endif
    };

    /**
    * Since we test many MP modules that require MP memory, but
    * we don't want to start a real MP service. We just "hack" the
    * system and initialize the GridMate MP allocator manually.
    * If you try to use m_gridMate->StartMultiplayerService the code will Warn that the allocator
    * has been created!
    */
    class GridMateMPTestFixture
        : public GridMateTestFixture
    {
    public:
        GridMateMPTestFixture(unsigned int memorySize = 100* 1024* 1024, bool needWSA = true)
            : GridMateTestFixture(memorySize)
            , m_needWSA(needWSA)
        {
            if (m_gridMate)
            {
#if AZ_TRAIT_OS_USE_WINDOWS_SOCKETS
                if (m_needWSA)
                {
                    WSAData wsaData;
                    int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
                    if (err != 0)
                    {
                        AZ_TracePrintf("GridMate", "GridMateMPTestFixture: Failed on WSAStartup with code %d\n", err);
                    }
                }
#endif

#ifndef GRIDMATE_FOR_TOOLS
                GridMate::GridMateAllocatorMP::Descriptor desc;
                desc.m_custom = &AZ::AllocatorInstance<GridMate::GridMateAllocator>::Get();
                AZ::AllocatorInstance<GridMate::GridMateAllocatorMP>::Create(desc);

                AZ::Debug::AllocationRecords* records = AZ::AllocatorInstance<GridMate::GridMateAllocatorMP>::Get().GetRecords();
                if (records)
                {
                    records->SetMode(AZ::Debug::AllocationRecords::RECORD_FULL);
                }
#endif
            }
        }

        ~GridMateMPTestFixture()
        {
            if (m_gridMate)
            {
#ifndef GRIDMATE_FOR_TOOLS
                AZ::AllocatorInstance<GridMate::GridMateAllocatorMP>::Destroy();
#endif
            }

#if AZ_TRAIT_OS_USE_WINDOWS_SOCKETS
            if (m_needWSA)
            {
                WSACleanup();
            }
#endif
        }

        bool m_needWSA;
    };
}

namespace Render
{
    void Flip();
} // namespace Render

#if defined(AZ_TESTS_ENABLED)
// Wraps AZTest to a more GoogleTest format
#define GM_TEST_SUITE(...) namespace __VA_ARGS__ { namespace UnitTest {
#define GM_TEST(...) TEST(__VA_ARGS__, __VA_ARGS__) { ::UnitTest::__VA_ARGS__ tester; tester.run(); }
#define GM_TEST_SUITE_END(...) } }
#else
#define GM_TEST_SUITE(...)
#define GM_TEST(...)
#define GM_TEST_SUITE_END(...)
#endif

#if !defined(AZ_TESTS_ENABLED)
#undef AZ_TEST_ASSERT
#define AZ_TEST_ASSERT(...) ((void)(__VA_ARGS__));
#endif

#endif // GM_UNITTEST_FIXTURE_H
