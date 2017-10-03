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
#include "PersistentDataSystemComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Math/MathUtils.h>


namespace StarterGameGem
{
	PersistentDataSystemComponent* g_instance = NULL;

	PersistentDataSystemComponent* PersistentDataSystemComponent::GetInstance()
	{ 
		return g_instance; 
	}

	void PersistentDataSystemComponent::Reflect(AZ::ReflectContext* context)
	{
		if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
		{
			serializeContext->Class<PersistentDataSystemComponent, AZ::Component>()
				->Version(1)
				->SerializerForEmptyClass()
			;

			if (AZ::EditContext* editContext = serializeContext->GetEditContext())
			{
				editContext->Class<PersistentDataSystemComponent>(
					"Persistant Data System", "Stores data for acess for the game systems")
					->ClassElement(AZ::Edit::ClassElements::EditorData, "")
						->Attribute(AZ::Edit::Attributes::Category, "Game")
						->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
					;
				// note : this currently does not Serialize its contents
				// SetDynamicEditDataProvider
				// thats the magic that will allow this
			}
		}

		if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
		{
			behaviorContext->EBus<PersistentDataSystemRequestBus>("PersistentDataSystemRequestBus")
				->Event("HasData", &PersistentDataSystemRequestBus::Events::HasData)
				->Event("SetData", &PersistentDataSystemRequestBus::Events::SetData)
				->Event("GetData", &PersistentDataSystemRequestBus::Events::GetData)
				->Event("RemoveData", &PersistentDataSystemRequestBus::Events::RemoveData)
				//->Event("RegisterDataChangeCallback", &PersistentDataSystemRequestBus::Events::RegisterDataChangeCallback)
				//->Event("UnRegisterDataChangeCallback", &PersistentDataSystemRequestBus::Events::UnRegisterDataChangeCallback)
				->Event("ClearAllData", &PersistentDataSystemRequestBus::Events::ClearAllData)
				;
		}
	}

	void PersistentDataSystemComponent::Activate()
	{
		PersistentDataSystemRequestBus::Handler::BusConnect();

		g_instance = this;
	}

	void PersistentDataSystemComponent::Deactivate()
	{
		PersistentDataSystemRequestBus::Handler::BusDisconnect();

		g_instance = NULL;
	}

	/*static*/ PersistentDataSystemComponent::eBasicDataTypes PersistentDataSystemComponent::GetBasicType(const AZStd::any& value)
	{
		float number = 0.0f;
		if (AZStd::any_numeric_cast<float>(&value, number))
			return eBasicDataTypes::Number;

		// include char* ?
		if (value.is<char*>() || value.is<AZStd::string>())
			return eBasicDataTypes::String;

		if (value.is<bool>())
			return eBasicDataTypes::Bool;

		if (value.empty())
			return eBasicDataTypes::Empty;


		return eBasicDataTypes::Other;
	}


	bool PersistentDataSystemComponent::Compare(const AZStd::string& name, const AZStd::any& value, const eComparison compareType)
	{
		AZStd::list<cDataSet>::iterator currentData;

		if (FindData(name, currentData))
		{
			eBasicDataTypes myType = GetBasicType(currentData->GetValue());
			eBasicDataTypes theirType = GetBasicType(value);
			if (myType != theirType)
				return false;

			switch (myType)
			{
			case eBasicDataTypes::Number:
				{
					float myValue;
					AZStd::any_numeric_cast<float>(&(currentData->GetValue()), myValue);
					float theirValue;
					AZStd::any_numeric_cast<float>(&value, theirValue);

					switch (compareType)
					{
					case eComparison::Equal:
						return (myValue == theirValue);
					case eComparison::GreaterThan:
						return (myValue > theirValue);
					case eComparison::GreaterThanOrEqual:
						return (myValue >= theirValue);
					case eComparison::LessThan:
						return (myValue < theirValue);
					case eComparison::LessThanOrEqual:
						return (myValue <= theirValue);
					}

					return false;
				}

			case eBasicDataTypes::Bool:
				{
					bool myValue = AZStd::any_cast<bool>(currentData->GetValue());
					bool theirValue = AZStd::any_cast<bool>(value);

					switch (compareType)
					{
					case eComparison::Equal:
						return (myValue == theirValue);
					}

					return false;
				}

			case eBasicDataTypes::String:
				{
					AZStd::string myValue = (currentData->GetValue().is<AZStd::string>() ? AZStd::any_cast<AZStd::string>(currentData->GetValue()) : AZStd::string(AZStd::any_cast<char*>(currentData->GetValue())));
					AZStd::string theirValue = (value.is<AZStd::string>() ? AZStd::any_cast<AZStd::string>(value) : AZStd::string(AZStd::any_cast<char*>(value)));

					int comparison = myValue.compare(theirValue);

					switch (compareType)
					{
					case eComparison::Equal:
						return (comparison == 0);
					case eComparison::GreaterThan:
						return (comparison > 0);
					case eComparison::GreaterThanOrEqual:
						return (comparison >= 0);
					case eComparison::LessThan:
						return (comparison < 0);
					case eComparison::LessThanOrEqual:
						return (comparison <= 0);
					}

					return false;
				}
			case eBasicDataTypes::Other:
			case eBasicDataTypes::Empty:
				return false;
			}
		}

		return false;
	}

	bool PersistentDataSystemComponent::Manipulate(const AZStd::string& name, const AZStd::any& value, const eDataManipulationTypes manipulationType)
	{
		AZStd::list<cDataSet>::iterator currentData;
		bool hasData = FindData(name, currentData);

		if (hasData && (GetBasicType(currentData->GetValue()) == eBasicDataTypes::Empty))
		{
			hasData = false;
		}

		switch (manipulationType)
		{
		case eDataManipulationTypes::AddOnly :
			if (!hasData)
			{
				SetData(name, value, true);
				return true;
			}
			return false;
		case eDataManipulationTypes::SetOnly :
			if (hasData)
			{
				SetData(name, value, false);
				return true;
			}
			return false;
		case eDataManipulationTypes::AddOrSet :
			SetData(name, value, true);
			return true;
		case eDataManipulationTypes::Toggle :
			if (hasData)
			{
				if (GetBasicType(currentData->GetValue()) == eBasicDataTypes::Bool)
				{
					currentData->SetValue( AZStd::any(!AZStd::any_cast<bool>(currentData->GetValue())) );
					return true;
				}
				return false;
			}
			else if(GetBasicType(value) == eBasicDataTypes::Bool)
			{
				SetData(name, value, true);
			}
			return false;
		case eDataManipulationTypes::Increment :
			if (GetBasicType(value) == eBasicDataTypes::Number)
			{
				float newValue;
				AZStd::any_numeric_cast<float>(&value, newValue);
				
				if (hasData)
				{
					float myValue;
					if (AZStd::any_numeric_cast<float>(&(currentData->GetValue()), myValue))
					{
						currentData->SetValue(AZStd::any(myValue + newValue));
						return true;
					}
					return false;
				}
				else// if (GetBasicType(value) == eBasicDataTypes::Number)
				{
					SetData(name, value, true);
					return true;
				}
			}
			return false;
		case eDataManipulationTypes::Append :
			if(GetBasicType(value) == eBasicDataTypes::String)
			{
				AZStd::string newValue = (value.is<AZStd::string>() ? AZStd::any_cast<AZStd::string>(value) : AZStd::string(AZStd::any_cast<char*>(value)));
				if (hasData)
				{
					if (GetBasicType(currentData->GetValue()) == eBasicDataTypes::String)
					{
						AZStd::string oldValue = (currentData->GetValue().is<AZStd::string>() ? AZStd::any_cast<AZStd::string>(currentData->GetValue()) : AZStd::string(AZStd::any_cast<char*>(currentData->GetValue())));
						
						currentData->SetValue( AZStd::any( oldValue.append(newValue) ) );
						return true;
					}
					return false;
				}
				else if (GetBasicType(value) == eBasicDataTypes::String)
				{
					SetData(name, value, true);
					return true;
				}
			}
			return false;
		case eDataManipulationTypes::Prepend :
			if (GetBasicType(value) == eBasicDataTypes::String)
			{
				AZStd::string newValue = (value.is<AZStd::string>() ? AZStd::any_cast<AZStd::string>(value) : AZStd::string(AZStd::any_cast<char*>(value)));
				if (hasData)
				{
					if (GetBasicType(currentData->GetValue()) == eBasicDataTypes::String)
					{
						AZStd::string oldValue = (currentData->GetValue().is<AZStd::string>() ? AZStd::any_cast<AZStd::string>(currentData->GetValue()) : AZStd::string(AZStd::any_cast<char*>(currentData->GetValue())));

						currentData->SetValue(AZStd::any(newValue.append(oldValue)));
						return true;
					}
					return false;
				}
				else if (GetBasicType(value) == eBasicDataTypes::String)
				{
					SetData(name, value, true);
					return true;
				}
			}
			return false;

		}

		return false;
	}


	////////////////////////////////////////////////////////////////////////
	// PersistentDataSystemRequests
	bool PersistentDataSystemComponent::HasData(const AZStd::string& name)
	{
		AZStd::list<cDataSet>::iterator currentData;
		if (FindData(name, currentData))
		{
			return (GetBasicType(currentData->GetValue()) != eBasicDataTypes::Empty);
		}
		return false;
	}

	bool PersistentDataSystemComponent::SetData(const AZStd::string& name, const AZStd::any& value, const bool addIfNotExist/* = false*/)
	{
		AZStd::list<cDataSet>::iterator currentData;

		if (FindData(name, currentData))
		{
			currentData->SetValue(value);
			return true;
		}
		else if (addIfNotExist)
		{
			AddData(name, value);
			return true;
		}

		return false;
	}

	AZStd::any PersistentDataSystemComponent::GetData(const AZStd::string& name)
	{
		AZStd::list<cDataSet>::iterator currentData;

		if (FindData(name, currentData))
		{
			return currentData->GetValue();
		}

		return AZStd::any();
	}

	void PersistentDataSystemComponent::RemoveData(const AZStd::string& name)
	{
		AZStd::list<cDataSet>::iterator currentData;

		if (FindData(name, currentData))
		{
			m_Datas.erase(currentData);
		}
	}

	// when this callback gets called, the data with it will be the new value
	void PersistentDataSystemComponent::RegisterDataChangeCallback(const AZStd::string& name, const AZ::GameplayNotificationId& messageID)
	{
		AZStd::list<cDataSet>::iterator currentData;

		if (!FindData(name, currentData))
		{
			// add it with a null data as a sentinal
			AddData(name, AZStd::any());
		}

		if (FindData(name, currentData))
		{
			currentData->RegisterCallback(messageID);
		}
	}

	void PersistentDataSystemComponent::UnRegisterDataChangeCallback(const AZStd::string& name, const AZ::GameplayNotificationId& messageID)
	{
		AZStd::list<cDataSet>::iterator currentData;

		if (FindData(name, currentData))
		{
			currentData->UnRegisterCallback(messageID);
		}
	}

	void PersistentDataSystemComponent::ClearAllData()
	{
		m_Datas.clear();
	}

	bool PersistentDataSystemComponent::FindData(const AZStd::string& name, AZStd::list<cDataSet>::iterator& currentData)
	{
		for (AZStd::list<cDataSet>::iterator iter(m_Datas.begin()); iter != m_Datas.end(); iter++)
		{
			if (iter->GetName() == name)
			{
				currentData = iter;
				return true;
			}
		}

		return false;
	}

	void PersistentDataSystemComponent::AddData(const AZStd::string& name, const AZStd::any& value)
	{
		AZStd::list<cDataSet>::iterator currentData;

		if (FindData(name, currentData))
		{
			// ?!
			currentData->SetValue(value);
		}
		else
		{
			m_Datas.push_back(cDataSet(name, value));
		}

	}	
	
	////////////////////////////////////////////////////////////////////////
	
	void PersistentDataSystemComponent::cDataSet::SetValue(const AZStd::any& value)
	{
		// i cannot check to see if the data is differnet / the same ?!
		//if (m_value == value)
		{
			m_value = value;

			for (AZStd::list<AZ::GameplayNotificationId>::iterator iter(m_callbacks.begin()); iter != m_callbacks.end(); iter++)
			{
				AZ::GameplayNotificationBus::Event(*iter, &AZ::GameplayNotificationBus::Events::OnEventBegin, m_value);
			}
		}
	}

	bool PersistentDataSystemComponent::cDataSet::AlreadyHaveCallback(const AZ::GameplayNotificationId& messageID, AZStd::list<AZ::GameplayNotificationId>::iterator& currentCallback)
	{
		for (AZStd::list<AZ::GameplayNotificationId>::iterator iter(m_callbacks.begin()); iter != m_callbacks.end(); iter++)
		{
			if (*iter == messageID)
			{
				currentCallback = iter;
				return true;
			}
		}

		return false;
	}


	void PersistentDataSystemComponent::cDataSet::RegisterCallback(const AZ::GameplayNotificationId& messageID)
	{
		AZStd::list<AZ::GameplayNotificationId>::iterator callback;

		if (!AlreadyHaveCallback(messageID, callback))
		{
			m_callbacks.push_back(messageID);
		}
	}

	void PersistentDataSystemComponent::cDataSet::UnRegisterCallback(const AZ::GameplayNotificationId& messageID)
	{
		AZStd::list<AZ::GameplayNotificationId>::iterator callback;

		if (AlreadyHaveCallback(messageID, callback))
		{
			m_callbacks.erase(callback);
		}
	}


} // namespace LmbrCentral


