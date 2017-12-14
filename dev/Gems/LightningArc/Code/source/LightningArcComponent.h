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

#include <AzCore/std/string/string.h>

#include <AzCore/Math/Random.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/TickBus.h>

#include <LmbrCentral/Rendering/MaterialAsset.h>
#include <LmbrCentral/Rendering/MaterialOwnerBus.h> //For MaterialRequestBus
#include <IMaterial.h>

#include "LightningArc/LightningArcComponentBus.h"
#include "LightningArc/LightningArcBus.h"

#include "LightningGameEffectCommon.h"

namespace Lightning
{
    class LightningArcConfiguration
    {
    public:
        AZ_TYPE_INFO(LightningArcConfiguration, "{8DFA58D6-305A-427E-A711-9F1F66DA5D0E}");
        AZ_CLASS_ALLOCATOR(LightningArcConfiguration, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* context);

        virtual ~LightningArcConfiguration() = default;

        bool m_enabled = true;
        AZStd::vector<AZ::EntityId> m_targets;
        AzFramework::SimpleAssetReference<LmbrCentral::MaterialAsset> m_materialAsset;
        LightningArcParams m_arcParams;
        double m_delay = 2.0;
        double m_delayVariation = 0.5;
        
        virtual AZ::Crc32 OnPresetNameChange() { return AZ::Crc32(); }
        virtual AZ::Crc32 OnParamsChange() { return AZ::Crc32(); }
    };

    class LightningArcComponent
        : public AZ::Component
        , public LightningArcComponentRequestBus::Handler
        , public LmbrCentral::MaterialOwnerRequestBus::Handler
        , public AZ::TransformNotificationBus::Handler
        , public AZ::TickBus::Handler
    {
    public:
        AZ_COMPONENT(LightningArcComponent, "{D49C26E7-596E-4E7F-B34C-983B9E09FD38}", AZ::Component);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provides);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& requires);
        static void Reflect(AZ::ReflectContext* context);

        // Constructors / Destructors
        LightningArcComponent();
        explicit LightningArcComponent(LightningArcConfiguration *params)
        {
            m_config = *params;
        }
        ~LightningArcComponent() = default;

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // TransformNotificationBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        // LightningArcComponentRequestBus
        void Enable() override;
        void Disable() override;
        void Toggle() override;
        bool IsEnabled() override { return m_config.m_enabled; }

        void SetTargets(const AZStd::vector<AZ::EntityId>& targets) override { m_config.m_targets = targets; }
        const AZStd::vector<AZ::EntityId>& GetTargets() override { return m_config.m_targets; }

        void SetDelay(double delay) override { m_config.m_delay = delay; }
        double GetDelay() override { return m_config.m_delay; }

        void SetDelayVariation(double delayVariation) override { m_config.m_delayVariation = delayVariation; }
        double GetDelayVariation() override { return m_config.m_delayVariation; }

        void SetStrikeTimeMin(float strikeTimeMin) override { m_config.m_arcParams.m_strikeTimeMin = strikeTimeMin; }
        float GetStrikeTimeMin() override { return m_config.m_arcParams.m_strikeTimeMin; }

        void SetStrikeTimeMax(float strikeTimeMax) override { m_config.m_arcParams.m_strikeTimeMax = strikeTimeMax; }
        float GetStrikeTimeMax() override { return m_config.m_arcParams.m_strikeTimeMax; }

        void SetStrikeFadeOut(float strikeFadeOut) override { m_config.m_arcParams.m_strikeFadeOut = strikeFadeOut; }
        float GetStrikeFadeOut() override { return m_config.m_arcParams.m_strikeFadeOut; }

        void SetStrikeSegmentCount(AZ::u32 strikeSegmentCount) override;
        AZ::u32 GetStrikeSegmentCount() override { return m_config.m_arcParams.m_strikeNumSegments; }

        void SetStrikePointCount(AZ::u32 strikePointCount) override;
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

        // MaterialOwnerRequestBus
        void SetMaterial(_smart_ptr<IMaterial> material) override { m_material = material; }
        _smart_ptr<IMaterial> GetMaterial() { return m_material; }

    private:
        //Reflected data
        LightningArcConfiguration m_config;
    
        //Unreflected data
        _smart_ptr<IMaterial> m_material;
        double m_currentTimePoint;
        double m_sparkTimePoint;
        AZ::Vector3 m_currentWorldPos;
        AZ::SimpleLcgRandom m_rand;

        AZStd::shared_ptr<LightningGameEffectAZ> m_gameEffect;

        void CalculateNextSparkTime();
        void TriggerSpark();
    };
} //namespace Lightning