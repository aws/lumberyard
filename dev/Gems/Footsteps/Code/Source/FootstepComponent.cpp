/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
*
*/

#include "Footsteps_precompiled.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include "FootstepComponent.h"
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
#include <Cry3DEngine/Environment/OceanEnvironmentBus.h>

namespace Footsteps
{
    void FootstepComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<FootstepComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<FootstepComponent>("Footsteps", "Provides Footsteps from animation events sent to the material effects system.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Game")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ;
            }
        }
    }

    void FootstepComponent::Init()
    {
    }

    void FootstepComponent::OnAnimationEvent(const LmbrCentral::AnimationEvent& event)
    {
        if (event.m_eventName[0] && _stricmp(event.m_eventName, "footstep") == 0)
        {
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
                if (event.m_boneName1[0])
                {
                    uint32 boneID = character->GetIDefaultSkeleton().GetJointIDByName(event.m_boneName1);
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
            AZStd::string effectName = "default";

            float feetWaterLevel = OceanToggle::IsActive() ? OceanRequest::GetWaterLevel(params.pos) : gEnv->p3DEngine->GetWaterLevel(&params.pos);
            const float deepWaterLevel = 1.0;
            bool usingWaterEffectId = false;


            if (event.m_parameter[0])
            {
                effectName = event.m_parameter;
            }

            if (feetWaterLevel != WATER_LEVEL_UNKNOWN)
            {
                const float depth = feetWaterLevel - params.pos.z;
                if (depth >= 0.0f)
                {
                    usingWaterEffectId = true;

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
                }
            }

            if (usingWaterEffectId)
            {
                effectId = pMaterialEffects->GetEffectIdByName(fxLibName.c_str(), effectName.c_str());
				if (effectId == InvalidEffectId)
                {
                    AZ_Warning("FootstepComponent", false, "Failed to find material for footstep sounds in FXLib %s, Effect: %s", fxLibName.c_str(), effectName.c_str());
                }
            }
            else
            {
                // get the ground material idx
                pe_status_living livingStatus;
                livingStatus.groundSlope.Set(0.f, 0.f, 1.f);
                LmbrCentral::CryPhysicsComponentRequestBus::Event(GetEntityId(), &LmbrCentral::CryPhysicsComponentRequestBus::Events::GetPhysicsStatus, livingStatus);

                if (livingStatus.groundSurfaceIdx >= 0)  // Filter out the surface id -1 as this means the entity was not in contact with the ground
                {
                    effectId = pMaterialEffects->GetEffectId(fxLibName.c_str(), livingStatus.groundSurfaceIdx);
                    if (effectId == InvalidEffectId)
                    {
                        // In non release builds, make sure that we don't spam the log output by only warning once per surface
#if defined( AZ_ENABLE_TRACING )
                        static AZStd::vector<int> s_surfacesWarnedAbout;
                        if (AZStd::find(s_surfacesWarnedAbout.begin(), s_surfacesWarnedAbout.end(), livingStatus.groundSurfaceIdx) == s_surfacesWarnedAbout.end())
                        {
                            AZ_Warning("FootstepComponent", false, "Failed to find material for footstep sounds in FXLib %s, SurfaceIdx: %d", fxLibName.c_str(), livingStatus.groundSurfaceIdx);
                            s_surfacesWarnedAbout.push_back(livingStatus.groundSurfaceIdx);
                        }
#else
                        AZ_Warning("FootstepComponent", false, "Failed to find material for footstep sounds in FXLib %s, SurfaceIdx: %d", fxLibName.c_str(), livingStatus.groundSurfaceIdx);
#endif
                    }
                }
            }

            if (effectId != InvalidEffectId)
            {
                pMaterialEffects->ExecuteEffect(effectId, params);
            }
        }
    }

    void FootstepComponent::Activate()
    {
        LmbrCentral::CharacterAnimationNotificationBus::Handler::BusConnect(GetEntityId());
    }

    void FootstepComponent::Deactivate()
    {
        LmbrCentral::CharacterAnimationNotificationBus::Handler::BusDisconnect();
    }
}
