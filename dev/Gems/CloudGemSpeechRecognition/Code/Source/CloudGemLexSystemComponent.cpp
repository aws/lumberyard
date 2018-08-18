
#include "CloudGemSpeechRecognition_precompiled.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <Base64.h>
#include <AzCore/JSON/document.h>

#include "CloudGemLexSystemComponent.h"
#include "AWS/ServiceAPI/CloudGemLexClientComponent.h"
#include "VoiceRecorderSystemComponent.h"

namespace CloudGemLex
{
    void CloudGemLexSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<CloudGemLexSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<CloudGemLexSystemComponent>("CloudGemLex", "Adds AWS Lex speech to text")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        AZ::BehaviorContext* bc = azrtti_cast<AZ::BehaviorContext*>(context);
        if (bc)
        {
            bc->Class<CloudGemLexSystemComponent>()
                ->Method("BeginSpeechCapture", &CloudGemLexSystemComponent::BeginSpeechCapture)
                ->Method("EndSpeechCaptureAndCallBot", &CloudGemLexSystemComponent::EndSpeechCaptureAndCallBot)
                ;
        }
    }

    void CloudGemLexSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("CloudGemLexService"));
    }

    void CloudGemLexSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("CloudGemLexService"));
    }

    void CloudGemLexSystemComponent::Init()
    {
    }

    void CloudGemLexSystemComponent::Activate()
    {
        EBUS_EVENT(AZ::ComponentApplicationBus, RegisterComponentDescriptor, CloudGemLex::ServiceAPI::CloudGemLexClientComponent::CreateDescriptor());
        CloudGemLexRequestBus::Handler::BusConnect();
    }

    void CloudGemLexSystemComponent::Deactivate()
    {
        CloudGemLexRequestBus::Handler::BusDisconnect();
    }


    void CloudGemLexSystemComponent::BeginSpeechCapture()
    {
        if (m_isRecording == true)
        {
            AZ_Printf("CloudGemLexSystemComponent", "Already recording. SpeechToText can only record one sound at a time.");
            return;
        }

        VoiceRecorderRequestBus::Broadcast(&VoiceRecorderRequestBus::Events::StartRecording);
    }

    void CloudGemLexSystemComponent::EndSpeechCaptureAndCallBot(
        const AZStd::string& botName,
        const AZStd::string& botAlias,
        const AZStd::string& userId,
        const AZStd::string& sessionAttributes)
    {
        VoiceRecorderRequestBus::Broadcast(&VoiceRecorderRequestBus::Events::StopRecording);

        ServiceAPI::PostAudioRequest request;
        request.bot_alias = botAlias;
        request.name = botName;
        request.user_id = userId;
        request.session_attributes = sessionAttributes;

        VoiceRecorderRequestBus::BroadcastResult(request.audio, &VoiceRecorderRequestBus::Events::GetSoundBufferBase64);
        ServiceAPI::CloudGemLexRequestBus::Broadcast(&ServiceAPI::CloudGemLexRequestBus::Events::PostServicePostaudio, request, nullptr);
    }

}
