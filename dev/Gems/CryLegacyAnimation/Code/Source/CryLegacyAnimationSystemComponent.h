
#pragma once

#include <AzCore/Component/Component.h>

#include <CryLegacyAnimation/CryLegacyAnimationBus.h>

namespace CryLegacyAnimation
{
    class CryLegacyAnimationSystemComponent
        : public AZ::Component
        , protected CryLegacyAnimationRequestBus::Handler
    {
    public:
        AZ_COMPONENT(CryLegacyAnimationSystemComponent, "{0B2CC82A-81AA-4206-A42E-4A1FD252CA10}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // CryLegacyAnimationRequestBus interface implementation

        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////
    };
}
