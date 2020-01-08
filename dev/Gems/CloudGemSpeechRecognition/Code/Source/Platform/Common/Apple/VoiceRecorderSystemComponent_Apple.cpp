
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

#include <AzCore/IO/FileIO.h>

#include "CocoaAudioRecorder.h"

namespace CloudGemSpeechRecognition
{
    namespace Platform
    {
        class AppleVoiceRecorderImplementation : public VoiceRecorderSystemComponent::Implementation
        {
        public:
            void Init() override;
            void Activate() override;
            void Deactivate() override;
            void ReadBuffer() override;
            void StartRecording() override;
            void StopRecording() override;
            bool IsMicrophoneAvailable() override;
        };
    }
}

void CloudGemSpeechRecognition::Platform::AppleVoiceRecorderImplementation::Init()
{
}

void CloudGemSpeechRecognition::Platform::AppleVoiceRecorderImplementation::Activate()
{
    CocoaAudioRecorder::initialize();
}

void CloudGemSpeechRecognition::Platform::AppleVoiceRecorderImplementation::Deactivate()
{
    CocoaAudioRecorder::shutdown();
}

void CloudGemSpeechRecognition::Platform::AppleVoiceRecorderImplementation::ReadBuffer()
{
}

void CloudGemSpeechRecognition::Platform::AppleVoiceRecorderImplementation::StartRecording()
{
    CocoaAudioRecorder::startRecording();
}

void CloudGemSpeechRecognition::Platform::AppleVoiceRecorderImplementation::StopRecording()
{
    CocoaAudioRecorder::stopRecording();
    AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
    if (fileIO)
    {
        AZ::IO::HandleType fileHandle;
        if (fileIO->Open(CocoaAudioRecorder::getFileName().c_str(), AZ::IO::OpenMode::ModeRead, fileHandle))
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
}

bool CloudGemSpeechRecognition::Platform::AppleVoiceRecorderImplementation::IsMicrophoneAvailable()
{
    return true;
}

AZStd::unique_ptr<CloudGemSpeechRecognition::VoiceRecorderSystemComponent::Implementation> CloudGemSpeechRecognition::VoiceRecorderSystemComponent::Implementation::Create()
{
    return AZStd::make_unique<CloudGemSpeechRecognition::Platform::AppleVoiceRecorderImplementation>();
}
