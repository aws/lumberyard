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

#include <AzCore/Math/Plane.h>
#include <AzCore/Component/TransformBus.h>
#include <AZCore/Math/Color.h>
#include <AZCore/Math/Vector2.h>

#include <LmbrCentral/Rendering/MaterialAsset.h>
#include <LmbrCentral/Shape/SplineComponentBus.h>
#include <LmbrCentral/Rendering/MaterialOwnerBus.h>

#include "SplineGeometry.h"
#include "IEntityRenderState.h"
#include "RoadsAndRivers/RoadsAndRiversBus.h"

namespace RoadsAndRivers
{
    /**
     * Implementation class for the RiverComponent. 
     * Contains parameters for the river and logic for the creation of the river rendering nodes
     */
    class River
        : public SplineGeometry
        , private AZ::TransformNotificationBus::Handler
        , private LmbrCentral::MaterialOwnerRequestBus::Handler
        , private RiverRequestBus::Handler
    {
    public:
        AZ_RTTI(River, "{A9181128-B403-4255-BB56-1FAA6CEDB1F1}", SplineGeometry);
        AZ_CLASS_ALLOCATOR_DECL

        using RiverRenderNode = SectorRenderNode<IWaterVolumeRenderNode>;

        River();

        static void Reflect(AZ::ReflectContext* context);
        void Activate(AZ::EntityId entityId) override;
        void Deactivate() override;

        // MaterialOwnerRequestBus
        void SetMaterial(_smart_ptr<IMaterial> material) override;
        _smart_ptr<IMaterial> GetMaterial() override;

        void SetMaterialHandle(const LmbrCentral::MaterialHandle& materialHandle) override;
        LmbrCentral::MaterialHandle GetMaterialHandle() override;

        // RiverRequestsBus handler
        void SetWaterVolumeDepth(float waterVolumeDepth) override;
        float GetWaterVolumeDepth() override;

        void SetTileWidth(float tileWidth) override;
        float GetTileWidth() override;

        void SetWaterCapFogAtVolumeDepth(bool waterCapFogAtVolumeDepth) override;
        bool GetWaterCapFogAtVolumeDepth() override;

        void SetWaterFogDensity(float waterFogDensity) override;
        float GetWaterFogDensity() override;

        void SetFogColor(AZ::Color fogColor) override;
        AZ::Color GetFogColor() override;

        void SetFogColorAffectedBySun(bool fogColorAffectedBySun) override;
        bool GetFogColorAffectedBySun() override;

        void SetWaterFogShadowing(float waterFogShadowing) override;
        float GetWaterFogShadowing() override;

        void SetWaterCaustics(bool waterCaustics) override;
        bool GetWaterCaustics() override;

        void SetWaterCausticIntensity(float waterCausticIntensity) override;
        float GetWaterCausticIntensity() override;

        void SetWaterCausticHeight(float waterCausticHeight) override;
        float GetWaterCausticHeight() override;

        void SetWaterCausticTiling(float waterCausticTiling) override;
        float GetWaterCausticTiling() override;

        void SetPhysicsEnabled(bool physicalize) override;
        bool GetPhysicsEnabled() override;

        void SetWaterStreamSpeed(float waterStreamSpeed) override;
        float GetWaterStreamSpeed() override;

        AZ::Plane GetWaterSurfacePlane() override;

        /**
         * Triggers full rebuild of the river object, including geometry and render node generation
         */
        void Rebuild() override;

        /**
         * Destroys geometry and rendering nodes
         */
        void Clear() override;

    private:
        // Rendering properties
        AzFramework::SimpleAssetReference<LmbrCentral::MaterialAsset> m_material;
        float m_tileWidth = 1.0f;

        // Fog properties
        float m_waterFogDensity = 0.5f;
        float m_waterFogShadowing = 0.5f;
        AZ::Color m_fogColor{0.0f, 0.0f, 0.0f, 1.0f };
        bool m_fogColorAffectedBySun = true;
        bool m_waterCapFogAtVolumeDepth = false;

        // Caustics properties
        float m_waterCausticIntensity = 20.0f;
        float m_waterCausticTiling = 40.0f;
        float m_waterCausticHeight = 0.0f;
        bool m_waterCaustics = true;

        // Physics properties
        bool m_physicalize = false;
        float m_waterVolumeDepth = 10.0f;
        float m_waterStreamSpeed = 0.0f;

        AZStd::vector<RiverRenderNode> m_renderNodes;
        AZ::Plane m_fogPlane = AZ::Plane::CreateFromNormalAndDistance(AZ::Vector3(0, 0, 1), 0);

        void GenerateRenderNodes();
        void SetRenderProperties();
        void UpdateRenderNodeWithAssetMaterial();
        void SetPhysicalProperties();

        void BindWithPhysics();

        void GeneralPropertyModified() override;
        void WidthPropertyModified() override;
        void RenderingPropertyModified() override;

        void MaterialChanged();
        void WaterVolumeDepthModified();
        void PhysicsPropertyModified();
        void TilingPropertyModified();

        // TransformNotificationBus::Handler
        virtual void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;
    };
}
