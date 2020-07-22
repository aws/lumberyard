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
#include <AzFramework/Terrain/TerrainDataRequestBus.h>

#include <AzFramework/Physics/RigidBody.h>
#include <AzFramework/Physics/TerrainBus.h>
#include <AzFramework/Physics/Casts.h>
#include <AzFramework/Physics/Material.h>
#include <AzFramework/Physics/Shape.h>

namespace Footsteps
{
    void FootstepComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<FootstepComponent, AZ::Component>()
                ->Version(1)
                ->Field("Left Foot Event Name", &FootstepComponent::m_leftFootEventName)
                ->Field("Right Foot Event Name", &FootstepComponent::m_rightFootEventName)
                ->Field("Default FXLib", &FootstepComponent::m_defaultFXLib)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<FootstepComponent>("Footsteps", "Provides Footsteps from animation events sent to the material effects system.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Game")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->DataElement("Left Foot Event Name", &FootstepComponent::m_leftFootEventName, "Left Foot Event", "Name of the left foot motion event")
                    ->DataElement("Right Foot Event Name", &FootstepComponent::m_rightFootEventName, "Right Foot Event", "Name of the right foot motion event")
                    ->DataElement("Default FXLib", &FootstepComponent::m_defaultFXLib, "Default FX Lib", "The name of the default FX Lib to use")
                    ;
            }
        }
    }

    void FootstepComponent::OnAnimationEvent(const LmbrCentral::AnimationEvent& event)
    {
        if (event.m_eventName[0] && azstricmp(event.m_eventName, "footstep") == 0)
        {
            bool physEnabled = false;
            AzFramework::PhysicsComponentRequestBus::EventResult(physEnabled, GetEntityId(), &AzFramework::PhysicsComponentRequestBus::Events::IsPhysicsEnabled);
            if (!physEnabled)
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

            AZStd::string fxLibName = "footstep";
            AZStd::string effectName = "default";

            if (event.m_parameter[0])
            {
                effectName = event.m_parameter;
            }

            TMFXEffectId effectId = CheckForWaterEffect(fxLibName, LYVec3ToAZVec3(params.pos), entityTransform, relativeSpeed);
            if (effectId == InvalidEffectId)
            {
                effectId = CheckForLegacySurfaceEffect(fxLibName);
            }

            if (effectId != InvalidEffectId)
            {
                if (IMaterialEffects* pMaterialEffects = gEnv->pMaterialEffects)
                {
                    pMaterialEffects->ExecuteEffect(effectId, params);
                }
            }
        }
    }

    void FootstepComponent::OnMotionEvent(EMotionFX::Integration::MotionEvent motionEvent)
    {
        const AZStd::string& eventName = motionEvent.m_eventTypeName;
        if (eventName.empty())
        {
            return;
        }

        // get the bone name based on the event name e.g. LeftFoot, RightFoot
        AZStd::string_view boneName;
        if (azstricmp(eventName.data(), m_leftFootEventName.c_str()) == 0)
        {
            boneName = m_leftFootEventName;
        }
        else if (azstricmp(eventName.data(), m_rightFootEventName.c_str()) == 0)
        {
            boneName = m_rightFootEventName;
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
            AZ_WarningOnce("FootstepComponent", false, "Missing bone name for footsteps events in %s, effects will play from entity origin", __FUNCTION__);
        }

        AZ::Transform entityTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(entityTransform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
        Matrix34 tm = AZTransformToLYTransform(entityTransform);
        tm.OrthonormalizeFast();
        Ang3 angles = Ang3::GetAnglesXYZ(tm);

        // Setup FX params
        SMFXRunTimeEffectParams params;
        params.angle = angles.z + (gf_PI * 0.5f);
        params.pos = params.decalPos = tm * jointOffset;

        const AZStd::string& fxLibName = motionEvent.m_parameter[0] ? motionEvent.m_parameter : "footstep";

        TMFXEffectId effectId = CheckForWaterEffect(fxLibName, LYVec3ToAZVec3(params.pos), entityTransform);

        if (effectId == InvalidEffectId)
        {
            effectId = CheckForSurfaceEffect(fxLibName, entityTransform);
        }

        if (effectId != InvalidEffectId)
        {
            if (IMaterialEffects* pMaterialEffects = gEnv->pMaterialEffects)
            {
                pMaterialEffects->ExecuteEffect(effectId, params);
            }
        }
    }

    void FootstepComponent::Activate()
    {
        LmbrCentral::CharacterAnimationNotificationBus::Handler::BusConnect(GetEntityId());
        EMotionFX::Integration::ActorNotificationBus::Handler::BusConnect(GetEntityId());
    }

    void FootstepComponent::Deactivate()
    {
        LmbrCentral::CharacterAnimationNotificationBus::Handler::BusDisconnect();
        EMotionFX::Integration::ActorNotificationBus::Handler::BusDisconnect();
    }


    TMFXEffectId FootstepComponent::CheckForWaterEffect(const AZStd::string& fxLibName, const AZ::Vector3& position, const AZ::Transform& entityTransform, float relativeSpeed /*= 0.f*/)
    {
        // it's possible in unit tests that there is no global environment
        if (!gEnv || !gEnv->p3DEngine)
        {
            return InvalidEffectId;
        }

        TMFXEffectId effectId = InvalidEffectId;
        AZStd::string effectName;

        Vec3 legacyPosition = AZVec3ToLYVec3(position);

        float feetWaterLevel = OceanToggle::IsActive() ? OceanRequest::GetWaterLevel(legacyPosition) : gEnv->p3DEngine->GetWaterLevel(&legacyPosition);
        const float deepWaterLevel = 1.0;
        bool usingWaterEffectId = false;

        if (feetWaterLevel != WATER_LEVEL_UNKNOWN)
        {
            const float depth = feetWaterLevel - legacyPosition.z;
            if (depth >= 0.0f)
            {
                Matrix34 legacyTransform = AZTransformToLYTransform(entityTransform);

                // Plug water hits directly into water sim
                gEnv->pRenderer->EF_AddWaterSimHit(legacyTransform.GetTranslation(), 1.0f, relativeSpeed * 2.f);

                // trigger water splashes from here
                if (depth > deepWaterLevel)
                {
                    effectName = "water_deep";
                }
                else
                {
                    effectName = "water_shallow";
                }

                IMaterialEffects* pMaterialEffects = gEnv->pMaterialEffects;
                if (pMaterialEffects)
                {
                    effectId = pMaterialEffects->GetEffectIdByName(fxLibName.c_str(), effectName.c_str());
                    if (effectId == InvalidEffectId)
                    {
                        AZ_Warning("FootstepComponent", false, "Failed to find material for footstep sounds in FXLib %s, Effect: %s", fxLibName.c_str(), effectName.c_str());
                    }
                }
            }
        }

        return effectId;
    }

    TMFXEffectId FootstepComponent::CheckForLegacySurfaceEffect(const AZStd::string& fxLibName)
    {
        TMFXEffectId effectId = InvalidEffectId;

        // get the ground material idx
        pe_status_living livingStatus;
        livingStatus.groundSlope.Set(0.f, 0.f, 1.f);
        LmbrCentral::CryPhysicsComponentRequestBus::Event(GetEntityId(), &LmbrCentral::CryPhysicsComponentRequestBus::Events::GetPhysicsStatus, livingStatus);

        if (livingStatus.groundSurfaceIdx >= 0)  // Filter out the surface id -1 as this means the entity was not in contact with the ground
        {
            if (IMaterialEffects* pMaterialEffects = gEnv->pMaterialEffects)
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

        return effectId;
    }

    TMFXEffectId FootstepComponent::CheckForSurfaceEffect(const AZStd::string & fxLibName, const AZ::Transform& entityTransform)
    {
        TMFXEffectId effectId = InvalidEffectId;

        bool terrainExists = true;
        
        AZStd::string_view surfaceName;
        AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(surfaceName, &AzFramework::Terrain::TerrainDataRequests::GetMaxSurfaceName, entityTransform.GetTranslation(), AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR, &terrainExists);

        if (terrainExists && !surfaceName.empty())
        {
            if (IMaterialEffects* pMaterialEffects = gEnv->pMaterialEffects)
            {
                effectId = pMaterialEffects->GetEffectIdByName(fxLibName.c_str(), surfaceName.data());
            }
        }

        const AZ::Vector3& position = entityTransform.GetTranslation();
        Physics::RigidBodyStatic* terrainBody = nullptr;
        Physics::TerrainRequestBus::BroadcastResult(terrainBody, &Physics::TerrainRequests::GetTerrainTile, position.GetX(), position.GetY());

        AZ::EntityId terrainEntityId;
        if (terrainBody)
        {
            terrainEntityId = terrainBody->GetEntityId();
        }

        Physics::RayCastRequest request;
        request.m_start = entityTransform.GetTranslation();
        request.m_direction = AZ::Vector3(0.0f, 0.0f, -1.0f);
        request.m_distance = 2;

        Physics::RayCastHit hit;
        Physics::WorldRequestBus::BroadcastResult(hit, &Physics::WorldRequests::RayCast, request);
        if (hit.m_body && hit.m_body->GetEntityId() != terrainEntityId)
        {
            if (const auto material = hit.m_shape->GetMaterial())
            {
                surfaceName = material->GetSurfaceTypeName();
                if (!surfaceName.empty())
                {
                    if (IMaterialEffects* pMaterialEffects = gEnv->pMaterialEffects)
                    {
                        effectId = pMaterialEffects->GetEffectIdByName(fxLibName.c_str(), surfaceName.data());
                    }
                }
            }
        }

        if (effectId == InvalidEffectId)
        {
            AZ_Warning("FootstepComponent", false, "Failed to find material for footstep sounds in FXLib %s, Effect: %s", fxLibName.c_str(), surfaceName.data());
            return CheckForLegacySurfaceEffect(fxLibName);
        }

        return effectId;
    }

}

