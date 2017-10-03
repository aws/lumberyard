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

#include <AzCore/Component/EntityId.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <MathConversion.h>

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/EBus/EBus.h>
#include <LmbrCentral/Physics/PhysicsSystemComponentBus.h>
#include <LmbrCentral/Physics/PhysicsComponentBus.h>
#include <AZCore/Component/TransformBus.h>
#include <LmbrCentral/Scripting/TagComponentBus.h>

#include <AzCore/Casting/numeric_cast.h>
#include <IPhysics.h>
#include <physinterface.h>

namespace AZ
{
	class ReflectContext;
}

namespace StarterGameGem
{
	struct GotShotParams
	{
		AZ_TYPE_INFO(GotShotParams, "{BC1EA56B-4099-41E7-BCE3-2163BBE26D04}");
		AZ_CLASS_ALLOCATOR(GotShotParams, AZ::SystemAllocator, 0);

		GotShotParams()
			: m_damage(0.0f)
			, m_immediatelyRagdoll(false)
		{}

		float m_damage;
		AZ::Vector3 m_direction;
		AZ::EntityId m_assailant;
		bool m_immediatelyRagdoll;

	};

    struct InteractParams
    {
        AZ_TYPE_INFO(InteractParams, "{1A8A5460-8D8A-4FC9-A31A-06C7349DD427}");
        AZ_CLASS_ALLOCATOR(InteractParams, AZ::SystemAllocator, 0);

        InteractParams()
        {}

        AZ::EntityId m_positionEntity;
        AZ::EntityId m_cameraEntity;
    };

	/*!
	* Wrapper for utility functions exposed to Lua for StarterGame.
	*/
	class StarterGameUtility
	{
	public:
		AZ_TYPE_INFO(StarterGameUtility, "{E8AD1E8A-A67D-44EB-8A08-B6881FD72F2F}");
		AZ_CLASS_ALLOCATOR(StarterGameUtility, AZ::SystemAllocator, 0);

		static void Reflect(AZ::ReflectContext* reflection);

        static AZStd::string GetEntityName(AZ::EntityId entityId);
        static AZ::Uuid GetUuid(const AZStd::string& className);

	};

} // namespace StarterGameGem
