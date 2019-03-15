/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
*
*/

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
