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
#include "VisualiseAIStatesSystemComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContext.h>

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/MathUtils.h>

#include <IRenderAuxGeom.h>
#include <MathConversion.h>


namespace StarterGameGem
{
    // Keep this enum in sync with the 'g_aiStateColours' array.
    enum AIStates
    {
        AI_Unknown = 0,
        // DO NOT MODIFY ABOVE HERE

        AI_Idle,
        AI_Suspicious,
        AI_Combat,
        AI_Tracking,

        // DO NOT MODIFY BELOW HERE
        AI_Count,
    };

    // Keep this array in sync with the 'AIStates' enum.
    static const ColorB gsc_aiStateColours[] =
    {
        ColorB(255, 255, 255, 255),     // Unknown
        ColorB(0, 255, 0, 255),         // Idle
        ColorB(255, 204, 0, 255),       // Suspicious
        ColorB(255, 0, 0, 255),         // Combat
        ColorB(153, 102, 0, 255),       // Tracking
        // 'Count' shouldn't need a colour.
    };

    class AIStatesHolder
    {
    public:
        AZ_TYPE_INFO(AIStatesHolder, "{F23F3105-3421-4E28-94F3-4073333E1F5B}");
        AZ_CLASS_ALLOCATOR(AIStatesHolder, AZ::SystemAllocator, 0);

        AIStatesHolder() = default;
        ~AIStatesHolder() = default;
    };

	VisualiseAIStatesSystemComponent* g_vaissc_instance = nullptr;

	VisualiseAIStatesSystemComponent* VisualiseAIStatesSystemComponent::GetInstance()
	{ 
		return g_vaissc_instance;
	}

	void VisualiseAIStatesSystemComponent::Reflect(AZ::ReflectContext* context)
	{
		if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
		{
			serializeContext->Class<VisualiseAIStatesSystemComponent, AZ::Component>()
				->Version(1)
				->SerializerForEmptyClass()
			;

			if (AZ::EditContext* editContext = serializeContext->GetEditContext())
			{
				editContext->Class<VisualiseAIStatesSystemComponent>("Visualise A.I. States System", "Renders visualisations to indicate each A.I. state")
					->ClassElement(AZ::Edit::ClassElements::EditorData, "")
						->Attribute(AZ::Edit::Attributes::Category, "Game")
						->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
					;
			}
		}

		if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
		{
            // AIStates constants.
            // IMPORTANT: requires an empty class 'AIStatesHolder'
            behaviorContext->Class<AIStatesHolder>("AIStates")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Constant("Unknown", BehaviorConstant(AIStates::AI_Unknown))
                ->Constant("Idle", BehaviorConstant(AIStates::AI_Idle))
                ->Constant("Suspicious", BehaviorConstant(AIStates::AI_Suspicious))
                ->Constant("Combat", BehaviorConstant(AIStates::AI_Combat))
                ->Constant("Tracking", BehaviorConstant(AIStates::AI_Tracking))
                ;

			behaviorContext->EBus<VisualiseAIStatesSystemRequestBus>("VisualiseAIStatesSystemRequestBus")
                ->Event("SetAIState", &VisualiseAIStatesSystemRequestBus::Events::SetAIState)
                ->Event("ClearAIState", &VisualiseAIStatesSystemRequestBus::Events::ClearAIState)
				;
		}
	}

	void VisualiseAIStatesSystemComponent::Activate()
	{
		VisualiseAIStatesSystemRequestBus::Handler::BusConnect();
		AZ::TickBus::Handler::BusConnect();

        g_vaissc_instance = this;
	}

	void VisualiseAIStatesSystemComponent::Deactivate()
	{
		VisualiseAIStatesSystemRequestBus::Handler::BusDisconnect();
		//AZ::TickBus::Handler::BusDisconnect();

        g_vaissc_instance = nullptr;
	}

	void VisualiseAIStatesSystemComponent::OnTick(float deltaTime, AZ::ScriptTimePoint time)
	{
		IRenderAuxGeom* renderAuxGeom = gEnv->pRenderer->GetIRenderAuxGeom();
		SAuxGeomRenderFlags flags;
		flags.SetAlphaBlendMode(e_AlphaNone);
		flags.SetCullMode(e_CullModeBack);
		flags.SetDepthTestFlag(e_DepthTestOn);
		flags.SetFillMode(e_FillModeSolid);
		renderAuxGeom->SetRenderFlags(flags);

		static ICVar* const cvarDraw = gEnv->pConsole->GetCVar("ai_debugDrawAIStates");
		if (cvarDraw)
		{
			if (cvarDraw->GetIVal() > 0)
			{
				Vec3 riser = Vec3(0.0f, 0.0, 2.0f);
				for (AZStd::list<AIState>::const_iterator it = m_states.begin(); it != m_states.end(); ++it)
                {
                    AZ::Transform tm = AZ::Transform::CreateIdentity();
                    AZ::TransformBus::EventResult(tm, it->m_entityId, &AZ::TransformInterface::GetWorldTM);

                    renderAuxGeom->DrawSphere(AZVec3ToLYVec3(tm.GetTranslation()) + riser, 0.25f, gsc_aiStateColours[it->m_state]);
				}
			}
		}
	}

    void VisualiseAIStatesSystemComponent::SetAIState(const AZ::EntityId& entityId, int state)
    {
        // Clamp the state so we can correctly colour it.
        if (state >= AIStates::AI_Count || state < AIStates::AI_Unknown)
        {
            state = AIStates::AI_Unknown;
        }

        AIState* const aiState = FindState(entityId);
        if (aiState == nullptr)
        {
            AIState newState;
            newState.m_entityId = entityId;
            newState.m_state = state;
            m_states.push_back(newState);
        }
        else
        {
            aiState->m_state = state;
        }
    }

    void VisualiseAIStatesSystemComponent::ClearAIState(const AZ::EntityId& entityId)
    {
        for (AZStd::list<AIState>::iterator it = m_states.begin(); it != m_states.end(); ++it)
        {
            if (it->m_entityId == entityId)
            {
                m_states.erase(it);
                break;
            }
        }
    }

    VisualiseAIStatesSystemComponent::AIState* const VisualiseAIStatesSystemComponent::FindState(const AZ::EntityId& entityId)
    {
        AZStd::list<AIState>::iterator it;
        for (it = m_states.begin(); it != m_states.end(); ++it)
        {
            if (it->m_entityId == entityId)
                break;
        }

        if (it == m_states.end())
            return nullptr;
        else
            return &(*it);
    }

} // namespace StarterGameGem


