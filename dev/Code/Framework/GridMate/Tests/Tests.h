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

#ifndef GM_UNITTEST_FIXTURE_H
#define GM_UNITTEST_FIXTURE_H

#include <GridMate/GridMate.h>
#include <GridMate/Session/Session.h>

#include <GridMateTests/Tests_Platform.h>
#include <AzCore/UnitTest/UnitTest.h>

#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/Memory/AllocationRecords.h>

#include <AzCore/Driller/Driller.h>
#include <AzCore/Memory/MemoryDriller.h>

#include <GridMate/Carrier/Carrier.h>

#define GM_TEST_MEMORY_DRILLING 0
//////////////////////////////////////////////////////////////////////////
// Drillers
#include <AzCore/Driller/Driller.h>
#include <AzCore/IO/Streamer.h>

#define AZ_ROOT_TEST_FOLDER ""

#include <AzCore/AzCore_Traits_Platform.h>

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
        : public GridMateTestFixture_Platform
    {
        protected:
            GridMate::IGridMate* m_gridMate;
            AZ::Debug::DrillerSession* m_drillerSession;
            AZ::Debug::DrillerOutputFileStream* m_drillerStream;
            AZ::Debug::DrillerManager* m_drillerManager;
            void* m_allocatorBuffer;
    
        private:
            using Platform = GridMateTestFixture_Platform;

    public:
        GridMateTestFixture(unsigned int memorySize = 100 * 1024 * 1024)
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
            sysAllocDesc.m_heap.m_numFixedMemoryBlocks = 1;
            sysAllocDesc.m_heap.m_fixedMemoryBlocksByteSize[0] = memorySize;
            m_allocatorBuffer = azmalloc(memorySize, sysAllocDesc.m_heap.m_memoryBlockAlignment, AZ::OSAllocator);
            sysAllocDesc.m_heap.m_fixedMemoryBlocks[0] = m_allocatorBuffer;
            AZ::AllocatorInstance<AZ::SystemAllocator>::Create(sysAllocDesc);
            desc.m_allocatorDesc.m_custom = &AZ::AllocatorInstance<AZ::SystemAllocator>::Get();

            // register/enable streamer for data IO operations
            //AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create(AZ::ThreadPoolAllocator::Descriptor());
            //AZ::IO::Streamer::Descriptor streamerDesc;
            //AZ::IO::Streamer::Create(streamerDesc);
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

            Platform::Construct();
        }

        virtual ~GridMateTestFixture()
        {
            Platform::Destruct();

            if (m_gridMate)
            {
                GridMateDestroy(m_gridMate);
                m_gridMate = NULL;
            }

            // stop streamer
            //AZ::IO::Streamer::Destroy();
            //AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();

            AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
            azfree(m_allocatorBuffer, AZ::OSAllocator);

            if (m_drillerManager)
            {
                AZ::Debug::DrillerManager::Destroy(m_drillerManager);
                m_drillerManager = nullptr;
            }
        }

        void Update()
        {
        }
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
