
#pragma once

#include <AzCore/Component/Component.h>

#include <CloudGemComputeFarm/CloudGemComputeFarmBus.h>

namespace CloudGemComputeFarm
{
    class CloudGemComputeFarmSystemComponent
        : public AZ::Component
        , protected CloudGemComputeFarmRequestBus::Handler
    {
    public:
        AZ_COMPONENT(CloudGemComputeFarmSystemComponent, "{F75BF084-E0EE-46D1-BF53-5FBD71D166B8}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // CloudGemComputeFarmRequestBus interface implementation

        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////
    };
}
