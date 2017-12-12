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
#include "WaypointsComponent.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>

#include <IRenderAuxGeom.h>

namespace StarterGameGem
{
	void WaypointsComponent::Init()
	{
		m_currentWaypoint = 0;
	}

	void WaypointsComponent::Activate()
	{
		AZ::TickBus::Handler::BusConnect();
		WaypointsComponentRequestsBus::Handler::BusConnect(GetEntityId());
	}

	void WaypointsComponent::Deactivate()
	{
		WaypointsComponentRequestsBus::Handler::BusDisconnect();
	}

	void WaypointsComponent::Reflect(AZ::ReflectContext* reflection)
	{
		AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
		if (serializeContext)
		{
			serializeContext->Class<WaypointsComponent>()
				->Version(1)
				->Field("IsSentry", &WaypointsComponent::m_isSentry)
				->Field("IsLazySentry", &WaypointsComponent::m_isLazySentry)
				->Field("Waypoints", &WaypointsComponent::m_waypoints)
				->Field("CurrentWaypoint", &WaypointsComponent::m_currentWaypoint)
			;

			AZ::EditContext* editContext = serializeContext->GetEditContext();
			if (editContext)
			{
				editContext->Class<WaypointsComponent>("Waypoints", "Contains a list of waypoints")
					->ClassElement(AZ::Edit::ClassElements::EditorData, "")
					->Attribute(AZ::Edit::Attributes::Category, "AI")
					->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/SG_Icon.png")
					->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/SG_Icon.dds")
					->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
					->DataElement(0, &WaypointsComponent::m_isSentry, "Sentry?", "If ticked, will ignore waypoints and periodically turn on the spot.")
					->DataElement(0, &WaypointsComponent::m_isLazySentry, "Lazy Sentry?", "If ticked, will become sentry that doesn't turn (prioritised over 'Sentry?').")
					->DataElement(0, &WaypointsComponent::m_waypoints, "Waypoints", "A list of waypoints.")
				;
			}
		}

		if (AZ::BehaviorContext* behavior = azrtti_cast<AZ::BehaviorContext*>(reflection))
		{
			behavior->EBus<WaypointsComponentRequestsBus>("WaypointsComponentRequestsBus")
				->Event("GetFirstWaypoint", &WaypointsComponentRequestsBus::Events::GetFirstWaypoint)
				->Event("GetNextWaypoint", &WaypointsComponentRequestsBus::Events::GetNextWaypoint)
				->Event("GetWaypointCount", &WaypointsComponentRequestsBus::Events::GetWaypointCount)
				->Event("IsSentry", &WaypointsComponentRequestsBus::Events::IsSentry)
				->Event("IsLazySentry", &WaypointsComponentRequestsBus::Events::IsLazySentry)
				->Event("CloneWaypoints", &WaypointsComponentRequestsBus::Events::CloneWaypoints)
				;
		}
	}

    void WaypointsComponent::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
		IRenderAuxGeom* renderAuxGeom = gEnv->pRenderer->GetIRenderAuxGeom();
		SAuxGeomRenderFlags flags;
		flags.SetAlphaBlendMode(e_AlphaNone);
		flags.SetCullMode(e_CullModeBack);
		flags.SetDepthTestFlag(e_DepthTestOn);
		flags.SetFillMode(e_FillModeSolid);
		renderAuxGeom->SetRenderFlags(flags);

		static ICVar* const cvarSight = gEnv->pConsole->GetCVar("ai_debugDrawWaypoints");
		if (cvarSight)
		{
			if (cvarSight->GetFVal() != 0 && m_waypoints.size() >= 2)
			{
				AZ::Vector3 raised = AZ::Vector3(0.0f, 0.0, cvarSight->GetFVal());

				Vec3* positions = new Vec3[m_waypoints.size()];
				int i = 0;
				for (i; i < m_waypoints.size(); ++i)
				{
					AZ::Transform tm = AZ::Transform::CreateIdentity();
					AZ::TransformBus::EventResult(tm, m_waypoints[i], &AZ::TransformInterface::GetWorldTM);
					positions[i] = AZVec3ToLYVec3(tm.GetTranslation() + raised);
				}

				renderAuxGeom->DrawPolyline(positions, i, (i > 2), ColorB(255, 255, 130, 255));
			}
		}
    }

	AZ::EntityId WaypointsComponent::GetFirstWaypoint()
	{
		AZ::EntityId res;
		if (m_waypoints.size() > 0)
		{
			res = m_waypoints[0];
			m_currentWaypoint = 0;
		}
		else
		{
			AZ_Assert("AI", "WaypointsComponent: Tried to get the first waypoint but no waypoints exist.");
		}

		return res;
	}

	AZ::EntityId WaypointsComponent::GetCurrentWaypoint()
	{
		return m_waypoints[m_currentWaypoint];
	}

	AZ::EntityId WaypointsComponent::GetNextWaypoint()
	{
		++m_currentWaypoint;

		AZ::EntityId res;
		if (m_currentWaypoint >= m_waypoints.size())
		{
			// This function should reset the waypoint counter.
			res = GetFirstWaypoint();
		}
		else
		{
			res = m_waypoints[m_currentWaypoint];
		}

		return res;
	}

	int WaypointsComponent::GetWaypointCount()
	{
		return m_waypoints.size();
	}

	bool WaypointsComponent::IsSentry()
	{
		return m_isSentry && !m_isLazySentry;
	}

	bool WaypointsComponent::IsLazySentry()
	{
		return m_isLazySentry;
	}

	void WaypointsComponent::CloneWaypoints(const AZ::EntityId& srcEntityId)
	{
		AZ_Assert(srcEntityId.IsValid(), "WaypointsComponent: 'CloneWaypoints()' was given a bad entity ID.");

		bool res = false;
		WaypointsComponentRequestsBus::EventResult(res, srcEntityId, &WaypointsComponentRequestsBus::Events::IsSentry);
		m_isSentry = res;

		WaypointsComponentRequestsBus::EventResult(res, srcEntityId, &WaypointsComponentRequestsBus::Events::IsLazySentry);
		m_isLazySentry = res;

		// Only bother copying the waypoints if we're not a sentry.
		if (!this->IsSentry() && !this->IsLazySentry())
		{
			VecOfEntityIds* waypoints;
			WaypointsComponentRequestsBus::EventResult(waypoints, srcEntityId, &WaypointsComponentRequestsBus::Events::GetWaypoints);
			m_waypoints = *waypoints;

			// Clean up the vector so we don't have empty or invalid entries.
			for (VecOfEntityIds::iterator it = m_waypoints.begin(); it != m_waypoints.end(); )
			{
				if (!(*it).IsValid())
				{
					it = m_waypoints.erase(it);
				}
				else
				{
					++it;
				}
			}

			// If we didn't copy any waypoints then assign ourselves as a lazy sentry.
			if (m_waypoints.size() == 0)
			{
				m_isLazySentry = true;
			}
		}

		// If we don't have any waypoints then use the source entity as our only waypoint. This is
		// purely so that we have somewhere to go back to if we're drawn away for combat.
		if (m_waypoints.size() == 0)
		{
			m_waypoints.push_back(srcEntityId);
		}
	}

	VecOfEntityIds* WaypointsComponent::GetWaypoints()
	{
		return &m_waypoints;
	}

}
