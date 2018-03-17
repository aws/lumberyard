
#include "CryLegacyAnimation_precompiled.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include "CryLegacyAnimationSystemComponent.h"

namespace CryLegacyAnimation
{
    void CryLegacyAnimationSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<CryLegacyAnimationSystemComponent, AZ::Component>()
                ->Version(0)
                ->SerializerForEmptyClass();

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<CryLegacyAnimationSystemComponent>("CryLegacyAnimation", "A boilerplate gem that enable cry animation when activated")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Animation (Legacy)")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void CryLegacyAnimationSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("CryLegacyAnimationService", 0xbe300459));
    }

    void CryLegacyAnimationSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("CryLegacyAnimationService", 0xbe300459));
    }

    void CryLegacyAnimationSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void CryLegacyAnimationSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void CryLegacyAnimationSystemComponent::Init()
    {
    }

    void CryLegacyAnimationSystemComponent::Activate()
    {
        CryLegacyAnimationRequestBus::Handler::BusConnect();
    }

    void CryLegacyAnimationSystemComponent::Deactivate()
    {
        CryLegacyAnimationRequestBus::Handler::BusDisconnect();
    }
}
