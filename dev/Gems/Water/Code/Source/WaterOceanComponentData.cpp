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

#include "Water_precompiled.h"

#include <Water/WaterOceanComponentData.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <Cry3DEngine/Environment/OceanEnvironmentBus.h>
#include <CryCommon/ITerrain.h>

#if WATER_GEM_EDITOR
#include "EditorDefs.h"
#include <Material/MaterialManager.h>
#endif

namespace Water
{
    static int s_numberOfOceanComponentsActivated = 0;

    void WaterOceanComponentData::Reflect(AZ::ReflectContext* context)
    {
        General::Reflect(context);
        Caustics::Reflect(context);
        Animation::Reflect(context);
        Reflection::Reflect(context);
        Fog::Reflect(context);
        Advanced::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<WaterOceanComponentData>()
                ->Version(1)
                ->Field("general", &WaterOceanComponentData::m_general)
                ->Field("animation", &WaterOceanComponentData::m_animation)
                ->Field("fog", &WaterOceanComponentData::m_fog)
                ->Field("caustics", &WaterOceanComponentData::m_caustics)
                ->Field("reflection", &WaterOceanComponentData::m_reflection)
                ->Field("advanced", &WaterOceanComponentData::m_advanced)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<WaterOceanComponentData>("Infinite Ocean", "Provides an infinite ocean. (Preview)")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WaterOceanComponentData::m_general, "General", "General ocean settings")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WaterOceanComponentData::m_animation, "Animation", "Ocean animations for the waves and texture scrolling")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WaterOceanComponentData::m_fog, "Fog", "The parameters of the ocean fog")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WaterOceanComponentData::m_caustics, "Caustics", "Ocean caustics effects on surfaces below the water")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WaterOceanComponentData::m_reflection, "Reflection", "Reflection flags and parameters for the ocean")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WaterOceanComponentData::m_advanced, "Advanced", "Advanced settings")
                    ;
            }
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->Class<WaterOceanComponentData>()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->RequestBus("OceanEnvironmentRequestBus");
        }
    }

    WaterOceanComponentData::WaterOceanComponentData()
    {
        m_enabled = false;
    }

    WaterOceanComponentData::~WaterOceanComponentData()
    {
        m_enabled = false;
    }

    _smart_ptr<IMaterial> WaterOceanComponentData_LoadMaterial(const AZStd::string& materialName)
    {
        _smart_ptr<IMaterial> pMaterial;

        if (gEnv->p3DEngine)
        {
#if WATER_GEM_EDITOR
            CMaterial* cMaterial = GetIEditor()->GetMaterialManager()->LoadMaterial(materialName.c_str(), false);
            if (cMaterial)
            {
                pMaterial = cMaterial->GetMatInfo();
            }
#else
            pMaterial = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(materialName.c_str(), false);
#endif
        }
        return pMaterial;
    }

    void WaterOceanComponentData_UpdateOceanMaterial(const AZStd::string& materialName)
    {
        if (gEnv->p3DEngine)
        {
            auto pMaterial = WaterOceanComponentData_LoadMaterial(materialName);
            if (pMaterial)
            {
                gEnv->p3DEngine->GetITerrain()->ChangeOceanMaterial(pMaterial);
            }
        }
    }

    void WaterOceanComponentData_UpdateOceanBuoyancy(float height)
    {
        if (gEnv->p3DEngine && gEnv->p3DEngine->GetITerrain())
        {
            // updates the buoyancy area associated with the ocean.
            gEnv->p3DEngine->GetITerrain()->SetOceanWaterLevel(height); 
        }
    }

    void WaterOceanComponentData::Activate()
    {
        ++s_numberOfOceanComponentsActivated;
        AZ_Error("Error", s_numberOfOceanComponentsActivated == 1, "Only one ocean component should be active at a time.");

        m_enabled = true;
        AZ::OceanEnvironmentBus::Handler::BusConnect();
        WaterOceanComponentData_UpdateOceanBuoyancy(m_general.m_height);
        WaterOceanComponentData_UpdateOceanMaterial(m_general.m_oceanMaterialAsset.GetAssetPath());

        if (gEnv->p3DEngine)
        {
            gEnv->p3DEngine->EnableOceanRendering(true); // Sets a bool that allows the ocean to render
            if (gEnv->p3DEngine->GetITerrain())
            {
                auto pMaterial = WaterOceanComponentData_LoadMaterial(m_general.m_oceanMaterialAsset.GetAssetPath());
                gEnv->p3DEngine->GetITerrain()->InitTerrainWater(pMaterial); // Causes the ocean to be created
            }
        }
    }

    void WaterOceanComponentData::Deactivate()
    {
        --s_numberOfOceanComponentsActivated;

        m_enabled = false;
        AZ::OceanEnvironmentBus::Handler::BusDisconnect();
        WaterOceanComponentData_UpdateOceanBuoyancy(m_general.m_height);

        // turn off ocean and delete it
        if (gEnv->p3DEngine)
        {
            if (gEnv->p3DEngine->GetITerrain())
            {
                gEnv->p3DEngine->GetITerrain()->InitTerrainWater(nullptr); // Causes the ocean to be deleted
            }
            gEnv->p3DEngine->EnableOceanRendering(false); // turns off ocean rendering
        }
    }

    bool WaterOceanComponentData::OceanIsEnabled() const
    {
        return m_enabled;
    }

    float WaterOceanComponentData::GetOceanLevel() const
    {
        return m_general.m_height;
    }

    float WaterOceanComponentData::GetOceanLevelOrDefault(const float defaultValue) const
    {
        return OceanIsEnabled() ? m_general.m_height : defaultValue;
    }

    float WaterOceanComponentData::GetWaterLevel(const Vec3& pvPos) const
    {
        return gEnv->p3DEngine->GetWaterLevel(&pvPos);
    }

    float WaterOceanComponentData::GetAccurateOceanHeight(const Vec3& pCurrPos) const
    {
        return gEnv->p3DEngine->GetAccurateOceanHeight(pCurrPos);
    }

    int WaterOceanComponentData::GetWaterTessellationAmount() const
    {
        return m_advanced.m_waterTessellationAmount;
    }

    const AZStd::string& WaterOceanComponentData::GetOceanMaterialName() const
    {
        return m_general.m_oceanMaterialAsset.GetAssetPath();
    }

    void WaterOceanComponentData::SetOceanMaterialName(const AZStd::string& matName)
    {
        m_general.m_oceanMaterialAsset.SetAssetPath(matName.c_str());
        WaterOceanComponentData_UpdateOceanMaterial(matName);
    }

    float WaterOceanComponentData::GetAnimationWindDirection() const
    {
        return m_animation.m_windDirection;
    }

    float WaterOceanComponentData::GetAnimationWindSpeed() const
    {
        return m_animation.m_windSpeed;
    }

    float WaterOceanComponentData::GetAnimationWavesSpeed() const
    {
        return m_animation.m_wavesSpeed;
    }

    float WaterOceanComponentData::GetAnimationWavesSize() const
    {
        return m_animation.m_wavesSize;
    }

    float WaterOceanComponentData::GetAnimationWavesAmount() const
    {
        return m_animation.m_wavesAmount;
    }

    void WaterOceanComponentData::ApplyReflectRenderFlags(int& flags) const
    {
        if (m_reflection.m_entities)
        {
            flags |= SRenderingPassInfo::ENTITIES;
        }
        if (m_reflection.m_particles)
        {
            flags |= SRenderingPassInfo::PARTICLES;
        }
        if (m_reflection.m_terrainDetailMaterials)
        {
            flags |= SRenderingPassInfo::TERRAIN_DETAIL_MATERIALS;
        }
        if (m_reflection.m_staticObjects)
        {
            flags |= SRenderingPassInfo::STATIC_OBJECTS;
        }
    }

    bool WaterOceanComponentData::GetReflectionAnisotropic() const
    {
        return m_reflection.m_anisotropic;
    }

    float WaterOceanComponentData::GetReflectResolutionScale() const
    {
        return m_reflection.m_resolutionScale;
    }

    bool WaterOceanComponentData::GetReflectRenderFlag(ReflectionFlags flag) const
    {
        switch (flag)
        {
        case OceanEnvironmentRequests::ReflectionFlags::Entities:
            return m_reflection.m_entities;
        case OceanEnvironmentRequests::ReflectionFlags::Particles:
            return m_reflection.m_particles;
        case OceanEnvironmentRequests::ReflectionFlags::TerrainDetailMaterials:
            return m_reflection.m_terrainDetailMaterials;
        case OceanEnvironmentRequests::ReflectionFlags::StaticObjects:
            return m_reflection.m_staticObjects;
        default:
            return false;
        }
    }

    bool WaterOceanComponentData::GetUseOceanBottom() const
    {
        return m_general.m_useOceanBottom;
    }

    bool WaterOceanComponentData::GetGodRaysEnabled() const
    {
        return m_advanced.m_godraysEnabled;
    }

    float WaterOceanComponentData::GetUnderwaterDistortion() const
    {
        return m_advanced.m_underwaterDistortion;
    }

    bool  WaterOceanComponentData::GetCausticsEnabled() const
    {
        return m_caustics.m_enabled;
    }

    float WaterOceanComponentData::GetCausticsDepth() const
    {
        return m_caustics.m_depth;
    }

    float WaterOceanComponentData::GetCausticsIntensity() const
    {
        return m_caustics.m_intensity;
    }

    float WaterOceanComponentData::GetCausticsTiling() const
    {
        return m_caustics.m_tiling;
    }

    float WaterOceanComponentData::GetCausticsDistanceAttenuation() const
    {
        return m_caustics.m_distanceAttenuation;
    }

    AZ::Color WaterOceanComponentData::GetFogColor() const
    {
        return m_fog.m_color;
    }

    float WaterOceanComponentData::GetFogColorMulitplier() const
    {
        return m_fog.m_colorMultiplier;
    }

    AZ::Color WaterOceanComponentData::GetNearFogColor() const
    {
        return m_fog.m_nearFogColor;
    }

    AZ::Color WaterOceanComponentData::GetFogColorPremultiplied() const
    {
        return m_fog.m_color * m_fog.m_colorMultiplier;
    }

    float WaterOceanComponentData::GetFogDensity() const
    {
        // A fog density of 0.0f causes shader problems, so return a value slightly above zero if that's the case.
        return AZ::GetMax(m_fog.m_density, 0.0001f);
    }

    void WaterOceanComponentData::SetOceanLevel(float oceanLevel)
    {
        m_general.m_height = oceanLevel;
        WaterOceanComponentData_UpdateOceanBuoyancy(m_general.m_height);
    }

    void WaterOceanComponentData::SetFogColor(const AZ::Color& fogColor)
    {
        m_fog.m_color = fogColor;
    }

    void WaterOceanComponentData::SetFogColorMulitplier(float fogMulitplier)
    {
        m_fog.m_colorMultiplier = fogMulitplier;
    }

    void WaterOceanComponentData::SetNearFogColor(const AZ::Color& nearColor)
    {
        m_fog.m_nearFogColor = nearColor;
    }

    void WaterOceanComponentData::SetFogDensity(float density)
    {
        m_fog.m_density = density;
    }

    void WaterOceanComponentData::SetWaterTessellationAmount(int amount)
    {
        m_advanced.m_waterTessellationAmount = amount;
    }

    void WaterOceanComponentData::SetAnimationWindDirection(float dir)
    {
        m_animation.m_windDirection = dir;
    }

    void WaterOceanComponentData::SetAnimationWindSpeed(float speed)
    {
        m_animation.m_windSpeed = speed;
    }

    void WaterOceanComponentData::SetAnimationWavesSpeed(float speed)
    {
        m_animation.m_wavesSpeed = speed;
    }

    void WaterOceanComponentData::SetAnimationWavesSize(float size)
    {
        m_animation.m_wavesSize = size;
    }

    void WaterOceanComponentData::SetAnimationWavesAmount(float amount)
    {
        m_animation.m_wavesAmount = amount;
    }

    void WaterOceanComponentData::SetReflectRenderFlag(ReflectionFlags flag, bool value)
    {
        switch (flag)
        {
        case OceanEnvironmentRequests::ReflectionFlags::Entities:
            m_reflection.m_entities = value;
            break;
        case OceanEnvironmentRequests::ReflectionFlags::Particles:
            m_reflection.m_particles = value;
            break;
        case OceanEnvironmentRequests::ReflectionFlags::TerrainDetailMaterials:
            m_reflection.m_terrainDetailMaterials = value;
            break;
        case OceanEnvironmentRequests::ReflectionFlags::StaticObjects:
            m_reflection.m_staticObjects = value;
            break;
        default:
            return;
        }
    }

    void WaterOceanComponentData::SetReflectResolutionScale(float scale)
    {
        m_reflection.m_resolutionScale = scale;
    }

    void WaterOceanComponentData::SetReflectionAnisotropic(bool enabled)
    {
        m_reflection.m_anisotropic = enabled;
    }

    void WaterOceanComponentData::SetUseOceanBottom(bool use)
    {
        m_general.m_useOceanBottom = use;
    }

    void WaterOceanComponentData::SetGodRaysEnabled(bool enabled)
    {
        m_advanced.m_godraysEnabled = enabled;
    }

    void WaterOceanComponentData::SetUnderwaterDistortion(float distortion)
    {
        m_advanced.m_underwaterDistortion = distortion;
    }

    void WaterOceanComponentData::SetCausticsEnabled(bool enable)
    {
        m_caustics.m_enabled = enable;
    }

    void WaterOceanComponentData::SetCausticsDepth(float depth)
    {
        m_caustics.m_depth = depth;
    }

    void WaterOceanComponentData::SetCausticsIntensity(float intensity)
    {
        m_caustics.m_intensity = intensity;
    }

    void WaterOceanComponentData::SetCausticsTiling(float tiling)
    {
        m_caustics.m_tiling = tiling;
    }

    void WaterOceanComponentData::SetCausticsDistanceAttenuation(float dist)
    {
        m_caustics.m_distanceAttenuation = dist;
    }

    // General
    //
    void WaterOceanComponentData::General::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<General>()
                ->Version(1, &VersionConverter)
                ->Field("useOceanBottom", &General::m_useOceanBottom)
                ->Field("oceanMaterial", &General::m_oceanMaterialAsset)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<General>("General", "General")
                    ->ClassElement(AZ::Edit::ClassElements::Group, "General")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &General::m_oceanMaterialAsset, "Water Material", "The material assigned for rendering the ocean (note: must be a water shader, like default_ocean.mtl)")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &General::OnOceanMaterialChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &General::m_useOceanBottom, "Enable Ocean Bottom", "Toggles the infinite plane beneath the ocean")
                    ;
            }
        }
    }

    WaterOceanComponentData::General::General()
    {
        AZStd::string currentOceanMaterialName = m_oceanMaterialAsset.GetAssetPath();
        if (currentOceanMaterialName.empty())
        {
            const AZStd::string defaultOceanMaterial = "EngineAssets/Materials/Water/Ocean_default.mtl";
            m_oceanMaterialAsset.SetAssetPath(defaultOceanMaterial.c_str());
        }
        OnOceanMaterialChanged();
    }

    bool WaterOceanComponentData::General::VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        if (classElement.GetVersion() == 0)
        {
            // height was removed in version 1 to be driven instead by the transform component, so just remove it.
            int heightIndex = classElement.FindElement(AZ_CRC("height", 0xf54de50f));
            classElement.RemoveElement(heightIndex);
        }
        return true;
    }

    void WaterOceanComponentData::General::OnOceanMaterialChanged()
    {
        WaterOceanComponentData_UpdateOceanMaterial(m_oceanMaterialAsset.GetAssetPath());
    }
      
    // Animation
    //
    void WaterOceanComponentData::Animation::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<Animation>()
                ->Version(0)
                ->Field("animationWavesAmount", &Animation::m_wavesAmount)
                ->Field("animationWavesSize", &Animation::m_wavesSize)
                ->Field("animationWavesSpeed", &Animation::m_wavesSpeed)
                ->Field("animationWindDirection", &Animation::m_windDirection)
                ->Field("animationWindSpeed", &Animation::m_windSpeed)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<Animation>("Animation", "Animation")
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Animation")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &Animation::m_wavesAmount, "Waves Amount", "Controls the frequency of ocean waves (higher values, the waves are smaller and closer together)")
                        ->Attribute(AZ::Edit::Attributes::Min, AZ::OceanConstants::s_animationWavesAmountMin)
                        ->Attribute(AZ::Edit::Attributes::Max, AZ::OceanConstants::s_animationWavesAmountMax)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &Animation::m_wavesSize, "Waves Size", "Controls the height of ocean waves (higher values, work better with lower Waves Amount)")
                        ->Attribute(AZ::Edit::Attributes::Min, AZ::OceanConstants::s_animationWavesSizeMin)
                        ->Attribute(AZ::Edit::Attributes::Max, AZ::OceanConstants::s_animationWavesSizeMax)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &Animation::m_wavesSpeed, "Waves Speed", "Controls the speed of ocean wave movement (elasticity of surface)")
                        ->Attribute(AZ::Edit::Attributes::Min, AZ::OceanConstants::s_animationWavesSpeedMin)
                        ->Attribute(AZ::Edit::Attributes::Max, AZ::OceanConstants::s_animationWavesSpeedMax)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &Animation::m_windDirection, "Wind Direction", "Controls the direction of ocean wind, set in radians (micro detail direction)")
                        ->Attribute(AZ::Edit::Attributes::Min, AZ::OceanConstants::s_animationWindDirectionMin)
                        ->Attribute(AZ::Edit::Attributes::Max, AZ::OceanConstants::s_animationWindDirectionMax)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &Animation::m_windSpeed, "Wind Speed", "Controls the speed of ocean wind (how fast micro detail scrolls)")
                        ->Attribute(AZ::Edit::Attributes::Min, AZ::OceanConstants::s_animationWindSpeedMin)
                        ->Attribute(AZ::Edit::Attributes::Max, AZ::OceanConstants::s_animationWindSpeedMax)
                ;
            }
        }
    }

    // Reflection
    //
    void WaterOceanComponentData::Reflection::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<Reflection>()
                ->Version(0)
                ->Field("reflectionResolutionScale", &Reflection::m_resolutionScale)
                ->Field("reflectEntities", &Reflection::m_entities)
                ->Field("reflectStaticObjects", &Reflection::m_staticObjects)
                ->Field("reflectTerrainDetailMaterials", &Reflection::m_terrainDetailMaterials)
                ->Field("reflectParticles", &Reflection::m_particles)
                ->Field("reflectionAnisotropic", &Reflection::m_anisotropic)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<Reflection>("Reflection", "Reflection")
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Reflection")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &Reflection::m_resolutionScale, "Resolution Scale", "The scale of the screen resolution to use for rendering ocean reflections")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.05f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Reflection::m_entities, "Reflect Entities", "Controls whether the ocean will render entities in reflection pass")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Reflection::m_staticObjects, "Reflect Objects", "Controls whether the ocean will render static mesh objects in reflection pass")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Reflection::m_terrainDetailMaterials, "Reflect Terrain", "Controls whether the ocean will use the terrain detail materials, when terrain is rendered in reflection pass")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Reflection::m_particles, "Reflect Particles", "Controls whether the ocean will render particles in reflection pass")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Reflection::m_anisotropic, "Anisotropic", "Turns on anisotropic filter for rendered ocean reflections")
                    ;
            }
        }
    }

    // Fog
    //
    void WaterOceanComponentData::Fog::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<Fog>()
                ->Version(0)
                ->Field("fogColor", &Fog::m_color)
                ->Field("fogColorMultiplier", &Fog::m_colorMultiplier)
                ->Field("fogDensity", &Fog::m_density)
                ->Field("nearFogColor", &Fog::m_nearFogColor)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<Fog>("Fog", "Fog")
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Fog")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Fog::m_color, "Color", "The color used to render the ocean's fog (below water)")
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &Fog::m_colorMultiplier, "Color Multiplier", "A multiplier that influences the intensity of the ocean fog color")
                        ->Attribute(AZ::Edit::Attributes::Min, AZ::OceanConstants::s_OceanFogColorMultiplierMin)
                        ->Attribute(AZ::Edit::Attributes::Max, AZ::OceanConstants::s_OceanFogColorMultiplierMax)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &Fog::m_density, "Density", "Controls density of the ocean fog (with higher values, the transperancy falls off more quickly)")
                        ->Attribute(AZ::Edit::Attributes::Min, AZ::OceanConstants::s_OceanFogDensityMin)
                        ->Attribute(AZ::Edit::Attributes::Max, AZ::OceanConstants::s_OceanFogDensityMax)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Fog::m_nearFogColor, "Near Fog Color", "The ocean fog color attenuates to this color near shallow depths (it is recommended to be left at the default color, or near black.)")
                    ;
            }
        }
    }

    // Advanced
    //
    void WaterOceanComponentData::Advanced::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<Advanced>()
                ->Version(0)
                ->Field("waterTessellationAmount", &Advanced::m_waterTessellationAmount)
                ->Field("godraysEnabled", &Advanced::m_godraysEnabled)
                ->Field("underwaterDistortion", &Advanced::m_underwaterDistortion)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<Advanced>("Advanced", "Advanced")
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Advanced")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &Advanced::m_waterTessellationAmount, "Tessellation", "Sets the amount of ocean water surface geometry tessellation.")
                        ->Attribute(AZ::Edit::Attributes::Min, AZ::OceanConstants::s_waterTessellationAmountMin)
                        ->Attribute(AZ::Edit::Attributes::Max, AZ::OceanConstants::s_waterTessellationAmountMax)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Advanced::m_godraysEnabled, "Godrays Enabled", "Enables under ocean god rays")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Advanced::m_underwaterDistortion, "Underwater Distortion", "Controls the amount the rendering of the scene is distortied, when the camera is underwater")
                    ;
            }
        }
    }

    // Caustics
    //
    void WaterOceanComponentData::Caustics::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<Caustics>()
                ->Version(0)
                ->Field("causticsEnabled", &Caustics::m_enabled)
                ->Field("causticDepth", &Caustics::m_depth)
                ->Field("causticIntensity", &Caustics::m_intensity)
                ->Field("causticTiling", &Caustics::m_tiling)
                ->Field("causticDistanceAttenuation", &Caustics::m_distanceAttenuation)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<Caustics>("Caustics", "Caustics")
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Caustic")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Caustics::m_enabled, "Enables Caustics", "Applies the caustics effect of the ocean on geometry below the water surface")
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &Caustics::m_distanceAttenuation, "Distance Attenuation", "Controls the attenuation distance (from camera) the caustic effects of the ocean is applied")
                        ->Attribute(AZ::Edit::Attributes::Min, AZ::OceanConstants::s_CausticsDistanceAttenMin)
                        ->Attribute(AZ::Edit::Attributes::Max, AZ::OceanConstants::s_CausticsDistanceAttenMax)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &Caustics::m_depth, "Depth", "The depth below the ocean (calculated in meters) that influences when the caustic effect of the ocean is applied")
                        ->Attribute(AZ::Edit::Attributes::Min, AZ::OceanConstants::s_CausticsDepthMin)
                        ->Attribute(AZ::Edit::Attributes::Max, AZ::OceanConstants::s_CausticsDepthMax)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &Caustics::m_intensity, "Intensity", "Controls the intensity of the light in the ocean caustics effect. 0 will turn caustics off completely. Near-zero values still show caustics rather strongly (going from 0.00 to 0.01 is a sudden change)")
                        ->Attribute(AZ::Edit::Attributes::Min, AZ::OceanConstants::s_CausticsIntensityMin)
                        ->Attribute(AZ::Edit::Attributes::Max, AZ::OceanConstants::s_CausticsIntensityMax)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &Caustics::m_tiling, "Tiling", "Controls the amount the ocean caustic effect is tiled (lower valules spread it out, higher values tighten the detail)")
                        ->Attribute(AZ::Edit::Attributes::Min, AZ::OceanConstants::s_CausticsTilingMin)
                        ->Attribute(AZ::Edit::Attributes::Max, AZ::OceanConstants::s_CausticsTilingMax)
                    ;

            }
        }
    }

}