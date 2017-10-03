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
#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Component/ComponentBus.h>

struct ICharacterInstance;

namespace AZ
{
    class EntityId;
}

namespace LmbrCentral
{
    namespace Animation
    {
        struct MotionParameterSmoothingSettings;
    }

    /**
     * General character animation requests serviced by the CharacterAnimationManager component.
     */
    class CharacterAnimationRequests 
        : public AZ::ComponentBus
    {
    public:

        /// Set custom blend parameter.
        /// \param blendParameter - corresponds to EMotionParamID
        /// \param value - value to set
        virtual void SetBlendParameter(AZ::u32 /*blendParameter*/, float /*value*/) {};

        /// Retrieve custom blend parameter.
        /// \param blendParameter - corresponds to EMotionParamID
        virtual float GetBlendParameter(AZ::u32 /*blendParameter*/) { return 0.f; }

        /// Enables or disables animation-driven root motion.
        /// \param useAnimDrivenMotion
        virtual void SetAnimationDrivenMotion(bool /*useAnimDrivenMotion*/) {};

        /// Sets smoothing settings for motion parameters (for blending).
        /// \param structure containing smoothing settings.
        virtual void SetMotionParameterSmoothingSettings(const Animation::MotionParameterSmoothingSettings& /*settings*/) {};

        /// Retrieves the character instance associated with the entity.
        virtual ICharacterInstance* GetCharacterInstance() { return nullptr; }
    };

    using CharacterAnimationRequestBus = AZ::EBus<CharacterAnimationRequests>;

    /**
     * Aim IK component request bus.
     * \todo - This is currently serviced by the system, but will be migrated to a component
     * if CryAnimation's lifetime turns out to extend beyond the next couple of months.
     */
    class AimIKComponentRequests
        : public AZ::ComponentBus
    {
    public:

        /// Enable/disable Aim IK
        /// \param enable - true to enable, false to disable.
        virtual void EnableAimIK(bool enable) = 0;

        /// Sets the world-space target.
        /// \param target - world space IK target position.
        virtual void SetAimIKTarget(const AZ::Vector3& target) = 0;

        /// Notifies the AimIK system the layer on which the AimIK asset is currently playing.
        /// \param layerIndex - animation layer index [0..15].
        virtual void SetAimIKLayer(AZ::u32 layerIndex) = 0;

        /// Sets the fade out angle for Aim IK.
        /// \param angle - in radians, once the target vector is outside of this angle with respect to the character's facing, IK will fade out.
        virtual void SetAimIKFadeOutAngle(float angleRadians) = 0;

        /// Sets the IK fade out time.
        /// \param fadeOutTime - in seconds. Once aim IK fades out due to failing angle/range restrictions, the blend out will be performed over this time duration.
        virtual void SetAimIKFadeOutTime(float fadeOutTime) = 0;

        /// Sets the IK fade out time.
        /// \param fadeInTime - in seconds. Once aim IK fades in due to meeting angle/range restrictions, the blend in will be performed over this time duration.
        virtual void SetAimIKFadeInTime(float fadeInTime) = 0;

        /// Sets the IK fade out distance.
        /// \param distance - in meters. Once the target is beyond this distance from the effector bone, aim IK will fade out.
        virtual void SetAimIKFadeOutMinDistance(float distance) = 0;

        /// Sets a polar coordinate offset.
        /// \param offset - with x and y components. This applies a bias to the internal polar coordinates for "off-center" aim IK.
        virtual void SetAimIKPolarCoordinatesOffset(const AZ::Vector2& offset) = 0;

        /// Sets a polar coordinate smooth time.
        /// \param smoothTimeSeconds - IK converges on the target over this time, controlling the dampening effect to the aim behavior.
        virtual void SetAimIKPolarCoordinatesSmoothTimeSeconds(float smoothTimeSeconds) = 0;

        /// Sets a polar coordinate max angle change per second.
        /// \param smoothTimeSeconds - IK convergence is limited to this number of radians per second.
        virtual void SetAimIKPolarCoordinatesMaxRadiansPerSecond(const AZ::Vector2& maxRadiansPerSecond) = 0;

        /// Retrieves the current weight of the aim IK
        /// \return current weight - ranges from [0..1].
        virtual float GetAimIKBlend() = 0;
    };

    using AimIKComponentRequestBus = AZ::EBus<AimIKComponentRequests>;

    /**
     * Represents a timed event/annotation dispatched during animation playback.
     */
    struct AnimationEvent
    {
        AZ_TYPE_INFO(AnimationEvent, "{1D927664-8C19-4EA9-A59F-23A81EC486F2}");

        float m_time                        = 0.f;                          ///< Normalized animation time at which event fired/started.
        float m_endTime                     = 0.f;                          ///< Normalized animation time at which the event ends.
        const char* m_animName              = nullptr;                      ///< Path to originating animation.
        const char* m_eventName             = nullptr;                      ///< Logical event name (footstep, etc).
        const char* m_parameter             = nullptr;                      ///< Optional customer parameter.
        const char* m_boneName1             = nullptr;                      ///< Optional bone name.
        const char* m_boneName2             = nullptr;                      ///< Optional auxiliary bone name.
        AZ::Vector3 m_offset                = AZ::Vector3::CreateZero();    ///< Optional translation offset value.
        AZ::Vector3 m_direction             = AZ::Vector3::CreateZero();    ///< Optional direction value.
    };

    /**
     * Animation event bus for a given character instance.
     */
    class CharacterAnimationNotifications
        : public AZ::ComponentBus
    {
    public:

        /// An animation event has been fired during animation playback.
        virtual void OnAnimationEvent(const AnimationEvent& event) { (void)event;  };

        /// Sent when a character instance for this entity is registered and ready for animation.
        virtual void OnCharacterInstanceRegistered(ICharacterInstance* character) { (void)character; }

        /// Sent when a character instance is unregistered from animation and is likely being destroyed.
        virtual void OnCharacterInstanceUnregistered() {}
    };
    using CharacterAnimationNotificationBus = AZ::EBus<CharacterAnimationNotifications>;

} // namespace LmbrCentral
