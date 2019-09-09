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

#include "CloudGemLeaderboard_precompiled.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>


#include "CloudGemLeaderboardSystemComponent.h"
#include "AWS/ServiceAPI/CloudGemLeaderboardClientComponent.h"

namespace CloudGemLeaderboard
{
    void CloudGemLeaderboardSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<CloudGemLeaderboardSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<CloudGemLeaderboardSystemComponent>("CloudGemLeaderboard", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        // ->Attribute(AZ::Edit::Attributes::Category, "") Set a category
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void CloudGemLeaderboardSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("CloudGemLeaderboardService"));
    }

    void CloudGemLeaderboardSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("CloudGemLeaderboardService"));
    }

    void CloudGemLeaderboardSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void CloudGemLeaderboardSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void CloudGemLeaderboardSystemComponent::Init()
    {
    }

    void CloudGemLeaderboardSystemComponent::Activate()
    {
        EBUS_EVENT(AZ::ComponentApplicationBus, RegisterComponentDescriptor, CloudGemLeaderboard::ServiceAPI::CloudGemLeaderboardClientComponent::CreateDescriptor());
        CloudGemLeaderboardRequestBus::Handler::BusConnect();
    }

    void CloudGemLeaderboardSystemComponent::Deactivate()
    {
        EBUS_EVENT_ID(AZ::AzTypeInfo<CloudGemLeaderboard::ServiceAPI::CloudGemLeaderboardClientComponent>::Uuid(), AZ::ComponentDescriptorBus, ReleaseDescriptor);
        CloudGemLeaderboardRequestBus::Handler::BusDisconnect();
    }
}
