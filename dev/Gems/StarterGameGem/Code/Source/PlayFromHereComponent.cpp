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
#include "StarterGameGem_precompiled.h"
#include "PlayFromHereComponent.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Crc.h>


namespace StarterGameGem
{

    void PlayFromHereComponent::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<PlayFromHereComponent, AZ::Component>()
                ->Version(1)
                ->SerializerForEmptyClass()
            ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<PlayFromHereComponent>("Play From Here", "Moves this entity when the 'Play From Here' menu option is used.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Game")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/SG_Icon.png")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/SG_Icon.dds")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
				;
            }
        }
    }

    void PlayFromHereComponent::Init()
    {
    }

    void PlayFromHereComponent::Activate()
    {
#ifdef STARTER_GAME_EDITOR
        PlayFromHereEditorSystemComponentNotificationBus::Handler::BusConnect(GetEntityId());
#endif
    }

	void PlayFromHereComponent::Deactivate()
	{
#ifdef STARTER_GAME_EDITOR
        PlayFromHereEditorSystemComponentNotificationBus::Handler::BusDisconnect();
#endif
    }

#ifdef STARTER_GAME_EDITOR
    void PlayFromHereComponent::OnPlayFromHere(const AZ::Vector3& pos)
    {
        AZ::EntityId entityId = GetEntityId();
        AZ::Transform tm = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(tm, entityId, &AZ::TransformBus::Events::GetWorldTM);
        tm.SetTranslation(pos);
        AZ::TransformBus::Event(entityId, &AZ::TransformBus::Events::SetWorldTM, tm);
    }
#endif

}

