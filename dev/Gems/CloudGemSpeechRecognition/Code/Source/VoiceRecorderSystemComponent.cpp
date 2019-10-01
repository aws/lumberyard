
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

#include "VoiceRecorderSystemComponent.h"

namespace CloudGemSpeechRecognition
{
    VoiceRecorderSystemComponent::VoiceRecorderSystemComponent() : m_impl(Implementation::Create())
    {
    }

    void VoiceRecorderSystemComponent::Init()
    {
        m_impl->Init();
    }

    void VoiceRecorderSystemComponent::Activate()
    {
        m_impl->Activate();
        m_impl->m_currentB64Buffer.reset(nullptr);
        VoiceRecorderRequestBus::Handler::BusConnect(GetEntityId());
    }

    void VoiceRecorderSystemComponent::Deactivate()
    {
        VoiceRecorderRequestBus::Handler::BusDisconnect();
        m_impl->Deactivate();
        m_impl->m_currentB64Buffer.reset(nullptr);
    }

    void VoiceRecorderSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<VoiceRecorderSystemComponent, AZ::Component>()
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

    void VoiceRecorderSystemComponent::ReadBuffer()
    {
        m_impl->ReadBuffer();
    }

    void VoiceRecorderSystemComponent::StartRecording()
    {
        m_impl->StartRecording();
    }

    void VoiceRecorderSystemComponent::StopRecording()
    {
        m_impl->StopRecording();
    }

    bool VoiceRecorderSystemComponent::IsMicrophoneAvailable()
    {
        return m_impl->IsMicrophoneAvailable();
    }

    AZStd::string VoiceRecorderSystemComponent::GetSoundBufferBase64()
    {
        if (m_impl->m_currentB64Buffer == nullptr)
        {
            return{};
        }
        else
        {
            return AZStd::string(reinterpret_cast<char*>(m_impl->m_currentB64Buffer.get()));
        }
    }

    VoiceRecorderSystemComponent::Implementation::~Implementation()
    {
    }
}
