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
#include "PersistentDataSystemComparitorComponent.h"

#include "StarterGameUtility.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Component/Entity.h>


namespace StarterGameGem
{

	PersistentDataSystemComparitorComponent::PersistentDataSystemComparitorComponent()
		: m_keyName("")
		, m_sendMessagesOnChange(false)
		, m_testMessage("")
		, m_dataToUse(PersistentDataSystemComponent::eBasicDataTypes::Number)
		, m_BoolData(false)
		, m_stringData("")
		, m_numberData(0)
		, m_comparisonForData(PersistentDataSystemComponent::eComparison::Equal)
		, m_successMessage("")
		, m_failMessage("")
		, m_targetOfMessage()
		, m_callbackMessageID()
	{
	
	}


	void PersistentDataSystemComparitorComponent::Reflect(AZ::ReflectContext* context)
	{
		AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
		if (serializeContext)
		{
			serializeContext->Class<PersistentDataSystemComparitorComponent, AZ::Component>()
				->Version(1)
				->Field("keyName",			&PersistentDataSystemComparitorComponent::m_keyName)
				->Field("MessagesOnChange",	&PersistentDataSystemComparitorComponent::m_sendMessagesOnChange)
				->Field("testMessage",		&PersistentDataSystemComparitorComponent::m_testMessage)
				->Field("dataToUse",		&PersistentDataSystemComparitorComponent::m_dataToUse)
				->Field("BoolData",			&PersistentDataSystemComparitorComponent::m_BoolData)
				->Field("stringData",		&PersistentDataSystemComparitorComponent::m_stringData)
				->Field("numberData",		&PersistentDataSystemComparitorComponent::m_numberData)
				->Field("comparisonForData", &PersistentDataSystemComparitorComponent::m_comparisonForData)
				->Field("successMessage",	&PersistentDataSystemComparitorComponent::m_successMessage)
				->Field("failMessage",		&PersistentDataSystemComparitorComponent::m_failMessage)
				->Field("targetOfMessage",	&PersistentDataSystemComparitorComponent::m_targetOfMessage)
				;

			AZ::EditContext* editContext = serializeContext->GetEditContext();
			if (editContext)
			{
				editContext->Class<PersistentDataSystemComparitorComponent>("Persistent Data Comparitor", "Compares data in the Persistent Data System and fires events when conditions happen")
					->ClassElement(AZ::Edit::ClassElements::EditorData, "")
					->Attribute(AZ::Edit::Attributes::Category, "Game")
					->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/SG_Icon.png")
					->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/SG_Icon.dds")
					->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
					
					->DataElement(0, &PersistentDataSystemComparitorComponent::m_keyName, "Key Name", "The Name of the key that i am manipulating.")
					->DataElement(0, &PersistentDataSystemComparitorComponent::m_sendMessagesOnChange, "Send On Change", "If this is set it will listend to the key for changes anf fire off the events whenever it does.")
					->DataElement(0, &PersistentDataSystemComparitorComponent::m_testMessage, "Test Message", "The message to fire that this component to perform the test.")
					->DataElement(0, &PersistentDataSystemComparitorComponent::m_successMessage, "Success Message", "The message to send at the target when the comparisons are made.")
					->DataElement(0, &PersistentDataSystemComparitorComponent::m_failMessage, "Fail Message", "The message to send at the target when the comparisons are made.")
					->DataElement(0, &PersistentDataSystemComparitorComponent::m_targetOfMessage, "Message Target", "The message to send at the target when the comparisons are made.")

					->DataElement(AZ::Edit::UIHandlers::ComboBox, &PersistentDataSystemComparitorComponent::m_dataToUse,
						"Data Type", "Use this data type of information to manipulate.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PersistentDataSystemComparitorComponent::MajorPropertyChanged)
						->EnumAttribute(PersistentDataSystemComponent::eBasicDataTypes::String, "String")
						->EnumAttribute(PersistentDataSystemComponent::eBasicDataTypes::Number, "Number")
						->EnumAttribute(PersistentDataSystemComponent::eBasicDataTypes::Bool, "Bool")

					->DataElement(AZ::Edit::UIHandlers::ComboBox, &PersistentDataSystemComparitorComponent::m_comparisonForData,
						"Use", "What to do with the data.")
						->EnumAttribute(PersistentDataSystemComponent::eComparison::Equal, "Equal")
						->EnumAttribute(PersistentDataSystemComponent::eComparison::GreaterThan, "GreaterThan")
						->EnumAttribute(PersistentDataSystemComponent::eComparison::GreaterThanOrEqual, "GreaterThanOrEqual")
						->EnumAttribute(PersistentDataSystemComponent::eComparison::LessThan, "LessThan")
						->EnumAttribute(PersistentDataSystemComponent::eComparison::LessThanOrEqual, "LessThanOrEqual")

					->DataElement(0, &PersistentDataSystemComparitorComponent::m_BoolData, "Data", "True // false.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &PersistentDataSystemComparitorComponent::IsDataBool)
					->DataElement(0, &PersistentDataSystemComparitorComponent::m_stringData, "Data", "characters.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &PersistentDataSystemComparitorComponent::IsDataString)
					->DataElement(0, &PersistentDataSystemComparitorComponent::m_numberData, "Data", "float values.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &PersistentDataSystemComparitorComponent::IsDataNumber)
					;
			}
		}
	}

    AZ::u32 PersistentDataSystemComparitorComponent::MajorPropertyChanged()
    {
        return AZ::Edit::PropertyRefreshLevels::EntireTree;
    }

    bool PersistentDataSystemComparitorComponent::IsDataBool() const
    {
        return m_dataToUse == PersistentDataSystemComponent::eBasicDataTypes::Bool;
    }

    bool PersistentDataSystemComparitorComponent::IsDataString() const
    {
        return m_dataToUse == PersistentDataSystemComponent::eBasicDataTypes::String;
    }

    bool PersistentDataSystemComparitorComponent::IsDataNumber() const
    {
        return m_dataToUse == PersistentDataSystemComponent::eBasicDataTypes::Number;
    }

	void PersistentDataSystemComparitorComponent::Activate()
	{
		AZ::EntityId myID = GetEntityId();
		m_testMessageID = AZ::GameplayNotificationId(myID, AZ::Crc32(m_testMessage.c_str()), StarterGameUtility::GetUuid("float"));
		AZ::GameplayNotificationBus::MultiHandler::BusConnect(m_testMessageID);
		if (m_sendMessagesOnChange)
		{
			m_callbackMessageID = AZ::GameplayNotificationId(myID, AZ::Crc32((m_keyName + " " + m_entity->GetName()).c_str()), StarterGameUtility::GetUuid("float"));

			PersistentDataSystemComponent* systemPtr = PersistentDataSystemComponent::GetInstance();
			if (systemPtr != NULL)
			{
				systemPtr->RegisterDataChangeCallback(m_keyName, m_callbackMessageID.m_channel, m_callbackMessageID.m_actionNameCrc);
			}
			else
			{
				assert(!"no system");
			}

			AZ::GameplayNotificationBus::MultiHandler::BusConnect(m_callbackMessageID);
		}
	}

	void PersistentDataSystemComparitorComponent::Deactivate()
	{
		AZ::GameplayNotificationBus::MultiHandler::BusDisconnect(m_testMessageID);
		if (m_sendMessagesOnChange)
		{
			PersistentDataSystemComponent* systemPtr = PersistentDataSystemComponent::GetInstance();
			if (systemPtr != NULL)
			{
				systemPtr->UnRegisterDataChangeCallback(m_keyName, m_callbackMessageID.m_channel, m_callbackMessageID.m_actionNameCrc);
			}
			AZ::GameplayNotificationBus::MultiHandler::BusDisconnect(m_callbackMessageID);
		}
	}

	void PersistentDataSystemComparitorComponent::OnEventBegin(const AZStd::any& value)
	{
		// apply the change
		AZStd::any myData;
		
		switch (m_dataToUse)
		{
		case PersistentDataSystemComponent::eBasicDataTypes::Number:
			myData = AZStd::any(m_numberData);
			break;
		case PersistentDataSystemComponent::eBasicDataTypes::String:
			myData = AZStd::any(m_stringData);
			break;
		case PersistentDataSystemComponent::eBasicDataTypes::Bool:
			myData = AZStd::any(m_BoolData);
			break;
		case PersistentDataSystemComponent::eBasicDataTypes::Other:
			myData = value;
			break;
		default:
			// !
			return;
		}

		PersistentDataSystemComponent* systemPtr = PersistentDataSystemComponent::GetInstance();
		bool result = false;
		if (systemPtr != NULL)
		{
			result = systemPtr->Compare(m_keyName, myData, m_comparisonForData);
		}

		if (result)
		{
			if (m_successMessage != "")
			{
				AZ::GameplayNotificationId messageID = AZ::GameplayNotificationId(m_targetOfMessage, AZ::Crc32(m_successMessage.c_str()), StarterGameUtility::GetUuid("float"));
				AZ::GameplayNotificationBus::Event(messageID, &AZ::GameplayNotificationBus::Events::OnEventBegin, value);
			}
		}
		else
		{
			if (m_failMessage != "")
			{
				AZ::GameplayNotificationId messageID = AZ::GameplayNotificationId(m_targetOfMessage, AZ::Crc32(m_failMessage.c_str()), StarterGameUtility::GetUuid("float"));
				AZ::GameplayNotificationBus::Event(messageID, &AZ::GameplayNotificationBus::Events::OnEventBegin, value);
			}
		}
	}

	void PersistentDataSystemComparitorComponent::OnEventUpdating(const AZStd::any& value)
	{
	}

	void PersistentDataSystemComparitorComponent::OnEventEnd(const AZStd::any& value)
	{
	}

} // namespace LmbrCentral


