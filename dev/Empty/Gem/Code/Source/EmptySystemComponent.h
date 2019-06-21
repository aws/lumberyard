#pragma once

#include <AzCore/Component/Component.h>

#include <Empty/EmptyBus.h>

namespace Empty
{
    class EmptySystemComponent
        : public AZ::Component
        , protected EmptyRequestBus::Handler
    {
    public:
        AZ_COMPONENT(EmptySystemComponent, "{65A8CBA7-8F6D-4D8E-9285-27F3BBE3BD29}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // EmptyRequestBus interface implementation

        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////
    };
}
