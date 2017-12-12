
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

#include <StdAfx.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include "VoiceRecorderSystemComponent.h"
#include <base64.h>

#include <AzCore/IO/FileIO.h>

using namespace Audio;

namespace CloudGemLex
{
    struct WAVHeader
    {
        AZ::u8 riffTag[4];
        AZ::u32 fileSize;
        AZ::u8 waveTag[4];
        AZ::u8 fmtTag[4];
        AZ::u32 fmtSize;
        AZ::u16 audioFormat;
        AZ::u16 channels;
        AZ::u32 sampleRate;
        AZ::u32 byteRate;
        AZ::u16 blockAlign;
        AZ::u16 bitsPerSample;
        AZ::u8 dataTag[4];
        AZ::u32 dataSize;

        WAVHeader();
    };

    // This is very specific initialization for the format that Lex requires
    const AZ::u32 SAMPLE_RATE = 16000; // preferred data rate for Lex
    const AZ::u16 BITS_PER_SAMPLE = 16; // Lex requires 16 bit samples
    const AZ::u32 BYTES_PER_SAMPLE = (AZ::u32)(BITS_PER_SAMPLE / 8);
    const AZ::u16 CHANNELS = 1;
    const AZ::u32 FRAME_SIZE = 4096; // this is the number of frames that will be requested each tick, might not get that many though.
    const AZ::u32 BUFFER_ALLOC_SIZE = sizeof(WAVHeader) + (SAMPLE_RATE * CHANNELS * BYTES_PER_SAMPLE * 5); // alloc for 5 seconds each time 
    //TODO: If we need to talk longer, allocate a larger buffer. For now, cap speech at 5 seconds. There's
    // no realloc in azmemory, perhaps keep a linked list of buffers?

    WAVHeader::WAVHeader()
        : fileSize { 0 }
        , fmtSize { 16 }
        , audioFormat { 1 }     // 1 = PCM
        , channels { CHANNELS }
        , sampleRate { SAMPLE_RATE }
        , byteRate { SAMPLE_RATE * BYTES_PER_SAMPLE }
        , blockAlign { 2 }
        , bitsPerSample { BITS_PER_SAMPLE }
        , dataSize { 0 }
    {
        memcpy(riffTag, "RIFF", 4);
        memcpy(waveTag, "WAVE", 4);
        memcpy(fmtTag, "fmt ", 4);
        memcpy(dataTag, "data", 4);
    }

    void VoiceRecorderSystemComponent::Init()
    {
        m_bufferConfig = SAudioInputConfig(
            AudioInputSourceType::Microphone,
            SAMPLE_RATE,
            CHANNELS,
            BITS_PER_SAMPLE,
            AudioInputSampleType::Int
            );
    }

    void VoiceRecorderSystemComponent::Activate()
    {
        m_fillBuffer = nullptr;
        m_currentB64Buffer = nullptr;
        m_currentB64BufferSize = 0;
        VoiceRecorderRequestBus::Handler::BusConnect(GetEntityId());
    }

    void VoiceRecorderSystemComponent::Deactivate()
    {
        VoiceRecorderRequestBus::Handler::BusDisconnect();
        if (m_fillBuffer)
        {
            delete[] m_fillBuffer;
            m_fillBuffer = nullptr;
        }
        if (m_currentB64Buffer)
        {
            delete[] m_currentB64Buffer;
            m_currentB64Buffer = nullptr;
            m_currentB64BufferSize = 0;
        }
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
        bool isCapturing = false;
        MicrophoneRequestBus::BroadcastResult(isCapturing, &MicrophoneRequestBus::Events::IsCapturing);
        if (isCapturing)
        {
            ReadBuffer();
        }
    }

    void VoiceRecorderSystemComponent::ReadBuffer()
    {
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

            AZ::u32 bytesRead = (actualSampleFrames * CHANNELS * BYTES_PER_SAMPLE);

            m_curFillBufferPos += bytesRead;
        }
    }

    void VoiceRecorderSystemComponent::StartRecording()
    {
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
    }

    void VoiceRecorderSystemComponent::StopRecording()
    {
        if (m_fillBuffer && m_curFillBufferPos != m_fillBuffer && IsMicrophoneAvailable())
        {
            AZ::TickBus::Handler::BusDisconnect();

            MicrophoneRequestBus::Broadcast(&MicrophoneRequestBus::Events::EndSession);

            ReadBuffer();

            auto wavSize = static_cast<AZ::u32>(m_curFillBufferPos - m_fillBuffer);

            SetWAVHeader(m_fillBuffer, wavSize);


            // BEGIN PHISTER TEST
            // This will go to "\dev\Cache\CloudGemSamples\pc\cloudgemsamples\push_to_talk.wav"
            AZStd::string fileName = "push_to_talk.wav";
            AZ::IO::FileIOStream fileStream(fileName.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeBinary);
            if (fileStream.IsOpen())
            {
                auto bytesWritten = fileStream.Write(wavSize, m_fillBuffer);
                AZ_TracePrintf("VoiceRecorderSystemComponent", "Wrote file: %s, %d bytes\n", fileName.c_str(), bytesWritten);
            }
            // END PHISTER TEST

            int encodedSize = Base64::encodedsize_base64(wavSize);
            if (m_currentB64Buffer)
            {
                delete[] m_currentB64Buffer;
                m_currentB64Buffer = nullptr;
                m_currentB64BufferSize = 0;
            }
            m_currentB64Buffer = new char[encodedSize + 1];
            m_currentB64BufferSize = encodedSize;
            Base64::encode_base64(m_currentB64Buffer, reinterpret_cast<char*>(m_fillBuffer), wavSize, true);

            delete[] m_fillBuffer;
            m_fillBuffer = nullptr;
        }
    }

    void VoiceRecorderSystemComponent::SetWAVHeader(AZ::u8* fillBuffer, AZ::u32 bufferSize)
    {
        WAVHeader header;
        header.fileSize = bufferSize - 8;   // the 'RIFF' tag and filesize aren't counted in this.
        header.dataSize = bufferSize - sizeof(WAVHeader);
        AZ::u8* headerBuff = reinterpret_cast<AZ::u8*>(&header);
        ::memcpy(fillBuffer, headerBuff, sizeof(WAVHeader));
    }


    bool VoiceRecorderSystemComponent::IsMicrophoneAvailable()
    {
        return m_isMicrophoneInitialized;
    }

    AZStd::string VoiceRecorderSystemComponent::GetSoundBufferBase64()
    {
        if (m_currentB64Buffer == nullptr || m_currentB64BufferSize == 0)
        {
            return {};
        }
        return AZStd::string(m_currentB64Buffer, m_currentB64BufferSize);
    }
}


