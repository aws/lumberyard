
#include "CloudGemComputeFarm_precompiled.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include "CloudGemComputeFarmSystemComponent.h"

namespace CloudGemComputeFarm
{
    void CloudGemComputeFarmSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<CloudGemComputeFarmSystemComponent, AZ::Component>()
                ->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<CloudGemComputeFarmSystemComponent>("CloudGemComputeFarm", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void CloudGemComputeFarmSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("CloudGemComputeFarmService"));
    }

    void CloudGemComputeFarmSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("CloudGemComputeFarmService"));
    }

    void CloudGemComputeFarmSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void CloudGemComputeFarmSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void CloudGemComputeFarmSystemComponent::Init()
    {
    }

    void CloudGemComputeFarmSystemComponent::Activate()
    {
        CloudGemComputeFarmRequestBus::Handler::BusConnect();
    }

    void CloudGemComputeFarmSystemComponent::Deactivate()
    {
        CloudGemComputeFarmRequestBus::Handler::BusDisconnect();
    }
}
