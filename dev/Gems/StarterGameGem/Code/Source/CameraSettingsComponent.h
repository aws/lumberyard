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
#include <AzCore/Component/EntityId.h>
#include <AzCore/Memory/SystemAllocator.h>

//#include <AzCore/Math/Transform.h>
//#include <AzCore/std/smart_ptr/shared_ptr.h>

//#include <AzCore/Serialization/SerializeContext.h>
//#include <AzCore/Serialization/EditContext.h>

namespace AZ
{
	class ReflectContext;
}

namespace StarterGameGem
{
	class CameraSettings
	{
	public:
		AZ_RTTI(CameraSettings, "{0A6F349D-98F1-43DE-AE65-679669E089FC}");
		AZ_CLASS_ALLOCATOR(CameraSettings, AZ::SystemAllocator, 0);

		CameraSettings(bool reset = false);
		virtual ~CameraSettings() {}

		void CopySettings(const CameraSettings& other);
		void BlendSettings(const CameraSettings& setA, const CameraSettings& setB, float t);

		// Required Reflect function.
		static void Reflect(AZ::ReflectContext* context);

		AZStd::string m_name;
		int m_priority;

		float m_followDistance;
		float m_followDistanceLow;
		float m_followHeight;
		float m_followOffset;
		
		float m_pivotOffsetFront;
		float m_pivotOffsetBack;

		float m_maxLookUpAngle;
		float m_maxLookDownAngle;

		float m_verticalSpeed;
		float m_horizontalSpeed;
		float m_horizontalBoostSpeed;
		float m_horizontalBoostAccel;
		float m_horizontalBoostDecel;
		
		float m_controllerSensitivityPower;
		float m_mouseSensitivityScale;

		float m_fov;
		float m_nearClip;
		float m_farClip;

	};

	class CameraSettingsEventArgs
	{
	public:
		AZ_RTTI(CameraSettingsEventArgs, "{EF120303-86A3-448B-BEAD-C76415FCFD0C}");
		AZ_CLASS_ALLOCATOR(CameraSettingsEventArgs, AZ::SystemAllocator, 0);

		AZStd::string m_name;
		AZ::EntityId m_entityId;
	};
	

	/*!
	* CameraSettingsComponentRequests
	* Messages serviced by the CameraSettingsComponent
	*/
	class CameraSettingsComponentRequests
		: public AZ::ComponentBus
	{
	public:
		virtual ~CameraSettingsComponentRequests() {}

		virtual void PushSettings(const AZStd::string& settingsName, AZ::EntityId sourceEntity) = 0;
		virtual void PopSettings(const AZStd::string& settingsName, AZ::EntityId sourceEntity) = 0;
		virtual CameraSettings GetActiveSettings() = 0;

	};

	using CameraSettingsComponentRequestsBus = AZ::EBus<CameraSettingsComponentRequests>;


	class CameraSettingsComponent
		: public AZ::Component
		, private CameraSettingsComponentRequestsBus::Handler
	{
	public:
		AZ_COMPONENT(CameraSettingsComponent, "{84CD0FC9-2EEA-4A2C-83E9-D70C2B25B096}");

		//////////////////////////////////////////////////////////////////////////
		// AZ::Component interface implementation
		void Init() override;
		void Activate() override;
		void Deactivate() override;
		//////////////////////////////////////////////////////////////////////////

		// Required Reflect function.
		static void Reflect(AZ::ReflectContext* context);

		//////////////////////////////////////////////////////////////////////////
		// CameraSettingsComponentRequestsBus::Handler
		void PushSettings(const AZStd::string& settingsName, AZ::EntityId sourceEntity);
		void PopSettings(const AZStd::string& settingsName, AZ::EntityId sourceEntity);
		CameraSettings GetActiveSettings();

	protected:

		const CameraSettings *FindSettings(const AZStd::string& settingsName) const;

		AZStd::string m_initialSettings;
		AZStd::vector<CameraSettings> m_settings;

		struct CameraSettingsStackEntry
		{
			const CameraSettings *m_settings;
			AZ::EntityId m_entity;
			int m_priority;
		};
		AZStd::vector<CameraSettingsStackEntry> m_settingsStack;

	};

} // namespace StarterGameGem
