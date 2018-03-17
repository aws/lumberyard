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
#include "WaypointSettingsComponent.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>


namespace StarterGameGem
{
    class WaypointEventTypeHolder
    {
    public:
        AZ_TYPE_INFO(WaypointEventTypeHolder, "{62BC29FC-2936-4857-B413-D5ADED9DC4A1}");
        AZ_CLASS_ALLOCATOR(WaypointEventTypeHolder, AZ::SystemAllocator, 0);

        WaypointEventTypeHolder() = default;
        ~WaypointEventTypeHolder() = default;
    };


    //////////////////////////////////////////////////////////////////////////
    // StarterGameGem::WaypointSettingsComponentRequests::WaypointEventSettings
    //////////////////////////////////////////////////////////////////////////

    WaypointSettingsComponentRequests::WaypointEventSettings::WaypointEventSettings()
        : m_type(WaypointEventType::WET_Invalid)
        , m_duration(0.0f)
        , m_turnTowardsEntity(true)
        , m_animName("")
    {}

    void WaypointSettingsComponentRequests::WaypointEventSettings::Reflect(AZ::ReflectContext* context)
	{
		AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
		if (serializeContext)
		{
            serializeContext->Class<WaypointSettingsComponentRequests::WaypointEventSettings>()
                ->Version(1)
                ->Field("Type", &WaypointSettingsComponentRequests::WaypointEventSettings::m_type)
                ->Field("Duration", &WaypointSettingsComponentRequests::WaypointEventSettings::m_duration)
                ->Field("TurnTowardsEntity", &WaypointSettingsComponentRequests::WaypointEventSettings::m_turnTowardsEntity)
                ->Field("EntityToFace", &WaypointSettingsComponentRequests::WaypointEventSettings::m_entityToFace)
                ->Field("DirectionToFace", &WaypointSettingsComponentRequests::WaypointEventSettings::m_directionToFace)
                ->Field("AnimName", &WaypointSettingsComponentRequests::WaypointEventSettings::m_animName)
            ;

			AZ::EditContext* editContext = serializeContext->GetEditContext();
			if (editContext)
			{
				editContext->Class<WaypointSettingsComponentRequests::WaypointEventSettings>("Event Settings", "Settings for a particular waypoint event")
					->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &WaypointSettingsComponentRequests::WaypointEventSettings::m_type, "Type", "The type of event")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &WaypointSettingsComponentRequests::WaypointEventSettings::MajorPropertyChanged)
                        ->EnumAttribute(WaypointEventType::WET_Wait,        "Wait")
                        ->EnumAttribute(WaypointEventType::WET_TurnToFace,	"Turn To Face")
                        ->EnumAttribute(WaypointEventType::WET_PlayAnim,    "Play Anim")
                        ->EnumAttribute(WaypointEventType::WET_Invalid,     "None")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &WaypointSettingsComponentRequests::WaypointEventSettings::m_duration, "Duration", "How long to wait for")
                        ->Attribute(AZ::Edit::Attributes::Suffix, " s")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &WaypointSettingsComponentRequests::WaypointEventSettings::IsDurationVisible)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &WaypointSettingsComponentRequests::WaypointEventSettings::m_turnTowardsEntity, "Face entity?", "Turn towards an entity or a global direction?")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &WaypointSettingsComponentRequests::WaypointEventSettings::MajorPropertyChanged)
                        ->Attribute(AZ::Edit::Attributes::Visibility, &WaypointSettingsComponentRequests::WaypointEventSettings::IsTurnTowardsEntityVisible)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &WaypointSettingsComponentRequests::WaypointEventSettings::m_entityToFace, "Entity", "The entity to turn towards")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &WaypointSettingsComponentRequests::WaypointEventSettings::IsEntityToFaceVisible)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &WaypointSettingsComponentRequests::WaypointEventSettings::m_directionToFace, "Direction", "The direction to turn towards")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &WaypointSettingsComponentRequests::WaypointEventSettings::IsDirectionToFaceVisible)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &WaypointSettingsComponentRequests::WaypointEventSettings::m_animName, "Animation", "The name of the animation to play")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &WaypointSettingsComponentRequests::WaypointEventSettings::IsAnimNameVisible)
				;
			}
		}

        if (AZ::BehaviorContext* behavior = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behavior->Class<WaypointSettingsComponentRequests::WaypointEventSettings>()
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Property("type", BehaviorValueGetter(&WaypointSettingsComponentRequests::WaypointEventSettings::m_type), nullptr)
                ->Property("duration", BehaviorValueProperty(&WaypointSettingsComponentRequests::WaypointEventSettings::m_duration))
                ->Property("turnTowardsEntity", BehaviorValueGetter(&WaypointSettingsComponentRequests::WaypointEventSettings::m_turnTowardsEntity), nullptr)
                ->Property("entityToFace", BehaviorValueGetter(&WaypointSettingsComponentRequests::WaypointEventSettings::m_entityToFace), nullptr)
                ->Property("directionToFace", BehaviorValueGetter(&WaypointSettingsComponentRequests::WaypointEventSettings::m_directionToFace), nullptr)
                ->Property("animName", BehaviorValueGetter(&WaypointSettingsComponentRequests::WaypointEventSettings::m_animName), nullptr)
                ->Method("IsValid", &WaypointSettingsComponentRequests::WaypointEventSettings::IsValid)
            ;
        }
	}

    AZ::u32 WaypointSettingsComponentRequests::WaypointEventSettings::MajorPropertyChanged()
    {
        return AZ::Edit::PropertyRefreshLevels::EntireTree;
    }

    bool WaypointSettingsComponentRequests::WaypointEventSettings::IsValid() const
    {
        return m_type >= 0 && m_type < WaypointEventType::WET_Invalid;
    }

    bool WaypointSettingsComponentRequests::WaypointEventSettings::IsDurationVisible() const
    {
        return m_type == WaypointEventType::WET_Wait;
    }

    bool WaypointSettingsComponentRequests::WaypointEventSettings::IsTurnTowardsEntityVisible() const
    {
        return m_type == WaypointEventType::WET_TurnToFace;
    }

    bool WaypointSettingsComponentRequests::WaypointEventSettings::IsEntityToFaceVisible() const
    {
        return IsTurnTowardsEntityVisible() && m_turnTowardsEntity;
    }

    bool WaypointSettingsComponentRequests::WaypointEventSettings::IsDirectionToFaceVisible() const
    {
        return IsTurnTowardsEntityVisible() && !m_turnTowardsEntity;
    }

    bool WaypointSettingsComponentRequests::WaypointEventSettings::IsAnimNameVisible() const
    {
        return m_type == WaypointEventType::WET_PlayAnim;
    }


    //////////////////////////////////////////////////////////////////////////
    // StarterGameGem::WaypointSettingsComponent
    //////////////////////////////////////////////////////////////////////////

	void WaypointSettingsComponent::Activate()
	{
		WaypointSettingsComponentRequestsBus::Handler::BusConnect(GetEntityId());
	}

	void WaypointSettingsComponent::Deactivate()
	{
		WaypointSettingsComponentRequestsBus::Handler::BusDisconnect();
	}

	void WaypointSettingsComponent::Reflect(AZ::ReflectContext* context)
	{
        WaypointSettingsComponentRequests::WaypointEventSettings::Reflect(context);

		AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
		if (serializeContext)
		{
			serializeContext->Class<WaypointSettingsComponent>()
				->Version(1)
				->Field("EventsOnArrival", &WaypointSettingsComponent::m_eventsOnArrival)
			;

			AZ::EditContext* editContext = serializeContext->GetEditContext();
			if (editContext)
			{
                editContext->Class<WaypointSettingsComponent>("Waypoint Settings", "Specifies events for a waypoint")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "AI")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/SG_Icon.png")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/SG_Icon.dds")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                    
                    ->DataElement(0, &WaypointSettingsComponent::m_eventsOnArrival, "Arrival", "Events to be performed on arrival.")
                ;
			}
		}

		if (AZ::BehaviorContext* behavior = azrtti_cast<AZ::BehaviorContext*>(context))
		{
            // WaypointEventType constants.
            // IMPORTANT: requires an empty class 'WaypointEventTypeHolder'
            behavior->Class<WaypointEventTypeHolder>("WaypointEventType")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Constant("Wait", BehaviorConstant(WaypointEventType::WET_Wait))
                ->Constant("TurnToFace", BehaviorConstant(WaypointEventType::WET_TurnToFace))
                ->Constant("PlayAnim", BehaviorConstant(WaypointEventType::WET_PlayAnim))
            ;

			behavior->EBus<WaypointSettingsComponentRequestsBus>("WaypointSettingsComponentRequestsBus")
                ->Event("HasEvents", &WaypointSettingsComponentRequestsBus::Events::HasEvents)
                ->Event("GetNextEvent", &WaypointSettingsComponentRequestsBus::Events::GetNextEvent)
			;
		}
	}

    bool WaypointSettingsComponent::HasEvents()
    {
        return m_eventsOnArrival.size() > 0;
    }

    WaypointSettingsComponentRequests::WaypointEventSettings WaypointSettingsComponent::GetNextEvent(const AZ::EntityId& entityId)
    {
        if (!HasEvents())
            return WaypointSettingsComponentRequests::WaypointEventSettings();

        VecOfProgress::iterator it = FindEntitysProgress(entityId);
        if (it == nullptr)
        {
            EntityProgress newSettings(entityId);
            m_progress.push_back(newSettings);
            it = m_progress.end() - 1;
            it->iter = m_eventsOnArrival.begin();
        }
        else
        {
            ++it->iter;
            if (it->iter == m_eventsOnArrival.end())
            {
                m_progress.erase(it);
                return WaypointSettingsComponentRequests::WaypointEventSettings();
            }
        }

        return *it->iter;
    }

    WaypointSettingsComponent::VecOfProgress::iterator WaypointSettingsComponent::FindEntitysProgress(const AZ::EntityId& entityId)
    {
        for (VecOfProgress::iterator it = m_progress.begin(); it != m_progress.end(); ++it)
        {
            if (it->m_entityId == entityId)
            {
                return it;
            }
        }

        return nullptr;
    }

	//AZ::EntityId WaypointSettingsComponent::GetFirstWaypoint()
	//{
	//	AZ::EntityId res;
	//	if (m_waypoints.size() > 0)
	//	{
	//		res = m_waypoints[0];
	//		m_currentWaypoint = 0;
	//	}
	//	else
	//	{
	//		AZ_Assert("AI", "WaypointSettingsComponent: Tried to get the first waypoint but no waypoints exist.");
	//	}

	//	return res;
	//}

	//AZ::EntityId WaypointSettingsComponent::GetCurrentWaypoint()
	//{
	//	return m_waypoints[m_currentWaypoint];
	//}

	//AZ::EntityId WaypointSettingsComponent::GetNextWaypoint()
	//{
	//	++m_currentWaypoint;

	//	AZ::EntityId res;
	//	if (m_currentWaypoint >= m_waypoints.size())
	//	{
	//		// This function should reset the waypoint counter.
	//		res = GetFirstWaypoint();
	//	}
	//	else
	//	{
	//		res = m_waypoints[m_currentWaypoint];
	//	}

	//	return res;
	//}

	//int WaypointSettingsComponent::GetWaypointCount()
	//{
	//	return m_waypoints.size();
	//}

	//bool WaypointSettingsComponent::IsSentry()
	//{
	//	return m_isSentry && !m_isLazySentry;
	//}

	//bool WaypointSettingsComponent::IsLazySentry()
	//{
	//	return m_isLazySentry;
	//}

	//void WaypointSettingsComponent::CloneWaypoints(const AZ::EntityId& srcEntityId)
	//{
	//	AZ_Assert(srcEntityId.IsValid(), "WaypointSettingsComponent: 'CloneWaypoints()' was given a bad entity ID.");

	//	bool res = false;
	//	WaypointSettingsComponentRequestsBus::EventResult(res, srcEntityId, &WaypointSettingsComponentRequestsBus::Events::IsSentry);
	//	m_isSentry = res;

	//	WaypointSettingsComponentRequestsBus::EventResult(res, srcEntityId, &WaypointSettingsComponentRequestsBus::Events::IsLazySentry);
	//	m_isLazySentry = res;

	//	// Only bother copying the waypoints if we're not a sentry.
	//	if (!this->IsSentry() && !this->IsLazySentry())
	//	{
	//		VecOfEntityIds* waypoints;
	//		WaypointSettingsComponentRequestsBus::EventResult(waypoints, srcEntityId, &WaypointSettingsComponentRequestsBus::Events::GetWaypoints);
	//		m_waypoints = *waypoints;

	//		// Clean up the vector so we don't have empty or invalid entries.
	//		for (VecOfEntityIds::iterator it = m_waypoints.begin(); it != m_waypoints.end(); )
	//		{
	//			if (!(*it).IsValid())
	//			{
	//				it = m_waypoints.erase(it);
	//			}
	//			else
	//			{
	//				++it;
	//			}
	//		}

	//		// If we didn't copy any waypoints then assign ourselves as a lazy sentry.
	//		if (m_waypoints.size() == 0)
	//		{
	//			m_isLazySentry = true;
	//		}
	//	}

	//	// If we don't have any waypoints then use the source entity as our only waypoint. This is
	//	// purely so that we have somewhere to go back to if we're drawn away for combat.
	//	if (m_waypoints.size() == 0)
	//	{
	//		m_waypoints.push_back(srcEntityId);
	//	}
	//}

	//VecOfEntityIds* WaypointSettingsComponent::GetWaypoints()
	//{
	//	return &m_waypoints;
	//}

}
