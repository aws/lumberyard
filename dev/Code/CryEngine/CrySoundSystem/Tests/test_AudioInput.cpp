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
#include <IAudioSystem.h>

using namespace Audio;

// Add a 48kHz Sine Wave Table
#include "WaveTable48000Sine.h"
// Defines the following:
//   WaveTable::g_sineTable[]
//   WaveTable::TABLE_SIZE
const AZStd::size_t SAMPLE_RATE = WaveTable::TABLE_SIZE;


//                         //
// PlayMultiChannelAudio51 //
//                         //
class MultiChannelAudio51Test
    : public ::testing::Test
{
public:
    void SetUp() override
    {
        // Speaker channel ordering is laid out to match Audiokinetic's.
        // See AkSpeakerConfig.h - #define AK_IDX_SETUP_5_*

        AZStd::size_t trackSizeBytes = SAMPLE_RATE * sizeof(float);
        m_frontLeft = static_cast<float*>(azmalloc(trackSizeBytes));
        m_frontRight = static_cast<float*>(azmalloc(trackSizeBytes));
        m_center = static_cast<float*>(azmalloc(trackSizeBytes));
        m_rearLeft = static_cast<float*>(azmalloc(trackSizeBytes));
        m_rearRight = static_cast<float*>(azmalloc(trackSizeBytes));
        m_lfe = static_cast<float*>(azmalloc(trackSizeBytes));

        ::memset(m_frontLeft, 0, trackSizeBytes);
        ::memset(m_frontRight, 0, trackSizeBytes);
        ::memset(m_center, 0, trackSizeBytes);
        ::memset(m_rearLeft, 0, trackSizeBytes);
        ::memset(m_rearRight, 0, trackSizeBytes);
        ::memset(m_lfe, 0, trackSizeBytes);

        m_multiTrackData.m_data[0] = m_frontLeft;
        m_multiTrackData.m_data[1] = m_frontRight;
        m_multiTrackData.m_data[2] = m_center;
        m_multiTrackData.m_data[3] = m_rearLeft;
        m_multiTrackData.m_data[4] = m_rearRight;
        m_multiTrackData.m_data[5] = m_lfe;
        m_multiTrackData.m_sizeBytes = trackSizeBytes - 1;  // This works around an edge case in the AudioRingBuffer.h

        enum Channels
        {
            FL = 1,
            FR = 2,
            CN = 4,
            RL = 8,
            RR = 16,
            LF = 32,
        };

        // The idea is to eventually run independent tests, masking on/off discrete speaker channels.
        // Might need to add parameterized fixtures support to integration tests.
        AZ::u32 enabledChannelsMask = (0
            | FL
            | FR
            | CN
            | RL
            | RR
            | LF
            );

        // This will write 1 second of sine wave data into each channel at different frequencies.
        // Only write if the channel is enabled via enabledChannelsMask.  Eventually the mask would become configurable.
        float currPhase;
        float phaseIncr;
        float gain = 1.0f;
        float freqs[6] = { 100.f, 200.f, 1600.f, 400.f, 800.f, 50.f };
        float* tracks[6] = { m_frontLeft, m_frontRight, m_center, m_rearLeft, m_rearRight, m_lfe };

        for (AZ::u32 channel = 0; channel < 6; ++channel)
        {
            if (enabledChannelsMask & (1 << channel))
            {
                currPhase = 0.f;
                phaseIncr = static_cast<float>(freqs[channel]);
                for (AZStd::size_t sample = 0; sample < SAMPLE_RATE; ++sample)
                {
                    tracks[channel][sample] = gain * WaveTable::g_sineTable[static_cast<AZStd::size_t>(currPhase)];
                    currPhase += phaseIncr;
                    if (currPhase >= SAMPLE_RATE)
                    {
                        currPhase -= SAMPLE_RATE;
                    }
                }
            }
        }
    }

    void TearDown() override
    {
        azfree(m_frontLeft);
        azfree(m_frontRight);
        azfree(m_center);
        azfree(m_lfe);
        azfree(m_rearLeft);
        azfree(m_rearRight);
    }

protected:
    AudioStreamMultiTrackData m_multiTrackData;
    float* m_frontLeft = nullptr;
    float* m_frontRight = nullptr;
    float* m_center = nullptr;
    float* m_lfe = nullptr;
    float* m_rearLeft = nullptr;
    float* m_rearRight = nullptr;
};


INTEG_TEST_F(MultiChannelAudio51Test, PlayMultiChannelAudio51)
{
    const char* triggerName = "Play_AudioInput2D";
    TAudioControlID triggerId = INVALID_AUDIO_CONTROL_ID;
    AudioSystemRequestBus::BroadcastResult(triggerId, &AudioSystemRequestBus::Events::GetAudioTriggerID, triggerName);
    EXPECT_TRUE(triggerId != INVALID_AUDIO_CONTROL_ID);

    SAudioInputConfig config;
    config.m_bitsPerSample = 32;
    config.m_numChannels = 6;
    config.m_sampleRate = 48000;
    config.m_sampleType = AudioInputSampleType::Float;
    config.m_sourceType = AudioInputSourceType::ExternalStream;
    TAudioSourceId sourceId = INVALID_AUDIO_SOURCE_ID;
    AudioSystemRequestBus::BroadcastResult(sourceId, &AudioSystemRequestBus::Events::CreateAudioSource, config);
    EXPECT_TRUE(sourceId != INVALID_AUDIO_SOURCE_ID);

    const int sleep_ms = 16;

    AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::ExternalUpdate);
    AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(sleep_ms));

    SAudioRequest request;
    SAudioObjectRequestData<eAORT_EXECUTE_SOURCE_TRIGGER> requestDataPlay(triggerId, sourceId);
    request.nFlags = eARF_PRIORITY_NORMAL;
    request.pData = &requestDataPlay;
    AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, request);

    AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::ExternalUpdate);
    AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(sleep_ms));

    // Read the data into the streaming buffer (to be consumed by wwise)
    AZStd::size_t framesPushed = 0;
    AudioStreamingRequestBus::BroadcastResult(framesPushed, &AudioStreamingRequestBus::Events::ReadStreamingMultiTrackInput, m_multiTrackData);
    EXPECT_EQ(framesPushed, m_multiTrackData.m_sizeBytes / sizeof(float));

    AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::ExternalUpdate);
    AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(sleep_ms));

    int time_ms = 1000;
    int time = time_ms;
    while (time > 0)
    {
        AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::ExternalUpdate);
        AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(sleep_ms));
        time -= sleep_ms;
    }

    SAudioObjectRequestData<eAORT_STOP_TRIGGER> requestDataStop(triggerId);
    request.nFlags = eARF_PRIORITY_NORMAL;
    request.pData = &requestDataStop;
    AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, request);

    // Destroy the audio source, end the mic session, and reset state...
    AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::DestroyAudioSource, sourceId);
    sourceId = INVALID_AUDIO_SOURCE_ID;
    triggerId = INVALID_AUDIO_CONTROL_ID;
}

