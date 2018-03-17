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
#include "VisualiseRangeComponent.h"

#include "StarterGameUtility.h"

#include <IRenderAuxGeom.h>
#include <MathConversion.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace StarterGameGem
{

	static const float s_alpha = 100.0f;


	ColorB AZVec4ToColorB(const AZ::Vector4& vec)
	{
		return ColorB((uint8)vec.GetX(), (uint8)vec.GetY(), (uint8)vec.GetZ(), (uint8)vec.GetW());
	}


	void VisualiseRangeComponent::Init()
	{
		m_suspicionRange = m_suspicionFoV = m_suspicionRearRange = -1.0f;
		m_colourSuspicion = AZ::Vector4(255.0f, 255.0f, 130.0f, s_alpha);

		m_rangeSight = m_rangeAggro = -1.0f;
		m_colourCombat = AZ::Vector4(255.0f, 0.0f, 0.0f, s_alpha);
	}

	void VisualiseRangeComponent::Activate()
	{
        m_currentEntityTransform = AZ::Transform::CreateIdentity();
        EBUS_EVENT_ID_RESULT(m_currentEntityTransform, GetEntityId(), AZ::TransformBus, GetWorldTM);
		
		AZ::TickBus::Handler::BusConnect();
        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
		VisualiseRangeComponentRequestsBus::Handler::BusConnect(GetEntityId());
	}

	void VisualiseRangeComponent::Deactivate()
	{
        AZ::TransformNotificationBus::Handler::BusDisconnect();
		VisualiseRangeComponentRequestsBus::Handler::BusDisconnect();
	}

	void VisualiseRangeComponent::Reflect(AZ::ReflectContext* reflection)
	{
		AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
		if (serializeContext)
		{
			serializeContext->Class<VisualiseRangeComponent>()
				->Version(1)
				// For suspicion.
				->Field("SuspicionRange", &VisualiseRangeComponent::m_suspicionRange)
				->Field("SuspicionFoV", &VisualiseRangeComponent::m_suspicionFoV)
				->Field("SuspicionRearRange", &VisualiseRangeComponent::m_suspicionRearRange)
				->Field("ColourSuspicion", &VisualiseRangeComponent::m_colourSuspicion)
				// For combat.
				->Field("RangeSight", &VisualiseRangeComponent::m_rangeSight)
				->Field("RangeAggro", &VisualiseRangeComponent::m_rangeAggro)
				->Field("ColourCombat", &VisualiseRangeComponent::m_colourCombat)
			;

			AZ::EditContext* editContext = serializeContext->GetEditContext();
			if (editContext)
			{
				editContext->Class<VisualiseRangeComponent>("VisualiseRange", "Allows visualisation of conceptual things at runtime.")
					->ClassElement(AZ::Edit::ClassElements::EditorData, "")
						->Attribute(AZ::Edit::Attributes::Category, "AI")
						->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/SG_Icon.png")
						->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/SG_Icon.dds")
						->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
					//->DataElement(0, &VisualiseRangeComponent::m_colour, "Colour", "The colour of the visualisations.")
				;
			}
		}

		if (AZ::BehaviorContext* behavior = azrtti_cast<AZ::BehaviorContext*>(reflection))
		{
			behavior->EBus<VisualiseRangeComponentRequestsBus>("VisualiseRangeComponentRequestsBus")
				->Event("SetSightRange", &VisualiseRangeComponentRequestsBus::Events::SetSightRange)
				->Event("SetSuspicionRange", &VisualiseRangeComponentRequestsBus::Events::SetSuspicionRange)
				;
		}
	}

    void VisualiseRangeComponent::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
        if (!GetEntityId().IsValid())
            return;

		IRenderAuxGeom* renderAuxGeom = gEnv->pRenderer->GetIRenderAuxGeom();
		SAuxGeomRenderFlags flags;
		flags.SetAlphaBlendMode(e_AlphaBlended);
		flags.SetCullMode(e_CullModeNone);
		flags.SetDepthTestFlag(e_DepthTestOn);
		flags.SetFillMode(e_FillModeWireframe);		// render them all as wireframe otherwise we can't see the inner shapes
		renderAuxGeom->SetRenderFlags(flags);

        AZ::Transform tm = StarterGameUtility::GetJointWorldTM(GetEntityId(), "head");
		Vec3 pos = AZVec3ToLYVec3(tm.GetPosition());
		Vec3 forward = AZVec3ToLYVec3(tm.GetColumn(1).GetNormalized());

		static ICVar* const cvarSight = gEnv->pConsole->GetCVar("ai_debugDrawSightRange");
		if (cvarSight && m_rangeSight > 0.0f && m_rangeAggro > 0.0f)
		{
			if (cvarSight->GetIVal() != 0)
			{
				// The aggro sight range.
				renderAuxGeom->DrawSphere(pos, m_rangeAggro, AZVec4ToColorB(m_colourCombat));	// should really be a cone

				// The maximum sight range.
				renderAuxGeom->DrawSphere(pos, m_rangeSight, ColorB(255, 255, 255, (uint8)s_alpha));
			}
		}

		static ICVar* const cvarSuspicion = gEnv->pConsole->GetCVar("ai_debugDrawSuspicionRange");
		if (cvarSuspicion && m_suspicionRange > 0.0f && m_suspicionFoV > 0.0f && m_suspicionRearRange > 0.0f)
		{
			if (cvarSuspicion->GetIVal() != 0)
			{
				// The suspicion range in front of the character.
				f32 sinFoV, cosFoV;
				sincos_tpl(m_suspicionFoV, &sinFoV, &cosFoV);
				cosFoV *= m_suspicionRange;
				renderAuxGeom->DrawCone(pos + (forward * cosFoV), -forward, m_suspicionRange * sinFoV, cosFoV, AZVec4ToColorB(m_colourSuspicion));

				// The minimum suspicion range (activate around the entire character).
				renderAuxGeom->DrawSphere(pos, m_suspicionRearRange, AZVec4ToColorB(m_colourSuspicion));
			}
		}
    }

    void VisualiseRangeComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_currentEntityTransform = world;
    }

	void VisualiseRangeComponent::SetSightRange(float aggroRange, float sightRange)
	{
		m_rangeSight = sightRange;
		m_rangeAggro = aggroRange;
	}

	void VisualiseRangeComponent::SetSuspicionRange(float range, float fov, float rearRange)
	{
		m_suspicionRange = range;
		m_suspicionFoV = DEG2RAD(fov * 0.5f);
		m_suspicionRearRange = rearRange;
	}
}
