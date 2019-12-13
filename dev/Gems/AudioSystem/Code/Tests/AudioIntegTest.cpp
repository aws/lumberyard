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
#include <AzCore/IO/FileIO.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <ATLComponents.h>
#include <ISystem.h>
#include <LmbrCentral/Audio/AudioSystemComponentBus.h>

#include <Mocks/ATLEntitiesMock.h>

using namespace Audio;
using namespace LmbrCentral;
using ::testing::NiceMock;

//
// These tests are going to interact with audio content in a variety of ways, including playing sounds, 
// loading/unloading banks, setting state, generating audio, etc.  These are HIGHLY dependent on the content
// available under test, i.e. the Game Project, and what banks and sounds have been authored there.
// While this is OK to tie it to a specific project in the short term, we will need to have a dedicated
// project for this purpose in the long term.
//
// Audio Triggers in StarterGame
//"Play_HUD_results_fail";
//"Play_HUD_press_start";
//"Play_AMZN_sfx_env_Tport_End";
//"Play_AMZ_sfx_spfx_generator_loop";
//"play_pickup_health_collect";
//"play_music_combat_entered";
//"Play_AudioInput2D"
//"Play_AudioInput3D"
//"Stop_AudioInput2D"
//"Stop_AudioInput3D"
//"Play_EXT_VO"
//
// Banks in StarterGame (modified from default)
// init.bnk - default
// content.bnk - auto-load global
// vox.bnk - manual-load global
//


AZ_INTEG_TEST_HOOK()


//-----------------------//
// Test CATLXmlProcessor //
//-----------------------//

class ATLXmlProcessorTestFixture
    : public ::testing::Test
{
public:
    ATLXmlProcessorTestFixture()
        : m_triggers()
        , m_rtpcs()
        , m_switches()
        , m_environments()
        , m_preloads()
        , m_afcm(m_preloads)
        , m_xmlProcessor(m_triggers, m_rtpcs, m_switches, m_environments, m_preloads, m_afcm)
    {
        m_xmlProcessor.SetDebugNameStore(&m_mockDebugNameStore);
    }

    void SetUp() override
    {
        // Store and remove the existing fileIO...
        m_prevFileIO = AZ::IO::FileIOBase::GetInstance();
        if (m_prevFileIO)
        {
            AZ::IO::FileIOBase::SetInstance(nullptr);
        }

        // Replace with a new LocalFileIO...
        m_fileIO = AZStd::make_unique<AZ::IO::LocalFileIO>();
        AZ::IO::FileIOBase::SetInstance(m_fileIO.get());

        // Get exe folder from ComponentApplication...
        AZStd::string rootFolder;
        AZ::ComponentApplicationBus::BroadcastResult(rootFolder, &AZ::ComponentApplicationBus::Events::GetExecutableFolder);
        AzFramework::StringFunc::Path::Join(rootFolder.c_str(), "..", rootFolder);

        // Set up paths...
        m_xmlProcessor.SetRootPath(m_audioTestAlias);
        m_fileIO->SetAlias(m_audioTestAlias, rootFolder.c_str());
    }

    void TearDown() override
    {
        // Destroy our LocalFileIO...
        m_fileIO->ClearAlias(m_audioTestAlias);
        m_fileIO.reset();

        // Replace the old fileIO (set instance to null first)...
        AZ::IO::FileIOBase::SetInstance(nullptr);
        if (m_prevFileIO)
        {
            AZ::IO::FileIOBase::SetInstance(m_prevFileIO);
            m_prevFileIO = nullptr;
        }
    }

protected:
    TATLTriggerLookup m_triggers;
    TATLRtpcLookup m_rtpcs;
    TATLSwitchLookup m_switches;
    TATLEnvironmentLookup m_environments;
    TATLPreloadRequestLookup m_preloads;
    CFileCacheManager m_afcm;

    CATLXmlProcessor m_xmlProcessor;

private:
    NiceMock<ATLDebugNameStoreMock> m_mockDebugNameStore;

    const char* m_audioTestAlias { "@audiotestroot@" };
    AZ::IO::FileIOBase* m_prevFileIO { nullptr };
    AZStd::unique_ptr<AZ::IO::LocalFileIO> m_fileIO;
};

INTEG_TEST_F(ATLXmlProcessorTestFixture, ParsePreloadsXml)
{
    m_xmlProcessor.ParsePreloadsData("TestAssets/Audio/ATLData", eADS_GLOBAL);

    EXPECT_EQ(m_preloads.size(), 1);

    auto it = m_preloads.find(AudioStringToID<TAudioPreloadRequestID>("content"));
    EXPECT_NE(it, m_preloads.end());

    m_xmlProcessor.ClearPreloadsData(eADS_GLOBAL);
}

INTEG_TEST_F(ATLXmlProcessorTestFixture, ParseControlsXml)
{
    m_xmlProcessor.ParseControlsData("TestAssets/Audio/ATLData", eADS_GLOBAL);

    EXPECT_EQ(m_triggers.size(), 1);

    auto itTrigger = m_triggers.find(AudioStringToID<TAudioControlID>("AssignedTriggerTest"));
    EXPECT_NE(itTrigger, m_triggers.end());

    EXPECT_EQ(m_rtpcs.size(), 1);

    auto itRtpc = m_rtpcs.find(AudioStringToID<TAudioControlID>("AssignedRtpcTest"));
    EXPECT_NE(itRtpc, m_rtpcs.end());

    EXPECT_EQ(m_switches.size(), 2);

    auto itSwitch = m_switches.find(AudioStringToID<TAudioControlID>("InternalSwitchTest"));
    EXPECT_NE(itSwitch, m_switches.end());

    if (itSwitch != m_switches.end())
    {
        EXPECT_EQ(itSwitch->second->cStates.size(), 2);

        auto itState = itSwitch->second->cStates.find(AudioStringToID<TAudioSwitchStateID>("InternalSwitchState0"));
        EXPECT_NE(itState, itSwitch->second->cStates.end());

        itState = itSwitch->second->cStates.find(AudioStringToID<TAudioSwitchStateID>("InternalSwitchState1"));
        EXPECT_NE(itState, itSwitch->second->cStates.end());
    }

    itSwitch = m_switches.find(AudioStringToID<TAudioControlID>("SwitchTest"));
    EXPECT_NE(itSwitch, m_switches.end());

    if (itSwitch != m_switches.end())
    {
        EXPECT_EQ(itSwitch->second->cStates.size(), 2);

        auto itState = itSwitch->second->cStates.find(AudioStringToID<TAudioSwitchStateID>("SwitchState0"));
        EXPECT_NE(itState, itSwitch->second->cStates.end());

        itState = itSwitch->second->cStates.find(AudioStringToID<TAudioSwitchStateID>("SwitchState1"));
        EXPECT_NE(itState, itSwitch->second->cStates.end());
    }

    EXPECT_EQ(m_environments.size(), 1);

    auto itEnv = m_environments.find(AudioStringToID<TAudioEnvironmentID>("EnvironmentTest"));
    EXPECT_NE(itEnv, m_environments.end());

    m_xmlProcessor.ClearControlsData(eADS_GLOBAL);
}


//        //
// Sanity //
//        //
INTEG_TEST(AudioIntegrationTests, SanityTest)
{
    // Sanity check that the SSystemGlobalEnvironment is available.
    EXPECT_TRUE(gEnv != nullptr);
    EXPECT_TRUE(gEnv->pConsole != nullptr);
}


//                //
// ExecuteTrigger //
//                //
INTEG_TEST(AudioIntegrationTests, ExecuteTriggerTest_WaitTillFinished)
{
    const char* triggerName = "Play_AMZN_sfx_env_Tport_End";

    TAudioControlID triggerId = INVALID_AUDIO_CONTROL_ID;
    AudioSystemRequestBus::BroadcastResult(triggerId, &AudioSystemRequestBus::Events::GetAudioTriggerID, triggerName);
    EXPECT_TRUE(triggerId != INVALID_AUDIO_CONTROL_ID);
    if (triggerId == INVALID_AUDIO_CONTROL_ID)
    {
        // No need to continue this test
        return;
    }

    // Setup a sound-finished callback
    int callbackCookie = 666;
    bool soundFinished = false;
    auto finishedCallback = [](const SAudioRequestInfo* const requestInfo) -> void
    {
        if (requestInfo->eAudioRequestType == eART_AUDIO_CALLBACK_MANAGER_REQUEST)
        {
            const auto notificationType = static_cast<EAudioCallbackManagerRequestType>(requestInfo->nSpecificAudioRequest);
            if (notificationType == eACMRT_REPORT_FINISHED_TRIGGER_INSTANCE)
            {
                if (requestInfo->eResult == eARR_SUCCESS)
                {
                    auto finished = reinterpret_cast<bool*>(requestInfo->pUserData);
                    ASSERT_TRUE(finished != nullptr);
                    EXPECT_FALSE(*finished);
                    *finished = !(*finished);
                    EXPECT_TRUE(*finished);

                    auto cookie = reinterpret_cast<int*>(requestInfo->pOwner);
                    ASSERT_TRUE(cookie != nullptr);
                    EXPECT_EQ(*cookie, 666);
                }
            }
        }
    };

    // Register the callback
    AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::AddRequestListener,
        finishedCallback,
        &callbackCookie,
        eART_AUDIO_CALLBACK_MANAGER_REQUEST,
        eACMRT_REPORT_FINISHED_TRIGGER_INSTANCE
    );

    // Execute Trigger
    SAudioRequest request;
    SAudioObjectRequestData<eAORT_EXECUTE_TRIGGER> requestData(triggerId, 0.f);
    request.nAudioObjectID = INVALID_AUDIO_OBJECT_ID;
    request.nFlags = (eARF_PRIORITY_NORMAL | eARF_SYNC_FINISHED_CALLBACK);
    request.pData = &requestData;
    request.pOwner = &callbackCookie;
    request.pUserData = &soundFinished;

    EXPECT_NO_FATAL_FAILURE(AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, request));

    // While the sound is still playing, pump audio udpates
    while (!soundFinished)
    {
        AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::ExternalUpdate);
        AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(10));
    }

    // Remove the callback
    AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::RemoveRequestListener, finishedCallback, &callbackCookie);
}


//             //
// GetDuration //
//             //
class GetDurationTest
    : public ::testing::Test
    , protected AudioTriggerNotificationBus::Handler
{
public:
    GetDurationTest()
        : m_triggerId(INVALID_AUDIO_CONTROL_ID)
        , m_duration(0.f)
        , m_estimatedDuration(0.f)
        , m_gotDuration(false)
    {}

    void SetUp() override
    {
        const char* triggerName = "Play_HUD_press_start";
        AudioSystemRequestBus::BroadcastResult(m_triggerId, &AudioSystemRequestBus::Events::GetAudioTriggerID, triggerName);
        EXPECT_TRUE(m_triggerId != INVALID_AUDIO_CONTROL_ID);
        AudioTriggerNotificationBus::Handler::BusConnect(m_triggerId);
    }

    void TearDown() override
    {
        AudioTriggerNotificationBus::Handler::BusDisconnect();
    }

protected:
    void ReportDurationInfo(TAudioEventID eventId, float duration, float estimatedDuration) override
    {
        m_duration = duration;
        m_estimatedDuration = estimatedDuration;
        //m_gotDuration = true;     // For some strange reason, uncommenting this line causes a shutdown atexit crash.
    }

    TAudioControlID m_triggerId;
    float m_duration;
    float m_estimatedDuration;
    bool m_gotDuration;
};

INTEG_TEST_F(GetDurationTest, GetDuration)
{
    if (m_triggerId == INVALID_AUDIO_CONTROL_ID)
    {
        return;
    }
    // Execute Trigger
    SAudioRequest request;
    SAudioObjectRequestData<eAORT_EXECUTE_TRIGGER> requestData(m_triggerId, 0.f);
    request.nAudioObjectID = INVALID_AUDIO_OBJECT_ID;
    request.nFlags = (eARF_PRIORITY_NORMAL);
    request.pData = &requestData;

    EXPECT_NO_FATAL_FAILURE(AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, request));

    int timeout = 200;
    while (!m_gotDuration && timeout >= 0)
    {
        AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::ExternalUpdate);
        AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(10));
        timeout -= 10;
    }

    EXPECT_TRUE(m_duration != 0.f);
    EXPECT_FLOAT_EQ(m_duration, 20.f);
}


//           //
// Load Bank //
//           //

// Helper class to test Audio Preload notifications
class BankLoadNotificationTest
    : public ::testing::Test
    , protected AudioPreloadNotificationBus::Handler
{
public:
    BankLoadNotificationTest()
        : m_preloadId(INVALID_AUDIO_PRELOAD_REQUEST_ID)
        , m_cached(false)
    {
    }

    void SetUp() override
    {
        const char* preloadName = "vox";
        AudioSystemRequestBus::BroadcastResult(m_preloadId, &AudioSystemRequestBus::Events::GetAudioPreloadRequestID, preloadName);
        EXPECT_TRUE(m_preloadId != INVALID_AUDIO_PRELOAD_REQUEST_ID);
        AudioPreloadNotificationBus::Handler::BusConnect(m_preloadId);
    }

    void TearDown() override
    {
        AudioPreloadNotificationBus::Handler::BusDisconnect();
    }

    bool IsLoaded() const
    {
        return m_cached;
    }

protected:
    void OnAudioPreloadCached() override
    {
        EXPECT_FALSE(m_cached);
        m_cached = true;
    }

    void OnAudioPreloadUncached() override
    {
        EXPECT_TRUE(m_cached);
        m_cached = false;
    }

    TAudioPreloadRequestID m_preloadId;
    bool m_cached;
};

// DISABLED TEST - Keep this disabled until streaming changes come in (tracking: LY-95259), which will fix an EBus assert.
INTEG_TEST_F(BankLoadNotificationTest, DISABLED_BankLoadedNotification)
{
    // Load the bank
    EXPECT_FALSE(IsLoaded());
    SAudioRequest request;
    SAudioManagerRequestData<eAMRT_PRELOAD_SINGLE_REQUEST> loadRequestData(m_preloadId);
    request.nFlags = (eARF_PRIORITY_NORMAL);
    request.pData = &loadRequestData;

    AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, request);

    // Immediately after issuing the request, we expect that it's still not loaded.
    EXPECT_FALSE(IsLoaded());

    // Loop while the bank isn't loaded and pump updates.
    while (!IsLoaded())
    {
        AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::ExternalUpdate);
        AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(1));
    }

    // Now the bank should be loaded.
    EXPECT_TRUE(IsLoaded());

    // Unload the bank
    SAudioManagerRequestData<eAMRT_UNLOAD_SINGLE_REQUEST> unloadRequestData(m_preloadId);
    request.pData = &unloadRequestData;

    AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, request);

    // Loop while the bank is still loaded and pump updates.
    while (IsLoaded())
    {
        AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::ExternalUpdate);
        AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(2));
    }

    // Now the bank should be unloaded.
    EXPECT_FALSE(IsLoaded());
}


//                                 //
// Global AudioSystemComponent API //
//                                 //
INTEG_TEST(AudioIntegrationTests, GlobalExecuteTrigger)
{
    AudioSystemComponentRequestBus::Broadcast(&AudioSystemComponentRequestBus::Events::GlobalExecuteAudioTrigger,
        "Play_HUD_press_start", AZ::EntityId());

    int time = 200; // ms
    while (time > 0)
    {
        AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::ExternalUpdate);
        AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(10));
        time -= 10;
    }
}

INTEG_TEST(AudioIntegrationTests, DISABLED_GlobalSetRtpc_QuarterVolume)
{
    AudioSystemComponentRequestBus::Broadcast(&AudioSystemComponentRequestBus::Events::GlobalExecuteAudioTrigger, "Play_AMZN_sfx_env_Tport_End", AZ::EntityId());
    int time = 3000;
    const int sleep_ms = 10;

    AudioSystemComponentRequestBus::Broadcast(&AudioSystemComponentRequestBus::Events::GlobalSetAudioRtpc, "rtpc_MasterVolume", 0.25f);

    while (time >= 0)
    {
        AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::ExternalUpdate);
        AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(sleep_ms));
        time -= sleep_ms;
    }

    AudioSystemComponentRequestBus::Broadcast(&AudioSystemComponentRequestBus::Events::GlobalStopAllSounds);
    AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::ExternalUpdate);
}

INTEG_TEST(AudioIntegrationTests, DISABLED_GlobalSetRtpc_FullVolume)
{
    AudioSystemComponentRequestBus::Broadcast(&AudioSystemComponentRequestBus::Events::GlobalExecuteAudioTrigger, "Play_AMZN_sfx_env_Tport_End", AZ::EntityId());
    int time = 3000;
    const int sleep_ms = 10;

    AudioSystemComponentRequestBus::Broadcast(&AudioSystemComponentRequestBus::Events::GlobalSetAudioRtpc, "rtpc_MasterVolume", 1.f);

    while (time >= 0)
    {
        AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::ExternalUpdate);
        AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(sleep_ms));
        time -= sleep_ms;
    }

    AudioSystemComponentRequestBus::Broadcast(&AudioSystemComponentRequestBus::Events::GlobalStopAllSounds);
    AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::ExternalUpdate);
}

INTEG_TEST(AudioIntegrationTests, DISABLED_GlobalSetSwitchState_HighPass)
{
    AudioSystemComponentRequestBus::Broadcast(&AudioSystemComponentRequestBus::Events::GlobalSetAudioSwitchState, "demo_state", "high_pass");
    AudioSystemComponentRequestBus::Broadcast(&AudioSystemComponentRequestBus::Events::GlobalExecuteAudioTrigger, "play_music_combat_entered", AZ::EntityId());

    int time = 4000;
    const int sleep_ms = 16;
    while (time >= 0)
    {
        AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::ExternalUpdate);
        AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(sleep_ms));
        time -= sleep_ms;
    }

    AudioSystemComponentRequestBus::Broadcast(&AudioSystemComponentRequestBus::Events::GlobalStopAllSounds);
    AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::ExternalUpdate);
}


INTEG_TEST(AudioIntegrationTests, DISABLED_GlobalSetSwitchState_LowPass)
{
    AudioSystemComponentRequestBus::Broadcast(&AudioSystemComponentRequestBus::Events::GlobalSetAudioSwitchState, "demo_state", "low_pass");
    AudioSystemComponentRequestBus::Broadcast(&AudioSystemComponentRequestBus::Events::GlobalExecuteAudioTrigger, "play_music_combat_entered", AZ::EntityId());

    int time = 4000;
    const int sleep_ms = 16;
    while (time >= 0)
    {
        AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::ExternalUpdate);
        AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(sleep_ms));
        time -= sleep_ms;
    }

    AudioSystemComponentRequestBus::Broadcast(&AudioSystemComponentRequestBus::Events::GlobalStopAllSounds);
    AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::ExternalUpdate);
}

INTEG_TEST(AudioIntegrationTests, DISABLED_GlobalSetSwitchState_HalfVolume)
{
    AudioSystemComponentRequestBus::Broadcast(&AudioSystemComponentRequestBus::Events::GlobalSetAudioSwitchState, "demo_state", "half_volume");
    AudioSystemComponentRequestBus::Broadcast(&AudioSystemComponentRequestBus::Events::GlobalExecuteAudioTrigger, "play_music_combat_entered", AZ::EntityId());

    int time = 4000;
    const int sleep_ms = 16;
    while (time >= 0)
    {
        AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::ExternalUpdate);
        AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(sleep_ms));
        time -= sleep_ms;
    }

    AudioSystemComponentRequestBus::Broadcast(&AudioSystemComponentRequestBus::Events::GlobalStopAllSounds);
    AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::ExternalUpdate);
}


INTEG_TEST(AudioIntegrationTests, DISABLED_GlobalSetSwitchState_Normal)
{
    AudioSystemComponentRequestBus::Broadcast(&AudioSystemComponentRequestBus::Events::GlobalSetAudioSwitchState, "demo_state", "none");
    AudioSystemComponentRequestBus::Broadcast(&AudioSystemComponentRequestBus::Events::GlobalExecuteAudioTrigger, "play_music_combat_entered", AZ::EntityId());

    int time = 4000;
    const int sleep_ms = 16;
    while (time >= 0)
    {
        AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::ExternalUpdate);
        AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(sleep_ms));
        time -= sleep_ms;
    }

    AudioSystemComponentRequestBus::Broadcast(&AudioSystemComponentRequestBus::Events::GlobalStopAllSounds);
    AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::ExternalUpdate);
}
