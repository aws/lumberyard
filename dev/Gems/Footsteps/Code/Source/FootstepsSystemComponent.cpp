
#include "Footsteps_precompiled.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Physics/PhysicsSystemComponentBus.h>

#include "FootstepsSystemComponent.h"

namespace Footsteps
{
    void FootstepsSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<FootstepsSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<FootstepsSystemComponent>("Footsteps", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        // ->Attribute(AZ::Edit::Attributes::Category, "") Set a category
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void FootstepsSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("FootstepsService"));
    }

    void FootstepsSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("FootstepsService"));
    }

    void FootstepsSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void FootstepsSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void FootstepsSystemComponent::Init()
    {
        using namespace AzFramework;

        //AZStd::vector<AZ::EntityId> aabbResults = PhysicsSystemRequestBus::Broadcast(&PhysicsSystemRequestBus::Events::GatherPhysicalEntitiesInAABB, aabb, query);
        //AZStd::vector<AZ::EntityId> pointResults = PhysicsSystemRequestBus::Broadcast(&PhysicsSystemRequestBus::Events::GatherPhysicalEntitiesAroundPoint, center, radius, query);

    }

    void FootstepsSystemComponent::Activate()
    {
        FootstepsRequestBus::Handler::BusConnect();
    }

    void FootstepsSystemComponent::Deactivate()
    {
        FootstepsRequestBus::Handler::BusDisconnect();
    }
}
