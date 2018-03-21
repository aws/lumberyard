
#include "CloudGemInGameSurvey_precompiled.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>

#include "CloudGemInGameSurveySystemComponent.h"
#include "AWS/ServiceApi/CloudGemInGameSurveyClientComponent.h"

namespace CloudGemInGameSurvey
{
    void CloudGemInGameSurveySystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<CloudGemInGameSurveySystemComponent, AZ::Component>()
                ->Version(0)
                ->SerializerForEmptyClass();

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<CloudGemInGameSurveySystemComponent>("CloudGemInGameSurvey", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        // ->Attribute(AZ::Edit::Attributes::Category, "") Set a category
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void CloudGemInGameSurveySystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("CloudGemInGameSurveyService"));
    }

    void CloudGemInGameSurveySystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("CloudGemInGameSurveyService"));
    }

    void CloudGemInGameSurveySystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void CloudGemInGameSurveySystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void CloudGemInGameSurveySystemComponent::Init()
    {
    }

    void CloudGemInGameSurveySystemComponent::Activate()
    {
        CloudGemInGameSurveyRequestBus::Handler::BusConnect();

        EBUS_EVENT(AZ::ComponentApplicationBus, RegisterComponentDescriptor, CloudGemInGameSurvey::ServiceAPI::CloudGemInGameSurveyClientComponent::CreateDescriptor());
    }

    void CloudGemInGameSurveySystemComponent::Deactivate()
    {
        CloudGemInGameSurveyRequestBus::Handler::BusDisconnect();
    }
}
