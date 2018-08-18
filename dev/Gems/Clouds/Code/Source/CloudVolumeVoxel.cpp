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
#include "StdAfx.h"

#include "BitFiddling.h"

#include "CloudVolumeVoxel.h"
#include "CloudVolumeTexture.h"
#include "CloudVolumePerlinNoise.h"
#include "CloudVolumeRenderElement.h"

namespace CloudsGem
{
    static const int s_volumeShadowSize = 32;

    // Allocation helper methods for physics allocation
    static void* AllocHull(size_t s) 
    { 
        return new uint8[s]; 
    }
    
    // Allocation helper methods for physics allocation
    void FreeHull(void* p)
    { 
        delete[](uint8*) p; 
    }

    // Computes saturated value between 0 and 255
    static int Saturate(int value) 
    { 
        return (value < 0) ? 0 : ((value > 255) ? 255 : value); 
    }

    // Compute maximum distance
    static float ComputeMaximumDistance(const AABB& bbox)
    {
        float max = bbox.max.x - bbox.min.x;
        if (bbox.max.y - bbox.min.y > max) { max = bbox.max.y - bbox.min.y; }
        if (bbox.max.z - bbox.min.z > max) { max = bbox.max.z - bbox.min.z; }
        return max;
    }

    _smart_ptr<IRenderMesh> CloudVolumeHull::Create(const CloudVolumeDataSource& src, CloudVolumeDataSource::Points& points)
    {
        if (!points.empty())
        {
            Vec3* pPoints = &points[0];
            size_t numPoints = points.size();

            // compute convex hull
            index_t* pIndices = nullptr;
            int numTris = gEnv->pPhysicalWorld->GetPhysUtils()->qhull(pPoints, static_cast<int>(numPoints), pIndices, AllocHull);
            if (pIndices && numTris > 6)
            {
                // Determine the minimum set of vertices required.

                // Insert the 3 indices for each triangle. 
                // Note that duplicate indicies are ignored due to the way std::map::insert works. 
                // http://www.cplusplus.com/reference/map/map/insert/
                // When this loop completes we will be left with a unique set of indices
                std::map<int, int> v2v;
                std::map<int, int> mappy;
                for (int i = 0, idx = 0; i < numTris; ++i)
                {
                    v2v.insert({ pIndices[idx++], -1 });
                    v2v.insert({ pIndices[idx++], -1 });
                    v2v.insert({ pIndices[idx++], -1 });
                }

                // Generate a new sequence of indicies in the second element of the map
                int newIndexCount = 0;
                for (auto& mapping : v2v)
                {
                    mapping.second = newIndexCount++;
                }

                // Write used vertices into the hull points
                SAFE_DELETE_ARRAY(m_points);
                m_numPts = newIndexCount;
                m_points = new SVF_P3F[newIndexCount];
                int index = 0;
                for (auto& mapping : v2v)
                {
                    assert(mapping.second == index);
                    m_points[index++].xyz = pPoints[mapping.first];
                }

                // write remapped indices
                SAFE_DELETE_ARRAY(m_indices);
                m_numTris = numTris;
                m_indices = new vtx_idx[numTris * 3];
                for (int i = 0; i < numTris * 3; ++i)
                {
                    assert(v2v.find(pIndices[i]) != v2v.end());
                    m_indices[i] = v2v[pIndices[i]];
                }

                FreeHull(pIndices);

                return gEnv->pRenderer->CreateRenderMeshInitialized(m_points, m_numPts, eVF_P3F, m_indices, numTris * 3, prtTriangleList, "VolumeObjectHull", "VolumeObjectHull");
            }
            else
            {
                FreeHull(pIndices);
                SAFE_DELETE_ARRAY(m_points);
                SAFE_DELETE_ARRAY(m_indices);
                m_numPts = 0;
                m_numTris = 0;

            }
        }
        return nullptr;
    }

    CloudVolumeVoxel::CloudVolumeVoxel()
    {
        m_inverseWorldMatrix.SetIdentity();
        m_localBoundingBox.Reset();
        m_worldBoundingBox.Reset();

        for (uint32_t i = 0; i < RT_COMMAND_BUF_COUNT; ++i)
        {
            m_renderElements[i] = new CloudVolumeRenderElement();
        }
    }

    CloudVolumeVoxel::~CloudVolumeVoxel()
    {
        for (auto& element : m_renderElements)
        {
            SAFE_DELETE(element);
        }
        Reset();
    }

    
    void CloudVolumeVoxel::CalcBoundingBox(const CloudParticles& particles, AABB& boundingBox)
    {
        // Add every particle to the bounding box
        boundingBox.Reset();
        for (auto& particle : particles)
        {
            boundingBox.Add(particle.GetPosition(), particle.GetRadius());
        }
    }

    void CloudVolumeVoxel::CalcTightBounds(const AABB& bbox, AABB& tightBounds, float& scale)
    {
        // Compute maximum side length
        float max = ComputeMaximumDistance(bbox);

        // Normalize
        tightBounds.min.x = -(bbox.max.x - bbox.min.x) / max;
        tightBounds.max.x =  (bbox.max.x - bbox.min.x) / max;
        tightBounds.min.y = -(bbox.max.y - bbox.min.y) / max;
        tightBounds.max.y =  (bbox.max.y - bbox.min.y) / max;
        tightBounds.min.z = -(bbox.max.z - bbox.min.z) / max;
        tightBounds.max.z =  (bbox.max.z - bbox.min.z) / max;

        scale = max * 0.5f;
    }

    void CloudVolumeVoxel::AdjustBoundingBox(AABB& bbox)
    {
        // Compute maximum side length
        float distance = ComputeMaximumDistance(bbox);
        
        auto adjust = [distance](float& max, float& min) 
        { 
            float adj = (distance - (max - min)) * 0.5f;
            min -= adj;
            max += adj;
        };
        adjust(bbox.max.x, bbox.min.x);
        adjust(bbox.max.y, bbox.min.y);
        adjust(bbox.max.z, bbox.min.z);
    }

    uint8 CloudVolumeVoxel::TrilinearFilteredLookup(const CloudVolumeDataSource& density, const float lx, const float ly, const float lz)
    {
        if (lx < 0 || ly < 0 || lz < 0)
        {
            return 0;
        }

        int x = int(lx);
        int y = int(ly);
        int z = int(lz);

        if (x > (int)density.m_width - 2 || y > (int)density.m_height - 2 || z > (int)density.m_depth - 2)
        {
            return 0;
        }

        const uint8* src = &density[density.Idx(x, y, z)];

        int lerpX = (int)((lx - x) * 256.0f);
        int lerpY = (int)((ly - y) * 256.0f);
        int lerpZ = (int)((lz - z) * 256.0f);

        int _s000 = src[0];
        int _s001 = src[1];
        int _s010 = src[density.m_width];
        int _s011 = src[1 + density.m_width];

        src += density.m_slice;

        int _s100 = src[0];
        int _s101 = src[1];
        int _s110 = src[density.m_width];
        int _s111 = src[1 + density.m_width];

        int s00 = (_s000 << 8) + (_s001 - _s000) * lerpX;
        int s01 = (_s010 << 8) + (_s011 - _s010) * lerpX;
        int s0 = ((s00 << 8) + (s01 - s00) * lerpY) >> 8;

        int s10 = (_s100 << 8) + (_s101 - _s100) * lerpX;
        int s11 = (_s110 << 8) + (_s111 - _s110) * lerpX;
        int s1 = ((s10 << 8) + (s11 - s10) * lerpY) >> 8;

        return ((s0 << 8) + (s1 - s0) * lerpZ) >> 16;
    }

    void CloudVolumeVoxel::Voxelize(const CloudParticles& particles, float globalDensity, const AABB& bbox, CloudVolumeDataSource& trg)
    {
        using namespace Noise;

        CloudVolumeDataSource tmp(trg.m_width + 2, trg.m_height + 2, trg.m_depth + 2);

        // clear temporary volume
        for (size_t i(0); i < tmp.size(); ++i)
        {
            tmp[i] = 0;
        }

        // rasterize spheres
        for (const auto& particle : particles)
        {
            const Vec3& position = particle.GetPosition();
            float radius = particle.GetRadius();

            int sz = (int)floor((float)(trg.m_depth - 1) * ((position.z - radius) - bbox.min.z) / (bbox.max.z - bbox.min.z));
            int ez = (int)ceil((float)(trg.m_depth - 1) * ((position.z + radius) - bbox.min.z) / (bbox.max.z - bbox.min.z));

            int sy = (int)floor((float)(trg.m_height - 1) * ((position.y - radius) - bbox.min.y) / (bbox.max.y - bbox.min.y));
            int ey = (int)ceil((float)(trg.m_height - 1) * ((position.y + radius) - bbox.min.y) / (bbox.max.y - bbox.min.y));

            int sx = (int)floor((float)(trg.m_width - 1) * ((position.x - radius) - bbox.min.x) / (bbox.max.x - bbox.min.x));
            int ex = (int)ceil((float)(trg.m_width - 1) * ((position.x + radius) - bbox.min.x) / (bbox.max.x - bbox.min.x));

            float stepZ = (bbox.max.z - bbox.min.z) / (float)trg.m_depth;
            float wz = position.z - (bbox.min.z + ((float)sz + 0.5f) * stepZ);

            for (int z = sz; z <= ez; ++z, wz -= stepZ)
            {
                float dz2 = wz * wz;

                float stepY = (bbox.max.y - bbox.min.y) / (float)trg.m_height;
                float wy = position.y - (bbox.min.y + ((float)sy + 0.5f) * stepY);

                for (int y = sy; y <= ey; ++y, wy -= stepY)
                {
                    float dy2 = wy * wy;

                    float stepX = (bbox.max.x - bbox.min.x) / (float)trg.m_width;
                    float wx = position.x - (bbox.min.x + ((float)sx + 0.5f) * stepX);

                    size_t idx = tmp.Idx(sx + 1, y + 1, z + 1);
                    for (int x = sx; x <= ex; ++x, wx -= stepX, ++idx)
                    {
                        float dx2 = wx * wx;
                        float d = sqrt_tpl(dx2 + dy2 + dz2);
                        float v = max(1.0f - d / radius, 0.0f) * globalDensity;
                        assert(idx < tmp.size());
                        tmp[idx] = max(tmp[idx], (uint8)(v * 255.0f));
                    }
                }
            }
        }

#if 1
        // perturb volume using Perlin noise
        {
            float stepGx = 5.0f / (float)trg.m_width;
            float stepGy = 5.0f / (float)trg.m_height;
            float stepGz = 5.0f / (float)trg.m_depth;

            const float origBias = 0.25f;
            const float origFillDens = 1.2f;

            const uint8 bias = (uint8)(origBias * 256.0f);
            const uint32 biasNorm = (uint32)(256.0f * 256.0f * (origFillDens / (1.0f - origBias)));

            size_t idx = 0;

            float nz = 0;
            float gz = 0;
            for (uint32 z = 0; z < trg.m_depth; ++z, nz += 1.0f, gz += stepGz)
            {
                float ny = 0;
                float gy = 0;
                for (uint32 y = 0; y < trg.m_height; ++y, ny += 1.0f, gy += stepGy)
                {
                    float nx = 0;
                    float gx = 0;
                    for (uint32 x = 0; x < trg.m_width; ++x, nx += 1.0f, gx += stepGx, ++idx)
                    {
                        float gtx = (float)nx + 5.0f * PerlinNoise3D(gx, gy, gz, 2.0f, 2.1525f, 5);
                        float gty = (float)ny + 5.0f * PerlinNoise3D(gx + 21.132f, gy, gz, 2.0f, 2.1525f, 5);
                        float gtz = (float)nz + 5.0f * PerlinNoise3D(gx, gy + 3.412f, gz, 2.0f, 2.1525f, 5);

                        uint8 val = TrilinearFilteredLookup(tmp, gtx + 1.0f, gty + 1.0f, gtz + 1.0f);
                        trg[idx] = Saturate(Saturate(val - bias) * biasNorm >> 16);
                    }
                }
            }
        }
#endif
    }

    void CloudVolumeVoxel::PerPixelFilteredLookup(uint8* pShadow, const uint8* pDensity, int s00, int s01, int s10, int s11, int lerpX, int lerpY, int shadowStrength)
    {
        int _s00 = pShadow[s00];
        int _s01 = pShadow[s01];
        int _s10 = pShadow[s10];
        int _s11 = pShadow[s11];

        int s0 = (_s00 << 8) + (_s01 - _s00) * lerpX;
        int s1 = (_s10 << 8) + (_s11 - _s10) * lerpX;
        int s = ((s0 << 8) + (s1 - s0) * lerpY) >> 8;
        int d = *pDensity * shadowStrength;

        *pShadow = s * (65280 - d) >> 24; // 65280 = 1.0 * 255 * 256
    }

    // shadow is propagated from one slice to the next
    void CloudVolumeVoxel::PerSliceFilteredLookup(uint8* pShadow, const uint8* pDensity, int duOffset, int dvOffset, int s00, int s01, int s10, int s11, int lerpX, int lerpY, int shadowStrength)
    {
        for (int v = 0; v < s_volumeShadowSize - 1; ++v)
        {
            for (int u = 0; u < s_volumeShadowSize - 1; ++u)
            {
                int offset(duOffset * u + dvOffset * v);
                PerPixelFilteredLookup(&pShadow[offset], &pDensity[offset], s00, s01, s10, s11, lerpX, lerpY, shadowStrength);
            }
        }
    }

    void CloudVolumeVoxel::PerBlockFilteredLookup(uint8* pShadow, const uint8* pDensity, int duOffset, int dvOffset, int dwOffset, int s00, int s01, int s10, int s11, int lerpX, int lerpY, int shadowStrength)
    {
        for (int w = 1; w < s_volumeShadowSize; ++w)
        {
            int offset(dwOffset * w);
            PerSliceFilteredLookup(&pShadow[offset], &pDensity[offset], duOffset, dvOffset, s00, s01, s10, s11, lerpX, lerpY, shadowStrength);
        }
    }

    bool CloudVolumeVoxel::GeneratePoints(const CloudVolumeDataSource& src, CloudVolumeDataSource::Points& pts)
    {
        struct SPointGenerator
        {
        public:
            SPointGenerator(const CloudVolumeDataSource& _src, uint32 _size, CloudVolumeDataSource::Points& _pts): m_src(_src), m_pts(_pts), m_cache()
            {
                size_t cacheSize = (m_src.m_width * m_src.m_height * m_src.m_depth + 7) >> 3;
                m_cache.resize(cacheSize);

                for (size_t i = 0; i < cacheSize; ++i)
                {
                    m_cache[i] = 0;
                }

                if (Traverse(0, 0, 0, _size))
                {
                    PushPts(0, 0, 0, _size);
                }
            }

        private:
            void PushPt(uint32 x, uint32 y, uint32 z)
            {
                size_t idx = m_src.Idx(x, y, z);
                if ((idx >> 3) <= m_cache.size())
                {
                    size_t _idx = m_src.Idx(x, y, z);
                    if ((m_cache[_idx >> 3] & (1 << (_idx & 7))) == 0)
                    {
                        Vec3 p;
                        p.x = 2.0f * ((float)x / (float)m_src.m_width) - 1.0f;
                        p.y = 2.0f * ((float)y / (float)m_src.m_height) - 1.0f;
                        p.z = 2.0f * ((float)z / (float)m_src.m_depth) - 1.0f;
                        assert(fabs(p.x) <= 1.0f && fabs(p.y) <= 1.0f && fabs(p.z) <= 1.0f);
                        m_pts.push_back(p);
                        m_cache[_idx >> 3] |= 1 << (_idx & 7);
                    }
                }
            }

            void PushPts(uint32 x, uint32 y, uint32 z, uint32 size)
            {
                PushPt(x, y, z);
                PushPt(x + size, y, z);
                PushPt(x, y + size, z);
                PushPt(x + size, y + size, z);
                PushPt(x, y, z + size);
                PushPt(x + size, y, z + size);
                PushPt(x, y + size, z + size);
                PushPt(x + size, y + size, z + size);
            }

            bool Traverse(uint32 x, uint32 y, uint32 z, uint32 size)
            {
                if (size == 1)
                {
                    bool isSolid = false;

                    uint32 zs = max((int)z - 1, 0);
                    uint32 ze = min(z + 2, m_src.m_depth);
                    for (; zs < ze; ++zs)
                    {
                        uint32 ys = max((int)y - 1, 0);
                        uint32 ye = min(y + 2, m_src.m_height);
                        for (; ys < ye; ++ys)
                        {
                            uint32 xs = max((int)x - 1, 0);
                            uint32 xe = min(x + 2, m_src.m_width);
                            for (; xs < xe; ++xs)
                            {
                                isSolid |= m_src[m_src.Idx(xs, ys, zs)] != 0;
                                if (isSolid)
                                {
                                    return true;
                                }
                            }
                        }
                    }
                    return false;
                }

                uint32 ns = size >> 1;

                bool isSolid[8];
                isSolid[0] = Traverse(x, y, z, ns);
                isSolid[1] = Traverse(x + ns, y, z, ns);
                isSolid[2] = Traverse(x, y + ns, z, ns);
                isSolid[3] = Traverse(x + ns, y + ns, z, ns);
                isSolid[4] = Traverse(x, y, z + ns, ns);
                isSolid[5] = Traverse(x + ns, y, z + ns, ns);
                isSolid[6] = Traverse(x, y + ns, z + ns, ns);
                isSolid[7] = Traverse(x + ns, y + ns, z + ns, ns);

                bool mergeSubtrees = isSolid[0];
                for (int i = 1; i < 8; ++i)
                {
                    mergeSubtrees &= isSolid[i];
                }

                if (!mergeSubtrees)
                {
                    if (isSolid[0]) { PushPts(x, y, z, ns);}
                    if (isSolid[1]) { PushPts(x + ns, y, z, ns); }
                    if (isSolid[2]) { PushPts(x, y + ns, z, ns); }
                    if (isSolid[3]) { PushPts(x + ns, y + ns, z, ns); }
                    if (isSolid[4]) { PushPts(x, y, z + ns, ns); }
                    if (isSolid[5]) { PushPts(x + ns, y, z + ns, ns); }
                    if (isSolid[6]) { PushPts(x, y + ns, z + ns, ns); }
                    if (isSolid[7]) { PushPts(x + ns, y + ns, z + ns, ns); }
                }

                return mergeSubtrees;
            }

        private:
            const CloudVolumeDataSource& m_src;
            CloudVolumeDataSource::Points& m_pts;

            AZStd::vector<uint8> m_cache;
        };

        if (src.m_width != src.m_height || src.m_width != src.m_depth || src.m_height != src.m_depth)
        {
            return false;
        }

        uint32 size = src.m_width;
        if (!IsPowerOfTwo(size))
        {
            return false;
        }

        SPointGenerator gen(src, size, pts);
        return true;
    }

    bool CloudVolumeVoxel::CreateVolumeShadow(const Vec3& lightDir, float shadowStrength, const CloudVolumeDataSource& density, CloudVolumeDataSource& shadows)
    {
        for (size_t i(0); i < shadows.size(); ++i)
        {
            shadows[i] = 255;
        }

        if (!density.HasValidDimensions(s_volumeShadowSize) || !shadows.HasValidDimensions(s_volumeShadowSize))
        {
            assert(!"CreateVolumeShadow() -- density and/or shadow volume has invalid dimension!");
            return false;
        }

        float sun[3] = { lightDir.x, lightDir.y, lightDir.z };

        int majAxis = 0;
        {
            float abssun[3] = { fabsf(sun[0]), fabsf(sun[1]), fabsf(sun[2]) };
            if (abssun[1] > abssun[0]) { majAxis = 1; }
            if (abssun[2] > abssun[majAxis]) { majAxis = 2; }

            // project direction onto the -1..1 cube sides
            float fInv = 1.0f / abssun[majAxis];

            sun[0] *= fInv;
            sun[1] *= fInv;
            sun[2] *= fInv;
        }

        int du[3]{ 0 };
        int dv[3]{ 0 };
        int dw[3]{ 0 };

        int w = 0;

        if (sun[majAxis] > 0)
        {
            dw[majAxis] = 1;
        }
        else
        {
            dw[majAxis] = -1;
            w = s_volumeShadowSize - 1;
        }

        int secAxis = (majAxis + 1) % 3;
        int thirdAxis = (majAxis + 2) % 3;

        du[secAxis] = 1;
        dv[thirdAxis] = 1;

        int duOffset = du[0] + (du[1] + du[2] * s_volumeShadowSize) * s_volumeShadowSize;
        int dvOffset = dv[0] + (dv[1] + dv[2] * s_volumeShadowSize) * s_volumeShadowSize;
        int dwOffset = dw[0] + (dw[1] + dw[2] * s_volumeShadowSize) * s_volumeShadowSize;
        
        float lerpX = sun[secAxis];
        float lerpY = sun[thirdAxis];

        lerpX = -lerpX;
        lerpY = -lerpY;

        int prevSliceOffset = -dwOffset;
        int offset = -w * dwOffset;

        if (lerpX < 0)
        {
            lerpX += 1.0f;
            prevSliceOffset -= duOffset;
            offset += duOffset;
        }

        if (lerpY < 0)
        {
            lerpY += 1.0f;
            prevSliceOffset -= dvOffset;
            offset += dvOffset;
        }

        PerBlockFilteredLookup(
            &shadows[offset], 
            &density[offset], 
            duOffset, 
            dvOffset, 
            dwOffset,
            prevSliceOffset, 
            duOffset + prevSliceOffset, 
            dvOffset + prevSliceOffset, 
            duOffset + dvOffset + prevSliceOffset,
            (int)(lerpX * 256.0f), (int)(lerpY * 256.0f), 
            (int)(clamp_tpl(shadowStrength, 0.0f, 1.0f) * 256.0f));

        return true;
    }

    void CloudVolumeVoxel::CreateDownscaledVolume(const CloudVolumeDataSource& src, CloudVolumeDataSource& target)
    {
        AZ_Assert(src.m_width == 2 * target.m_width && src.m_height == 2 * target.m_height && src.m_depth == 2 * target.m_depth, "Target must have half dimensions of source");

        for (uint32 z = 0; z < target.m_depth; ++z)
        {
            for (uint32 y = 0; y < target.m_height; ++y)
            {
                for (uint32 x = 0; x < target.m_width; ++x)
                {
                    // Average surrounding 8 pixels
                    target[target.Idx(x, y, z)] = (uint8)(0.125f * (
                        src[src.Idx(2 * x, 2 * y, 2 * z)] +
                        src[src.Idx(2 * x + 1, 2 * y, 2 * z)] +
                        src[src.Idx(2 * x, 2 * y + 1, 2 * z)] +
                        src[src.Idx(2 * x + 1, 2 * y + 1, 2 * z)] +
                        src[src.Idx(2 * x, 2 * y, 2 * z + 1)] +
                        src[src.Idx(2 * x + 1, 2 * y, 2 * z + 1)] +
                        src[src.Idx(2 * x, 2 * y + 1, 2 * z + 1)] +
                        src[src.Idx(2 * x + 1, 2 * y + 1, 2 * z + 1)]));
                }
            }
        }
    }

    void CloudVolumeVoxel::CalculateShadows(const Vec3& newLightDir, float shadowStrength, const CloudVolumeDataSource* dataSource, IVolumeTexture* shadowTexture)
    {
        if (dataSource && shadowTexture)
        {
            // Create or recreate shadow data source if bounds have changed
            if (!m_shadowDataSource ||
                m_shadowDataSource->m_width != dataSource->m_width ||
                m_shadowDataSource->m_height != dataSource->m_height ||
                m_shadowDataSource->m_depth != dataSource->m_depth)
            {
                delete m_shadowDataSource;
                m_shadowDataSource = new CloudVolumeDataSource(dataSource->m_width, dataSource->m_height, dataSource->m_depth);
            }

            // Update shadow texture with new data
            if (m_shadowDataSource)
            {
                CreateVolumeShadow(newLightDir, shadowStrength, *dataSource, *m_shadowDataSource);
                shadowTexture->Update(m_shadowDataSource->m_width, m_shadowDataSource->m_height, m_shadowDataSource->m_depth, m_shadowDataSource->m_data);
            }
        }
    }

    void CloudVolumeVoxel::UpdateShadows()
    {
        // Update shadows when either the shadow strength has changed or the light direction has changed
        const float shadowStrength = 0.4f;
        Vec3 newLightDir = gEnv->p3DEngine->GetSunDirNormalized();
        Vec3 newLightDirLS = m_inverseWorldMatrix.TransformVector(newLightDir).GetNormalized();
        if (m_shadowStrength != shadowStrength || newLightDirLS.Dot(m_cachedLightDir) < 0.999f)
        {
            CalculateShadows(-newLightDirLS, shadowStrength, m_dataSource, m_shadowTexture);
            m_cachedLightDir = newLightDirLS;
            m_shadowStrength = shadowStrength;
        }
    }

    bool CloudVolumeVoxel::IsViewerInsideVolume(const SRenderingPassInfo& passInfo) const
    {
        const Vec3& absCamPosOS = m_inverseWorldMatrix.TransformPoint(passInfo.GetCamera().GetPosition()).abs();
        return absCamPosOS.x < m_localBoundingBox.max.x && absCamPosOS.y < m_localBoundingBox.max.y && absCamPosOS.z < m_localBoundingBox.max.z;
    }
    
    bool CloudVolumeVoxel::NearPlaneIntersectsVolume(const SRenderingPassInfo& passInfo) const
    {
        const CCamera& camera = passInfo.GetCamera();

        // Check if bounding box intersects the near clipping plane
        const Plane* nearPlane = camera.GetFrustumPlane(FR_PLANE_NEAR);
        Vec3 cameraPosition = camera.GetPosition();
        Vec3 pointOnNearPlane = cameraPosition - nearPlane->DistFromPlane(cameraPosition) * nearPlane->n;
        Vec3 pointOnNearPlaneObjectSpace = m_inverseWorldMatrix.TransformPoint(pointOnNearPlane);

        Vec3 nearPlaneObjectSpaceNormal = m_inverseWorldMatrix.TransformVector(nearPlane->n);
        f32 nearPlaneObjectSpaceDistance = -nearPlaneObjectSpaceNormal.Dot(pointOnNearPlaneObjectSpace);

        // Get extents
        float t(fabsf(nearPlaneObjectSpaceNormal.x * m_localBoundingBox.max.x) + fabsf(nearPlaneObjectSpaceNormal.y * m_localBoundingBox.max.y) + fabsf(nearPlaneObjectSpaceNormal.z * m_localBoundingBox.max.z));
        float t0(t + nearPlaneObjectSpaceDistance);
        float t1(-t + nearPlaneObjectSpaceDistance);

        return t0 * t1 < 0.0f;
    }

    Plane CloudVolumeVoxel::GetVolumeTraceStartPlane(bool viewerInsideVolume, const SRenderingPassInfo& passInfo) const
    {
        const CCamera& camera = passInfo.GetCamera();
        const Vec3 viewDirection = camera.GetViewdir();
        Vec3 viewPosition = camera.GetPosition();

        if (!viewerInsideVolume)
        {
            const Vec3 boundingBoxCorners[8] =
            {
                Vec3(m_localBoundingBox.min.x, m_localBoundingBox.min.y, m_localBoundingBox.min.z),
                Vec3(m_localBoundingBox.min.x, m_localBoundingBox.max.y, m_localBoundingBox.min.z),
                Vec3(m_localBoundingBox.max.x, m_localBoundingBox.max.y, m_localBoundingBox.min.z),
                Vec3(m_localBoundingBox.max.x, m_localBoundingBox.min.y, m_localBoundingBox.min.z),
                Vec3(m_localBoundingBox.min.x, m_localBoundingBox.min.y, m_localBoundingBox.max.z),
                Vec3(m_localBoundingBox.min.x, m_localBoundingBox.max.y, m_localBoundingBox.max.z),
                Vec3(m_localBoundingBox.max.x, m_localBoundingBox.max.y, m_localBoundingBox.max.z),
                Vec3(m_localBoundingBox.max.x, m_localBoundingBox.min.y, m_localBoundingBox.max.z)
            };

            Plane viewPlane(viewDirection, -viewDirection.Dot(viewPosition));

            // Find the closest bounding box corner to the view plane
            Vec3 minWorldSpaceCorner = m_worldMatrix * boundingBoxCorners[0];
            float minDistance = viewPlane.DistFromPlane(minWorldSpaceCorner);
            for (uint32 i = 1; i < 8; ++i)
            {
                Vec3 worldSpaceCorner = m_worldMatrix * boundingBoxCorners[i];
                float distanceToCorner = viewPlane.DistFromPlane(worldSpaceCorner);
                if (distanceToCorner < minDistance)
                {
                    minWorldSpaceCorner = worldSpaceCorner;
                    minDistance = distanceToCorner;
                }
            }
            viewPosition = minWorldSpaceCorner;
        }
        return Plane(viewDirection, -viewDirection.Dot(viewPosition));
    }


    float CloudVolumeVoxel::GetDistanceToCamera(const SRenderingPassInfo& passInfo) const
    {
        const uint32 numCorners = 8;
        const Vec3 worldSpaceBoundingBoxCorners[numCorners] =
        {
            m_worldMatrix * Vec3(m_localBoundingBox.min.x, m_localBoundingBox.min.y, m_localBoundingBox.min.z),
            m_worldMatrix * Vec3(m_localBoundingBox.min.x, m_localBoundingBox.max.y, m_localBoundingBox.min.z),
            m_worldMatrix * Vec3(m_localBoundingBox.max.x, m_localBoundingBox.max.y, m_localBoundingBox.min.z),
            m_worldMatrix * Vec3(m_localBoundingBox.max.x, m_localBoundingBox.min.y, m_localBoundingBox.min.z),
            m_worldMatrix * Vec3(m_localBoundingBox.min.x, m_localBoundingBox.min.y, m_localBoundingBox.max.z),
            m_worldMatrix * Vec3(m_localBoundingBox.min.x, m_localBoundingBox.max.y, m_localBoundingBox.max.z),
            m_worldMatrix * Vec3(m_localBoundingBox.max.x, m_localBoundingBox.max.y, m_localBoundingBox.max.z),
            m_worldMatrix * Vec3(m_localBoundingBox.max.x, m_localBoundingBox.min.y, m_localBoundingBox.max.z)
        };

        const CCamera& camera = passInfo.GetCamera();
        const Plane* nearPlane = camera.GetFrustumPlane(FR_PLANE_NEAR);
        const Vec3 viewPosition = camera.GetPosition();

        f32 distanceToCameraSquared = 0.0f;
        for (uint32 i = 0; i < numCorners; ++i)
        {
            const Vec3& corner = worldSpaceBoundingBoxCorners[i];
            Vec3 distanceToCorner = corner - viewPosition;
            float distanceSquared = distanceToCorner.GetLengthSquared();
            if (distanceSquared > distanceToCameraSquared && nearPlane->DistFromPlane(corner) < 0.0f)
            {
                distanceToCameraSquared = distanceSquared;
            }
        }
        return sqrt_tpl(distanceToCameraSquared);
    }

    void CloudVolumeVoxel::SetParticles(const CloudParticles& particles)
    {
        Reset();

        m_shadowTexture = new CloudVolumeTexture();
        m_shadowTexture->Create(s_volumeShadowSize, s_volumeShadowSize, s_volumeShadowSize, 0);
        
        static const int s_volumeSize = 64;
        CloudVolumeDataSource dataSource(s_volumeSize, s_volumeSize, s_volumeSize);
        
        AABB bounds;
        CalcBoundingBox(particles, bounds);
        CalcTightBounds(bounds, m_localBoundingBox, m_scale);
        AdjustBoundingBox(bounds);

        do
        {
            AZStd::vector<Vec3> points;
            dataSource.Reset();

            Voxelize(particles, m_density, bounds, dataSource);
            GeneratePoints(dataSource, points);

            CloudVolumeHull hull;
            m_hullMesh = hull.Create(dataSource, points);
            m_density -= m_hullMesh ? 0.0f : 0.05f;
        } 
        while (!m_hullMesh && m_density > 0);

        m_dataSource = new CloudVolumeDataSource(s_volumeShadowSize, s_volumeShadowSize, s_volumeShadowSize);
        CreateDownscaledVolume(dataSource, *m_dataSource);
        
        m_densityTexture = new CloudVolumeTexture();
        m_densityTexture->Create(dataSource.m_width, dataSource.m_height, dataSource.m_depth, dataSource.m_data);
    }

    void CloudVolumeVoxel::Refresh(CloudParticleData& cloudData, const Matrix34& worldMatrix)
    {
        SetParticles(cloudData.GetParticles());
        Update(worldMatrix, cloudData.GetOffset());
    }

    void CloudVolumeVoxel::Update(const Matrix34& worldMatrix, const Vec3& offset)
    {
        Matrix34 scaleMatrix = Matrix34::CreateScale(Vec3(m_scale));
        m_worldMatrix = worldMatrix * scaleMatrix;
        m_worldMatrix.AddTranslation(-offset);
        m_inverseWorldMatrix = m_worldMatrix.GetInverted();
        m_worldBoundingBox.SetTransformedAABB(m_worldMatrix, m_localBoundingBox);
    }

    void CloudVolumeVoxel::Reset()
    {
        SAFE_DELETE(m_shadowDataSource);
        SAFE_DELETE(m_shadowTexture);
        SAFE_DELETE(m_densityTexture);
        SAFE_DELETE(m_dataSource);

        InvalidateCachedLightDir();

        m_inverseWorldMatrix.SetIdentity();
        m_localBoundingBox.Reset();
        m_worldBoundingBox.Reset();
    }


    void CloudVolumeVoxel::Render(const struct SRendParams& params, const SRenderingPassInfo& passInfo, float alpha, int isAfterWater)
    {
        if (!m_hullMesh) return;

        UpdateShadows();

        const int threadId = passInfo.ThreadID();
        const CCamera& camera = passInfo.GetCamera();

        IRenderer* renderer = gEnv->pRenderer;
        CRenderObject* renderObject = renderer->EF_GetObject_Temp(threadId);
        renderObject->m_II.m_Matrix = m_worldMatrix;
        renderObject->m_fSort = 0;
        renderObject->m_fDistance = GetDistanceToCamera(passInfo);
        renderObject->m_II.m_AmbColor = params.AmbientColor;
        renderObject->m_fAlpha = params.fAlpha * alpha;

        Vec3 position = m_worldMatrix.GetTranslation();
        bool viewerInsideVolume = IsViewerInsideVolume(passInfo);

        CloudVolumeRenderElement* renderElement = m_renderElements[threadId];
        renderElement->m_center = position;
        renderElement->m_inverseWorldMatrix = m_inverseWorldMatrix;
        renderElement->m_viewerInsideVolume = viewerInsideVolume;
        renderElement->m_volumeTraceStartPlane = GetVolumeTraceStartPlane(viewerInsideVolume, passInfo);
        renderElement->m_renderBoundsOS = m_localBoundingBox;
        renderElement->m_pHullMesh = m_hullMesh;
        renderElement->m_eyePosInWS = passInfo.GetCamera().GetPosition();
        renderElement->m_eyePosInOS = m_inverseWorldMatrix * renderElement->m_eyePosInWS;
        renderElement->m_nearPlaneIntersectsVolume = NearPlaneIntersectsVolume(passInfo);
        renderElement->m_pDensVol = m_densityTexture;
        renderElement->m_pShadVol = m_shadowTexture;
        renderElement->m_alpha = alpha;
        renderElement->m_scale = m_scale;

        SShaderItem& shaderItem = params.pMaterial ? params.pMaterial->GetShaderItem(0) : m_material->GetShaderItem(0);
        renderer->EF_AddEf(renderElement->m_gemRE, shaderItem, renderObject, passInfo, EFSLIST_TRANSP, isAfterWater, SRendItemSorter(params.rendItemSorter));
    }

    
}