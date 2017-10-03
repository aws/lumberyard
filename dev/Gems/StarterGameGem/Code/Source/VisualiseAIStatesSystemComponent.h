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

#include <AzCore/Component/Component.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Memory/SystemAllocator.h>


namespace StarterGameGem
{
	/*!
	* Requests for the A.I. state visualisation system.
	*/
	class VisualiseAIStatesSystemRequests
		: public AZ::EBusTraits
	{
	public:
		////////////////////////////////////////////////////////////////////////
		// EBusTraits
		// singleton pattern
		static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
		static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
		////////////////////////////////////////////////////////////////////////

		virtual ~VisualiseAIStatesSystemRequests() = default;

        virtual void SetAIState(const AZ::EntityId& entityId, int state) = 0;
        virtual void ClearAIState(const AZ::EntityId& entityId) = 0;
	};

	using VisualiseAIStatesSystemRequestBus = AZ::EBus<VisualiseAIStatesSystemRequests>;

	/*!
	 * System component which listens for A.I. state changes and display visualises to clearly
     * indicate what state each A.I. is in.
	 */
	class VisualiseAIStatesSystemComponent
		: public AZ::Component
		, public VisualiseAIStatesSystemRequestBus::Handler
		, private AZ::TickBus::Handler
	{
	public:
		AZ_COMPONENT(VisualiseAIStatesSystemComponent, "{B099B349-0451-4439-B40A-20CFF4764F7B}");

		////////////////////////////////////////////////////////////////////////
		// AZ::Component interface implementation
		void Activate() override;
		void Deactivate() override;
		////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		// AZ::TickBus interface implementation
		void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
		//////////////////////////////////////////////////////////////////////////

		// Required Reflect function.
		static void Reflect(AZ::ReflectContext* context);

		static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
		{
			provided.push_back(AZ_CRC("VisualiseAIStatesSystemService"));
		}

		static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
		{
			incompatible.push_back(AZ_CRC("VisualiseAIStatesSystemService"));
		}

		~VisualiseAIStatesSystemComponent() override {}

		static VisualiseAIStatesSystemComponent* GetInstance();

        ////////////////////////////////////////////////////////////////////////
        // VisualiseAIStatesSystemRequests
        virtual void SetAIState(const AZ::EntityId& entityId, int state) override;
        virtual void ClearAIState(const AZ::EntityId& entityId) override;
        ////////////////////////////////////////////////////////////////////////


	private:
        struct AIState
        {
            AZ::EntityId m_entityId;
            int m_state;
        };

        AIState* const FindState(const AZ::EntityId& entityId);

		AZStd::list<AIState> m_states;

	};
} // namespace StarterGameGem
