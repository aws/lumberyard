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
	class PersistentDataSystemComparitorComponent
		: public AZ::Component
		, public AZ::GameplayNotificationBus::MultiHandler
	{
	public:
		AZ_COMPONENT(PersistentDataSystemComparitorComponent, "{362949E8-F603-4D31-9581-01B966BFF1BB}");

		PersistentDataSystemComparitorComponent();

		static void Reflect(AZ::ReflectContext* context);


		~PersistentDataSystemComparitorComponent() override {}

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

		bool m_sendMessagesOnChange;
		AZStd::string m_testMessage;

		PersistentDataSystemComponent::eBasicDataTypes m_dataToUse;
		bool m_BoolData;
		AZStd::string m_stringData;
		float m_numberData;
		PersistentDataSystemComponent::eComparison m_comparisonForData;

		AZStd::string m_successMessage;
		AZStd::string m_failMessage;
		AZ::EntityId m_targetOfMessage;

		AZ::GameplayNotificationId m_callbackMessageID;
		AZ::GameplayNotificationId m_testMessageID;
	};
} // namespace LmbrCentral
