
#pragma once

#include <AzCore/Component/Component.h>

#include <CloudGemTextToSpeech/CloudGemTextToSpeechBus.h>

namespace CloudGemTextToSpeech
{
    class CloudGemTextToSpeechSystemComponent
        : public AZ::Component
        , protected CloudGemTextToSpeechRequestBus::Handler
    {
    public:
        AZ_COMPONENT(CloudGemTextToSpeechSystemComponent, "{D26E218C-989B-40F6-8709-0600FE7AC1CD}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // CloudGemTextToSpeechRequestBus interface implementation

        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////
    };
}
