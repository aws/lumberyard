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
#include "FogVolumeRenderNode.h"
#include "VisAreas.h"
#include "CREFogVolume.h"
#include "Cry_Geo.h"
#include "ObjMan.h"


///////////////////////////////////////////////////////////////////////////////
inline static float expf_s(float arg)
{
    return expf(clamp_tpl(arg, -80.0f, 80.0f));
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void CFogVolumeRenderNode::TraceFogVolumes(const Vec3& worldPos, ColorF& fogColor, const SRenderingPassInfo& passInfo)
{
    FUNCTION_PROFILER_3DENGINE;
    PrefetchLine(&s_tracableFogVolumeArea, 0);

    // init default result
    ColorF localFogColor = ColorF(0.0f, 0.0f, 0.0f, 0.0f);


    // trace is needed when volumetric fog is off.
    if (GetCVars()->e_Fog && GetCVars()->e_FogVolumes && (GetCVars()->e_VolumetricFog == 0))
    {
        // init view ray
        Vec3 camPos(s_tracableFogVolumeArea.GetCenter());
        Lineseg lineseg(camPos, worldPos);

#ifdef _DEBUG
        const SCachedFogVolume* prev(0);
#endif

        // loop over all traceable fog volumes
        CachedFogVolumes::const_iterator itEnd(s_cachedFogVolumes.end());
        for (CachedFogVolumes::const_iterator it(s_cachedFogVolumes.begin()); it != itEnd; ++it)
        {
            // get current fog volume
            const CFogVolumeRenderNode* pFogVol((*it).m_pFogVol);

            // only trace visible fog volumes
            if (!(pFogVol->GetRndFlags() & ERF_HIDDEN))
            {
                // check if view ray intersects with bounding box of current fog volume
                if (Overlap::Lineseg_AABB(lineseg, pFogVol->m_WSBBox))
                {
                    // compute contribution of current fog volume
                    ColorF color;
                    if (0 == pFogVol->m_volumeType)
                    {
                        pFogVol->GetVolumetricFogColorEllipsoid(worldPos, passInfo, color);
                    }
                    else
                    {
                        pFogVol->GetVolumetricFogColorBox(worldPos, passInfo, color);
                    }

                    color.a = 1.0f - color.a;       // 0 = transparent, 1 = opaque

                    // blend fog colors
                    localFogColor.r = Lerp(localFogColor.r, color.r, color.a);
                    localFogColor.g = Lerp(localFogColor.g, color.g, color.a);
                    localFogColor.b = Lerp(localFogColor.b, color.b, color.a);
                    localFogColor.a = Lerp(localFogColor.a, 1.0f, color.a);
                }
            }

#ifdef _DEBUG
            if (prev)
            {
                assert(prev->m_distToCenterSq >= (*it).m_distToCenterSq);
                prev = &(*it);
            }
#endif
        }

        const float fDivisor        = (float)fsel(-localFogColor.a, 1.0f, localFogColor.a);
        const float fMultiplier = (float)fsel(-localFogColor.a, 0.0f, 1.0f / fDivisor);

        localFogColor.r *= fMultiplier;
        localFogColor.g *= fMultiplier;
        localFogColor.b *= fMultiplier;
    }

    localFogColor.a = 1.0f - localFogColor.a;

    fogColor = localFogColor;
}

///////////////////////////////////////////////////////////////////////////////
void CFogVolumeRenderNode::GetVolumetricFogColorEllipsoid(const Vec3& worldPos, const SRenderingPassInfo& passInfo, ColorF& resultColor) const
{
    const CCamera& cam(passInfo.GetCamera());
    Vec3 camPos(cam.GetPosition());
    Vec3 camDir(cam.GetViewdir());
    Vec3 cameraLookDir(worldPos - camPos);

    resultColor = ColorF(1.0f, 1.0f, 1.0f, 1.0f);

    if (cameraLookDir.GetLengthSquared() > 1e-4f)
    {
        // setup ray tracing in OS
        Vec3 cameraPosInOSx2(m_matWSInv.TransformPoint(camPos) * 2.0f);
        Vec3 cameraLookDirInOS(m_matWSInv.TransformVector(cameraLookDir));

        float tI(sqrtf(cameraLookDirInOS.Dot(cameraLookDirInOS)));
        float invOfScaledCamDirLength(1.0f / tI);
        cameraLookDirInOS *= invOfScaledCamDirLength;

        // calc coefficients for ellipsoid parametrization (just a simple unit-sphere in its own space)
        float B(cameraPosInOSx2.Dot(cameraLookDirInOS));
        float Bsq(B * B);
        float C(cameraPosInOSx2.Dot(cameraPosInOSx2) - 4.0f);

        // solve quadratic equation
        float discr(Bsq - C);
        if (discr >= 0.0)
        {
            float discrSqrt = sqrtf(discr);

            // ray hit
            Vec3 cameraPosInWS(camPos);
            Vec3 cameraLookDirInWS((worldPos - camPos) * invOfScaledCamDirLength);

            //////////////////////////////////////////////////////////////////////////

            float tS(max(0.5f * (-B - discrSqrt), 0.0f));       // clamp to zero so front ray-ellipsoid intersection is NOT behind camera
            float tE(max(0.5f * (-B + discrSqrt), 0.0f));       // clamp to zero so back ray-ellipsoid intersection is NOT behind camera
            //float tI( ( worldPos - camPos ).Dot( camDir ) / cameraLookDirInWS.Dot( camDir ) );
            tI = max(tS, min(tI, tE));     // clamp to range [tS, tE]

            Vec3 front(tS * cameraLookDirInWS + cameraPosInWS);
            Vec3 dist((tI - tS) * cameraLookDirInWS);
            float distLength(dist.GetLength());
            float fogInt(distLength * expf_s(-(front - m_heightFallOffBasePoint).Dot(m_heightFallOffDirScaled)));

            //////////////////////////////////////////////////////////////////////////

            float heightDiff(dist.Dot(m_heightFallOffDirScaled));
            if (fabsf(heightDiff) > 0.001f)
            {
                fogInt *= (1.0f - expf_s(-heightDiff)) / heightDiff;
            }

            float softArg(clamp_tpl(discr * m_cachedSoftEdgesLerp.x + m_cachedSoftEdgesLerp.y, 0.0f, 1.0f));
            fogInt *= softArg * (2.0f - softArg);

            float fog(expf_s(-m_globalDensity * fogInt));

            resultColor = ColorF(m_cachedFogColor.r, m_cachedFogColor.g, m_cachedFogColor.b, min(fog, 1.0f));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
void CFogVolumeRenderNode::GetVolumetricFogColorBox(const Vec3& worldPos, const SRenderingPassInfo& passInfo, ColorF& resultColor) const
{
    const CCamera& cam(passInfo.GetCamera());
    Vec3 camPos(cam.GetPosition());
    Vec3 cameraLookDir(worldPos - camPos);

    resultColor = ColorF(1.0f, 1.0f, 1.0f, 1.0f);

    if (cameraLookDir.GetLengthSquared() > 1e-4f)
    {
        // setup ray tracing in OS
        Vec3 cameraPosInOS(m_matWSInv.TransformPoint(camPos));
        Vec3 cameraLookDirInOS(m_matWSInv.TransformVector(cameraLookDir));

        float tI(sqrtf(cameraLookDirInOS.Dot(cameraLookDirInOS)));
        float invOfScaledCamDirLength(1.0f / tI);
        cameraLookDirInOS *= invOfScaledCamDirLength;

        const float fMax = std::numeric_limits<float>::max();
        float tS(0), tE(fMax);

        //TODO:
        //  May be worth profiling use of a loop here, iterating over elements of vector;
        //  might save on i-cache, but suspect we'll lose instruction interleaving and hit
        //  more register dependency issues.

        //These fsels mean that the result is ignored if cameraLookDirInOS.x is 0.0f,
        //  avoiding a floating point compare. Avoiding the fcmp is ~15% faster
        const float fXSelect        = -fabsf(cameraLookDirInOS.x);
        const float fXDivisor       = (float)fsel(fXSelect, 1.0f, cameraLookDirInOS.x);
        const float fXMultiplier = (float)fsel(fXSelect, 0.0f, 1.0f);
        const float fXInvMultiplier = 1.0f - fXMultiplier;

        //Accurate to 255/256ths on console
        float invCameraDirInOSx = fres(fXDivisor); //(1.0f / fXDivisor);

        float tPosPlane((1 - cameraPosInOS.x) * invCameraDirInOSx);
        float tNegPlane((-1 - cameraPosInOS.x) * invCameraDirInOSx);

        float tFrontFace    = (float)fsel(-cameraLookDirInOS.x, tPosPlane, tNegPlane);
        float tBackFace     = (float)fsel(-cameraLookDirInOS.x, tNegPlane, tPosPlane);

        tS = max(tS, tFrontFace * fXMultiplier);
        tE = min(tE, (tBackFace * fXMultiplier) + (fXInvMultiplier * fMax));



        const float fYSelect        = -fabsf(cameraLookDirInOS.y);
        const float fYDivisor       = (float)fsel(fYSelect, 1.0f, cameraLookDirInOS.y);
        const float fYMultiplier = (float)fsel(fYSelect, 0.0f, 1.0f);
        const float fYInvMultiplier = 1.0f - fYMultiplier;

        //Accurate to 255/256ths on console
        float invCameraDirInOSy = fres(fYDivisor); //(1.0f / fYDivisor);

        tPosPlane = ((1 - cameraPosInOS.y) * invCameraDirInOSy);
        tNegPlane = ((-1 - cameraPosInOS.y) * invCameraDirInOSy);

        tFrontFace  = (float)fsel(-cameraLookDirInOS.y, tPosPlane, tNegPlane);
        tBackFace       = (float)fsel(-cameraLookDirInOS.y, tNegPlane, tPosPlane);

        tS = max(tS, tFrontFace * fYMultiplier);
        tE = min(tE, (tBackFace * fYMultiplier) + (fYInvMultiplier * fMax));



        const float fZSelect        = -fabsf(cameraLookDirInOS.z);
        const float fZDivisor       = (float)fsel(fZSelect, 1.0f, cameraLookDirInOS.z);
        const float fZMultiplier = (float)fsel(fZSelect, 0.0f, 1.0f);
        const float fZInvMultiplier = 1.0f - fZMultiplier;

        //Accurate to 255/256ths on console
        float invCameraDirInOSz = fres(fZDivisor); //(1.0f / fZDivisor);

        tPosPlane = ((1 - cameraPosInOS.z) * invCameraDirInOSz);
        tNegPlane = ((-1 - cameraPosInOS.z) * invCameraDirInOSz);

        tFrontFace  = (float)fsel(-cameraLookDirInOS.z, tPosPlane, tNegPlane);
        tBackFace       = (float)fsel(-cameraLookDirInOS.z, tNegPlane, tPosPlane);

        tS = max(tS, tFrontFace * fZMultiplier);
        tE = min(tE, (tBackFace * fZMultiplier) + (fZInvMultiplier * fMax));

        tE = max(tE, 0.0f);

        if (tS <= tE)
        {
            Vec3 cameraPosInWS(camPos);
            Vec3 cameraLookDirInWS((worldPos - camPos) * invOfScaledCamDirLength);

            //////////////////////////////////////////////////////////////////////////

            tI = max(tS, min(tI, tE));     // clamp to range [tS, tE]

            Vec3 front(tS * cameraLookDirInWS + cameraPosInWS);
            Vec3 dist((tI - tS) * cameraLookDirInWS);
            float distLength(dist.GetLength());
            float fogInt(distLength * expf_s(-(front - m_heightFallOffBasePoint).Dot(m_heightFallOffDirScaled)));

            //////////////////////////////////////////////////////////////////////////

            float heightDiff(dist.Dot(m_heightFallOffDirScaled));

            //heightDiff = fabsf( heightDiff ) > 0.001f ? heightDiff : 0.001f
            heightDiff = (float)fsel((-fabsf(heightDiff) + 0.001f), 0.001f, heightDiff);

            fogInt *= (1.0f - expf_s(-heightDiff)) * fres(heightDiff);

            float fog(expf_s(-m_globalDensity * fogInt));

            resultColor = ColorF(m_cachedFogColor.r, m_cachedFogColor.g, m_cachedFogColor.b, min(fog, 1.0f));
        }
    }
}