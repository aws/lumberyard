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
#include <AzCore/std/any.h>
#include <GameplayEventBus.h>

#include <IPathfinder.h>

namespace StarterGameGem
{


	/*!
	* Requests for the physics system
	*/
	class VisualisePathSystemRequests
		: public AZ::EBusTraits
	{
	public:
		////////////////////////////////////////////////////////////////////////
		// EBusTraits
		// singleton pattern
		static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
		static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
		////////////////////////////////////////////////////////////////////////

		virtual ~VisualisePathSystemRequests() = default;

		//virtual bool HasData(const AZStd::string& name) = 0;
	};

	using VisualisePathSystemRequestBus = AZ::EBus<VisualisePathSystemRequests>;

	/*!
	 * System component which listens for IPhysicalWorld events,
	 * filters for events that involve AZ::Entities,
	 * and broadcasts these events on the EntityPhysicsEventBus.
	 */
	class VisualisePathSystemComponent
		: public AZ::Component
		, public VisualisePathSystemRequestBus::Handler
		, private AZ::TickBus::Handler
	{
	public:
		AZ_COMPONENT(VisualisePathSystemComponent, "{2DB069B9-85F1-43B9-85A1-07FF273335CD}");

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
			provided.push_back(AZ_CRC("VisualisePathSystemService"));
		}

		static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
		{
			incompatible.push_back(AZ_CRC("VisualisePathSystemService"));
		}

		~VisualisePathSystemComponent() override {}

		static VisualisePathSystemComponent* GetInstance();

		void SetPath(const AZ::EntityId& id, AZStd::shared_ptr<const INavPath> path);
		void ClearPath(const AZ::EntityId& id);


	private:
		struct Path
		{
			Path()
				: entity()
			{}

			Path(const AZ::EntityId& e, AZStd::vector<Vec3> p)
				: entity(e)
				, path(p)
			{}

			AZ::EntityId entity;
			AZStd::vector<Vec3> path;
		};

		Path* GetPath(const AZ::EntityId& id);

		AZStd::list<Path> m_paths;

	};
} // namespace StarterGameGem
