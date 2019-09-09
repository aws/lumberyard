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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include <LegacyEntityConversion/LegacyEntityConversionBus.h>

#include "LightningArc/LightningArcComponentBus.h"
#include "LightningArcComponent.h"

namespace Lightning
{
    class LightningArcConverter;
    class EditorLightningArcComponent;

    class EditorLightningArcConfiguration
        : public LightningArcConfiguration
    {
    public:
        AZ_TYPE_INFO(EditorLightningArcConfiguration, "{61810E84-F74E-4EFF-8B23-EB1097C6E1BB}", LightningArcConfiguration);
        AZ_CLASS_ALLOCATOR(EditorLightningArcConfiguration, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* context);
        
        //Only exists in the Editor configuration because we extract params from this preset
        //or other parameter modifications and pass that to the runtime component
        AZStd::string m_arcPresetName = "Default"; 

        AZ::Crc32 OnPresetNameChange() override;
        AZ::Crc32 OnParamsChange() override;
    };

    class EditorLightningArcComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , public AzFramework::EntityDebugDisplayEventBus::Handler
        , public LightningArcComponentRequestBus::Handler
        , public LmbrCentral::MaterialOwnerRequestBus::Handler
    {
        friend class LightningArcConverter;
        friend class EditorLightningArcConfiguration;

    public:
        AZ_COMPONENT(EditorLightningArcComponent, "{A63711FB-0205-4DE7-9B41-B9291E1585BA}", AzToolsFramework::Components::EditorComponentBase);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provides);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void Reflect(AZ::ReflectContext* context);

        ~EditorLightningArcComponent() = default;

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        void BuildGameEntity(AZ::Entity* gameEntity);

        // LightningArcComponentRequestBus::Handler
        void Enable() override { m_config.m_enabled = true; }
        void Disable() override { m_config.m_enabled = false; }
        void Toggle() override { m_config.m_enabled = !m_config.m_enabled; }
        bool IsEnabled() override { return m_config.m_enabled; };

        void SetTargets(const AZStd::vector<AZ::EntityId>& targets) override { m_config.m_targets = targets; }
        const AZStd::vector<AZ::EntityId>& GetTargets() override { return m_config.m_targets; }

        void SetDelay(double delay) override { m_config.m_delay = delay; }
        double GetDelay() override { return m_config.m_delay; }

        void SetDelayVariation(double delayVariation) override { m_config.m_delay = delayVariation; }
        double GetDelayVariation() override { return m_config.m_delayVariation; }

        void SetStrikeTimeMin(float strikeTimeMin) override { m_config.m_arcParams.m_strikeTimeMin = strikeTimeMin; }
        float GetStrikeTimeMin() override { return m_config.m_arcParams.m_strikeTimeMin; }

        void SetStrikeTimeMax(float strikeTimeMax) override { m_config.m_arcParams.m_strikeTimeMax = strikeTimeMax; }
        float GetStrikeTimeMax() override { return m_config.m_arcParams.m_strikeTimeMax; }

        void SetStrikeFadeOut(float strikeFadeOut) override { m_config.m_arcParams.m_strikeFadeOut = strikeFadeOut; }
        float GetStrikeFadeOut() override { return m_config.m_arcParams.m_strikeFadeOut; }

        void SetStrikeSegmentCount(AZ::u32 strikeSegmentCount) override { m_config.m_arcParams.m_strikeNumSegments = strikeSegmentCount; }
        AZ::u32 GetStrikeSegmentCount() override { return m_config.m_arcParams.m_strikeNumSegments; }

        void SetStrikePointCount(AZ::u32 strikePointCount) override { m_config.m_arcParams.m_strikeNumPoints = strikePointCount; }
        AZ::u32 GetStrikePointCount() override { return m_config.m_arcParams.m_strikeNumPoints; }

        void SetLightningDeviation(float deviation) override { m_config.m_arcParams.m_lightningDeviation = deviation; }
        float GetLightningDeviation() override { return m_config.m_arcParams.m_lightningDeviation; }

        void SetLightningFuzziness(float fuzziness) override { m_config.m_arcParams.m_lightningFuzziness = fuzziness; }
        float GetLightningFuzziness() override { return m_config.m_arcParams.m_lightningFuzziness; }

        void SetLightningVelocity(float velocity) override { m_config.m_arcParams.m_lightningVelocity = velocity; }
        float GetLightningVelocity() override { return m_config.m_arcParams.m_lightningVelocity; }

        void SetBranchProbablity(float branchProbability) override { m_config.m_arcParams.m_branchProbability = branchProbability; }
        float GetBranchProbablity() override { return m_config.m_arcParams.m_branchProbability; }

        void SetBranchMaxLevel(AZ::u32 branchMaxLevel) override { m_config.m_arcParams.m_branchMaxLevel = branchMaxLevel; }
        AZ::u32 GetBranchMaxLevel() override { return m_config.m_arcParams.m_branchMaxLevel; }

        void SetMaxStrikeCount(AZ::u32 maxStrikeCount) override { m_config.m_arcParams.m_maxNumStrikes = maxStrikeCount; }
        AZ::u32 GetMaxStrikeCount() override { return m_config.m_arcParams.m_maxNumStrikes; }

        void SetBeamSize(float beamSize) override { m_config.m_arcParams.m_beamSize = beamSize; }
        float GetBeamSize() override { return m_config.m_arcParams.m_beamSize; }

        void SetBeamTexTiling(float beamTexTiling) override { m_config.m_arcParams.m_beamTexTiling = beamTexTiling; }
        float GetBeamTexTiling() override { return m_config.m_arcParams.m_beamTexTiling; }

        void SetBeamTexShift(float beamTexShift) override { m_config.m_arcParams.m_beamTexShift = beamTexShift; }
        float GetBeamTexShift() override { return m_config.m_arcParams.m_beamTexShift; }

        void SetBeamTexFrames(float beamTexFrames) override { m_config.m_arcParams.m_beamTexFrames = beamTexFrames; }
        float GetBeamTexFrames() override { return m_config.m_arcParams.m_beamTexFrames; }

        void SetBeamTexFPS(float beamTexFPS) override { m_config.m_arcParams.m_beamTexFPS = beamTexFPS; }
        float GetBeamTexFPS() override { return m_config.m_arcParams.m_beamTexFPS; }

        // MaterialRequestBus
        void SetMaterial(_smart_ptr<IMaterial> material) override;
        _smart_ptr<IMaterial> GetMaterial();

        // EntityDebugDisplayBus
        void DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;

    private:
        //Reflected data
        EditorLightningArcConfiguration m_config;
        bool m_refreshPresets; //Value doesn't actually matter; used for a button

        AZ::Crc32 RefreshPresets();

        static AZStd::vector<AZStd::string> GetPresetNames();
        static AZStd::vector<AZStd::string> s_presetNames;
        static AZStd::string s_customFallbackName;
    };

    /**
    * Converter for legacy lightning arc entities
    */
    class LightningArcConverter : public AZ::LegacyConversion::LegacyConversionEventBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(LightningArcConverter, AZ::SystemAllocator, 0);

        LightningArcConverter() {}

        // ------------------- LegacyConversionEventBus::Handler -----------------------------
        AZ::LegacyConversion::LegacyConversionResult ConvertEntity(CBaseObject* entityToConvert) override;
        bool BeforeConversionBegins() override;
        bool AfterConversionEnds() override;
        // END ----------------LegacyConversionEventBus::Handler ------------------------------
    };
} //namespace Lightning