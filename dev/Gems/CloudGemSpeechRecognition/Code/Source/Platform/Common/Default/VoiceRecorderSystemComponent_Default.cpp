
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

#include <CloudGemSpeechRecognition_precompiled.h>

#include <VoiceRecorderSystemComponent.h>
#include <Base64.h>

#include <AzCore/Component/TickBus.h>
#include <AzCore/IO/FileIO.h>
#include <Microphone/WAVUtil.h>

#include <CloudGemSpeechRecognition_Traits_Platform.h>

namespace CloudGemSpeechRecognition
{
    // This is very specific initialization for the format that Lex requires
    const AZ::u32 SAMPLE_RATE = 16000; // preferred data rate for Lex
    const AZ::u16 BITS_PER_SAMPLE = 16; // Lex requires 16 bit samples
    const AZ::u32 BYTES_PER_SAMPLE = (AZ::u32)(BITS_PER_SAMPLE / 8);
    const AZ::u16 CHANNELS = 1;
    const AZ::u32 FRAME_SIZE = 4096; // this is the number of frames that will be requested each tick, might not get that many though.
    const AZ::u32 BUFFER_ALLOC_SIZE = sizeof(Audio::WAVHeader) + (SAMPLE_RATE * CHANNELS * BYTES_PER_SAMPLE * 5); // alloc for 5 seconds each time, should be enough for most utterances

    class DefaultVoiceRecorderImplementation : public VoiceRecorderSystemComponent::Implementation, public AZ::TickBus::Handler
    {
    public:
        // VoiceRecorderImpl
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        void ReadBuffer() override;
        void StartRecording() override;
        void StopRecording() override;
        bool IsMicrophoneAvailable() override;

        // TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

    private:
        // Reserved for future use when we can tell if mic initialized
        bool m_isMicrophoneInitialized{ true };

        AZ::u8* m_fillBuffer{ nullptr };
        AZ::u8* m_curFillBufferPos{ nullptr };
        Audio::SAudioInputConfig m_bufferConfig;
    };
}

void CloudGemSpeechRecognition::DefaultVoiceRecorderImplementation::Init()
{
    m_bufferConfig = Audio::SAudioInputConfig(
        Audio::AudioInputSourceType::Microphone,
        SAMPLE_RATE,
        CHANNELS,
        BITS_PER_SAMPLE,
        Audio::AudioInputSampleType::Int
    );
}

void CloudGemSpeechRecognition::DefaultVoiceRecorderImplementation::Activate()
{
    m_fillBuffer = nullptr;
}

void CloudGemSpeechRecognition::DefaultVoiceRecorderImplementation::Deactivate()
{
    if (m_fillBuffer)
    {
        delete[] m_fillBuffer;
        m_fillBuffer = nullptr;
    }
}

void CloudGemSpeechRecognition::DefaultVoiceRecorderImplementation::OnTick(float deltaTime, AZ::ScriptTimePoint time)
{
    AZ_UNUSED(deltaTime);
    AZ_UNUSED(time);

    bool isCapturing = false;
    Audio::MicrophoneRequestBus::BroadcastResult(isCapturing, &Audio::MicrophoneRequestBus::Events::IsCapturing);
    if (isCapturing)
    {
        ReadBuffer();
    }
}

void CloudGemSpeechRecognition::DefaultVoiceRecorderImplementation::ReadBuffer()
{
    // if there isn't room, don't continue recording
    //TODO if it gets too big, create a new buffer and start recording in to that
    // will need a buffer struct with buffer, current pos and size of buffer
    if (m_curFillBufferPos + (FRAME_SIZE * CHANNELS * BYTES_PER_SAMPLE) <= m_fillBuffer + BUFFER_ALLOC_SIZE)
    {
        size_t actualSampleFrames = 0;
        Audio::MicrophoneRequestBus::BroadcastResult(actualSampleFrames, &Audio::MicrophoneRequestBus::Events::GetData,
            reinterpret_cast<void**>(&m_curFillBufferPos),
            FRAME_SIZE,
            m_bufferConfig,
            false);

        // The simple downsample used on non-windows plaforms for now yields different results than libsamplerate
        // When libsamplerate is ported to other platforms, the first version should become the standard
#if AZ_TRAIT_CLOUDGEMSPEECHRECOGNITION_SUPPORT_LIBSAMPLERATE
        AZ::u32 bytesRead = (static_cast<AZ::u32>(actualSampleFrames) * CHANNELS * BYTES_PER_SAMPLE);
#else
        AZ::u32 bytesRead = static_cast<AZ::u32>(actualSampleFrames);
#endif 
        m_curFillBufferPos += bytesRead;
    }
}

void CloudGemSpeechRecognition::DefaultVoiceRecorderImplementation::StartRecording()
{
    if (IsMicrophoneAvailable())
    {
        bool didSessionStart = false;

        Audio::MicrophoneRequestBus::BroadcastResult(didSessionStart, &Audio::MicrophoneRequestBus::Events::StartSession);
        if (didSessionStart)
        {
            AZ::TickBus::Handler::BusConnect();
            m_fillBuffer = new AZ::u8[BUFFER_ALLOC_SIZE];
            m_curFillBufferPos = m_fillBuffer + sizeof(Audio::WAVHeader); // leave room to add the wav header
        }
        else
        {
            AZ_Warning("VoiceRecorderSystemComponent", true, "Unable to start recording session");
        }
    }
}

void CloudGemSpeechRecognition::DefaultVoiceRecorderImplementation::StopRecording()
{
    if (m_fillBuffer && m_curFillBufferPos != m_fillBuffer && IsMicrophoneAvailable())
    {
        AZ::TickBus::Handler::BusDisconnect();

        Audio::MicrophoneRequestBus::Broadcast(&Audio::MicrophoneRequestBus::Events::EndSession);

        ReadBuffer();

        auto wavSize = static_cast<AZ::u32>(m_curFillBufferPos - m_fillBuffer);

        Audio::WAVUtil wavUtil(SAMPLE_RATE, BITS_PER_SAMPLE, CHANNELS, false);
        wavUtil.SetBuffer(m_fillBuffer, wavSize);

        // write test output to user folder
        wavUtil.WriteWAVToFile("@user@/push_to_talk.wav");

        int encodedSize = Base64::encodedsize_base64(wavSize);
        m_currentB64Buffer.reset(new AZ::s8[encodedSize + 1]);
        Base64::encode_base64(reinterpret_cast<char*>(m_currentB64Buffer.get()), reinterpret_cast<char*>(m_fillBuffer), wavSize, true);

        delete[] m_fillBuffer;
        m_fillBuffer = nullptr;
    }
}

bool CloudGemSpeechRecognition::DefaultVoiceRecorderImplementation::IsMicrophoneAvailable()
{
    return m_isMicrophoneInitialized;
}

AZStd::unique_ptr<CloudGemSpeechRecognition::VoiceRecorderSystemComponent::Implementation> CloudGemSpeechRecognition::VoiceRecorderSystemComponent::Implementation::Create()
{
    return AZStd::make_unique<CloudGemSpeechRecognition::DefaultVoiceRecorderImplementation>();
}
