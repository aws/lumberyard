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

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Math/Transform.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/any.h>
#include <GameplayEventBus.h>

namespace StarterGameGem
{

	/*!
	* StatRequests
	* Messages serviced by the StatComponent.
	*/
	class StatRequests : public AZ::ComponentBus
	{
	public:
		virtual ~StatRequests() {}
		
		virtual void SetMax(float max) = 0;
		virtual void SetMin(float min) = 0;
		virtual void SetValue(float value) = 0;
		virtual void SetRegenSpeed(float value) = 0;
		virtual void SetRegenDelay(float value) = 0;

		// making these const seems to cause problems
		virtual float GetMax(float max) /*const*/ = 0;
		virtual float GetMin(float min) /*const*/ = 0;
		virtual float GetValue(float value) /*const*/ = 0;
		virtual float GetRegenSpeed(float value) /*const*/ = 0;
		virtual float GetRegenDelay(float value) /*const*/ = 0;

		virtual void DeltaValue(float value) = 0;
		virtual void Reset() = 0;
	};
	using StatRequestBus = AZ::EBus<StatRequests>;
	

	class StatComponent
		: public AZ::Component
		, private StatRequestBus::Handler
		, public AZ::GameplayNotificationBus::MultiHandler
		, private AZ::TickBus::Handler
	{
	public:
		AZ_COMPONENT(StatComponent, "{76D127CC-5E52-4DC9-8B36-DEE62A7BAC39}");

		StatComponent();

		//////////////////////////////////////////////////////////////////////////
		// AZ::Component interface implementation
		void Init() override		{}
		void Activate() override;
		void Deactivate() override;
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		// AZ::TickBus interface implementation
		void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
		//////////////////////////////////////////////////////////////////////////

		// Required Reflect function.
		static void Reflect(AZ::ReflectContext* context);

		// Optional functions for defining provided and dependent services.
		//static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
		//{
		//	required.push_back(AZ_CRC("TransformService"));
		//}

		//////////////////////////////////////////////////////////////////////////
		// StatComponentRequestBus::Handler
		void SetMax(float max);
		void SetMin(float min);
		void SetValue(float value);
		void SetRegenSpeed(float value);
		void SetRegenDelay(float value);

		void SetFullEvent(const char* name);
		void SetEmptyEvent(const char* name);
		void SetRegenStartEvent(const char* name);
		void SetRegenEndEvent(const char* name);
		void SetValueChangedEvent(const char* name);

		float GetMax(float max) /*const*/;
		float GetMin(float min) /*const*/;
		float GetValue(float value) /*const*/;
		float GetRegenSpeed(float value) /*const*/;
		float GetRegenDelay(float value) /*const*/;

		AZStd::string GetFullEvent() const;
		AZStd::string GetEmptyEvent() const;
		AZStd::string GetRegenStartEvent() const;
		AZStd::string GetRegenEndEvent() const;
		AZStd::string GetValueChangedEvent() const;

		void DeltaValue(float value);	
		void Reset();

		void OnEventBegin(const AZStd::any& value) override;
		void OnEventUpdating(const AZStd::any&) override;
		void OnEventEnd(const AZStd::any&) override;


	protected:
		float m_max;
		float m_min;
		float m_value;
		float m_regenSpeed;
		float m_regenDelay;

		// storage for the initilised values, so i can be "reset"
		float m_defaultMax;
		float m_defaultMin;
		float m_defaultValue;
		float m_defaultRegenSpeed;
		float m_defaultRegenDelay;

		AZStd::string m_fullEventName;
		AZStd::string m_emptyEventName;
		AZStd::string m_regenStartEventName;
		AZStd::string m_regenEndEventName;
		AZStd::string m_valueChangedEventName;

		AZStd::string m_setMaxEventName;
		AZStd::string m_setMinEventName;
		AZStd::string m_setRegenSpeedEventName;
		AZStd::string m_setRegenDelayEventName;
		AZStd::string m_setValueEventName;
		AZStd::string m_deltaValueEventName;
		AZStd::string m_resetEventName;

		AZStd::string m_GetValueEventName;
		AZStd::string m_SendValueEventName;
		AZStd::string m_GetUsedCapacityEventName;
		AZStd::string m_SendUsedCapacityEventName;
		AZStd::string m_GetUnUsedCapacityEventName;
		AZStd::string m_SendUnUsedCapacityEventName;

	private:
		float m_timer;
		bool m_isRegening;
		void ValueChanged(bool resetTimer = true, bool regenStarted = false);
		
		AZ::GameplayNotificationId m_setMaxEventID;
		AZ::GameplayNotificationId m_setMinEventID;
		AZ::GameplayNotificationId m_setRegenSpeedEventID;
		AZ::GameplayNotificationId m_setRegenDelayEventID;
		AZ::GameplayNotificationId m_setValueEventID;
		AZ::GameplayNotificationId m_deltaValueEventID;
		AZ::GameplayNotificationId m_resetEventID;
		AZ::GameplayNotificationId m_GetValueEventID;
		AZ::GameplayNotificationId m_GetUsedCapacityEventID;
		AZ::GameplayNotificationId m_GetUnUsedCapacityEventID;

	};

}
