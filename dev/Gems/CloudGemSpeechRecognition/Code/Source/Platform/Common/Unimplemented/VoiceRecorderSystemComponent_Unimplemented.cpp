
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

namespace CloudGemSpeechRecognition
{
    namespace Platform
    {
        class UnimplementedVoiceRecorderImplementation : public VoiceRecorderSystemComponent::Implementation
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

void CloudGemSpeechRecognition::Platform::UnimplementedVoiceRecorderImplementation::Init()
{
}

void CloudGemSpeechRecognition::Platform::UnimplementedVoiceRecorderImplementation::Activate()
{
}

void CloudGemSpeechRecognition::Platform::UnimplementedVoiceRecorderImplementation::Deactivate()
{
}

void CloudGemSpeechRecognition::Platform::UnimplementedVoiceRecorderImplementation::ReadBuffer()
{
}

void CloudGemSpeechRecognition::Platform::UnimplementedVoiceRecorderImplementation::StartRecording()
{
}

void CloudGemSpeechRecognition::Platform::UnimplementedVoiceRecorderImplementation::StopRecording()
{
}

bool CloudGemSpeechRecognition::Platform::UnimplementedVoiceRecorderImplementation::IsMicrophoneAvailable()
{
    return false;
}

AZStd::unique_ptr<CloudGemSpeechRecognition::VoiceRecorderSystemComponent::Implementation> CloudGemSpeechRecognition::VoiceRecorderSystemComponent::Implementation::Create()
{
    return AZStd::make_unique<CloudGemSpeechRecognition::Platform::UnimplementedVoiceRecorderImplementation>();
}
