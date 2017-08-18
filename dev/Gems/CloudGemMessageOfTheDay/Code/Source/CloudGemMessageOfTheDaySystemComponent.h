
#pragma once

#include <AzCore/Component/Component.h>

#include <CloudGemMessageOfTheDay/CloudGemMessageOfTheDayBus.h>

namespace CloudGemMessageOfTheDay
{
    class CloudGemMessageOfTheDaySystemComponent
        : public AZ::Component
        , protected CloudGemMessageOfTheDayRequestBus::Handler
    {
    public:
        AZ_COMPONENT(CloudGemMessageOfTheDaySystemComponent, "{8C73E66B-0934-4ABE-826C-50B0A4C6DC28}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // CloudGemMessageOfTheDayRequestBus interface implementation

        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////
    };
}
