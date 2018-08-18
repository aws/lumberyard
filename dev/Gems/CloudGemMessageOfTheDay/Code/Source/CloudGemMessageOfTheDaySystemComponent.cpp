
#include "CloudGemMessageOfTheDay_precompiled.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>

#include "CloudGemMessageOfTheDaySystemComponent.h"
#include "AWS/ServiceAPI/CloudGemMessageOfTheDayClientComponent.h"

namespace CloudGemMessageOfTheDay
{
    void CloudGemMessageOfTheDaySystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<CloudGemMessageOfTheDaySystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<CloudGemMessageOfTheDaySystemComponent>("CloudGemMessageOfTheDay", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        // ->Attribute(AZ::Edit::Attributes::Category, "") Set a category
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void CloudGemMessageOfTheDaySystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("CloudGemMessageOfTheDayService"));
    }

    void CloudGemMessageOfTheDaySystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("CloudGemMessageOfTheDayService"));
    }

    void CloudGemMessageOfTheDaySystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void CloudGemMessageOfTheDaySystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void CloudGemMessageOfTheDaySystemComponent::Init()
    {
    }

    void CloudGemMessageOfTheDaySystemComponent::Activate()
    {
        EBUS_EVENT(AZ::ComponentApplicationBus, RegisterComponentDescriptor, CloudGemMessageOfTheDay::ServiceAPI::CloudGemMessageOfTheDayClientComponent::CreateDescriptor());
        CloudGemMessageOfTheDayRequestBus::Handler::BusConnect();

    }

    void CloudGemMessageOfTheDaySystemComponent::Deactivate()
    {
        EBUS_EVENT_ID(AZ::AzTypeInfo<CloudGemMessageOfTheDay::ServiceAPI::CloudGemMessageOfTheDayClientComponent>::Uuid(), AZ::ComponentDescriptorBus, ReleaseDescriptor);
        CloudGemMessageOfTheDayRequestBus::Handler::BusDisconnect();
    }
}
