/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include "LegacyGameInterface_precompiled.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include "LegacyGameInterfaceSystemComponent.h"
#include <Core/EditorGame.h>
#include <System/GameStartup.h>

#ifdef AZ_MONOLITHIC_BUILD
#include <IEditorGame.h>
#endif

namespace LegacyGameInterface
{
    void LegacyGameInterfaceSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<LegacyGameInterfaceSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<LegacyGameInterfaceSystemComponent>("LegacyGameInterface", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void LegacyGameInterfaceSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("LegacyGameInterfaceService"));
    }

    void LegacyGameInterfaceSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("LegacyGameInterfaceService"));
    }

    void LegacyGameInterfaceSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void LegacyGameInterfaceSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    IGameStartup* LegacyGameInterfaceSystemComponent::CreateGameStartup()
    {
        static char buffer[sizeof(GameStartup)];
        return static_cast<IGameStartup*>(new(buffer)GameStartup());
    }

    IEditorGame* LegacyGameInterfaceSystemComponent::CreateEditorGame()
    {
        return new EditorGame();
    }

    void LegacyGameInterfaceSystemComponent::Init()
    {
    }

    void LegacyGameInterfaceSystemComponent::Activate()
    {
        LegacyGameInterfaceRequestBus::Handler::BusConnect();
        EditorGameRequestBus::Handler::BusConnect();
    }

    void LegacyGameInterfaceSystemComponent::Deactivate()
    {
        EditorGameRequestBus::Handler::BusConnect();
        LegacyGameInterfaceRequestBus::Handler::BusDisconnect();
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
