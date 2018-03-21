
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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/IO/FileIO.h>

#include "VoiceRecorderSystemComponent.h"
#include <base64.h>

#include <AzCore/IO/FileIO.h>
#include <Microphone/WAVUtil.h>

using namespace Audio;

namespace CloudGemSpeechRecognition
{
    // This is very specific initialization for the format that Lex requires
    const AZ::u32 SAMPLE_RATE = 16000; // preferred data rate for Lex
    const AZ::u16 BITS_PER_SAMPLE = 16; // Lex requires 16 bit samples
    const AZ::u32 BYTES_PER_SAMPLE = (AZ::u32)(BITS_PER_SAMPLE / 8);
    const AZ::u16 CHANNELS = 1;
    const AZ::u32 FRAME_SIZE = 4096; // this is the number of frames that will be requested each tick, might not get that many though.
    const AZ::u32 BUFFER_ALLOC_SIZE = sizeof(WAVHeader) + (SAMPLE_RATE * CHANNELS * BYTES_PER_SAMPLE * 5); // alloc for 5 seconds each time, should be enough for most utterances

    void VoiceRecorderSystemComponent::Init()
    {
#if !defined(AZ_PLATFORM_APPLE)
        m_bufferConfig = SAudioInputConfig(
            AudioInputSourceType::Microphone,
            SAMPLE_RATE,
            CHANNELS,
            BITS_PER_SAMPLE,
            AudioInputSampleType::Int
            );
#endif
    }

    void VoiceRecorderSystemComponent::Activate()
    {
#if defined(AZ_PLATFORM_APPLE)
        CocoaAudioRecorder::initialize();
#else
        m_fillBuffer = nullptr;
#endif
        m_currentB64Buffer.reset(nullptr);
        VoiceRecorderRequestBus::Handler::BusConnect(GetEntityId());
    }

    void VoiceRecorderSystemComponent::Deactivate()
    {
        VoiceRecorderRequestBus::Handler::BusDisconnect();
#if defined(AZ_PLATFORM_APPLE)
        CocoaAudioRecorder::shutdown();
#else
        if (m_fillBuffer)
        {
            delete[] m_fillBuffer;
            m_fillBuffer = nullptr;
        }
#endif
        m_currentB64Buffer.reset(nullptr);
    }

    void VoiceRecorderSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<VoiceRecorderSystemComponent>()
                ->Version(1)
                ;
        
            if (AZ::EditContext* edit = serialize->GetEditContext())
            {
                edit->Class<VoiceRecorderSystemComponent>("Sound Recorder", "Works with the sound system and Lex to record voice")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ;
            }
        }

        AZ::BehaviorContext* bc = azrtti_cast<AZ::BehaviorContext*>(context);
        if (bc)
        {
            bc->Class<VoiceRecorderSystemComponent>()
                ->Method("StartRecording", &VoiceRecorderSystemComponent::StartRecording)
                ->Method("StopRecording", &VoiceRecorderSystemComponent::StopRecording)
                ->Method("IsMicrophoneAvailable", &VoiceRecorderSystemComponent::IsMicrophoneAvailable)
                ;
        }
    }

    void VoiceRecorderSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("VoiceRecorderSystemComponent"));
    }

    void VoiceRecorderSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("VoiceRecorderSystemComponent"));
    }


    void VoiceRecorderSystemComponent::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
#if !defined(AZ_PLATFORM_APPLE)
        bool isCapturing = false;
        MicrophoneRequestBus::BroadcastResult(isCapturing, &MicrophoneRequestBus::Events::IsCapturing);
        if (isCapturing)
        {
            ReadBuffer();
        }
#endif
    }

    void VoiceRecorderSystemComponent::ReadBuffer()
    {
#if !defined(AZ_PLATFORM_APPLE)
        // if there isn't room, don't continue recording
        //TODO if it gets too big, create a new buffer and start recording in to that
        // will need a buffer struct with buffer, current pos and size of buffer
        if (m_curFillBufferPos + (FRAME_SIZE * CHANNELS * BYTES_PER_SAMPLE) <= m_fillBuffer + BUFFER_ALLOC_SIZE)
        {
            AZ::u32 actualSampleFrames = 0;
            MicrophoneRequestBus::BroadcastResult(actualSampleFrames, &MicrophoneRequestBus::Events::GetData,
                reinterpret_cast<void**>(&m_curFillBufferPos),
                FRAME_SIZE,
                m_bufferConfig,
                false);

// The simple downsample used on non-windows plaforms for now yields different results than libsamplerate
// When libsamplerate is ported to other platforms, the "windows" version should become the standard
#if defined(AZ_PLATFORM_WINDOWS)
            AZ::u32 bytesRead = (actualSampleFrames * CHANNELS * BYTES_PER_SAMPLE);
#else
            AZ::u32 bytesRead = actualSampleFrames;
#endif 
            m_curFillBufferPos += bytesRead;
        }
#endif
    }

    void VoiceRecorderSystemComponent::StartRecording()
    {
#if defined(AZ_PLATFORM_APPLE)
        CocoaAudioRecorder::startRecording();
#else
        if (IsMicrophoneAvailable())
        {
            bool didSessionStart = false;

            MicrophoneRequestBus::BroadcastResult(didSessionStart, &MicrophoneRequestBus::Events::StartSession);
            if (didSessionStart)
            {
                AZ::TickBus::Handler::BusConnect();
                m_fillBuffer = new AZ::u8[BUFFER_ALLOC_SIZE];
                m_curFillBufferPos = m_fillBuffer + sizeof(WAVHeader); // leave room to add the wav header
            }
            else
            {
                AZ_Warning("VoiceRecorderSystemComponent", true, "Unable to start recording session");
            }
        }
#endif
    }

    void VoiceRecorderSystemComponent::StopRecording()
    {
#if defined(AZ_PLATFORM_APPLE)
        CocoaAudioRecorder::stopRecording();
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        if(fileIO)
        {
            AZ::IO::HandleType fileHandle;
            if(fileIO->Open(CocoaAudioRecorder::getFileName().c_str(), AZ::IO::OpenMode::ModeRead, fileHandle))
            {
                // read in the wav file and pack it up to send to lex
                AZ::u64 size = 0;
                fileIO->Size(fileHandle, size);
                AZStd::unique_ptr<AZ::s8[]> buffer(new AZ::s8[size]);
                fileIO->Read(fileHandle, buffer.get(), size);
                int encodedSize = Base64::encodedsize_base64(size);
                m_currentB64Buffer.reset(new AZ::s8[encodedSize + 1]);
                Base64::encode_base64(reinterpret_cast<char*>(m_currentB64Buffer.get()), reinterpret_cast<char*>(buffer.get()), size, true);
            }
            else
            {
                AZ_Printf("VoiceRecorderSystemComponent", "Unable to open audio recorder sample file");
                return;
            }
        }
#else
        if (m_fillBuffer && m_curFillBufferPos != m_fillBuffer && IsMicrophoneAvailable())
        {
            AZ::TickBus::Handler::BusDisconnect();

            MicrophoneRequestBus::Broadcast(&MicrophoneRequestBus::Events::EndSession);

            ReadBuffer();

            auto wavSize = static_cast<AZ::u32>(m_curFillBufferPos - m_fillBuffer);

            WAVUtil wavUtil(SAMPLE_RATE, BITS_PER_SAMPLE, CHANNELS, false);
            wavUtil.SetBuffer(m_fillBuffer, wavSize);

            // write test output to user folder
            wavUtil.WriteWAVToFile("@user@/push_to_talk.wav");

            int encodedSize = Base64::encodedsize_base64(wavSize);
            m_currentB64Buffer.reset(new AZ::s8[encodedSize + 1]);
            Base64::encode_base64(reinterpret_cast<char*>(m_currentB64Buffer.get()), reinterpret_cast<char*>(m_fillBuffer), wavSize, true);

            delete[] m_fillBuffer;
            m_fillBuffer = nullptr;
        }
#endif
    }

    bool VoiceRecorderSystemComponent::IsMicrophoneAvailable()
    {
#if defined(AZ_PLATFORM_APPLE)
        return true;
#else
        return m_isMicrophoneInitialized;
#endif
    }

    AZStd::string VoiceRecorderSystemComponent::GetSoundBufferBase64()
    {
        if (m_currentB64Buffer == nullptr)
        {
            return{};
        }
        else
        {
            return AZStd::string(reinterpret_cast<char*>(m_currentB64Buffer.get()));
        }
    }
}


