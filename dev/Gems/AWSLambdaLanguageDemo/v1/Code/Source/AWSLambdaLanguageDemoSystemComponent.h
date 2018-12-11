
#pragma once

#include <AzCore/Component/Component.h>

#include <AWSLambdaLanguageDemo/AWSLambdaLanguageDemoBus.h>

namespace AWSLambdaLanguageDemo
{
    class AWSLambdaLanguageDemoSystemComponent
        : public AZ::Component
        , protected AWSLambdaLanguageDemoRequestBus::Handler
    {
    public:
        AZ_COMPONENT(AWSLambdaLanguageDemoSystemComponent, "{78F7927E-8B96-4DCF-9501-202409DD0C3A}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // AWSLambdaLanguageDemoRequestBus interface implementation

        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////
    };
}
