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


#include "StdAfx.h"
#include "StarterGameUtility.h"
#include "CryAction.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <LyShine/Bus/UiCanvasBus.h>
#include <UiFaderComponent.h>
#include <UiScrollBarComponent.h>
#include <UiSliderComponent.h>
#include <UiTransform2dComponent.h>
#include <UiTextComponent.h>
#include <AzCore/Math/Vector2.h>
#include <IMovieSystem.h>
#include <CameraComponent.h>

#include <IAISystem.h>
#include <INavigationSystem.h>

namespace StarterGameGem
{
    // This has been copied from PhysicsSystemComponent.cpp because they made it private but it's
    // needed here as well.
    unsigned int EntFromEntityTypes(AZ::u32 types)
    {
        // Shortcut when requesting all entities
        if (types == LmbrCentral::PhysicalEntityTypes::All)
        {
            return ent_all;
        }

        unsigned int result = 0;

        if (types & LmbrCentral::PhysicalEntityTypes::Static)
        {
            result |= ent_static | ent_terrain;
        }
        if (types & LmbrCentral::PhysicalEntityTypes::Dynamic)
        {
            result |= ent_rigid | ent_sleeping_rigid;
        }
        if (types & LmbrCentral::PhysicalEntityTypes::Living)
        {
            result |= ent_living;
        }
        if (types & LmbrCentral::PhysicalEntityTypes::Independent)
        {
            result |= ent_independent;
        }

        return result;
    }

    AZ::EntityId FindClosestFromTag(const LmbrCentral::Tag& tag)
    {
        AZ::EBusAggregateResults<AZ::EntityId> results;
        AZ::EntityId ret = AZ::EntityId();
        EBUS_EVENT_ID_RESULT(results, tag, LmbrCentral::TagGlobalRequestBus, RequestTaggedEntities);
        for (const AZ::EntityId& entity : results.values)
        {
            if (entity.IsValid())
            {
                ret = entity;
                break;
            }
        }

        return ret;
    }

    // Get a random float between a and b
    float randomF(float a, float b)
    {
        float randNum = (float)std::rand() / RAND_MAX;
        if (a > b)
            std::swap(a, b);
        float delta = b - a;
        return a + delta * randNum;
    }

    // Gets the surface type of the first thing that's hit.
    int GetSurfaceType(const AZ::Vector3& pos, const AZ::Vector3& direction)
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

    int GetSurfaceIndexFromString(const AZStd::string surfaceName)
    {
        return gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeIdByName(surfaceName.c_str());
    }

    AZStd::string GetSurfaceNameFromId(int surfaceId)
    {
        ISurfaceType* surfaceType = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceType(surfaceId);
        if (surfaceType == nullptr)
        {
            return "unknown";
        }

        return surfaceType->GetName();
    }

    AZ::EntityId GetParentEntity(const AZ::EntityId& entityId)
    {
        AZ::EntityId parentId;
        EBUS_EVENT_ID_RESULT(parentId, entityId, AZ::TransformBus, GetParentId);
        return parentId;
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

    AZStd::string StarterGameUtility::GetEntityName(AZ::EntityId entityId)
    {
        AZStd::string entityName;
        AZ::ComponentApplicationBus::BroadcastResult(entityName, &AZ::ComponentApplicationRequests::GetEntityName, entityId);
        return entityName;
    }

    bool EntityHasTag(const AZ::EntityId& entityId, const AZStd::string& tag)
    {
        bool hasTag = false;
        LmbrCentral::TagComponentRequestBus::EventResult(hasTag, entityId, &LmbrCentral::TagComponentRequestBus::Events::HasTag, AZ::Crc32(tag.c_str()));
        return hasTag;
    }

    void RestartLevel(const bool& fade)
    {
        //CCryAction* pCryAction = CCryAction::GetCryAction();
        CCryAction* pCryAction = static_cast<CCryAction*>(gEnv->pGame->GetIGameFramework());
        pCryAction->ScheduleEndLevelNow(pCryAction->GetLevelName(), fade);
    }

    bool IsGameStarted()
    {
        //CCryAction* pCryAction = CCryAction::GetCryAction();
        CCryAction* pCryAction = static_cast<CCryAction*>(gEnv->pGame->GetIGameFramework());
        return pCryAction->IsGameStarted();
    }

    void UIFaderControl(const AZ::EntityId& canvasEntityID, const int& faderID, const float &fadeValue, const float& fadeTime)
    {
        if (!canvasEntityID.IsValid())
        {
            // AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::UIFaderControl: %s Component: is not valid\n", canvasEntityID.ToString());
            return;
        }

        AZ::Entity* element = nullptr;
        EBUS_EVENT_ID_RESULT(element, canvasEntityID, UiCanvasBus, FindElementById, faderID);
        if (element)
        {
            const AZ::Entity::ComponentArrayType& components = element->GetComponents();
            UiFaderComponent* faderComp = NULL;
            for (int count = 0; count < components.size(); ++count)
            {
                faderComp = azdynamic_cast<UiFaderComponent*>(element->GetComponents()[count]);

                if (faderComp != NULL)
                {
                    break;
                }
            }

            if (faderComp)
            {
				// fade from the value that i am at to the value that i want over the time specified.
				// so i need to calculate the "speed" of the fade
				const float currentFade = faderComp->GetFadeValue();
				float fadeSpeed = (fadeTime == 0) ? 0 : (abs(currentFade - fadeValue) / fadeTime);
                faderComp->Fade(fadeValue, fadeSpeed);
            }
            else
            {
                //AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::UIFaderControl: %s Component: couldn't find a fader component on UI element with ID: %d\n", canvasEntityID.ToString(), faderID);
            }
        }
        else
        {
            //AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::UIFaderControl: %s Component: couldn't find an image component with ID: %d\n", canvasEntityID.ToString(), faderID);
        }
    }

    void UIScrollControl(const AZ::EntityId& canvasEntityID, const int& scrollID, const float &value)
    {
        if (!canvasEntityID.IsValid())
        {
            // AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::UIScrollControl: %s Component: is not valid\n", canvasEntityID.ToString());
            return;
        }

        AZ::Entity* element = nullptr;
        EBUS_EVENT_ID_RESULT(element, canvasEntityID, UiCanvasBus, FindElementById, scrollID);
        if (element)
        {
            const AZ::Entity::ComponentArrayType& components = element->GetComponents();
            UiScrollBarComponent* scrollComp = NULL;
            for (int count = 0; count < components.size(); ++count)
            {
                scrollComp = azdynamic_cast<UiScrollBarComponent*>(element->GetComponents()[count]);

                if (scrollComp != NULL)
                {
                    break;
                }
            }

            if (scrollComp)
            {
                scrollComp->SetValue(value);
            }
            else
            {
                //AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::UIScrollControl: %s Component: couldn't find a scroll component on UI element with ID: %d\n", canvasEntityID.ToString(), faderID);
            }
        }
        else
        {
            //AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::UIScrollControl: %s Component: couldn't find an image component with ID: %d\n", canvasEntityID.ToString(), faderID);
        }
    }

    void UISliderControl(const AZ::EntityId& canvasEntityID, const int& sliderID, const float &value)
    {
        if (!canvasEntityID.IsValid())
        {
            // AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::UISliderControl: %s Component: is not valid\n", canvasEntityID.ToString());
            return;
        }

        AZ::Entity* element = nullptr;
        EBUS_EVENT_ID_RESULT(element, canvasEntityID, UiCanvasBus, FindElementById, sliderID);
        if (element)
        {
            const AZ::Entity::ComponentArrayType& components = element->GetComponents();
            UiSliderComponent* sliderComp = NULL;
            for (int count = 0; count < components.size(); ++count)
            {
                sliderComp = azdynamic_cast<UiSliderComponent*>(element->GetComponents()[count]);

                if (sliderComp != NULL)
                {
                    break;
                }
            }

            if (sliderComp)
            {
                sliderComp->SetValue(value);
            }
            else
            {
                //AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::UISliderControl: %s Component: couldn't find a slider component on UI element with ID: %d\n", canvasEntityID.ToString(), faderID);
            }
        }
        else
        {
            //AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::UISliderControl: %s Component: couldn't find an image component with ID: %d\n", canvasEntityID.ToString(), faderID);
        }
    }

    void UIElementMover(const AZ::EntityId& canvasEntityID, const int& elementID, const AZ::Vector2& value)
    {
        if (!canvasEntityID.IsValid())
        {
            // AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::UIElementMover: %s Component: is not valid\n", canvasEntityID.ToString());
            return;
        }

        AZ::Entity* element = nullptr;
        EBUS_EVENT_ID_RESULT(element, canvasEntityID, UiCanvasBus, FindElementById, elementID);
        if (element)
        {
            const AZ::Entity::ComponentArrayType& components = element->GetComponents();
            UiTransform2dComponent* transformComp = NULL;
            for (int count = 0; count < components.size(); ++count)
            {
                transformComp = azdynamic_cast<UiTransform2dComponent*>(element->GetComponents()[count]);

                if (transformComp != NULL)
                {
                    break;
                }
            }

            if (transformComp)
            {
                UiTransform2dInterface::Offsets offs(transformComp->GetOffsets());
                offs.m_left = offs.m_right = value.GetX();
                offs.m_top = offs.m_bottom = value.GetY();
                transformComp->SetOffsets(offs);
            }
            else
            {
                //AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::UIElementMover: %s Component: couldn't find a transform component on UI element with ID: %d\n", canvasEntityID.ToString(), faderID);
            }
        }
        else
        {
            //AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::UIElementMover: %s Component: couldn't find an image component with ID: %d\n", canvasEntityID.ToString(), faderID);
        }
    }

	void UIElementScaler(const AZ::EntityId& canvasEntityID, const int& elementID, const AZ::Vector2& value)
	{
		if (!canvasEntityID.IsValid())
		{
			// AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::UIElementScaler: %s Component: is not valid\n", canvasEntityID.ToString());
			return;
		}

		AZ::Entity* element = nullptr;
		EBUS_EVENT_ID_RESULT(element, canvasEntityID, UiCanvasBus, FindElementById, elementID);
		if (element)
		{
			const AZ::Entity::ComponentArrayType& components = element->GetComponents();
			UiTransform2dComponent* transformComp = NULL;
			for (int count = 0; count < components.size(); ++count)
			{
				transformComp = azdynamic_cast<UiTransform2dComponent*>(element->GetComponents()[count]);

				if (transformComp != NULL)
				{
					break;
				}
			}

			if (transformComp)
			{
				transformComp->SetScale(value);
			}
			else
			{
				//AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::UIElementScaler: %s Component: couldn't find a transform component on UI element with ID: %d\n", canvasEntityID.ToString(), faderID);
			}
		}
		else
		{
			//AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::UIElementScaler: %s Component: couldn't find an image component with ID: %d\n", canvasEntityID.ToString(), faderID);
		}
	}

	void UIElementRotator(const AZ::EntityId& canvasEntityID, const int& elementID, const float& value)
	{
		if (!canvasEntityID.IsValid())
		{
			// AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::UIElementRotator: %s Component: is not valid\n", canvasEntityID.ToString());
			return;
		}

		AZ::Entity* element = nullptr;
		EBUS_EVENT_ID_RESULT(element, canvasEntityID, UiCanvasBus, FindElementById, elementID);
		if (element)
		{
			const AZ::Entity::ComponentArrayType& components = element->GetComponents();
			UiTransform2dComponent* transformComp = NULL;
			for (int count = 0; count < components.size(); ++count)
			{
				transformComp = azdynamic_cast<UiTransform2dComponent*>(element->GetComponents()[count]);

				if (transformComp != NULL)
				{
					break;
				}
			}

			if (transformComp)
			{
				transformComp->SetZRotation(value);
			}
			else
			{
				//AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::UIElementRotator: %s Component: couldn't find a transform component on UI element with ID: %d\n", canvasEntityID.ToString(), faderID);
			}
		}
		else
		{
			//AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::UIElementRotator: %s Component: couldn't find an image component with ID: %d\n", canvasEntityID.ToString(), faderID);
		}
	}

	void UITextSetter(const AZ::EntityId& canvasEntityID, const int& textID, const AZStd::string& value)
	{
		if (!canvasEntityID.IsValid())
		{
			// AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::UISliderControl: %s Component: is not valid\n", canvasEntityID.ToString());
			return;
		}

		AZ::Entity* element = nullptr;
		EBUS_EVENT_ID_RESULT(element, canvasEntityID, UiCanvasBus, FindElementById, textID);
		if (element)
		{
			const AZ::Entity::ComponentArrayType& components = element->GetComponents();
			UiTextComponent* textComp = NULL;
			for (int count = 0; count < components.size(); ++count)
			{
				textComp = azdynamic_cast<UiTextComponent*>(element->GetComponents()[count]);

				if (textComp != NULL)
				{
					break;
				}
			}

			if (textComp)
			{
				textComp->SetTextWithFlags(value, UiTextInterface::SetEscapeMarkup);
			}
			else
			{
				//AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::UISliderControl: %s Component: couldn't find a transform component on UI element with ID: %d\n", canvasEntityID.ToString(), faderID);
			}
		}
		else
		{
			//AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::UISliderControl: %s Component: couldn't find an image component with ID: %d\n", canvasEntityID.ToString(), faderID);
		}
	}

    bool GetElementPosOnScreen(AZ::EntityId entityId, AZ::Vector2& value)
    {
        if (!entityId.IsValid())
        {
            // AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::UISliderControl: %s Component: is not valid\n", canvasEntityID.ToString());
            return false;
        }

        AZ::Transform transform;
        EBUS_EVENT_ID_RESULT(transform, entityId, AZ::TransformBus, GetWorldTM);

        const CCamera& camera = gEnv->pRenderer->GetCamera();
        Vec3 screenPos;
        if (camera.Project(AZVec3ToLYVec3(transform.GetPosition()), screenPos, Vec2i(0, 0), Vec2i(0, 0)))
        {
            value.SetX(screenPos.x);
            value.SetY(screenPos.y); // the height is z
            return true;
        }
        return false;
    }

    void PlaySequence(const AZStd::string& sequenceName)
    {
   		IMovieSystem* movieSys = gEnv->pMovieSystem;
		if (movieSys == NULL)
		{
			// AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::PlaySequence: There is no system");
		}
		else
		{
			movieSys->PlaySequence(sequenceName.c_str(), NULL, true, false);
		}

    }

    bool StopSequence(const AZStd::string& sequenceName)
    {
   		IMovieSystem* movieSys = gEnv->pMovieSystem;
		if (movieSys == NULL)
		{
			// AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::PlaySequence: There is no system");
		}
		else
		{
			return movieSys->StopSequence(sequenceName.c_str());
		}
		return false;
    }

    bool IsOnNavMesh(const AZ::Vector3& pos)
    {
        INavigationSystem* navSystem = gEnv->pAISystem->GetNavigationSystem();
        NavigationAgentTypeID agentType = navSystem->GetAgentTypeID("MediumSizedCharacters");
        bool isValid = false;
        if (agentType)
            isValid = navSystem->IsLocationValidInNavigationMesh(agentType, AZVec3ToLYVec3(pos));
        else
            AZ_Warning("StarterGame", false, "%s: Invalid agent type.", __FUNCTION__);

        return isValid;
    }

    bool GetClosestPointInNavMesh(const AZ::Vector3& pos, AZ::Vector3& found)
    {
        if (IsOnNavMesh(pos))
        {
            found = pos;
            return true;
        }

        INavigationSystem* navSystem = gEnv->pAISystem->GetNavigationSystem();
        NavigationAgentTypeID agentType = navSystem->GetAgentTypeID("MediumSizedCharacters");
        AZ_Warning("StarterGame", agentType, "%s: Invalid agent type.", __FUNCTION__);

        Vec3 closestPoint(0.0f, 0.0f, 0.0f);
        if (!navSystem->GetClosestPointInNavigationMesh(agentType, AZVec3ToLYVec3(pos), 3.0f, 3.0f, &closestPoint))
        {
            //AZ_Warning("StarterGame", false, "%s: Failed to find the closest point.", __FUNCTION__);
            found = AZ::Vector3::CreateZero();
            return false;
        }

        found = LYVec3ToAZVec3(closestPoint);
        return true;
    }

    void StarterGameUtility::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection);
        if (behaviorContext)
        {
            behaviorContext->Class<StarterGameUtility>("StarterGameUtility")
                ->Method("FindClosestFromTag", &FindClosestFromTag)
                ->Method("randomF", &randomF)
                ->Method("ArcTan2", &atan2f)
                ->Method("GetSurfaceType", &GetSurfaceType)
                ->Method("GetSurfaceIndexFromString", &GetSurfaceIndexFromString)
                ->Method("GetSurfaceNameFromId", &GetSurfaceNameFromId)
                ->Method("GetParentEntity", &GetParentEntity)
                ->Method("GetEntityName", &GetEntityName)
                ->Method("EntityHasTag", &EntityHasTag)
                ->Method("RestartLevel", &RestartLevel)
                ->Method("IsGameStarted", &IsGameStarted)
                ->Method("GetElementPosOnScreen", &GetElementPosOnScreen)
                ->Method("PlaySequence", &PlaySequence)
                ->Method("StopSequence", &StopSequence)
                ->Method("UIFaderControl", &UIFaderControl)
                ->Method("UIScrollControl", &UIScrollControl)
                ->Method("UISliderControl", &UISliderControl)
                ->Method("UIElementMover", &UIElementMover)
	            ->Method("UIElementScaler", &UIElementScaler)
	            ->Method("UIElementRotator", &UIElementRotator)
				->Method("UITextSetter", &UITextSetter)
                ->Method("IsOnNavMesh", &IsOnNavMesh)
                ->Method("GetClosestPointInNavMesh", &GetClosestPointInNavMesh)
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
        }

    }

}
