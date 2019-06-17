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
#include <AzCore/UnitTest/TestTypes.h>

#include <AudioSystemImpl_wwise.h>

#include <Mocks/IConsoleMock.h>
#include <Mocks/ILogMock.h>
#include <Mocks/ITimerMock.h>
#include <Mocks/ISystemMock.h>

#include <AK/SoundEngine/Common/AkSoundEngine.h>        // Sound engine
#include <AK/SoundEngine/Common/AkMemoryMgr.h>          // Memory Manager
#include <AK/SoundEngine/Common/AkModule.h>             // Default memory and stream managers

using namespace Audio;
using ::testing::NiceMock;

namespace UnitTest
{
    class WwiseTestEnvironment
        : public AZ::Test::ITestEnvironment
    {
    public:
        AZ_TEST_CLASS_ALLOCATOR(WwiseTestEnvironment)

        WwiseTestEnvironment() = default;
        ~WwiseTestEnvironment() override = default;

    protected:

        struct MockHolder
        {
            AZ_TEST_CLASS_ALLOCATOR(MockHolder)

            NiceMock<ConsoleMock> m_console;
            NiceMock<TimerMock> m_timer;
            NiceMock<LogMock> m_log;
            NiceMock<SystemMock> m_system;
        };

        void SetupEnvironment() override
        {
            // Create AZ::SystemAllocator
            if (!AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady())
            {
                AZ::SystemAllocator::Descriptor desc;
                desc.m_heap.m_numFixedMemoryBlocks = 1;
                desc.m_heap.m_fixedMemoryBlocksByteSize[0] = 8 << 20;
                desc.m_heap.m_fixedMemoryBlocks[0] = DebugAlignAlloc(desc.m_heap.m_fixedMemoryBlocksByteSize[0], desc.m_heap.m_memoryBlockAlignment);

                AZ::AllocatorInstance<AZ::SystemAllocator>::Create(desc);
            }

            // Setup Mocks on a stub environment
            m_mocks = new MockHolder();
            m_stubEnv.bProfilerEnabled = false;
            m_stubEnv.pFrameProfileSystem = nullptr;
            m_stubEnv.callbackStartSection = nullptr;
            m_stubEnv.callbackEndSection = nullptr;

            m_stubEnv.pConsole = &m_mocks->m_console;
            m_stubEnv.pTimer = &m_mocks->m_timer;
            m_stubEnv.pLog = &m_mocks->m_log;
            m_stubEnv.pSystem = &m_mocks->m_system;
            gEnv = &m_stubEnv;

            // Create AudioImplAllocator (used by Wwise)
            if (!AZ::AllocatorInstance<AudioImplAllocator>::IsReady())
            {
                AudioImplAllocator::Descriptor allocDesc;
                allocDesc.m_allocationRecords = false;
                allocDesc.m_heap.m_numFixedMemoryBlocks = 1;
                allocDesc.m_heap.m_fixedMemoryBlocksByteSize[0] = 8 << 20;
                allocDesc.m_heap.m_fixedMemoryBlocks[0] = AZ::AllocatorInstance<AZ::OSAllocator>::Get().Allocate(allocDesc.m_heap.m_fixedMemoryBlocksByteSize[0], allocDesc.m_heap.m_memoryBlockAlignment);

                AZ::AllocatorInstance<AudioImplAllocator>::Create(allocDesc);
            }

            // Init Wwise
            AkMemSettings memSettings;
            AK::MemoryMgr::Init(&memSettings);
            AkStreamMgrSettings strmSettings;
            AK::StreamMgr::GetDefaultSettings(strmSettings);
            strmSettings.uMemorySize = 64 << 10;
            AK::StreamMgr::Create(strmSettings);
            AkInitSettings initSettings;
            AK::SoundEngine::GetDefaultInitSettings(initSettings);
            initSettings.uCommandQueueSize = 256 << 10;
            initSettings.uDefaultPoolSize = 2 << 20;
            AkPlatformInitSettings platSettings;
            AK::SoundEngine::GetDefaultPlatformInitSettings(platSettings);
            platSettings.uLEngineDefaultPoolSize = 2 << 20;
            AK::SoundEngine::Init(&initSettings, &platSettings);
        }

        void TeardownEnvironment() override
        {
            AK::SoundEngine::Term();
            AK::IAkStreamMgr::Get()->Destroy();
            AK::MemoryMgr::Term();

            if (AZ::AllocatorInstance<AudioImplAllocator>::IsReady())
            {
                AZ::AllocatorInstance<AudioImplAllocator>::Destroy();
            }

            delete m_mocks;
            m_mocks = nullptr;

            if (AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady())
            {
                AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
            }
        }

    private:
        SSystemGlobalEnvironment m_stubEnv;
        MockHolder* m_mocks = nullptr;
    };

    class AudioSystemImplWwiseTests
        : public ::testing::Test
    {
    protected:
        void SetUp() override {}
        void TearDown() override {}

        CAudioSystemImpl_wwise m_wwiseImpl;
    };

    TEST_F(AudioSystemImplWwiseTests, WwiseSanityTest)
    {
        EXPECT_TRUE(true);
    }

    TEST_F(AudioSystemImplWwiseTests, WwiseMultiPosition_GoodData)
    {
        SATLAudioObjectData_wwise wwiseObject(1, true);
        MultiPositionParams params;
        params.m_positions.push_back(AZ::Vector3(1.f, 2.f, 3.f));
        params.m_type = MultiPositionBehaviorType::Blended;

        auto result = m_wwiseImpl.SetMultiplePositions(&wwiseObject, params);
        EXPECT_EQ(result, eARS_SUCCESS);
    }

    TEST_F(AudioSystemImplWwiseTests, WwiseMultiPosition_BadObject)
    {
        MultiPositionParams params;
        params.m_positions.push_back(AZ::Vector3(1.f, 2.f, 3.f));
        params.m_type = MultiPositionBehaviorType::Separate;

        auto result = m_wwiseImpl.SetMultiplePositions(nullptr, params);
        EXPECT_EQ(result, eARS_FAILURE);
    }

    TEST_F(AudioSystemImplWwiseTests, WwiseMultiPosition_ZeroPositions)
    {
        SATLAudioObjectData_wwise wwiseObject(1, true);
        MultiPositionParams params;
        auto result = m_wwiseImpl.SetMultiplePositions(&wwiseObject, params);
        EXPECT_EQ(result, eARS_SUCCESS);
    }

} // namespace UnitTest


AZ_UNIT_TEST_HOOK(new UnitTest::WwiseTestEnvironment);
