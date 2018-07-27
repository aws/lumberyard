
#include "CloudGemDefectReportSample_precompiled.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include "CloudGemDefectReportSampleSystemComponent.h"

namespace CloudGemDefectReportSample
{
    void CloudGemDefectReportSampleSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<CloudGemDefectReportSampleSystemComponent, AZ::Component>()
                ->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<CloudGemDefectReportSampleSystemComponent>("CloudGemDefectReportSample", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void CloudGemDefectReportSampleSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("CloudGemDefectReportSampleService"));
    }

    void CloudGemDefectReportSampleSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("CloudGemDefectReportSampleService"));
    }

    void CloudGemDefectReportSampleSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void CloudGemDefectReportSampleSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void CloudGemDefectReportSampleSystemComponent::Init()
    {
    }

    void CloudGemDefectReportSampleSystemComponent::Activate()
    {
        CloudGemDefectReportSampleRequestBus::Handler::BusConnect();
    }

    void CloudGemDefectReportSampleSystemComponent::Deactivate()
    {
        CloudGemDefectReportSampleRequestBus::Handler::BusDisconnect();
    }
}
