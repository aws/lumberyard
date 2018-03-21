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
#include "StatComponent.h"

#include "StarterGameUtility.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <GameplayEventBus.h>
#include <AzCore/Math/Crc.h>

namespace StarterGameGem
{
    StatComponent::StatComponent()
        : m_name("Stat")
		, m_max(100)
        , m_min(0)
        , m_value(100)
        , m_regenSpeed(10)
        , m_regenDelay(1)
        , m_defaultMax(100)
        , m_defaultMin(0)
        , m_defaultValue(100)
        , m_defaultRegenSpeed(10)
        , m_defaultRegenDelay(1)
        , m_fullEventName("Full")
        , m_emptyEventName("Empty")
        , m_regenStartEventName("RegenStarted")
        , m_regenEndEventName("RegenStopped")
        , m_valueChangedEventName("Changed")
        , m_setMaxEventName("SetMaxEvent")
        , m_setMinEventName("SetMinEvent")
        , m_setRegenSpeedEventName("SetRegenSpeedEvent")
        , m_setRegenDelayEventName("SetRegenDelayEvent")
        , m_setValueEventName("SetValueEvent")
        , m_deltaValueEventName("DeltaValueEvent")
        , m_regenResetEventName("RegenResetEvent")
        , m_resetEventName("ResetEvent")
        , m_GetValueEventName("GetValueEvent")
        , m_SendValueEventName("ValueEvent")
        , m_GetUsedCapacityEventName("GetUsedCapacity")
        , m_SendUsedCapacityEventName("UsedCapacity")
        , m_GetUnUsedCapacityEventName("GetUnUsedCapacity")
        , m_SendUnUsedCapacityEventName("UnUsedCapacity")
		, m_GetMinEventName("GetMinEvent")
		, m_SendMinEventName("SendMinEvent")
		, m_GetMaxEventName("GetMaxEvent")
		, m_SendMaxEventName("SendMaxEvent")
        , m_timer(0)
        , m_isRegening(false)
    {}

    void StatComponent::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<StatComponent, AZ::Component>()
                ->Version(1)
                ->Field("Name", &StatComponent::m_name)
                ->Field("Max", &StatComponent::m_max)
                ->Field("Min", &StatComponent::m_min)
                ->Field("Value", &StatComponent::m_value)
                ->Field("RegenSpeed", &StatComponent::m_regenSpeed)
                ->Field("RegenDelay", &StatComponent::m_regenDelay)
                ->Field("DefaultMax", &StatComponent::m_defaultMax)
                ->Field("DefaultMin", &StatComponent::m_defaultMin)
                ->Field("DefaultValue", &StatComponent::m_defaultValue)
                ->Field("DefaultRegenSpeed", &StatComponent::m_defaultRegenSpeed)
                ->Field("DefaultRegenDelay", &StatComponent::m_defaultRegenDelay)
                ->Field("FullEventName", &StatComponent::m_fullEventName)
                ->Field("EmptyEventName", &StatComponent::m_emptyEventName)
                ->Field("RegenStartEventName", &StatComponent::m_regenStartEventName)
                ->Field("RegenEndEventName", &StatComponent::m_regenEndEventName)
                ->Field("ValueChangedEventName", &StatComponent::m_valueChangedEventName)
                ->Field("Timer", &StatComponent::m_timer)
                ->Field("IsRegening", &StatComponent::m_isRegening)
                ->Field("SetMaxEvent;", &StatComponent::m_setMaxEventName)
                ->Field("setMinEvent;", &StatComponent::m_setMinEventName)
                ->Field("SetRegenSpeedEvent", &StatComponent::m_setRegenSpeedEventName)
                ->Field("SetRegenDelayEvent", &StatComponent::m_setRegenDelayEventName)
                ->Field("SetValueEvent", &StatComponent::m_setValueEventName)
                ->Field("SetDeltaValueEvent", &StatComponent::m_deltaValueEventName)
                ->Field("RegenResetEvent", &StatComponent::m_regenResetEventName)
                ->Field("ResetEvent", &StatComponent::m_resetEventName)
                ->Field("GetValueEvent", &StatComponent::m_GetValueEventName)
                ->Field("SendValueEvent", &StatComponent::m_SendValueEventName)
                ->Field("GetUsedCapacityEvent", &StatComponent::m_GetUsedCapacityEventName)
                ->Field("SendUsedCapacityEvent", &StatComponent::m_SendUsedCapacityEventName)
                ->Field("GetUnUsedCapacityEvent", &StatComponent::m_GetUnUsedCapacityEventName)
                ->Field("SendUnUsedCapacityEvent", &StatComponent::m_SendUnUsedCapacityEventName)
				->Field("GetMinEventName", &StatComponent::m_GetMinEventName) 
				->Field("SendMinEventName", &StatComponent::m_SendMinEventName)
				->Field("GetMaxEventName", &StatComponent::m_GetMaxEventName)
				->Field("SendMaxEventName", &StatComponent::m_SendMaxEventName)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<StatComponent>("Stat Component", "Maintains health of an object")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Game")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/SG_Icon.png")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/SG_Icon.dds")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                    
					->DataElement(0, &StatComponent::m_name, "Name", "The name of the stat, will be a prefix on all messages and used in messages on the bus to identify specific stats.")

                    ->DataElement(0, &StatComponent::m_defaultMax, "Max", "Max value that i can have.")
                    ->DataElement(0, &StatComponent::m_defaultMin, "Min", "Min value that i can have.")
                    ->DataElement(0, &StatComponent::m_defaultValue, "Value", "The current value.")
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Regen")
                    ->DataElement(0, &StatComponent::m_defaultRegenSpeed, "Speed", "The speed of the regen (-ve for degen / decay), units per second.")
                    ->DataElement(0, &StatComponent::m_defaultRegenDelay, "Delay", "The time from an adjustment / set untill the regen applies in seconds, -ve values implies no regen.")
                    ->ClassElement(AZ::Edit::ClassElements::Group, "MessagesOut")
                    ->DataElement(0, &StatComponent::m_fullEventName, "Full", "The message to send when i have a full stat.")
                    ->DataElement(0, &StatComponent::m_emptyEventName, "Empty", "The message to send when i have an empty stat.")
                    ->DataElement(0, &StatComponent::m_regenStartEventName, "RegenStart", "The message to send when i have started to regen stat.")
                    ->DataElement(0, &StatComponent::m_regenEndEventName, "RegenEnd", "The message to send when i have stopped regen stat.")
                    ->DataElement(0, &StatComponent::m_valueChangedEventName, "ValueChanged", "The message to send when my value has changed.")
                    ->ClassElement(AZ::Edit::ClassElements::Group, "MessagesIn")
                    ->DataElement(0, &StatComponent::m_setMaxEventName, "SetMaxEvent;", "The message to recieve so set the max.")
                    ->DataElement(0, &StatComponent::m_setMinEventName, "SetMinEvent;", "The message to recieve so set the min.")
                    ->DataElement(0, &StatComponent::m_setRegenSpeedEventName, "SetRegenSpeedEvent", "The message to recieve so set the regen speed.")
                    ->DataElement(0, &StatComponent::m_setRegenDelayEventName, "SetRegenDelayEvent", "The message to recieve so set the regen delay.")
                    ->DataElement(0, &StatComponent::m_setValueEventName, "SetValueEvent", "The message to recieve so set the value.")
                    ->DataElement(0, &StatComponent::m_deltaValueEventName, "SetDeltaValueEvent", "The message recieve so change the value by the given amount.")
					->DataElement(0, &StatComponent::m_regenResetEventName, "RegenResetEvent", "The message recieve to put the regen timer back to its max.")
					->DataElement(0, &StatComponent::m_resetEventName, "ResetEvent", "The message recieve to reset my params back to factory state.")
                    ->ClassElement(AZ::Edit::ClassElements::Group, "MessageRequests")
                    ->DataElement(0, &StatComponent::m_GetValueEventName, "GetValueEvent", "The message to request my value (parameter is the ID to send the message to).")
                    ->DataElement(0, &StatComponent::m_SendValueEventName, "SendValueEvent", "The message to send when i have been requested for my value (to the ID that the request was made with).")
                    ->DataElement(0, &StatComponent::m_GetUsedCapacityEventName, "GetUsedCapacityEvent", "The message to request my used capacity (parameter is the ID to send the message to).")
                    ->DataElement(0, &StatComponent::m_SendUsedCapacityEventName, "SendUsedCapacityEvent", "The message to send when i have been requested for my used capacity (to the ID that the request was made with).")
                    ->DataElement(0, &StatComponent::m_GetUnUsedCapacityEventName, "GetUnUsedCapacityEvent", "The message to request my unused capacity (parameter is the ID to send the message to).")
                    ->DataElement(0, &StatComponent::m_SendUnUsedCapacityEventName, "SendUnUsedCapacityEvent", "The message to send when i have been requested for my unused capacity (to the ID that the request was made with).")
					->DataElement(0, &StatComponent::m_GetMinEventName, "GetMinEvent", "The message to request my max value (parameter is the ID to send the message to).")
					->DataElement(0, &StatComponent::m_SendMinEventName, "SendMinEvent", "The message to send when i have been requested for my max value (to the ID that the request was made with).")
					->DataElement(0, &StatComponent::m_GetMaxEventName, "GetMaxEvent", "The message to request my max value (parameter is the ID to send the message to).")
					->DataElement(0, &StatComponent::m_SendMaxEventName, "SendMaxEvent", "The message to send when i have been requested for my max value (to the ID that the request was made with).")
					;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection))
        {
            behaviorContext->EBus<StatRequestBus>("StatRequestBus")
                ->Event("SetMax", &StatRequestBus::Events::SetMax)
                ->Event("SetMin", &StatRequestBus::Events::SetMin)
                ->Event("SetValue", &StatRequestBus::Events::SetValue)
                ->Event("SetRegenSpeed", &StatRequestBus::Events::SetRegenSpeed)
                ->Event("SetRegenDelay", &StatRequestBus::Events::SetRegenDelay)
                ->Event("GetMax", &StatRequestBus::Events::GetMax)
                ->Event("GetMin", &StatRequestBus::Events::GetMin)
                ->Event("GetValue", &StatRequestBus::Events::GetValue)
                ->Event("GetRegenSpeed", &StatRequestBus::Events::GetRegenSpeed)
                ->Event("GetRegenDelay", &StatRequestBus::Events::GetRegenDelay)
                ->Event("DeltaValue", &StatRequestBus::Events::DeltaValue)
                ->Event("Reset", &StatRequestBus::Events::Reset)
                ;
        }
    }

    void StatComponent::Activate()
    {
        AZ::EntityId myID = GetEntityId();

        m_max = m_defaultMax;
        m_min = m_defaultMin;
        m_value = m_defaultValue;
        m_regenSpeed = m_defaultRegenSpeed;
        m_regenDelay = m_defaultRegenDelay;

        StatRequestBus::Handler::BusConnect(myID);
        AZ::TickBus::Handler::BusConnect();

        // set IDs
        m_setMaxEventID = AZ::GameplayNotificationId(myID, AZ::Crc32((m_name + m_setMaxEventName).c_str()), StarterGameUtility::GetUuid("float"));
        m_setMinEventID = AZ::GameplayNotificationId(myID, AZ::Crc32((m_name + m_setMinEventName).c_str()), StarterGameUtility::GetUuid("float"));
        m_setRegenSpeedEventID = AZ::GameplayNotificationId(myID, AZ::Crc32((m_name + m_setRegenSpeedEventName).c_str()), StarterGameUtility::GetUuid("float"));
        m_setRegenDelayEventID = AZ::GameplayNotificationId(myID, AZ::Crc32((m_name + m_setRegenDelayEventName).c_str()), StarterGameUtility::GetUuid("float"));
        m_setValueEventID = AZ::GameplayNotificationId(myID, AZ::Crc32((m_name + m_setValueEventName).c_str()), StarterGameUtility::GetUuid("float"));
        m_deltaValueEventID = AZ::GameplayNotificationId(myID, AZ::Crc32((m_name + m_deltaValueEventName).c_str()), StarterGameUtility::GetUuid("float"));
        m_regenResetEventID = AZ::GameplayNotificationId(myID, AZ::Crc32((m_name + m_regenResetEventName).c_str()), StarterGameUtility::GetUuid("float"));
        m_resetEventID = AZ::GameplayNotificationId(myID, AZ::Crc32((m_name + m_resetEventName).c_str()), StarterGameUtility::GetUuid("float"));
        m_GetValueEventID = AZ::GameplayNotificationId(myID, AZ::Crc32((m_name + m_GetValueEventName).c_str()), StarterGameUtility::GetUuid("float"));
        m_GetUsedCapacityEventID = AZ::GameplayNotificationId(myID, AZ::Crc32((m_name + m_GetUsedCapacityEventName).c_str()), StarterGameUtility::GetUuid("float"));
        m_GetUnUsedCapacityEventID = AZ::GameplayNotificationId(myID, AZ::Crc32((m_name + m_GetUnUsedCapacityEventName).c_str()), StarterGameUtility::GetUuid("float"));
		m_GetMinEventID = AZ::GameplayNotificationId(myID, AZ::Crc32((m_name + m_GetMinEventName).c_str()), StarterGameUtility::GetUuid("float"));
		m_GetMaxEventID = AZ::GameplayNotificationId(myID, AZ::Crc32((m_name + m_GetMaxEventName).c_str()), StarterGameUtility::GetUuid("float"));

        //AZ_TracePrintf("StarterGame", "StatComponent::Activate.  [%llu] \"%s\" combinedID == %s", (AZ::u64)myID, m_deltaValueEventName.c_str(), m_deltaValueEventID.ToString().c_str());

        // attach
        AZ::GameplayNotificationBus::MultiHandler::BusConnect(m_setMaxEventID);
        AZ::GameplayNotificationBus::MultiHandler::BusConnect(m_setMinEventID);
        AZ::GameplayNotificationBus::MultiHandler::BusConnect(m_setRegenSpeedEventID);
        AZ::GameplayNotificationBus::MultiHandler::BusConnect(m_setRegenDelayEventID);
        AZ::GameplayNotificationBus::MultiHandler::BusConnect(m_setValueEventID);
        AZ::GameplayNotificationBus::MultiHandler::BusConnect(m_deltaValueEventID);
		AZ::GameplayNotificationBus::MultiHandler::BusConnect(m_regenResetEventID);
        AZ::GameplayNotificationBus::MultiHandler::BusConnect(m_resetEventID);
        AZ::GameplayNotificationBus::MultiHandler::BusConnect(m_GetValueEventID);
        AZ::GameplayNotificationBus::MultiHandler::BusConnect(m_GetUsedCapacityEventID);
        AZ::GameplayNotificationBus::MultiHandler::BusConnect(m_GetUnUsedCapacityEventID);
        AZ::GameplayNotificationBus::MultiHandler::BusConnect(m_GetMinEventID);
		AZ::GameplayNotificationBus::MultiHandler::BusConnect(m_GetMaxEventID);

        ValueChanged();
    }

    void StatComponent::Deactivate()
    {
        //AZ::TickBus::Handler::BusDisconnect();
        StatRequestBus::Handler::BusDisconnect();

        // disconnect
        AZ::GameplayNotificationBus::MultiHandler::BusDisconnect(m_setMaxEventID);
        AZ::GameplayNotificationBus::MultiHandler::BusDisconnect(m_setMinEventID);
        AZ::GameplayNotificationBus::MultiHandler::BusDisconnect(m_setRegenSpeedEventID);
        AZ::GameplayNotificationBus::MultiHandler::BusDisconnect(m_setRegenDelayEventID);
        AZ::GameplayNotificationBus::MultiHandler::BusDisconnect(m_setValueEventID);
        AZ::GameplayNotificationBus::MultiHandler::BusDisconnect(m_deltaValueEventID);
        AZ::GameplayNotificationBus::MultiHandler::BusDisconnect(m_regenResetEventID);
        AZ::GameplayNotificationBus::MultiHandler::BusDisconnect(m_resetEventID);
        AZ::GameplayNotificationBus::MultiHandler::BusDisconnect(m_GetValueEventID);
        AZ::GameplayNotificationBus::MultiHandler::BusDisconnect(m_GetUsedCapacityEventID);
        AZ::GameplayNotificationBus::MultiHandler::BusDisconnect(m_GetUnUsedCapacityEventID);
        AZ::GameplayNotificationBus::MultiHandler::BusDisconnect(m_GetMinEventID);
		AZ::GameplayNotificationBus::MultiHandler::BusDisconnect(m_GetMaxEventID);
    }

    void StatComponent::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
        if (m_regenDelay >= 0)
        {
            m_timer -= deltaTime;

            if (m_timer <= 0.0f)
            {
                m_timer = 0;

                float newValue = m_value + (m_regenSpeed * deltaTime);

                if (newValue < m_min)
                    newValue = m_min;
                if (newValue > m_max)
                    newValue = m_max;

                if (newValue != m_value)
                {
                    m_value = newValue;
                    ValueChanged(false, true);
                }
            }
        }
    }



    float StatComponent::GetMax() const
    {
        return m_max;
    }

    float StatComponent::GetMin() const
    {
        return m_min;
    }

    float StatComponent::GetValue() const
    {
        return m_value;
    }

    float StatComponent::GetRegenSpeed() const
    {
        return m_regenSpeed;
    }

    float StatComponent::GetRegenDelay() const
    {
        return m_regenDelay;
    }


    AZStd::string StatComponent::GetFullEvent() const
    {
        return m_fullEventName;
    }

    AZStd::string StatComponent::GetEmptyEvent() const
    {
        return m_emptyEventName;
    }

    AZStd::string StatComponent::GetRegenStartEvent() const
    {
        return m_regenStartEventName;
    }

    AZStd::string StatComponent::GetRegenEndEvent() const
    {
        return m_regenEndEventName;
    }

    AZStd::string StatComponent::GetValueChangedEvent() const
    {
        return m_valueChangedEventName;
    }

    void StatComponent::SetRegenSpeed(float value)
    {
        m_regenSpeed = value;
    }

    void StatComponent::SetRegenDelay(float value)
    {
        m_regenDelay = value;
    }

    void StatComponent::SetFullEvent(const char* name)
    {
        m_fullEventName = name;
    }

    void StatComponent::SetEmptyEvent(const char* name)
    {
        m_emptyEventName = name;
    }

    void StatComponent::SetRegenStartEvent(const char* name)
    {
        m_regenStartEventName = name;
    }

    void StatComponent::SetRegenEndEvent(const char* name)
    {
        m_regenEndEventName = name;
    }

    void StatComponent::SetValueChangedEvent(const char* name)
    {
        m_valueChangedEventName = name;
    }

    void StatComponent::SetMax(float max)
    {
        m_max = max;

        if (m_max < m_min)
        {
            m_min = m_max;
        }

        if (m_value > m_max)
        {
            m_value = m_max;
            ValueChanged();
        }
    }

    void StatComponent::SetMin(float min)
    {
        m_min = min;

        if (m_min > m_max)
        {
            m_max = m_min;
        }

        if (m_value < m_min)
        {
            m_value = m_min;
            ValueChanged();
        }
    }

    void StatComponent::SetValue(float value)
    {
        if (value < m_min)
            value = m_min;
        if (value > m_max)
            value = m_max;

        if (m_value != value)
        {
            m_value = value;
            ValueChanged();
        }
    }

    void StatComponent::DeltaValue(float value)
    {
        if (value == 0)
            return;

        float valueToSet = m_value + value;
        if (valueToSet < m_min)
            valueToSet = m_min;
        if (valueToSet > m_max)
            valueToSet = m_max;

        if (valueToSet != m_value)
        {
            m_value = valueToSet;
            ValueChanged();
        }
    }

    void StatComponent::Reset()
    {
        m_max = m_defaultMax;
        m_min = m_defaultMin;
        m_value = m_defaultValue;
        m_regenSpeed = m_defaultRegenSpeed;
        m_regenDelay = m_defaultRegenDelay;

        // force this on reset
        ValueChanged();
    }

    void StatComponent::ValueChanged(bool resetTimer/* = true*/, bool regenBegin /*= false*/)
    {
        bool regenRequired = ((m_regenSpeed < 0) ? (m_value != m_min) : (m_value != m_max));
        //bool regenRequired = ((m_regenSpeed < 0) ? (m_value == m_min) : (m_value == m_max));

        bool regenStopped = m_isRegening && !regenRequired;
        bool regenStarted = !resetTimer && regenBegin && !m_isRegening && regenRequired;

        if (resetTimer)
        {
            regenStopped = true;
            m_isRegening = false;
            m_timer = m_regenDelay;
        }

        AZ::EntityId myID = GetEntityId();
        //AZ_TracePrintf("StarterGame", "StatComponent::ValueChanged to \"%.03f\" on %llu", m_value, (AZ::u64)myID);

        AZStd::any paramToBus(m_value);

        // send any messages that are relevent for the current state of my value
        {	// changed
			AZStd::string messageText = (m_name + m_valueChangedEventName);
            AZ::GameplayNotificationId gameplayId = AZ::GameplayNotificationId(myID, messageText.c_str(), StarterGameUtility::GetUuid("float"));
            AZ::GameplayNotificationBus::Event(gameplayId, &AZ::GameplayNotificationBus::Events::OnEventBegin, paramToBus);
        }

        if (regenStarted)
        {	// regen begin
			m_isRegening = true;
            AZ::GameplayNotificationId gameplayId = AZ::GameplayNotificationId(myID, (m_name + m_regenStartEventName).c_str(), StarterGameUtility::GetUuid("float"));
            AZ::GameplayNotificationBus::Event(gameplayId, &AZ::GameplayNotificationBus::Events::OnEventBegin, paramToBus);
        }

        if (regenStopped)
        {	// changed
			m_isRegening = false;
            AZ::GameplayNotificationId gameplayId = AZ::GameplayNotificationId(myID, (m_name + m_regenEndEventName).c_str(), StarterGameUtility::GetUuid("float"));
            AZ::GameplayNotificationBus::Event(gameplayId, &AZ::GameplayNotificationBus::Events::OnEventBegin, paramToBus);
        }

        if (m_value == m_min)
        {
            AZ::GameplayNotificationId gameplayId = AZ::GameplayNotificationId(myID, (m_name + m_emptyEventName).c_str(), StarterGameUtility::GetUuid("float"));
            AZ::GameplayNotificationBus::Event(gameplayId, &AZ::GameplayNotificationBus::Events::OnEventBegin, paramToBus);
        }

        if (m_value == m_max)
        {
            AZ::GameplayNotificationId gameplayId = AZ::GameplayNotificationId(myID, (m_name + m_fullEventName).c_str(), StarterGameUtility::GetUuid("float"));
            AZ::GameplayNotificationBus::Event(gameplayId, &AZ::GameplayNotificationBus::Events::OnEventBegin, paramToBus);
        }
    }



	bool StatComponent::SetMax(const AZStd::string& statName, const float max)
	{
		if (statName == m_name)
		{
			SetMax(max);
			return true;
		}
		return false;
	}

	bool StatComponent::SetMin(const AZStd::string& statName, const float min)
	{
		if (statName == m_name)
		{
			SetMin(min);
			return true;
		}
		return false;
	}

	bool StatComponent::SetValue(const AZStd::string& statName, const float value)
	{
		if (statName == m_name)
		{
			SetValue(value);
			return true;
		}
		return false;
	}

	bool StatComponent::SetRegenSpeed(const AZStd::string& statName, const float value)
	{
		if (statName == m_name)
		{
			SetRegenSpeed(value);
			return true;
		}
		return false;
	}

	bool StatComponent::SetRegenDelay(const AZStd::string& statName, const float value)
	{
		if (statName == m_name)
		{
			SetRegenDelay(value);
			return true;
		}
		return false;
	}

	bool StatComponent::GetMax(const AZStd::string& statName, float& max) /*const*/
	{
		if (statName == m_name)
		{
			max = GetMax();
			return true;
		}
		return false;
	}

	bool StatComponent::GetMin(const AZStd::string& statName, float& min) /*const*/
	{
		if (statName == m_name)
		{
			min = GetMin();
			return true;
		}
		return false;
	}

	bool StatComponent::GetValue(const AZStd::string& statName, float& value) /*const*/
	{
		if (statName == m_name)
		{
			value = GetValue();
			return true;
		}
		return false;
	}

	bool StatComponent::GetRegenSpeed(const AZStd::string& statName, float& value) /*const*/
	{
		if (statName == m_name)
		{
			value = GetRegenSpeed();
			return true;
		}
		return false;
	}

	bool StatComponent::GetRegenDelay(const AZStd::string& statName, float& value) /*const*/
	{
		if (statName == m_name)
		{
			value = GetRegenDelay();
			return true;
		}
		return false;
	}

	bool StatComponent::DeltaValue(const AZStd::string& statName, const float value)
	{
		if (statName == m_name)
		{
			DeltaValue(value);
			return true;
		}
		return false;
	}

	bool StatComponent::Reset(const AZStd::string& statName)
	{
		if (statName == m_name)
		{
			Reset();
			return true;
		}
		return false;
	}


	void StatComponent::OnEventBegin(const AZStd::any& value)
	{
		float valueAsFloat = 0;
		bool isNumber = AZStd::any_numeric_cast<float>(&value, valueAsFloat);
		AZ::EntityId valueAsEntityID;
		if (value.is<AZ::EntityId>()) // if you are not this type the below function crashes
			valueAsEntityID = AZStd::any_cast<AZ::EntityId>(value);

		AZ::EntityId myID = GetEntityId();
		//AZ_TracePrintf("StarterGame", "StatComponent::OnEventBegin. [%llu], is a number %d : %.03f", (AZ::u64)myID, isNumber, valueAsFloat);

		if (*AZ::GameplayNotificationBus::GetCurrentBusId() == m_setMaxEventID)
		{
			assert(isNumber);
			SetMax(valueAsFloat);
		}
		else if (*AZ::GameplayNotificationBus::GetCurrentBusId() == m_setMinEventID)
		{
			assert(isNumber);
			SetMin(valueAsFloat);
		}
		else if (*AZ::GameplayNotificationBus::GetCurrentBusId() == m_setRegenSpeedEventID)
		{
			assert(isNumber);
			SetRegenSpeed(valueAsFloat);
		}
		else if (*AZ::GameplayNotificationBus::GetCurrentBusId() == m_setRegenDelayEventID)
		{
			assert(isNumber);
			SetRegenDelay(valueAsFloat);
		}
		else if (*AZ::GameplayNotificationBus::GetCurrentBusId() == m_setValueEventID)
		{
			SetValue(valueAsFloat);
		}
		else if (*AZ::GameplayNotificationBus::GetCurrentBusId() == m_deltaValueEventID)
		{
			assert(isNumber);
			DeltaValue(valueAsFloat);
		}
		else if (*AZ::GameplayNotificationBus::GetCurrentBusId() == m_regenResetEventID)
		{
			ValueChanged(true);
		}
		else if (*AZ::GameplayNotificationBus::GetCurrentBusId() == m_resetEventID)
		{
			Reset();
		}
		else if (*AZ::GameplayNotificationBus::GetCurrentBusId() == m_GetValueEventID)
		{
			AZStd::any paramToBus(m_value - m_min);
			AZ::GameplayNotificationId gameplayId = AZ::GameplayNotificationId(valueAsEntityID, (m_name + m_SendValueEventName).c_str(), StarterGameUtility::GetUuid("float"));
			AZ::GameplayNotificationBus::Event(gameplayId, &AZ::GameplayNotificationBus::Events::OnEventBegin, paramToBus);
		}
		else if (*AZ::GameplayNotificationBus::GetCurrentBusId() == m_GetUsedCapacityEventID)
		{
			AZStd::any paramToBus(m_value - m_min);
			AZ::GameplayNotificationId gameplayId = AZ::GameplayNotificationId(valueAsEntityID, (m_name + m_SendUsedCapacityEventName).c_str(), StarterGameUtility::GetUuid("float"));
			AZ::GameplayNotificationBus::Event(gameplayId, &AZ::GameplayNotificationBus::Events::OnEventBegin, paramToBus);
		}
		else if (*AZ::GameplayNotificationBus::GetCurrentBusId() == m_GetUnUsedCapacityEventID)
		{
			AZStd::any paramToBus(m_max - m_value);
			AZ::GameplayNotificationId gameplayId = AZ::GameplayNotificationId(valueAsEntityID, (m_name + m_SendUnUsedCapacityEventName).c_str(), StarterGameUtility::GetUuid("float"));
			AZ::GameplayNotificationBus::Event(gameplayId, &AZ::GameplayNotificationBus::Events::OnEventBegin, paramToBus);
		}
		else if (*AZ::GameplayNotificationBus::GetCurrentBusId() == m_GetMinEventID)
		{
			AZStd::any paramToBus(m_min);
			AZ::GameplayNotificationId gameplayId = AZ::GameplayNotificationId(valueAsEntityID, (m_name + m_SendMinEventName).c_str(), StarterGameUtility::GetUuid("float"));
			AZ::GameplayNotificationBus::Event(gameplayId, &AZ::GameplayNotificationBus::Events::OnEventBegin, paramToBus);
		}
		else if (*AZ::GameplayNotificationBus::GetCurrentBusId() == m_GetMaxEventID)
		{
			AZStd::any paramToBus(m_max);
			AZ::GameplayNotificationId gameplayId = AZ::GameplayNotificationId(valueAsEntityID, (m_name + m_SendMaxEventName).c_str(), StarterGameUtility::GetUuid("float"));
			AZ::GameplayNotificationBus::Event(gameplayId, &AZ::GameplayNotificationBus::Events::OnEventBegin, paramToBus);
		}
	}

    void StatComponent::OnEventUpdating(const AZStd::any& value)
    {
    }

    void StatComponent::OnEventEnd(const AZStd::any& value)
    {
    }


}

