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

#include <LmbrCentral/Rendering/MaterialAsset.h>
#include <Cry3DEngine/Environment/OceanEnvironmentBus.h>

namespace AZ
{
    class Vector3;
}

namespace Water
{
    /**
      * The data model for the Ocean Component (both Editor and Runtime versions)
      */      
    struct WaterOceanComponentData 
        : private AZ::OceanEnvironmentBus::Handler
    {
        AZ_RTTI(WaterOceanComponentData, "{2C999C70-328C-44B1-A499-4A8BC9EAC159}");

        struct General
        {
            AZ_RTTI(General, "{3218DB33-BF7C-4508-9A95-2B7561CA1497}");
            
            General();
            virtual ~General() = default;
            
            static void Reflect(AZ::ReflectContext* context);

            float m_height = AZ::OceanConstants::s_DefaultHeight; ///< the plane's height in the world, cached from the transform component and not serialized.
            bool m_useOceanBottom = true; ///< if FALSE then the ocean bottom will not render
            AzFramework::SimpleAssetReference<LmbrCentral::MaterialAsset> m_oceanMaterialAsset; ///< the material asset for the ocean

        private:
            static bool VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
            void OnOceanMaterialChanged();
        };

        struct Caustics
        {
            AZ_RTTI(Caustics, "{376EE3DF-0225-4AB1-B50A-12852D961C3F}");
            
            Caustics() = default;
            virtual ~Caustics() = default;
            
            static void Reflect(AZ::ReflectContext* context);

            bool  m_enabled = true; ///< false/true turns off/on water caustics effects on surfaces below the water
            float m_distanceAttenuation = AZ::OceanConstants::s_CausticsDistanceAttenDefault; ///< the fall off range to render ocean caustics in meters
            float m_depth = AZ::OceanConstants::s_CausticsDepthDefault; ///< The depth below the ocean that caustics are calculated. This appears to be in meters. There's a hard cut-off where the calculation stops that looks ugly, it would be better to lerp this off.
            float m_intensity = AZ::OceanConstants::s_CausticsIntensityDefault; ///< The intensity of the light in caustics. 0 turns caustics off completely. Near-zero values still show caustics rather strongly, so going from 0.01 to 0.0 is a sudden change. This should be fixed.
            float m_tiling = AZ::OceanConstants::s_CausticsTilingDefault; ///< Affects the tiling of the caustics texture
        };

        struct Animation
        {
            AZ_RTTI(Animation, "{2EC40FEA-8691-42A0-9F71-205A35B4EC4F}");
            
            Animation() = default;
            virtual ~Animation() = default;
            
            static void Reflect(AZ::ReflectContext* context);

            float m_wavesAmount = AZ::OceanConstants::s_animationWavesAmountDefault; ///< The frequency of the waves - higher numbers makes waves closer together.
            float m_wavesSize = AZ::OceanConstants::s_animationWavesSizeDefault; ///< The height of the waves in meters
            float m_wavesSpeed = AZ::OceanConstants::s_animationWavesSpeedDefault; ///< A multiplier on the speed that the waves animate
            float m_windDirection = AZ::OceanConstants::s_animationWindDirectionDefault; ///< The direction of the wind affecting the detail texture of the waves (in radians)
            float m_windSpeed = AZ::OceanConstants::s_animationWindSpeedDefault; ///< The speed of the wind (possibly in centimeters per second)
        };

        struct Reflection
        {
            AZ_RTTI(Reflection, "{7E602EC1-B6C9-4112-96C7-CECA8C448E11}");

            Reflection() = default;
            virtual ~Reflection() = default;
            
            static void Reflect(AZ::ReflectContext* context);

            float m_resolutionScale = 0.5f; ///< the scale of the screen resolution to use for ocean reflections
            bool m_entities = true; ///< The ocean reflects entities.
            bool m_staticObjects = true; ///< The ocean reflects static mesh objects
            bool m_terrainDetailMaterials = true; ///< The ocean reflects the terrain detail materials
            bool m_particles = true; ///< The ocean reflects particles
            bool m_anisotropic = true; ///< renders ocean reflections using an anisotropic filter
        };

        struct Fog
        {
            AZ_RTTI(Fog, "{022C7A5C-1A04-4424-AFFB-256994321185}");

            Fog() = default;
            virtual ~Fog() = default;

            static void Reflect(AZ::ReflectContext* context);

            AZ::Color m_color = AZ::OceanConstants::s_oceanFogColorDefault; ///< The color of the ocean fog
            AZ::Color m_nearFogColor = AZ::OceanConstants::s_oceanNearFogColorDefault; ///< The color of the ocean fog when it is near shallow depths
            float m_colorMultiplier = AZ::OceanConstants::s_oceanFogColorMultiplierDefault; ///< A multiplier applied to the color of the ocean fog
            float m_density = AZ::OceanConstants::s_oceanFogDensityDefault; ///< The density of the ocean fog
        };

        struct Advanced
        {
            AZ_RTTI(Advanced, "{2AACAF8C-AD1F-44A5-B5C8-D329F41F2853}");
            
            Advanced() = default;
            virtual ~Advanced() = default;
            
            static void Reflect(AZ::ReflectContext* context);

            int m_waterTessellationAmount = AZ::OceanConstants::s_waterTessellationDefault; ///< Sets the amount of water tessellation
            bool m_godraysEnabled = AZ::OceanConstants::s_GodRaysEnabled; ///< 0/1 turns off/on rays of light coming down from the surface of the water. Only visible when camera is below water.
            float m_underwaterDistortion = AZ::OceanConstants::s_UnderwaterDistortion; ///< A multiplier on the distortion of the background while underwater.
        };

        static void Reflect(AZ::ReflectContext* context);

        WaterOceanComponentData();
        virtual ~WaterOceanComponentData();

        void Activate();
        void Deactivate();

        ////////////////////////////////////////////////////////////////////////
        // AZ::OceanEnvironmentBus::Handler
        bool OceanIsEnabled() const override;
        float GetOceanLevel() const override;
        float GetOceanLevelOrDefault(const float defaultValue) const override;
        float GetWaterLevel(const Vec3& pvPos) const override;
        float GetAccurateOceanHeight(const Vec3& pCurrPos) const override;
        void SetOceanLevel(float oceanLevel) override;

        int GetWaterTessellationAmount() const override;
        void SetWaterTessellationAmount(int amount) override;

        bool GetUseOceanBottom() const override;
        void SetUseOceanBottom(bool use) override;

        const AZStd::string& GetOceanMaterialName() const override;
        void SetOceanMaterialName(const AZStd::string& matName) override;

        float GetAnimationWindDirection() const override;
        float GetAnimationWindSpeed() const override;
        float GetAnimationWavesSpeed() const override;
        float GetAnimationWavesSize() const override;
        float GetAnimationWavesAmount() const override;
        void SetAnimationWindDirection(float dir) override;
        void SetAnimationWindSpeed(float speed) override;
        void SetAnimationWavesSpeed(float speed) override;
        void SetAnimationWavesSize(float size) override;
        void SetAnimationWavesAmount(float amount) override;

        void ApplyReflectRenderFlags(int& flags) const override;
        bool GetReflectionAnisotropic() const override;
        float GetReflectResolutionScale() const override;
        bool GetReflectRenderFlag(ReflectionFlags flag) const override;
        void SetReflectRenderFlag(ReflectionFlags flag, bool value) override;
        void SetReflectResolutionScale(float scale) override;
        void SetReflectionAnisotropic(bool enabled) override;

        bool GetGodRaysEnabled() const override;
        void SetGodRaysEnabled(bool enabled) override;

        float GetUnderwaterDistortion() const override;
        void SetUnderwaterDistortion(float) override;

        bool  GetCausticsEnabled() const override;
        float GetCausticsDepth() const override;
        float GetCausticsIntensity() const override;
        float GetCausticsTiling() const override;
        float GetCausticsDistanceAttenuation() const override;
        void SetCausticsEnabled(bool enable) override;
        void SetCausticsDepth(float depth) override;
        void SetCausticsIntensity(float intensity) override;
        void SetCausticsTiling(float tiling) override;
        void SetCausticsDistanceAttenuation(float dist) override;

        AZ::Color GetFogColor() const override;
        float GetFogColorMulitplier() const override;
        AZ::Color GetNearFogColor() const override;
        AZ::Color GetFogColorPremultiplied() const override;
        float GetFogDensity() const override;
        void SetFogColor(const AZ::Color& fogColor) override;
        void SetFogColorMulitplier(float fogMulitplier) override;
        void SetNearFogColor(const AZ::Color& nearColor) override;
        void SetFogDensity(float density) override;

    private:
        bool m_enabled;
        General m_general;
        Caustics m_caustics;
        Animation m_animation;
        Reflection m_reflection;
        Fog m_fog;
        Advanced m_advanced;
    };
}
