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

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/any.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <LmbrCentral/Rendering/MaterialAsset.h>
#include <LmbrCentral/Rendering/MeshAsset.h>
#include <SurfaceData/SurfaceDataTypes.h>

namespace Vegetation
{
    static constexpr float s_defaultLowerSurfaceDistanceInMeters = -1000.0f;
    static constexpr float s_defaultUpperSurfaceDistanceInMeters = 1000.0f;
    struct SurfaceTagDistance final
    {
        AZ_CLASS_ALLOCATOR(SurfaceTagDistance, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(SurfaceTagDistance, "{2AB6096D-C7C0-4C5E-AA84-7CA804A9680C}");
        static void Reflect(AZ::ReflectContext* context);

        bool operator==(const SurfaceTagDistance& rhs) const;

        SurfaceData::SurfaceTagVector m_tags;
        float m_upperDistanceInMeters = s_defaultUpperSurfaceDistanceInMeters;
        float m_lowerDistanceInMeters = s_defaultLowerSurfaceDistanceInMeters;

        size_t GetNumTags() const;
        AZ::Crc32 GetTag(int tagIndex) const;
        void RemoveTag(int tagIndex);
        void AddTag(const AZStd::string& tag);
    };

    enum class BoundMode : AZ::u8
    {
        Radius = 0,
        MeshRadius
    };

    enum class OverrideMode : AZ::u8
    {
        Disable = 0, // Ignore descriptor level values
        Replace, // Replace component level values with descriptor level values
        Extend, // Combine component level values with descriptor level values
    };

    static const AZ::Uuid VegetationDescriptorTypeId = "{A5A5E7F7-FC36-4BD1-8A93-21362574B9DA}";

    /**
    * Details used to create vegetation instances
    */
    struct Descriptor final
    {
        AZ_CLASS_ALLOCATOR(Descriptor, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(Descriptor, VegetationDescriptorTypeId);
        static void Reflect(AZ::ReflectContext* context);

        Descriptor();
        ~Descriptor();

        bool operator==(const Descriptor& rhs) const;

        void ResetAssets(bool resetMaterialOverride = true);
        void LoadAssets();
        void UpdateAssets();

        // basic
        AZ::Data::Asset<LmbrCentral::MeshAsset> m_meshAsset;
        bool m_meshLoaded = false; //cached value to not access asset or statObj on other threads
        float m_meshRadius = 0.0f; //cached value to not access asset or statObj on other threads

        AzFramework::SimpleAssetReference<LmbrCentral::MaterialAsset> m_materialAsset;
        _smart_ptr<IMaterial> m_materialOverride = nullptr;

        float m_weight = 1.0f;
        bool  m_autoMerge = true;
        bool  m_useTerrainColor = false;
        bool  m_advanced = false;

        // (expert)

        // view settings
        float m_viewDistanceRatio = 1.0f;
        float m_lodDistanceRatio = 1.0f;

        // surface tag settings
        SurfaceTagDistance m_surfaceTagDistance;

        // surface tag filter settings
        OverrideMode m_surfaceFilterOverrideMode = OverrideMode::Disable;
        SurfaceData::SurfaceTagVector m_inclusiveSurfaceFilterTags;
        SurfaceData::SurfaceTagVector m_exclusiveSurfaceFilterTags;

        // radius
        bool m_radiusOverrideEnabled = false;
        BoundMode m_boundMode = BoundMode::Radius;
        float m_radiusMin = 0.0f;
        AZ_INLINE float GetRadius() const { return (m_boundMode == BoundMode::MeshRadius) ? m_meshRadius : m_radiusMin; }
        AZ_INLINE bool IsRadiusReadOnly() const { return m_boundMode != BoundMode::Radius; }

        // surface alignment
        bool m_surfaceAlignmentOverrideEnabled = false;
        float m_surfaceAlignmentMin = 0.0f;
        float m_surfaceAlignmentMax = 1.0f;

        // position
        bool m_positionOverrideEnabled = false;
        float m_positionMinX = -0.3f;
        float m_positionMaxX = 0.3f;
        float m_positionMinY = -0.3f;
        float m_positionMaxY = 0.3f;
        float m_positionMinZ = 0.0f;
        float m_positionMaxZ = 0.0f;
        AZ_INLINE AZ::Vector3 GetPositionMin() const { return AZ::Vector3(m_positionMinX, m_positionMinY, m_positionMinZ); }
        AZ_INLINE AZ::Vector3 GetPositionMax() const { return AZ::Vector3(m_positionMaxX, m_positionMaxY, m_positionMaxZ); }

        // rotation
        bool m_rotationOverrideEnabled = false;
        float m_rotationMinX = 0.0f;
        float m_rotationMaxX = 0.0f;
        float m_rotationMinY = 0.0f;
        float m_rotationMaxY = 0.0f;
        float m_rotationMinZ = -180.0f;
        float m_rotationMaxZ = 180.0f;
        AZ_INLINE AZ::Vector3 GetRotationMin() const { return AZ::Vector3(m_rotationMinX, m_rotationMinY, m_rotationMinZ); }
        AZ_INLINE AZ::Vector3 GetRotationMax() const { return AZ::Vector3(m_rotationMaxX, m_rotationMaxY, m_rotationMaxZ); }

        // scale
        bool m_scaleOverrideEnabled = false;
        float m_scaleMin = 0.1f;
        float m_scaleMax = 1.0f;

        // altitude filter
        bool m_altitudeFilterOverrideEnabled = false;
        float m_altitudeFilterMin = 0.0f;
        float m_altitudeFilterMax = 128.0f;

        // slope filter
        bool m_slopeFilterOverrideEnabled = false;
        float m_slopeFilterMin = 0.0f;
        float m_slopeFilterMax = 20.0f;

        // bending
        float m_windBending = 0.1f;
        float m_airResistance = 1.0f;
        float m_stiffness = 0.5f;
        float m_damping = 2.5f;
        float m_variance = 0.6f;

        AZStd::string GetMeshAssetPath() const;
        void SetMeshAssetPath(const AZStd::string& assetPath);
        AZStd::string GetMaterialAssetPath() const;
        void SetMaterialAssetPath(const AZStd::string& path);

        size_t GetNumInclusiveSurfaceFilterTags() const;
        AZ::Crc32 GetInclusiveSurfaceFilterTag(int tagIndex) const;
        void RemoveInclusiveSurfaceFilterTag(int tagIndex);
        void AddInclusiveSurfaceFilterTag(const AZStd::string& tag);

        size_t GetNumExclusiveSurfaceFilterTags() const;
        AZ::Crc32 GetExclusiveSurfaceFilterTag(int tagIndex) const;
        void RemoveExclusiveSurfaceFilterTag(int tagIndex);
        void AddExclusiveSurfaceFilterTag(const AZStd::string& tag);

        const char* GetMeshName();

    private:
        // This is written in the negative tense because we want to set the ReadOnly attribute for
        // certain bending parameters only when we do NOT have AutoMerge enabled.
        bool AutoMergeIsDisabled() const;

        AZ::u32 GetAdvancedGroupVisibility() const;

        void UpdateMeshAssetName(bool forceUpdate = false);
        AZ::u32 MeshAssetChanged();

        AZStd::string m_meshAssetName;
    };

    using DescriptorPtr = AZStd::shared_ptr<Descriptor>;
    using DescriptorPtrVec = AZStd::vector<DescriptorPtr>;

} // namespace Vegetation