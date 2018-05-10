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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#pragma once

#include "CloudParticle.h"
namespace CloudsGem
{
    class CloudVolumeDataSource
    {
    public:
        using Points = AZStd::vector<Vec3>;

        CloudVolumeDataSource(uint32 width, uint32 height, uint32 depth)
            : m_width(width)
            , m_height(height)
            , m_depth(depth)
            , m_slice(width * depth)
        {
            Reset();
        }

        ~CloudVolumeDataSource() { SAFE_DELETE_ARRAY(m_data) }

        size_t size() const { return m_width * m_height * m_depth; }
        size_t Idx(uint32 x, uint32 y, uint32 z) const { return (z * m_height + y) * m_width + x; }
        uint8& operator[](size_t index) { return m_data[index]; }
        const uint8& operator[](size_t index) const { return m_data[index]; }
        bool HasValidDimensions(int size) const { return m_width == size && m_height == size && m_depth == size; }
        
        void Reset()
        {
            SAFE_DELETE_ARRAY(m_data);
            m_data = new uint8[m_width * m_height * m_depth];
        }

        // Members
        uint32 m_width{ 0 };
        uint32 m_height{ 0 };
        uint32 m_depth{ 0 };
        uint32 m_slice{ 0 };
        uint8* m_data{ nullptr };
    };

    class CloudVolumeHull
    {
    public:

        CloudVolumeHull() = default;
        ~CloudVolumeHull()
        {
            SAFE_DELETE_ARRAY(m_points);
            SAFE_DELETE_ARRAY(m_indices);
        }

        _smart_ptr<IRenderMesh> Create(const CloudVolumeDataSource& src, CloudVolumeDataSource::Points& points);

        size_t m_numPts{ 0 };               ///< Number of points in hull
        size_t m_numTris{ 0 };              ///< Number of triangles in hull
        SVF_P3F* m_points{ nullptr };       ///< Point data
        vtx_idx* m_indices{ nullptr };      ///< Triangle data
    };

    class CloudVolumeRenderElement;
    class CloudVolumeTexture;
    class CloudParticleData;
    class CloudVolumeVoxel : public ICloudVolume
    {
    public:
        CloudVolumeVoxel();
        virtual ~CloudVolumeVoxel();

        const AABB& GetBoundingBox() const override { return m_worldBoundingBox; }
        MaterialPtr GetMaterial() { return m_material; }

        void SetBoundingBox(const AABB& box) override { m_worldBoundingBox = box; }
        void SetMaterial(MaterialPtr material) { m_material = material; }
        void SetDensity(float density) { m_density = density; }
        void Update(const Matrix34& worldMatrix, const Vec3& offset) override;
        void Refresh(CloudParticleData& cloudData, const Matrix34& worldMatrix) override;
        void Render(const struct SRendParams& rParams, const SRenderingPassInfo& passInfo, float alpha, int isAfterWater) override;

    private:

        /**
         * Sets the particles that will be used to generate the cloud volume
         * @param particles particles used to generate volume
         * @param box bounding box for particles
         */
        void SetParticles(const CloudParticles& particles);

        void UpdateShadows();
        void InvalidateCachedLightDir() { m_cachedLightDir.Set(0.0f, 0.0f, 0.0f); }
        bool NearPlaneIntersectsVolume(const SRenderingPassInfo& passInfo) const;
        bool IsViewerInsideVolume(const SRenderingPassInfo& passInfo) const;
        float GetDistanceToCamera(const SRenderingPassInfo& passInfo) const;
        Plane GetVolumeTraceStartPlane(bool viewerInsideVolume, const SRenderingPassInfo& passInfo) const;
        bool CreateVolumeShadow(const Vec3& lightDir, float shadowStrength, const CloudVolumeDataSource& density, CloudVolumeDataSource& shadows);
        void CreateDownscaledVolume(const CloudVolumeDataSource& src, CloudVolumeDataSource& dst);
        void CalculateShadows(const Vec3& newLightDir, float shadowStrength, const CloudVolumeDataSource* pVolSrc, IVolumeTexture* pShadDst);
        void CalcBoundingBox(const CloudParticles& volDesc, AABB& bbox);
        void CalcTightBounds(const AABB& bbox, AABB& tightBounds, float& scale);
        void AdjustBoundingBox(AABB& bbox);
        uint8 TrilinearFilteredLookup(const CloudVolumeDataSource& density, const float lx, const float ly, const float lz);
        void Reset();
        void Voxelize(const CloudParticles& volDesc, float globalDensity, const AABB& bbox, CloudVolumeDataSource& trg);
        void PerPixelFilteredLookup(uint8* shadow, const uint8* density, int s00, int s01, int s10, int s11, int lX, int lY, int shadowStrength);
        void PerSliceFilteredLookup(uint8* shadow, const uint8* density, int du, int dv, int s00, int s01, int s10, int s11, int lerpX, int lerpY, int shadowStrength);
        void PerBlockFilteredLookup(uint8* shadow, const uint8* density, int du, int dv, int dw, int s00, int s01, int s10, int s11, int X, int Y, int shadowStrength);
        bool GeneratePoints(const CloudVolumeDataSource& source, CloudVolumeDataSource::Points& points);

        CloudVolumeDataSource* m_shadowDataSource{ nullptr };
        CloudVolumeDataSource* m_dataSource{ nullptr };
        float m_shadowStrength{ 1 };
        float m_scale{ 1.0f };
        float m_density{1.0f};
        Matrix34 m_worldMatrix;
        Matrix34 m_inverseWorldMatrix;
        AABB m_localBoundingBox;
        AABB m_worldBoundingBox;
        Vec3 m_cachedLightDir{ 0.0f, 0.0f, 0.0f };

        IVolumeTexture* m_shadowTexture{ nullptr };
        IVolumeTexture* m_densityTexture{ nullptr };
        _smart_ptr<IRenderMesh> m_hullMesh{ nullptr };
        _smart_ptr<IMaterial> m_material{nullptr};
        using CloudVolumeRenderElementArray = AZStd::array<CloudVolumeRenderElement*, RT_COMMAND_BUF_COUNT>;
        CloudVolumeRenderElementArray m_renderElements;
    };
}