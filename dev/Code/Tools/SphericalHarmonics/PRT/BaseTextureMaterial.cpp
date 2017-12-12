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

#include "stdafx.h"

#if defined(OFFLINE_COMPUTATION)

#include "BaseTextureMaterial.h"
#include <PRT/SimpleIndexedMesh.h>

NSH::NMaterial::CBaseTextureSHMaterial::CBaseTextureSHMaterial
    (CSimpleIndexedMesh* pMesh,
    SAlphaImageValue* pImageData,
    const uint32 cImageWidth,
    const uint32 cImageHeight)
    : m_pMesh(pMesh)
    , m_pImageData(pImageData)
    , m_ImageWidth(cImageWidth)
    , m_ImageHeight(cImageHeight)
    , m_RedIntensity(1.f)
    , m_GreenIntensity(1.f)
    , m_BlueIntensity(1.f)
{
    assert(pMesh);
    assert(pImageData);
}

NSH::NMaterial::CBaseTextureSHMaterial::~CBaseTextureSHMaterial()
{
    static CSHAllocator<float> sAllocator;
    if (m_pImageData)
    {
        sAllocator.delete_mem_array(m_pImageData, sizeof(SAlphaImageValue) * m_ImageWidth * m_ImageHeight);
    }
}

const NSH::TRGBCoeffD NSH::NMaterial::CBaseTextureSHMaterial::DiffuseIntensity
(
    const NSH::TRGBCoeffD& crIncidentIntensity,
    const uint32 cTriangleIndex,
    const TVec& rBaryCoord,
    const NSH::TCartesianCoord& rIncidentDir,
    const bool cApplyCosTerm,
    const bool cApplyExitanceTerm,
    const bool cAbsCosTerm,
    const bool
) const
{
    //retrieve normal
    const CObjFace& f = m_pMesh->GetObjFace(cTriangleIndex);
    const Vec3& n0 = m_pMesh->GetWSNormal(f.n[0]);
    const Vec3& n1 = m_pMesh->GetWSNormal(f.n[1]);
    const Vec3& n2 = m_pMesh->GetWSNormal(f.n[2]);
    //interpolate according to barycentric coordinates
    assert(Abs(rBaryCoord.x + rBaryCoord.y + rBaryCoord.z - 1.) < 0.01);
    const TVec normal
    (
        rBaryCoord.x * n0.x + rBaryCoord.y * n1.x + rBaryCoord.z * n2.x,
        rBaryCoord.x * n0.y + rBaryCoord.y * n1.y + rBaryCoord.z * n2.y,
        rBaryCoord.x * n0.z + rBaryCoord.y * n1.z + rBaryCoord.z * n2.z
    );

    const double cCosAngle = cAbsCosTerm ? abs(normal * rIncidentDir) : (normal * rIncidentDir);
    if (cApplyCosTerm && cCosAngle <= 0.)
    {
        return NSH::TRGBCoeffD(0., 0., 0.);
    }

    const Vec2 uv0(m_pMesh->GetTexCoord(f.n[0]).s, m_pMesh->GetTexCoord(f.n[0]).t);
    const Vec2 uv1(m_pMesh->GetTexCoord(f.n[1]).s, m_pMesh->GetTexCoord(f.n[1]).t);
    const Vec2 uv2(m_pMesh->GetTexCoord(f.n[2]).s, m_pMesh->GetTexCoord(f.n[2]).t);
    const Vec2 uvCoord
    (
        (float)(uv0.x * rBaryCoord.x + uv1.x * rBaryCoord.y + uv2.x * rBaryCoord.z),
        (float)(uv0.y * rBaryCoord.x + uv1.y * rBaryCoord.y + uv2.y * rBaryCoord.z)
    );
    return ComputeDiffuseIntensity(crIncidentIntensity, uvCoord, cApplyCosTerm, cApplyExitanceTerm, cCosAngle);
}

const NSH::TRGBCoeffD NSH::NMaterial::CBaseTextureSHMaterial::DiffuseIntensity
(
    const TVec&,
    const TVec& crVertexNormal,
    const Vec2& rUVCoords,
    const NSH::TRGBCoeffD& crIncidentIntensity,
    const NSH::TCartesianCoord& rIncidentDir,
    const bool cApplyCosTerm,
    const bool cApplyExitanceTerm,
    const bool cAbsCosTerm,
    const bool
) const
{
    const double cCosAngle = cAbsCosTerm ? abs(crVertexNormal * rIncidentDir) : (crVertexNormal * rIncidentDir);
    if (cApplyCosTerm && cCosAngle <= 0.)
    {
        return NSH::TRGBCoeffD(0., 0., 0.);
    }
    return ComputeDiffuseIntensity(crIncidentIntensity, rUVCoords, cApplyCosTerm, cApplyExitanceTerm, cCosAngle);
}

void NSH::NMaterial::CBaseTextureSHMaterial::RetrieveTexelValue(const Vec2& rUVCoords, NSH::NMaterial::SAlphaImageValue& rTexelValue) const
{
    //clamp texture coordinates
    float u = rUVCoords.x - (float)((int)rUVCoords.x);
    float v = rUVCoords.y - (float)((int)rUVCoords.y);
    if (u < 0)//unmirror u-coord
    {
        u = 1.f - fabs(u);
    }
    if (v < 0)//unmirror u-coord
    {
        v = 1.f - fabs(v);
    }
    //get pixel coordinates
    u = u * (float)m_ImageWidth;
    v = v * (float)m_ImageHeight;
    //now fetch the texels for bilinear filter
    const int x0 = int(u), y0 = int(v);
    const float dx = u - x0, dy = v - y0, omdx = 1 - dx, omdy = 1 - dy;
    //fetch the 4 texels
    const SAlphaImageValue cI0(m_pImageData[std::min(y0, (int)(m_ImageHeight - 1)) * m_ImageWidth + std::min(x0, (int)(m_ImageWidth - 1))]);
    assert(std::min((y0), (int)(m_ImageWidth - 1)) * m_ImageWidth + std::min((x0), (int)(m_ImageWidth - 1)) < m_ImageWidth * m_ImageHeight);
    const SAlphaImageValue cI1(m_pImageData[std::min(y0 + 1, (int)(m_ImageHeight - 1)) * m_ImageWidth + std::min(x0, (int)(m_ImageWidth - 1))]);
    assert(std::min((y0 + 1), (int)(m_ImageWidth - 1)) * m_ImageWidth + std::min((x0), (int)(m_ImageWidth - 1)) < m_ImageWidth * m_ImageHeight);
    const SAlphaImageValue cI2(m_pImageData[std::min(y0, (int)(m_ImageHeight - 1)) * m_ImageWidth + std::min((x0 + 1), (int)(m_ImageWidth - 1))]);
    assert(std::min((y0), (int)(m_ImageWidth - 1)) * m_ImageWidth + std::min((x0 + 1), (int)(m_ImageWidth - 1)) < m_ImageWidth * m_ImageHeight);
    const SAlphaImageValue cI3(m_pImageData[std::min((y0 + 1), (int)(m_ImageHeight - 1)) * m_ImageWidth + std::min((x0 + 1), (int)(m_ImageWidth - 1))]);
    assert(std::min((y0 + 1), (int)(m_ImageWidth - 1)) * m_ImageWidth + std::min((x0 + 1), (int)(m_ImageWidth - 1)) < m_ImageWidth * m_ImageHeight);
    rTexelValue = cI0 * (omdx * omdy) + cI1 * (omdx * dy) + cI2 * (dx * omdy) + cI3 * (dx * dy);
}

inline void NSH::NMaterial::CBaseTextureSHMaterial::SetDiffuseIntensity(const float cRedIntensity, const float cGreenIntensity, const float cBlueIntensity, const float)
{
    m_RedIntensity      = cRedIntensity;
    m_GreenIntensity    = cGreenIntensity;
    m_BlueIntensity     = cBlueIntensity;
}

const NSH::TRGBCoeffD NSH::NMaterial::CBaseTextureSHMaterial::ComputeDiffuseIntensity
(
    const TRGBCoeffD& crIncidentIntensity,
    const Vec2& rUVCoords,
    const bool cApplyCosTerm,
    const bool cApplyExitanceTerm,
    const double cCosAngle
) const
{
    if (cApplyExitanceTerm)
    {
        SAlphaImageValue texelValue;
        RetrieveTexelValue(rUVCoords, texelValue);
        if (cApplyCosTerm)
        {
            return NSH::TRGBCoeffD
                   (
                m_RedIntensity * texelValue.r * crIncidentIntensity.x * cCosAngle,
                m_GreenIntensity * texelValue.g * crIncidentIntensity.y * cCosAngle,
                m_BlueIntensity * texelValue.b * crIncidentIntensity.z * cCosAngle
                   );
        }
        else
        {
            return NSH::TRGBCoeffD
                   (
                m_RedIntensity * texelValue.r * crIncidentIntensity.x,
                m_GreenIntensity * texelValue.g * crIncidentIntensity.y,
                m_BlueIntensity * texelValue.b * crIncidentIntensity.z
                   );
        }
    }
    else
    {
        if (cApplyCosTerm)
        {
            return NSH::TRGBCoeffD
                   (
                crIncidentIntensity.x * cCosAngle,
                crIncidentIntensity.y * cCosAngle,
                crIncidentIntensity.z * cCosAngle
                   );
        }
        else
        {
            return NSH::TRGBCoeffD
                   (
                crIncidentIntensity.x,
                crIncidentIntensity.y,
                crIncidentIntensity.z
                   );
        }
    }
}

#endif