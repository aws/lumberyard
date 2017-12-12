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

#include "DefaultMaterial.h"
#include <PRT/SimpleIndexedMesh.h>

NSH::NMaterial::CDefaultSHMaterial::CDefaultSHMaterial(CSimpleIndexedMesh* pMesh)
    : m_pMesh(pMesh)
    , m_RedIntensity(1.f)
    , m_GreenIntensity(1.f)
    , m_BlueIntensity(1.f)
{
    assert(pMesh);
}

const NSH::TRGBCoeffD NSH::NMaterial::CDefaultSHMaterial::DiffuseIntensity
(
    const TRGBCoeffD& crIncidentIntensity,
    const uint32 cTriangleIndex,
    const TVec& rBaryCoord,
    const TCartesianCoord& rIncidentDir,
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
    //set intensities according to requirement: do we need the exitance term? (for interreflection computation) or is it the direct pass where the material property is applied in the shader
    const float redIntensity        = cApplyExitanceTerm ? m_RedIntensity : 1.f;
    const float greenIntensity  = cApplyExitanceTerm ? m_GreenIntensity : 1.f;
    const float blueIntensity       = cApplyExitanceTerm ? m_BlueIntensity : 1.f;

    const double cosAngle = cAbsCosTerm ? abs(normal * rIncidentDir) : (normal * rIncidentDir);
    if (cApplyCosTerm)
    {
        return (cosAngle > 0.) ? TRGBCoeffD(redIntensity * crIncidentIntensity.x * cosAngle, greenIntensity * crIncidentIntensity.y * cosAngle, blueIntensity * crIncidentIntensity.z * cosAngle) : TRGBCoeffD(0., 0., 0.);
    }
    else
    {
        return (cosAngle > 0.) ? TRGBCoeffD(redIntensity * crIncidentIntensity.x, greenIntensity * crIncidentIntensity.y, blueIntensity * crIncidentIntensity.z) : TRGBCoeffD(0., 0., 0.);
    }
}

const NSH::TRGBCoeffD NSH::NMaterial::CDefaultSHMaterial::DiffuseIntensity
(
    const TVec&,
    const TVec& crVertexNormal,
    const Vec2&,
    const TRGBCoeffD& crIncidentIntensity,
    const TCartesianCoord& rIncidentDir,
    const bool cApplyCosTerm,
    const bool cApplyExitanceTerm,
    const bool cAbsCosTerm,
    const bool
) const
{
    const double cosAngle = cAbsCosTerm ? abs(crVertexNormal * rIncidentDir) : (crVertexNormal * rIncidentDir);
    //return unweighted if in the same hemisphere
    //set intensities according to requirement: do we need the exitance term? (for interreflection computation) or is it the direct pass where the material property is applied in the shader
    const float redIntensity        = cApplyExitanceTerm ? m_RedIntensity : 1.f;
    const float greenIntensity  = cApplyExitanceTerm ? m_GreenIntensity : 1.f;
    const float blueIntensity       = cApplyExitanceTerm ? m_BlueIntensity : 1.f;
    if (cApplyCosTerm)
    {
        return (cosAngle > 0.) ? TRGBCoeffD(redIntensity * crIncidentIntensity.x * cosAngle, greenIntensity * crIncidentIntensity.y * cosAngle, blueIntensity * crIncidentIntensity.z * cosAngle) : TRGBCoeffD(0., 0., 0.);
    }
    else
    {
        return (cosAngle > 0.) ? TRGBCoeffD(redIntensity * crIncidentIntensity.x, greenIntensity * crIncidentIntensity.y, blueIntensity * crIncidentIntensity.z) : TRGBCoeffD(0., 0., 0.);
    }
}

void NSH::NMaterial::CDefaultSHMaterial::SetDiffuseIntensity(const float cRedIntensity, const float cGreenIntensity, const float cBlueIntensity, const float)
{
    m_RedIntensity      = cRedIntensity;
    m_GreenIntensity    = cGreenIntensity;
    m_BlueIntensity     = cBlueIntensity;
}

#endif