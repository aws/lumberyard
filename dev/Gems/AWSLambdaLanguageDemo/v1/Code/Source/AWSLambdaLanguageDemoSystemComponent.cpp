
#include "AWSLambdaLanguageDemo_precompiled.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include "AWSLambdaLanguageDemoSystemComponent.h"

namespace AWSLambdaLanguageDemo
{
    void AWSLambdaLanguageDemoSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<AWSLambdaLanguageDemoSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<AWSLambdaLanguageDemoSystemComponent>("AWSLambdaLanguageDemo", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void AWSLambdaLanguageDemoSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("AWSLambdaLanguageDemoService"));
    }

    void AWSLambdaLanguageDemoSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("AWSLambdaLanguageDemoService"));
    }

    void AWSLambdaLanguageDemoSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void AWSLambdaLanguageDemoSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void AWSLambdaLanguageDemoSystemComponent::Init()
    {
    }

    void AWSLambdaLanguageDemoSystemComponent::Activate()
    {
        AWSLambdaLanguageDemoRequestBus::Handler::BusConnect();
    }

    void AWSLambdaLanguageDemoSystemComponent::Deactivate()
    {
        AWSLambdaLanguageDemoRequestBus::Handler::BusDisconnect();
    }
}
