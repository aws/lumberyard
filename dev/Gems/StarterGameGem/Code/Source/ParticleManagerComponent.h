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

#include <DecalSelectorComponent.h>

namespace AZ
{
	class ReflectContext;
}

namespace StarterGameGem
{


	/*!
	* ParticleManagerComponentRequests
	* Messages serviced by the ParticleManagerComponent
	*/
	class ParticleManagerComponentRequests
		: public AZ::ComponentBus
	{

	public:
		virtual ~ParticleManagerComponentRequests() {}

        struct ParticleSpawnerParams
        {
            AZ_TYPE_INFO(ParticleSpawnerParams, "{E1E4BB32-1B51-4CB8-A351-9CEA7DD5EE8C}");
            AZ_CLASS_ALLOCATOR(ParticleSpawnerParams, AZ::SystemAllocator, 0);

            AZStd::string m_eventName = "";
            AZ::Transform m_transform = AZ::Transform::CreateIdentity();
            AZ::EntityId m_owner = AZ::EntityId();
            AZ::EntityId m_target = AZ::EntityId();
            bool m_attachToTargetEntity = false;
            AZ::Vector3 m_impulse = AZ::Vector3::CreateZero();
        };

		//! Spawns a particle.
        virtual void SpawnParticle(const ParticleSpawnerParams& params) = 0;

        virtual void SpawnDecal(const DecalSelectorComponentRequests::DecalSpawnerParams& params) = 0;

        virtual bool AcceptsDecals(const DecalSelectorComponentRequests::DecalSpawnerParams& params) = 0;

	};

	using ParticleManagerComponentRequestsBus = AZ::EBus<ParticleManagerComponentRequests>;


	class ParticleManagerComponent
		: public AZ::Component
		, private ParticleManagerComponentRequestsBus::Handler
	{
	public:
		AZ_COMPONENT(ParticleManagerComponent, "{35991A4D-69E8-4520-85DB-E8866225CAAE}");

		//////////////////////////////////////////////////////////////////////////
		// AZ::Component interface implementation
		void Init() override;
		void Activate() override;
		void Deactivate() override;
		//////////////////////////////////////////////////////////////////////////

		// Required Reflect function.
		static void Reflect(AZ::ReflectContext* context);

		//////////////////////////////////////////////////////////////////////////
		// ParticleManagerComponentRequestsBus::Handler
		void SpawnParticle(const ParticleSpawnerParams& params) override;

        void SpawnDecal(const DecalSelectorComponentRequests::DecalSpawnerParams& params) override;

        bool AcceptsDecals(const DecalSelectorComponentRequests::DecalSpawnerParams& params) override;
        //////////////////////////////////////////////////////////////////////////

	private:


	};

} // namespace StarterGameGem
