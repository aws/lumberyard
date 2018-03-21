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
#include "StarterGameUtility.h"

#include <AzCore/RTTI/BehaviorContext.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AZCore/Component/TransformBus.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/time.h>
#include <CryAction.h>
#include <ICryAnimation.h>
#include <AzFramework/Physics/PhysicsSystemComponentBus.h>
#include <LmbrCentral/Rendering/MeshComponentBus.h>
#include <MathConversion.h>
#include <EMotionFX/Source/EMotionFX.h>
#include <Include/Integration/ActorComponentBus.h>
#include <Source/Integration/Components/ActorComponent.h>

namespace StarterGameGem
{
    // This has been copied from PhysicsSystemComponent.cpp because they made it private but it's
    // needed here as well.
    unsigned int StarterGameUtility::EntFromEntityTypes(AZ::u32 types)
    {
        // Shortcut when requesting all entities
        if (types == AzFramework::PhysicalEntityTypes::All)
        {
            return ent_all;
        }

        unsigned int result = 0;

        if (types & AzFramework::PhysicalEntityTypes::Static)
        {
            result |= ent_static | ent_terrain;
        }
        if (types & AzFramework::PhysicalEntityTypes::Dynamic)
        {
            result |= ent_rigid | ent_sleeping_rigid;
        }
        if (types & AzFramework::PhysicalEntityTypes::Living)
        {
            result |= ent_living;
        }
        if (types & AzFramework::PhysicalEntityTypes::Independent)
        {
            result |= ent_independent;
        }

        return result;
    }

    bool StarterGameUtility::IsGameStarted()
    {
        //CCryAction* pCryAction = CCryAction::GetCryAction();
        CCryAction* pCryAction = static_cast<CCryAction*>(gEnv->pGame->GetIGameFramework());
        return pCryAction->IsGameStarted();
    }

    void StarterGameUtility::RestartLevel(const bool& fade)
    {
        //CCryAction* pCryAction = CCryAction::GetCryAction();
        CCryAction* pCryAction = static_cast<CCryAction*>(gEnv->pGame->GetIGameFramework());
        pCryAction->ScheduleEndLevelNow(pCryAction->GetLevelName(), fade);
    }
	bool StarterGameUtility::IsLegacyCharacter(const AZ::EntityId& entityId)
	{
        ICharacterInstance* character = nullptr;
        EBUS_EVENT_ID_RESULT(character, entityId, LmbrCentral::SkinnedMeshComponentRequestBus, GetCharacterInstance);
		return character != nullptr;
	}
    AZ::Transform StarterGameUtility::GetJointWorldTM(const AZ::EntityId& entityId, const AZStd::string& bone)
    {
        AZ::Transform tm = AZ::Transform::CreateIdentity();
        EBUS_EVENT_ID_RESULT(tm, entityId, AZ::TransformBus, GetWorldTM);
		AZ::s32 jointIndex;
		EBUS_EVENT_ID_RESULT(jointIndex, entityId, LmbrCentral::SkeletalHierarchyRequestBus, GetJointIndexByName, bone.c_str());
		if (jointIndex >= 0)
		{
			AZ::Transform characterRelativeTransform;
			EBUS_EVENT_ID_RESULT(characterRelativeTransform, entityId, LmbrCentral::SkeletalHierarchyRequestBus, GetJointTransformCharacterRelative, jointIndex);
			tm *= characterRelativeTransform;
		}
        return tm;
    }

    // This function was created because I couldn't populate the 'Collision' struct in Lua.
    AzFramework::PhysicsComponentNotifications::Collision StarterGameUtility::CreatePseudoCollisionEvent(const AZ::EntityId& entity, const AZ::Vector3& position, const AZ::Vector3& normal, const AZ::Vector3& direction)
    {
        AzFramework::PhysicsComponentNotifications::Collision coll;
        coll.m_entity = entity;
        coll.m_position = position;
        coll.m_normal = normal;
        coll.m_surfaces[1] = GetSurfaceFromRayCast(position, direction);
        return coll;
    }

    // Gets the surface type of the first thing that's hit.
    int StarterGameUtility::GetSurfaceFromRayCast(const AZ::Vector3& pos, const AZ::Vector3& direction)
    {
        AZ::u32 query = 15;			// hit everything
        AZ::u32 pierceability = 14;	// stop at the first thing
        float maxDistance = 1.1f;
        AZ::u32 maxHits = 1;		// only care about the first thing

        AZStd::vector<ray_hit> cryHits(maxHits);
        Vec3 start = AZVec3ToLYVec3(pos);
        Vec3 end = AZVec3ToLYVec3(direction.GetNormalized() * maxDistance);
        unsigned int flags = EntFromEntityTypes(query);

        int surfaceType = -1;

        // Perform raycast
        int hitCount = gEnv->pPhysicalWorld->RayWorldIntersection(start, end, flags, pierceability, cryHits.data(), aznumeric_caster(maxHits));

        if (hitCount != 0)
        {
            const ray_hit& cryHit = cryHits[0];

            if (cryHit.dist > 0.0f)
            {
                surfaceType = cryHit.surface_idx;
            }
        }

        return surfaceType;
    }

    AZStd::sys_time_t StarterGameUtility::GetTimeNowMicroSecond()
    {
        return AZStd::GetTimeNowMicroSecond();
    }

    // This is not exposed to Lua because Lua's GameplayNotificationId constructor has this
    // built-in and, in fact, is where this code is copied from: ReflectScriptableEvents.cpp (177).
    AZ::Uuid StarterGameUtility::GetUuid(const AZStd::string& className)
    {
        AZ::Crc32 classNameCrc = AZ::Crc32(className.c_str());
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        AZ_Assert(serializeContext != nullptr, "SerializeContext couldn't be found.");
        AZStd::vector<AZ::Uuid> classIds = serializeContext->FindClassId(classNameCrc);

        if (classIds.size() == 1)
        {
            return classIds[0];
        }
        else
        {
            if (classIds.size() == 0)
            {
                AZ_Warning("StarterGame", false, "No class found with key %s", className.c_str());
            }
            else
            {
                AZStd::string errorOutput = AZStd::string::format("Too many classes with key %s.  You may need to create a Uiid via Uuid.CreateString() using one of the following uuids: ", className.c_str());
                for (AZ::Uuid classId : classIds)
                {
                    errorOutput += AZStd::string::format("%s, ", classId.ToString<AZStd::string>().c_str());
                }
                AZ_Warning("StarterGame", false, errorOutput.c_str());
            }
        }

        return AZ::Uuid::CreateNull();
    }

    void StarterGameUtility::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection);
        if (behaviorContext)
        {
            behaviorContext->Class<StarterGameUtility>("StarterGameUtility")
                ->Method("IsGameStarted", &IsGameStarted)
				->Method("IsLegacyCharacter", &IsLegacyCharacter)
				->Method("RestartLevel", &RestartLevel)

                ->Method("GetJointWorldTM", &GetJointWorldTM)

                ->Method("GetSurfaceFromRayCast", &GetSurfaceFromRayCast)

                ->Method("CreatePseudoCollisionEvent", &CreatePseudoCollisionEvent)

                ->Method("ArcTan2", &atan2f)

                ->Method("GetTimeNowMicroSecond", &GetTimeNowMicroSecond)
            ;

            behaviorContext->Class<GotShotParams>("GotShotParams")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Property("damage", BehaviorValueProperty(&GotShotParams::m_damage))
                ->Property("direction", BehaviorValueProperty(&GotShotParams::m_direction))
                ->Property("assailant", BehaviorValueProperty(&GotShotParams::m_assailant))
                ->Property("immediatelyRagdoll", BehaviorValueProperty(&GotShotParams::m_immediatelyRagdoll))
            ;

            behaviorContext->Class<InteractParams>("InteractParams")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Property("positionEntity", BehaviorValueProperty(&InteractParams::m_positionEntity))
                ->Property("cameraEntity", BehaviorValueProperty(&InteractParams::m_cameraEntity))
            ;

            behaviorContext->Class<PlayAnimParams>("PlayAnimParams")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Property("animName", BehaviorValueProperty(&PlayAnimParams::m_animName))
                ->Property("loop", BehaviorValueProperty(&PlayAnimParams::m_loop))
            ;

            behaviorContext->Class<CompanionPOIParams>("CompanionPOIParams")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Property("reset", BehaviorValueProperty(&CompanionPOIParams::m_reset))
                ->Property("position", BehaviorValueProperty(&CompanionPOIParams::m_position))
                ->Property("lookAt", BehaviorValueProperty(&CompanionPOIParams::m_lookAt))
            ;
        }

    }

}
