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

// Description : Split out from ParticleEmitter.cpp

#include "StdAfx.h"
#include "ParticleSubEmitter.h"
#include "ParticleEmitter.h"
#include <IAudioSystem.h>
#include <AzCore/Math/Internal/MathTypes.h>

#include <PNoise3.h>

//////////////////////////////////////////////////////////////////////////
// CParticleSubEmitter implementation.
//////////////////////////////////////////////////////////////////////////

#define BEAM_COUNT_CORRECTION 1 //Beams require one extra particle to finish each beam. 

CParticleSubEmitter::CParticleSubEmitter(CParticleSource* pSource, CParticleContainer* pCont)
    : m_ChaosKey(0U)
    , m_nEmitIndex(0)
    , m_nSequenceIndex(0)
    , m_pContainer(pCont)
    , m_pSource(pSource)
    , m_nStartAudioTriggerID(INVALID_AUDIO_CONTROL_ID)
    , m_nStopAudioTriggerID(INVALID_AUDIO_CONTROL_ID)
    , m_nAudioRtpcID(INVALID_AUDIO_CONTROL_ID)
    , m_pIAudioProxy(NULL)
    , m_bExecuteAudioTrigger(false)
    , m_vPosIncrementXYZ(0,0,0)
    , m_prevRelativeRotation(IDENTITY)
{
    assert(pCont);
    assert(pSource);
    SetLastLoc();
    m_fActivateAge = -fHUGE;
    m_fStartAge = m_fStopAge = m_fRepeatAge = fHUGE;
    m_fLastEmitAge = 0.f;
    m_pForce = NULL;
}

CParticleSubEmitter::~CParticleSubEmitter()
{
    Deactivate();
}

//////////////////////////////////////////////////////////////////////////
void CParticleSubEmitter::Initialize(float fAge)
{
    const ResourceParticleParams& params = GetParams();

    m_nEmitIndex = 0;
    m_nSequenceIndex = m_pContainer->GetNextEmitterSequence();
    SetLastLoc();

    // Reseed randomness.
    m_ChaosKey = CChaosKey(cry_random_uint32());
    m_fActivateAge = fAge;

    // Compute next repeat age.
    m_fRepeatAge = fHUGE;
    if (params.fPulsePeriod.GetMaxValue() > 0.f)
    {
        float fRepeat = params.fPulsePeriod(VRANDOM);
        fRepeat = max(fRepeat, 0.1f);
        if (m_fActivateAge + fRepeat < GetAge())
        {
            // Needs to repeat again. Calculate final repeat before present, adjust.
            float fAdjust = GetAge() - m_fActivateAge;
            fAdjust -= fmod(fAdjust, fRepeat);
            m_fActivateAge += fAdjust;
        }
        m_fRepeatAge = m_fActivateAge + fRepeat;
    }

    // Compute lifetime params.
    m_fStartAge = m_fStopAge = m_fLastEmitAge = m_fActivateAge + params.fSpawnDelay(VRANDOM);
    if (params.bContinuous || !params.fParticleLifeTime)
    {
        if (params.fEmitterLifeTime)
        {
            m_fStopAge += params.fEmitterLifeTime(VRANDOM);
        }
        else
        {
            m_fStopAge = fHUGE;
        }
    }

    m_bExecuteAudioTrigger = true;

    m_vPosIncrementXYZ = Vec3(0, 0, 0);

    UpdateEmitterBounds();
}

//////////////////////////////////////////////////////////////////////////
void CParticleSubEmitter::Deactivate()
{
    DeactivateAudio();

    if (m_pForce)
    {
        GetPhysicalWorld()->DestroyPhysicalEntity(m_pForce);
        m_pForce = 0;
    }
}

bool CParticleSubEmitter::UpdateState(float fAgeAdjust)
{
    // Evolve emitter state.
    m_fActivateAge += fAgeAdjust;
    m_fLastEmitAge += fAgeAdjust;

    float fAge = GetAge();

    if (fAge >= m_fRepeatAge && GetSource().GetStopAge() > m_fRepeatAge)
    {
        Initialize(m_fRepeatAge);
    }
    else
    {
        ParticleParams::ESpawn eSpawn = GetParams().eSpawnIndirection;
        float fActivateAge = eSpawn == eSpawn.ParentCollide ? GetSource().GetCollideAge()
            : eSpawn == eSpawn.ParentDeath ? GetSource().GetStopAge()
            : 0.f;
        if (fActivateAge > m_fActivateAge && fAge >= fActivateAge)
        {
            Initialize(fActivateAge);
        }
        else if (!GetSource().IsAlive())
        {
            return false;
        }
    }

    m_fStopAge = min(m_fStopAge, GetSource().GetStopAge());

    UpdateEmitterBounds();
    return true;
}

float CParticleSubEmitter::GetStopAge(ParticleParams::ESoundControlTime eControl) const
{
    switch (eControl)
    {
    default:
    case ParticleParams::ESoundControlTime::EmitterLifeTime:
        return GetStopAge();
    case ParticleParams::ESoundControlTime::EmitterExtendedLifeTime:
        return GetParticleStopAge();
    case ParticleParams::ESoundControlTime::EmitterPulsePeriod:
        return m_fRepeatAge;
    }
}

float CParticleSubEmitter::GetAgeRelativeTo(float fStopAge, float fAgeAdjust) const
{
    float fAge = GetAge() + fAgeAdjust;
    if (min(fAge, fStopAge) <= m_fStartAge)
    {
        return 0.f;
    }

    return div_min(fAge - m_fStartAge, fStopAge - m_fStartAge, 1.f);
}

float CParticleSubEmitter::GetStrength(float fAgeAdjust /* = 0.f */, ParticleParams::ESoundControlTime const eControl /* = EmitterLifeTime */) const
{
    float fStrength = GetMain().GetSpawnParams().fStrength;
    if (fStrength < 0.f)
    {
        if (GetParams().bContinuous)
        {
            return GetRelativeAge(eControl, fAgeAdjust);
        }
        else
        {
            return div_min((float)m_nEmitIndex, GetParams().fCount.GetMaxValue(), 1.f);
        }
    }
    else
    {
        return min(fStrength, 1.f);
    }
}

void CParticleSubEmitter::EmitParticles(SParticleUpdateContext& context)
{
    const ResourceParticleParams& params = GetParams();
    //ensure we have the correct bounding volume
    UpdateEmitterBounds();
    //If the lod is finished blending to 0, particles will no longer be visible and we can stop emitting    
    if (context.lodBlend <= 0)
    {
        m_fLastEmitAge = GetAge();
        SetLastLoc();
        return;
    }
    context.fDensityAdjust = 1.f;

    float fAge = GetAge();
    if (fAge >= m_fStartAge && (params.bContinuous || fAge >= m_fLastEmitAge))
    {
        const float fStrength = GetStrength(context.fUpdateTime * 0.5f);
        float fCount = params.GetCount()(VRANDOM, fStrength);
        if (!GetContainer().GetParent())
        {
            fCount *= GetMain().GetEmitCountScale();
        }
        if (fCount <= 0.f)
        {
            return;
        }
        else if (fCount > PARTICLE_PARAMS_MAX_COUNT_CPU)
        {
            fCount = PARTICLE_PARAMS_MAX_COUNT_CPU;
        }

        const float fEmitterLife = GetStopAge() - GetStartAge();

        // We use the original particle lifetime instead of lifetime with strength so the emit rate will be stable. 
        // Otherwise, the strength curve with a key value 0 could result a lifetime very close to 0 and very high emit rate 
        // which leads to a very high amount of particles generated. 
        // Also, we can't limit the total particles generated per container since the use case for indirect spawned emitters is complicated (the child emitter instances are using the same container).
        // And anyway, it seems more intuitive to the user that the rate be constant even when particle lifetime is ramping down.
        float fParticleLife = params.GetEmitterShape() == ParticleParams::EEmitterShapeType::BEAM ? params.fBeamAge : params.fParticleLifeTime;
        
        //Since we are using the original particle lifetime without the strength curve, it's relatively safe to keep this code here; 
        //i.e. it won't ramp down toward zero and suddenly jump up to fEmitterLife.
        if (fParticleLife < AZ_FLT_EPSILON)
        {
            fParticleLife = fEmitterLife;
            if (fParticleLife <= 0.f)
            {
                return;
            }
        }

        EmitParticleData data;
        data.Location = GetSource().GetLocation();

        if (params.bContinuous)
        {
            float fEmitRate;
            // Continuous emission rate which maintains fCount particles, while checking for possible Div by Zero.
            if (fEmitterLife <= 0.f)
            {
                fEmitRate = fCount / fParticleLife; // I don't think this will ever be hit because EmitterLifetime can't be negative, and when EmitterLifetime is 0, fEmitterLife will be fHUGE.
            }
            else
            {
                fEmitRate = max(fCount / fParticleLife, fCount / fEmitterLife);
            }
            float fAgeIncrement = 1.f / fEmitRate;

            // Set up location interpolation.
            const QuatTS& locA = GetSource().GetLocation();
            const QuatTS& locB = m_LastLoc;

            struct
            {
                bool    bInterp;
                bool    bSlerp;
                float fInterp;
                float fInterpInc;
                Vec3    vArc;
                float   fA;
                float fCosA;
                float   fInvSinA;
            } Interp;

            Interp.bInterp = context.fUpdateTime > 0.f && !params.eMoveRelEmitter
                && m_LastLoc.s >= 0.f && !QuatTS::IsEquivalent(locA, locB, 0.0045f, 1e-5f);

            if (params.fMaintainDensity && (Interp.bInterp || params.nEnvFlags & ENV_PHYS_AREA))
            {
                float fAdjust = ComputeDensityIncrease(fStrength, fParticleLife, locA, Interp.bInterp ? &locB : 0);
                if (fAdjust > 1.f)
                {
                    fAdjust = Lerp(1.f, fAdjust, params.fMaintainDensity);
                    fEmitRate *= fAdjust;
                    fAgeIncrement = 1.f / fEmitRate;
                    context.fDensityAdjust = Lerp(1.f, 1.f / fAdjust, params.fMaintainDensity.fReduceAlpha);
                }
            }

            // Determine time window to update.
            float fAge0 = max(m_fLastEmitAge, m_fStartAge);
            const float fFullLife = GetContainer().GetMaxParticleFullLife();
            float fSkipTime = fAge - fFullLife - fAge0;
            if (fSkipTime > 0.f)
            {
                // Skip time before emitted particles would still be alive.
                fAge0 += ceilf(fSkipTime * fEmitRate) * fAgeIncrement;
            }
            float fAge1 = min(fAge, m_fStopAge);

            if (Interp.bInterp)
            {
                Interp.bSlerp = false;
                Interp.fInterp = div_min(fAge - fAge0, context.fUpdateTime, 1.f);
                Interp.fInterpInc = -div_min(fAgeIncrement, context.fUpdateTime, 1.f);

                if (!(GetCVars()->e_ParticlesDebug & AlphaBit('q')))
                {
                    /*
                    Spherically interpolate based on rotation changes and velocity.
                    Instead of interpolating linearly along path (P0,P1,t);
                    Interpolate along an arc:

                        (P0,P1,y) + C x, where
                        a = half angle of arc segment = angle(R0,R1) / 2 = acos (R1 ~R0).w
                        C = max arc extension, perpendicular to (P0,P1)
                            = (P0,P1)/2 ^ (R1 ~R0).xyz.norm (1 - cos a)
                        y = (sin (a (2t-1)) / sin(a) + 1)/2
                        x = cos (a (2t-1)) - cos a
                    */

                    Vec3 vVelB = GetSource().GetVelocity().vLin;
                    Vec3 vDeltaAdjusted = locB.t - locA.t - vVelB * context.fUpdateTime;
                    if (!vDeltaAdjusted.IsZero())
                    {
                        Quat qDelta = locB.q * locA.q.GetInverted();
                        float fSinA = qDelta.v.GetLength();
                        if (fSinA > 0.001f)
                        {
                            Interp.bSlerp = true;
                            if (qDelta.w < 0.f)
                            {
                                qDelta = -qDelta;
                            }
                            Interp.fA = asin_tpl(fSinA);
                            Interp.fInvSinA = 1.f / fSinA;
                            Interp.fCosA = qDelta.w;
                            Interp.vArc = vDeltaAdjusted ^ qDelta.v;
                            Interp.vArc *= Interp.fInvSinA * Interp.fInvSinA * 0.5f;
                        }
                    }
                }
            }

            if (params.Connection.bConnectToOrigin)
            {
                fAge -= fAgeIncrement;
            }
            
            float fPast = fAge - fAge0, fMinPast = fAge - fAge1;
            for (; fPast - fMinPast > 0; fPast -= fAgeIncrement)
            {
                //if spawn age is greater than the end age, this particle shouldn't be spawned. Continue to time prior to this. 
                if (fAge0 + fPast > fAge1)
                {
                    continue;
                }

                if (Interp.bInterp)
                {
                    // Interpolate the location linearly.
                    data.Location.q.SetNlerp(locA.q, locB.q, Interp.fInterp);
                    data.Location.s = locA.s + (locB.s - locA.s) * Interp.fInterp;

                    if (Interp.bSlerp)
                    {
                        float fX, fY;
                        sincos_tpl(Interp.fA * (2.f * Interp.fInterp - 1.f), &fY, &fX);
                        fY = (fY * Interp.fInvSinA + 1.f) * 0.5f;
                        fX -= Interp.fCosA;

                        data.Location.t.SetLerp(locA.t, locB.t, fY);
                        data.Location.t += Interp.vArc * fX;
                    }
                    else
                    {
                        data.Location.t.SetLerp(locA.t, locB.t, Interp.fInterp);
                    }
                    Interp.fInterp += Interp.fInterpInc;
                }

                if (!EmitParticle(context, data, fPast))
                {
                    GetContainer().GetCounts().ParticlesReject += (fPast - fMinPast) * fEmitRate;
                    break;
                }
            }
            m_fLastEmitAge = fAge - fPast;
        }
        else
        {
            // Emit only once, if still valid.
            // Always emit first frame, even if lifetime < frame time.
            if (fAge <= m_fStartAge + fParticleLife + GetParticleTimer()->GetFrameTime() * 2)
            {
                for (int nEmit = int_round(fCount); nEmit > 0; nEmit--)
                {
                    if (!EmitParticle(context, data, fAge - m_fStartAge))
                    {
                        GetContainer().GetCounts().ParticlesReject += nEmit;
                        break;
                    }
                }
                m_fLastEmitAge = fHUGE;
            }
        }
    }

    SetLastLoc();
}

bool CParticleSubEmitter::GetMoveRelative(Vec3& vPreTrans, QuatTS& qtsMove) const
{
    if (m_LastLoc.s < 0.f)
    {
        return false;
    }

    vPreTrans = -m_LastLoc.t;
    const QuatTS& locCur = GetSource().GetLocation();
    qtsMove.q = locCur.q * m_LastLoc.q.GetInverted();
    qtsMove.s = m_LastLoc.s == 0.f ? 1.f : locCur.s / m_LastLoc.s;
    qtsMove.t = locCur.t;
    return true;
}

void CParticleSubEmitter::UpdateForce()
{
    FUNCTION_PROFILER_SYS(PARTICLE);

    // Set or clear physical force.

    float fAge = GetAge();
    if (fAge >= m_fStartAge && fAge <= GetParticleStopAge())
    {
        struct SForceGeom
        {
            QuatTS  qpLoc;                          // world placement of force
            AABB        bbOuter, bbInner;       // local boundaries of force.
            Vec3        vForce3;
            float       fForceW;
        } force;

        //
        // Compute force geom.
        //

        const ResourceParticleParams& params = GetParams();

        float fStrength = GetStrength();
        SPhysEnviron const& PhysEnv = GetMain().GetPhysEnviron();

        // Location.
        force.qpLoc = GetSource().GetLocation();
        force.qpLoc.s = GetMain().GetParticleScale();
        force.qpLoc.t = force.qpLoc * params.vSpawnPosOffset;

        // Direction.
        GetEmitFocusDir(force.qpLoc, fStrength, &force.qpLoc.q);

        force.qpLoc.t = force.qpLoc * params.vSpawnPosOffset;

        // Set inner box from spawn geometry.
        Quat qToLocal = force.qpLoc.q.GetInverted() * m_pSource->GetLocation().q;
        Vec3 vOffset = qToLocal * Vec3(params.vSpawnPosRandomOffset);
        force.bbInner = AABB(-vOffset, vOffset);

        // Emission directions.
        float fPhiMax = DEG2RAD(params.fEmitAngle(VMAX, fStrength)),
              fPhiMin = DEG2RAD(params.fEmitAngle(VMIN, fStrength));

        AABB bbTrav;
        bbTrav.max.y = cosf(fPhiMin);
        bbTrav.min.y = cosf(fPhiMax);
        float fCosAvg = (bbTrav.max.y + bbTrav.min.y) * 0.5f;
        bbTrav.max.x = bbTrav.max.z = (bbTrav.min.y * bbTrav.max.y < 0.f ? 1.f : sin(fPhiMax));
        bbTrav.min.x = bbTrav.min.z = -bbTrav.max.x;
        bbTrav.Add(Vec3(ZERO));

        // Force magnitude: speed times relative particle density.
        float fPLife = params.fParticleLifeTime(0.5f, fStrength);
        float fTime = fAge - m_fStartAge;

        float fSpeed = (params.fSpeed(0.5f, fStrength)) * GetMain().GetSpawnParams().fSpeedScale;
        float fDist = Travel::TravelDistance(abs(fSpeed), params.fAirResistance(0.5f, fStrength, 0.5f), min(fTime, fPLife));
        fSpeed = Travel::TravelSpeed(abs(fSpeed), params.fAirResistance(0.5f, fStrength, 0.5f), min(fTime, fPLife));
        float fForce = fSpeed * params.fAlpha(0.5f, fStrength, 0.5f, 0.0f) * force.qpLoc.s;
        {
            //user velocity is applied before other movement calculations
            Vec3 velocity = params.GetVelocityVector(fStrength, fPLife);
            float velLength = velocity.GetLengthFast();
            float speed = velLength * GetMain().GetSpawnParams().fSpeedScale;
            fDist += Travel::TravelDistance(abs(speed), params.fAirResistance(0.5f, fStrength, 0.5f), min(fTime, fPLife));
            speed = Travel::TravelSpeed(abs(speed), params.fAirResistance(0.5f, fStrength, 0.5f), min(fTime, fPLife));
            fForce += speed * params.fAlpha(0.5f, fStrength, 0.5f, 0.0f) * force.qpLoc.s;
            force.qpLoc.t += velocity.normalize();
        }
        if (params.bContinuous && fPLife > 0.f)
        {
            // Ramp up/down over particle life.
            float fStopAge = GetStopAge();
            if (fTime < fPLife)
            {
                fForce *= fTime / fPLife;
            }
            else if (fTime > fStopAge)
            {
                fForce *= 1.f - (fTime - fStopAge) / fPLife;
            }
        }

        // Force direction.
        force.vForce3.zero();
        force.vForce3.y = fCosAvg * fForce;
        force.fForceW = sqrtf(1.f - square(fCosAvg)) * fForce;

        // Travel distance.
        bbTrav.min *= fDist;
        bbTrav.max *= fDist;

        // Set outer box.
        force.bbOuter = force.bbInner;
        force.bbOuter.Augment(bbTrav);

        // Expand by size.
        //float fSize = params.fSize(0.5f, fStrength, VMAX);
        //float fSize = params.fSizeY.GetValue(fStrength, 0.5f);
        float fSize = params.fSizeY(0.5f, fStrength, VMAX);
        force.bbOuter.Expand(Vec3(fSize));

        // Scale: Normalise box size, so we can handle some geom changes through scaling.
        Vec3 vSize = force.bbOuter.GetSize() * 0.5f;
        float fRadius = max(max(vSize.x, vSize.y), vSize.z);

        if (fForce * fRadius == 0.f)
        {
            // No force.
            if (m_pForce)
            {
                GetPhysicalWorld()->DestroyPhysicalEntity(m_pForce);
                m_pForce = NULL;
            }
            return;
        }

        force.qpLoc.s *= fRadius;
        float fIRadius = 1.f / fRadius;
        force.bbOuter.min *= fIRadius;
        force.bbOuter.max *= fIRadius;
        force.bbInner.min *= fIRadius;
        force.bbInner.max *= fIRadius;

        //
        // Create physical area for force.
        //

        primitives::box geomBox;
        geomBox.Basis.SetIdentity();
        geomBox.bOriented = 0;
        geomBox.center = force.bbOuter.GetCenter();
        geomBox.size = force.bbOuter.GetSize() * 0.5f;

        pe_status_pos spos;
        if (m_pForce)
        {
            // Check whether shape changed.
            m_pForce->GetStatus(&spos);
            if (spos.pGeom)
            {
                primitives::box curBox;
                spos.pGeom->GetBBox(&curBox);
                if (!curBox.center.IsEquivalent(geomBox.center, 0.001f)
                    || !curBox.size.IsEquivalent(geomBox.size, 0.001f))
                {
                    spos.pGeom = NULL;
                }
            }
            if (!spos.pGeom)
            {
                GetPhysicalWorld()->DestroyPhysicalEntity(m_pForce);
                m_pForce = NULL;
            }
        }

        if (!m_pForce)
        {
            IGeometry* pGeom = m_pPhysicalWorld->GetGeomManager()->CreatePrimitive(primitives::box::type, &geomBox);
            m_pForce = m_pPhysicalWorld->AddArea(pGeom, force.qpLoc.t, force.qpLoc.q, force.qpLoc.s);
            if (!m_pForce)
            {
                return;
            }

            // Tag area with this emitter, so we can ignore it in the emitter family.
            pe_params_foreign_data fd;
            fd.pForeignData = (void*)&GetMain();
            fd.iForeignData = fd.iForeignFlags = 0;
            m_pForce->SetParams(&fd);
        }
        else
        {
            // Update position & box size as needed.
            if (!spos.pos.IsEquivalent(force.qpLoc.t, 0.01f)
                || !Quat::IsEquivalent(spos.q, force.qpLoc.q)
                || spos.scale != force.qpLoc.s)
            {
                pe_params_pos pos;
                pos.pos = force.qpLoc.t;
                pos.q = force.qpLoc.q;
                pos.scale = force.qpLoc.s;
                m_pForce->SetParams(&pos);
            }
        }

        // To do: 4D flow
        pe_params_area area;
        float fVMagSqr = force.vForce3.GetLengthSquared(),
              fWMagSqr = square(force.fForceW);
        float fMag = sqrtf(fVMagSqr + fWMagSqr);
        area.bUniform = (fVMagSqr > fWMagSqr) * 2;
        if (area.bUniform)
        {
            force.vForce3 *= fMag * isqrt_tpl(fVMagSqr);
        }
        else
        {
            force.vForce3.z = fMag * (force.fForceW < 0.f ? -1.f : 1.f);
            force.vForce3.x = force.vForce3.y = 0.f;
        }
        area.falloff0 = force.bbInner.GetRadius();

        if (params.eForceGeneration == params.eForceGeneration.Gravity)
        {
            area.gravity = force.vForce3;
        }
        m_pForce->SetParams(&area);

        if (params.eForceGeneration == params.eForceGeneration.Wind)
        {
            pe_params_buoyancy buoy;
            buoy.iMedium = 1;
            buoy.waterDensity = buoy.waterResistance = 1;
            buoy.waterFlow = force.vForce3;
            buoy.waterPlane.n = -PhysEnv.m_UniformForces.plWater.n;
            buoy.waterPlane.origin = PhysEnv.m_UniformForces.plWater.n * PhysEnv.m_UniformForces.plWater.d;
            m_pForce->SetParams(&buoy);
        }
    }
    else
    {
        if (m_pForce)
        {
            GetPhysicalWorld()->DestroyPhysicalEntity(m_pForce);
            m_pForce = NULL;
        }
    }
}

void CParticleSubEmitter::UpdateAudio()
{
    FUNCTION_PROFILER_SYS(PARTICLE);

    if (GetMain().GetSpawnParams().bEnableAudio && GetCVars()->e_ParticlesAudio > 0)
    {
        ResourceParticleParams const& rParams = GetParams();

        if (m_pIAudioProxy != NULL)
        {
            if (m_nAudioRtpcID != INVALID_AUDIO_CONTROL_ID && rParams.fSoundFXParam(VMAX, VMIN) < 1.0f)
            {
                float const fValue = rParams.fSoundFXParam(m_ChaosKey, GetStrength(0.0f, rParams.eSoundControlTime));
                m_pIAudioProxy->SetRtpcValue(m_nAudioRtpcID, fValue);
            }

            m_pIAudioProxy->SetPosition(GetEmitPos());
            if (m_bExecuteAudioTrigger)
            {
                m_pIAudioProxy->SetCurrentEnvironments();
                m_pIAudioProxy->ExecuteTrigger(m_nStartAudioTriggerID, eLSM_None);
            }
        }
        else
        {
            if (!rParams.sStartTrigger.empty())
            {
                Audio::AudioSystemRequestBus::BroadcastResult(m_nStartAudioTriggerID, &Audio::AudioSystemRequestBus::Events::GetAudioTriggerID, rParams.sStartTrigger.c_str());
            }

            if (!rParams.sStopTrigger.empty())
            {
                Audio::AudioSystemRequestBus::BroadcastResult(m_nStopAudioTriggerID, &Audio::AudioSystemRequestBus::Events::GetAudioTriggerID, rParams.sStopTrigger.c_str());
            }

            if (m_nStartAudioTriggerID != INVALID_AUDIO_CONTROL_ID || m_nStopAudioTriggerID != INVALID_AUDIO_CONTROL_ID)
            {
                assert(m_pIAudioProxy == NULL);
                Audio::AudioSystemRequestBus::BroadcastResult(m_pIAudioProxy, &Audio::AudioSystemRequestBus::Events::GetFreeAudioProxy);

                if (m_pIAudioProxy != NULL)
                {
                    m_pIAudioProxy->Initialize("ParticleSubEmitter");
                    m_pIAudioProxy->SetObstructionCalcType(Audio::eAOOCT_SINGLE_RAY);

                    if (!GetMain().GetSpawnParams().sAudioRTPC.empty())
                    {
                        Audio::AudioSystemRequestBus::BroadcastResult(m_nAudioRtpcID, &Audio::AudioSystemRequestBus::Events::GetAudioRtpcID, GetMain().GetSpawnParams().sAudioRTPC.c_str());
                        if (m_nAudioRtpcID != INVALID_AUDIO_CONTROL_ID)
                        {
                            float const fValue = rParams.fSoundFXParam(m_ChaosKey, GetStrength(0.0f, rParams.eSoundControlTime));
                            m_pIAudioProxy->SetRtpcValue(m_nAudioRtpcID, fValue);
                        }
                    }

                    // Execute start trigger immediately.
                    if (m_nStartAudioTriggerID != INVALID_AUDIO_CONTROL_ID)
                    {
                        m_pIAudioProxy->SetPosition(GetEmitPos());
                        m_pIAudioProxy->SetCurrentEnvironments();
                        m_pIAudioProxy->ExecuteTrigger(m_nStartAudioTriggerID, eLSM_None);
                    }
                }
            }
        }
    }
    else if (m_pIAudioProxy != NULL)
    {
        DeactivateAudio();
    }

    m_bExecuteAudioTrigger = false;
}


int CParticleSubEmitter::EmitParticle(SParticleUpdateContext& context, const EmitParticleData& data_in, float fAge, QuatTS* plocPreTransform)
{
    const ResourceParticleParams& params = GetParams();

    EmitParticleData data = data_in;

    // Determine geometry
    if (!data.pStatObj)
    {
        data.pStatObj = params.pStatObj;
    }
    if (data.pStatObj && !data.pPhysEnt && params.eGeometryPieces != params.eGeometryPieces.Whole)
    {
        // Emit sub-meshes rather than parent mesh.
        IStatObj* pParentObj = data.pStatObj;
        QuatTS* plocMainPreTransform = plocPreTransform;

        int nPieces = 0;
        for (int i = pParentObj->GetSubObjectCount() - 1; i >= 0; i--)
        {
            if (IStatObj::SSubObject* pSub = GetSubGeometry(pParentObj, i))
            {
                if (params.eGeometryPieces == params.eGeometryPieces.AllPieces || (nPieces++ == 0 || cry_random(0, nPieces - 1) == 0))
                {
                    // Emit this piece
                    data.pStatObj = pSub->pStatObj;

                    QuatTS locPreTransform;
                    if (params.bNoOffset)
                    {
                        // Use piece's sub-location
                        locPreTransform = QuatTS(pSub->localTM);
                        if (plocMainPreTransform)
                        {
                            locPreTransform = *plocMainPreTransform * locPreTransform;
                        }
                        plocPreTransform = &locPreTransform;
                    }
                    if (params.eGeometryPieces == params.eGeometryPieces.AllPieces)
                    {
                        // Emit each piece as a separate particle
                        nPieces += EmitParticle(context, data, fAge, plocPreTransform);
                    }
                }
            }
        }
        if (params.eGeometryPieces == params.eGeometryPieces.AllPieces && nPieces > 0)
        {
            return nPieces;
        }
    }

    m_nEmitIndex++;

    if (params.GetEmitterShape() == ParticleParams::EEmitterShapeType::BEAM)
    {
        return SpawnBeamEmitter(params, context, fAge, data);
    }
    else
    {
        return SpawnParticleToContainer(params, context, fAge, data, plocPreTransform, 1.f);
    }
}

int CParticleSubEmitter::SpawnBeamEmitter(const ResourceParticleParams& params, SParticleUpdateContext& context, float fAge, EmitParticleData data)
{
    // Init local particle, copy to container. Use raw data buffer, to avoid extra destructor.
    //instead of spawning a single particle we spawn segmentCount (or total length/segmentLength rounded up) particles
    // Immediately spawn particles, otherwise the renderer will try to render nothing and crash in the process
    float fEmissionStrength = GetStrength(-fAge);
    data.fBeamAge = params.fBeamAge(VRANDOM, GetStrength(-fAge));
    Matrix34 segmentTransformMatrix = Matrix34::CreateIdentity();
    Vec3 spawnPos = params.GetEmitterOffset(context.vEmitBox, context.vEmitScale, fEmissionStrength, params.vSpawnPosOffset, params.vSpawnPosRandomOffset);
    Vec3 targetPos = params.GetEmitterOffset(context.vEmitBox, context.vEmitScale, fEmissionStrength, params.vTargetPosition, params.vTargetRandOffset);
    segmentTransformMatrix.AddTranslation(spawnPos);
    Vec3 segmentSpawnStep;
    Vec3 totalOffsetOfBeam;
    Vec3 segmentUpVector = params.vBeamUpVector;
    segmentUpVector.Normalize();
    segmentSpawnStep.zero();
    CPNoise3 noise;
    noise.SetSeedAndReinitialize(rand());
    unsigned int finalCount = BEAM_COUNT_CORRECTION;
    //A tax of using trail emitter system. 
    //calculate proper steps and count
    totalOffsetOfBeam = targetPos - spawnPos;
    float dist = totalOffsetOfBeam.GetLength();
    float uvOffset = 0.f;
    bool isTexturePerStream = params.Connection.eTextureMapping == ParticleParams::SConnection::ETextureMapping::PerStream;
    if (params.eSegmentType == ParticleParams::ESegmentType::FIXED && params.fSegmentCount != 0)
    {
        segmentSpawnStep = totalOffsetOfBeam / params.fSegmentCount;
        finalCount += static_cast<unsigned int>(ceil(params.fSegmentCount)); //overestimate the count, we trim the final quad later.
        uvOffset = params.fSegmentCount - static_cast<float>(ceil(params.fSegmentCount));
    }
    else if (params.eSegmentType == ParticleParams::ESegmentType::LENGTH && params.fSegmentLength != 0)
    {
        float preciseCount = dist / params.fSegmentLength;
        unsigned int nextCountUp = static_cast<unsigned int>(ceil(preciseCount));
        segmentSpawnStep = totalOffsetOfBeam.normalized() * params.fSegmentLength;
        finalCount += nextCountUp; //overestimate the count, we trim the final quad later.
        uvOffset = preciseCount - static_cast<float>(nextCountUp);
    }
    Vec3 currentPos = spawnPos;
    Vec3 originalSegmentStep = segmentSpawnStep;
    for (unsigned int i = 0; i < finalCount; i++)
    {
        //move half a step before and after to get the beam edge to hit the target and origin
        bool isEdgeParticle = (i == 0) || (finalCount - 1 == i);
        currentPos = (static_cast<float>(i)*segmentSpawnStep) + spawnPos; //discard first iteration since it simply starts the beam and does not move.
        if (currentPos.GetSquaredDistance(spawnPos) > targetPos.GetSquaredDistance(spawnPos))
        {
            segmentSpawnStep = targetPos - (currentPos - segmentSpawnStep);
            if (segmentSpawnStep.len2() <= AZ_FLT_EPSILON)
            {
                //add an epsilon in the original direction, this is to fix the orientation of the last quad.
                segmentSpawnStep = originalSegmentStep * AZ_FLT_EPSILON;
            }
            currentPos = targetPos;
        }
        float segmentOffset = 0.f;
        if (isTexturePerStream || finalCount - 1 == i)
        {
            segmentOffset = uvOffset;
        }
        WalkAlongWaveForm(params, segmentTransformMatrix, static_cast<float>(i), static_cast<float>(finalCount), dist, segmentSpawnStep, spawnPos, segmentUpVector, noise, params.fTangent(VRANDOM, GetRelativeAge()));
        QuatTS plocSegmentPreTransform(segmentTransformMatrix);
        SpawnParticleToContainer(params, context, fAge, data, &plocSegmentPreTransform, segmentOffset, finalCount - BEAM_COUNT_CORRECTION, segmentSpawnStep, isEdgeParticle);
    }
    return static_cast<int>(finalCount); // there is a two particle tax to using trail particles these are not rendered.
}

void CParticleSubEmitter::WalkAlongWaveForm(const ResourceParticleParams& params, Matrix34& mat, const float& currentStep, const float& totalSteps, float totalLength, const Vec3& segmentSpawnStep, const Vec3& FinalOrigin, const Vec3& NormalizedUp, CPNoise3& noise, const float& tangent)
{
    if (totalSteps == 0 || currentStep == 0)
    {
        return;
    }

    if (params.fAmplitude == 0 || params.fFrequency == 0)
    {
        mat.AddTranslation(Vec3(segmentSpawnStep.x, segmentSpawnStep.y, segmentSpawnStep.z));
        return;
    }

    switch (params.eBeamWaveType)
    {
    case ResourceParticleParams::EBeamWaveType::SINE:
    {
        //y = amp*sin(freq*x + offset);
        //the offset = acos(slope/amp)/freq
        //distance to target location
        float tangentLocation = params.eBeamWaveTangentSource == ParticleParams::EBeamWaveTangentSource::ORIGIN ? 0 : totalLength;
        float offset = DEG2RAD(fmodf(tangent, 360.0f)) / params.fFrequency + tangentLocation;
        float currentX = ((currentStep / totalSteps) * totalLength);
        float lastX = (((currentStep - 1.f) / totalSteps) * totalLength);
        float currentY = params.fAmplitude * sin(params.fFrequency * currentX + offset);
        float lastY = params.fAmplitude * sin(params.fFrequency * lastX + offset);
        mat.AddTranslation(Vec3(segmentSpawnStep.x, segmentSpawnStep.y, segmentSpawnStep.z) + (NormalizedUp * (currentY - lastY)));
    }
    break;
    case ResourceParticleParams::EBeamWaveType::SQUARE:
    {
        //Limitations
        //1) user can enter length or a fixed number either way removes the ability to ensure right angle corners
        //1.a) we may need to remove users ability to select segment length/count/type while square wave is used and just plot particles at the end points where they are needed.
        //1.b) tangent is mathematically invalid for this wave type, we use the sin's tangent to give some control to the user instead.
        float tangentLocation = params.eBeamWaveTangentSource == ParticleParams::EBeamWaveTangentSource::ORIGIN ? 0 : totalLength;
        float offset = DEG2RAD(fmodf(tangent, 360.0f)) / params.fFrequency + tangentLocation;
        float currentX = offset + ((currentStep / totalSteps) * totalLength);
        float lastX = offset + (((currentStep - 1.f) / totalSteps) * totalLength);
        float fAmplitude = params.fAmplitude;
        float currentY = (fAmplitude * sin(params.fFrequency * currentX)) > 0 ? fAmplitude : -fAmplitude;
        float lastY = (fAmplitude * sin(params.fFrequency * lastX)) > 0 ? fAmplitude : -fAmplitude;
        mat.AddTranslation(Vec3(segmentSpawnStep.x, segmentSpawnStep.y, segmentSpawnStep.z) + (NormalizedUp * (currentY - lastY)));
    }
    break;
    case ResourceParticleParams::EBeamWaveType::NOISE:
    {
        //Limitations
        //1.b) tangent is mathematically invalid for this wave type, it is designed to be unpredictable.
        float tangentLocation = params.eBeamWaveTangentSource == ParticleParams::EBeamWaveTangentSource::ORIGIN ? 0 : totalLength;
        float offset = 0;
        float currentX = offset + ((currentStep / totalSteps) * totalLength);
        float lastX = offset + (((currentStep - 1.f) / totalSteps) * totalLength);
        float currentY = noise.Noise1D(currentX * params.fFrequency) * params.fAmplitude;
        float lastY = noise.Noise1D(lastX * params.fFrequency) * params.fAmplitude;
        mat.AddTranslation(Vec3(segmentSpawnStep.x, segmentSpawnStep.y, segmentSpawnStep.z) + (NormalizedUp * (currentY - lastY)));
    }
    break;
    default:
        mat.AddTranslation(Vec3(segmentSpawnStep.x, segmentSpawnStep.y, segmentSpawnStep.z));
        break;
    }
}

void CParticleSubEmitter::GetEmitterBounds(Vec3& emitterSize) const
{
    emitterSize.CheckMin(m_emitterBounds);
}

void CParticleSubEmitter::UpdateEmitterBounds()
{
    //Called once per update to update the bounds
    ResourceParticleParams const& params = GetParams();
    float emitterAge = GetRelativeAge(m_fStartAge);
    //bounds are 1/2 of total. (projected from center)
    SFloat x;
    SFloat y;
    SFloat z;
    switch (params.GetEmitterShape())
    {
        case ParticleParams::EEmitterShapeType::BOX:
        {
            x = params.vEmitterSizeXYZ.fX * 0.5f;
            y = params.vEmitterSizeXYZ.fY * 0.5f;
            z = params.vEmitterSizeXYZ.fZ * 0.5f;
            break;
        }
        case ParticleParams::EEmitterShapeType::CIRCLE:
        {
            x = y = params.fEmitterSizeDiameter * 0.5f;
            z = fHUGE;
        }
        case ParticleParams::EEmitterShapeType::SPHERE:
        {
            x = y = z = params.fEmitterSizeDiameter * 0.5f;
            break;
        }
        default:
        {
            x = y = z = fHUGE;
            break;
        }
    }
    m_emitterBounds = Vec3(x, y, z);
}

int CParticleSubEmitter::SpawnParticleToContainer(const ResourceParticleParams& params, SParticleUpdateContext& context, float fAge, EmitParticleData data, QuatTS* plocPreTransform, const float uvOffset, uint segmentCount /*= 0*/, Vec3 segmentStep /*= Vec3(0.f)*/, const bool isEdgeParticle /*= false */)
{
    char buffer[sizeof(CParticle)] _ALIGN(128);
    CParticle* pPart = new(buffer)CParticle;

    pPart->Init(context, fAge, this, data);
    pPart->SetBeamInfo(segmentCount, segmentStep, uvOffset, isEdgeParticle);
    if (plocPreTransform)
    {
        pPart->Transform(*plocPreTransform);
    }

    CParticle* pNewPart = GetContainer().AddParticle(context, *pPart);
    if (!pNewPart)
    {
        pPart->~CParticle();
        return 0;
    }
    if (GetContainer().GetChildFlags())
    {
        // Create child emitters (on actual particle address, not pPart)
        GetMain().CreateIndirectEmitters(pNewPart, &GetContainer());
    }
    return 1;
}

Vec3 CParticleSubEmitter::GetEmitPos() const
{
    return GetSource().GetLocation() * GetParams().vSpawnPosOffset;
}

Vec3 CParticleSubEmitter::GetEmitFocusDir(const QuatTS& loc, float fStrength, Quat* pRot) const
{
    const ParticleParams& params = GetParams();

    Quat q = loc.q;
    // vFocus = vector to deviate from default up vector for particle emission direction.
    Vec3 vFocus = loc.q.GetColumn2();

    if (params.bFocusGravityDir)
    {
        float fLenSq = GetMain().GetPhysEnviron().m_UniformForces.vAccel.GetLengthSquared();
        if (fLenSq > FLT_MIN)
        {
            vFocus = GetMain().GetPhysEnviron().m_UniformForces.vAccel * -isqrt_tpl(fLenSq);
            if (params.fFocusAngle || pRot)
            {
                RotateToUpVector(q, vFocus);
            }
        }
    }

    if (params.fFocusCameraDir)
    {
        float fBias = params.fFocusCameraDir(VRANDOM, fStrength);
        Vec3 vCamDir = (gEnv->p3DEngine->GetRenderingCamera().GetPosition() - loc.t).GetNormalized();
        vFocus.SetSlerp(vFocus, vCamDir, fBias);
        if (params.fFocusAngle || pRot)
        {
            RotateToUpVector(q, vFocus);
        }
    }

    if (params.fFocusAngle)
    {
        float fAngle = DEG2RAD(params.fFocusAngle(m_ChaosKey, fStrength));
        float fAzimuth = DEG2RAD(params.fFocusAzimuth(m_ChaosKey, fStrength));
        q = q * Quat::CreateRotationXYZ(Ang3(fAngle, 0, fAzimuth));
        // vFocus = vector to deviate from default up vector for particle emission direction.
        vFocus = q.GetColumn2();
    }

    if (pRot)
    {
        *pRot = q;
    }

    return vFocus;
}

void CParticleSubEmitter::DeactivateAudio()
{
    if (m_pIAudioProxy != nullptr)
    {
        m_pIAudioProxy->SetPosition(GetEmitPos());

        if (m_nAudioRtpcID != INVALID_AUDIO_CONTROL_ID)
        {
            ResourceParticleParams const& rParams = GetParams();
            float const fValue = rParams.fSoundFXParam(m_ChaosKey, GetStrength(0.0f, rParams.eSoundControlTime));
            m_pIAudioProxy->SetRtpcValue(m_nAudioRtpcID, fValue);
        }

        if (m_nStopAudioTriggerID != INVALID_AUDIO_CONTROL_ID)
        {
            m_pIAudioProxy->ExecuteTrigger(m_nStopAudioTriggerID, eLSM_None);
        }
        else
        {
            assert(m_nStartAudioTriggerID != INVALID_AUDIO_CONTROL_ID);
            m_pIAudioProxy->StopTrigger(m_nStartAudioTriggerID);
        }

        m_pIAudioProxy->Release();
        m_nStartAudioTriggerID  = INVALID_AUDIO_CONTROL_ID;
        m_nStopAudioTriggerID       = INVALID_AUDIO_CONTROL_ID;
        m_nAudioRtpcID                  = INVALID_AUDIO_CONTROL_ID;
        m_pIAudioProxy = nullptr;
    }
}

float CParticleSubEmitter::ComputeDensityIncrease(float fStrength, float fParticleLife, const QuatTS& locA, const QuatTS* plocB) const
{
    // Increase emission rate to compensate for moving emitter and non-uniform forces.
    const ResourceParticleParams& params = GetParams();
    SPhysEnviron const& PhysEnv = GetMain().GetPhysEnviron();

    AABB bbSource = params.GetEmitOffsetBounds();

    FStaticBounds opts;
    opts.fSpeedScale = GetMain().GetSpawnParams().fSpeedScale;
    opts.fMaxLife = fParticleLife;

    FEmitterFixed fixed = { m_ChaosKey, fStrength };
    float fSize = params.fSize(0.5f, fStrength, VMAX) * 2.f;
    float fGravityScale = params.fGravityScale(0.5f, fStrength, 0.5f);

    // Get emission volume for default orientation and forces
    SForceParams forces;
    static_cast<SPhysForces&>(forces) = PhysEnv.m_UniformForces;
    forces.vAccel = forces.vAccel * fGravityScale + params.vAcceleration;
    forces.vWind.zero();
    forces.fDrag = params.fAirResistance(0.5f, fStrength, 0.5f);
    forces.fStretch = 0.f;

    AABB bbTravel(AABB::RESET);
    float fDist = params.GetTravelBounds(bbTravel, IParticleEffect::ParticleLoc(Vec3(ZERO)), forces, opts, fixed);
    float fStandardVolume = Travel::TravelVolume(bbSource, bbTravel, fDist, fSize);

    // Get emission volume for current forces, in local space
    Quat qToA = locA.q.GetInverted();

    PhysEnv.GetForces(forces, GetSource().GetLocation().t, GetContainer().GetEnvironmentFlags());
    forces.vAccel = qToA * forces.vAccel * fGravityScale + params.vAcceleration;
    forces.vWind = qToA * forces.vWind * params.fAirResistance.fWindScale;

    fDist = params.GetTravelBounds(bbTravel, QuatTS(IDENTITY), forces, opts, fixed);
    if (plocB && locA.s > FLT_EPSILON)
    {
        // Add previous emission from previous location, in local space
        QuatTS locToA = locA.GetInverted() * *plocB;
        bbSource.Add(AABB::CreateTransformedAABB(QuatT(locToA), bbSource));
        AABB bbTravelB;
        float fDistB = params.GetTravelBounds(bbTravelB, locToA, forces, opts, fixed);
        fDist = max(fDist, fDistB);
        bbTravel.Add(bbTravelB);
    }
    float fVolume = Travel::TravelVolume(bbSource, bbTravel, fDist, fSize);

    return div_min(fVolume, fStandardVolume, fMAX_DENSITY_ADJUST);
}
