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

#include <Vegetation/InstanceSpawner.h>
#include <AzCore/Asset/AssetCommon.h>
#include <LmbrCentral/Rendering/MaterialAsset.h>
#include <LmbrCentral/Rendering/MeshAsset.h>
#include <StatObjBus.h>

namespace Vegetation
{
    /**
    * Instance spawner of legacy vegetation instances.  This can spawn both CVegetation and
    * MergedMesh instances.
    * Technically these could (should?) be split into two separate spawners, but they have
    * been left together as a user convenience - this makes it easier to try turning merged
    * meshes on and off for any given descriptor.  If they were split apart, switching would
    * take extra clicks to set up the Mesh Asset (and optional material override) again.  By
    * leaving it together, they share the same data, so it's possible to switch by just checking
    * a checkbox.
    */
    class LegacyVegetationInstanceSpawner
        : public InstanceSpawner
        , private AZ::Data::AssetBus::MultiHandler
    {
    public:
        AZ_RTTI(LegacyVegetationInstanceSpawner, "{9DFEE312-2C4A-4DC7-B4BD-86BCF81A1AD6}", InstanceSpawner);
        AZ_CLASS_ALLOCATOR(LegacyVegetationInstanceSpawner, AZ::SystemAllocator, 0);
        static void Reflect(AZ::ReflectContext* context);

        LegacyVegetationInstanceSpawner();
        virtual ~LegacyVegetationInstanceSpawner();

        //! Start loading any assets that the spawner will need.
        void LoadAssets() override;

        //! Unload any assets that the spawner loaded.
        void UnloadAssets() override;

        //! Perform any extra initialization needed at the point of registering with the vegetation system.
        void OnRegisterUniqueDescriptor() override;

        //! Perform any extra cleanup needed at the point of unregistering with the vegetation system.
        void OnReleaseUniqueDescriptor() override;

        //! Does this exist but have empty asset references?
        bool HasEmptyAssetReferences() const override;

        //! Has this finished loading any assets that are needed?
        bool IsLoaded() const override;

        //! Are the assets loaded, initialized, and spawnable?
        bool IsSpawnable() const override;

        //! Does this spawner have the capability to provide radius data?
        bool HasRadiusData() const override { return true; }

        //! Radius of the instances that will be spawned.
        float GetRadius() const override;

        //! Display name of the instances that will be spawned.
        AZStd::string GetName() const override;

        //! Create a single instance.
        InstancePtr CreateInstance(const InstanceData& instanceData) override;

        //! Destroy a single instance.
        void DestroyInstance(InstanceId id, InstancePtr instance) override;

        AZStd::string GetMeshAssetPath() const;
        void SetMeshAssetPath(const AZStd::string& assetPath);
        AZStd::string GetMaterialAssetPath() const;
        void SetMaterialAssetPath(const AZStd::string& path);

        static AZStd::shared_ptr<InstanceSpawner> ConvertLegacyDescriptorData(AZ::SerializeContext::DataElementNode& classElement);

    private:
        bool DataIsEquivalent(const InstanceSpawner& rhs) const override;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Data::AssetBus::Handler
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

        // This is written in the negative tense because we want to set the ReadOnly attribute for
        // certain bending parameters only when we do NOT have AutoMerge enabled.
        bool AutoMergeIsDisabled() const;

        AZ::u32 MeshAssetChanged();
        void ResetMeshAsset();

        void UpdateCachedValues();

        void RequestStatInstGroupId();
        void ReleaseStatInstGroupId();

        _smart_ptr<IMaterial> LoadMaterialAsset(const AZStd::string& materialPath);

        I3DEngine* m_engine = nullptr;

        //! Current 3D engine group Id of this object
        StatInstGroupId m_statInstGroupId = StatInstGroupEvents::s_InvalidStatInstGroupId;

        // Cached values so that asset and statObj aren't accessed on other threads
        bool m_meshLoaded = false;
        bool m_meshSpawnable = false;
        float m_meshRadius = 0.0f;

        // Legacy Vegetation spawner settings.

        // asset data
        AZ::Data::Asset<LmbrCentral::MeshAsset> m_meshAsset;
        AzFramework::SimpleAssetReference<LmbrCentral::MaterialAsset> m_materialAsset;
        _smart_ptr<IMaterial> m_materialOverride = nullptr;

        // misc settings
        bool  m_autoMerge = true;
        bool  m_useTerrainColor = false;

        // view settings
        float m_viewDistanceRatio = 1.0f;
        float m_lodDistanceRatio = 1.0f;

        // bending
        float m_windBending = 0.1f;
        float m_airResistance = 1.0f;
        float m_stiffness = 0.5f;
        float m_damping = 2.5f;
        float m_variance = 0.6f;
    };

} // namespace Vegetation
