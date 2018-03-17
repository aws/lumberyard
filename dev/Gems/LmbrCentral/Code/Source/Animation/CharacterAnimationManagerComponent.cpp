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
#include "LmbrCentral_precompiled.h"

#include <ISystem.h>
#include <ICryAnimation.h>
#include <CryExtension/CryCreateClassInstance.h>

#include <MathConversion.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Math/Plane.h>
#include <AzFramework/Physics/PhysicsComponentBus.h>

#include <LmbrCentral/Physics/CryCharacterPhysicsBus.h>
#include <LmbrCentral/Physics/CryPhysicsComponentRequestBus.h>

#include "CharacterAnimationManagerComponent.h"

namespace LmbrCentral
{
    using AzFramework::PhysicsSystemEventBus;

    class CharacterAnimationNotificationBusHandler : public CharacterAnimationNotificationBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(CharacterAnimationNotificationBusHandler, "{8D0EBC9B-0DB5-42FD-BA17-B9F513411BC4}", AZ::SystemAllocator,
            OnAnimationEvent);

        void OnAnimationEvent(const AnimationEvent& event) override
        {
            Call(FN_OnAnimationEvent, event);
        }
    };

    //////////////////////////////////////////////////////////////////////////

    CharacterAnimationManagerComponent::CharacterInstanceEntry::CharacterInstanceEntry(const AZ::EntityId& entityId, ICharacterInstance* characterInstance)
        : m_entityId(entityId)
        , m_characterInstance(characterInstance)
        , m_currentWorldTransform(AZ::Transform::Identity())
        , m_previousWorldTransform(AZ::Transform::Identity())
        , m_useAnimDrivenMotion(false)
        , m_appliedAnimDrivenMotion(false)
    {
    }

    CharacterAnimationManagerComponent::CharacterInstanceEntry::~CharacterInstanceEntry()
    {
        Deactivate();
    }

    void CharacterAnimationManagerComponent::CharacterInstanceEntry::Activate()
    {
        AZ::TransformNotificationBus::Handler::BusConnect(m_entityId);
        CharacterAnimationRequestBus::Handler::BusConnect(m_entityId);
        AimIKComponentRequestBus::Handler::BusConnect(m_entityId);

        PhysicsSystemEventBus::Handler::BusConnect();

        EBUS_EVENT_ID_RESULT(m_currentWorldTransform, m_entityId, AZ::TransformBus, GetWorldTM);
        m_previousWorldTransform = m_currentWorldTransform;

        if (m_characterInstance && m_characterInstance->GetISkeletonAnim())
        {
            // This simply enables the ability to sample root motion. It does not alone turn on animation-driven root motion.
            m_characterInstance->GetISkeletonAnim()->SetAnimationDrivenMotion(true);

            // Create the limb IK pose modifier instance (legacy compatibility).
            CryCreateClassInstance("PoseAlignerLimbIK", m_limbIK);
            if (m_limbIK)
            {
                if (!m_limbIK->Initialize(m_entityId, *m_characterInstance))
                {
                    m_limbIK = nullptr;
                }
            }
        }
    }

    void CharacterAnimationManagerComponent::CharacterInstanceEntry::Deactivate()
    {
        m_limbIK = nullptr;

        PhysicsSystemEventBus::Handler::BusDisconnect();

        AimIKComponentRequestBus::Handler::BusDisconnect();
        CharacterAnimationRequestBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
    }

    void CharacterAnimationManagerComponent::CharacterInstanceEntry::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_currentWorldTransform = world;
    }

    bool CharacterAnimationManagerComponent::CharacterInstanceEntry::TickAnimationSystems(float deltaTime)
    {
        if (m_characterInstance && m_characterInstance->GetISkeletonAnim())
        {
            // Tick limb IK.
            if (m_limbIK)
            {
                m_limbIK->Update(AZTransformToLYQuatT(m_currentWorldTransform), deltaTime);
            }

            // Compute motion delta and update parametric blending.
            const AZ::Transform frameMotionDelta = m_previousWorldTransform.GetInverseFull() * m_currentWorldTransform;
            UpdateParametricBlendParameters(deltaTime, frameMotionDelta);

            // Queue anim jobs for this character.
            auto transform = AZTransformToLYTransform(m_currentWorldTransform);
            SAnimationProcessParams params;
            params.locationAnimation = QuatTS(transform);
            m_characterInstance->StartAnimationProcessing(params);

            m_previousWorldTransform = m_currentWorldTransform;

            return true;
        }

        return false;
    }

    void CharacterAnimationManagerComponent::CharacterInstanceEntry::UpdateParametricBlendParameters(float deltaTime, const AZ::Transform& frameMotionDelta)
    {
        ISkeletonAnim* skeletonAnim = m_characterInstance ? m_characterInstance->GetISkeletonAnim() : nullptr;
        if (skeletonAnim)
        {
            pe_status_living livingStatus;
            livingStatus.groundSlope.Set(0.f, 0.f, 1.f);
            EBUS_EVENT_ID(m_entityId, CryPhysicsComponentRequestBus, GetPhysicsStatus, livingStatus);

            const Animation::MotionParameters targetMotionParameters = Animation::ExtractMotionParameters(
                deltaTime, 
                m_currentWorldTransform, 
                frameMotionDelta, 
                LYVec3ToAZVec3(livingStatus.groundSlope));

            // Apply smoothing to blend params.
            Animation::SmoothMotionParameters(m_smoothedMotionParameters, targetMotionParameters, m_motionParamsSmoothingState, m_motionParamsSmoothingSettings, deltaTime);

            // Convert character-relative travel direction into an angle for blend parameters. 
            // Signed, ranges from -Pi..Pi, with positive angles to the left.
            float relativeTravelAngle = 0.f;
            if (!m_smoothedMotionParameters.m_relativeTravelDelta.IsZero())
            {
                relativeTravelAngle = atan2f(-m_smoothedMotionParameters.m_relativeTravelDelta.GetX(), m_smoothedMotionParameters.m_relativeTravelDelta.GetY());
            }

            // Apply final blend params to the animation layer.
            skeletonAnim->SetDesiredMotionParam(eMotionParamID_TravelAngle, relativeTravelAngle, deltaTime);
            skeletonAnim->SetDesiredMotionParam(eMotionParamID_TravelSlope, m_smoothedMotionParameters.m_groundAngleSigned, deltaTime);
            skeletonAnim->SetDesiredMotionParam(eMotionParamID_TravelDist, m_smoothedMotionParameters.m_travelDistance, deltaTime);
            skeletonAnim->SetDesiredMotionParam(eMotionParamID_TravelSpeed, m_smoothedMotionParameters.m_travelSpeed, deltaTime);
            skeletonAnim->SetDesiredMotionParam(eMotionParamID_TurnAngle, m_smoothedMotionParameters.m_turnAngle, deltaTime);
            skeletonAnim->SetDesiredMotionParam(eMotionParamID_TurnSpeed, m_smoothedMotionParameters.m_turnSpeed, deltaTime);
        }
    }

    void CharacterAnimationManagerComponent::CharacterInstanceEntry::SetBlendParameter(AZ::u32 blendParameter, float value)
    {
        ISkeletonAnim* skeletonAnim = m_characterInstance ? m_characterInstance->GetISkeletonAnim() : nullptr;
        if (skeletonAnim)
        {
            if (blendParameter >= eMotionParamID_First && blendParameter <= eMotionParamID_Last)
            {
                float frameDeltaTime = 0.f;
                EBUS_EVENT_RESULT(frameDeltaTime, AZ::TickRequestBus, GetTickDeltaTime);
                skeletonAnim->SetDesiredMotionParam(static_cast<EMotionParamID>(blendParameter), AZ::GetClamp(value, 0.f, 1.f), frameDeltaTime);
            }
            else
            {
                AZ_Warning("Character Animation", false, 
                           "Unable to set out-of-range blend parameter %u. Use eMotionParamID_BlendWeight through eMotionParamID_BlendWeight7.",
                           blendParameter);
            }
        }
    }
    
    float CharacterAnimationManagerComponent::CharacterInstanceEntry::GetBlendParameter(AZ::u32 blendParameter)
    {
        float value = 0.f;

        ISkeletonAnim* skeletonAnim = m_characterInstance ? m_characterInstance->GetISkeletonAnim() : nullptr;
        if (skeletonAnim)
        {
            if (blendParameter >= eMotionParamID_First && blendParameter <= eMotionParamID_Last)
            {
                if (!skeletonAnim->GetDesiredMotionParam(static_cast<EMotionParamID>(blendParameter), value))
                {
                    AZ_Warning("Character Animation", false, 
                               "Failed to retrieve blend parameter %u. This generally means no parametric animations (blend spaces) are playing.",
                               blendParameter);
                }
            }
            else
            {
                AZ_Warning("Character Animation", false, 
                           "Unable to set out-of-range blend parameter %u. Use eMotionParamID_BlendWeight through eMotionParamID_BlendWeight7.",
                           blendParameter);
            }
        }

        return value;
    }

    void CharacterAnimationManagerComponent::CharacterInstanceEntry::SetAnimationDrivenMotion(bool useAnimDrivenMotion)
    {
        m_useAnimDrivenMotion = useAnimDrivenMotion;
    }

    void CharacterAnimationManagerComponent::CharacterInstanceEntry::SetMotionParameterSmoothingSettings(const Animation::MotionParameterSmoothingSettings& settings)
    {
        m_motionParamsSmoothingSettings = settings;
    }

    ICharacterInstance* CharacterAnimationManagerComponent::CharacterInstanceEntry::GetCharacterInstance()
    {
        return m_characterInstance;
    }

    void CharacterAnimationManagerComponent::CharacterInstanceEntry::OnPrePhysicsUpdate()
    {
        ISkeletonAnim* skeletonAnim = m_characterInstance ? m_characterInstance->GetISkeletonAnim() : nullptr;

        if (!skeletonAnim)
        {
            return;
        }

        // Animation-driven root motion is enabled one of two ways:
        //  1. SetAnimationDrivenMotion() has been called via the CharacterAnimationRequestBus ebus (script/C++ driven flag).
        //  2. The master animation on layer 0 has the CA_FULL_ROOT_PRIORITY flag set (mannequin or legacy flag).
        bool activeAnimHasRootPriority = false;
        if (skeletonAnim->GetNumAnimsInFIFO(0))
        {
            const CAnimation& topAnim = skeletonAnim->GetAnimFromFIFO(0, 0);
            activeAnimHasRootPriority = topAnim.HasStaticFlag(CA_FULL_ROOT_PRIORITY);
        }

        const bool applyAnimDrivenMotion = (m_useAnimDrivenMotion || activeAnimHasRootPriority);

        if (applyAnimDrivenMotion)
        {
            AZ::Transform entityTransform;
            EBUS_EVENT_ID_RESULT(entityTransform, m_entityId, AZ::TransformBus, GetWorldTM);

            float frameDeltaTime = 0.f;
            EBUS_EVENT_RESULT(frameDeltaTime, AZ::TickRequestBus, GetTickDeltaTime);
            const float frameDeltaTimeInv = (frameDeltaTime > 0.f) ? (1.f / frameDeltaTime) : 0.f;

            // Apply relative translation to the character via physics.
            const AZ::Transform motion = LYQuatTToAZTransform(skeletonAnim->CalculateRelativeMovement(frameDeltaTime, 0));
            const AZ::Vector3 relativeTranslation = motion.GetTranslation();
            const AZ::Quaternion characterOrientation = AZ::Quaternion::CreateFromTransform(entityTransform);
            const AZ::Vector3 velocity = characterOrientation * (relativeTranslation * frameDeltaTimeInv);
            EBUS_EVENT_ID(m_entityId, CryCharacterPhysicsRequestBus, RequestVelocity, velocity, 0);

            // Rotation is applied to the entity's transform directly, since physics only handles translation.
            const AZ::Quaternion& rotation = AZ::Quaternion::CreateFromTransform(motion);
            entityTransform = entityTransform * AZ::Transform::CreateFromQuaternion(rotation);
            entityTransform.Orthogonalize();
            EBUS_EVENT_ID(m_entityId, AZ::TransformBus, SetWorldTM, entityTransform);
        }
        else if (m_appliedAnimDrivenMotion)
        {
            // Issue a stop if we're exiting animation-driven root motion.
            EBUS_EVENT_ID(m_entityId, CryCharacterPhysicsRequestBus, RequestVelocity, AZ::Vector3::CreateZero(), 0);
        }

        m_appliedAnimDrivenMotion = applyAnimDrivenMotion;
    }

    //////////////////////////////////////////////////////////////////////////

    void CharacterAnimationManagerComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<CharacterAnimationManagerComponent, AZ::Component>()
                ->Version(1)
                ->SerializerForEmptyClass();

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<CharacterAnimationManagerComponent>(
                    "Character Animation", "Character animation system")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Game")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ;
            }
            
            serializeContext->Class<AnimationEvent>()
                ->Version(1)
                ->Field("name", &AnimationEvent::m_eventName)
                ->Field("time", &AnimationEvent::m_time)
                ->Field("endTime", &AnimationEvent::m_endTime)
                ->Field("animName", &AnimationEvent::m_animName)
                ->Field("parameter", &AnimationEvent::m_parameter)
                ->Field("boneName1", &AnimationEvent::m_boneName1)
                ->Field("boneName2", &AnimationEvent::m_boneName2)
                ->Field("offset", &AnimationEvent::m_offset)
                ->Field("direction", &AnimationEvent::m_direction)
                ;

        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            // Expose motion parameter enum constants to script.
            behaviorContext->Constant("eMotionParamID_TravelSpeed", BehaviorConstant(eMotionParamID_TravelSpeed));
            behaviorContext->Constant("eMotionParamID_TurnSpeed", BehaviorConstant(eMotionParamID_TurnSpeed));
            behaviorContext->Constant("eMotionParamID_TravelAngle", BehaviorConstant(eMotionParamID_TravelAngle));
            behaviorContext->Constant("eMotionParamID_TravelSlope", BehaviorConstant(eMotionParamID_TravelSlope));
            behaviorContext->Constant("eMotionParamID_TurnAngle", BehaviorConstant(eMotionParamID_TurnAngle));
            behaviorContext->Constant("eMotionParamID_TravelDist", BehaviorConstant(eMotionParamID_TravelDist));
            behaviorContext->Constant("eMotionParamID_StopLeg", BehaviorConstant(eMotionParamID_StopLeg));
            behaviorContext->Constant("eMotionParamID_BlendWeight", BehaviorConstant(eMotionParamID_BlendWeight));
            behaviorContext->Constant("eMotionParamID_BlendWeight2", BehaviorConstant(eMotionParamID_BlendWeight2));
            behaviorContext->Constant("eMotionParamID_BlendWeight3", BehaviorConstant(eMotionParamID_BlendWeight3));
            behaviorContext->Constant("eMotionParamID_BlendWeight4", BehaviorConstant(eMotionParamID_BlendWeight4));
            behaviorContext->Constant("eMotionParamID_BlendWeight5", BehaviorConstant(eMotionParamID_BlendWeight5));
            behaviorContext->Constant("eMotionParamID_BlendWeight6", BehaviorConstant(eMotionParamID_BlendWeight6));
            behaviorContext->Constant("eMotionParamID_BlendWeight7", BehaviorConstant(eMotionParamID_BlendWeight7));

            // Reflect character animation buses.
            //////////////////////////////////////////////////////////////////////////
            
            behaviorContext->EBus<CharacterAnimationRequestBus>("CharacterAnimationRequestBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Event("SetBlendParameter", &CharacterAnimationRequestBus::Events::SetBlendParameter)
                ->Event("GetBlendParameter", &CharacterAnimationRequestBus::Events::GetBlendParameter)
                ->Event("SetAnimationDrivenMotion", &CharacterAnimationRequestBus::Events::SetAnimationDrivenMotion)
                ;

            behaviorContext->Class<AnimationEvent>()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Property("name", BehaviorValueGetter(&AnimationEvent::m_eventName), nullptr)
                ->Property("time", BehaviorValueGetter(&AnimationEvent::m_time), nullptr)
                ->Property("endTime", BehaviorValueGetter(&AnimationEvent::m_endTime), nullptr)
                ->Property("animName", BehaviorValueGetter(&AnimationEvent::m_animName), nullptr)
                ->Property("parameter", BehaviorValueGetter(&AnimationEvent::m_parameter), nullptr)
                ->Property("boneName1", BehaviorValueGetter(&AnimationEvent::m_boneName1), nullptr)
                ->Property("boneName2", BehaviorValueGetter(&AnimationEvent::m_boneName2), nullptr)
                ->Property("offset", BehaviorValueGetter(&AnimationEvent::m_offset), nullptr)
                ->Property("direction", BehaviorValueGetter(&AnimationEvent::m_direction), nullptr)
                ;

            behaviorContext->EBus<CharacterAnimationNotificationBus>("CharacterAnimationNotificationBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Handler<CharacterAnimationNotificationBusHandler>();

            behaviorContext->EBus<AimIKComponentRequestBus>("AimIKComponentRequestBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Event("EnableAimIK", &AimIKComponentRequestBus::Events::EnableAimIK)
                ->Event("SetAimIKTarget", &AimIKComponentRequestBus::Events::SetAimIKTarget)
                ->Event("SetAimIKLayer", &AimIKComponentRequestBus::Events::SetAimIKLayer)
                ->Event("SetAimIKFadeOutAngle", &AimIKComponentRequestBus::Events::SetAimIKFadeOutAngle)
                ->Event("SetAimIKFadeOutTime", &AimIKComponentRequestBus::Events::SetAimIKFadeOutTime)
                ->Event("SetAimIKFadeInTime", &AimIKComponentRequestBus::Events::SetAimIKFadeInTime)
                ->Event("SetAimIKFadeOutMinDistance", &AimIKComponentRequestBus::Events::SetAimIKFadeOutMinDistance)
                ->Event("SetAimIKPolarCoordinatesOffset", &AimIKComponentRequestBus::Events::SetAimIKPolarCoordinatesOffset)
                ->Event("SetAimIKPolarCoordinatesSmoothTimeSeconds", &AimIKComponentRequestBus::Events::SetAimIKPolarCoordinatesSmoothTimeSeconds)
                ->Event("SetAimIKPolarCoordinatesMaxRadiansPerSecond", &AimIKComponentRequestBus::Events::SetAimIKPolarCoordinatesMaxRadiansPerSecond)
                ->Event("GetAimIKBlend", &AimIKComponentRequestBus::Events::GetAimIKBlend)
                ;
        }
    }

    CharacterAnimationManagerComponent::CharacterAnimationManagerComponent()
    {
    }

    void CharacterAnimationManagerComponent::Activate()
    {
        for (auto& meshCharacterInstance : m_characterInstanceMap)
        {
            meshCharacterInstance.second.Activate();
        }

        SkinnedMeshInformationBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();
    }

    void CharacterAnimationManagerComponent::Deactivate()
    {
        SkinnedMeshInformationBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();

        for (auto& meshCharacterInstance : m_characterInstanceMap)
        {
            meshCharacterInstance.second.Deactivate();
        }
    }

    void CharacterAnimationManagerComponent::OnSkinnedMeshCreated(ICharacterInstance* characterInstance, const AZ::EntityId& entityId)
    {
        auto result = m_characterInstanceMap.insert(AZStd::make_pair<AZ::EntityId, CharacterInstanceEntry>(entityId, CharacterInstanceEntry(entityId, characterInstance)));

        if (result.second)
        {
            characterInstance->SetOwnerId(entityId);

            result.first->second.Activate();

            CharacterAnimationNotificationBus::Event(entityId, &CharacterAnimationNotifications::OnCharacterInstanceRegistered, characterInstance);
        }
    }

    void CharacterAnimationManagerComponent::OnSkinnedMeshDestroyed(const AZ::EntityId& entityId)
    {
        const auto& entry = m_characterInstanceMap.find(entityId);
        if (entry != m_characterInstanceMap.end())
        {
            CharacterAnimationNotificationBus::Event(entityId, &CharacterAnimationNotifications::OnCharacterInstanceUnregistered);

            m_characterInstanceMap.erase(entry);
        }
    }

    void CharacterAnimationManagerComponent::OnTick(float deltaTime, AZ::ScriptTimePoint /*time*/)
    {
        for (auto characterIter = m_characterInstanceMap.begin(); characterIter != m_characterInstanceMap.end();)
        {
            bool wasTicked = characterIter->second.TickAnimationSystems(deltaTime);
            if (!wasTicked)
            {
                characterIter = m_characterInstanceMap.erase(characterIter);
            }
            else
            {
                ++characterIter;
            }
        }
    }

    int CharacterAnimationManagerComponent::GetTickOrder()
    {
        return AZ::TICK_ANIMATION;
    }

    //////////////////////////////////////////////////////////////////////////
    // Aim IK
    // \todo - This is currently serviced by the system, but will be migrated to a component
    // if CryAnimation's lifetime turns out to extend beyond the next couple of months.

    IAnimationPoseBlenderDir* GetAimIK(ICharacterInstance* character)
    {
        AZ_Assert(character, "Invalid character instance.");
        ISkeletonPose* skeleton = character->GetISkeletonPose();
        return skeleton ? skeleton->GetIPoseBlenderAim() : nullptr;
    }

    void CharacterAnimationManagerComponent::CharacterInstanceEntry::EnableAimIK(bool enable)
    {
        IAnimationPoseBlenderDir* aimIK = GetAimIK(m_characterInstance);
        if (aimIK)
        {
            aimIK->SetState(enable);
        }
    }
    
    void CharacterAnimationManagerComponent::CharacterInstanceEntry::SetAimIKTarget(const AZ::Vector3& target)
    {
        IAnimationPoseBlenderDir* aimIK = GetAimIK(m_characterInstance);
        if (aimIK)
        {
            aimIK->SetTarget(AZVec3ToLYVec3(target));
        }
    }
    
    void CharacterAnimationManagerComponent::CharacterInstanceEntry::SetAimIKLayer(AZ::u32 layerIndex)
    {
        IAnimationPoseBlenderDir* aimIK = GetAimIK(m_characterInstance);
        if (aimIK)
        {
            aimIK->SetLayer(layerIndex);
        }
    }
    
    void CharacterAnimationManagerComponent::CharacterInstanceEntry::SetAimIKFadeOutAngle(float angleRadians)
    {
        IAnimationPoseBlenderDir* aimIK = GetAimIK(m_characterInstance);
        if (aimIK)
        {
            aimIK->SetFadeoutAngle(angleRadians);
        }
    }
    
    void CharacterAnimationManagerComponent::CharacterInstanceEntry::SetAimIKFadeOutTime(float fadeOutTime)
    {
        IAnimationPoseBlenderDir* aimIK = GetAimIK(m_characterInstance);
        if (aimIK)
        {
            aimIK->SetFadeOutSpeed(fadeOutTime);
        }
    }
    
    void CharacterAnimationManagerComponent::CharacterInstanceEntry::SetAimIKFadeInTime(float fadeInTime)
    {
        IAnimationPoseBlenderDir* aimIK = GetAimIK(m_characterInstance);
        if (aimIK)
        {
            aimIK->SetFadeInSpeed(fadeInTime);
        }
    }
    
    void CharacterAnimationManagerComponent::CharacterInstanceEntry::SetAimIKFadeOutMinDistance(float distance)
    {
        IAnimationPoseBlenderDir* aimIK = GetAimIK(m_characterInstance);
        if (aimIK)
        {
            aimIK->SetFadeOutMinDistance(distance);
        }
    }
    
    void CharacterAnimationManagerComponent::CharacterInstanceEntry::SetAimIKPolarCoordinatesOffset(const AZ::Vector2& offset)
    {
        IAnimationPoseBlenderDir* aimIK = GetAimIK(m_characterInstance);
        if (aimIK)
        {
            aimIK->SetPolarCoordinatesOffset(Vec2(offset.GetX(), offset.GetY()));
        }
    }
    
    void CharacterAnimationManagerComponent::CharacterInstanceEntry::SetAimIKPolarCoordinatesSmoothTimeSeconds(float smoothTimeSeconds)
    {
        IAnimationPoseBlenderDir* aimIK = GetAimIK(m_characterInstance);
        if (aimIK)
        {
            aimIK->SetPolarCoordinatesSmoothTimeSeconds(smoothTimeSeconds);
        }
    }
    
    void CharacterAnimationManagerComponent::CharacterInstanceEntry::SetAimIKPolarCoordinatesMaxRadiansPerSecond(const AZ::Vector2& maxRadiansPerSecond)
    {
        IAnimationPoseBlenderDir* aimIK = GetAimIK(m_characterInstance);
        if (aimIK)
        {
            aimIK->SetPolarCoordinatesMaxRadiansPerSecond(Vec2(maxRadiansPerSecond.GetX(), maxRadiansPerSecond.GetY()));
        }
    }
    
    float CharacterAnimationManagerComponent::CharacterInstanceEntry::GetAimIKBlend()
    {
        IAnimationPoseBlenderDir* aimIK = GetAimIK(m_characterInstance);
        if (aimIK)
        {
            return aimIK->GetBlend();
        }
        return 0.f;
    }
    //////////////////////////////////////////////////////////////////////////

} // namespace LmbrCentral
