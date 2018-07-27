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

#include <AzCore/Component/TransformBus.h>

#include <LmbrCentral/Rendering/MaterialAsset.h>
#include <LmbrCentral/Shape/SplineComponentBus.h>
#include <LmbrCentral/Rendering/MaterialOwnerBus.h>

#include "SplineGeometry.h"
#include "IEntityRenderState.h"
#include "RoadsAndRivers/RoadsAndRiversBus.h"

namespace RoadsAndRivers
{
    /**
     * Implementation class for the RoadComponent. 
     * Contains parameters for the road and logic for the creation of the road rendering nodes
     */
    class Road
        : public SplineGeometry
        , private AZ::TransformNotificationBus::Handler
        , private LmbrCentral::MaterialOwnerRequestBus::Handler
        , private RoadRequestBus::Handler
    {
    public:
        AZ_RTTI(Road, "{1D3FAF38-C5AE-4ADA-8423-DC0D6C4A11AF}", SplineGeometry);
        AZ_CLASS_ALLOCATOR_DECL

            using RoadRenderNode = SectorRenderNode<IRoadRenderNode>;

        Road();

        static void Reflect(AZ::ReflectContext* context);
        void Activate(AZ::EntityId entityId) override;
        void Deactivate() override;

        // MaterialOwnerRequestBus
        void SetMaterial(_smart_ptr<IMaterial> material) override;
        _smart_ptr<IMaterial> GetMaterial() override;

        void SetMaterialHandle(LmbrCentral::MaterialHandle handle) override;
        LmbrCentral::MaterialHandle GetMaterialHandle() override;

        // RoadRequestBus
        void SetIgnoreTerrainHoles(bool ignoreTerrainHoles) override;
        bool GetIgnoreTerrainHoles() override;

        /**
         * Triggers full rebuild of the road object, including geometry and render node generation
         */
        void Rebuild() override;

        /**
         * Destroys geometry and rendering nodes
         */
        void Clear() override;

    private:
        AzFramework::SimpleAssetReference<LmbrCentral::MaterialAsset> m_material;
        AZStd::vector<RoadRenderNode> m_roadRenderNodes;
        bool m_ignoreTerrainHoles = false;

        void GenerateRenderNodes();
        void SetRenderProperties();
        void UpdateRenderNodeWithAssetMaterial();

        void GeneralPropertyModified() override;
        void WidthPropertyModified() override;
        void RenderingPropertyModified() override;

        void MaterialPropertyChanged();

        // TransformNotificationBus::Handler
        virtual void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;
    };
}
