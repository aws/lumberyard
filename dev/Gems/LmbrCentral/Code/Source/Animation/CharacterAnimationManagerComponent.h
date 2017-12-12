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

#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/TransformBus.h>

#include <LmbrCentral/Rendering/MeshComponentBus.h>
#include <LmbrCentral/Animation/CharacterAnimationBus.h>
#include <LmbrCentral/Physics/PhysicsSystemComponentBus.h>

#include <LmbrCentral/Animation/MotionExtraction.h>

#include <AzCore/std/containers/unordered_map.h>

#include <IAnimationPoseModifier.h>

namespace LmbrCentral
{
    /*
    * This component is responsible for ticking all available ICharacterInstances for animation purposes
    */
    class CharacterAnimationManagerComponent
        : public AZ::Component
        , private SkinnedMeshInformationBus::Handler
        , private AZ::TickBus::Handler
    {
    public:

        AZ_COMPONENT(CharacterAnimationManagerComponent, "{ABD0848C-0CFC-43D3-AEFB-7EEED64AF164}");

        CharacterAnimationManagerComponent();
        ~CharacterAnimationManagerComponent() = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // SkinnedMeshInformationBus::Handler Implementation
        void OnSkinnedMeshCreated(ICharacterInstance* characterInstance, const AZ::EntityId& entityId) override;
        void OnSkinnedMeshDestroyed(const AZ::EntityId& entityId) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        int GetTickOrder() override;
        //////////////////////////////////////////////////////////////////////////

    protected:

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("CharacterAnimationManagementService", 0x4b3a6e86));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("CharacterAnimationManagementService", 0x4b3a6e86));
        }

        static void Reflect(AZ::ReflectContext* context);

    private:

        class CharacterInstanceEntry
            : private AZ::TransformNotificationBus::Handler
            , private CharacterAnimationRequestBus::Handler
            , private PhysicsSystemEventBus::Handler
            , private AimIKComponentRequestBus::Handler
        {
        public:

            CharacterInstanceEntry(const AZ::EntityId& entityId, ICharacterInstance* characterInstance);
            ~CharacterInstanceEntry();
            
            bool TickAnimationSystems(float deltaTime);

            //////////////////////////////////////////////////////////////////////////
            // These methods are to mirror the Component's state
            void Activate();
            void Deactivate();
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////////////
            // Transform notification bus handler
            /// Called when the local transform of the entity has changed.
            void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;
            //////////////////////////////////////////////////////////////////////////////////
            
            //////////////////////////////////////////////////////////////////////////
            // CharacterAnimationRequestBus handler
            void SetBlendParameter(AZ::u32 blendParameter, float value) override;
            float GetBlendParameter(AZ::u32 blendParameter) override;
            void SetAnimationDrivenMotion(bool useAnimDrivenMotion) override;
            void SetMotionParameterSmoothingSettings(const Animation::MotionParameterSmoothingSettings& settings) override;
            ICharacterInstance* GetCharacterInstance() override;
            //////////////////////////////////////////////////////////////////////////
            
            //////////////////////////////////////////////////////////////////////////
            // PhysicsSystemEventBus handler
            void OnPrePhysicsUpdate() override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // AimIKComponentRequestBus handler
            void EnableAimIK(bool enable) override;
            void SetAimIKTarget(const AZ::Vector3& target) override;
            void SetAimIKLayer(AZ::u32 layerIndex) override;
            void SetAimIKFadeOutAngle(float angleRadians) override;
            void SetAimIKFadeOutTime(float fadeOutTime) override;
            void SetAimIKFadeInTime(float fadeInTime) override;
            void SetAimIKFadeOutMinDistance(float distance) override;
            void SetAimIKPolarCoordinatesOffset(const AZ::Vector2& offset) override;
            void SetAimIKPolarCoordinatesSmoothTimeSeconds(float smoothTimeSeconds) override;
            void SetAimIKPolarCoordinatesMaxRadiansPerSecond(const AZ::Vector2& maxRadiansPerSecond) override;
            float GetAimIKBlend() override;
            //////////////////////////////////////////////////////////////////////////

        private:

            void UpdateParametricBlendParameters(float deltaTime, const AZ::Transform& frameMotionDelta);

            AZ::EntityId m_entityId;
            AZ::Transform m_previousWorldTransform;
            AZ::Transform m_currentWorldTransform;
            ICharacterInstance* m_characterInstance;

            bool m_useAnimDrivenMotion;         ///< Set if root motion has been externally requested via script or C++ ebus.
            bool m_appliedAnimDrivenMotion;     ///< Runtime state set if either root motion is manually enabled, or current animation playback has root control.

            Animation::MotionParameters m_smoothedMotionParameters;                         ///< Continuously-smoothed motion parameters that are passed to the blending layer.
            Animation::MotionParameterSmoothingState m_motionParamsSmoothingState;          ///< Required state for smoothing calculations.
            Animation::MotionParameterSmoothingSettings m_motionParamsSmoothingSettings;    ///< Parameter smoothing settings currently set this character instance.

            IAnimationPoseAlignerPtr m_limbIK;  ///< Limb IK instance.
        };

        //////////////////////////////////////////////////////////////////////////
        //! Stores all active character instances
        AZStd::unordered_map<AZ::EntityId, CharacterInstanceEntry> m_characterInstanceMap;
        //////////////////////////////////////////////////////////////////////////
    };
} // namespace LmbrCentral
