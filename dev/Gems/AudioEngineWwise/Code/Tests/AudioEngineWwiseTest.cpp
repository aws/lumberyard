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

#include <AzTest/AzTest.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/Application/Application.h>

#include <AudioSystemImpl_wwise.h>
#include <Config_wwise.h>

#include <AK/SoundEngine/Common/AkSoundEngine.h>
#include <AK/SoundEngine/Common/AkMemoryMgr.h>
#include <AK/SoundEngine/Common/AkModule.h>

#include <platform.h>
#include <Mocks/IConsoleMock.h>
#include <Mocks/ISystemMock.h>

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
            NiceMock<SystemMock> m_system;
        };

        void SetupEnvironment() override
        {
            // Create AZ::SystemAllocator
            if (!AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady())
            {
                AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
            }

            // Setup Mocks on a stub environment
            m_mocks = new MockHolder();
            m_stubEnv.bProfilerEnabled = false;
            m_stubEnv.pFrameProfileSystem = nullptr;
            m_stubEnv.callbackStartSection = nullptr;
            m_stubEnv.callbackEndSection = nullptr;

            m_stubEnv.pConsole = &m_mocks->m_console;
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
        }

        void TeardownEnvironment() override
        {
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
    public:
        AZ_TEST_CLASS_ALLOCATOR(AudioSystemImplWwiseTests)

    protected:
        AudioSystemImplWwiseTests()
            : m_wwiseImpl("")
        {}

        void SetUp() override
        {
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

        void TearDown() override
        {
            // Term Wwise
            AK::SoundEngine::Term();
            AK::IAkStreamMgr::Get()->Destroy();
            AK::MemoryMgr::Term();
        }

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


    class AudioSystemImpl_wwise_Test
        : public CAudioSystemImpl_wwise
    {
    public:
        AZ_TEST_CLASS_ALLOCATOR(AudioSystemImpl_wwise_Test)

        explicit AudioSystemImpl_wwise_Test(const char* assetPlatform)
            : CAudioSystemImpl_wwise(assetPlatform)
        {}

        friend class GTEST_TEST_CLASS_NAME_(AudioSystemImplWwiseConfigTests, WwiseSetBankPaths_NonDefaultPath_PathMatches);
        friend class GTEST_TEST_CLASS_NAME_(AudioSystemImplWwiseConfigTests, WwieSetBankPaths_NoInitBnk_DefaultPath);
    };


    class AudioSystemImplWwiseConfigTests
        : public ::testing::Test
    {
    public:
        AZ_TEST_CLASS_ALLOCATOR(AudioSystemImplWwiseConfigTests)

    protected:
        AudioSystemImplWwiseConfigTests()
            : m_wwiseImpl("")
        {
        }

        void SetUp() override
        {
            m_app.Start(AZ::ComponentApplication::Descriptor());

            // Store and remove the existing fileIO...
            m_prevFileIO = AZ::IO::FileIOBase::GetInstance();
            if (m_prevFileIO)
            {
                AZ::IO::FileIOBase::SetInstance(nullptr);
            }

            // Replace with a new LocalFileIO...
            m_fileIO = AZStd::make_unique<AZ::IO::LocalFileIO>();
            AZ::IO::FileIOBase::SetInstance(m_fileIO.get());

            // Reflect the wwise config settings...
            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            AZ_TEST_ASSERT(serializeContext != nullptr);
            Audio::Wwise::ConfigurationSettings::Reflect(serializeContext);

            // So we don't have to compute it in each test...
            AZ::StringFunc::AssetDatabasePath::Join(Audio::Wwise::DefaultBanksPath, Audio::Wwise::ConfigFile, m_configFilePath);

            // Set the @assets@ alias to the path where *this* cpp file lives.
            AZStd::string rootFolder(__FILE__);
            AZ::StringFunc::Path::StripFullName(rootFolder);
            m_fileIO->SetAlias("@assets@", rootFolder.c_str());
        }

        void TearDown() override
        {
            // Destroy our LocalFileIO...
            m_fileIO.reset();

            // Replace the old fileIO (set instance to null first)...
            AZ::IO::FileIOBase::SetInstance(nullptr);
            if (m_prevFileIO)
            {
                AZ::IO::FileIOBase::SetInstance(m_prevFileIO);
                m_prevFileIO = nullptr;
            }

            m_app.Stop();
        }

        AZStd::unique_ptr<AZ::IO::LocalFileIO> m_fileIO;
        AZStd::string m_configFilePath;
        Audio::Wwise::ConfigurationSettings::PlatformMapping m_mapEntry;
        AudioSystemImpl_wwise_Test m_wwiseImpl;

    private:
        AzFramework::Application m_app;
        AZ::IO::FileIOBase* m_prevFileIO { nullptr };
    };


    TEST_F(AudioSystemImplWwiseConfigTests, WwiseSetBankPaths_NonDefaultPath_PathMatches)
    {
        // The mapping here points to a custom directory that exists (and contains an init.bnk).
        // The custom bank path should be set.
        Audio::Wwise::ConfigurationSettings config;
        m_mapEntry.m_enginePlatform = AZ_TRAIT_OS_PLATFORM_NAME;
        m_mapEntry.m_bankSubPath = "soundbanks";
        config.m_platformMappings.push_back(m_mapEntry);
        config.Save(m_configFilePath);

        m_wwiseImpl.SetBankPaths();

        m_fileIO->Remove(m_configFilePath.c_str());
        EXPECT_STREQ(m_wwiseImpl.m_soundbankFolder.c_str(), "sounds/wwise/soundbanks/");
    }


    TEST_F(AudioSystemImplWwiseConfigTests, WwieSetBankPaths_NoInitBnk_DefaultPath)
    {
        // The mapping here points to a directory that does not exist (and doesn't contain init.bnk).
        // The default bank path should be set.
        Audio::Wwise::ConfigurationSettings config;
        m_mapEntry.m_enginePlatform = AZ_TRAIT_OS_PLATFORM_NAME;
        m_mapEntry.m_bankSubPath = "no_soundbanks";
        config.m_platformMappings.push_back(m_mapEntry);
        config.Save(m_configFilePath);

        m_wwiseImpl.SetBankPaths();

        m_fileIO->Remove(m_configFilePath.c_str());
        EXPECT_STREQ(m_wwiseImpl.m_soundbankFolder.c_str(), Audio::Wwise::DefaultBanksPath);
    }

} // namespace UnitTest


AZ_UNIT_TEST_HOOK(new UnitTest::WwiseTestEnvironment);
