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

// Description : SVO brick implementation

#include "StdAfx.h"

#include <AzCore/Debug/Profiler.h>
#include <AzCore/Math/Plane.h>
#include <AzCore/std/parallel/lock.h>

#include "SvoBrick.h"
#include "I3DEngine.h"
#include "IEntityRenderState.h"
#include "ITerrain.h"



#include "MathConversion.h"

namespace SVOGI
{
#define SVO_CPU_VOXELIZATION_OFFSET_MESH     0.02f
#define SVO_CPU_VOXELIZATION_OFFSET_TERRAIN -0.04f
#define SVO_CPU_VOXELIZATION_OFFSET_VISAREA  (gEnv->pConsole->GetCVar("e_svoMinNodeSize")->GetFVal() / (float)brickDimension)
#define SVO_CPU_VOXELIZATION_POOL_SIZE_MB (12 * 1024)
#define SVO_CPU_VOXELIZATION_AREA_SCALE 200.f

    //Free functions for processing triangles to brick data

    bool GetBarycentricTC(const AZ::Vector3& a, const AZ::Vector3& b, const AZ::Vector3& c,
        AZ::VectorFloat& u, AZ::VectorFloat& v, AZ::VectorFloat& w,
        const AZ::Vector3& p, const AZ::VectorFloat& fBorder)
    {
        AZ::Vector3 v0 = b - a, v1 = c - a, v2 = p - a;
        AZ::VectorFloat d00 = v0.Dot(v0);
        AZ::VectorFloat d01 = v0.Dot(v1);
        AZ::VectorFloat d11 = v1.Dot(v1);
        AZ::VectorFloat d20 = v2.Dot(v0);
        AZ::VectorFloat d21 = v2.Dot(v1);
        AZ::VectorFloat d = d00 * d11 - d01 * d01;
        AZ::VectorFloat invDenom = d ? (AZ::VectorFloat(1.0f) / d) : AZ::VectorFloat(1000000.f);
        v = (d11 * d20 - d01 * d21) * invDenom;
        w = (d00 * d21 - d01 * d20) * invDenom;
        u = 1.0f - v - w;
        return (u >= -fBorder) && (v >= -fBorder) && (w >= -fBorder);
    }

    AZ::Color GetColor(AZ::s32 x, AZ::s32 y, const ColorB* pImg, AZ::s32 nImgSizeW, AZ::s32 nImgSizeH)
    {
        return LYColorBToAZColor(pImg[x + y * nImgSizeW]);
    }

    AZ::Color GetBilinearAt(float iniX, float iniY, const ColorB* pImg,
        AZ::s32 nDimW, AZ::s32 nDimH)
    {
        AZ::s32 nImgSizeW = nDimW;
        AZ::s32 nImgSizeH = nDimH;

        iniX *= nImgSizeW;
        iniY *= nImgSizeH;

        AZ::s32 x = static_cast<AZ::s32>(iniX);
        AZ::s32 y = static_cast<AZ::s32>(iniY);

        float rx = iniX - x;        // fractional part
        float ry = iniY - y;        // fractional part

        AZ::s32 nMaskW = nImgSizeW - 1;
        AZ::s32 nMaskH = nImgSizeH - 1;

        AZ::Color top = GetColor(nMaskW & (x), nMaskH & (y), pImg, nImgSizeW, nImgSizeH) * (1.f - rx)  // left top
            + GetColor(nMaskW & (x + 1), nMaskH & (y), pImg, nImgSizeW, nImgSizeH) * rx;              // right top
        AZ::Color bot = GetColor(nMaskW & (x), nMaskH & (y + 1), pImg, nImgSizeW, nImgSizeH) * (1.f - rx)  // left bottom
            + GetColor(nMaskW & (x + 1), nMaskH & (y + 1), pImg, nImgSizeW, nImgSizeH) * rx;              // right bottom

        return (top * (1.f - ry) + bot * ry) / 255.f;
    }

    AZ::Color ProcessMaterial(
        const SuperTriangle& tr,
        const SvoMaterialInfo& matInfo,
        const AZStd::vector<AZ::Vector3>& verts,
        const AZStd::vector<AZ::Vector2>& uvs,
        const AZStd::vector<ColorB>& colors,
        const AZ::Vector3& vHitPos
    )
    {
        //AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);
        AZ_Assert(verts.size() == 3, "Triangles have 3 verts");
        AZ_Assert(uvs.size() == 3, "Triangles have 3 verts");
        AZ_Assert(colors.size() == 3, "Triangles have 3 verts");

        AZ::Color colVert = LYColorFToAZColor(Col_White);
        AZ::Vector2 vHitTC(0, 0);

        SShaderItem* pShItem = matInfo.m_material ? &matInfo.m_material->GetShaderItem() : NULL;

        AZ::VectorFloat w0 = 0, w1 = 0, w2 = 0;
        if (GetBarycentricTC(verts[0], verts[1], verts[2], w0, w1, w2, vHitPos, 2.f))
        {
            vHitTC = uvs[0] * w0 + uvs[1] * w1 + uvs[2] * w2;

            if (!(pShItem && pShItem->m_pShader) || pShItem->m_pShader->GetFlags2() & EF2_VERTEXCOLORS)
            {

                if (!(pShItem && pShItem->m_pShader) || pShItem->m_pShader->GetShaderType() != eST_Vegetation)
                {
                    AZ::Color color0 = LYColorBToAZColor(colors[0]);
                    AZ::Color color1 = LYColorBToAZColor(colors[1]);
                    AZ::Color color2 = LYColorBToAZColor(colors[2]);
                    AZ::Color colInter = color0 * w0 + color1 * w1 + color2 * w2;

                    if (pShItem)
                    { // swap r and b
                        colVert.SetR(colInter.GetB());
                        colVert.SetG(colInter.GetG());
                        colVert.SetB(colInter.GetR());
                    }
                    else
                    {
                        colVert = colInter;
                    }
                }
            }
        }
        else
        {
            colVert = LYColorFToAZColor(Col_DimGray);
        }

        AZ::Color colTex = LYColorFToAZColor(Col_Gray);

        ColorB* m_textureColor = 0;
        AZ::s32 m_textureWidth = 0, m_textureHeight = 0;

        if (matInfo.m_material)
        { // objects
            m_textureColor = matInfo.m_textureColor;
            m_textureWidth = matInfo.m_textureWidth;
            m_textureHeight = matInfo.m_textureHeight;
        }

        if (m_textureColor)
        {
            if (matInfo.m_material)
            {
                colTex = GetBilinearAt(
                    vHitTC.GetX(),
                    vHitTC.GetY(),
                    m_textureColor, m_textureWidth, m_textureHeight);
                colTex = colTex.GammaToLinear();
            }
            else
            { // terrain tex-gen
                AZ::VectorFloat worldSize = static_cast<float>(gEnv->p3DEngine->GetTerrainSize());
                colTex = GetBilinearAt(
                    vHitPos.GetY() / worldSize,
                    vHitPos.GetX() / worldSize,
                    m_textureColor, m_textureWidth, m_textureHeight);

                colTex = colTex.GammaToLinear();
                colTex.SetA(1);
            }
        }

        AZ::Color colMat = LYColorFToAZColor((pShItem && pShItem->m_pShaderResources) ? pShItem->m_pShaderResources->GetColorValue(EFTT_DIFFUSE) : Col_White);

        AZ::Color colRes = colTex * colMat * colVert;

        return colRes;
    }


    //Note: This should ultimately be vectorized properly. 
    bool SphereTriangleIntersection(const AZStd::vector<AZ::Vector3>& tri, const AZ::Vector3& center, AZ::VectorFloat radiusSq)
    {
        AZ::Vector3 v01 = tri[1] - tri[0];
        AZ::Vector3 v02 = tri[2] - tri[0];

        AZ::VectorFloat zero = AZ::VectorFloat(0.0f);
        AZ::VectorFloat t;
        AZ::Vector3 p;

        p = center - tri[0];
        AZ::VectorFloat d10 = v01.Dot(p);
        AZ::VectorFloat d20 = v02.Dot(p);

        //Nearest point is 0 index
        if (d10 <= zero && d20 <= zero)
        {
            return p.Dot(p) <= radiusSq;
        }

        p = center - tri[1];
        AZ::VectorFloat d11 = v01.Dot(p);
        AZ::VectorFloat d21 = v02.Dot(p);

        //Nearest point is 1 index.
        if (d11 >= zero && d21 <= d11)
        {
            return p.Dot(p) <= radiusSq;
        }

        //Nearest point is on 0 to 1 edge
        if (d10*d21 - d11 * d20 <= zero && d10 >= zero && d11 <= zero)
        {
            t = d10 / (d10 - d11);
            p = center - (tri[0] + t * v01);
            return p.Dot(p) <= radiusSq;
        }

        //Nearest point is 2 index;
        p = center - tri[2];
        AZ::VectorFloat d12 = v01.Dot(p);
        AZ::VectorFloat d22 = v02.Dot(p);
        if (d22 >= zero && d12 <= d22)
        {
            return p.Dot(p) <= radiusSq;
        }

        //Nearest point is along 0 to 2 edge.
        if (d12*d20 - d10 * d22 <= zero && d20 >= zero && d22 <= zero)
        {
            t = d20 / (d20 - d22);
            p = center - (tri[0] + t * v02);
            return p.Dot(p) <= radiusSq;
        }

        //Nearest point is along 1 to 2 edge
        if (d11*d22 - d12 * d21 <= zero && (d21 - d11) >= zero && (d12 - d22) >= zero)
        {
            t = (d21 - d11) / ((d21 - d11) + (d12 - d22));
            p = center - (tri[1] + t * (tri[2] - tri[1]));
            return p.Dot(p) <= radiusSq;
        }

        //If we made it this far we are inside the triangle
        return true;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Brick
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    Brick::Brick()
    {
        m_brickAabb.SetNull();
        m_opacities = m_colors = m_normals = nullptr;
        m_counts = nullptr;
        m_brickOrigin = AZ::Vector3(0, 0, 0);
        m_collectedLegacyObjects = false;
        m_terrainOnly = true;
        m_numLegacyObjects = 0;
        m_lastUploaded = 0;
        m_lastUpdated = 0;
    }

    Brick::~Brick()
    {
        FreeBrickData();
        ClearTriangleData();
    }

    void Brick::FreeBrickData()
    {
        if (m_colors)
        {
            delete m_colors;
            m_colors = nullptr;
        }

        if (m_opacities)
        {
            delete m_opacities;
            m_opacities = nullptr;
        }

        if (m_normals)
        {
            delete m_normals;
            m_normals = nullptr;
        }

        if (m_counts)
        {
            delete m_counts;
            m_counts = nullptr;
        }
    }

    void Brick::FreeTriangleData()
    {
        Clear();

        // Force reallocation after clearing triangle data to free memory
        AZStd::vector<SuperTriangle>().swap(m_triangles);
        AZStd::vector<AZ::Vector3>().swap(m_faceNormals);
        AZStd::vector<SRayHitVertex>().swap(m_vertices);
        AZStd::vector<SvoMaterialInfo>().swap(m_materials);
    }

    void Brick::ClearTriangleData()
    {
        Clear();
    }

    bool Brick::ProcessTriangles(
        DataBrick<GISubVoxels>& data,
        DataBrick<AZ::Vector3>& centers
    )
    {
        //For each triangle insert the triangle into the brick data.  
        AZStd::vector<AZ::Vector3> triVerts(3);
        AZStd::vector<AZ::Vector2> triUvs(3);
        AZStd::vector<ColorB> triColors(3);

        AZ::VectorFloat subBrickRadius = (GetBrickSize() / brickDimension) * .5f;
        AZ::VectorFloat subBrickRadiusSq = subBrickRadius * subBrickRadius;

        AZ::VectorFloat subSubBrickRadius = subBrickRadius * .5f;
        AZ::VectorFloat subSubBrickRadiusSq = subSubBrickRadius * subSubBrickRadius;

        bool dataGenerated = false;

        const AZ::VectorFloat offsets[4] = { -3.f, -1.f, 1.f, 3.f };

        for (const auto& triangle : m_triangles)
        {
            //Note: All this triangle data needs to be restructured to be better vectorized.
            // AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::Renderer, "Brick::ProcessMeshes::PerTriWork");
            //Extract needed triangle data and compute containing plane. 
            triVerts = { LYVec3ToAZVec3(m_vertices[triangle.arrVertId[0]].v),
                LYVec3ToAZVec3(m_vertices[triangle.arrVertId[1]].v),
                LYVec3ToAZVec3(m_vertices[triangle.arrVertId[2]].v)
            };
            triUvs = { LYVec2ToAZVec2(m_vertices[triangle.arrVertId[0]].t),
                LYVec2ToAZVec2(m_vertices[triangle.arrVertId[1]].t),
                LYVec2ToAZVec2(m_vertices[triangle.arrVertId[2]].t)
            };

            triColors = { m_vertices[triangle.arrVertId[0]].c, m_vertices[triangle.arrVertId[1]].c, m_vertices[triangle.arrVertId[2]].c };

            AZ::Plane plane = AZ::Plane::CreateFromTriangle(triVerts[0], triVerts[1], triVerts[2]);

            AZ::VectorFloat triEmitance = 0.0f;
            SvoMaterialInfo& matInfo = m_materials.at(triangle.nMatID);
            if (matInfo.m_material)
            {
                triEmitance = matInfo.m_material->GetShaderItem(0).m_pShaderResources->GetFinalEmittance().Luminance();
            }


            //Each brick consists of NxNxN samples.  For each sample compute the triangle contribution.
            for (AZ::u32 offset = 0; offset < brickDimension * brickDimension * brickDimension; ++offset)
            {
                //Check if the triangle is within the circumscribed sphere of the subbrick.  
                //This could have the disadvantage of missing triangles that cut through the corners
                //but if that becomes an issue there are several remedies. 
                //1) decrease the voxel min size to increase sampling density.
                //2) test against the circumscribing sphere and then do a box triangle test

                //If the bounding sphere doesn't touch the plane skip. 
                if (fabs(plane.GetPointDist(centers[offset])) > subBrickRadius)
                {
                    continue;
                }

                //Check if the sphere touches the triangle. 
                if (!SphereTriangleIntersection(triVerts, centers[offset], subBrickRadiusSq))
                {
                    continue;
                }
                for (int x = 0; x < 4; ++x)
                {
                    for (int y = 0; y < 4; ++y)
                    {
                        for (int z = 0; z < 4; ++z)
                        {
                            //AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::Renderer, "Brick::ProcessMeshes::ComputeVoxelData");
                            AZ::Vector3 samplePoint = centers[offset] + AZ::Vector3(offsets[x], offsets[y], offsets[z])*subSubBrickRadius;
                            //Check if the sphere touches the triangle. 
                            if (!SphereTriangleIntersection(triVerts, samplePoint, subSubBrickRadiusSq))
                            {
                                continue;
                            }
                            //Note: There are some redundant pointer indirections and function calls we could 
                            //elide here if we need more perf later. 
                            AZ::Color colTraced = ProcessMaterial(triangle, matInfo, triVerts, triUvs, triColors, samplePoint);
                            if (colTraced.GetA() > 0.f)
                            {
                                //opacity = AZStd::max(opacity, AZStd::min(AZ::VectorFloat(triangle.nOpacity), SATURATEB(colTraced.GetA()*255.f)));
                                data[offset].m_opacities[x][y][z] += AZ::VectorFloat(triangle.nOpacity);
                                data[offset].m_normals[x][y][z] += (LYVec3ToAZVec3(triangle.vFaceNorm));
                                data[offset].m_colors[x][y][z] += AZ::Color(colTraced.GetR(), colTraced.GetG(), colTraced.GetB(), AZ::VectorFloat(1.f));
                                data[offset].m_emittances[x][y][z] += triEmitance;
                                dataGenerated = true;
                            }
                        }
                    }
                }
            }
        }
        return dataGenerated;
    }

    void CookSubVoxelData(GISubVoxels& data, AZ::Vector3& opacities, AZ::Color& color, AZ::Vector3& normal, AZ::VectorFloat& emittance)
    {
        AZ::VectorFloat triPlanarOpacity[3][4][4];
        memset(&triPlanarOpacity, 0, sizeof(triPlanarOpacity));
        for (int x = 0; x < 4; ++x)
        {
            for (int y = 0; y < 4; ++y)
            {
                for (int z = 0; z < 4; ++z)
                {
                    AZ::VectorFloat& opacity = data.m_opacities[x][y][z];
                    normal += data.m_normals[x][y][z];
                    color += data.m_colors[x][y][z];
                    emittance += data.m_emittances[x][y][z];

                    triPlanarOpacity[0][y][z] = max(triPlanarOpacity[0][y][z], opacity);
                    triPlanarOpacity[1][x][z] = max(triPlanarOpacity[1][x][z], opacity);
                    triPlanarOpacity[2][x][y] = max(triPlanarOpacity[2][x][y], opacity);
                }
            }
        }

        AZ::Color opacitySummation(0.f, 0.f, 0.f, 0.f);

        for (int k = 0; k < 4; k++)
        {
            for (int m = 0; m < 4; m++)
            {
                opacitySummation.SetR(opacitySummation.GetR() + triPlanarOpacity[0][k][m]);
                opacitySummation.SetG(opacitySummation.GetG() + triPlanarOpacity[1][k][m]);
                opacitySummation.SetB(opacitySummation.GetB() + triPlanarOpacity[2][k][m]);
            }
        }
        const AZ::VectorFloat scale = AZ::VectorFloat(1.f / (4.f*4.f));
        opacities.SetX(opacitySummation.GetR() * scale);
        opacities.SetY(opacitySummation.GetG() * scale);
        opacities.SetZ(opacitySummation.GetB() * scale);
    }

    void Brick::UpdateBrickData(
        DataBrick<GISubVoxels>& data,
        bool increment
    )
    {
        const AZ::VectorFloat sign = increment ? 1.0f : -1.0f;

        //R/W lock to avoid data conflicts on updating voxel data. 
        AZStd::unique_lock<AZStd::shared_mutex> writeLock(m_brickDataMutex);
        if (!m_colors)
        {
            m_colors = aznew DataBrick<ColorB>();
        }

        if (!m_normals)
        {
            m_normals = aznew DataBrick<ColorB>();
        }

        if (!m_opacities)
        {
            m_opacities = aznew DataBrick<ColorB>();
        }

        if (!m_counts)
        {
            m_counts = aznew DataBrick<AZ::s32>();
        }

        //For each sample finalize data.
        for (AZ::u32 offset = 0; offset < brickDimension * brickDimension * brickDimension; ++offset)
        {
            //Brick data is stored in linear arrays. Compute the correct offest.
            ColorB& outColor = (*m_colors)[offset];
            ColorB& outNormal = (*m_normals)[offset];
            ColorB& outOpacity = (*m_opacities)[offset];
            AZ::VectorFloat outCount = AZ::VectorFloat(static_cast<float>((*m_counts)[offset]));

            AZ::Color color = AZ::Color::CreateZero();
            AZ::Vector3 normal = AZ::Vector3::CreateZero();
            AZ::Vector3 opacities = AZ::Vector3::CreateZero();
            AZ::VectorFloat emittance = AZ::VectorFloat(0.f);
            CookSubVoxelData(data[offset], opacities, color, normal, emittance);

            if (color.GetA() > AZ::VectorFloat(0.0f))
            {
                AZ::VectorFloat divisor = outCount + sign * color.GetA();
                if (static_cast<AZ::s32>(divisor) > 0)
                {
                    AZ::VectorFloat invDivisor = AZ::VectorFloat(1.0f) / divisor;
                    const AZ::VectorFloat scale = AZ::VectorFloat(255.f);
                    //Linear color average isn't really correct for the human visual system but 
                    //this is what was done in the legacy system.
                    outColor.r = static_cast<AZ::u8>(SATURATEB(float((AZ::VectorFloat(outColor.r) * outCount + sign * color.GetR()) * invDivisor * scale)));
                    outColor.g = static_cast<AZ::u8>(SATURATEB(float((AZ::VectorFloat(outColor.g) * outCount + sign * color.GetG()) * invDivisor * scale)));
                    outColor.b = static_cast<AZ::u8>(SATURATEB(float((AZ::VectorFloat(outColor.b) * outCount + sign * color.GetB()) * invDivisor * scale)));
                    outColor.a = static_cast<AZ::u8>(SATURATEB(float((AZ::VectorFloat(outColor.a) * outCount + sign * emittance) * invDivisor)));

                    outOpacity.r = static_cast<AZ::u8>(SATURATEB(float((outCount * AZ::VectorFloat(outOpacity.r) + opacities.GetZ()) * invDivisor * scale)));
                    outOpacity.g = static_cast<AZ::u8>(SATURATEB(float((outCount * AZ::VectorFloat(outOpacity.g) + opacities.GetY()) * invDivisor * scale)));
                    outOpacity.b = static_cast<AZ::u8>(SATURATEB(float((outCount * AZ::VectorFloat(outOpacity.b) + opacities.GetX()) * invDivisor * scale)));

                    //Sure.  Legacy concept. 
                    bool bTerrainTrisDetected;
                    ITerrain* pTerrain = gEnv->p3DEngine->GetITerrain();
                    AZ::Vector3 vH = m_brickOrigin;
                    if (vH.GetZ() > (pTerrain->GetZ(static_cast<int32_t>(vH.GetX()), static_cast<int32_t>(vH.GetY())) + 1.5f))
                    {
                        bTerrainTrisDetected = false;
                    }
                    else
                    {
                        bTerrainTrisDetected = true;
                    }

                    outOpacity.a = bTerrainTrisDetected ? 0 : 1; // reserved for opacity of dynamic voxels or [0 = triangle is missing in RSM]
                    //The normals are not 'normalized' as the length is being used in the shaders
                    //as a quality metric. 
                    //Note: This is not really being done correctly as the length trick still requires better
                    //than a linear average on the normals.

                    //Legacy unpacking to update
                    AZ::VectorFloat temp[3];
                    const AZ::VectorFloat bias = AZ::VectorFloat(127.5f);
                    if (outCount > 0)
                    {
                        for (int c = 0; c < 3; c++)
                        {
                            temp[2 - c] = (AZ::VectorFloat(outNormal[c]) - bias) / bias;
                        }
                    }
                    temp[0] = (temp[0] * outCount + sign * normal.GetX()) * invDivisor;
                    temp[1] = (temp[1] * outCount + sign * normal.GetY()) * invDivisor;
                    temp[2] = (temp[2] * outCount + sign * normal.GetZ()) * invDivisor;
                    outNormal.a = (outOpacity.r > 0.f) || (outOpacity.g > 0.f) || (outOpacity.b > 0.f) ? 255 : 0;

                    //Legacy packing conversion.
                    for (int c = 0; c < 3; c++)
                    {
                        outNormal[c] = (static_cast<AZ::u8>(temp[2 - c] * bias + bias));
                    }
                }
                else
                {
                    //Linear color average isn't really correct for the human visual system but 
                    //this is what was done in the legacy system.
                    outColor.r = 0;
                    outColor.g = 0;
                    outColor.b = 0;
                    outColor.a = 0;
                    outOpacity.r = 0;
                    outOpacity.g = 0;
                    outOpacity.b = 0;

                    //Sure.  Legacy concept. 
                    outOpacity.a = 0;

                    //The normals are not 'normalized' as the length is being used in the shaders
                    //as a quality metric. 
                    //Note: This is not really being done correctly as the length trick still requires better
                    //than a linear average on the normals.

                    outNormal.r = 0;
                    outNormal.g = 0;
                    outNormal.b = 0;
                    outNormal.a = 0;

                    //Legacy packing conversion.
                    for (int c = 0; c < 3; c++)
                    {
                        outNormal[c] = (static_cast<AZ::u8>(outNormal[2 - c] * 127.5f + 127.5f));
                    }
                }

                (*m_counts)[offset] = static_cast<AZ::u8>(divisor);
                AZ_Assert((*m_counts)[offset] >= 0, "Over removed data from GI.");
            }
        }
    }

    bool Brick::HasBrickData() const
    {
        AZStd::unique_lock<AZStd::shared_mutex> readLock(m_brickDataMutex);
        return m_colors && m_normals && m_opacities && m_counts;
    }

    void Brick::ProcessMeshes(
        const EntityMeshDataMap& insertions,
        const EntityMeshDataMap& removals,
        DataBrick<GISubVoxels>& scratch
    )
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);

        AZStd::vector<ObjectInfo> objects;

        //Scratch buffers, to avoid locking during processing.
        scratch.Reset();
        DataBrick<AZ::Vector3> centers;

        AZ::Aabb voxBox;
        AZ::Vector3 brickAabbData[2];
        brickAabbData[0] = AZ::Vector3(m_brickAabb.GetMin() + m_brickOrigin);
        brickAabbData[1] = AZ::Vector3(m_brickAabb.GetMax() + m_brickOrigin);

        //Generate subbrick centers.
        for (AZ::u32 X = 0; X < brickDimension; X++)
        {
            for (AZ::u32 Y = 0; Y < brickDimension; Y++)
            {
                for (AZ::u32 Z = 0; Z < brickDimension; Z++)
                {
                    AZ::Vector3 vMin = brickAabbData[0] + (brickAabbData[1] - brickAabbData[0]) * AZ::Vector3((float)X / brickDimension, (float)Y / brickDimension, (float)Z / brickDimension);
                    AZ::Vector3 vMax = brickAabbData[0] + (brickAabbData[1] - brickAabbData[0]) * AZ::Vector3((float)(X + 1) / brickDimension, (float)(Y + 1) / brickDimension, (float)(Z + 1) / brickDimension);

                    voxBox.SetMin(vMin);
                    voxBox.SetMax(vMax);

                    AZ::u32 brickOffset = Z * brickDimension * brickDimension + Y * brickDimension + X;
                    centers[brickOffset] = voxBox.GetCenter();
                }
            }
        }

        {//Process Legacy Elements

            //Legacy elements should be processed only once as they are considered truly static.
            if (!m_collectedLegacyObjects)
            {
                m_collectedLegacyObjects = true;

                AZ::Aabb worldBrickAabb;
                worldBrickAabb.SetMin(m_brickAabb.GetMin() + m_brickOrigin);
                worldBrickAabb.SetMax(m_brickAabb.GetMax() + m_brickOrigin);
                Brick::CollectLegacyObjects(worldBrickAabb, &objects);

                m_numLegacyObjects = objects.size();
                if (!objects.empty())
                {
                    m_terrainOnly = false;
                }

                ExtractTriangles(objects);
                ExtractTerrainTriangles();
                ExtractVisAreaTriangles();

                bool dataGenerated = ProcessTriangles(scratch, centers);

                if (dataGenerated)
                {
                    UpdateBrickData(scratch, true); // Include data generated from legacy meshes
                }

                ClearTriangleData();
            }
        }

        {//Process Removals
            objects.clear();
            for (auto& mesh : removals)
            {
                int numEntitiesErased = m_entityIDs.erase(mesh.first);
                if (numEntitiesErased == 0)
                {
                    // If the entity was not erased then it was not processed previously.
                    continue;
                }

                ObjectInfo objInfo;
                AZStd::shared_ptr<MeshData> meshData = mesh.second;
                objInfo.fObjScale = meshData->m_transform.RetrieveScale().GetX();
                objInfo.matObj.SetColumn(0, AZVec3ToLYVec3(meshData->m_transform.GetColumn(0)));
                objInfo.matObj.SetColumn(1, AZVec3ToLYVec3(meshData->m_transform.GetColumn(1)));
                objInfo.matObj.SetColumn(2, AZVec3ToLYVec3(meshData->m_transform.GetColumn(2)));
                objInfo.matObj.SetColumn(3, AZVec3ToLYVec3(meshData->m_transform.GetPosition()));
                objInfo.matObjInv = objInfo.matObj.GetInverted();
                objInfo.m_material = meshData->m_material;
                objInfo.pStatObj = meshData->m_meshAsset.Get()->m_statObj;
                objects.emplace_back(objInfo);
            }
            if (m_collectedLegacyObjects && m_numLegacyObjects == 0 && m_entityIDs.empty())
            {
                m_terrainOnly = true; // No more objects in the brick
            }
            ExtractTriangles(objects);

            if (!m_triangles.empty())
            {
                scratch.Reset();
            }

            bool dataGenerated = ProcessTriangles(scratch, centers);

            if (dataGenerated)
            {
                UpdateBrickData(scratch, false); // Substract data generated from meshes to be removed
            }

            ClearTriangleData();
        }

        {//Process Insertions
            objects.clear();
            for (auto& mesh : insertions)
            {
                auto entityInsertedPair = m_entityIDs.insert(mesh.first);
                if (!entityInsertedPair.second)
                {
                    // If insertion didn't take place then the entity was already processed.
                    continue;
                }

                ObjectInfo objInfo;
                AZStd::shared_ptr<MeshData> meshData = mesh.second;
                objInfo.fObjScale = meshData->m_transform.RetrieveScale().GetX();
                objInfo.matObj.SetColumn(0, AZVec3ToLYVec3(meshData->m_transform.GetColumn(0)));
                objInfo.matObj.SetColumn(1, AZVec3ToLYVec3(meshData->m_transform.GetColumn(1)));
                objInfo.matObj.SetColumn(2, AZVec3ToLYVec3(meshData->m_transform.GetColumn(2)));
                objInfo.matObj.SetColumn(3, AZVec3ToLYVec3(meshData->m_transform.GetPosition()));
                objInfo.matObjInv = objInfo.matObj.GetInverted();
                objInfo.m_material = meshData->m_material;
                objInfo.pStatObj = meshData->m_meshAsset.Get()->m_statObj;
                objects.emplace_back(objInfo);
            }
            if (!objects.empty())
            {
                m_terrainOnly = false;
            }
            ExtractTriangles(objects);

            if (!m_triangles.empty())
            {
                scratch.Reset();
            }

            bool dataGenerated = ProcessTriangles(scratch, centers);

            if (dataGenerated)
            {
                UpdateBrickData(scratch, true); // Include data generated from new inserted meshes
            }

            // Free triangle data instead of just clearing it to free
            // memory until the next time it has to process meshes again.
            FreeTriangleData();
        }
    }

    bool Brick::CollectLegacyObjects(const AZ::Aabb& worldBrickAabb, AZStd::vector<ObjectInfo>* parrObjects)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);
        bool bSuccess = true;

        {
            for (AZ::s32 nObjType = 0; nObjType < eERType_TypesNum; nObjType++)
            {
                if ((nObjType == eERType_Brush) ||
                    (nObjType == eERType_Vegetation))
                {
                    PodArray<IRenderNode*> arrRenderNodes;
                    AABB legacyAABB = AZAabbToLyAABB(worldBrickAabb);
                    gEnv->p3DEngine->GetObjectsByTypeInBox((EERType)nObjType, legacyAABB, &arrRenderNodes);
                    if (gEnv->p3DEngine->GetIVisAreaManager())
                    {
                        gEnv->p3DEngine->GetIVisAreaManager()->GetObjectsByType(arrRenderNodes, (EERType)nObjType, &legacyAABB);
                    }

                    if (!arrRenderNodes.Count())
                    {
                        continue;
                    }

                    for (AZ::s32 d = 0; d < arrRenderNodes.Count(); d++)
                    {
                        IRenderNode* pNode = arrRenderNodes[d];

                        if (pNode->GetRndFlags() & (ERF_COLLISION_PROXY | ERF_RAYCAST_PROXY))
                        {
                            continue;
                        }

                        if (_smart_ptr<IMaterial> material = pNode->GetMaterial())
                        {
                            Matrix34A mat;
                            pNode->GetEntityStatObj(0, 0, &mat);
                            ObjectInfo info;
                            info.matObjInv = mat.GetInverted();
                            info.matObj = mat;
                            info.pStatObj = pNode->GetEntityStatObj(0, 0, NULL);
                            if (!info.pStatObj)
                            {
                                continue;
                            }

                            AZ::s32 nLod = 1;//This should be either the current lod of the object or a per object control

                            bool bUnloadable = info.pStatObj->IsUnloadable();

                            info.pStatObj = info.pStatObj->GetLodObject(nLod, true);

                            if (pNode->GetRenderNodeType() == eERType_Brush)
                            {
                                info.fObjScale = ((IBrush*)pNode)->GetScale();
                            }
                            else if (pNode->GetRenderNodeType() == eERType_Vegetation)
                            {
                                info.fObjScale = ((IVegetation*)pNode)->GetScale();
                            }
                            else
                            {
                                assert(!"Undefined object type");
                            }

                            info.m_material = material;

                            if (info.pStatObj->GetFlags() & STATIC_OBJECT_HIDDEN)
                            {
                                continue;
                            }


                            info.bVegetation = (nObjType == eERType_Vegetation);

                            if (parrObjects)
                            {
                                parrObjects->push_back(info);
                            }
                            else if (info.pStatObj->m_eStreamingStatus != ecss_Ready && bUnloadable)
                            {
                                info.pStatObj->UpdateStreamableComponents(0.5f, info.matObj, false, nLod);
                                bSuccess = false;
                            }
                        }
                    }
                }
            }
        }
        return bSuccess;
    }

    void Brick::ExtractTriangles(AZStd::vector<ObjectInfo>& objects)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);
        AZ::Aabb worldBrickAabb;
        worldBrickAabb.SetMin(m_brickAabb.GetMin() + m_brickOrigin);
        worldBrickAabb.SetMax(m_brickAabb.GetMax() + m_brickOrigin);

        // get tris from real level geometry
        SuperMesh superMesh;
        PodArray<SRayHitTriangle> arrTris;

        for (AZ::s32 d = 0; d < objects.size(); d++)
        {
            SRayHitInfo nodeHitInfo;
            nodeHitInfo.bInFirstHit = true;
            nodeHitInfo.bUseCache = false;
            nodeHitInfo.bGetVertColorAndTC = true;

            ObjectInfo& info = objects[d];

            nodeHitInfo.nHitTriID = HIT_UNKNOWN;
            nodeHitInfo.nHitMatID = HIT_UNKNOWN;
            nodeHitInfo.inRay.origin = info.matObjInv.TransformPoint(AZVec3ToLYVec3(m_brickOrigin));

            //By making the direction 0, cry has secretly turned a hit test into a box test. Huzzah!
            nodeHitInfo.inRay.direction = Vec3(0, 0, 0);
            nodeHitInfo.inReferencePoint = nodeHitInfo.inRay.origin + nodeHitInfo.inRay.direction * 0.5f;
            nodeHitInfo.fMaxHitDistance = GetBrickSize() / 2.f / info.fObjScale * sqrt(3.f);

            arrTris.Clear();
            nodeHitInfo.pHitTris = &arrTris;

            nodeHitInfo.fMinHitOpacity = 0.0f;
            info.pStatObj->RayIntersection(nodeHitInfo, info.m_material);

            if (arrTris.Count())
            {
                superMesh.Clear();

                float fEpsilon = (VEC_EPSILON / 5);

                for (AZ::s32 t = 0; t < arrTris.Count(); t++)
                {
                    // Workaround for over occlusion from vegetation; TODO: make thin geoemtry produce less occlusion
                    if (info.bVegetation && arrTris[t].pMat)
                    {
                        float fMidZ = 0;
                        for (AZ::s32 v = 0; v < 3; v++)
                        {
                            fMidZ += arrTris[t].v[v].z;
                        }
                        fMidZ *= 0.333f;

                        SShaderItem& rSI = arrTris[t].pMat->GetShaderItem();

                        if (rSI.m_pShaderResources && rSI.m_pShader)
                        {
                            bool bVegetationLeaves = (rSI.m_pShaderResources->GetAlphaRef() > 0.05f && rSI.m_pShader->GetShaderType() == eST_Vegetation);
                            //The .2 multiplier is a transfer of a default legacy cvar value.  It is here temporarily until we figure out why it was needed.  
                            //See the todo above. 
                            if (bVegetationLeaves)
                            {
                                arrTris[t].nOpacity = min(arrTris[t].nOpacity, AZ::u8(SATURATEB(.2f * 255.f)));
                            }
                            else
                            {
                                arrTris[t].nOpacity = min(arrTris[t].nOpacity, AZ::u8(SATURATEB(LERP(255.f, .2f * 255.f, SATURATE(fMidZ * .5f)))));
                            }
                        }
                    }

                    for (AZ::s32 v = 0; v < 3; v++)
                    {
                        arrTris[t].v[v] = info.matObj.TransformPoint(arrTris[t].v[v]);
                    }

                    arrTris[t].nTriArea = SATURATEB(AZ::s32(SVO_CPU_VOXELIZATION_AREA_SCALE * 0.5f * (arrTris[t].v[1] - arrTris[t].v[0]).Cross(arrTris[t].v[2] - arrTris[t].v[0]).GetLength()));
                    Plane pl;
                    pl.SetPlane(arrTris[t].v[0], arrTris[t].v[1], arrTris[t].v[2]);
                    arrTris[t].n = pl.n;
                    if (!arrTris[t].v[0].IsEquivalent(arrTris[t].v[1], fEpsilon) && !arrTris[t].v[1].IsEquivalent(arrTris[t].v[2], fEpsilon) && !arrTris[t].v[2].IsEquivalent(arrTris[t].v[0], fEpsilon))
                    {
                        if ((arrTris[t].nTriArea) && Overlap::AABB_Triangle(AZAabbToLyAABB(worldBrickAabb), arrTris[t].v[0], arrTris[t].v[1], arrTris[t].v[2]))
                        {
                            superMesh.AddSuperTriangle(arrTris[t]);
                        }
                    }
                }

                AddSuperMesh(superMesh, SVO_CPU_VOXELIZATION_OFFSET_MESH);
            }
        }
    }

    //Since X,Y are sampled from within the bounding box, check if the Z is in the box range.
    bool TerrainTriBoundsCheck(const SRayHitTriangle& ht, const AZ::Aabb& aabb)
    {
        AZ::Vector3 z = AZ::Vector3(ht.v[0].z, ht.v[1].z, ht.v[2].z);
        AZ::Vector3 max = AZ::Vector3(aabb.GetMax().GetZ());
        AZ::Vector3 min = AZ::Vector3(aabb.GetMin().GetZ());

        return z.IsLessEqualThan(max) && z.IsGreaterEqualThan(min);
    }

    void Brick::ExtractTerrainTriangles()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);
        SuperMesh superMesh;
        PodArray<SRayHitTriangle> arrTris;
        AZ::Aabb worldBrickAabb;
        worldBrickAabb.SetMin(m_brickAabb.GetMin() + m_brickOrigin);
        worldBrickAabb.SetMax(m_brickAabb.GetMax() + m_brickOrigin);

        // add terrain
        if (gEnv->p3DEngine->GetShowTerrainSurface())
        {
            ITerrain* pTerrain = gEnv->p3DEngine->GetITerrain();
            AZ::s32 nWorldSize = gEnv->p3DEngine->GetTerrainSize();
            AZ::s32 S = gEnv->p3DEngine->GetHeightMapUnitSize();

            AZ::s32 nHalfStep = S / 2;

            SRayHitTriangle ht;
            memset(&ht, 0, sizeof(ht));
            ht.c[0] = ht.c[1] = ht.c[2] = Col_White;
            ht.nOpacity = 255;
            ht.nHitObjType = HIT_OBJ_TYPE_TERRAIN;
            Plane pl;

            AZ::s32 I = 0, X = 0, Y = 0;

            superMesh.Clear();

            for (AZ::s32 x = (AZ::s32)worldBrickAabb.GetMin().GetX(); x < (AZ::s32)worldBrickAabb.GetMax().GetX(); x += S)
            {
                for (AZ::s32 y = (AZ::s32)worldBrickAabb.GetMin().GetY(); y < (AZ::s32)worldBrickAabb.GetMax().GetY(); y += S)
                {
                    if (!pTerrain->IsHole(x + nHalfStep, y + nHalfStep))
                    {
                        // prevent surface interpolation over long edge
                        bool bFlipTris = false;
                        AZ::s32 nType10 = pTerrain->GetSurfaceWeight(x + S, y).PrimaryId();
                        AZ::s32 nType01 = pTerrain->GetSurfaceWeight(x, y + S).PrimaryId();
                        if (nType10 != nType01)
                        {
                            AZ::s32 nType00 = pTerrain->GetSurfaceWeight(x, y).PrimaryId();
                            AZ::s32 nType11 = pTerrain->GetSurfaceWeight(x + S, y + S).PrimaryId();
                            if ((nType10 == nType00 && nType10 == nType11) || (nType01 == nType00 && nType01 == nType11))
                            {
                                bFlipTris = true;
                            }
                        }

                        if (bFlipTris)
                        {
                            I = 0;
                            X = x + S, Y = y + 0;
                            ht.v[I].Set((float)X, (float)Y, pTerrain->GetZ(X, Y));
                            I++;
                            X = x + S, Y = y + S;
                            ht.v[I].Set((float)X, (float)Y, pTerrain->GetZ(X, Y));
                            I++;
                            X = x + 0, Y = y + 0;
                            ht.v[I].Set((float)X, (float)Y, pTerrain->GetZ(X, Y));
                            I++;

                            if (TerrainTriBoundsCheck(ht, worldBrickAabb))
                            {
                                ht.nTriArea = SATURATEB(AZ::s32(SVO_CPU_VOXELIZATION_AREA_SCALE * 0.5f * (ht.v[1] - ht.v[0]).Cross(ht.v[2] - ht.v[0]).GetLength()));
                                pl.SetPlane(ht.v[0], ht.v[1], ht.v[2]);
                                ht.n = pl.n;
                                superMesh.AddSuperTriangle(ht);
                            }

                            I = 0;
                            X = x + 0, Y = y + 0;
                            ht.v[I].Set((float)X, (float)Y, pTerrain->GetZ(X, Y));
                            I++;
                            X = x + S, Y = y + S;
                            ht.v[I].Set((float)X, (float)Y, pTerrain->GetZ(X, Y));
                            I++;
                            X = x + 0, Y = y + S;
                            ht.v[I].Set((float)X, (float)Y, pTerrain->GetZ(X, Y));
                            I++;

                            if (TerrainTriBoundsCheck(ht, worldBrickAabb))
                            {
                                ht.nTriArea = SATURATEB(AZ::s32(SVO_CPU_VOXELIZATION_AREA_SCALE * 0.5f * (ht.v[1] - ht.v[0]).Cross(ht.v[2] - ht.v[0]).GetLength()));
                                pl.SetPlane(ht.v[0], ht.v[1], ht.v[2]);
                                ht.n = pl.n;
                                superMesh.AddSuperTriangle(ht);
                            }
                        }
                        else
                        {
                            I = 0;
                            X = x + 0, Y = y + 0;
                            ht.v[I].Set((float)X, (float)Y, pTerrain->GetZ(X, Y));
                            I++;
                            X = x + S, Y = y + 0;
                            ht.v[I].Set((float)X, (float)Y, pTerrain->GetZ(X, Y));
                            I++;
                            X = x + 0, Y = y + S;
                            ht.v[I].Set((float)X, (float)Y, pTerrain->GetZ(X, Y));
                            I++;

                            if (TerrainTriBoundsCheck(ht, worldBrickAabb))
                            {
                                ht.nTriArea = SATURATEB(AZ::s32(SVO_CPU_VOXELIZATION_AREA_SCALE * 0.5f * (ht.v[1] - ht.v[0]).Cross(ht.v[2] - ht.v[0]).GetLength()));
                                pl.SetPlane(ht.v[0], ht.v[1], ht.v[2]);
                                ht.n = pl.n;
                                superMesh.AddSuperTriangle(ht);
                            }

                            I = 0;
                            X = x + S, Y = y + 0;
                            ht.v[I].Set((float)X, (float)Y, pTerrain->GetZ(X, Y));
                            I++;
                            X = x + S, Y = y + S;
                            ht.v[I].Set((float)X, (float)Y, pTerrain->GetZ(X, Y));
                            I++;
                            X = x + 0, Y = y + S;
                            ht.v[I].Set((float)X, (float)Y, pTerrain->GetZ(X, Y));
                            I++;

                            if (TerrainTriBoundsCheck(ht, worldBrickAabb))
                            {
                                ht.nTriArea = SATURATEB(AZ::s32(SVO_CPU_VOXELIZATION_AREA_SCALE * 0.5f * (ht.v[1] - ht.v[0]).Cross(ht.v[2] - ht.v[0]).GetLength()));
                                pl.SetPlane(ht.v[0], ht.v[1], ht.v[2]);
                                ht.n = pl.n;
                                superMesh.AddSuperTriangle(ht);
                            }
                        }
                    }
                }
            }

            AddSuperMesh(superMesh, SVO_CPU_VOXELIZATION_OFFSET_TERRAIN);
        }
    }

    void Brick::ExtractVisAreaTriangles()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);
        SuperMesh superMesh;
        PodArray<SRayHitTriangle> arrTris;
        AZ::Aabb worldBrickAabb;
        worldBrickAabb.SetMin(m_brickAabb.GetMin() + m_brickOrigin);
        worldBrickAabb.SetMax(m_brickAabb.GetMax() + m_brickOrigin);

        AZ::Aabb worldBrickAabb_VisAreaEx = worldBrickAabb;
        worldBrickAabb_VisAreaEx.Expand(AZ::Vector3(SVO_CPU_VOXELIZATION_OFFSET_VISAREA, SVO_CPU_VOXELIZATION_OFFSET_VISAREA, SVO_CPU_VOXELIZATION_OFFSET_VISAREA));

        // add visarea shapes
        if (gEnv->p3DEngine->GetIVisAreaManager())
        {
            for (AZ::s32 v = 0; ; v++)
            {
                superMesh.Clear();

                IVisArea* pVisArea = gEnv->p3DEngine->GetIVisAreaManager()->GetVisAreaById(v);
                if (!pVisArea)
                {
                    break;
                }

                if (pVisArea->IsPortal() || !Overlap::AABB_AABB(*pVisArea->GetAABBox(), AZAabbToLyAABB(worldBrickAabb_VisAreaEx)))
                {
                    continue;
                }

                size_t nPoints = 0;
                const Vec3* pPoints = 0;
                pVisArea->GetShapePoints(pPoints, nPoints);
                float fHeight = pVisArea->GetHeight();

                SRayHitTriangle ht;
                memset(&ht, 0, sizeof(ht));
                ht.c[0] = ht.c[1] = ht.c[2] = Col_Black;
                ht.nOpacity = 255;
                ht.nHitObjType = HIT_OBJ_TYPE_VISAREA;
                Plane pl;

                // sides
                for (size_t i = 0; i < nPoints; i++)
                {
                    const Vec3& v0 = (pPoints)[i];
                    const Vec3& v1 = (pPoints)[(i + 1) % nPoints];

                    ht.v[0] = v0;
                    ht.v[1] = v0 + Vec3(0, 0, fHeight);
                    ht.v[2] = v1;

                    if (Overlap::AABB_Triangle(AZAabbToLyAABB(worldBrickAabb_VisAreaEx), ht.v[0], ht.v[1], ht.v[2]))
                    {
                        ht.nTriArea = SATURATEB(AZ::s32(SVO_CPU_VOXELIZATION_AREA_SCALE * 0.5f * (ht.v[1] - ht.v[0]).Cross(ht.v[2] - ht.v[0]).GetLength()));
                        pl.SetPlane(ht.v[0], ht.v[1], ht.v[2]);
                        ht.n = pl.n;
                        superMesh.AddSuperTriangle(ht);
                    }

                    ht.v[0] = v1;
                    ht.v[1] = v0 + Vec3(0, 0, fHeight);
                    ht.v[2] = v1 + Vec3(0, 0, fHeight);

                    if (Overlap::AABB_Triangle(AZAabbToLyAABB(worldBrickAabb_VisAreaEx), ht.v[0], ht.v[1], ht.v[2]))
                    {
                        ht.nTriArea = SATURATEB(AZ::s32(SVO_CPU_VOXELIZATION_AREA_SCALE * 0.5f * (ht.v[1] - ht.v[0]).Cross(ht.v[2] - ht.v[0]).GetLength()));
                        pl.SetPlane(ht.v[0], ht.v[1], ht.v[2]);
                        ht.n = pl.n;
                        superMesh.AddSuperTriangle(ht);
                    }
                }

                // top and bottom
                for (float fH = 0; fH <= fHeight; fH += fHeight)
                {
                    for (AZ::s32 p = 0; p < ((AZ::s32)nPoints - 2l); p++)
                    {
                        ht.v[0] = (pPoints)[0 + 0] + Vec3(0, 0, fH);
                        ht.v[1] = (pPoints)[p + 1] + Vec3(0, 0, fH);
                        ht.v[2] = (pPoints)[p + 2] + Vec3(0, 0, fH);

                        if (Overlap::AABB_Triangle(AZAabbToLyAABB(worldBrickAabb_VisAreaEx), ht.v[0], ht.v[1], ht.v[2]))
                        {
                            ht.nTriArea = SATURATEB(AZ::s32(SVO_CPU_VOXELIZATION_AREA_SCALE * 0.5f * (ht.v[1] - ht.v[0]).Cross(ht.v[2] - ht.v[0]).GetLength()));
                            pl.SetPlane(ht.v[0], ht.v[1], ht.v[2]);
                            ht.n = pl.n;
                            superMesh.AddSuperTriangle(ht);
                        }
                    }
                }

                AddSuperMesh(superMesh, SVO_CPU_VOXELIZATION_OFFSET_VISAREA);
            }
        }
    }


    const float fSvoSuperMeshHashScale = .1f;

    AZ::s32 SuperMesh::AddVertex(const SRayHitVertex& rVert, AZStd::vector<SRayHitVertex>& vertsInArea)
    {
        vertsInArea.push_back(rVert);
        return vertsInArea.size() - 1;
    }

    void SuperMesh::AddSuperTriangle(SRayHitTriangle& htIn)
    {
        if (m_vertices.size() + 3 > (SMINDEX)~0)
        {
            return;
        }

        SuperTriangle htOut;

        htOut.vFaceNorm = htIn.n;

        SvoMaterialInfo matInfo;
        matInfo.m_material = htIn.pMat;
        auto matIt = std::find(m_materials.begin(), m_materials.end(), matInfo);
        AZ::s32 nMatId = static_cast<AZ::s32>(std::distance(m_materials.begin(), matIt));

        if (matIt == m_materials.end())
        {
            nMatId = m_materials.size();

            // stat obj, get access to texture RGB data
            if (htIn.pMat)
            {
                SShaderItem* pShItem = &htIn.pMat->GetShaderItem();
                AZ::s32* pLowResSystemCopyAtlasId = 0;
                if (pShItem->m_pShaderResources)
                {
                    SEfResTexture* pResTexture = pShItem->m_pShaderResources->GetTextureResource(EFTT_DIFFUSE);
                    if (pResTexture)
                    {
                        ITexture* pITex = pResTexture->m_Sampler.m_pITex;
                        if (pITex)
                        {
                            matInfo.m_textureColor = (ColorB*)pITex->GetLowResSystemCopy(matInfo.m_textureWidth, matInfo.m_textureHeight, &pLowResSystemCopyAtlasId);
                            matInfo.m_texture = pITex;
                            pITex->AddRef();
                        }
                    }
                }
            }

            m_materials.push_back(matInfo);
        }

        htOut.nMatID = nMatId;
        htOut.nTriArea = htIn.nTriArea;
        htOut.nOpacity = htIn.nOpacity;
        htOut.nHitObjType = htIn.nHitObjType;

        for (AZ::s32 v = 0; v < 3; v++)
        {
            SRayHitVertex hv;
            hv.v = htIn.v[v];
            hv.t = htIn.t[v];
            hv.c = htIn.c[v];
            htOut.arrVertId[v] = AddVertex(hv, m_vertices);
        }

        m_triangles.push_back(htOut);
        m_faceNormals.push_back(LYVec3ToAZVec3(htIn.n));
    }

    void SuperMesh::AddSuperMesh(SuperMesh& smIn, float fVertexOffset)
    {
        if (smIn.m_triangles.empty())
        {
            return;
        }

        if (m_vertices.size() + smIn.m_vertices.size() > (SMINDEX)~0)
        {
            return;
        }

        AZStd::vector<AZ::Vector3> vertInNormals(smIn.m_vertices.size(), AZ::Vector3(0.0f));

        for (AZ::s32 t = 0; t < smIn.m_triangles.size(); t++)
        {
            SuperTriangle tr = smIn.m_triangles.at(t);

            for (AZ::s32 v = 0; v < 3; v++)
            {
                vertInNormals[tr.arrVertId[v]] += smIn.m_faceNormals.at(t);
            }
        }

        for (AZ::s32 v = 0; v < smIn.m_vertices.size(); v++)
        {
            smIn.m_vertices.at(v).v += AZVec3ToLYVec3(vertInNormals[v].GetNormalized()) * fVertexOffset;
        }

        AZ::s32 nNumVertBefore = m_vertices.size();

        m_triangles.reserve(m_triangles.size() + smIn.m_triangles.size());

        for (AZ::s32 t = 0; t < smIn.m_triangles.size(); t++)
        {
            SuperTriangle tr = smIn.m_triangles.at(t);

            for (AZ::s32 v = 0; v < 3; v++)
            {
                tr.arrVertId[v] += nNumVertBefore;
            }

            SvoMaterialInfo matInfo;
            matInfo.m_material = smIn.m_materials[tr.nMatID].m_material;

            auto matIt = std::find(m_materials.begin(), m_materials.end(), matInfo);
            AZ::s32 nMatId = static_cast<AZ::s32>(std::distance(m_materials.begin(), matIt));
            if (matIt == m_materials.end())
            {
                nMatId = m_materials.size();

                // stat obj, get access to texture RGB data
                if (matInfo.m_material)
                {
                    SShaderItem* pShItem = &matInfo.m_material->GetShaderItem();
                    AZ::s32* pLowResSystemCopyAtlasId = 0;
                    if (pShItem->m_pShaderResources)
                    {
                        SEfResTexture* pResTexture = pShItem->m_pShaderResources->GetTextureResource(EFTT_DIFFUSE);
                        if (pResTexture)
                        {
                            ITexture* pITex = pResTexture->m_Sampler.m_pITex;
                            if (pITex)
                            {
                                matInfo.m_textureColor = (ColorB*)pITex->GetLowResSystemCopy(matInfo.m_textureWidth, matInfo.m_textureHeight, &pLowResSystemCopyAtlasId);
                                matInfo.m_texture = pITex;
                                pITex->AddRef();
                            }
                        }
                    }
                }

                m_materials.push_back(matInfo);
            }
            tr.nMatID = nMatId;

            m_triangles.push_back(tr);
        }

        m_vertices.insert(m_vertices.end(), smIn.m_vertices.begin(), smIn.m_vertices.end());

        if (fVertexOffset == SVO_CPU_VOXELIZATION_OFFSET_TERRAIN)
        {
            AddSuperMesh(smIn, -1.f);
        }

        smIn.Clear();
    }

    SuperMesh::SuperMesh()
    {
        memset(this, 0, sizeof(*this));
    }

    SuperMesh::~SuperMesh()
    {
        Clear();
    }

    void SuperMesh::ReleaseTextures()
    {
        for (AZ::u32 i = 0; i < m_materials.size(); ++i)
        {
            SvoMaterialInfo& material = m_materials.at(i);
            SAFE_RELEASE(material.m_texture);
        }
    }

    void SuperMesh::Clear()
    {
        ReleaseTextures();

        m_triangles.clear();
        m_vertices.clear();
        m_materials.clear();
        m_faceNormals.clear();
    }
}