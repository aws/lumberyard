
#pragma once

#include <AzCore/Component/Component.h>

#include <CloudGemInGameSurvey/CloudGemInGameSurveyBus.h>

namespace CloudGemInGameSurvey
{
    class CloudGemInGameSurveySystemComponent
        : public AZ::Component
        , protected CloudGemInGameSurveyRequestBus::Handler
    {
    public:
        AZ_COMPONENT(CloudGemInGameSurveySystemComponent, "{7891EEF0-1478-4706-BA93-6D953BCDDC93}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // CloudGemInGameSurveyRequestBus interface implementation

        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////
    };
}
