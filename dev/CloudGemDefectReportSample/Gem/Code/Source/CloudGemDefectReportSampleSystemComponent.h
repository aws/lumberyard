
#pragma once

#include <AzCore/Component/Component.h>

#include <CloudGemDefectReportSample/CloudGemDefectReportSampleBus.h>

namespace CloudGemDefectReportSample
{
    class CloudGemDefectReportSampleSystemComponent
        : public AZ::Component
        , protected CloudGemDefectReportSampleRequestBus::Handler
    {
    public:
        AZ_COMPONENT(CloudGemDefectReportSampleSystemComponent, "{CC9BB2B4-03F1-4299-B2E6-1D49659783BC}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // CloudGemDefectReportSampleRequestBus interface implementation

        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////
    };
}
