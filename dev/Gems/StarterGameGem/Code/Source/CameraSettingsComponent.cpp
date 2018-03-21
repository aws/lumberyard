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
#include "CameraSettingsComponent.h"

#include <AzCore/RTTI/BehaviorContext.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/Entity.h>

#include <GameplayEventBus.h>

namespace StarterGameGem
{
	//-------------------------------------------
	// CameraSettings
	//-------------------------------------------
	CameraSettings::CameraSettings(bool reset /*= false*/)
		: m_name("")
		, m_priority(0)
		, m_followDistance(3.8f)
		, m_followDistanceLow(2.0f)
		, m_followHeight(1.5f)
		, m_followOffset(0.65f)
		, m_pivotOffsetFront(0.25f)
		, m_pivotOffsetBack(0.75f)
		, m_maxLookUpAngle(52.0f)
		, m_maxLookDownAngle(-89.0f)
		, m_verticalSpeed(120.0f)
		, m_horizontalSpeed(140.0f)
		, m_horizontalBoostSpeed(270.0f)
		, m_horizontalBoostAccel(180.0f)
		, m_horizontalBoostDecel(360.0f)
		, m_controllerSensitivityPower(2.1f)
		, m_mouseSensitivityScale(5.0f)
		, m_fov(45.0f)
		, m_nearClip(0.05f)
		, m_farClip(2048.0f)
	{}

	void CameraSettings::CopySettings(const CameraSettings& other)
	{
		m_name							= other.m_name;
		m_followDistance				= other.m_followDistance;
		m_followDistanceLow				= other.m_followDistanceLow;
		m_followHeight					= other.m_followHeight;
		m_followOffset					= other.m_followOffset;
		m_pivotOffsetFront				= other.m_pivotOffsetFront;
		m_pivotOffsetBack				= other.m_pivotOffsetBack;
		m_maxLookUpAngle				= other.m_maxLookUpAngle;
		m_maxLookDownAngle				= other.m_maxLookDownAngle;
		m_verticalSpeed					= other.m_verticalSpeed;
		m_horizontalSpeed				= other.m_horizontalSpeed;
		m_horizontalBoostSpeed			= other.m_horizontalBoostSpeed;
		m_horizontalBoostAccel			= other.m_horizontalBoostAccel;
		m_horizontalBoostDecel			= other.m_horizontalBoostDecel;
		m_controllerSensitivityPower	= other.m_controllerSensitivityPower;
		m_mouseSensitivityScale			= other.m_mouseSensitivityScale;
		m_fov							= other.m_fov;
		m_nearClip						= other.m_nearClip;
		m_farClip						= other.m_farClip;
	}

	void CameraSettings::BlendSettings(const CameraSettings& setA, const CameraSettings& setB, float t)
	{
		float weightB = MIN(MAX(t, 0.0f), 1.0f);
		float weightA = 1.0f - weightB;

		m_followDistance				= weightA * setA.m_followDistance   			+ weightB * setB.m_followDistance;
		m_followDistanceLow				= weightA * setA.m_followDistanceLow   			+ weightB * setB.m_followDistanceLow;
		m_followHeight					= weightA * setA.m_followHeight   				+ weightB * setB.m_followHeight;
		m_followOffset					= weightA * setA.m_followOffset   				+ weightB * setB.m_followOffset;
		m_pivotOffsetFront				= weightA * setA.m_pivotOffsetFront   			+ weightB * setB.m_pivotOffsetFront;
		m_pivotOffsetBack				= weightA * setA.m_pivotOffsetBack   			+ weightB * setB.m_pivotOffsetBack;
		m_maxLookUpAngle				= weightA * setA.m_maxLookUpAngle   			+ weightB * setB.m_maxLookUpAngle;
		m_maxLookDownAngle				= weightA * setA.m_maxLookDownAngle   			+ weightB * setB.m_maxLookDownAngle;
		m_verticalSpeed					= weightA * setA.m_verticalSpeed   				+ weightB * setB.m_verticalSpeed;
		m_horizontalSpeed				= weightA * setA.m_horizontalSpeed   			+ weightB * setB.m_horizontalSpeed;
		m_horizontalBoostSpeed			= weightA * setA.m_horizontalBoostSpeed   		+ weightB * setB.m_horizontalBoostSpeed;
		m_horizontalBoostAccel			= weightA * setA.m_horizontalBoostAccel   		+ weightB * setB.m_horizontalBoostAccel;
		m_horizontalBoostDecel			= weightA * setA.m_horizontalBoostDecel   		+ weightB * setB.m_horizontalBoostDecel;
		m_controllerSensitivityPower	= weightA * setA.m_controllerSensitivityPower   + weightB * setB.m_controllerSensitivityPower;
		m_mouseSensitivityScale			= weightA * setA.m_mouseSensitivityScale   		+ weightB * setB.m_mouseSensitivityScale;
		m_fov							= weightA * setA.m_fov   						+ weightB * setB.m_fov;
		m_nearClip						= weightA * setA.m_nearClip   					+ weightB * setB.m_nearClip;
		m_farClip						= weightA * setA.m_farClip   					+ weightB * setB.m_farClip;
	}

	void CameraSettings::Reflect(AZ::ReflectContext* context)
	{
		AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
		if (serializeContext)
		{
			serializeContext->Class<CameraSettings>()
				->Version(1)
				->Field("Name", &CameraSettings::m_name)
				->Field("Priority", &CameraSettings::m_priority)
				->Field("FollowDistance", &CameraSettings::m_followDistance)
				->Field("FollowDistanceLow", &CameraSettings::m_followDistanceLow)
				->Field("FollowHeight", &CameraSettings::m_followHeight)
				->Field("FollowOffset", &CameraSettings::m_followOffset)
				->Field("PivotOffsetFront", &CameraSettings::m_pivotOffsetFront)
				->Field("PivotOffsetBack", &CameraSettings::m_pivotOffsetBack)
				->Field("MaxLookUpAngle", &CameraSettings::m_maxLookUpAngle)
				->Field("MaxLookDownAngle", &CameraSettings::m_maxLookDownAngle)
				->Field("VerticalSpeed", &CameraSettings::m_verticalSpeed)
				->Field("HorizontalSpeed", &CameraSettings::m_horizontalSpeed)
				->Field("HorizontalBoostSpeed", &CameraSettings::m_horizontalBoostSpeed)
				->Field("HorizontalBoostAccel", &CameraSettings::m_horizontalBoostAccel)
				->Field("HorizontalBoostDecel", &CameraSettings::m_horizontalBoostDecel)
				->Field("ControllerSensitivityPower", &CameraSettings::m_controllerSensitivityPower)
				->Field("MouseSensitivityScale", &CameraSettings::m_mouseSensitivityScale)
				->Field("FOV", &CameraSettings::m_fov)
				->Field("NearClip", &CameraSettings::m_nearClip)
				->Field("FarClip", &CameraSettings::m_farClip)
			;

			AZ::EditContext* editContext = serializeContext->GetEditContext();
			if (editContext)
			{
				// CameraSettings
				editContext->Class<CameraSettings>("Camera Settings", "A set of settings for the camera")
					->ClassElement(AZ::Edit::ClassElements::EditorData, "")
					->DataElement(0, &CameraSettings::m_name, "Name", "The name of the set")
					->DataElement(0, &CameraSettings::m_priority, "Priority", "The priority of these settings on the stack (higher values override lower ones)")
					->DataElement(0, &CameraSettings::m_followDistance, "Follow Distance", "The distance (in meters) of the camera from the character")
                        ->Attribute("Suffix", " m")
                        ->Attribute("Min", 0.f)
					->DataElement(0, &CameraSettings::m_followDistanceLow, "Follow Distance Low", "")
                        ->Attribute("Suffix", " m")
                        ->Attribute("Min", 0.f)
                    ->DataElement(0, &CameraSettings::m_followHeight, "Follow Height", "")
                        ->Attribute("Suffix", " m")
					->DataElement(0, &CameraSettings::m_followOffset, "Follow Offset", "")
                        ->Attribute("Suffix", " m")
					->DataElement(0, &CameraSettings::m_pivotOffsetFront, "Pivot Offset Front", "")
                        ->Attribute("Suffix", " m")
					->DataElement(0, &CameraSettings::m_pivotOffsetBack, "Pivot Offset Back", "")
                        ->Attribute("Suffix", " m")
					->DataElement(0, &CameraSettings::m_maxLookUpAngle, "Max Look Up Angle", "")
                        ->Attribute("Suffix", " deg")
                        ->Attribute("Min", 0.f)
                        ->Attribute("Max", 89.f)
                    ->DataElement(0, &CameraSettings::m_maxLookDownAngle, "Max Look Down Angle", "")
                        ->Attribute("Suffix", " deg")
                        ->Attribute("Min", -89.f)
                        ->Attribute("Max", 0.f)
					->DataElement(0, &CameraSettings::m_verticalSpeed, "Vertical Speed", "")
                        ->Attribute("Suffix", " deg/sec")
                        ->Attribute("Min", 0.f)
                    ->DataElement(0, &CameraSettings::m_horizontalSpeed, "Horizontal Speed", "")
                        ->Attribute("Suffix", " deg/sec")
                        ->Attribute("Min", 0.f)
					->DataElement(0, &CameraSettings::m_horizontalBoostSpeed, "Horizontal Boost Speed", "")
                        ->Attribute("Suffix", " deg/sec")
                        ->Attribute("Min", 0.f)
					->DataElement(0, &CameraSettings::m_horizontalBoostAccel, "Horizontal Boost Accel", "")
                        ->Attribute("Suffix", " deg/sec/sec")
                        ->Attribute("Min", 0.f)
                    ->DataElement(0, &CameraSettings::m_horizontalBoostDecel, "Horizontal Boost Decel", "")
                        ->Attribute("Suffix", " deg/sec/sec")
                        ->Attribute("Min", 0.f)
					->DataElement(0, &CameraSettings::m_controllerSensitivityPower, "Controller Sensitivity Power", "")
                        ->Attribute("Min", 0.f)
                    ->DataElement(0, &CameraSettings::m_mouseSensitivityScale, "Mouse Sensitivity Scale", "")
                        ->Attribute("Min", 0.f)
					->DataElement(0, &CameraSettings::m_fov, "FOV", "")
                        ->Attribute("Suffix", " deg")
                        ->Attribute("Min", 0.f)
                        ->Attribute("Max", 180.f)
                    ->DataElement(0, &CameraSettings::m_nearClip, "Near Clip", "")
                        ->Attribute("Suffix", " m")
                        ->Attribute("Min", 0.001f)
                        ->Attribute("Max", 100.0f)
                    ->DataElement(0, &CameraSettings::m_farClip, "Far Clip", "")
                        ->Attribute("Suffix", " m")
                        ->Attribute("Min", 0.2f)
                    ;
			}
		}

		if (AZ::BehaviorContext* behavior = azrtti_cast<AZ::BehaviorContext*>(context))
		{
			// CameraSettings
			behavior->Class<CameraSettings>()
				->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
				->Method("CopySettings", &CameraSettings::CopySettings)
				->Method("BlendSettings", &CameraSettings::BlendSettings)
				->Property("Name", BehaviorValueProperty(&CameraSettings::m_name))
				->Property("Priority", BehaviorValueProperty(&CameraSettings::m_priority))
				->Property("FollowDistance", BehaviorValueProperty(&CameraSettings::m_followDistance))
				->Property("FollowDistanceLow", BehaviorValueProperty(&CameraSettings::m_followDistanceLow))
				->Property("FollowHeight", BehaviorValueProperty(&CameraSettings::m_followHeight))
				->Property("FollowOffset", BehaviorValueProperty(&CameraSettings::m_followOffset))
				->Property("PivotOffsetFront", BehaviorValueProperty(&CameraSettings::m_pivotOffsetFront))
				->Property("PivotOffsetBack", BehaviorValueProperty(&CameraSettings::m_pivotOffsetBack))
				->Property("MaxLookUpAngle", BehaviorValueProperty(&CameraSettings::m_maxLookUpAngle))
				->Property("MaxLookDownAngle", BehaviorValueProperty(&CameraSettings::m_maxLookDownAngle))
				->Property("VerticalSpeed", BehaviorValueProperty(&CameraSettings::m_verticalSpeed))
				->Property("HorizontalSpeed", BehaviorValueProperty(&CameraSettings::m_horizontalSpeed))
				->Property("HorizontalBoostSpeed", BehaviorValueProperty(&CameraSettings::m_horizontalBoostSpeed))
				->Property("HorizontalBoostAccel", BehaviorValueProperty(&CameraSettings::m_horizontalBoostAccel))
				->Property("HorizontalBoostDecel", BehaviorValueProperty(&CameraSettings::m_horizontalBoostDecel))
				->Property("ControllerSensitivityPower", BehaviorValueProperty(&CameraSettings::m_controllerSensitivityPower))
				->Property("MouseSensitivityScale", BehaviorValueProperty(&CameraSettings::m_mouseSensitivityScale))
				->Property("FOV", BehaviorValueProperty(&CameraSettings::m_fov))
				->Property("NearClip", BehaviorValueProperty(&CameraSettings::m_nearClip))
				->Property("FarClip", BehaviorValueProperty(&CameraSettings::m_farClip))
                ->Property("TransitionTime", BehaviorValueProperty(&CameraSettings::m_transitionTime))
            ;
		}
	}


	//-------------------------------------------
	// CameraSettingsComponent
	//-------------------------------------------
	void CameraSettingsComponent::Init()
	{
	}

	void CameraSettingsComponent::Activate()
	{
		CameraSettingsComponentRequestsBus::Handler::BusConnect(GetEntityId());

		if (m_initialSettings != "")
		{
			PushSettings(m_initialSettings, GetEntityId());
		}
	}

	void CameraSettingsComponent::Deactivate()
	{
		CameraSettingsComponentRequestsBus::Handler::BusDisconnect();
	}

	void CameraSettingsComponent::Reflect(AZ::ReflectContext* context)
	{
		CameraSettings::Reflect(context);

		AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
		if (serializeContext)
		{
			serializeContext->Class<CameraSettingsComponent>()
				->Version(1)
				->Field("InitialSettings", &CameraSettingsComponent::m_initialSettings)
				->Field("Settings", &CameraSettingsComponent::m_settings)
			;
			
			AZ::EditContext* editContext = serializeContext->GetEditContext();
			if (editContext)
			{
				editContext->Class<CameraSettingsComponent>("Camera Settings", "Create a series of settings for the camera")
					->ClassElement(AZ::Edit::ClassElements::EditorData, "")
					->Attribute(AZ::Edit::Attributes::Category, "Game")
					->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/SG_Icon.png")
					->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/SG_Icon.dds")
					->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
					->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20))
					->DataElement(0, &CameraSettingsComponent::m_initialSettings, "Initial Settings", "The settings used at startup.")
					->DataElement(0, &CameraSettingsComponent::m_settings, "Settings", "An array of settings.")
				;
			}
		}

		if (AZ::BehaviorContext* behavior = azrtti_cast<AZ::BehaviorContext*>(context))
		{
			behavior->Class<CameraSettingsEventArgs>()
				->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
				->Property("name", BehaviorValueProperty(&CameraSettingsEventArgs::m_name))
				->Property("entityId", BehaviorValueProperty(&CameraSettingsEventArgs::m_entityId))
                ->Property("transitionTime", BehaviorValueProperty(&CameraSettingsEventArgs::m_transitionTime))
            ;

			behavior->EBus<CameraSettingsComponentRequestsBus>("CameraSettingsComponentRequestsBus")
				->Event("PushSettings", &CameraSettingsComponentRequestsBus::Events::PushSettings)
				->Event("PopSettings", &CameraSettingsComponentRequestsBus::Events::PopSettings)
				->Event("GetActiveSettings", &CameraSettingsComponentRequestsBus::Events::GetActiveSettings)
			;
		}
	}

	const CameraSettings *CameraSettingsComponent::FindSettings(const AZStd::string& settingsName) const
	{
		AZStd::vector<CameraSettings>::const_iterator it = m_settings.begin();
		for (it; it != m_settings.end(); ++it)
		{
			if (it->m_name == settingsName)
			{
				break;
			}
		}

		if (it == m_settings.end())
		{
			return nullptr;
		}
		else
		{
			return it;
		}
	}


	void CameraSettingsComponent::PushSettings(const AZStd::string& settingsName, AZ::EntityId sourceEntity)
	{
		const CameraSettings *settings = FindSettings(settingsName);
		if (!settings) 
		{ 
			return; 
		}

		int priority = settings->m_priority;

		AZStd::vector<CameraSettingsStackEntry>::const_iterator it = m_settingsStack.begin();
		for (it; it != m_settingsStack.end(); ++it)
		{
			if (it->m_priority > priority)
			{
				break;
			}
		}
		
		CameraSettingsStackEntry newEntry;
		newEntry.m_entity = sourceEntity;
		newEntry.m_priority = priority;
		newEntry.m_settings = settings;

		m_settingsStack.insert(it, newEntry);
	}

	void CameraSettingsComponent::PopSettings(const AZStd::string& settingsName, AZ::EntityId sourceEntity)
	{
		AZStd::vector<CameraSettingsStackEntry>::const_iterator it = m_settingsStack.begin();
		for (it; it != m_settingsStack.end(); ++it)
		{
			if (it->m_entity == sourceEntity && it->m_settings->m_name == settingsName)
			{
				break;
			}
		}
		if (it != m_settingsStack.end())
		{
			m_settingsStack.erase(it);
		}
	}


	CameraSettings CameraSettingsComponent::GetActiveSettings()
	{
		CameraSettings active;
		if (m_settingsStack.size() > 0)
		{
			active.CopySettings(*m_settingsStack.back().m_settings);
		}
		return active;
	}


}
