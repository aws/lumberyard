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


#include "StdAfx.h"
#include "StarterGameNavigationComponent.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include "VisualisePathSystemComponent.h"


namespace StarterGameGem
{

	// Behavior Context forwarder for StarterGameNavigationComponentNotificationBus
	class StarterGameBehaviorNavigationComponentNotificationBusHandler
		: public StarterGameNavigationComponentNotificationBus::Handler
		, public AZ::BehaviorEBusHandler
	{
	public:
		AZ_EBUS_BEHAVIOR_BINDER(StarterGameBehaviorNavigationComponentNotificationBusHandler,"{467912F0-F099-442A-ADC8-EAE8CDE29633}", AZ::SystemAllocator,
            OnPathFoundFirstPoint);

		bool OnPathFoundFirstPoint(LmbrCentral::PathfindRequest::NavigationRequestId requestId, AZ::Vector3 firstPoint) override
		{
			bool traverse = true;
			CallResult(traverse, FN_OnPathFoundFirstPoint, requestId, firstPoint);
			return traverse;
		}

	};


	void StarterGameNavigationComponent::Init()
	{

	}

	void StarterGameNavigationComponent::Activate()
	{
		LmbrCentral::NavigationComponentNotificationBus::Handler::BusConnect(GetEntityId());
	}

	void StarterGameNavigationComponent::Deactivate()
	{
        LmbrCentral::NavigationComponentNotificationBus::Handler::BusDisconnect();

		VisualisePathSystemComponent::GetInstance()->ClearPath(GetEntityId());
	}

	bool StarterGameNavigationComponent::OnPathFound(LmbrCentral::PathfindRequest::NavigationRequestId requestId, AZStd::shared_ptr<const INavPath> currentPath)
	{
		if (currentPath == nullptr || currentPath->Empty())
			return false;

		bool traverse = true;
		Vec3 firstPos;
		currentPath->GetPosAlongPath(firstPos, 1.0f, false, false);
		AZ::Vector3 AZFirstPos = LYVec3ToAZVec3(firstPos);
		EBUS_EVENT_ID_RESULT(traverse, GetEntityId(), StarterGameNavigationComponentNotificationBus, OnPathFoundFirstPoint, requestId, AZFirstPos);

		if (traverse)
		{
			VisualisePathSystemComponent::GetInstance()->SetPath(GetEntityId(), currentPath);
		}

		return traverse;
	}

	void StarterGameNavigationComponent::OnTraversalComplete(LmbrCentral::PathfindRequest::NavigationRequestId requestId)
	{
		VisualisePathSystemComponent::GetInstance()->ClearPath(GetEntityId());
	}

	void StarterGameNavigationComponent::OnTraversalCancelled(LmbrCentral::PathfindRequest::NavigationRequestId requestId)
	{
		VisualisePathSystemComponent::GetInstance()->ClearPath(GetEntityId());
	}

	void StarterGameNavigationComponent::Reflect(AZ::ReflectContext* reflection)
	{
		AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
		if (serializeContext)
		{
			serializeContext->Class<StarterGameNavigationComponent, AZ::Component>()
				->Version(2)
			;

			AZ::EditContext* editContext = serializeContext->GetEditContext();
			if (editContext)
			{
				editContext->Class<StarterGameNavigationComponent>("Navigation (StarterGame)", "Exposes a Lua callback to see the first pathing point")
					->ClassElement(AZ::Edit::ClassElements::EditorData, "")
						->Attribute(AZ::Edit::Attributes::Category, "AI")
						->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/SG_Icon.png")
						->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/SG_Icon.png")
						->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
				;
			}
		}

		AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection);
		if (behaviorContext)
		{
			behaviorContext->EBus<StarterGameNavigationComponentNotificationBus>("StarterGameNavigationComponentNotificationBus")
				->Handler<StarterGameBehaviorNavigationComponentNotificationBusHandler>();
		}
	}

}
