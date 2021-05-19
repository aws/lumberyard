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

#include <AzCore/EBus/EBus.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/TickBus.h>

#include <AzCore/Math/Random.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Transform.h>

#include <AzFramework/Asset/SimpleAsset.h>

#include <LmbrCentral/Rendering/MeshAsset.h>
#include <LmbrCentral/Rendering/MaterialAsset.h>

#include "Lightning/LightningComponentBus.h"

namespace Lightning
{
    class LightningConfiguration
    {
    public:
        AZ_TYPE_INFO(LightningConfiguration, "{176135EE-7AD5-4CDB-BADC-80722786F038}");
        AZ_CLASS_ALLOCATOR(LightningConfiguration, AZ::SystemAllocator,0);

        static void Reflect(AZ::ReflectContext* context);
        
        bool m_startOnActivate = true;
        bool m_relativeToPlayer = false;
        AZ::EntityId m_lightningParticleEntity;
        AZ::EntityId m_lightEntity;
        AZ::EntityId m_skyHighlightEntity;
        AZ::EntityId m_audioThunderEntity;
        float m_speedOfSoundScale = 1.0f;
        float m_lightRadiusVariation = 0.2f;
        float m_lightIntensityVariation = 0.2f;
        float m_particleSizeVariation = 0.2f;
        double m_lightningDuration = 0.2; //A double because this is time in seconds
    };

    class LightningComponent
        : public AZ::Component
        , private LightningComponentRequestBus::Handler
        , public AZ::TransformNotificationBus::Handler
        , public AZ::TickBus::Handler
    {
    public:
        AZ_COMPONENT(LightningComponent, "{E2553D12-131E-47E3-8AB9-9BF788930F9D}", AZ::Component);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provides);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& requires);
        static void Reflect(AZ::ReflectContext* context);    

        // Constructors / Destructors
        LightningComponent() {}
        explicit LightningComponent(LightningConfiguration *params) 
        {
            m_config = *params;
        }
        ~LightningComponent() = default;

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // TransformNotificationBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        // LightningRequestBus
        void StartEffect() override;

        AZ_INLINE void SetStartOnActivate(bool startOnActivate) override { m_config.m_startOnActivate = startOnActivate; }
        AZ_INLINE bool GetStartOnActivate() override { return m_config.m_startOnActivate; }

        AZ_INLINE void SetRelativeToPlayer(bool relativeToPlayer) override { m_config.m_relativeToPlayer = relativeToPlayer; }
        AZ_INLINE bool GetRelativeToPlayer() override { return m_config.m_relativeToPlayer; }

        void SetLightningParticleEntity(AZ::EntityId particleEntity) override;
        AZ_INLINE AZ::EntityId GetLightningParticleEntity() override { return m_config.m_lightningParticleEntity; }

        void SetLightEntity(AZ::EntityId lightEntity) override;
        AZ_INLINE AZ::EntityId GetLightEntity() override { return m_config.m_lightEntity; }

        void SetSkyHighlightEntity(AZ::EntityId skyHighlightEntity) override;
        AZ_INLINE AZ::EntityId GetSkyHighlightEntity() override { return m_config.m_skyHighlightEntity; }

        AZ_INLINE void SetAudioEntity(AZ::EntityId audioEntity) override { m_config.m_audioThunderEntity = audioEntity; }
        AZ_INLINE AZ::EntityId GetAudioEntity() override { return m_config.m_audioThunderEntity; }

        AZ_INLINE void SetSpeedOfSoundScale(float speedOfSoundScale) override { m_config.m_speedOfSoundScale = speedOfSoundScale; }
        AZ_INLINE float GetSpeedOfSoundScale() override { return m_config.m_speedOfSoundScale; }

        AZ_INLINE void SetLightRadiusVariation(float lightRadiusVariation) override { m_config.m_lightRadiusVariation = lightRadiusVariation; }
        AZ_INLINE float GetLightRadiusVariation() override { return m_config.m_lightRadiusVariation; }

        AZ_INLINE void SetLightIntensityVariation(float lightIntensityVariation) override { m_config.m_lightIntensityVariation = lightIntensityVariation; }
        AZ_INLINE float GetLightIntensityVariation() override { return m_config.m_lightIntensityVariation; }

        AZ_INLINE void SetParticleSizeVariation(float particleSizeVariation) override { m_config.m_particleSizeVariation = particleSizeVariation; }
        AZ_INLINE float GetParticleSizeVariation() override { return m_config.m_particleSizeVariation; }

        AZ_INLINE void SetLightningDuration(double lightningDuration) override { m_config.m_lightningDuration = lightningDuration; }
        AZ_INLINE double GetLightningDuration() override { return m_config.m_lightningDuration; }

    private:
        // Reflected Data
        LightningConfiguration m_config;

        //Unreflected but stored data
        bool m_active = false;
        AZ::Vector3 m_worldPos;

        AZ::SimpleLcgRandom m_rng;

        //Generated from parameters and randomness and applied to engine's lightning systems
        bool m_stopStrike;
        bool m_striking;
        float m_lightFade;
        float m_lightIntensity;
        AZ::Vector3 m_strikeOffset;

        AZ::Transform m_strikeLocalTM;
        bool m_transformInitialized = false;

        float m_originalParticleScale;

        AZ::Color m_originalSkyColor;
        float m_originalSkyColorMultiplier;

        float m_originalLightDiffuseMultiplier;
        float m_originalLightSpecularMultiplier;
        float m_originalLightRadius;

        //Generated sky highlight settings
        AZ::Vector3 m_skyHighlightPosition;

        //Audio settings for thunder
        bool m_thunderPlaying;

        //Timing
        double m_currentTimePoint; ///< Time point retrieved from the last OnTick
        double m_strikeStopTimePoint; ///< Time point when we want to stop rendering the current strike
        double m_thunderStartTimePoint; ///< Time point when we want to play the thunder audio
        double m_thunderStopTimePoint;

        //Const values
        const float m_distanceVariation = 0.2f;

        void StopStrike(); ///< Stops the lightning effect

        void Strike(); ///< Generates a lightning strike

        void RetrieveOriginalParticleSettings();
        void RetrieveOriginalSkyHighlightSettings();
        void RetrieveOriginalLightSettings();

        void TurnOnSkyHighlight();
        void UpdateSkyHighlight();
        void TurnOffSkyHighlight();

        void TurnOnLight();
        void UpdateLight();
        void TurnOffLight();

        void Destroy(); ///< Destroys the entity and all referenced entities

        /**
        * Calculates the light intensity value
        * 
        * Generates a random value from 0.5f to 1.0f
        */
        AZ_INLINE float CalculateLightIntensity() 
        {
            return 1.0f - (m_rng.GetRandomFloat() * 0.5f);
        }

        /**
        * Calculates the light fade value
        * 
        * Generates a random value from 3.0f to 8.0f
        */
        AZ_INLINE float CalculateLightFade()
        {
            return 3.0f + (m_rng.GetRandomFloat() * 5.0f);
        }

        /**
        * Applies a random amount of variation to the given value
        *
        * @param value The float to apply a random variation to
        * @param variation A float that represents the percent of variation
        *
        * Example: Value is 5.0f, variation is 0.2f
        * This means that the range of the return value is:
        * 5.0f +/- 5.0f * 0.2f or 5.0f +/- 20% of 5.0f
        * 
        * Taken from Cry Lightning.lua file 
        */
        AZ_INLINE float GetValueWithVariation(float value, float variation)
        {
            //random is a random value from -1.0f to 1.0f
            float random = ((m_rng.GetRandomFloat() * 2.0f) - 1.0f); 
            return value + (random * value * variation);
        }

        /**
        * Applies a random amount of variation to the given value
        *
        * @param value The double to apply a random variation to
        * @param variation A double that represents the percent of variation
        *
        * Example: Value is 5.0, variation is 0.2
        * This means that the range of the return value is:
        * 5.0 +/- 5.0 * 0.2 or 5.0 +/- 20% of 5.0
        *
        * Taken from Cry Lightning.lua file
        */
        AZ_INLINE double GetValueWithVariation(double value, double variation)
        {
            //random is a random value from -1.0 to 1.0
            double random = ((static_cast<double>(m_rng.GetRandomFloat()) * 2.0) - 1.0);
            return value + (random * value * variation);
        }
    };

} // namespace Lightning
