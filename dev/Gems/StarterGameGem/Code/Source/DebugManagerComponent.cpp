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
#include "DebugManagerComponent.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/Entity.h>

#include <GameplayEventBus.h>

namespace StarterGameGem
{

	class DebugManagerComponentNotifications
		: public AZ::ComponentBus
	{
	public:
		//////////////////////////////////////////////////////////////////////////
		// EBusTraits overrides (Configuring this Ebus)
		static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;


		virtual ~DebugManagerComponentNotifications() {}

		/**
		* Callback indicating that the component has been activated.
		*/
		virtual void OnDebugManagerComponentActivated() {}

	};

	using DebugManagerComponentNotificationBus = AZ::EBus<DebugManagerComponentNotifications>;

	// Behavior Context forwarder for DebugManagerComponentNotificationBus
	class DebugManagerBehaviorNavigationComponentNotificationBusHandler
		: public DebugManagerComponentNotificationBus::Handler
		, public AZ::BehaviorEBusHandler
	{
	public:
		AZ_EBUS_BEHAVIOR_BINDER(DebugManagerBehaviorNavigationComponentNotificationBusHandler,"{596DC48A-DD5D-44F2-A6AF-E05076261214}", AZ::SystemAllocator,
			OnDebugManagerComponentActivated);

		void OnDebugManagerComponentActivated() override
		{
			Call(FN_OnDebugManagerComponentActivated);
		}

	};

	//-------------------------------------------
	// DebugManagerComponent
	//-------------------------------------------
	void DebugManagerComponent::Init()
	{
	}

	void DebugManagerComponent::Activate()
	{
		DebugManagerComponentRequestsBus::Handler::BusConnect(GetEntityId());

		EBUS_EVENT_ID(GetEntityId(), DebugManagerComponentNotificationBus, OnDebugManagerComponentActivated);
	}

	void DebugManagerComponent::Deactivate()
	{
		DebugManagerComponentRequestsBus::Handler::BusDisconnect();
	}

	void DebugManagerComponent::Reflect(AZ::ReflectContext* context)
	{
		AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
		if (serializeContext)
		{
			serializeContext->Class<DebugManagerComponent, AZ::Component>()
				->Version(1)
			;

			AZ::EditContext* editContext = serializeContext->GetEditContext();
			if (editContext)
			{
				editContext->Class<DebugManagerComponent>("Debug Manager", "Holds and distributes debug variables")
					->ClassElement(AZ::Edit::ClassElements::EditorData, "")
					->Attribute(AZ::Edit::Attributes::Category, "Game")
					->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/SG_Icon.png")
					->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/SG_Icon.dds")
					->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
					->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20))
					//->DataElement(0, &DebugManagerComponent::m_objects, "Objects", "Lists of objects.")
				;
			}
		}

		if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
		{
			behaviorContext->EBus<DebugManagerComponentRequestsBus>("DebugManagerComponentRequestsBus")
				->Event("GetDebugBool", &DebugManagerComponentRequestsBus::Events::GetDebugBool)
				->Event("GetDebugFloat", &DebugManagerComponentRequestsBus::Events::GetDebugFloat)
				->Event("SetDebugBool", &DebugManagerComponentRequestsBus::Events::SetDebugBool)
				->Event("SetDebugFloat", &DebugManagerComponentRequestsBus::Events::SetDebugFloat)
				;

			behaviorContext->EBus<DebugManagerComponentNotificationBus>("DebugManagerComponentNotificationBus")
				->Handler<DebugManagerBehaviorNavigationComponentNotificationBusHandler>()
			;
		}
	}

	void DebugManagerComponent::SetDebugBool(const AZStd::string& eventName, bool value)
	{
		int index = 0;
		for (index; index < m_bools.size(); ++index)
		{
			if (strcmp(m_bools[index].m_eventName.c_str(), eventName.c_str()) == 0)
			{
				break;
			}
		}

		if (index == m_bools.size())
		{
			// Add the variable.
			DebugVarBool newVar;
			newVar.m_eventName = eventName;
			newVar.m_value = value;
			m_bools.push_back(newVar);
		}
		else
		{
			// Set the variable.
			m_bools[index].m_value = value;
		}
	}

	void DebugManagerComponent::SetDebugFloat(const AZStd::string& eventName, float value)
	{
		int index = 0;
		for (index; index < m_floats.size(); ++index)
		{
			if (strcmp(m_floats[index].m_eventName.c_str(), eventName.c_str()) == 0)
			{
				break;
			}
		}

		if (index == m_floats.size())
		{
			// Add the variable.
			DebugVarFloat newVar;
			newVar.m_eventName = eventName;
			newVar.m_value = value;
			m_floats.push_back(newVar);
		}
		else
		{
			// Set the variable.
			m_floats[index].m_value = value;
		}
	}

	bool DebugManagerComponent::GetDebugBool(const AZStd::string& eventName)
	{
		int index = 0;
		for (index; index < m_bools.size(); ++index)
		{
			if (strcmp(m_bools[index].m_eventName.c_str(), eventName.c_str()) == 0)
			{
				break;
			}
		}

		bool res = false;
		if (index == m_bools.size())
		{
			// Couldn't find the variable.
			AZ_Error("DebugManager", false, "Variable '%s' doesn't exist.", eventName.c_str());
		}
		else
		{
			// Get the variable.
			res = m_bools[index].m_value;
		}

		return res;
	}

	float DebugManagerComponent::GetDebugFloat(const AZStd::string& eventName)
	{
		int index = 0;
		for (index; index < m_floats.size(); ++index)
		{
			if (strcmp(m_floats[index].m_eventName.c_str(), eventName.c_str()) == 0)
			{
				break;
			}
		}

		float res = 0.0f;
		if (index == m_floats.size())
		{
			// Couldn't find the variable.
			AZ_Error("DebugManager", false, "Variable '%s' doesn't exist.", eventName.c_str());
		}
		else
		{
			// Get the variable.
			res = m_floats[index].m_value;
		}

		return res;
	}

}
