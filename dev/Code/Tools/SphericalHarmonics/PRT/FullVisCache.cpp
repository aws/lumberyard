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

#include "FullVisCache.h"
#include "RayCaster.h"

//all implementations taken from Lightmap compiler
const NSH::EReturnSinkValue NSH::CFullVisCache::CSimpleHit::ReturnElement(const RasterCubeUserT& crInObject, float&)
{
    float u, v, t;
    if (!InsertAlreadyTested(&crInObject))
    {
        Vec3 outputPos;
        if (NSH::NRayTriangleIntersect::RayTriangleIntersectTest(m_RayOrigin, m_RayDir, crInObject.vertices[0], crInObject.vertices[1], crInObject.vertices[2], outputPos, t, u, v, crInObject.isSingleSided))
        {
            if (t >= m_Bias && t < m_RayLen)
            {
                float fDist = outputPos * m_PNormal + m_D;
                // we need get shadow for near lying geometry.
                const float cfThreshold = 0.01f;
                if (fabs(fDist) > cfThreshold)
                {
                    // Make sure the ray is not too parallel to the plane, fixed the other 50% of the error cases
                    float fDot = crInObject.vNormal * m_RayDir;
                    const float cfBias = 0.01f;
                    if (fabs(fDot) > cfBias)
                    {
                        m_HasHit = true;
                        return crInObject.fastProcessing ? NSH::RETURN_SINK_HIT_AND_EARLY_OUT : NSH::RETURN_SINK_HIT;
                    }
                }
            }
        }
    }
    return NSH::RETURN_SINK_FAIL;
}

NSH::CFullVisCache::CFullVisCache()
    : m_UpperHemisphereIsVisible(false)
    , m_LowerHemisphereIsVisible(false)
{
    m_UpperDirections.resize(gscHemisphereSamplesPerTheta * gscHemisphereSamplesPerPhi);
    m_LowerDirections.resize(gscHemisphereSamplesPerTheta * gscHemisphereSamplesPerPhi);
    //now generate directions
    //upper hemisphere
    //go from 5 degree 80 degree to 85 degree in each direction to not consider geometry lying on the very edge
    const float scThetaStepWidth = (float)(NSH::g_cPi * (80.0f /*80 degree*/ / 180.0f) / (float)gscHemisphereSamplesPerTheta);
    const float scPhiStepWidth = (float)(NSH::g_cPi * 2.0 / (float)gscHemisphereSamplesPerPhi);
    const float scThetaOffset = (float)(NSH::g_cPi * (5.0f /*5 degree*/ / 180.0f));
    NSH::SPolarCoord_tpl<double> upperPolarAngle(scThetaOffset, 0);
    NSH::SPolarCoord_tpl<double> lowerPolarAngle(NSH::g_cPi * 0.5f + scThetaOffset, 0);
    for (int i = 0, index = 0; i < gscHemisphereSamplesPerTheta; ++i)
    {
        upperPolarAngle.phi = 0;
        lowerPolarAngle.phi = 0;
        for (int j = 0; j < gscHemisphereSamplesPerPhi; ++j, ++index)
        {
            NSH::SCartesianCoord_tpl<double> cartCoord;
            ConvertToCartesian(cartCoord, upperPolarAngle);
            m_UpperDirections[index] = cartCoord;
            ConvertToCartesian(cartCoord, lowerPolarAngle);
            m_LowerDirections[index] = cartCoord;
            upperPolarAngle.phi += scPhiStepWidth;
            lowerPolarAngle.phi += scPhiStepWidth;
        }
        upperPolarAngle.theta += scThetaStepWidth;
        lowerPolarAngle.theta += scThetaStepWidth;
    }
}

void NSH::CFullVisCache::GenerateFullVisInfo
(
    const NSH::TRasterCubeImpl& crRasterCube,
    const TVec& crOrigin,
    const float cRayLen,
    const TVec& cNormal,
    const float cRayTracingBias,
    const bool cOnlyUpperHemisphere
)
{
    m_UpperHemisphereIsVisible = true;
    m_LowerHemisphereIsVisible = cOnlyUpperHemisphere ? false : true;

    CSimpleHit anyHit;
    //upper hemisphere
    uint32 hits = 0;
    const TVecDVec::const_iterator cEnd = m_UpperDirections.end();
    for (TVecDVec::const_iterator iter = m_UpperDirections.begin(); iter != cEnd; ++iter)
    {
        const TVec& crDir = *iter;
        anyHit.SetupRay(crDir, crOrigin, (float)cRayLen, (float)-(cNormal * crOrigin), cNormal, cRayTracingBias);
        crRasterCube.GatherRayHitsDirection<CSimpleHit, true>(TVector3D(crOrigin.x, crOrigin.y, crOrigin.z), TVector3D(crDir.x, crDir.y, crDir.z), anyHit);
        if (anyHit.IsIntersecting())
        {
            hits++;
        }
        if (hits > gscFailThreshold)
        {
            m_UpperHemisphereIsVisible = false;
            break;
        }
    }
    if (!cOnlyUpperHemisphere)
    {
        //lower hemisphere
        uint32 hits = 0;
        const TVecDVec::const_iterator cEnd = m_LowerDirections.end();
        for (TVecDVec::const_iterator iter = m_LowerDirections.begin(); iter != cEnd; ++iter)
        {
            const TVec& crDir = *iter;
            anyHit.SetupRay(crDir, crOrigin, (float)cRayLen, (float)-(cNormal * crOrigin), cNormal, cRayTracingBias);
            crRasterCube.GatherRayHitsDirection<CSimpleHit, true>(TVector3D(crOrigin.x, crOrigin.y, crOrigin.z), TVector3D(crDir.x, crDir.y, crDir.z), anyHit);
            if (anyHit.IsIntersecting())
            {
                hits++;
            }
            if (hits > gscFailThreshold)
            {
                m_LowerHemisphereIsVisible = false;
                break;
            }
        }
    }
}

#endif