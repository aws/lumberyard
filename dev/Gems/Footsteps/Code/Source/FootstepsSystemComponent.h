
#pragma once

#include <AzCore/Component/Component.h>

#include <Footsteps/FootstepsBus.h>

namespace Footsteps
{
    class FootstepsSystemComponent
        : public AZ::Component
        , protected FootstepsRequestBus::Handler
    {
    public:
        AZ_COMPONENT(FootstepsSystemComponent, "{175DBE54-5155-4421-B6B2-E9110CF7CE55}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // FootstepsRequestBus interface implementation

        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////
    };
}
