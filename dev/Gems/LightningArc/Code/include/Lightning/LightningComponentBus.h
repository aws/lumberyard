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
#include <AzCore/Math/Vector3.h>

#include <AzFramework/Asset/SimpleAsset.h>

#include <LmbrCentral/Rendering/MeshAsset.h>
#include <LmbrCentral/Rendering/MaterialAsset.h>

namespace Lightning
{
    // Request bus for the component
    class LightningComponentRequests
        : public AZ::ComponentBus
    {
    public:
        // EBus Traits overrides (Configuring this Ebus)
        // Using Defaults

        virtual ~LightningComponentRequests() {}

        /**
         * Starts the lightning effect.
         *
         * If the StartOnActivate parameter is set to true and 
         * the component has activated this will have no effect.
         */
        virtual void StartEffect() = 0;

        /**
         * Sets whether or not the Lightning effect should start when the component activates.
         *
         * This is only useful at edit-time.
         *
         * @param startOnActivate Set to true to have the component start its effect on activation
         */
        virtual void SetStartOnActivate(bool startOnActivate) = 0;
        /**
         * Gets whether or not the lightning effect will (or did) start on component activation.
         *
         * @return True if the component will start its effect on activation.
         */
        virtual bool GetStartOnActivate() = 0;

        /**
         * Sets whether or not the lightning effect will be relative to the player camera or not.
         *
         * @param relativeToPlayer Set to true to have the lightning particle effect be relative to the player.
         */
        virtual void SetRelativeToPlayer(bool relativeToPlayer) = 0;
        /**
         * Gets whether or not the lightning effect will be relative to the player camera or not.
         *
         * @return True if the lightning effect will be rendered relative to the player camera.
         */
        virtual bool GetRelativeToPlayer() = 0;

        /**
         * Sets the entity to handle the lightning particle.
         *
         * This entity will be destroyed when the effect is over.
         *
         * @param particleEntity The AZ::EntityId of the entity that has a particle effect.
         */
        virtual void SetLightningParticleEntity(AZ::EntityId particleEntity) = 0;
        /**
         * Gets the entity that is being used for the lightning particle effect.
         *
         * @return the AZ::EntityId of the entity used for the lightning particle effect.
         */
        virtual AZ::EntityId GetLightningParticleEntity() = 0;

        /**
         * Sets the entity that has the light used for this effect.
         * 
         * This entity will be destroyed when the effect is over.
         * 
         * @param lightEntity The AZ::EntityId of an entity that has a light component.
         */
        virtual void SetLightEntity(AZ::EntityId lightEntity) = 0;
        /**
         * Gets the entity that is being used for the lighting effect.
         *
         * @return the AZ::EntityId of an entity that has a light component.
         */
        virtual AZ::EntityId GetLightEntity() = 0;

        /**
         * Sets the entity that has the sky highlight used for this effect.
         *
         * This entity will be destroyed when the effect is over.
         *
         * @param skyHighlightEntity The AZ::EntityId of an entity that has a sky highlight component.
         */
        virtual void SetSkyHighlightEntity(AZ::EntityId skyHighlightEntity) = 0;
        /**
         * Gets the entity that is being used for the sky highlight effect.
         *
         * @return the AZ::EntityId of an entity that has a sky highlight component.
         */
        virtual AZ::EntityId GetSkyHighlightEntity() = 0;

        /**
         * Sets the entity that will be used to produce the audio for this effect.
         *
         * The entity must have an AudioTrigger, AudioRTPC and AudioProxy component. Missing components
         * will mean that some features of this effect will not work.
         *
         * This entity will be destroyed when the effect is over.
         *
         * @param audioEntity The AZ::EntityId of an entity that has an audio trigger component and an audio RTPC component.
         */
        virtual void SetAudioEntity(AZ::EntityId audioEntity) = 0;
        /**
         * Gets the entity that has the audio components used for this effect.
         *
         * @return the AZ::EntityId of an entity that has the necessary audio components.
         */
        virtual AZ::EntityId GetAudioEntity() = 0;

        /**
         * Sets the Speed of Sound scale that will be used for this effect.
         *
         * Sound travels at 340.29 meters per second. Use this parameter if you 
         * don't want to move your lightning effect but want the audio to 
         * take more or less time to be heard by the player.
         *
         * A value of 0.5f will make sound from this effect take half as much time to 
         * "reach the player".
         * 
         * @param speedOfSoundScale Scale to apply to the speed of sound.
         */
        virtual void SetSpeedOfSoundScale(float speedOfSoundScale) = 0;
        /**
         * Gets the Speed of Sound scale that is used for this effect.
         *
         * @return The float value used to scale the speed of sound used by this effect.
         */
        virtual float GetSpeedOfSoundScale() = 0;

        /**
         * Sets the amount of random variation to apply to the light's radius.
         *
         * This variation is a percentage of the light radius.
         * A value of 0.2 will mean that between -20% and 20% of the 
         * light's radius value will be added back to the light's radius.
         *
         * Example: Light radius is 5.0, variation is 0.2
         * This means that the range of the light radius is:
         * 5.0 +/- (5.0 * 0.2) or 5.0 +/- 20% of 5.0.
         *
         * @param lightRadiusVariation The variation to apply to the light's radius.
         */
        virtual void SetLightRadiusVariation(float lightRadiusVariation) = 0;
        /**
         * Gets the amount of random variation applied to the light's radius.
         *
         * @return The variation applied to the light's radius.
         */
        virtual float GetLightRadiusVariation() = 0;

        /**
         * Sets the amount of random variation to apply to the light's diffuse and specular multipliers.
         *
         * Light intensity is calculated by this component based on 
         * how far into the effect it is. This variation is a percentage of 
         * that light intensity value. A value of 0.2 will mean that between 
         * -20% and 20% of the light intensity value will be added back onto 
         * the light's diffuse and specular multipliers.
         *
         * Example: Light intensity is 5.0, variation is 0.2
         * This means that the range of the light intensity is:
         * 5.0 +/- (5.0 * 0.2) or 5.0 +/- 20% of 5.0.
         *
         * @param lightIntensityVariation The variation to apply to the light's intensity.
         */
        virtual void SetLightIntensityVariation(float lightIntensityVariation) = 0;
        /**
         * Gets the amount of random variation applied to the light's intensity.
         *
         * @return The variation applied to the light's intensity.
         */
        virtual float GetLightIntensityVariation() = 0;

        /**
         * Sets the amount of random variation to apply to the particle's size.
         *
         * This variation is a percentage of the particle component's global
         * size parameter. A value of 0.2 will mean that between -20% and
         * 20% of the particle's global size value will be added back
         * onto the size of the emitted particles.
         *
         * Example: Particle's global size is 5.0, variation is 0.2
         * This means that the range of the particle size is:
         * 5.0 +/- (5.0 * 0.2) or 5.0 +/- 20% of 5.0.
         *
         * @param particleSizeVariation The variation to apply to the particle's size.
         */
        virtual void SetParticleSizeVariation(float particleSizeVariation) = 0;
        /**
         * Gets the amount of random variation applied to the
         * particle's global size.
         *
         * @return The variation applied to the particle's size.
         */
        virtual float GetParticleSizeVariation() = 0;

        /**
         * Sets how long (in seconds) the lightning strike will last.
         *
         * This is how long the lightning particle effect will last. During
         * this time the sky highlight and light may have enough time to continue to flash.
         *
         * @param lightningDuration The time in seconds that the lightning effect will last for.
         */
        virtual void SetLightningDuration(double lightningDuration) = 0;
        /**
         * Gets how long (in seconds) the lightning strike will last.
         *
         * @return The duration of the lightning effect.
         */
        virtual double GetLightningDuration() = 0;
    };

    using LightningComponentRequestBus = AZ::EBus<LightningComponentRequests>;

} // namespace Lightning
