
#include "CloudGemTextToSpeech_precompiled.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include "AWS/ServiceAPI/CloudGemTextToSpeechClientComponent.h"



#include "CloudGemTextToSpeechSystemComponent.h"

#include "CloudGemTextToSpeech/TextToSpeech.h"
#include "CloudGemTextToSpeech/SpeechComponent.h"

namespace CloudGemTextToSpeech
{
    void CloudGemTextToSpeechSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<CloudGemTextToSpeechSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<CloudGemTextToSpeechSystemComponent>("CloudGemTextToSpeech", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        // ->Attribute(AZ::Edit::Attributes::Category, "") Set a category
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void CloudGemTextToSpeechSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("CloudGemTextToSpeechService"));
    }

    void CloudGemTextToSpeechSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("CloudGemTextToSpeechService"));
    }

    void CloudGemTextToSpeechSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void CloudGemTextToSpeechSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void CloudGemTextToSpeechSystemComponent::Init()
    {
    }

    void CloudGemTextToSpeechSystemComponent::Activate()
    {
        EBUS_EVENT(AZ::ComponentApplicationBus, RegisterComponentDescriptor, CloudGemTextToSpeech::TextToSpeech::CreateDescriptor());
        EBUS_EVENT(AZ::ComponentApplicationBus, RegisterComponentDescriptor, CloudGemTextToSpeech::SpeechComponent::CreateDescriptor());
        EBUS_EVENT(AZ::ComponentApplicationBus, RegisterComponentDescriptor, CloudGemTextToSpeech::ServiceAPI::CloudGemTextToSpeechClientComponent::CreateDescriptor());
        CloudGemTextToSpeechRequestBus::Handler::BusConnect();
    }

    void CloudGemTextToSpeechSystemComponent::Deactivate()
    {
        CloudGemTextToSpeechRequestBus::Handler::BusDisconnect();
    }
}
