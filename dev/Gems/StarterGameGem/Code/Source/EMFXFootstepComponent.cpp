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
                ->Version(1)
                ->Field("Left Foot Event Name", &EMFXFootstepComponent::m_leftFootEventName)
                ->Field("Right Foot Event Name", &EMFXFootstepComponent::m_rightFootEventName)
                ->Field("Left Foot Bone Name", &EMFXFootstepComponent::m_leftFootBoneName)
                ->Field("Right Foot Bone Name", &EMFXFootstepComponent::m_rightFootBoneName)
                ->Field("Default FXLib", &EMFXFootstepComponent::m_defaultFXLib)
            ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<EMFXFootstepComponent>("EMFXFootsteps", "Provides Footsteps from EMFX animation events sent to the material effects system.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Game")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->DataElement("Left Foot Event Name", &EMFXFootstepComponent::m_leftFootEventName, "Left Foot Event", "Name of the left foot motion event")
                    ->DataElement("Right Foot Event Name", &EMFXFootstepComponent::m_rightFootEventName, "Right Foot Event", "Name of the right foot motion event")
                    ->DataElement("Left Foot Bone Name", &EMFXFootstepComponent::m_leftFootBoneName, "Left Foot Bone", "Name of the left foot bone")
                    ->DataElement("Right Foot Bone Name", &EMFXFootstepComponent::m_rightFootBoneName, "Right Foot Bone", "Name of the right foot bone")
                    ->DataElement("Default FXLib", &EMFXFootstepComponent::m_defaultFXLib, "Default FX Lib", "The name of the default FX Lib to use")
                ;
            }
        }
    }

    void EMFXFootstepComponent::OnFootstepEvent(const AZStd::string_view eventName, const AZStd::string_view fxLib)
    {
        if (eventName.empty())
        {
            return;
        }

        IMaterialEffects* pMaterialEffects = gEnv->pMaterialEffects;
        if (!pMaterialEffects)
        {
            return;
        }

        // Physics is necessary to get the surface the character is standing on
        // NOTE: PhysX not currently supported
        bool physEnabled = false;
        AzFramework::PhysicsComponentRequestBus::EventResult(physEnabled, GetEntityId(), &AzFramework::PhysicsComponentRequestBus::Events::IsPhysicsEnabled);
        if (!physEnabled)
        {
            return;
        }

        // get the bone name based on the event name e.g. LeftFoot, RightFoot
        AZStd::string_view boneName;
        if (azstricmp(eventName.data(), m_leftFootEventName.c_str()) == 0)
        {
            boneName = m_leftFootBoneName;
        }
        else if (azstricmp(eventName.data(), m_rightFootEventName.c_str()) == 0)
        {
            boneName = m_rightFootBoneName;
        }
        else
        {
            return;
        }

        Vec3 jointOffset(ZERO);

        if (boneName.data())
        {
            AZ::s32 jointIndex = -1;
            LmbrCentral::SkeletalHierarchyRequestBus::EventResult(jointIndex, GetEntityId(), &LmbrCentral::SkeletalHierarchyRequestBus::Events::GetJointIndexByName, boneName.data());
            if (jointIndex >= 0)
            {
                AZ::Transform characterRelativeTransform;
                LmbrCentral::SkeletalHierarchyRequestBus::EventResult(characterRelativeTransform, GetEntityId(), &LmbrCentral::SkeletalHierarchyRequestBus::Events::GetJointTransformCharacterRelative, jointIndex);
                jointOffset = AZVec3ToLYVec3(characterRelativeTransform.GetPosition());
            }
        }
        else
        {
            AZ_WarningOnce("EMFXFootstepComponent", false, "Missing bone name for footsteps events in %s, effects will play from entity origin", __FUNCTION__);
        }

        AZ::Transform entityTransform;
        AZ::TransformBus::EventResult(entityTransform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
        Matrix34 tm = AZTransformToLYTransform(entityTransform);
        tm.OrthonormalizeFast();
        Ang3 angles = Ang3::GetAnglesXYZ(tm);

        // Setup FX params
        SMFXRunTimeEffectParams params;
        params.angle = angles.z + (gf_PI * 0.5f);
        params.pos = params.decalPos = tm * jointOffset;

        TMFXEffectId effectId = InvalidEffectId;

        const float feetWaterLevel = gEnv->p3DEngine->GetWaterLevel(&params.pos);
        const float deepWaterLevel = 1.0;
        const float depth = feetWaterLevel - params.pos.z;

        const AZStd::string_view fxLibName = fxLib.empty() ? AZStd::string_view(m_defaultFXLib) : fxLib;

        if (feetWaterLevel != WATER_LEVEL_UNKNOWN && depth >= 0.0f)
        {
            const AZStd::string_view effectName = depth > deepWaterLevel ? "water_deep" : "water_shallow";
            const float hitScale = 1.f;
            const float hitStrength = 0.f;

            // Plug water hits directly into water sim
            gEnv->pRenderer->EF_AddWaterSimHit(tm.GetTranslation(), hitScale, hitStrength);

            effectId = pMaterialEffects->GetEffectIdByName(fxLibName.data(), effectName.data());

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION EMFXFOOTSTEPCOMPONENT_CPP_SECTION_1
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/EMFXFootstepComponent_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/EMFXFootstepComponent_cpp_provo.inl"
    #endif
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
            if (effectId == InvalidEffectId)
            {
                gEnv->pSystem->Warning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, VALIDATOR_FLAG_AUDIO, 0, "Failed to find material for footstep sounds in FXLib %s, Effect: %s", fxLibName, effectName);
            }
#endif
        }
        else
        {
            // get the ground material idx
            pe_status_living livingStatus;
            livingStatus.groundSlope.Set(0.f, 0.f, 1.f);
            LmbrCentral::CryPhysicsComponentRequestBus::Event(GetEntityId(), &LmbrCentral::CryPhysicsComponentRequestBus::Events::GetPhysicsStatus, livingStatus);
            if (livingStatus.groundSurfaceIdx >= 0)
            {
                // Don't attempt to play footsteps when the physics system doesn't detect a ground surface.
                effectId = pMaterialEffects->GetEffectId(fxLibName.data(), livingStatus.groundSurfaceIdx);
    #if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION EMFXFOOTSTEPCOMPONENT_CPP_SECTION_2
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/EMFXFootstepComponent_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/EMFXFootstepComponent_cpp_provo.inl"
    #endif
    #endif
    #if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
    #undef AZ_RESTRICTED_SECTION_IMPLEMENTED
    #else
                if (effectId == InvalidEffectId)
                {
                    gEnv->pSystem->Warning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, VALIDATOR_FLAG_AUDIO, 0, "Failed to find material for footstep sounds in FXLib %s, SurfaceIdx: %d", fxLibName, livingStatus.groundSurfaceIdx);
                }
    #endif
            }
        }

        if (effectId != InvalidEffectId)
        {
            pMaterialEffects->ExecuteEffect(effectId, params);
        }
    }

    void EMFXFootstepComponent::OnMotionEvent(EMotionFX::Integration::MotionEvent motionEvent)
    {
        OnFootstepEvent(motionEvent.m_eventTypeName, motionEvent.m_parameter);
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
