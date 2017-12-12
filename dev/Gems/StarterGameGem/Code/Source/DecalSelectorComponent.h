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

#include <AzCore/Math/Transform.h>

namespace AZ
{
	class ReflectContext;
}

namespace StarterGameGem
{
	/*
	* The information required to know which decal should be used for each surface.
	*/
	class DecalPool
	{
	public:
		AZ_RTTI(DecalPool, "{4714324F-4ED2-4226-989C-0746CA15EE74}");
		AZ_CLASS_ALLOCATOR(DecalPool, AZ::SystemAllocator, 0);

		DecalPool();

		void AddDecal(AZ::EntityId& decal);
		AZ::EntityId GetAvailableDecal();

		AZStd::vector<AZ::EntityId> m_pool;
		int m_index;
	};

	/*!
	* DecalSelectorComponentRequests
	* Messages serviced by the DecalSelectorComponent
	*/
	class DecalSelectorComponentRequests
		: public AZ::ComponentBus
	{
	public:
		virtual ~DecalSelectorComponentRequests() {}

        struct DecalSpawnerParams
        {
            AZ_TYPE_INFO(DecalSpawnerParams, "{C564B5F2-CAB8-424C-BBE1-21F99F9EF39A}");
            AZ_CLASS_ALLOCATOR(DecalSpawnerParams, AZ::SystemAllocator, 0);

            AZStd::string m_eventName = "";
            AZ::Transform m_transform = AZ::Transform::CreateIdentity();
            AZ::EntityId m_target = AZ::EntityId();
            bool m_attachToTargetEntity = false;
            short m_surfaceType = 0;
        };

		//! Spawns a particle.
		virtual void SpawnDecal(const DecalSpawnerParams& params) {}
		virtual void AddDecalToPool(AZ::EntityId decal, int surfaceIndex) {}
	};

	using DecalSelectorComponentRequestsBus = AZ::EBus<DecalSelectorComponentRequests>;


	class DecalSelectorComponent
		: public AZ::Component
		, private DecalSelectorComponentRequestsBus::Handler
	{
	public:
		AZ_COMPONENT(DecalSelectorComponent, "{EAF2F82F-0901-41F4-9C74-AB982FFCFB99}");

		//////////////////////////////////////////////////////////////////////////
		// AZ::Component interface implementation
		void Init() override;
		void Activate() override;
		void Deactivate() override;
		//////////////////////////////////////////////////////////////////////////

		// Required Reflect function.
		static void Reflect(AZ::ReflectContext* context);

		//////////////////////////////////////////////////////////////////////////
		// DecalSelectorComponentRequestsBus::Handler
		void SpawnDecal(const DecalSpawnerParams& params) override;

		void AddDecalToPool(AZ::EntityId decal, int surfaceIndex) override;

	private:
		void ResetDecalEntity(AZ::EntityId& decal);

		bool m_useDefaultMat = false;

		AZStd::vector<DecalPool> m_decalPools;


	};

} // namespace StarterGameGem
