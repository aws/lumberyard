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

#include "WaterVolumeComponent.h"

#include <AzCore/RTTI/BehaviorContext.h>

#include "MathConversion.h"

#include "I3DEngine.h"
#include "physinterface.h"

namespace Water
{
    void WaterVolumeCommon::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<WaterVolumeCommon>()
                ->Version(1)
                ->Field("MaterialAsset", &WaterVolumeCommon::m_materialAsset)
                ->Field("MinSpec", &WaterVolumeCommon::m_minSpec)
                ->Field("SurfaceUScale", &WaterVolumeCommon::m_surfaceUScale)
                ->Field("SurfaceVScale", &WaterVolumeCommon::m_surfaceVScale)
                ->Field("ViewDistanceMultiplier", &WaterVolumeCommon::m_viewDistanceMultiplier)

                ->Field("FogDensity", &WaterVolumeCommon::m_fogDensity)
                ->Field("FogColor", &WaterVolumeCommon::m_fogColor)
                ->Field("FogColorMultiplier", &WaterVolumeCommon::m_fogColorMultiplier)
                ->Field("FogColorAffectedBySun", &WaterVolumeCommon::m_fogColorAffectedBySun)
                ->Field("FogShadowing", &WaterVolumeCommon::m_fogShadowing)
                ->Field("CapFogAtVolumeDepth", &WaterVolumeCommon::m_capFogAtVolumeDepth)
                
                ->Field("CausticsEnabled", &WaterVolumeCommon::m_causticsEnabled)
                ->Field("CausticIntensity", &WaterVolumeCommon::m_causticIntensity)
                ->Field("CausticTiling", &WaterVolumeCommon::m_causticTiling)
                ->Field("CausticHeight", &WaterVolumeCommon::m_causticHeight)

                ->Field("SpillableVolume", &WaterVolumeCommon::m_spillableVolume)
                ->Field("VolumeAccuracy", &WaterVolumeCommon::m_volumeAccuracy)
                ->Field("ExtrudeBorder", &WaterVolumeCommon::m_extrudeBorder)
                ->Field("ConvexBorder", &WaterVolumeCommon::m_convexBorder)
                ->Field("ObjectSizeLimit", &WaterVolumeCommon::m_objectSizeLimit)

                ->Field("WaveSurfaceCellSize", &WaterVolumeCommon::m_waveSurfaceCellSize)
                ->Field("WaveSpeed", &WaterVolumeCommon::m_waveSpeed)
                ->Field("WaveDampening", &WaterVolumeCommon::m_waveDampening)
                ->Field("WaveTimestep", &WaterVolumeCommon::m_waveTimestep)
                ->Field("WaveSleepThreshold", &WaterVolumeCommon::m_waveSleepThreshold)
                ->Field("WaveDepthCellSize", &WaterVolumeCommon::m_waveDepthCellSize)
                ->Field("WaveHeightLimit", &WaterVolumeCommon::m_waveHeightLimit)
                ->Field("WaveForce", &WaterVolumeCommon::m_waveForce)
                ->Field("WaveSimulationAreaGrowth", &WaterVolumeCommon::m_waveSimulationAreaGrowth)
            ;
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<WaterVolumeComponentRequestBus>("WaterVolumeComponentRequestBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Event("SetSurfaceUScale", &WaterVolumeComponentRequestBus::Events::SetSurfaceUScale)
                ->Event("GetSurfaceUScale", &WaterVolumeComponentRequestBus::Events::GetSurfaceUScale)
                ->VirtualProperty("SurfaceUScale", "GetSurfaceUScale", "SetSurfaceUScale")
                ->Event("SetSurfaceVScale", &WaterVolumeComponentRequestBus::Events::SetSurfaceVScale)
                ->Event("GetSurfaceVScale", &WaterVolumeComponentRequestBus::Events::GetSurfaceVScale)
                ->VirtualProperty("SurfaceVScale", "GetSurfaceVScale", "SetSurfaceVScale")

                ->Event("SetFogDensity", &WaterVolumeComponentRequestBus::Events::SetFogDensity)
                ->Event("GetFogDensity", &WaterVolumeComponentRequestBus::Events::GetFogDensity)
                ->VirtualProperty("FogDensity", "GetFogDensity", "SetFogDensity")
                ->Event("SetFogColor", &WaterVolumeComponentRequestBus::Events::SetFogColor)
                ->Event("GetFogColor", &WaterVolumeComponentRequestBus::Events::GetFogColor)
                ->VirtualProperty("FogColor", "GetFogColor", "SetFogColor")
                ->Event("SetFogColorMultiplier", &WaterVolumeComponentRequestBus::Events::SetFogColorMultiplier)
                ->Event("GetFogColorMultiplier", &WaterVolumeComponentRequestBus::Events::GetFogColorMultiplier)
                ->VirtualProperty("FogColorMultiplier", "GetFogColorMultiplier", "SetFogColorMultiplier")
                ->Event("SetFogColorAffectedBySun", &WaterVolumeComponentRequestBus::Events::SetFogColorAffectedBySun)
                ->Event("GetFogColorAffectedBySun", &WaterVolumeComponentRequestBus::Events::GetFogColorAffectedBySun)
                ->VirtualProperty("FogColorAffectedBySun", "GetFogColorAffectedBySun", "SetFogColorAffectedBySun")
                ->Event("SetFogShadowing", &WaterVolumeComponentRequestBus::Events::SetFogShadowing)
                ->Event("GetFogShadowing", &WaterVolumeComponentRequestBus::Events::GetFogShadowing)
                ->VirtualProperty("FogShadowing", "GetFogShadowing", "SetFogShadowing")
                ->Event("SetCapFogAtVolumeDepth", &WaterVolumeComponentRequestBus::Events::SetCapFogAtVolumeDepth)
                ->Event("GetCapFogAtVolumeDepth", &WaterVolumeComponentRequestBus::Events::GetCapFogAtVolumeDepth)
                ->VirtualProperty("CapFogAtVolumeDepth", "GetCapFogAtVolumeDepth", "SetCapFogAtVolumeDepth")

                ->Event("SetCausticsEnabled", &WaterVolumeComponentRequestBus::Events::SetCausticsEnabled)
                ->Event("GetCausticsEnabled", &WaterVolumeComponentRequestBus::Events::GetCausticsEnabled)
                ->VirtualProperty("CausticsEnabled", "GetCausticsEnabled", "SetCausticsEnabled")
                ->Event("SetCausticIntensity", &WaterVolumeComponentRequestBus::Events::SetCausticIntensity)
                ->Event("GetCausticIntensity", &WaterVolumeComponentRequestBus::Events::GetCausticIntensity)
                ->VirtualProperty("CausticIntensity", "GetCausticIntensity", "SetCausticIntensity")
                ->Event("SetCausticTiling", &WaterVolumeComponentRequestBus::Events::SetCausticTiling)
                ->Event("GetCausticTiling", &WaterVolumeComponentRequestBus::Events::GetCausticTiling)
                ->VirtualProperty("CausticTiling", "GetCausticTiling", "SetCausticTiling")
                ->Event("SetCausticHeight", &WaterVolumeComponentRequestBus::Events::SetCausticHeight)
                ->Event("GetCausticHeight", &WaterVolumeComponentRequestBus::Events::GetCausticHeight)
                ->VirtualProperty("CausticHeight", "GetCausticHeight", "SetCausticHeight")

                ->Event("SetSpillableVolume", &WaterVolumeComponentRequestBus::Events::SetSpillableVolume)
                ->Event("GetSpillableVolume", &WaterVolumeComponentRequestBus::Events::GetSpillableVolume)
                ->VirtualProperty("SpillableVolume", "GetSpillableVolume", "SetSpillableVolume")
                ->Event("SetVolumeAccuracy", &WaterVolumeComponentRequestBus::Events::SetVolumeAccuracy)
                ->Event("GetVolumeAccuracy", &WaterVolumeComponentRequestBus::Events::GetVolumeAccuracy)
                ->VirtualProperty("VolumeAccuracy", "GetVolumeAccuracy", "SetVolumeAccuracy")
                ->Event("SetExtrudeBorder", &WaterVolumeComponentRequestBus::Events::SetExtrudeBorder)
                ->Event("GetExtrudeBorder", &WaterVolumeComponentRequestBus::Events::GetExtrudeBorder)
                ->VirtualProperty("ExtrudeBorder", "GetExtrudeBorder", "SetExtrudeBorder")
                ->Event("SetConvexBorder", &WaterVolumeComponentRequestBus::Events::SetConvexBorder)
                ->Event("GetConvexBorder", &WaterVolumeComponentRequestBus::Events::GetConvexBorder)
                ->VirtualProperty("ConvexBorder", "GetConvexBorder", "SetConvexBorder")
                ->Event("SetObjectSizeLimit", &WaterVolumeComponentRequestBus::Events::SetObjectSizeLimit)
                ->Event("GetObjectSizeLimit", &WaterVolumeComponentRequestBus::Events::GetObjectSizeLimit)
                ->VirtualProperty("ObjectSizeLimit", "GetObjectSizeLimit", "SetObjectSizeLimit")

                ->Event("SetWaveSurfaceCellSize", &WaterVolumeComponentRequestBus::Events::SetWaveSurfaceCellSize)
                ->Event("GetWaveSurfaceCellSize", &WaterVolumeComponentRequestBus::Events::GetWaveSurfaceCellSize)
                ->VirtualProperty("WaveSurfaceCellSize", "GetWaveSurfaceCellSize", "SetWaveSurfaceCellSize")
                ->Event("SetWaveSpeed", &WaterVolumeComponentRequestBus::Events::SetWaveSpeed)
                ->Event("GetWaveSpeed", &WaterVolumeComponentRequestBus::Events::GetWaveSpeed)
                ->VirtualProperty("WaveSpeed", "GetWaveSpeed", "SetWaveSpeed")
                ->Event("SetWaveDampening", &WaterVolumeComponentRequestBus::Events::SetWaveDampening)
                ->Event("GetWaveDampening", &WaterVolumeComponentRequestBus::Events::GetWaveDampening)
                ->VirtualProperty("WaveDampening", "GetWaveDampening", "SetWaveDampening")
                ->Event("SetWaveTimestep", &WaterVolumeComponentRequestBus::Events::SetWaveTimestep)
                ->Event("GetWaveTimestep", &WaterVolumeComponentRequestBus::Events::GetWaveTimestep)
                ->VirtualProperty("WaveTimestep", "GetWaveTimestep", "SetWaveTimestep")
                ->Event("SetWaveSleepThreshold", &WaterVolumeComponentRequestBus::Events::SetWaveSleepThreshold)
                ->Event("GetWaveSleepThreshold", &WaterVolumeComponentRequestBus::Events::GetWaveSleepThreshold)
                ->VirtualProperty("WaveSleepThreshold", "GetWaveSleepThreshold", "SetWaveSleepThreshold")
                ->Event("SetWaveDepthCellSize", &WaterVolumeComponentRequestBus::Events::SetWaveDepthCellSize)
                ->Event("GetWaveDepthCellSize", &WaterVolumeComponentRequestBus::Events::GetWaveDepthCellSize)
                ->VirtualProperty("WaveDepthCellSize", "GetWaveDepthCellSize", "SetWaveDepthCellSize")
                ->Event("SetWaveHeightLimit", &WaterVolumeComponentRequestBus::Events::SetWaveHeightLimit)
                ->Event("GetWaveHeightLimit", &WaterVolumeComponentRequestBus::Events::GetWaveHeightLimit)
                ->VirtualProperty("WaveHeightLimit", "GetWaveHeightLimit", "SetWaveHeightLimit")
                ->Event("SetWaveForce", &WaterVolumeComponentRequestBus::Events::SetWaveForce)
                ->Event("GetWaveForce", &WaterVolumeComponentRequestBus::Events::GetWaveForce)
                ->VirtualProperty("WaveForce", "GetWaveForce", "SetWaveForce")
                ->Event("SetWaveSimulationAreaGrowth", &WaterVolumeComponentRequestBus::Events::SetWaveSimulationAreaGrowth)
                ->Event("GetWaveSimulationAreaGrowth", &WaterVolumeComponentRequestBus::Events::GetWaveSimulationAreaGrowth)
                ->VirtualProperty("WaveSimulationAreaGrowth", "GetWaveSimulationAreaGrowth", "SetWaveSimulationAreaGrowth")
                ;

            behaviorContext->Class<WaterVolumeComponent>()->RequestBus("WaterVolumeComponentRequestBus");
        }
    }

    void WaterVolumeCommon::Init(const AZ::EntityId& entityId)
    {
        m_entityId = entityId;
    }

    void WaterVolumeCommon::Activate()
    {
        m_volumeId = static_cast<AZ::u64>(m_entityId);
        m_waterRenderNode = static_cast<IWaterVolumeRenderNode*>(gEnv->p3DEngine->CreateRenderNode(eERType_WaterVolume));
        m_waterRenderNode->m_hasToBeSerialised = false;

        //Load material
        _smart_ptr<IMaterial> material = nullptr;
        const AZStd::string& materialPath = m_materialAsset.GetAssetPath();
        if (!materialPath.empty())
        {
            material = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(materialPath.c_str());
        }

        //Get the initial world transform
        AZ::TransformBus::EventResult(m_currentWorldTransform, m_entityId, &AZ::TransformBus::Events::GetWorldTM);

        //Get the initial shape points
        OnShapeChanged(ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);

        SetMaterial(material);

        Update();

        WaterVolumeComponentRequestBus::Handler::BusConnect(m_entityId);
        AZ::TransformNotificationBus::Handler::BusConnect(m_entityId);
        LmbrCentral::MaterialOwnerRequestBus::Handler::BusConnect(m_entityId);
        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusConnect(m_entityId);
    }

    void WaterVolumeCommon::Deactivate()
    {
        m_volumeId = 0;

        if (m_waterRenderNode)
        {
            gEnv->p3DEngine->DeleteRenderNode(m_waterRenderNode);
            m_waterRenderNode = nullptr;
        }
        
        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusDisconnect(m_entityId);
        LmbrCentral::MaterialOwnerRequestBus::Handler::BusDisconnect(m_entityId);
        AZ::TransformNotificationBus::Handler::BusDisconnect(m_entityId);
        WaterVolumeComponentRequestBus::Handler::BusDisconnect(m_entityId);
    }

    void WaterVolumeCommon::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_currentWorldTransform = world;

        UpdateVertices();
        UpdatePhysicsAreaParams();
        UpdateWaterArea();
    }

    void WaterVolumeCommon::OnShapeChanged(LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons reasons)
    {
        if (reasons != ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged)
        {
            return;
        }

        m_azVerts.clear();

        AZ::Crc32 shapeType;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(shapeType, m_entityId, &LmbrCentral::ShapeComponentRequestsBus::Events::GetShapeType);

        if (shapeType == AZ_CRC("Box", 0x08a9483a))
        {
            HandleBoxVerts();
        }
        else if (shapeType == AZ_CRC("Cylinder", 0x9b045bea))
        {
            HandleCylinderVerts();
        }
        else if (shapeType == AZ_CRC("PolygonPrism", 0xd6b50036))
        {
            HandlePolygonPrismVerts();
        }

        UpdateVertices();
        UpdateWaterArea();
        UpdatePhysicsAreaParams();

        m_waterRenderNode->SetVolumeDepth(m_waterDepthScaled);
    }

    void WaterVolumeCommon::SetMaterial(_smart_ptr<IMaterial> material)
    {
        if (material)
        {
            m_materialAsset.SetAssetPath(material->GetName());
        }

        if (m_waterRenderNode && material)
        {
            const SShaderItem& shaderItem = material->GetShaderItem();
            if (shaderItem.m_pShader && shaderItem.m_pShader->GetShaderType() != eST_Water)
            {
                AZ_Warning("WaterVolumeCommon", false, "Incorrect shader set for water volume component!");
            }

            m_waterRenderNode->SetMaterial(material);
        }
    }

    _smart_ptr<IMaterial> WaterVolumeCommon::GetMaterial()
    {
        if (m_waterRenderNode)
        {
            return m_waterRenderNode->GetMaterial();
        }

        return nullptr;
    }

    void WaterVolumeCommon::SetSurfaceUScale(float surfaceUScale)
    {
        m_surfaceUScale = surfaceUScale;
        UpdateWaterArea();
    }

    void WaterVolumeCommon::SetSurfaceVScale(float surfaceVScale)
    {
        m_surfaceVScale = surfaceVScale;
        UpdateWaterArea();
    }

    void WaterVolumeCommon::SetFogDensity(float fogDensity)
    {
        m_fogDensity = fogDensity;
        OnFogDensityChange();
    }

    void WaterVolumeCommon::SetFogColor(AZ::Vector3 fogColor)
    {
        m_fogColor = fogColor;
        OnFogColorChange();
    }

    void WaterVolumeCommon::SetFogColorMultiplier(float fogColorMultiplier)
    {
        m_fogColorMultiplier = fogColorMultiplier;
        OnFogColorChange();
    }

    void WaterVolumeCommon::SetFogColorAffectedBySun(bool fogColorAffectedBySun)
    {
        m_fogColorAffectedBySun = fogColorAffectedBySun;
        OnFogColorAffectedBySunChange();
    }

    void WaterVolumeCommon::SetFogShadowing(float fogShadowing)
    {
        m_fogShadowing = fogShadowing;
        OnFogShadowingChange();
    }

    void WaterVolumeCommon::SetCapFogAtVolumeDepth(bool capFogAtVolumeDepth)
    {
        m_capFogAtVolumeDepth = capFogAtVolumeDepth;
        OnCapFogAtVolumeDepthChange();
    }

    void WaterVolumeCommon::SetCausticsEnabled(bool causticsEnabled)
    {
        m_causticsEnabled = causticsEnabled;
        OnCausticsEnableChange();
    }

    void WaterVolumeCommon::SetCausticIntensity(float causticIntensity)
    {
        m_causticIntensity = causticIntensity;
        OnCausticIntensityChange();
    }

    void WaterVolumeCommon::SetCausticTiling(float causticTiling)
    {
        m_causticTiling = causticTiling;
        OnCausticTilingChange();
    }

    void WaterVolumeCommon::SetCausticHeight(float causticHeight)
    {
        m_causticHeight = causticHeight;
        OnCausticHeightChange();
    }

    void WaterVolumeCommon::SetSpillableVolume(float spillableVolume)
    {
        m_spillableVolume = spillableVolume;
        OnPhysicsParamChange();
    }

    void WaterVolumeCommon::SetVolumeAccuracy(float volumeAccuracy)
    {
        m_volumeAccuracy = volumeAccuracy;
        OnPhysicsParamChange();
    }

    void WaterVolumeCommon::SetExtrudeBorder(float extrudeBorder)
    {
        m_extrudeBorder = extrudeBorder;
        OnPhysicsParamChange();
    }

    void WaterVolumeCommon::SetConvexBorder(bool convexBorder)
    {
        m_convexBorder = convexBorder;
        OnPhysicsParamChange();
    }

    void WaterVolumeCommon::SetObjectSizeLimit(float objectSizeLimit)
    {
        m_objectSizeLimit = objectSizeLimit;
        OnPhysicsParamChange();
    }

    void WaterVolumeCommon::SetWaveSurfaceCellSize(float waveSurfaceCellSize)
    {
        m_waveSurfaceCellSize = waveSurfaceCellSize;
        OnPhysicsParamChange();
    }

    void WaterVolumeCommon::SetWaveSpeed(float waveSpeed)
    {
        m_waveSpeed = waveSpeed;
        OnPhysicsParamChange();
    }

    void WaterVolumeCommon::SetWaveDampening(float waveDampening)
    {
        m_waveDampening = waveDampening;
        OnPhysicsParamChange();
    }

    void WaterVolumeCommon::SetWaveTimestep(float waveTimestep)
    {
        m_waveTimestep = waveTimestep;
        OnPhysicsParamChange();
    }

    void WaterVolumeCommon::SetWaveSleepThreshold(float waveSleepThreshold)
    {
        m_waveSleepThreshold = waveSleepThreshold;
        OnPhysicsParamChange();
    }

    void WaterVolumeCommon::SetWaveDepthCellSize(float waveDepthCellSize)
    {
        m_waveDepthCellSize = waveDepthCellSize;
        OnPhysicsParamChange();
    }

    void WaterVolumeCommon::SetWaveHeightLimit(float waveHeightLimit)
    {
        m_waveHeightLimit = waveHeightLimit;
        OnPhysicsParamChange();
    }

    void WaterVolumeCommon::SetWaveForce(float waveForce)
    {
        m_waveForce = waveForce;
        OnPhysicsParamChange();
    }

    void WaterVolumeCommon::SetWaveSimulationAreaGrowth(float waveSimulationAreaGrowth)
    {
        m_waveSimulationAreaGrowth = waveSimulationAreaGrowth;
        OnPhysicsParamChange();
    }

    void WaterVolumeCommon::OnMaterialAssetChange()
    {
        _smart_ptr<IMaterial> material = nullptr;
        const AZStd::string& materialPath = m_materialAsset.GetAssetPath();
        if (!materialPath.empty())
        {
            material = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(materialPath.c_str());
        }

        SetMaterial(material);
    }

    void WaterVolumeCommon::OnMinSpecChange()
    {
        if (m_waterRenderNode)
        {
            m_waterRenderNode->SetMinSpec(static_cast<AZ::u32>(m_minSpec));
            
            if (gEnv && gEnv->p3DEngine)
            {
                const int configSpec = gEnv->pSystem->GetConfigSpec(true);

                AZ::u32 rendFlags = static_cast<AZ::u32>(m_waterRenderNode->GetRndFlags());

                const bool hidden = static_cast<AZ::u32>(configSpec) < static_cast<AZ::u32>(m_minSpec);
                if (hidden)
                {
                    rendFlags |= ERF_HIDDEN;
                }
                else
                {
                    rendFlags &= ~ERF_HIDDEN;
                }

                m_waterRenderNode->SetRndFlags(rendFlags);
            }
        }
    }

    void WaterVolumeCommon::OnWaterAreaParamChange()
    {
        UpdateWaterArea();
    }
    void WaterVolumeCommon::OnViewDistanceMultiplierChange()
    {
        if (m_waterRenderNode)
        {
            m_waterRenderNode->SetViewDistanceMultiplier(m_viewDistanceMultiplier);
        }
    }

    void WaterVolumeCommon::OnFogDensityChange()
    {
        if (m_waterRenderNode)
        {
            m_waterRenderNode->SetFogDensity(m_fogDensity);
        }
    }
    void WaterVolumeCommon::OnFogColorChange()
    {
        if (m_waterRenderNode)
        {
            Vec3 fogColor = AZVec3ToLYVec3(m_fogColor) * m_fogColorMultiplier;
            m_waterRenderNode->SetFogColor(fogColor);
        }
    }
    void WaterVolumeCommon::OnFogColorAffectedBySunChange()
    {
        if (m_waterRenderNode)
        {
            m_waterRenderNode->SetFogColorAffectedBySun(m_fogColorAffectedBySun);
        }
    }
    void WaterVolumeCommon::OnFogShadowingChange()
    {
        if (m_waterRenderNode)
        {
            m_waterRenderNode->SetFogShadowing(m_fogShadowing);
        }
    }
    void WaterVolumeCommon::OnCapFogAtVolumeDepthChange()
    {
        if (m_waterRenderNode)
        {
            m_waterRenderNode->SetCapFogAtVolumeDepth(m_capFogAtVolumeDepth);
        }
    }

    void WaterVolumeCommon::OnCausticsEnableChange()
    {
        if (m_waterRenderNode)
        {
            m_waterRenderNode->SetCaustics(m_causticsEnabled);
        }
    }
    void WaterVolumeCommon::OnCausticIntensityChange()
    {
        if (m_waterRenderNode)
        {
            m_waterRenderNode->SetCausticIntensity(m_causticIntensity);
        }
    }
    void WaterVolumeCommon::OnCausticTilingChange()
    {
        if (m_waterRenderNode)
        {
            m_waterRenderNode->SetCausticTiling(m_causticTiling);
        }
    }
    void WaterVolumeCommon::OnCausticHeightChange()
    {
        if (m_waterRenderNode)
        {
            m_waterRenderNode->SetCausticHeight(m_causticHeight);
        }
    }

    void WaterVolumeCommon::OnPhysicsParamChange()
    {
        UpdateAuxPhysicsParams();
    }

    void WaterVolumeCommon::Update()
    {
        UpdateAllWaterParams();

        UpdateAuxPhysicsParams();

        UpdateVertices();

        UpdateWaterArea();

        UpdatePhysicsAreaParams();
    }

    void WaterVolumeCommon::UpdateAllWaterParams()
    {
        Vec3 fogColor = AZVec3ToLYVec3(m_fogColor) * m_fogColorMultiplier;

        m_waterRenderNode->SetMinSpec(static_cast<AZ::u32>(m_minSpec));
        m_waterRenderNode->SetVolumeDepth(m_waterDepthScaled);
        m_waterRenderNode->SetViewDistanceMultiplier(m_viewDistanceMultiplier);
        m_waterRenderNode->SetFogDensity(m_fogDensity);
        m_waterRenderNode->SetFogColor(fogColor);
        m_waterRenderNode->SetFogColorAffectedBySun(m_fogColorAffectedBySun);
        m_waterRenderNode->SetFogShadowing(m_fogShadowing);
        m_waterRenderNode->SetCapFogAtVolumeDepth(m_capFogAtVolumeDepth);
        m_waterRenderNode->SetCaustics(m_causticsEnabled);
        m_waterRenderNode->SetCausticIntensity(m_causticIntensity);
        m_waterRenderNode->SetCausticTiling(m_causticTiling);
        m_waterRenderNode->SetCausticHeight(m_causticHeight);
    }

    void WaterVolumeCommon::UpdateAuxPhysicsParams()
    {
        pe_params_area physicalAreaParams;
        physicalAreaParams.volume = m_spillableVolume;
        physicalAreaParams.volumeAccuracy = m_volumeAccuracy;
        physicalAreaParams.borderPad = m_extrudeBorder;
        physicalAreaParams.bConvexBorder = m_convexBorder;
        physicalAreaParams.objectVolumeThreshold = m_objectSizeLimit;
        physicalAreaParams.cellSize = m_waveSurfaceCellSize;
        physicalAreaParams.waveSim.waveSpeed = m_waveSpeed;
        physicalAreaParams.waveSim.dampingCenter = m_waveDampening;
        physicalAreaParams.waveSim.timeStep = m_waveTimestep;
        physicalAreaParams.waveSim.heightLimit = m_waveHeightLimit;
        physicalAreaParams.waveSim.minVel = m_waveSleepThreshold;
        physicalAreaParams.waveSim.resistance = m_waveForce;
        physicalAreaParams.growthReserve = m_waveSimulationAreaGrowth;
        m_waterRenderNode->SetAuxPhysParams(&physicalAreaParams);
    }

    void WaterVolumeCommon::UpdatePhysicsAreaParams()
    {
        //This is possible if you are currently in the process of adding verts to a PolygonPrism
        if (m_legacyVerts.size() < 3)
        {
            return;
        }

        m_waterRenderNode->Dephysicalize();
        m_waterRenderNode->SetAreaPhysicsArea(&m_legacyVerts[0], m_legacyVerts.size(), true);
        m_waterRenderNode->Physicalize();
    }

    void WaterVolumeCommon::UpdateVertices()
    {
        //Retrieve all vertices in the vertex container in world space
        m_legacyVerts.clear();

        for (AZ::Vector3 vertex : m_azVerts)
        {
            vertex = m_currentWorldTransform * vertex;
            m_legacyVerts.push_back(AZVec3ToLYVec3(vertex));
        }

        m_waterDepthScaled = m_waterDepth * m_currentWorldTransform.RetrieveScale().GetZ();
    }

    void WaterVolumeCommon::UpdateWaterArea()
    {
        //This is possible if you are currently in the process of adding verts to a PolygonPrism
        if (m_legacyVerts.size() < 3)
        {
            return;
        }

        Vec3 planeNormal = AZVec3ToLYVec3(m_currentWorldTransform.GetColumn(2));
        Plane fogPlane;
        fogPlane.SetPlane(planeNormal.GetNormalized(), m_legacyVerts[0]);

        Vec2 surfaceScale(m_surfaceUScale, m_surfaceVScale);
        m_waterRenderNode->CreateArea(m_volumeId, &m_legacyVerts[0], m_legacyVerts.size(), surfaceScale, fogPlane, true);
    }

    void WaterVolumeCommon::HandleBoxVerts()
    {
        AZ::Vector3 dimensions = AZ::Vector3::CreateZero();
        LmbrCentral::BoxShapeComponentRequestsBus::EventResult(dimensions, m_entityId, &LmbrCentral::BoxShapeComponentRequestsBus::Events::GetBoxDimensions);

        AZ::Vector3 halfDimensions = dimensions * 0.5f;

        m_azVerts.resize(4);

        m_azVerts[0] = AZ::Vector3(halfDimensions);
        m_azVerts[1] = AZ::Vector3(+halfDimensions.GetX(), -halfDimensions.GetY(), halfDimensions.GetZ());
        m_azVerts[2] = AZ::Vector3(-halfDimensions.GetX(), -halfDimensions.GetY(), halfDimensions.GetZ());
        m_azVerts[3] = AZ::Vector3(-halfDimensions.GetX(), +halfDimensions.GetY(), halfDimensions.GetZ());

        m_waterDepth = dimensions.GetZ();
    }

    void WaterVolumeCommon::HandleCylinderVerts()
    {
        LmbrCentral::CylinderShapeConfig cylinderConfig;
        LmbrCentral::CylinderShapeComponentRequestsBus::EventResult(cylinderConfig, m_entityId, &LmbrCentral::CylinderShapeComponentRequestsBus::Events::GetCylinderConfiguration);

        m_waterDepth = cylinderConfig.m_height;
        const float halfHeight = m_waterDepth * 0.5f;

        const float radius = cylinderConfig.m_radius;
        const AZ::u32 radiusInt = static_cast<AZ::u32>(ceilf(cylinderConfig.m_radius));
        const AZ::u32 segments = (radiusInt * 4) + 16;

        m_azVerts.resize(segments);
        for (AZ::u32 i = 0; i < segments; ++i)
        {
            const float radians = i * (AZ::Constants::TwoPi / static_cast<float>(segments));

            const float x = radius * cosf(radians);
            const float y = radius * sinf(radians);
            const float z = halfHeight;

            m_azVerts[i] = AZ::Vector3(x, y, z);
        }
    }
    void WaterVolumeCommon::HandlePolygonPrismVerts()
    {
        AZ::ConstPolygonPrismPtr polygonPrism = nullptr;
        LmbrCentral::PolygonPrismShapeComponentRequestBus::EventResult(polygonPrism, m_entityId, &LmbrCentral::PolygonPrismShapeComponentRequestBus::Events::GetPolygonPrism);

        if (polygonPrism == nullptr)
        {
            return;
        }

        m_waterDepth = polygonPrism->GetHeight();

        const AZStd::vector<AZ::Vector2>& prismVerts = polygonPrism->m_vertexContainer.GetVertices();

        m_azVerts.resize(prismVerts.size());

        for (size_t i = 0; i < prismVerts.size(); ++i)
        {
            const AZ::Vector2& vert = prismVerts[i];
            m_azVerts[i] = AZ::Vector3(vert.GetX(), vert.GetY(), m_waterDepth);
        }
    }

    void WaterVolumeComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provides)
    {
        provides.push_back(AZ_CRC("WaterVolumeService", 0x895e29b1));
    }

    void WaterVolumeComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& requires)
    {
        requires.push_back(AZ_CRC("TransformService", 0x8ee22c50));
    }

    void WaterVolumeComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<WaterVolumeComponent, AZ::Component>()
                ->Version(1)
                ->Field("Common", &WaterVolumeComponent::m_common)
                ;
        }

        WaterVolumeCommon::Reflect(context);
    }

    void WaterVolumeComponent::Init()
    {
        m_common.Init(GetEntityId());
    }
    void WaterVolumeComponent::Activate()
    {
        m_common.Activate();
    }
    void WaterVolumeComponent::Deactivate()
    {
        m_common.Deactivate();
    }

} // namespace Water