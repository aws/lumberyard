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
#include "PersistentDataSystemManipulatorComponent.h"

#include "StarterGameUtility.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Math/MathUtils.h>


namespace StarterGameGem
{

	PersistentDataSystemManipulatorComponent::PersistentDataSystemManipulatorComponent()
		: m_keyName("")
		, m_dataToUse(PersistentDataSystemComponent::eBasicDataTypes::Number)
		, m_BoolData(false)
		, m_stringData("")
		, m_numberData(0)
		, m_useForData(PersistentDataSystemComponent::eDataManipulationTypes::SetOnly)
		, m_triggerMessage("")
	{
	
	}


	void PersistentDataSystemManipulatorComponent::Reflect(AZ::ReflectContext* context)
	{
		AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
		if (serializeContext)
		{
			serializeContext->Class<PersistentDataSystemManipulatorComponent, AZ::Component>()
				->Version(1)
				->Field("keyName",			&PersistentDataSystemManipulatorComponent::m_keyName)
				->Field("dataToUse",		&PersistentDataSystemManipulatorComponent::m_dataToUse)
				->Field("BoolData",			&PersistentDataSystemManipulatorComponent::m_BoolData)
				->Field("stringData",		&PersistentDataSystemManipulatorComponent::m_stringData)
				->Field("numberData",		&PersistentDataSystemManipulatorComponent::m_numberData)
				->Field("useForData",		&PersistentDataSystemManipulatorComponent::m_useForData)
				->Field("triggerMessage",	&PersistentDataSystemManipulatorComponent::m_triggerMessage)
				;

			AZ::EditContext* editContext = serializeContext->GetEditContext();
			if (editContext)
			{
				editContext->Class<PersistentDataSystemManipulatorComponent>("Persistent Data Manipulator", "Manipulates data in the Persistent Data System")
					->ClassElement(AZ::Edit::ClassElements::EditorData, "")
					->Attribute(AZ::Edit::Attributes::Category, "Game")
					->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/SG_Icon.png")
					->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/SG_Icon.dds")
					->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
					
					->DataElement(0, &PersistentDataSystemManipulatorComponent::m_triggerMessage, "Trigger Message", "The Name of gameplay message that triggers this.")

					->DataElement(0, &PersistentDataSystemManipulatorComponent::m_keyName, "Key Name", "The Name of the key that i am manipulating.")
					->DataElement(AZ::Edit::UIHandlers::ComboBox, &PersistentDataSystemManipulatorComponent::m_dataToUse,
						"Data Type", "Use this data type of information to manipulate.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PersistentDataSystemManipulatorComponent::MajorPropertyChanged)
						->EnumAttribute(PersistentDataSystemComponent::eBasicDataTypes::String, "String")
						->EnumAttribute(PersistentDataSystemComponent::eBasicDataTypes::Number, "Number")
						->EnumAttribute(PersistentDataSystemComponent::eBasicDataTypes::Bool, "Bool")
						->EnumAttribute(PersistentDataSystemComponent::eBasicDataTypes::Other, "Trigger message param")

					->DataElement(AZ::Edit::UIHandlers::ComboBox, &PersistentDataSystemManipulatorComponent::m_useForData,
						"Use", "What to do with the data.")
						->EnumAttribute(PersistentDataSystemComponent::eDataManipulationTypes::AddOnly,     "AddOnly")
						->EnumAttribute(PersistentDataSystemComponent::eDataManipulationTypes::SetOnly,     "SetOnly")
						->EnumAttribute(PersistentDataSystemComponent::eDataManipulationTypes::AddOrSet,    "AddOrSet")
						->EnumAttribute(PersistentDataSystemComponent::eDataManipulationTypes::Toggle,      "Toggle")
						->EnumAttribute(PersistentDataSystemComponent::eDataManipulationTypes::Increment,   "Increment")
						->EnumAttribute(PersistentDataSystemComponent::eDataManipulationTypes::Append,      "Append")
						->EnumAttribute(PersistentDataSystemComponent::eDataManipulationTypes::Prepend,	    "Prepend")

					->DataElement(0, &PersistentDataSystemManipulatorComponent::m_BoolData, "Data", "True // false.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &PersistentDataSystemManipulatorComponent::IsDataBool)
					->DataElement(0, &PersistentDataSystemManipulatorComponent::m_stringData, "Data", "characters.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &PersistentDataSystemManipulatorComponent::IsDataString)
					->DataElement(0, &PersistentDataSystemManipulatorComponent::m_numberData, "Data", "float values.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &PersistentDataSystemManipulatorComponent::IsDataNumber)
					;
			}
		}
	}

    AZ::u32 PersistentDataSystemManipulatorComponent::MajorPropertyChanged()
    {
        return AZ::Edit::PropertyRefreshLevels::EntireTree;
    }

    bool PersistentDataSystemManipulatorComponent::IsDataBool() const
    {
        return m_dataToUse == PersistentDataSystemComponent::eBasicDataTypes::Bool;
    }

    bool PersistentDataSystemManipulatorComponent::IsDataString() const
    {
        return m_dataToUse == PersistentDataSystemComponent::eBasicDataTypes::String;
    }

    bool PersistentDataSystemManipulatorComponent::IsDataNumber() const
    {
        return m_dataToUse == PersistentDataSystemComponent::eBasicDataTypes::Number;
    }

	void PersistentDataSystemManipulatorComponent::Activate()
	{
		AZ::EntityId myID = GetEntityId();
		m_triggerMessageID = AZ::GameplayNotificationId(myID, AZ::Crc32(m_triggerMessage.c_str()), StarterGameUtility::GetUuid("float"));
		AZ::GameplayNotificationBus::Handler::BusConnect(m_triggerMessageID);
	}

	void PersistentDataSystemManipulatorComponent::Deactivate()
	{
		AZ::GameplayNotificationBus::Handler::BusDisconnect(m_triggerMessageID);
	}

	void PersistentDataSystemManipulatorComponent::OnEventBegin(const AZStd::any& value)
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
		if(systemPtr != NULL)
			systemPtr->Manipulate(m_keyName, myData, m_useForData);
	}

	void PersistentDataSystemManipulatorComponent::OnEventUpdating(const AZStd::any& value)
	{
	}

	void PersistentDataSystemManipulatorComponent::OnEventEnd(const AZStd::any& value)
	{
	}


} // namespace LmbrCentral


