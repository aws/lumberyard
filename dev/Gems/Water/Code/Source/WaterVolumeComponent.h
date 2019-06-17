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

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>

#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/VertexContainer.h>

#include <AzFramework/Asset/SimpleAsset.h>

#include <LmbrCentral/Rendering/MaterialAsset.h>
#include <LmbrCentral/Rendering/MaterialOwnerBus.h>

#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <LmbrCentral/Shape/CylinderShapeComponentBus.h>
#include <LmbrCentral/Shape/PolygonPrismShapeComponentBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>

#include <Water/WaterVolumeComponentBus.h>
#include "IEntityRenderState.h" //For EngineSpec

namespace Water
{
    class WaterVolumeCommon
        : public AZ::TransformNotificationBus::Handler
        , public LmbrCentral::ShapeComponentNotificationsBus::Handler
        , public WaterVolumeComponentRequestBus::Handler
        , public LmbrCentral::MaterialOwnerRequestBus::Handler
    {
    public:
        friend class EditorWaterVolumeCommon;
        friend class WaterVolumeConverter; //So that it can access all the members

        AZ_TYPE_INFO(WaterVolumeCommon, "{053FC2CB-9E3E-40AD-A81B-C28212D9F7A3}");
        AZ_CLASS_ALLOCATOR(WaterVolumeCommon, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* context);

        ~WaterVolumeCommon();

        //Helper startup
        virtual void Init(const AZ::EntityId& entityId);
        virtual void Activate();
        virtual void Deactivate();

        // TransformNotificationBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world);

        // ShapeComponentNotificationBus 
        void OnShapeChanged(LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons reasons) override;

        // MaterialOwnerRequestBus
        void SetMaterial(_smart_ptr<IMaterial> material) override;
        _smart_ptr<IMaterial> GetMaterial() override;

        // WaterVolumeComponentRequestBus
        void SetSurfaceUScale(float UScale) override;
        float GetSurfaceUScale() override { return m_surfaceUScale; }
        void SetSurfaceVScale(float VScale) override;
        float GetSurfaceVScale() override { return m_surfaceVScale; }

        void SetFogDensity(float fogDensity) override;
        float GetFogDensity() override { return m_fogDensity; }
        void SetFogColor(AZ::Vector3 fogColor) override;
        AZ::Vector3 GetFogColor() override { return m_fogColor; }
        void SetFogColorMultiplier(float fogColorMultiplier) override;
        float GetFogColorMultiplier() override { return m_fogColorMultiplier; }
        void SetFogColorAffectedBySun(bool fogColorAffectedBySun) override;
        bool GetFogColorAffectedBySun() override { return m_fogColorAffectedBySun; }
        void SetFogShadowing(float fogShadowing) override;
        float GetFogShadowing() override { return m_fogShadowing; }
        void SetCapFogAtVolumeDepth(bool capFogAtVolumeDepth) override;
        bool GetCapFogAtVolumeDepth() override { return m_capFogAtVolumeDepth; }

        void SetCausticsEnabled(bool causticsEnabled) override;
        bool GetCausticsEnabled() override { return m_causticsEnabled; }
        void SetCausticIntensity(float causticIntensity) override;
        float GetCausticIntensity() override { return m_causticIntensity; }
        void SetCausticTiling(float causticTiling) override;
        float GetCausticTiling() override { return m_causticTiling; }
        void SetCausticHeight(float causticHeight) override;
        float GetCausticHeight() override { return m_causticHeight; }

        void SetSpillableVolume(float spillableVolume) override;
        float GetSpillableVolume() override { return m_spillableVolume; }
        void SetVolumeAccuracy(float volumeAccuracy) override;
        float GetVolumeAccuracy() override { return m_volumeAccuracy; }
        void SetExtrudeBorder(float extrudeBorder) override;
        float GetExtrudeBorder() override { return m_extrudeBorder; }
        void SetConvexBorder(bool convexBorder) override;
        bool GetConvexBorder() override { return m_convexBorder; }
        void SetObjectSizeLimit(float objectSizeLimit) override;
        float GetObjectSizeLimit() override { return m_objectSizeLimit; }

        void SetWaveSurfaceCellSize(float waveSurfaceCellSize) override;
        float GetWaveSurfaceCellSize() override { return m_waveSurfaceCellSize; }
        void SetWaveSpeed(float waveSpeed) override;
        float GetWaveSpeed() override { return m_waveSpeed; }
        void SetWaveDampening(float waveDampening) override;
        float GetWaveDampening() override { return m_waveDampening; }
        void SetWaveTimestep(float waveTimestep) override;
        float GetWaveTimestep() override { return m_waveTimestep; }
        void SetWaveSleepThreshold(float waveSleepThreshold) override;
        float GetWaveSleepThreshold() override { return m_waveSleepThreshold; }
        void SetWaveDepthCellSize(float waveDepthCellSize) override;
        float GetWaveDepthCellSize() override { return m_waveDepthCellSize; }
        void SetWaveHeightLimit(float waveHeightLimit) override;
        float GetWaveHeightLimit() override { return m_waveHeightLimit; }
        void SetWaveForce(float waveForce) override;
        float GetWaveForce() override { return m_waveForce; }
        void SetWaveSimulationAreaGrowth(float waveSimulationAreaGrowth) override;
        float GetWaveSimulationAreaGrowth() override { return m_waveSimulationAreaGrowth; }

        // Reflected member change callbacks
        void OnMaterialAssetChange();
        
        void OnMinSpecChange();
        void OnWaterAreaParamChange(); //Event for when a parameter that affects the water area changes
        void OnViewDistanceMultiplierChange();
        
        void OnFogDensityChange();
        void OnFogColorChange();
        void OnFogColorAffectedBySunChange();
        void OnFogShadowingChange();
        void OnCapFogAtVolumeDepthChange();

        void OnCausticsEnableChange();
        void OnCausticIntensityChange();
        void OnCausticTilingChange();
        void OnCausticHeightChange();

        void OnPhysicsParamChange(); //Event for when a parameter that effects physics (wave sim) changes
        
    protected:
        //Reflected members
        AzFramework::SimpleAssetReference<LmbrCentral::MaterialAsset> m_materialAsset;

        EngineSpec m_minSpec = EngineSpec::Low;
        float m_surfaceUScale = 1.0f;
        float m_surfaceVScale = 1.0f;
        float m_viewDistanceMultiplier = 1.0f;

        float m_fogDensity = 0.5f;
        AZ::Vector3 m_fogColor = AZ::Vector3(0.005f, 0.01f, 0.02f);
        float m_fogColorMultiplier = 1.0f;
        bool m_fogColorAffectedBySun = true;
        float m_fogShadowing = 0.5f;
        bool m_capFogAtVolumeDepth = false;
        
        bool m_causticsEnabled = true;
        float m_causticIntensity = 1.0f;
        float m_causticTiling = 1.0f;
        float m_causticHeight = 0.5f;

        float m_spillableVolume = 0.0f;
        float m_volumeAccuracy = 0.001f;
        float m_extrudeBorder = 0.0f;
        bool m_convexBorder = false;
        float m_objectSizeLimit = 0.001f;

        float m_waveSurfaceCellSize = 0.0f;
        float m_waveSpeed = 100.0f;
        float m_waveDampening = 0.2f;
        float m_waveTimestep = 0.02f;
        float m_waveSleepThreshold = 0.01f;
        float m_waveDepthCellSize = 8.0f;
        float m_waveHeightLimit = 7.0f;
        float m_waveForce = 10.0f;
        float m_waveSimulationAreaGrowth = 0.0f;     

        //Unreflected Members
        AZ::EntityId m_entityId = AZ::EntityId(0);
        AZ::u64 m_volumeId = 0;
        IWaterVolumeRenderNode* m_waterRenderNode = nullptr;
        AZ::Transform m_currentWorldTransform = AZ::Transform::CreateIdentity();
        AZStd::vector<Vec3> m_legacyVerts;
        AZStd::vector<AZ::Vector3> m_azVerts;
        float m_waterDepth = 10.0f;
        float m_waterDepthScaled = m_waterDepth;

        //Helper methods
        void Update();

        void UpdateAllWaterParams();
        void UpdateAuxPhysicsParams();
        void UpdatePhysicsAreaParams();
        void UpdateVertices();
        void UpdateWaterArea();

        void HandleBoxVerts();
        void HandleCylinderVerts();
        void HandlePolygonPrismVerts();
    };

    class WaterVolumeComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(WaterVolumeComponent, "{D1576A29-B1B5-4FE2-9D9E-3A7781561C49}", AZ::Component);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provides);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& requires);
        static void Reflect(AZ::ReflectContext* context);

        WaterVolumeComponent() {};
        explicit WaterVolumeComponent(WaterVolumeCommon* common)
        {
            m_common = *common;
        }
        ~WaterVolumeComponent() = default;

        void Init() override;
        void Activate() override;
        void Deactivate() override;

    private:
        //Reflected members
        WaterVolumeCommon m_common;
    };

} // namespace Water