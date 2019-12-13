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

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>

#include <AzFramework/Entity/EntityDebugDisplayBus.h>

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/TickBus.h>

#include <LegacyEntityConversion/LegacyEntityConversionBus.h>

#include "Lightning/LightningComponentBus.h"
#include "LightningComponent.h"

namespace Lightning
{    
    class LightningConverter;
    class EditorLightningComponent;

    /**
     * Editor side of the Lightning Configuration
     */
    class EditorLightningConfiguration 
        : public LightningConfiguration
    {
    public:
        AZ_TYPE_INFO_LEGACY(EditorLightningConfiguration, "{7D8B140F-D2C9-573A-94CE-A7884079BF09}", LightningConfiguration);
        AZ_CLASS_ALLOCATOR(EditorLightningConfiguration, AZ::SystemAllocator,0);

        static void Reflect(AZ::ReflectContext* context);
    };

    /**
     * Editor side of the Lightning Component
     */
    class EditorLightningComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , public LightningComponentRequestBus::Handler
    {
        friend class LightningConverter;

    public:
        AZ_COMPONENT(EditorLightningComponent, "{50FDA3BF-3B84-4EF5-BB87-00C17AC963D7}", AzToolsFramework::Components::EditorComponentBase);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provides);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void Reflect(AZ::ReflectContext* context);

        ~EditorLightningComponent() = default;

        // LightningComponentRequestBus implementation
        void StartEffect() {/*Do nothing for the editor impl*/}

        void SetStartOnActivate(bool startOnActivate) override { m_config.m_startOnActivate = startOnActivate; }
        bool GetStartOnActivate() override { return m_config.m_startOnActivate; }

        void SetRelativeToPlayer(bool relativeToPlayer) override { m_config.m_relativeToPlayer = relativeToPlayer; }
        bool GetRelativeToPlayer() override { return m_config.m_relativeToPlayer; }

        void SetLightningParticleEntity(AZ::EntityId particleEntity) override { m_config.m_lightningParticleEntity = particleEntity; }
        AZ::EntityId GetLightningParticleEntity() override { return m_config.m_lightningParticleEntity; }

        void SetLightEntity(AZ::EntityId lightEntity) override { m_config.m_lightEntity = lightEntity; }
        AZ::EntityId GetLightEntity() override { return m_config.m_lightEntity; }

        void SetSkyHighlightEntity(AZ::EntityId skyHighlightEntity) override { m_config.m_skyHighlightEntity = skyHighlightEntity; }
        AZ::EntityId GetSkyHighlightEntity() override { return m_config.m_skyHighlightEntity; }

        void SetAudioEntity(AZ::EntityId audioEntity) override { m_config.m_audioThunderEntity = audioEntity; }
        AZ::EntityId GetAudioEntity() override { return m_config.m_audioThunderEntity; }

        void SetSpeedOfSoundScale(float speedOfSoundScale) override { m_config.m_speedOfSoundScale = speedOfSoundScale; }
        float GetSpeedOfSoundScale() override { return m_config.m_speedOfSoundScale; }

        void SetLightRadiusVariation(float lightRadiusVariation) override { m_config.m_lightRadiusVariation = lightRadiusVariation; }
        float GetLightRadiusVariation() override { return m_config.m_lightRadiusVariation; }

        void SetLightIntensityVariation(float lightIntensityVariation) override { m_config.m_lightIntensityVariation = lightIntensityVariation; }
        float GetLightIntensityVariation() override { return m_config.m_lightIntensityVariation; }

        void SetParticleSizeVariation(float particleSizeVariation) override { m_config.m_particleSizeVariation = particleSizeVariation; }
        float GetParticleSizeVariation() override { return m_config.m_particleSizeVariation; }

        void SetLightningDuration(double lightningDuration) override { m_config.m_lightningDuration = lightningDuration; }
        double GetLightningDuration() override { return m_config.m_lightningDuration; }
        
        void BuildGameEntity(AZ::Entity* gameEntity);

    protected:
        EditorLightningConfiguration m_config; ///< Serialized configuration for the component
    };

    /**
     * Converter for legacy lightning entities
     *
     * Technically lightning entities don't directly convert to Lightning components 
     * but this seemed like the right place to keep this converter. Instead lightning
     * entities are broken up into a series of components to be more modular. 
     */
    class LightningConverter : public AZ::LegacyConversion::LegacyConversionEventBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(LightningConverter, AZ::SystemAllocator, 0);

        LightningConverter() {}

        // ------------------- LegacyConversionEventBus::Handler -----------------------------
        AZ::LegacyConversion::LegacyConversionResult ConvertEntity(CBaseObject* pEntityToConvert) override;
        bool BeforeConversionBegins() override;
        bool AfterConversionEnds() override;
        // END ----------------LegacyConversionEventBus::Handler ------------------------------
    };
} // namespace Lightning
