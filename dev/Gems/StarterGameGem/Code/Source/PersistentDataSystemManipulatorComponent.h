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
#pragma once

//#include <LmbrCentral/Physics/PersistentDataSystemComponentBus.h>
#include <AzCore/Component/Component.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/any.h>
#include <GameplayEventBus.h>
#include "PersistentDataSystemComponent.h"

namespace StarterGameGem
{

	/*!
	 * System component which listens for IPhysicalWorld events,
	 * filters for events that involve AZ::Entities,
	 * and broadcasts these events on the EntityPhysicsEventBus.
	 */
	class PersistentDataSystemManipulatorComponent
		: public AZ::Component
		, public AZ::GameplayNotificationBus::Handler
	{
	public:
		AZ_COMPONENT(PersistentDataSystemManipulatorComponent, "{7021A957-8A66-4EED-A24A-AA9691F0FA69}");

		PersistentDataSystemManipulatorComponent();

		static void Reflect(AZ::ReflectContext* context);


		~PersistentDataSystemManipulatorComponent() override {}

		//static eBasicDataTypes GetBasicType(const AZStd::any& value);
		//// ture if passes test, otherwise false, including type missmatch
		//bool Compare(const AZStd::string& name, const AZStd::any& value, const PersistentDataSystemComponent::eComparison compareType);
		//// returns tru if operation could be performed
		//bool Manipulate(const AZStd::string& name, const AZStd::any& value, const PersistentDataSystemComponent::eDataManipulationTypes manipulationType);

		//////////////////////////////////////////////////////////////////////////
		// AZ::GameplayNotificationBus
		void OnEventBegin(const AZStd::any& value) override;
		void OnEventUpdating(const AZStd::any& value) override;
		void OnEventEnd(const AZStd::any& value) override;

	private:
		////////////////////////////////////////////////////////////////////////
		// AZ::Component
		void Activate() override;
		void Deactivate() override;
		////////////////////////////////////////////////////////////////////////


		AZStd::string m_keyName;

		PersistentDataSystemComponent::eBasicDataTypes m_dataToUse;
		bool m_BoolData;
		AZStd::string m_stringData;
		float m_numberData;
		PersistentDataSystemComponent::eDataManipulationTypes m_useForData;

		AZStd::string m_triggerMessage;


		AZ::GameplayNotificationId m_triggerMessageID;

	};
} // namespace LmbrCentral
