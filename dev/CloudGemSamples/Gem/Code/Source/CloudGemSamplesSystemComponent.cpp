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

#include "StdAfx.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include "CloudGemSamplesSystemComponent.h"
#include "Core/EditorGame.h"
#include "System/GameStartup.h"

namespace LYGame
{
    void CloudGemSamplesSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<CloudGemSamplesSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<CloudGemSamplesSystemComponent>("CloudGemSamples", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        // ->Attribute(AZ::Edit::Attributes::Category, "") Set a category
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void CloudGemSamplesSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("CloudGemSamplesService", 0x5ef46e07));
        provided.push_back(AZ_CRC("LegacyEditorGameRequests", 0x2be64eb2));
    }

    void CloudGemSamplesSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("CloudGemSamplesService", 0x5ef46e07));
        incompatible.push_back(AZ_CRC("LegacyEditorGameRequests", 0x2be64eb2));
    }

    void CloudGemSamplesSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("CryLegacyService", 0xdfa3b326));
    }

    void CloudGemSamplesSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }


    IGameStartup* CloudGemSamplesSystemComponent::CreateGameStartup()
    {
        static char buffer[sizeof(GameStartup)];
        return static_cast<IGameStartup*>(new(buffer)GameStartup());
    }

    IEditorGame* CloudGemSamplesSystemComponent::CreateEditorGame()
    {
        return new EditorGame();
    }

    void CloudGemSamplesSystemComponent::Init()
    {
    }

    void CloudGemSamplesSystemComponent::Activate()
    {
        EditorGameRequestBus::Handler::BusConnect();
        CloudGemSamplesRequestBus::Handler::BusConnect();
    }

    void CloudGemSamplesSystemComponent::Deactivate()
    {
        EditorGameRequestBus::Handler::BusDisconnect();
        CloudGemSamplesRequestBus::Handler::BusDisconnect();
    }
}

#ifdef AZ_MONOLITHIC_BUILD
extern "C"
{
    IGameStartup* CreateGameStartup()
    {
        IGameStartup* pGameStartup = nullptr;
        EditorGameRequestBus::BroadcastResult(pGameStartup, &EditorGameRequestBus::Events::CreateGameStartup);
        return pGameStartup;
    }
}
#endif
