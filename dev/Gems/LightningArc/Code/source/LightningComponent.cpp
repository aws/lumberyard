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

#include "LightningArc_precompiled.h"
#include "LightningComponent.h"

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/VectorConversions.h>
#include <MathConversion.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>

#include <LmbrCentral/Rendering/ParticleComponentBus.h>
#include <LmbrCentral/Rendering/LightComponentBus.h>
#include <LmbrCentral/Audio/AudioTriggerComponentBus.h>
#include <LmbrCentral/Audio/AudioRtpcComponentBus.h>
#include "Lightning/SkyHighlightComponentBus.h"

namespace Lightning
{
    void LightningComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provides)
    {
        provides.push_back(AZ_CRC("LightningService"));
    }
    void LightningComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("LightningService"));
    }
    void LightningComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& requires)
    {
        requires.push_back(AZ_CRC("TransformService"));
    }

    void LightningConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<LightningConfiguration>()
                ->Version(1)
                ->Field("ActiveOnStart", &LightningConfiguration::m_startOnActivate)
                ->Field("RelativeToPlayer", &LightningConfiguration::m_relativeToPlayer)
                ->Field("LightningDuration", &LightningConfiguration::m_lightningDuration)
                ->Field("ParticleEntity", &LightningConfiguration::m_lightningParticleEntity)
                ->Field("ParticleSizeVariation", &LightningConfiguration::m_particleSizeVariation)
                ->Field("SkyHighlightEntity", &LightningConfiguration::m_skyHighlightEntity)
                ->Field("LightEntity", &LightningConfiguration::m_lightEntity)
                ->Field("LightRadiusVariation", &LightningConfiguration::m_lightRadiusVariation)
                ->Field("LightIntensityVariation", &LightningConfiguration::m_lightIntensityVariation)
                ->Field("AudioThunderEntity", &LightningConfiguration::m_audioThunderEntity)
                ->Field("SpeedOfSoundScale", &LightningConfiguration::m_speedOfSoundScale)                
                ;
            ;
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<LightningComponentRequestBus>("LightningComponentRequestBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                
                ->Event("StartEffect", &LightningComponentRequestBus::Events::StartEffect)

                ->Event("SetStartOnActivate", &LightningComponentRequestBus::Events::SetStartOnActivate)
                ->Event("GetStartOnActivate", &LightningComponentRequestBus::Events::GetStartOnActivate)
                ->VirtualProperty("StartOnActivate", "GetStartOnActivate", "SetStartOnActivate")

                ->Event("SetRelativeToPlayer", &LightningComponentRequestBus::Events::SetRelativeToPlayer)
                ->Event("GetRelativeToPlayer", &LightningComponentRequestBus::Events::GetRelativeToPlayer)
                ->VirtualProperty("RelativeToPlayer", "GetRelativeToPlayer", "SetRelativeToPlayer")

                ->Event("SetLightningParticleEntity", &LightningComponentRequestBus::Events::SetLightningParticleEntity)
                ->Event("GetLightningParticleEntity", &LightningComponentRequestBus::Events::GetLightningParticleEntity)
                ->VirtualProperty("LightningParticleEntity", "GetLightningParticleEntity", "SetLightningParticleEntity")

                ->Event("SetLightEntity", &LightningComponentRequestBus::Events::SetLightEntity)
                ->Event("GetLightEntity", &LightningComponentRequestBus::Events::GetLightEntity)
                ->VirtualProperty("LightEntity", "GetLightEntity", "SetLightEntity")

                ->Event("SetSkyHighlightEntity", &LightningComponentRequestBus::Events::SetSkyHighlightEntity)
                ->Event("GetSkyHighlightEntity", &LightningComponentRequestBus::Events::GetSkyHighlightEntity)
                ->VirtualProperty("SkyHighlightEntity", "GetSkyHighlightEntity", "SetSkyHighlightEntity")

                ->Event("SetAudioEntity", &LightningComponentRequestBus::Events::SetAudioEntity)
                ->Event("GetAudioEntity", &LightningComponentRequestBus::Events::GetAudioEntity)
                ->VirtualProperty("AudioThunderEntity", "GetAudioEntity", "SetAudioEntity")

                ->Event("SetSpeedOfSoundScale", &LightningComponentRequestBus::Events::SetSpeedOfSoundScale)
                ->Event("GetSpeedOfSoundScale", &LightningComponentRequestBus::Events::GetSpeedOfSoundScale)
                ->VirtualProperty("SpeedOfSoundScale", "GetSpeedOfSoundScale", "SetSpeedOfSoundScale")

                ->Event("SetLightRadiusVariation", &LightningComponentRequestBus::Events::SetLightRadiusVariation)
                ->Event("GetLightRadiusVariation", &LightningComponentRequestBus::Events::GetLightRadiusVariation)
                ->VirtualProperty("LightRadiusVariation", "GetLightRadiusVariation", "SetLightRadiusVariation")

                ->Event("SetLightIntensityVariation", &LightningComponentRequestBus::Events::SetLightIntensityVariation)
                ->Event("GetLightIntensityVariation", &LightningComponentRequestBus::Events::GetLightIntensityVariation)
                ->VirtualProperty("LightIntensityVariation", "GetLightIntensityVariation", "SetLightIntensityVariation")

                ->Event("SetParticleSizeVariation", &LightningComponentRequestBus::Events::SetParticleSizeVariation)
                ->Event("GetParticleSizeVariation", &LightningComponentRequestBus::Events::GetParticleSizeVariation)
                ->VirtualProperty("ParticleSizeVariation", "GetParticleSizeVariation", "SetParticleSizeVariation")

                ->Event("SetLightningDuration", &LightningComponentRequestBus::Events::SetLightningDuration)
                ->Event("GetLightningDuration", &LightningComponentRequestBus::Events::GetLightningDuration)
                ->VirtualProperty("LightningDuration", "GetLightningDuration", "SetLightningDuration")
                ;

            behaviorContext->Class<LightningComponent>()->RequestBus("LightningComponentRequestBus");
        }
    }

    void LightningComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<LightningComponent, AZ::Component>()
                ->Version(1)
                ->Field("m_config", &LightningComponent::m_config)
                ;
        }

        LightningConfiguration::Reflect(context);
    }


    void LightningComponent::Activate()
    {
        LightningComponentRequestBus::Handler::BusConnect(GetEntityId());
        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        
        //Only connect to the tick bus if this should be active on start
        if (m_config.m_startOnActivate)
        {
            m_active = true;
            AZ::TickBus::Handler::BusConnect();
        }

        //Set some defaults
        m_stopStrike = false;
        m_striking = false;
        m_lightFade = 0.0f;
        m_lightIntensity = 0.0f;
        m_strikeOffset = AZ::Vector3::CreateZero();

        m_currentTimePoint = 0;
        m_strikeStopTimePoint = 0;
        m_thunderStartTimePoint = 0;
        m_thunderStopTimePoint = 0;

        m_skyHighlightPosition = AZ::Vector3::CreateZero();

        //Retrieve the starting world position
        AZ::Transform worldTransform;
        AZ::TransformBus::EventResult(worldTransform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
        m_worldPos = worldTransform.GetPosition();

        //Grab original values on the first tick of the tick bus.
        //We have to do this on the first tick because we're relying on other entity's components.
        //There is no guarantee at this point that those entities will be activated and available to query.
        //However on the first tick of the tick bus, everything should be available.
        AZStd::function<void()> onFirstTick =
            [this]()
        {
            m_strikeLocalTM = AZ::Transform::CreateIdentity();

            //Retrieve original particle settings
            RetrieveOriginalParticleSettings();

            //Retrieve original light settings
            RetrieveOriginalLightSettings();

            //Retrieve original sky highlight settings
            RetrieveOriginalSkyHighlightSettings();
        };

        AZ::TickBus::QueueFunction(onFirstTick);
    }

    void LightningComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect(GetEntityId());
        LightningComponentRequestBus::Handler::BusDisconnect(GetEntityId());
        
        StopStrike();
    }

    void LightningComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_worldPos = world.GetPosition();
    }

    void LightningComponent::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
        m_lightIntensity -= (m_lightFade * deltaTime);

        if (m_lightIntensity <= 0.0f)
        {
            if (m_stopStrike)
            {
                StopStrike();

                //Destroy the entity and all referenced entities now that we've completed the effect
                Destroy();
                return;
            }

            m_lightIntensity = CalculateLightIntensity();
            m_lightFade = CalculateLightFade();
        }

        /*
        * In Legacy Cry LUA Lightning timing was handled mostly by a Timer class.
        * We have no such replacement class so instead we're managing everything
        * in this OnTick interface. Basically whenever we need to schedule an
        * event to happen X number of seconds in the future we just check here if
        * we've passed that time point or not.
        */

        m_currentTimePoint = time.GetSeconds();

        if (!m_striking)
        {
            Strike();
        }

        if (m_currentTimePoint >= m_strikeStopTimePoint && m_striking)
        {
            m_stopStrike = true; //Strike will be told to stop in the next series of ticks
            //We don't stop it immediately; we want to wait for it to properly fade out
        }

        if (m_currentTimePoint >= m_thunderStartTimePoint)
        {
            //Play thunder
            if (m_config.m_audioThunderEntity.IsValid() && m_thunderPlaying == false)
            {
                LmbrCentral::AudioTriggerComponentRequestBus::Event(m_config.m_audioThunderEntity, &LmbrCentral::AudioTriggerComponentRequestBus::Events::Play);
                m_thunderPlaying = true;
            }

            m_thunderStartTimePoint = 0; //Reset this timePoint so it will only fire again if a new time point is set
        }

        //Update the lighting if we know that a strike has started but not stopped
        if (m_striking)
        {
            //Update light-related entities
            UpdateLight();
            UpdateSkyHighlight();
        }
    }

    void LightningComponent::StartEffect()
    {
        //If we haven't already connected to the tick bus, connect
        //We are now active and the lightning effect will execute
        if (!m_active)
        {
            m_active = true;
            AZ::TickBus::Handler::BusConnect();
        }
    }

    void LightningComponent::SetLightningParticleEntity(AZ::EntityId particleEntity)
    {
        m_config.m_lightningParticleEntity = particleEntity;

        RetrieveOriginalParticleSettings();
    }

    void LightningComponent::SetLightEntity(AZ::EntityId lightEntity)
    {
        m_config.m_lightEntity = lightEntity;

        RetrieveOriginalLightSettings();
    }

    void LightningComponent::SetSkyHighlightEntity(AZ::EntityId skyHighlightEntity) 
    {
        m_config.m_skyHighlightEntity = skyHighlightEntity;

        RetrieveOriginalSkyHighlightSettings();
    }

    void LightningComponent::StopStrike()
    {
        //Turn off particle
        if (m_config.m_lightningParticleEntity.IsValid())
        {
            LmbrCentral::ParticleComponentRequestBus::Event(m_config.m_lightningParticleEntity, &LmbrCentral::ParticleComponentRequestBus::Events::Enable, false);
        }

        TurnOffLight();

        //Zero out the sky highlight
        TurnOffSkyHighlight();
    }

    void LightningComponent::Strike()
    {
        //Don't execute if we're already rendering a strike
        if (m_striking == true)
        {
            return;
        }

        //Tell the given light entity to turn on. It will be told to turn off in StopStrike
        TurnOnLight();
        TurnOnSkyHighlight();

        m_striking = true;

        m_lightIntensity = CalculateLightIntensity();
        m_lightFade = CalculateLightFade();

        AZ::Vector3 cameraPos = AZ::Vector3::CreateZero();
        AZ::Vector3 cameraViewDir = AZ::Vector3::CreateZero();

        if (gEnv->pSystem)
        {
            CCamera& camera = gEnv->pSystem->GetViewCamera();
            Matrix34 cameraMatrix = camera.GetMatrix();

            cameraPos = LYVec3ToAZVec3(camera.GetPosition());
            cameraViewDir = LYVec3ToAZVec3(cameraMatrix.GetColumn(1));
        }

        AZ::Vector3 strikePos = m_worldPos;

        m_strikeOffset.SetX(0);
        m_strikeOffset.SetY(0);

        if (m_config.m_relativeToPlayer)
        {
            if (gEnv->pSystem)
            {
                CCamera& camera = gEnv->pSystem->GetViewCamera();
                Matrix34 cameraMatrix = camera.GetMatrix();

                cameraViewDir = LYVec3ToAZVec3(cameraMatrix.GetColumn(1));
            }

            m_strikeOffset.SetX(m_strikeOffset.GetX() + (cameraPos.GetX() - m_worldPos.GetX()));
            m_strikeOffset.SetY(m_strikeOffset.GetY() + (cameraPos.GetY() - m_worldPos.GetY()));
        }

        //Find distance to the camera from the light strike (xy plane only)
        AZ::Vector3 difference = AZ::Vector3::CreateZero();
        difference = cameraPos - (m_worldPos + m_strikeOffset);

        float toCameraDistance = sqrtf(powf(difference.GetX(), 2) + powf(difference.GetY(), 2));

        m_skyHighlightPosition = AZ::Vector3::CreateZero();
        m_skyHighlightPosition.SetX(m_worldPos.GetX() + m_strikeOffset.GetX());
        m_skyHighlightPosition.SetY(m_worldPos.GetY() + m_strikeOffset.GetY());
        m_skyHighlightPosition.SetZ(m_worldPos.GetZ());

        m_strikeLocalTM.SetTranslation(m_strikeOffset);

        //Update the particle component
        if (m_config.m_lightningParticleEntity.IsValid())
        {
            //Scale the particle effect to keep the same size regardless of the distance to the lightning
            float particleScale = GetValueWithVariation(m_originalParticleScale, m_config.m_particleSizeVariation) * toCameraDistance;

            //Adjust particle entity as necessary before re-enabling
            AZ::TransformBus::Event(m_config.m_lightningParticleEntity, &AZ::TransformBus::Events::SetLocalTM, m_strikeLocalTM);

            LmbrCentral::ParticleComponentRequestBus::Event(m_config.m_lightningParticleEntity, &LmbrCentral::ParticleComponentRequestBus::Events::SetGlobalSizeScale, particleScale);
            LmbrCentral::ParticleComponentRequestBus::Event(m_config.m_lightningParticleEntity, &LmbrCentral::ParticleComponentRequestBus::Events::Enable, true);
        }

        //Update light-related entities
        UpdateLight();
        UpdateSkyHighlight();

        //Set the audio distance RTPC value
        if (m_config.m_audioThunderEntity.IsValid())
        {
            LmbrCentral::AudioRtpcComponentRequestBus::Event(m_config.m_audioThunderEntity, &LmbrCentral::AudioRtpcComponentRequestBus::Events::SetValue, toCameraDistance);
        }

        //Determine timing for thunder
        //340.29 meters per second is the speed of sound
        double thunderTimeDelay = (toCameraDistance / 340.29) * m_config.m_speedOfSoundScale;
        m_thunderStartTimePoint = m_currentTimePoint + thunderTimeDelay;

        //Determine timing for lightning stop event
        m_strikeStopTimePoint = m_currentTimePoint + GetValueWithVariation(m_config.m_lightningDuration, 0.5);
    }

    void LightningComponent::RetrieveOriginalParticleSettings()
    {
        if (m_config.m_lightningParticleEntity.IsValid())
        {
            m_originalParticleScale = 1.0f;
            LmbrCentral::ParticleComponentRequestBus::EventResult(m_originalParticleScale, GetEntityId(), &LmbrCentral::ParticleComponentRequestBus::Events::GetGlobalSizeScale);
        }
    }
    void LightningComponent::RetrieveOriginalSkyHighlightSettings()
    {
        m_originalSkyColor = AZ::Color::CreateZero();
        m_originalSkyColorMultiplier = 1.0f;

        if (m_config.m_skyHighlightEntity.IsValid())
        {
            SkyHighlightComponentRequestBus::EventResult(m_originalSkyColor, m_config.m_skyHighlightEntity, &SkyHighlightComponentRequestBus::Events::GetColor);
            SkyHighlightComponentRequestBus::EventResult(m_originalSkyColorMultiplier, m_config.m_skyHighlightEntity, &SkyHighlightComponentRequestBus::Events::GetColorMultiplier);
        }
    }
    void LightningComponent::RetrieveOriginalLightSettings()
    {
        m_originalLightRadius = 2.0f;
        m_originalLightDiffuseMultiplier = 1.0f;
        m_originalLightSpecularMultiplier = 1.0f;

        if (m_config.m_lightEntity.IsValid())
        {
            LmbrCentral::LightComponentRequestBus::EventResult(m_originalLightRadius, m_config.m_lightEntity, &LmbrCentral::LightComponentRequestBus::Events::GetPointMaxDistance);
            LmbrCentral::LightComponentRequestBus::EventResult(m_originalLightDiffuseMultiplier, m_config.m_lightEntity, &LmbrCentral::LightComponentRequestBus::Events::GetDiffuseMultiplier);
            LmbrCentral::LightComponentRequestBus::EventResult(m_originalLightSpecularMultiplier, m_config.m_lightEntity, &LmbrCentral::LightComponentRequestBus::Events::GetSpecularMultiplier);
        }
    }

    void LightningComponent::TurnOnSkyHighlight()
    {
        if (m_config.m_skyHighlightEntity.IsValid())
        {
            SkyHighlightComponentRequestBus::Event(m_config.m_skyHighlightEntity, &SkyHighlightComponentRequestBus::Events::Enable);
        }
    }

    void LightningComponent::UpdateSkyHighlight()
    {
        if (m_config.m_skyHighlightEntity.IsValid())
        {          
            //Adjust sky highlight transform
            AZ::TransformBus::Event(m_config.m_skyHighlightEntity, &AZ::TransformBus::Events::SetLocalTM, m_strikeLocalTM);

            float highlightIntensity = m_lightIntensity * m_originalSkyColorMultiplier;
            AZ::Color skyHighlightColor = m_originalSkyColor * highlightIntensity;

            SkyHighlightComponentRequestBus::Event(m_config.m_skyHighlightEntity, &SkyHighlightComponentRequestBus::Events::SetColor, skyHighlightColor);
        }
    }

    void LightningComponent::TurnOffSkyHighlight()
    {
        if (m_config.m_skyHighlightEntity.IsValid())
        {
            SkyHighlightComponentRequestBus::Event(m_config.m_skyHighlightEntity, &SkyHighlightComponentRequestBus::Events::Disable);
        }
    }

    void LightningComponent::TurnOnLight()
    {
        if (m_config.m_lightEntity.IsValid())
        {
            LmbrCentral::LightComponentRequestBus::Event(m_config.m_lightEntity, &LmbrCentral::LightComponentRequestBus::Events::TurnOnLight);
        }
    }
    void LightningComponent::UpdateLight()
    {
        if (m_config.m_lightEntity.IsValid())
        {
            //Adjust light transform
            AZ::TransformBus::Event(m_config.m_lightEntity, &AZ::TransformBus::Events::SetLocalTM, m_strikeLocalTM);

            //Modify light settings
            float lightVariation = GetValueWithVariation(m_lightIntensity, m_config.m_lightIntensityVariation);
            float lightDiffuse =  m_originalLightDiffuseMultiplier * lightVariation;
            float lightSpecular = m_originalLightSpecularMultiplier * lightVariation;
            float lightRadius = GetValueWithVariation(m_originalLightRadius, m_config.m_lightRadiusVariation);

            LmbrCentral::LightComponentRequestBus::Event(m_config.m_lightEntity, &LmbrCentral::LightComponentRequestBus::Events::SetDiffuseMultiplier, lightDiffuse);
            LmbrCentral::LightComponentRequestBus::Event(m_config.m_lightEntity, &LmbrCentral::LightComponentRequestBus::Events::SetSpecularMultiplier, lightSpecular);
            LmbrCentral::LightComponentRequestBus::Event(m_config.m_lightEntity, &LmbrCentral::LightComponentRequestBus::Events::SetPointMaxDistance, lightRadius);
        }
    }
    void LightningComponent::TurnOffLight()
    {
        if (m_config.m_lightEntity.IsValid())
        {
            LmbrCentral::LightComponentRequestBus::Event(m_config.m_lightEntity, &LmbrCentral::LightComponentRequestBus::Events::TurnOffLight);
        }
    }

    void LightningComponent::Destroy()
    {
        //Delete all referenced entities
        AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::DeleteEntity, m_config.m_lightningParticleEntity);
        AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::DeleteEntity, m_config.m_lightEntity);
        AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::DeleteEntity, m_config.m_skyHighlightEntity);
        AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::DeleteEntity, m_config.m_audioThunderEntity);

        //Delete lightning entity
        AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::DeleteEntity, GetEntityId());
    }

} // namespace Lightning