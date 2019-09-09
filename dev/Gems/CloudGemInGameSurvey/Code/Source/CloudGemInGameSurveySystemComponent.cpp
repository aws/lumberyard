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

#include "CloudGemInGameSurvey_precompiled.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>

#include "CloudGemInGameSurveySystemComponent.h"
#include "AWS/ServiceApi/CloudGemInGameSurveyClientComponent.h"

namespace CloudGemInGameSurvey
{
    void CloudGemInGameSurveySystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<CloudGemInGameSurveySystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<CloudGemInGameSurveySystemComponent>("CloudGemInGameSurvey", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        // ->Attribute(AZ::Edit::Attributes::Category, "") Set a category
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void CloudGemInGameSurveySystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("CloudGemInGameSurveyService"));
    }

    void CloudGemInGameSurveySystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("CloudGemInGameSurveyService"));
    }

    void CloudGemInGameSurveySystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void CloudGemInGameSurveySystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void CloudGemInGameSurveySystemComponent::Init()
    {
    }

    void CloudGemInGameSurveySystemComponent::Activate()
    {
        CloudGemInGameSurveyRequestBus::Handler::BusConnect();

        EBUS_EVENT(AZ::ComponentApplicationBus, RegisterComponentDescriptor, CloudGemInGameSurvey::ServiceAPI::CloudGemInGameSurveyClientComponent::CreateDescriptor());
    }

    void CloudGemInGameSurveySystemComponent::Deactivate()
    {
        CloudGemInGameSurveyRequestBus::Handler::BusDisconnect();
    }
}
