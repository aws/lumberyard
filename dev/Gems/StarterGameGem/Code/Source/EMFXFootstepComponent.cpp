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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include "EMFXFootstepComponent.h"
#include <ISystem.h>
#include <functor.h>
#include "CryFixedArray.h"
#include "IMaterialEffects.h"
#include "ICryAnimation.h"
#include <LmbrCentral/Rendering/MeshComponentBus.h>
#include <LmbrCentral/Physics/CryCharacterPhysicsBus.h>
#include <LmbrCentral/Physics/CryPhysicsComponentRequestBus.h>
#include <AzFramework/Physics/PhysicsComponentBus.h>
#include <AzCore/Component/TransformBus.h>
#include <MathConversion.h>

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define EMFXFOOTSTEPCOMPONENT_CPP_SECTION_1 1
#define EMFXFOOTSTEPCOMPONENT_CPP_SECTION_2 2
#endif

namespace StarterGameGem
{
    void EMFXFootstepComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<EMFXFootstepComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<EMFXFootstepComponent>("EMFXFootsteps", "Provides Footsteps from EMFX animation events sent to the material effects system.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Game")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ;
            }
        }
    }

    void EMFXFootstepComponent::Init()
    {
    }

	void EMFXFootstepComponent::OnFootstepEvent(const char* inEventName, const char* inBoneName, const char* inFxLibName)
	{
		bool footstepEvent = inEventName && _stricmp(inEventName, "footstep") == 0;
		if (!footstepEvent)
		{
			return;
		}

		bool physEnabled = false;
		AzFramework::PhysicsComponentRequestBus::EventResult(physEnabled, GetEntityId(), &AzFramework::PhysicsComponentRequestBus::Events::IsPhysicsEnabled);
		if (!physEnabled)
		{
			return;
		}

		IMaterialEffects* pMaterialEffects = gEnv->pMaterialEffects;
		if (!pMaterialEffects)
		{
			return;
		}

		// get the character instance
		ICharacterInstance* character = nullptr;
		LmbrCentral::SkinnedMeshComponentRequestBus::EventResult(character, GetEntityId(), &LmbrCentral::SkinnedMeshComponentRequestBus::Events::GetCharacterInstance);

		float relativeSpeed = 0.f;

		QuatT jointTransform(IDENTITY);
		if (character)
		{
			const float speedScaleFactor = 0.142f;
			relativeSpeed = character->GetISkeletonAnim()->GetCurrentVelocity().GetLength() * speedScaleFactor;
			if (inBoneName[0])
			{
				uint32 boneID = character->GetIDefaultSkeleton().GetJointIDByName(inBoneName);
				jointTransform = character->GetISkeletonPose()->GetAbsJointByID(boneID);
			}
		}

		// @TODO use this entity's existing audio trigger?
		// params.audioComponentEntityId = m_pEntity->GetId();
		AZ::Transform entityTransform;
		AZ::TransformBus::EventResult(entityTransform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
		Matrix34 tm = AZTransformToLYTransform(entityTransform);
		tm.OrthonormalizeFast();
		Ang3 angles = Ang3::GetAnglesXYZ(tm);

		// Setup FX params
		SMFXRunTimeEffectParams params;
		params.angle = angles.z + (gf_PI * 0.5f);
		params.pos = params.decalPos = tm * jointTransform.t;
		params.audioComponentOffset = tm.GetInverted().TransformVector(params.pos - tm.GetTranslation());

		TMFXEffectId effectId = InvalidEffectId;
		AZStd::string fxLibName = "footstep";

		const float feetWaterLevel = gEnv->p3DEngine->GetWaterLevel(&params.pos);
		const float deepWaterLevel = 1.0;
		const float depth = feetWaterLevel - params.pos.z;

		if (inFxLibName[0])
		{
			fxLibName = inFxLibName;
		}

		if (feetWaterLevel != WATER_LEVEL_UNKNOWN && depth >= 0.0f)
		{
			AZStd::string effectName = "default";

			// Plug water hits directly into water sim
			gEnv->pRenderer->EF_AddWaterSimHit(tm.GetTranslation(), 1.0f, relativeSpeed * 2.f);

			// trigger water splashes from here
			if (depth > deepWaterLevel)
			{
				effectName = "water_deep";
			}
			else
			{
				effectName = "water_shallow";
			}

			effectId = pMaterialEffects->GetEffectIdByName(fxLibName.c_str(), effectName.c_str());

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION EMFXFOOTSTEPCOMPONENT_CPP_SECTION_1
#include AZ_RESTRICTED_FILE(EMFXFootstepComponent_cpp, AZ_RESTRICTED_PLATFORM)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
			if (effectId == InvalidEffectId)
			{
				gEnv->pSystem->Warning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, VALIDATOR_FLAG_AUDIO, 0, "Failed to find material for footstep sounds in FXLib %s, Effect: %s", fxLibName.c_str(), effectName);
			}
#endif
		}
		else
		{
			// get the ground material idx
			pe_status_living livingStatus;
			livingStatus.groundSlope.Set(0.f, 0.f, 1.f);
			LmbrCentral::CryPhysicsComponentRequestBus::Event(GetEntityId(), &LmbrCentral::CryPhysicsComponentRequestBus::Events::GetPhysicsStatus, livingStatus);
			effectId = pMaterialEffects->GetEffectId(fxLibName.c_str(), livingStatus.groundSurfaceIdx);
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION EMFXFOOTSTEPCOMPONENT_CPP_SECTION_2
#include AZ_RESTRICTED_FILE(EMFXFootstepComponent_cpp, AZ_RESTRICTED_PLATFORM)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
			if (effectId == InvalidEffectId)
			{
				gEnv->pSystem->Warning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, VALIDATOR_FLAG_AUDIO, 0, "Failed to find material for footstep sounds in FXLib %s, SurfaceIdx: %d", fxLibName.c_str(), livingStatus.groundSurfaceIdx);
			}
#endif
		}

		if (effectId != InvalidEffectId)
		{
			pMaterialEffects->ExecuteEffect(effectId, params);
		}

	}

	void EMFXFootstepComponent::OnMotionEvent(EMotionFX::Integration::MotionEvent motionEvent)
	{
		OnFootstepEvent(motionEvent.m_eventTypeName, "", motionEvent.m_parameter);
	}
	void EMFXFootstepComponent::Activate()
    {
		EMotionFX::Integration::ActorNotificationBus::Handler::BusConnect(GetEntityId());
	}

    void EMFXFootstepComponent::Deactivate()
    {
		EMotionFX::Integration::ActorNotificationBus::Handler::BusDisconnect();
	}
}
