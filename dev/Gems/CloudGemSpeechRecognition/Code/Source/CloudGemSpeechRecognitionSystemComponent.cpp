
#include "CloudGemSpeechRecognition_precompiled.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <Base64.h>
#include <AzCore/JSON/document.h>

#include "CloudGemSpeechRecognitionSystemComponent.h"
#include "AWS/ServiceAPI/CloudGemSpeechRecognitionClientComponent.h"
#include "VoiceRecorderSystemComponent.h"

namespace CloudGemSpeechRecognition
{

    void CloudGemSpeechRecognitionSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<CloudGemSpeechRecognitionSystemComponent, AZ::Component>()
                ->Version(1)
                ->SerializerForEmptyClass();

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<CloudGemSpeechRecognitionSystemComponent>("CloudGemSpeechRecognition", "Adds AWS Lex speech to text")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        AZ::BehaviorContext* bc = azrtti_cast<AZ::BehaviorContext*>(context);
        if (bc)
        {
            bc->EBus<SpeechRecognitionRequestBus>("CloudGemSpeechRecoginition", "SpeechRecognitionRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Cloud Gem")
                ->Event("BeginSpeechCapture", &SpeechRecognitionRequestBus::Events::BeginSpeechCapture)
                ->Event("EndSpeechCaptureAndCallBot", &SpeechRecognitionRequestBus::Events::EndSpeechCaptureAndCallBot)
                ;
        }
    }

    void CloudGemSpeechRecognitionSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("CloudGemSpeechRecognitionService"));
    }

    void CloudGemSpeechRecognitionSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("CloudGemSpeechRecognitionService"));
    }

    void CloudGemSpeechRecognitionSystemComponent::Init()
    {
    }

    void CloudGemSpeechRecognitionSystemComponent::Activate()
    {
        EBUS_EVENT(AZ::ComponentApplicationBus, RegisterComponentDescriptor, CloudGemSpeechRecognition::ServiceAPI::CloudGemSpeechRecognitionClientComponent::CreateDescriptor());
        SpeechRecognitionRequestBus::Handler::BusConnect();
    }

    void CloudGemSpeechRecognitionSystemComponent::Deactivate()
    {
        SpeechRecognitionRequestBus::Handler::BusDisconnect();
    }


    void CloudGemSpeechRecognitionSystemComponent::BeginSpeechCapture()
    {
        if (m_isRecording == true)
        {
            AZ_Printf("CloudGemSpeechRecognitionSystemComponent", "Already recording. SpeechToText can only record one sound at a time.");
            return;
        }

        VoiceRecorderRequestBus::Broadcast(&VoiceRecorderRequestBus::Events::StartRecording);
    }

    void CloudGemSpeechRecognitionSystemComponent::EndSpeechCaptureAndCallBot(
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
        if (request.audio.empty())
        {
            AZ_Warning("CloudGemSpeechRecognitionService", false, "Recorded audio buffer was empty so no request was sent to Lex. Perhaps there is no microphone connected?");
            return;
        }

        ServiceAPI::CloudGemSpeechRecognitionRequestBus::Broadcast(&ServiceAPI::CloudGemSpeechRecognitionRequestBus::Events::PostServicePostaudio, request, nullptr);
    }

}
