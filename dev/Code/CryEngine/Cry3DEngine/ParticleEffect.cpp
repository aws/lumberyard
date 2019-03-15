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
#include "ParticleEffect.h"
#include "ParticleEmitter.h"
#include "ParticleManager.h"
#include <IStatoscope.h>
#include <IAudioSystem.h>

#include <StringUtils.h>
//////////////////////////////////////////////////////////////////////////
// TypeInfo XML serialisation code

#include "ParticleParams_TypeInfo.h"
#include <AzCore/std/string/conversions.h>

DEFINE_INTRUSIVE_LINKED_LIST(CParticleEffect)
#define LOD_DIST_OFFSET  10
#define WIDTH_TO_HALF_WIDTH 0.5f

//////////////////////////////////////////////////////////////////////////
// IMPORTANT: Remember to update nSERIALIZE_VERSION if you update the
// particle system
//////////////////////////////////////////////////////////////////////////
static const int nSERIALIZE_VERSION = 33;
static const int nMIN_SERIALIZE_VERSION = 19;

// Write a struct to an XML node.
template<class T>
void ToXML(IXmlNode& xml, T& val, const T& def_val, FToString flags)
{
    CTypeInfo const& info = TypeInfo(&val);
    for AllSubVars(pVar, info)
    {
        if (pVar->GetElemSize() == 0)
        {
            continue;
        }

        if (&pVar->Type == &TypeInfo((void**)0))
        {
            // Never serialize pointers.
            continue;
        }

        assert(pVar->GetDim() == 1);
        cstr name = pVar->GetName();

        if (name[0] == '_')
        {
            // Do not serialize private vars.
            continue;
        }

        string str = pVar->ToString(&val, flags, &def_val);
        if (flags.SkipDefault && str.length() == 0)
        {
            continue;
        }

        xml.setAttr(name, str);
    }
}

// Read a struct from an XML node.
template<class T>
void FromXML(IXmlNode& xml, T& val, FFromString flags)
{
    CTypeInfo const& info = TypeInfo(&val);
    int nAttrs = xml.getNumAttributes();
    for (int a = 0; a < nAttrs; a++)
    {
        cstr sKey, sVal;
        xml.getAttributeByIndex(a, &sKey, &sVal);

        CTypeInfo::CVarInfo const* pVar = info.FindSubVar(sKey);
        if (pVar)
        {
            assert(pVar->GetDim() == 1);
            pVar->FromString(&val, sVal, flags);
        }
    }
}

template<class T>
bool GetAttr(IXmlNode const& node, cstr attr, T& val)
{
    cstr sAttr = node.getAttr(attr);
    return *sAttr && TypeInfo(&val).FromString(&val, sAttr, FFromString().SkipEmpty(1));
}

template<class T>
T GetAttrValue(IXmlNode const& node, cstr attr, T defval)
{
    GetAttr(node, attr, defval);
    return defval;
}

template<class T>
void SetParamRange(TVarParam<T>& param, float fValMin, float fValMax)
{
    if (abs(fValMin) > abs(fValMax))
    {
        std::swap(fValMin, fValMax);
    }
    if (fValMax != 0.f)
    {
        param.Set(fValMax, (fValMax - fValMin) / fValMax);
    }
    else
    {
        param.Set(0.f, 0.f);
    }
}

template<class T>
void AdjustParamRange(TVarParam<T>& param, float fAdjust)
{
    float   fValMax = param(VMAX) + fAdjust,
            fValMin = param(VMIN) + fAdjust;
    SetParamRange(param, fValMin, fValMax);
}

//////////////////////////////////////////////////////////////////////////
// ResourceParticleParams implementation

static const float fTRAVEL_SAFETY = 0.1f;
static const float fSIZE_SAFETY = 0.1f;

struct SEmitParams
{
    Vec3    vAxis;
    float   fCosMin, fCosMax;
    float fSpeedMin, fSpeedMax;

    void SetOmniDir()
    { fCosMin = -1.f; fCosMax = 1.f; }
    bool bOmniDir() const
    { return fCosMin <= -1.f && fCosMax >= 1.f; }
    bool bSingleDir() const
    { return fCosMin >= 1.f; }
};

namespace Travel
{
    float TravelDistanceApprox(const Vec3& vVel0, float fTime, const SForceParams& forces)
    {
        if (fTime <= 0.f)
        {
            return 0.f;
        }

        Vec3 vVel = vVel0;

        // Check if trajectory has a speed minimum
        float dt[3] = { fTime, 0.f, 0.f };
        float s[5] = { vVel.GetLengthFast() };
        int nS = 1;

        if (forces.fDrag * fTime >= fDRAG_APPROX_THRESHOLD)
        {
            float fInvDrag = 1.f / forces.fDrag;
            Vec3 vTerm = forces.vWind + forces.vAccel * fInvDrag;
            /*
                s^2 = (V d + VT (1 - d))^2                  ; d = e^(-b t)
                   = (VT + (V-VT) e^(-b t))^2
                     = VT^2 + 2 VT*(V-VT) e^(-b t) + (V-VT)^2 e^(-2b t)
                s^2\t = -b 2 VT*(V-VT) e^(-b t) - 2b (V-VT)^2 e^(-2b t) = 0
                -VT*(V-VT) = (V-VT)^2 e^(-b t)
                e^(-b t) = -VT*(V-VT) / (V-VT)^2
                t = -log ( VT*(VT-V) / (VT-V)^2) / b
            */
            Vec3 vDV = vTerm - vVel;
            float fDD = vDV.len2(), fTD = vTerm * vDV;
            if (fDD * fTD > 0.f)
            {
                float fT = -logf(fTD / fDD) * fInvDrag;
                if (fT > 0.f && fT < fTime)
                {
                    dt[0] = fT;
                    dt[1] = fTime - fT;
                }
            }

            for (int t = 0; dt[t] > 0.f; t++)
            {
                float fDecay = 1.f - expf(-forces.fDrag * dt[t] * 0.5f);
                for (int i = 0; i < 2; i++)
                {
                    vVel = Lerp(vVel, vTerm, fDecay);
                    s[nS++] = vVel.GetLengthFast();
                }
            }
        }
        else
        {
            // Fast approx drag computation.
            Vec3 vAccel = forces.vAccel + (forces.vWind - vVel) * forces.fDrag;
            float fVA = vVel * vAccel,
                  fAA = vAccel * vAccel;
            if (fVA * fAA < 0.f && -fVA < fTime * fAA)
            {
                dt[0] = -fVA / fAA;
                dt[1] = fTime - dt[0];
            }

            for (int t = 0; dt[t] > 0.f; t++)
            {
                Vec3 vD = forces.vAccel * (dt[t] * 0.5f);
                for (int i = 0; i < 2; i++)
                {
                    vVel += vD;
                    s[nS++] = vVel.GetLengthFast();
                }
            }
        }

        if (nS == 5)
        {
            // 2-segment quadratic approximation
            return ((s[0] + s[1] * 4.f + s[2]) * dt[0]
                    + (s[2] + s[3] * 4.f + s[4]) * dt[1]) * 0.16666666f;
        }
        else
        {
            // Single segment quadratic approximation
            return (s[0] + s[1] * 4.f + s[2]) * 0.16666666f * dt[0];
        }
    }

    float TravelVolume(const AABB& bbSource, const AABB& bbTravel, float fDist, float fSize)
    {
        Vec3 V = bbSource.GetSize() + bbTravel.GetSize() + Vec3(fSize);
        Vec3 T = bbTravel.GetCenter().abs().GetNormalized() * fDist;

        return V.x * V.y * V.z  +  V.x * V.y * T.z  +  V.x * T.y * V.z  +  T.x * V.y * V.z;
    }

    void AddTravelVec(AABB& bb, Vec3 vVel, SForceParams const& force, float fTime)
    {
        Vec3 vTravel(ZERO);
        Travel(vTravel, vVel, fTime, force);
        vTravel += vVel * force.fStretch;
        bb.Add(vTravel);
    }

    void AddTravel(AABB& bb, Vec3 const& vVel, SForceParams const& force, float fTime, int nAxes)
    {
        if (force.fStretch != 0.f)
        {
            // Add start point
            bb.Add(vVel * force.fStretch);
        }

        // Add end point.
        AddTravelVec(bb, vVel, force, fTime);

        // Find time of min/max vals of each component, by solving for v[i] = 0
        if (nAxes && force.fDrag != 0)
        {
            // vt = a/d + w
            // v = (v0-vt) e^(-d t) + vt
            // vs = v + v\t s
            //      = (1 - d s)(v0-vt) e^(-d t) + vt
            //      = 0
            // e^(-d t) = vt / ((1 - d s)(vt-v0))
            // t = -log( vt / ((1 - d s)(vt-v0)) ) / d
            float fInvDrag = 1.f / force.fDrag;
            for (int i = 0; nAxes; i++, nAxes >>= 1)
            {
                if (nAxes & 1)
                {
                    float fVT = force.vAccel[i] * fInvDrag + force.vWind[i];
                    float d = (fVT - vVel[i]) * (1.f - force.fDrag * force.fStretch);
                    if (fVT * d > 0.f)
                    {
                        float fT = -logf(fVT / d) * fInvDrag;
                        if (fT > 0.f && fT < fTime)
                        {
                            AddTravelVec(bb, vVel, force, fT);
                        }
                    }
                }
            }
        }
        else
        {
            for (int i = 0; nAxes; i++, nAxes >>= 1)
            {
                if (nAxes & 1)
                {
                    if (force.vAccel[i] != 0.f)
                    {
                        // ps = p + v s
                        // vs = p\t + v\t s
                        //      = v + a s
                        // vs = 0
                        //      = v0 + a (t+s)
                        // t = -v0/a - s
                        float fT = -vVel[i] / force.vAccel[i] - force.fStretch;
                        if (fT > 0.f && fT < fTime)
                        {
                            AddTravelVec(bb, vVel, force, fT);
                        }
                    }
                }
            }
        }
    }

    Vec3 GetExtremeEmitVec(Vec3 const& vRefDir, SEmitParams const& emit)
    {
        float fEmitCos = vRefDir * emit.vAxis;
        if (fEmitCos >= emit.fCosMin && fEmitCos <= emit.fCosMax)
        {
            // Emit dir is in the cone.
            return vRefDir * emit.fSpeedMax;
        }
        else
        {
            // Find dir in emission cone closest to ref dir.
            Vec3 vEmitPerpX = vRefDir - emit.vAxis * fEmitCos;
            float fPerpLenSq = vEmitPerpX.GetLengthSquared();

            float fCos = clamp_tpl(fEmitCos, emit.fCosMin, emit.fCosMax);
            Vec3 vEmitMax = emit.vAxis * fCos + vEmitPerpX * sqrt_fast_tpl((1.f - fCos * fCos) / (fPerpLenSq + FLT_MIN));
            vEmitMax *= if_pos_else(vEmitMax * vRefDir, emit.fSpeedMin, emit.fSpeedMax);
            return vEmitMax;
        }
    }

    void AddEmitDirs(AABB& bb, Vec3 const& vRefDir, SEmitParams const& emit, SForceParams const& force, float fTime, int nAxes)
    {
        Vec3 vEmit = GetExtremeEmitVec(vRefDir, emit);
        AddTravel(bb, vEmit, force, fTime, nAxes);
        vEmit = GetExtremeEmitVec(-vRefDir, emit);
        AddTravel(bb, vEmit, force, fTime, nAxes);
    }

    inline float GetSinMax(float fCosMin, float fCosMax)
    {
        return fCosMin * fCosMax < 0.f ? 1.f : sqrtf(1.f - min(fCosMin * fCosMin, fCosMax * fCosMax));
    }

    inline float MaxComponent(Vec3 const& v)
    {
        return max(max(abs(v.x), abs(v.y)), abs(v.z));
    }

    // Compute bounds of a cone of emission, with applied gravity.
    void TravelBB(AABB& bb, SEmitParams const& emit, SForceParams const& force, float fTime, int nAxes)
    {
        if (emit.fSpeedMax == 0.f)
        {
            AddTravel(bb, Vec3(0), force, fTime, nAxes);
            return;
        }
        else if (emit.bSingleDir())
        {
            AddTravel(bb, emit.vAxis * emit.fSpeedMax, force, fTime, nAxes);
            if (emit.fSpeedMin != emit.fSpeedMax)
            {
                AddTravel(bb, emit.vAxis * emit.fSpeedMin, force, fTime, nAxes);
            }
            return;
        }

        // First expand box from emission in cardinal directions.
        AddEmitDirs(bb, Vec3(1, 0, 0), emit, force, fTime, nAxes & 1);
        AddEmitDirs(bb, Vec3(0, 1, 0), emit, force, fTime, nAxes & 2);
        AddEmitDirs(bb, Vec3(0, 0, 1), emit, force, fTime, nAxes & 4);

        // Add extreme dirs along gravity.
        Vec3 vDir;
        if (CheckNormalize(vDir, force.vAccel))
        {
            if (MaxComponent(vDir) < 0.999f)
            {
                AddEmitDirs(bb, vDir, emit, force, fTime, 0);
            }
        }

        // And wind.
        if (force.fDrag > 0.f && CheckNormalize(vDir, force.vWind))
        {
            if (MaxComponent(vDir) < 0.999f)
            {
                AddEmitDirs(bb, vDir, emit, force, fTime, 0);
            }
        }
    }
};

void ResourceParticleParams::GetStaticBounds(AABB& bb, const QuatTS& loc, const SPhysForces& forces, const FStaticBounds& opts) const
{
    // Compute spawn source box.
    bb = GetEmitOffsetBounds();

    //for Beam particle, need to add the target position and wave amplitude for the static bounding box
    if (GetEmitterShape() == EEmitterShapeType::BEAM)
    {
        bb.Add(vTargetPosition + vTargetRandOffset);
        bb.Add(vTargetPosition - vTargetRandOffset);

        Vec3 segmentUpVector = vBeamUpVector;
        segmentUpVector.Normalize();
        bb.Add(segmentUpVector * fAmplitude);
        bb.Add(-segmentUpVector* fAmplitude);
    }

    if (bSpaceLoop && bBindEmitterToCamera)
    {
        // Use CameraMaxDistance as additional space loop size.
        bb.max.y = max(bb.max.y, +fCameraMaxDistance);
    }

    bb.SetTransformedAABB(Matrix34(loc), bb);
    bb.Expand(opts.vSpawnSize);

    if (!bSpaceLoop)
    {
        AABB bbTrav(0.f);
        GetMaxTravelBounds(bbTrav, loc, forces, opts);

        // Expand by a safety factor.
        bbTrav.min *= (1.f + fTRAVEL_SAFETY);
        bbTrav.max *= (1.f + fTRAVEL_SAFETY);

        bb.Augment(bbTrav);
    }

    if (eFacing == eFacing.Water && HasWater(forces.plWater))
    {
        // Move to water plane
        float fDist0 = bb.min.z - forces.plWater.DistFromPlane(bb.min);
        float fDistD = abs(forces.plWater.n.x * (bb.max.x - bb.min.x)) + abs(forces.plWater.n.y * (bb.max.y - bb.min.y));
        bb.min.z = fDist0 - fDistD;
        bb.max.z = fDist0 + fDistD;
    }

    // Particle size.
    if (opts.bWithSize)
    {
        float fMaxSize = GetMaxVisibleSize();
        fMaxSize *= loc.s * (1.f + fSIZE_SAFETY);
        bb.Expand(Vec3(fMaxSize));
    }
    if (eMoveRelEmitter)
    {
        // Expand a bit for constant movement inaccuracy.
        bb.Expand(Vec3(0.01f));
    }
}

float ResourceParticleParams::GetMaxVisibleSize() const
{
    float fMaxSize = fSizeY.GetMaxValue() * GetMaxObjectSize(pStatObj);
    if (LightSource.fIntensity)
    {
        fMaxSize = max(fMaxSize, LightSource.fRadius.GetMaxValue());
    }
    fMaxSize += abs(fCameraDistanceOffset);
    return fMaxSize;
}

template<class Var>
void ResourceParticleParams::GetEmitParams(SEmitParams& emit, const QuatTS& loc, const SPhysForces& forces, const FStaticBounds& opts, const Var& var) const
{
    emit.vAxis = bFocusGravityDir ? -forces.vAccel.GetNormalizedSafe(Vec3(0, 0, -1)) : loc.q.GetColumn2();

    // Max sin and cos of emission relative to emit dir.
    if (fFocusCameraDir)
    {
        // Focus can point in any direction.
        emit.SetOmniDir();
    }
    else if (fFocusAngle)
    {
        // Incorporate focus variation into min/max cos.
        float fAngleMax = DEG2RAD(fFocusAngle(var.RMax(), var.EMax())), fAngleMin = DEG2RAD(fFocusAngle(var.RMin(), var.EMin()));
        float fAzimuthMax = DEG2RAD(fFocusAzimuth(var.RMax(), var.EMax())), fAzimuthMin = DEG2RAD(fFocusAzimuth(var.RMin(), var.EMin()));

        emit.fCosMax = cosf(DEG2RAD(fEmitAngle(VMIN, var.EMin())));
        emit.fCosMin = cosf(min(opts.fAngMax + DEG2RAD(fEmitAngle(VMAX, var.EMax())) + (fAngleMax - fAngleMin + fAzimuthMax - fAzimuthMin) * 0.5f, gf_PI));

        // Rotate focus about axis.
        Quat rotated = (loc.q * Quat::CreateRotationXYZ(Ang3((fAngleMin + fAngleMax) * 0.5f, 0, (fAzimuthMin + fAzimuthMax) * 0.5f)));
        emit.vAxis = rotated.GetColumn2(); // Use column 2 for Z axis up space
    }
    else
    {
        // Fixed focus.
        emit.fCosMax = cosf(DEG2RAD(fEmitAngle(VMIN, var.EMin())));
        emit.fCosMin = cosf(DEG2RAD(fEmitAngle(VMAX, var.EMax())));
    }

    if (bEmitOffsetDir)
    {
        AABB bb = GetEmitOffsetBounds();
        if (bb.max.z > 0)
        {
            emit.fCosMax = 1.f;
        }
        if (bb.min.z < 0)
        {
            emit.fCosMin = -1.f;
        }
        else if (bb.min.x < 0 || bb.max.x > 0 || bb.min.y < 0 || bb.max.y > 0)
        {
            emit.fCosMin = min(emit.fCosMin, 0.f);
        }
    }

    emit.fSpeedMin = fSpeed(VMIN, var.EMin()) * loc.s * opts.fSpeedScale;
    emit.fSpeedMax = fSpeed(VMAX, var.EMax()) * loc.s * opts.fSpeedScale;
}

void ResourceParticleParams::GetMaxTravelBounds(AABB& bbResult, const QuatTS& loc, const SPhysForces& forces, const FStaticBounds& opts) const
{
    float fTime = min(+opts.fMaxLife, GetMaxParticleLife());
    if (fTime <= 0.f)
    {
        return;
    }

    // Emission direction.
    SEmitParams emit;
    GetEmitParams(emit, loc, forces, opts, FEmitterRandom());

    SForceParams force;
    force.vWind = forces.vWind * fAirResistance.fWindScale;
    force.fStretch = !GetTailSteps() ? fStretch(VMAX) * max(fStretch.fOffsetRatio + 1.f, 0.f) : 0.f;

    // Perform separate checks for variations in drag and gravity.
    float afDrag[2] = { fAirResistance(VMAX), fAirResistance(VMIN) };
    float afGrav[2] = { fGravityScale(VMAX), fGravityScale(VMIN) };
    for (int iDragIndex = (afDrag[1] != afDrag[0] ? 1 : 0); iDragIndex >= 0; iDragIndex--)
    {
        force.fDrag = afDrag[iDragIndex];
        for (int iGravIndex = (afGrav[1] != afGrav[0] ? 1 : 0); iGravIndex >= 0; iGravIndex--)
        {
            force.vAccel = forces.vAccel * afGrav[iGravIndex] + vAcceleration * loc.s;
            Travel::TravelBB(bbResult, emit, force, fTime, 7);
        }
    }

    EEmitterShapeType emitterShape = GetEmitterShape();

    if (fTurbulence3DSpeed)
    {
        // Expansion from 3D turbulence.    a = T t^-/2;  d = a/2 t^2 = T/2 t^(3/2)
        float fAccel = fTurbulence3DSpeed(VMAX) * isqrt_tpl(fTime) * (1.f + fTRAVEL_SAFETY);
        SForceParams forcesTurb;
        forcesTurb.vAccel = Vec3(fAccel);
        forcesTurb.vWind.zero();
        forcesTurb.fDrag = fAirResistance(VMIN);
        Vec3 vTrav(ZERO), vVel(ZERO);
        Travel::Travel(vTrav, vVel, fTime, forcesTurb);
        bbResult.Expand(vTrav);
    }

    if (emitterShape == EEmitterShapeType::POINT ||
        emitterShape == EEmitterShapeType::CIRCLE ||
        emitterShape == EEmitterShapeType::SPHERE ||
        emitterShape == EEmitterShapeType::BOX)
    {
        Vec3 maxVel = vVelocity.GetMaxVector() + vVelocityXYZRandom.GetMaxVector();
        Vec3 minVel = vVelocity.GetMinVector() - vVelocityXYZRandom.GetMaxVector();
        if (!maxVel.IsZeroFast())
        {
            SForceParams forcesTurb;
            forcesTurb.vAccel.zero();
            forcesTurb.vWind.zero();
            forcesTurb.fDrag = 0.0f;

            Vec3 vTravMax(ZERO);
            Travel::Travel(vTravMax, maxVel, fTime, forcesTurb);

            Vec3 vTravMin(ZERO);
            Travel::Travel(vTravMin, minVel, fTime, forcesTurb);

            Vec3 max = vTravMax;
            Vec3 min = vTravMin;

            max.CheckMax(vTravMin);
            min.CheckMin(vTravMax);

            bbResult.max += max;
            bbResult.min += min;
        }

        if (emitterShape == EEmitterShapeType::POINT)
        {
            bbResult.Expand(Vec3(1,1,1) * fSpeed.GetMaxValue() * fParticleLifeTime.GetMaxValue());
        }
    }

    // Expansion from spiral vortex.
    if (fTurbulenceSpeed)
    {
        float fVortex = fTurbulenceSize(VMAX) * loc.s;
        bbResult.Expand(Vec3(fVortex));
    }
}

float ResourceParticleParams::GetTravelBounds(AABB& bbResult, const QuatTS& loc, const SForceParams& forces, const FStaticBounds& opts, const FEmitterFixed& var) const
{
    float fTime = min(+opts.fMaxLife, GetMaxParticleLife());
    if (fTime <= 0.f)
    {
        return 0.f;
    }

    // Emission direction
    SEmitParams emit;
    GetEmitParams(emit, loc, forces, opts, var);

    // Get destination BB

    bbResult.Reset();
    Travel::TravelBB(bbResult, emit, forces, fTime, 0);
    bbResult.Move(loc.t);

    // Estimate distance along arc
    return Travel::TravelDistanceApprox(emit.vAxis * (emit.fSpeedMax + emit.fSpeedMin) * 0.5f, fTime, forces);
}

void ResourceParticleParams::ComputeEnvironmentFlags()
{
    // Needs updated environ if particles interact with gravity or air.
    nEnvFlags = 0;

    if (!bEnabled)
    {
        return;
    }

    if (eFacing == eFacing.Water)
    {
        nEnvFlags |= ENV_WATER;
    }
    if (fAirResistance && fAirResistance.fWindScale)
    {
        nEnvFlags |= ENV_WIND;
    }
    if (fGravityScale || bFocusGravityDir)
    {
        nEnvFlags |= ENV_GRAVITY;
    }
    if (ePhysicsType == ePhysicsType.SimpleCollision)
    {
        if (bCollideTerrain)
        {
            nEnvFlags |= ENV_TERRAIN;
        }
        if (bCollideStaticObjects)
        {
            nEnvFlags |= ENV_STATIC_ENT;
        }
        if (bCollideDynamicObjects)
        {
            nEnvFlags |= ENV_DYNAMIC_ENT;
        }
        if (nEnvFlags & ENV_COLLIDE_ANY)
        {
            nEnvFlags |= ENV_COLLIDE_INFO;
        }
    }
    else if (ePhysicsType >= EPhysics::SimplePhysics)
    {
        nEnvFlags |= ENV_COLLIDE_INFO;
    }

    // Rendering params.
    if (fSizeY && fAlpha.GetMaxBaseLerp())
    {
        if (pStatObj != 0)
        {
            nEnvFlags |= REN_GEOMETRY;
        }
        else if (eFacing == eFacing.Decal)
        {
            if (pMaterial != 0)
            {
                nEnvFlags |= REN_DECAL;
            }
        }
        else
        {
            if (nTexId > 0 || nNormalTexId > 0 || nGlowTexId > 0 || pMaterial != 0)
            {
                nEnvFlags |= REN_SPRITE;
            }
        }
        // particle with geometry or GPU particles can cast shadows
        if (bCastShadows && (pStatObj || eEmitterType == EEmitterType::GPU))
        {
            nEnvFlags |= REN_CAST_SHADOWS;
        }

        if ((nEnvFlags & (REN_SPRITE | REN_GEOMETRY)) && eBlendType != eBlendType.Additive && !IsConnectedParticles())
        {
            nEnvFlags |= REN_SORT;
        }
    }
    if (LightSource.fIntensity && LightSource.fRadius)
    {
        nEnvFlags |= REN_LIGHTS;
    }
    if (!sStartTrigger.empty() || !sStopTrigger.empty())
    {
        nEnvFlags |= EFF_AUDIO;
    }
    else if (eForceGeneration != eForceGeneration.None)
    {
        nEnvFlags |= EFF_FORCE;
    }

    if (!fParticleLifeTime || bRemainWhileVisible                       // Infinite particle lifetime
        || bDynamicCulling
        || bBindEmitterToCamera                                                             // Visible every frame
        || ePhysicsType >= EPhysics::SimplePhysics)                     // Physicalized particles
    {
        nEnvFlags |= EFF_DYNAMIC_BOUNDS;
    }

    if (bBindEmitterToCamera)
    {
        nEnvFlags |= REN_BIND_CAMERA;
    }

    //
    // Compute desired and allowed renderer flags.
    //

    nRenObjFlags.Clear();
    nRenObjFlags.SetState(1,
        eBlendType
        + !HasVariableVertexCount() * FOB_POINT_SPRITE
        + bOctagonalShape * FOB_OCTAGONAL
        + bTessellation * FOB_ALLOW_TESSELLATION
        + TextureTiling.bAnimBlend * OS_ANIM_BLEND
        + ((fEnvProbeLighting > 0) ? OS_ENVIRONMENT_CUBEMAP : 0)
        + bGlobalIllumination * FOB_GLOBAL_ILLUMINATION
        + bReceiveShadows * FOB_PARTICLE_SHADOWS
        + bNotAffectedByFog * FOB_NO_FOG
        + bSoftParticle * FOB_SOFT_PARTICLE
        + bDrawOnTop * OS_NODEPTH_TEST
        + bDrawNear * FOB_NEAREST
        );

    // Disable impossible states.
    nRenObjFlags.SetState(-1,
        bDrawOnTop * FOB_NEAREST
        + (TextureTiling.nAnimFramesCount <= 1) * OS_ANIM_BLEND
        + (bDrawNear || bDrawOnTop) * FOB_SOFT_PARTICLE
        + bDrawNear * FOB_ALLOW_TESSELLATION
        + !fDiffuseLighting * (OS_ENVIRONMENT_CUBEMAP | FOB_GLOBAL_ILLUMINATION | FOB_PARTICLE_SHADOWS)
        + HasVariableVertexCount() * (FOB_MOTION_BLUR | FOB_OCTAGONAL | FOB_POINT_SPRITE)
        );

    // Construct config spec mask for allowed consoles.
    mConfigSpecMask = ((BIT(eConfigMax) * 2 - 1) & ~(BIT(eConfigMin) - 1)) << CONFIG_LOW_SPEC;
}

bool ResourceParticleParams::IsActive() const
{
    if (!bEnabled)
    {
        return false;
    }

    // If Quality cvar is set, it must match ConfigMin-Max params.
    if (GetCVars()->e_ParticlesQuality)
    {
        if (!(mConfigSpecMask & BIT(GetCVars()->e_ParticlesQuality)))
        {
            return false;
        }
    }

    // Get platform spec    
    ESystemConfigPlatform platform_spec = gEnv->pSystem->GetConfigPlatform();

    if (!Platforms.PCDX11 && platform_spec== CONFIG_PC ||
        !Platforms.hasMacOSMetal && platform_spec == CONFIG_OSX_METAL ||
        !Platforms.hasAndroid && platform_spec == CONFIG_ANDROID ||
#if defined(AZ_RESTRICTED_PLATFORM)
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/ParticleEffect_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/ParticleEffect_cpp_provo.inl"
    #endif
#endif
#if defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
#define AZ_RESTRICTED_PLATFORM_EXPANSION(CodeName, CODENAME, codename, PrivateName, PRIVATENAME, privatename, PublicName, PUBLICNAME, publicname, PublicAuxName1, PublicAuxName2, PublicAuxName3)\
        !Platforms.PublicAuxName1 && platform_spec == CONFIG_##PUBLICNAME ||
        AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS
#undef AZ_RESTRICTED_PLATFORM_EXPANSION
#endif
        !Platforms.hasIOS && platform_spec == CONFIG_IOS)
    {
        return false;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
int ResourceParticleParams::LoadResources()
{
    // Load only what is not yet loaded. Check everything, but caller may check params.bResourcesLoaded first.
    // Call UnloadResources to force unload/reload.
    LOADING_TIME_PROFILE_SECTION(gEnv->pSystem);

    if (!bEnabled)
    {
        nEnvFlags = 0;
        return 0;
    }

    if (ResourcesLoaded() || gEnv->IsDedicated())
    {
        ComputeEnvironmentFlags();
        return 0;
    }

    int nLoaded = 0;

    // Load material.
    if (!pMaterial && !nTexId && !sMaterial.empty() && eEmitterType == EEmitterType::CPU)
    {
        pMaterial = Get3DEngine()->GetMaterialManager()->LoadMaterial(sMaterial.c_str());
        if (!pMaterial || pMaterial == Get3DEngine()->GetMaterialManager()->GetDefaultMaterial())
        {
            CryWarning(VALIDATOR_MODULE_3DENGINE, VALIDATOR_WARNING, "Particle effect material %s not found",
                sMaterial.c_str());

            // Load error texture for artist debugging.
            pMaterial = 0;
            nTexId = GetRenderer()->EF_LoadTexture("!Error", FT_DONT_STREAM)->GetTextureID();
        }
        else if (sGeometry.empty())
        {
            SShaderItem& shader = pMaterial->GetShaderItem();

            bool validMaterial = shader.m_pShader != nullptr;

            if (!validMaterial)
            {
                //Check sub-materials for valid shader
                size_t subMatCount = pMaterial->GetSubMtlCount();
                for (size_t i = 0; i < subMatCount; i++)
                {
                    _smart_ptr<IMaterial> subMat = pMaterial->GetSubMtl(i);
                    if (subMat != nullptr && subMat->GetShaderItem().m_pShader != nullptr)
                    {
                        validMaterial = true;
                        break;
                    }
                }
            }

            //No material or submaterial had a valid shader; issue a warning
            if (!validMaterial)
            {
                CryWarning(VALIDATOR_MODULE_3DENGINE, VALIDATOR_WARNING, "Particle effect material %s has invalid shader",
                    sMaterial.c_str());
            }

            nLoaded++;
        }
    }
    if (eFacing == eFacing.Decal && sMaterial.empty())
    {
        CryWarning(VALIDATOR_MODULE_3DENGINE, VALIDATOR_WARNING, "Particle effect has no material for decal, texture = %s",
            sTexture.c_str());
    }

    // Load texture.
    if (!nTexId && !sTexture.empty() && !pMaterial)
    {
#if !defined(NULL_RENDERER)
        const uint32 textureLoadFlags = FT_DONT_STREAM;
        nTexId = GetRenderer()->EF_LoadTexture(sTexture.c_str(), textureLoadFlags)->GetTextureID();
        if (nTexId <= 0)
        {
            CryWarning(VALIDATOR_MODULE_3DENGINE, VALIDATOR_WARNING, "Particle effect texture %s not found", sTexture.c_str());
        }
        else
        {
            nLoaded++;
        }
#endif
    }

    // Load normal texture.
    if (!nNormalTexId && !sNormalMap.empty() && !pMaterial)
    {
#if !defined(NULL_RENDERER)
        const uint32 textureLoadFlags = FT_DONT_STREAM;
        nNormalTexId = GetRenderer()->EF_LoadTexture(sNormalMap.c_str(), textureLoadFlags)->GetTextureID();
        if (nNormalTexId <= 0)
        {
            CryWarning(VALIDATOR_MODULE_3DENGINE, VALIDATOR_WARNING, "Particle effect normalmap %s not found", sNormalMap.c_str());
        }
        else
        {
            nLoaded++;
        }
#endif
    }

    // Load glow texture.
    if (!nGlowTexId && !sGlowMap.empty() && !pMaterial)
    {
#if !defined(NULL_RENDERER)
        const uint32 textureLoadFlags = FT_DONT_STREAM;
        nGlowTexId = GetRenderer()->EF_LoadTexture(sGlowMap.c_str(), textureLoadFlags)->GetTextureID();
        if (nGlowTexId <= 0)
        {
            CryWarning(VALIDATOR_MODULE_3DENGINE, VALIDATOR_WARNING, "Particle effect glowmap %s not found", sGlowMap.c_str());
        }
        else
        {
            nLoaded++;
        }
#endif
    }

    // Set aspect ratio.
    if (fTexAspect == 0.f)
    {
        UpdateTextureAspect();
    }

    // Load geometry.
    if (!pStatObj && !sGeometry.empty())
    {
        pStatObj = Get3DEngine()->LoadStatObjAutoRef(sGeometry.c_str(), NULL, NULL, bStreamable);
        if (!pStatObj)
        {
            CryWarning(VALIDATOR_MODULE_3DENGINE, VALIDATOR_WARNING, "Particle effect geometry not found: %s", sGeometry.c_str());
        }
        else
        {
            nLoaded++;
        }
    }
    ComputeEnvironmentFlags();

    nEnvFlags |= EFF_LOADED;
    return nLoaded;
}

void ResourceParticleParams::UpdateTextureAspect()
{
    TextureTiling.Correct();

    fTexAspect = 1.f;
    ITexture* pTexture = GetRenderer()->EF_GetTextureByID(nTexId);
    if (pTexture)
    {
        float fWidth = pTexture->GetWidth() / (float)TextureTiling.nTilesX;
        float fHeight = pTexture->GetHeight() / (float)TextureTiling.nTilesY;
        if (fHeight == 0.f)
        {
            fTexAspect = 0.f;
        }
        else
        {
            fTexAspect = fWidth / fHeight;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void ResourceParticleParams::UnloadResources()
{
    // To do: Handle materials
    if (nTexId > 0)
    {
        GetRenderer()->RemoveTexture(static_cast<uint>(nTexId));
    }
    if (nNormalTexId > 0)
    {
        GetRenderer()->RemoveTexture(static_cast<uint>(nNormalTexId));
    }
    if (nGlowTexId > 0)
    {
        GetRenderer()->RemoveTexture(static_cast<uint>(nGlowTexId));
    }
    nTexId = 0;
    nNormalTexId = 0;
    nGlowTexId = 0;
    pStatObj = 0;
    pMaterial = 0;
    fTexAspect = 0.f;
    nEnvFlags &= ~EFF_LOADED;
}

//////////////////////////////////////////////////////////////////////////
//
// Struct and TypeInfo for reading older params
// Note: If you add a Vec3_tpl (or any object that owns a Vec3_tpl) they must be added to the initialization list.
// This is due to the default constructors executing after ZeroInit, and Vec3_tpl's default ctor initializes its values to NAN in _DEBUG
struct CompatibilityParticleParams
    : ParticleParams
    , ZeroInit<CompatibilityParticleParams>
    , Cry3DEngineBase
{
    int         nVersion;
    string  sSandboxVersion;

    // Version 32
    Vec3_TVarEParam_SFloat vSpawnPos;
    Vec3_TVarEParam_UFloat vSpawnPosRandom;
    Vec3_TVarEParam_SFloat vLocalSpawnPos;
    Vec3_TVarEParam_UFloat vLocalSpawnPosRandom;
    BoundingVolume EmitterSize; // used to confine emitter shapes from emitting past a volume

    // Version 31
    UnitFloat fOffsetInnerFraction;
    UnitFloat fLocalOffsetInnerFraction;

    // Version 30
    Vec3S       vPositionOffset;
    Vec3U       vRandomOffset;
    Vec3S       vLocalPositionOffset;
    Vec3U       vLocalRandomOffset;

    // Version 26
    float       fBounciness;

    // Version 25
    ETrinary tDX11;
    bool        bGeometryInPieces;

    // Version 24
    float       fAlphaTest;
    float       fCameraDistanceBias;
    int         nDrawLast;

    // Version 22
    float       fPosRandomOffset;
    float       fAlphaScale;
    float       fGravityScaleBias;
    float       fCollisionPercentage;

    bool        bSimpleParticle;
    bool        bSecondGeneration, bSpawnOnParentCollision, bSpawnOnParentDeath;

    string  sAllowHalfRes;

    // Version 20
    bool        bBindToEmitter;
    bool        bTextureUnstreamable, bGeometryUnstreamable;

    // Version 19
    bool        bIgnoreAttractor;

    void Correct(class CParticleEffect* pEffect);

    CompatibilityParticleParams(int nVer, cstr sSandboxVer, const ParticleParams& paramsDefault)
        : ParticleParams(paramsDefault)
        , nVersion(nVer)
        , sSandboxVersion(sSandboxVer)
#ifdef _DEBUG    // the default vec3 constructor sets values to nan if _DEBUG is defined.  This occurs after the ZeroInit base class initialization.  Reset these to zero.
        , vPositionOffset(ZERO)
        , vRandomOffset(ZERO)
        , vLocalPositionOffset(ZERO)
        , vLocalRandomOffset(ZERO)
#endif
    {
        if (nVersion < 21)
        {
            // Use old default values.
            fCount = 1.f;
            fParticleLifeTime = 1.f;
            fSpeed = 1.f;
        }
    }

    STRUCT_INFO
};

//////////////////////////////////////////////////////////////////////////
STRUCT_INFO_BEGIN(CompatibilityParticleParams)
BASE_INFO(ParticleParams)

VAR_INFO(vSpawnPos)
VAR_INFO(vSpawnPosRandom)
VAR_INFO(vLocalSpawnPos)
VAR_INFO(vLocalSpawnPosRandom)
VAR_INFO(EmitterSize)

// Version 31
VAR_INFO(fOffsetInnerFraction)
VAR_INFO(fLocalOffsetInnerFraction)


// Version 30
VAR_INFO(vPositionOffset)
VAR_INFO(vRandomOffset)
VAR_INFO(vLocalPositionOffset)
VAR_INFO(vLocalRandomOffset)

// Version 27
VAR_INFO(fSize)
VAR_INFO(fAspect)

// Version 26
VAR_INFO(fBounciness)

// Version 24
ALIAS_INFO(fRotationalDragScale, fAirResistance.fRotationalDragScale)
ALIAS_INFO(fWindScale, fAirResistance.fWindScale)
VAR_INFO(fAlphaTest)
VAR_INFO(fCameraDistanceBias)
VAR_INFO(nDrawLast)

// Version 22
VAR_INFO(fPosRandomOffset)
VAR_INFO(fAlphaScale)
VAR_INFO(fGravityScaleBias)
VAR_INFO(fCollisionPercentage)

VAR_INFO(bSimpleParticle)
VAR_INFO(bSecondGeneration)
VAR_INFO(bSpawnOnParentCollision)
VAR_INFO(bSpawnOnParentDeath)

VAR_INFO(sAllowHalfRes)

// Ver 22
ALIAS_INFO(bOctogonalShape, bOctagonalShape)
ALIAS_INFO(bOffsetInnerScale, fOffsetInnerFraction)
VAR_INFO(bSimpleParticle)

// Version 20
VAR_INFO(bBindToEmitter)
VAR_INFO(bTextureUnstreamable)
VAR_INFO(bGeometryUnstreamable)

// Ver 19
ALIAS_INFO(fLightSourceRadius, LightSource.fRadius)
ALIAS_INFO(fLightSourceIntensity, LightSource.fIntensity)
ALIAS_INFO(fStretchOffsetRatio, fStretch.fOffsetRatio)
ALIAS_INFO(nTailSteps, fTailLength.nTailSteps)
VAR_INFO(bIgnoreAttractor)
STRUCT_INFO_END(CompatibilityParticleParams)

//////////////////////////////////////////////////////////////////////////
void CompatibilityParticleParams::Correct(CParticleEffect* pEffect)
{
    CTypeInfo const& info = ::TypeInfo(this);

    // Convert any obsolete parameters set.
    switch (nVersion)
    {
    case 19:
        if (bIgnoreAttractor)
        {
            TargetAttraction.eTarget = TargetAttraction.eTarget.Ignore;
        }

    case 20:
        // Obsolete parameter, set equivalent params.
        if (bBindToEmitter && !ePhysicsType)
        {
            eMoveRelEmitter = eMoveRelEmitter.Yes;
            vPositionOffset.zero();
            vRandomOffset.zero();
            fSpeed = 0.f;
            fInheritVelocity = 0.f;
            fGravityScale = 0.f;
            fAirResistance.Set(0.f);
            vAcceleration.zero();
            fTurbulence3DSpeed = 0.f;
            fTurbulenceSize = 0.f;
            fTurbulenceSpeed = 0.f;
            bSpaceLoop = false;
        }

        if (bTextureUnstreamable || bGeometryUnstreamable)
        {
            bStreamable = false;
        }

    case 21:
        if (fFocusAzimuth.GetRandomRange() > 0.f)
        {
            // Convert confusing semantics of FocusAzimuth random variation in previous version.
            if (fFocusAngle.GetMaxValue() == 0.f)
            {
                // If FocusAngle = 0, FocusAzimuth is inactive.
                fFocusAzimuth.Set(0.f);
            }
            else if (fFocusAzimuth.GetMaxValue() > 0.f && fFocusAzimuth.GetRandomRange() == 1.f)
            {
                // Assume intention was to vary between 0 and max value, as with all other params.
            }
            else
            {
                // Convert previous absolute-360-based random range to standard relative.
                float fValMin = fFocusAzimuth.GetMaxValue();
                float fValMax = fValMin + fFocusAzimuth.GetRandomRange() * 360.f;
                if (fValMax > 360.f)
                {
                    fValMax -= 360.f;
                    fValMin -= 360.f;
                }
                if (-fValMin > fValMax)
                {
                    fFocusAzimuth.Set(fValMin, (fValMax - fValMin) / -fValMin);
                }
                else
                {
                    fFocusAzimuth.Set(fValMax, (fValMax - fValMin) / fValMax);
                }
            }
        }

    case 22:
        // Replace special-purpose flag with equivalent settings.
        if (bSimpleParticle)
        {
            sMaterial = "";
            fEmissiveLighting = 1;
            fDiffuseLighting = 0;
            fDiffuseBacklighting = 0;
            bReceiveShadows = bCastShadows = bTessellation = bGlobalIllumination = false;
            bSoftParticle.set(false);
            bNotAffectedByFog = true;
        }

        if (fAlphaScale != 0.f)
        {
            fAlpha.Set(fAlpha.GetMaxValue(0.0f) * fAlphaScale);
        }

        // Convert obsolete bias to proper random range.
        if (fGravityScaleBias != 0.f)
        {
            AdjustParamRange(fGravityScale, fGravityScaleBias);
        }

        if (sAllowHalfRes == "Allowed" || sAllowHalfRes == "Forced")
        {
            bHalfRes = true;
        }

        if (fCollisionPercentage != 0.f)
        {
            fCollisionFraction = fCollisionPercentage * 0.01f;
        }

        // Convert to current spawn enum.
        if (bSecondGeneration)
        {
            eSpawnIndirection =
                bSpawnOnParentCollision ? ESpawn::ParentCollide
                : bSpawnOnParentDeath ? ESpawn::ParentDeath
                : ESpawn::ParentStart;
        }

        if (fPosRandomOffset != 0.f)
        {
            // Convert to new roundness fraction.
            vRandomOffset += Vec3(fPosRandomOffset);
            fOffsetRoundness = fPosRandomOffset / max(max(vRandomOffset.x, vRandomOffset.y), vRandomOffset.z);
        }

        if (bEnabled && fPulsePeriod < 0.f)
        {
            Warning("Particle Effect '%s' has PulsePeriod < 0, obsolete setting", pEffect->GetFullName().c_str());
            fPulsePeriod = 0.f;
        }

    case 23:
    case 24:
        // Scaling by parent size.
        // Incorrect scaling of speed and offsets by parent particle size:
        //   Particle Version < 22, CryEngine < 3.3.3:         Correct
        //   Particle version = 22, CryEngine 3.3.4 .. 3.4.x:  Correct
        //   Particle version = 22, CryEngine 3.5.1 .. 3.5.3:  INCORRECT
        //   Particle version = 23, CryEngine 3.5.4:           INCORRECT
        //   Particle version = 24, CryEngine 3.5.5 .. 3.5.6:  INCORRECT
        //   Particle version = 24, CryEngine 3.5.7 .. 3.5.9:  Correct
        //   Particle version = 25, CryEngine 3.5.10 ..:       Correct
        if ((nVersion == 23 || nVersion >= 22 && sSandboxVersion >= "3.5.1" && sSandboxVersion < "3.5.7") && eSpawnIndirection && pEffect->GetParent())
        {
            // Print diagnostics when any corrections made.
            float fParentSize = pEffect->GetParent()->GetParticleParams().fSize.GetValueFromMod(1.f, 0.f);
            if (fParentSize != 1.f)
            {
                if (fSpeed)
                {
                    Warning("Particle Effect '%s' (version %d, %s): Speed corrected by parent scale %g from %g to %g",
                        pEffect->GetFullName().c_str(), nVersion, sSandboxVersion.c_str(), fParentSize,
                        +fSpeed, +fSpeed * fParentSize);
                    fSpeed.Set(fSpeed.GetMaxValue() * fParentSize);
                }
                if (!vPositionOffset.IsZero())
                {
                    Warning("Particle Effect '%s' (version %d, %s): PositionOffset corrected by parent scale %g from (%g,%g,%g) to (%g,%g,%g)",
                        pEffect->GetFullName().c_str(), nVersion, sSandboxVersion.c_str(), fParentSize,
                        +vPositionOffset.x, +vPositionOffset.y, +vPositionOffset.z,
                        +vPositionOffset.x * fParentSize, +vPositionOffset.y * fParentSize, +vPositionOffset.z * fParentSize);
                    vPositionOffset.x = vPositionOffset.x * fParentSize;
                    vPositionOffset.y = vPositionOffset.y * fParentSize;
                    vPositionOffset.z = vPositionOffset.z * fParentSize;
                }
                if (!vRandomOffset.IsZero())
                {
                    Warning("Particle Effect '%s' (version %d, %s): RandomOffset corrected by parent scale %g from (%g,%g,%g) to (%g,%g,%g)",
                        pEffect->GetFullName().c_str(), nVersion, sSandboxVersion.c_str(), fParentSize,
                        +vRandomOffset.x, +vRandomOffset.y, +vRandomOffset.z,
                        +vRandomOffset.x * fParentSize, +vRandomOffset.y * fParentSize, +vRandomOffset.z * fParentSize);
                    vRandomOffset.x = vRandomOffset.x * fParentSize;
                    vRandomOffset.y = vRandomOffset.y * fParentSize;
                    vRandomOffset.z = vRandomOffset.z * fParentSize;
                }
            }
        }

        // Alpha test changes.
        if (fAlphaTest != 0.f)
        {
            AlphaClip.fSourceMin.Min = AlphaClip.fSourceMin.Max = fAlphaTest;
        }

        // Sort param changes.
        if (nDrawLast)
        {
            fSortOffset = nDrawLast * -0.01f;
        }
        if (fCameraDistanceBias != 0.f)
        {
            fCameraDistanceOffset = -fCameraDistanceBias;
        }

    case 25:
        // DX11 spec
        if (tDX11 == ETrinary(false))
        {
            Platforms.PCDX11 = false;
        }

        // Fix reversed PivotY.
        fPivotY.Set(-fPivotY.GetMaxValue());

        if (bGeometryInPieces)
        {
            eGeometryPieces = eGeometryPieces.AllPieces;
        }
        else if (!sGeometry.empty())
        {
            if (_smart_ptr<IStatObj> pStatObj = Get3DEngine()->LoadStatObjAutoRef(sGeometry.c_str(), NULL, NULL, bStreamable))
            {
                if (GetSubGeometryCount(pStatObj))
                {
                    eGeometryPieces = eGeometryPieces.RandomPiece;
                }
            }
        }

    case 26:
        // Bounciness conversion
        if (fBounciness > 0.f)
        {
            fElasticity = fBounciness;
        }
        else if (fBounciness < 0.f)
        {
            nMaxCollisionEvents = 1;
            eFinalCollision = eFinalCollision.Die;
        }

        // In version 27, we removed fSize/fAspect and replaced it with fSizeX, fSizeY
        // Default to syncing and using fSize's curves/random, then override X properties and desync if fAspect doesn't match up.
        bMaintainAspectRatio = true;

        // Originally fSize represented the height of a particle and the width was represented by fAspect * fSize.
        fSizeY = fSize;

        // best approximation of random range is to take the max of the two components...if either size or aspect at 1 could
        // give it a size range of 0 to base
        float randomX;
        if (fSize.GetRandomRange() >= fAspect.GetRandomRange())
        {
            randomX = fSize.GetRandomRange();
        }
        else
        {
            randomX = fAspect.GetRandomRange();
            bMaintainAspectRatio = false;
        }
        {
            float baseX = fSize.Base() * fAspect.Base();
            fSizeX.Set(TVarParam<UFloat>::T(baseX), TVarParam<UFloat>::TRandom(randomX));
        }
        fSizeY.SetEmitterStrength(fSize.GetEmitterStrength());
        if (!fAspect.GetEmitterStrength().IsIdentity())
        {
            fSizeX.SetEmitterStrength(fAspect.GetEmitterStrength());
            bMaintainAspectRatio = false;
        }
        else
        {
            fSizeX.SetEmitterStrength(fSize.GetEmitterStrength());
        }

        fSizeY.SetParticleAge(fSize.GetParticleAge());
        if (!fAspect.GetParticleAge().IsIdentity())
        {
            fSizeX.SetParticleAge(fAspect.GetParticleAge());
            bMaintainAspectRatio = false;
        }
        else
        {
            fSizeX.SetParticleAge(fSize.GetParticleAge());
        }
    case 27:
        // In version 28, we changed the orientation of the particle preview window to match the editor window.
        // We also made the Z-axis the primary direction of the emitter rather than the Y-axis. This change is
        // to flip the emitter direction to the Z axis while preserving its original direction.

        if (fCount > 0)
        {
            float originalAngle = DEG2RAD(fFocusAngle);
            float originalAzimuth = DEG2RAD(fFocusAzimuth);

            // get the direction in current Z-up space with the original angle/azimuth
            Vec3 originalDir = Vec3(
                    sinf(originalAngle) * sinf(originalAzimuth),
                    sinf(originalAngle) * cosf(originalAzimuth),
                    cosf(originalAngle));

            // rotate the direction -90 degrees around the X axis to adjust for us adjusting from Y-up to Z-up
            Vec3 newDir = originalDir * Quat::CreateRotationX(DEG2RAD(-90.0f));

            // calculate the new angle and azimuth based on the new direction
            float newAngle = acosf(newDir.z);
            float newAzimuth = 0.0f;
            if (abs(newAngle) > RAD_EPSILON)     // don't divide by 0
            {
                newAzimuth = acosf(newDir.y / sinf(newAngle));
            }

            // store back as degrees and round to nearest tenth of a degree
            fFocusAngle.Set(roundf(RAD2DEG(newAngle) * 10.0f) / 10.0f);
            fFocusAzimuth.Set(roundf(RAD2DEG(newAzimuth) * 10.0f) / 10.0f);
        }
    case 28:
    {
        //convert stored final values to Local values for use with group nodes - confetti
        vLocalPositionOffset = vPositionOffset;
        vLocalRandomOffset = vRandomOffset;
        vLocalRandomAngles = vRandomAngles;
        vLocalInitAngles = vInitAngles;
        fLocalOffsetRoundness = fOffsetRoundness;
        fLocalOffsetInnerFraction = fOffsetInnerFraction;
        fLocalStretch = fStretch;
        bGroup = false;
    }
    case 29:
    {
        vLocalSpawnPosOffset = vLocalPositionOffset;
        vLocalSpawnPosRandomOffset = vLocalRandomOffset;
    }
    case 30:
    {
        fLocalSpawnIncrement.Set(fLocalOffsetInnerFraction);
    }
    case 31:
    {
        vPositionOffset.Set(vLocalSpawnPos.fX, vLocalSpawnPos.fY, vLocalSpawnPos.fZ);
        vRandomOffset.Set(vLocalSpawnPosRandom.fX, vLocalSpawnPosRandom.fY, vLocalSpawnPosRandom.fZ);
        fEmitterSizeDiameter.Set(EmitterSize.fX);
        vEmitterSizeXYZ.Set(SFloat(EmitterSize.fX), SFloat(EmitterSize.fY), SFloat(EmitterSize.fZ));
        bool confine = EmitterSize;
        bConfineX = bConfineY = bConfineZ = confine;
    }

        //////////////////////////////////////////////////////////////////////////
        // IMPORTANT: Remember to update nSERIALIZE_VERSION if you update the
        // particle system
        //////////////////////////////////////////////////////////////////////////
    };





    // Universal corrections.
    if (!fTailLength)
    {
        fTailLength.nTailSteps = 0;
    }

    if (eSpawnIndirection == ESpawn::ParentCollide || eSpawnIndirection == ESpawn::ParentDeath)
    {
        bContinuous = false;
    }

    TextureTiling.Correct();

    if (bSpaceLoop && fCameraMaxDistance == 0.f && GetEmitOffsetBounds().GetVolume() == 0.f)
    {
        Warning("Particle Effect '%s' has zero space loop volume: disabled", pEffect->GetFullName().c_str());
        bEnabled = false;
    }

    //In version 33, particle sizeZ was introduced for geometry particle. The default value of sizeZ should be set as sizeY 
    //when load from any previous version
    if (nVersion < 33)
    {
        fSizeZ = fSizeY;
    }
}

//////////////////////////////////////////////////////////////////////////
// CLodParticle implementation
//////////////////////////////////////////////////////////////////////////
CLodParticle::CLodParticle(CLodInfo* lod, const CParticleEffect* originalEffect)
{
    pLod = lod;

    if (originalEffect->HasParams())
    {
        pLodParticle = new CParticleEffect(GenerateName(originalEffect->GetName()).c_str(), originalEffect->GetParams());
    }
    else
    {
        pLodParticle = new CParticleEffect(GenerateName(originalEffect->GetName()).c_str(), originalEffect->GetParent()->GetDefaultParams());
    }

    //find matching parent lod effect from parent effect and set proper parent for this lod effect.
    IParticleEffect* parentLodEffect = nullptr;
    if (originalEffect->GetParent())
    {
        parentLodEffect = originalEffect->GetParent()->GetLodParticle(pLod);
    }
    pLodParticle->SetParent(parentLodEffect);
}

CLodParticle::CLodParticle()
    : pLod(nullptr)
    , pLodParticle(nullptr)
{
}

CLodParticle::~CLodParticle()
{
    if (pLodParticle)
    {
        pLodParticle->SetParent(NULL);
    }
}

//The name gererated in this function won't be used to identify the particle effect. It's only for debug or display in UI. 
AZStd::string CLodParticle::GenerateName(const AZStd::string& name)
{
    // Convert the distance from float to int to avoid adding '.' to the name which could cause issue 
    // since the particle manager is using on '.' as token for parsing particle names
    return (name + "Lod" + AZStd::to_string((int)pLod->GetDistance()).c_str());
}

void CLodParticle::Rename(const AZStd::string& name)
{
    pLodParticle->SetName(GenerateName(name).c_str());
}

CParticleEffect*    CLodParticle::GetParticle()
{
    return pLodParticle;
}

SLodInfo* CLodParticle::GetLod()
{
    return pLod;
}

void CLodParticle::SetLod(SLodInfo* lod)
{
    pLod = static_cast<CLodInfo*>(lod);
}

void CLodParticle::Serialize(XmlNodeRef node, bool bLoading, bool bAll, CParticleEffect* originalEffect)
{
    if (bLoading)
    {
        XmlNodeRef lodParticleChild = node->findChild("LodParticle");
        float distance = 0.0f;
        lodParticleChild->getAttr("Distance", distance);
        CParticleEffect* parentEffect = static_cast<CParticleEffect*>(originalEffect->GetParent());

        if (parentEffect != nullptr)
        {          
            pLod = static_cast<CLodInfo*>(parentEffect->GetLevelOfDetailByDistance(distance));
            // skip serialization if parent doesn't have Lod.
            // this could happen if a particle is moved to a parent which doesn't have Lod.
            if (pLod == nullptr)
            {
                return;
            }
        }
        else
        {            
            //the pLod may exist if the particle get re-serialized. 
            if (pLod == nullptr)
            {
                pLod = new CLodInfo();
            }
            pLod->SetDistance(distance);
            bool active = false;
            lodParticleChild->getAttr("Active", active);
            pLod->SetActive(active);

            //if the originalEffect is root effect which doesn't have parent, then it's the top particle in CLodInfo 
            pLod->SetTopParticle(originalEffect);
        }

        if (pLodParticle == nullptr)
        {
            pLodParticle = new CParticleEffect();
        }

        //find matching parent lod effect from parent effect and set proper parent for this lod effect.
        IParticleEffect* parentLodEffect = nullptr;
        if (originalEffect->GetParent())
        {
            parentLodEffect = originalEffect->GetParent()->GetLodParticle(pLod);
        }
        pLodParticle->SetParent(parentLodEffect);

        pLodParticle->Serialize(lodParticleChild->getChild(0), bLoading, bAll);

        //lod particle always uses generated name instead name in xml. 
        //The previous version may have generated some nonsense name due to the float number adding '.' in the name
        Rename(originalEffect->GetBaseName());
    }
    else //Saving
    {
        XmlNodeRef lodParticleChild = node->newChild("LodParticle");
        lodParticleChild->setAttr("Distance", pLod->GetDistance());
        lodParticleChild->setAttr("Active", pLod->GetActive());

        XmlNodeRef particleChild = lodParticleChild->newChild("Particle");
        if (pLodParticle)
        {
            pLodParticle->Serialize(particleChild, bLoading, bAll);
        }
    }
}

void CLodParticle::GetMemoryUsage(ICrySizer* pSizer) const
{
    if (!pSizer->AddObjectSize(this))
    {
        return;
    }

    pLodParticle->GetMemoryUsage(pSizer);
    pLod->GetMemoryUsage(pSizer);
}


bool CLodParticle::IsValid()
{
    return (pLod && pLodParticle);
}

//////////////////////////////////////////////////////////////////////////
// CLodInfo
//////////////////////////////////////////////////////////////////////////
CLodInfo::CLodInfo()
{
    pTopParticle = nullptr;
}
CLodInfo::CLodInfo(float distance, bool active, IParticleEffect* effect)
{
    fDistance = distance;
    bActive = active;
    pTopParticle = effect;
}

void CLodInfo::SetDistance(float distance)
{
    fDistance = distance;
    if (pTopParticle)
    {
        static_cast<CParticleEffect*>(pTopParticle.get())->SortLevelOfDetailBasedOnDistance();
        CParticleManager::Instance()->UpdateEmitters(pTopParticle);
    }
}

void CLodInfo::SetActive(bool active)
{
    bActive = active;
    CParticleManager::Instance()->UpdateEmitters(static_cast<CParticleEffect*>(pTopParticle.get()));
}

float CLodInfo::GetDistance()
{
    return fDistance;
}

bool CLodInfo::GetActive()
{
    return bActive;
}

IParticleEffect* CLodInfo::GetTopParticle()
{
    return pTopParticle;
}

void CLodInfo::SetTopParticle(IParticleEffect* particleEffect)
{
    pTopParticle = particleEffect;
}

void CLodInfo::GetMemoryUsage(ICrySizer* pSizer) const
{
    if (!pSizer->AddObjectSize(this))
    {
        return;
    }

    //No need to size up top particle as it is guaranteed to already be sized.
}

void CLodInfo::Refresh()
{
    CParticleManager::Instance()->UpdateEmitters(static_cast<CParticleEffect*>(pTopParticle.get()));
}


//////////////////////////////////////////////////////////////////////////
// ParticleEffect implementation

//////////////////////////////////////////////////////////////////////////
CParticleEffect::CParticleEffect()
    : m_parent(0)
    , m_pParticleParams(0)
{
}

CParticleEffect::CParticleEffect(const CParticleEffect& in, bool bResetInheritance)
    : m_parent(0)
    , m_strName(in.m_strName)
    , m_pParticleParams(new ResourceParticleParams(*in.m_pParticleParams))
{
    if (bResetInheritance)
    {
        m_pParticleParams->eInheritance = ParticleParams::EInheritance();
    }

    m_children.resize(in.m_children.size());
    for_array(i, m_children)
    {
        m_children.at(i) = new CParticleEffect(in.m_children[i], bResetInheritance);
        m_children[i].m_parent = this;
    }
}

CParticleEffect::CParticleEffect(const char* sName)
    : m_parent(0)
    , m_pParticleParams(0)
    , m_strName(sName)
{
}

CParticleEffect::CParticleEffect(const char* sName, const ParticleParams& params)
    : m_parent(0)
    , m_strName(sName)
{
    m_pParticleParams = new ResourceParticleParams(params);
    LoadResources(false);

    if (!(m_pParticleParams->nEnvFlags & (REN_ANY | EFF_ANY)))
    {
        // Assume custom params with no specified texture or geometry can have geometry particles added.
        m_pParticleParams->nEnvFlags |= REN_GEOMETRY;
    }
}

//////////////////////////////////////////////////////////////////////////
CParticleEffect::~CParticleEffect()
{
    UnloadResources();
    delete m_pParticleParams;
    m_pParticleParams = nullptr;

    for (int i = m_children.size() - 1; i >= 0; i--) //walking backwards since SetParent will remove items from m_children[]
    {
        m_children[i].SetParent(nullptr);
    }
}

void CParticleEffect::SetEnabled(bool bEnabled)
{
    if (bEnabled != IsEnabled())
    {
        InstantiateParams();
        m_pParticleParams->bEnabled = bEnabled;
        CParticleManager::Instance()->UpdateEmitters(this);
    }
}

//////////////////////////////////////////////////////////////////////////
void CParticleEffect::SetName(cstr sNewName)
{
    if (!m_parent)
    {
        // Top-level effect. Should be fully qualified with library and group names.
        // Remove and reinsert in set.
        if (m_strName != sNewName)
        {
            CParticleManager::Instance()->RenameEffect(this, sNewName);
            m_strName = sNewName;
        }
    }
    else
    {
        // Child effect. Use only final component, and prefix with parent name.
        cstr sBaseName = strrchr(sNewName, '.');
        sBaseName = sBaseName ? sBaseName + 1 : sNewName;

        // Ensure unique name.
        stack_string sNewBase;
        for (int i = m_parent->m_children.size() - 1; i >= 0; i--)
        {
            const CParticleEffect* pSibling = &m_parent->m_children[i];
            if (pSibling != this && pSibling->m_strName == sBaseName)
            {
                // Extract and replace number.
                cstr p = sBaseName + strlen(sBaseName);
                while (p > sBaseName && (p[-1] >= '0' && p[-1] <= '9'))
                {
                    p--;
                }
                int nIndex = atoi(p);
                sNewBase.assign(sBaseName, p);
                sNewBase.append(stack_string(ToString(nIndex + 1)));
                sBaseName = sNewBase;

                // Reset loop.
                i = m_parent->m_children.size() - 1;
            }
        }

        m_strName = sBaseName;
    }

    if (HasLevelOfDetail())
    {
        for (int i = 0; i < m_levelofdetail.size(); i++)
        {
            m_levelofdetail[i].Rename(GetName());
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CParticleEffect::InstantiateParams()
{
    if (!m_pParticleParams)
    {
        m_pParticleParams = new ResourceParticleParams(CParticleManager::Instance()->GetDefaultParams());
    }
}

//////////////////////////////////////////////////////////////////////////
const char* CParticleEffect::GetBaseName() const
{
    const char* sName = m_strName.c_str();
    if (m_parent)
    {
        // Should only have base name.
        assert(!strchr(sName, '.'));
    }
    else
    {
        // Return everything after lib name.
        if (const char* sBase = strchr(sName, '.'))
        {
            sName = sBase + 1;
        }
    }
    return sName;
}

string CParticleEffect::GetFullName() const
{
    if (!m_parent)
    {
        return m_strName;
    }
    return m_parent->GetFullName() + "." + m_strName;
}


//////////////////////////////////////////////////////////////////////////
void CParticleEffect::SetParent(IParticleEffect* pParent)
{
    if (pParent == m_parent)
    {
        return;
    }

    //remove this effect from its previous parent
    _smart_ptr<IParticleEffect> ref_ptr = this;
    if (m_parent)
    {
        stl::find_and_erase(m_parent->m_children, this);
        //update previous parent since its modified. 
        CParticleManager::Instance()->UpdateEmitters(m_parent);
    }

    //if it will become a child effect
    if (pParent)
    {
        if (!m_parent)
        {
            // Was previously top-level effect. delete from particle manager
            CParticleManager::Instance()->DeleteEffect(this);
        }
        static_cast<CParticleEffect*>(pParent)->m_children.push_back(this);
    }

    m_parent = static_cast<CParticleEffect*>(pParent);

    //Note: the previous implementation was trying to migrate the LODs and make them work with the new parent. But there are many cases won't be 
    // resolved in a good way. For example the amount of LODS mismatching, the distance of each LOD mismatch. After discussion with PD, 
    // we agreed it's better we clean all LOD for good. 

    //Remove all lods when parent changes 
    RemoveAllLevelOfDetails();

    CParticleManager::Instance()->UpdateEmitters(this);
}

//////////////////////////////////////////////////////////////////////////
void CParticleEffect::ClearChilds()
{
    // Instead of just clearing m_children and letting the destructor handle everything,
    // we ensure that children are deleted before their parents. This helps prevent code
    // re-entry to CParticleManager::UpdateEmitters() and CParticleEmitter::RefreshEffect(),
    // which was causing nasty issues like lists/iterators getting corrupted. (This issue
    // showed up when copying and pasting a complex emitter tree onto another one).
    ClearChildsBottomUp(this);

    CParticleManager::Instance()->UpdateEmitters(this);
}

//////////////////////////////////////////////////////////////////////////
void CParticleEffect::ClearChildsBottomUp(CParticleEffect* effect)
{
    if (effect)
    {
        for_array(i, effect->m_children)
        {
            ClearChildsBottomUp(&effect->m_children[i]);
        }
        effect->m_children.clear();
    }
}

//////////////////////////////////////////////////////////////////////////
void CParticleEffect::InsertChild(int slot, IParticleEffect* pEffect)
{
    if (slot < 0)
    {
        slot = 0;
    }
    if (slot > m_children.size())
    {
        slot = m_children.size();
    }

    assert(pEffect);
    CParticleEffect* pEff = (CParticleEffect*)pEffect;
    pEff->m_parent = this;
    m_children.insert(m_children.begin() + slot, pEff);
    CParticleManager::Instance()->UpdateEmitters(this);
}

void CParticleEffect::ReorderChildren(const std::vector<IParticleEffect*> & children)
{
    bool modified = false;
    if (children.size() != m_children.size())
    {
        AZ_Assert(false, "Wrong usage of function CParticleEffect::ReorderChildren: input children count different than effect children count");
        return;
    }

    //check if the new order is different than previous order
    for (int i = 0; i < m_children.size(); i++)
    {
        if (&m_children[i] != children[i])
        {
            modified = true;
            break;
        }
    }

    if (modified)
    {
        //validate all children from input are the children of this particle effect
        for (int i = 0; i < children.size(); i++)
        {
            if (children[i]->GetParent() != this)
            {
                AZ_Assert(false, "Wrong usage of function CParticleEffect::ReorderChildren: all children in the input need to be children of this particle effect");
                return;
            }
        }

        m_children.clear();
        for (int i = 0; i < children.size(); i++)
        {
            m_children.push_back(static_cast<CParticleEffect*>(children[i]));
        }
        CParticleManager::Instance()->UpdateEmitters(this, true);
    }
}

//////////////////////////////////////////////////////////////////////////
int CParticleEffect::FindChild(IParticleEffect* pEffect) const
{
    for_array(i, m_children)
    {
        if (&m_children[i] == pEffect)
        {
            return i;
        }
    }
    return -1;
}

CParticleEffect* CParticleEffect::FindChild(const char* szChildName) const
{
    for_array(i, m_children)
    {
        if (m_children[i].m_strName == szChildName)
        {
            return &m_children[i];
        }
    }

    return 0;
}

const CParticleEffect* CParticleEffect::FindActiveEffect(int nVersion) const
{
    // Find active effect in tree most closely matching version.
    const CParticleEffect* pFoundEffect = 0;
    int nFoundVersion = 0;

    if (!nVersion)
    {
        nVersion = nSERIALIZE_VERSION;
    }

    // Check parent effect.
    if (m_pParticleParams && m_pParticleParams->IsActive())
    {
        // Check version against effect name.
        int nEffectVersion = atoi(GetBaseName());
        if (nEffectVersion <= nVersion)
        {
            pFoundEffect = this;
            nFoundVersion = nEffectVersion;
        }
    }

    // Look for matching children, which take priority.
    for_array(i, m_children)
    {
        if (const CParticleEffect* pChild = m_children[i].FindActiveEffect(nVersion))
        {
            int nEffectVersion = atoi(pChild->GetBaseName());
            if (nEffectVersion >= nFoundVersion)
            {
                pFoundEffect = pChild;
                nFoundVersion = nEffectVersion;
            }
        }
    }

    return pFoundEffect;
}

//////////////////////////////////////////////////////////////////////////
bool CParticleEffect::LoadResources(bool bAll, cstr sSource) const
{
    // Check file access if sSource specified.
    if (sSource && !ResourcesLoaded(true) && !CParticleManager::Instance()->CanAccessFiles(GetFullName(), sSource))
    {
        return false;
    }

    int nLoaded = 0;
    if (IsEnabled() && !m_pParticleParams->ResourcesLoaded())
    {
        CRY_DEFINE_ASSET_SCOPE("Particle Effect", GetFullName());
        nLoaded = m_pParticleParams->LoadResources();
    }

    if (HasLevelOfDetail())
    {
        for_array(i, m_levelofdetail)
        {
            m_levelofdetail[i].GetParticle()->LoadResources(bAll, sSource);
        }
    }

    if (bAll)
    {
        for_array(i, m_children)
        nLoaded += (int)m_children[i].LoadResources(true);
    }
    return nLoaded > 0;
}

void CParticleEffect::UnloadResources(bool bAll) const
{
    if (m_pParticleParams)
    {
        m_pParticleParams->UnloadResources();
    }

    for_array(i, m_levelofdetail)
    {
        m_levelofdetail[i].GetParticle()->UnloadResources(bAll);
    }

    if (bAll)
    {
        for_all(m_children).UnloadResources(true);
    }
}

bool CParticleEffect::ResourcesLoaded(bool bAll) const
{
    // Return whether all resources loaded.
    if (m_pParticleParams && !m_pParticleParams->ResourcesLoaded())
    {
        return false;
    }

    for_array(i, m_levelofdetail)
    {
        return m_levelofdetail[i].GetParticle()->ResourcesLoaded(bAll);
    }

    if (bAll)
    {
        for_array(i, m_children)
        if (!m_children[i].ResourcesLoaded(true))
        {
            return false;
        }
    }
    return true;
}

const ParticleParams& CParticleEffect::GetDefaultParams() const
{
    return GetDefaultParams(m_pParticleParams ? m_pParticleParams->eInheritance : ParticleParams::EInheritance(), 0);
}

const ParticleParams& CParticleEffect::GetDefaultParams(ParticleParams::EInheritance eInheritance, int nVersion) const
{
    if (eInheritance == eInheritance.Parent)
    {
        // If the parent is disabled, then it will not have its parameters initialized.
        if (m_parent && m_parent->HasParams())
        {
            return m_parent->GetParams();
        }
        eInheritance = eInheritance.System;
    }

    return CParticleManager::Instance()->GetDefaultParams(eInheritance, nVersion);
}

static int UpdateDefaultValues(const CTypeInfo& info, void* data, const void* def_data, const void* new_data)
{
    //the default data address shouldn't be same as new_data address. otherwise the function won't do anything
    AZ_Assert(def_data != new_data, "The default data address shouldn't be same as new_data address. otherwise the function won't do anything");

    int nUpdated = 0;
    if (info.HasSubVars())
    {
        for AllSubVars(pVar, info)
        nUpdated += UpdateDefaultValues(pVar->Type, pVar->GetAddress(data), pVar->GetAddress(def_data), pVar->GetAddress(new_data));
    }
    else
    {
        if (info.ValueEqual(data, def_data) && !info.ValueEqual(data, new_data))
        {
            info.FromValue(data, new_data, info);
            return 1;
        }
    }
    return nUpdated;
}

//Steven, Confetti - updates vars used in group nodes
static void CalculateFinalGroupValues(ResourceParticleParams& params)
{
    params.vSpawnPosOffset = static_cast<Vec3>(params.vLocalSpawnPosOffset + params.inheritedGroup.vPositionOffset);
    params.vSpawnPosRandomOffset = static_cast<Vec3>(params.vLocalSpawnPosRandomOffset + params.inheritedGroup.vRandomOffset);
    params.fOffsetRoundness = params.fLocalOffsetRoundness + params.inheritedGroup.fOffsetRoundness;
    params.fSpawnIncrement = params.fLocalSpawnIncrement + params.inheritedGroup.fOffsetInnerFraction;
    params.fSpawnIncrement.SetEmitterStrength(params.fLocalSpawnIncrement.GetEmitterStrength());
    params.vInitAngles = static_cast<Vec3>(params.vLocalInitAngles + params.inheritedGroup.vInitAngles);
    params.vRandomAngles = static_cast<Vec3>(params.vLocalRandomAngles + params.inheritedGroup.vRandomAngles);
    params.fStretch.Set(params.fLocalStretch + params.inheritedGroup.fStretch, params.fLocalStretch.GetRandomRange() + params.inheritedGroup.fStretch.GetRandomRange());
    params.fStretch.SetEmitterStrength(params.fLocalStretch.GetEmitterStrength());
    params.fStretch.SetParticleAge(params.fLocalStretch.GetParticleAge());
    params.fStretch.fOffsetRatio = params.fLocalStretch.fOffsetRatio;

    if (params.bGroup)
    {
        //no spawning particles if you're a group item
        params.fCount = 0; 
        params.fBeamCount = 0; 
        params.bContinuous = false;
    }
}

static void UpdateGroupValues(const ParticleParams& parent, ResourceParticleParams& child)
{
    if (!parent.bGroup && parent.inheritedGroup.bInheritedGroupData)
    {
        child.inheritedGroup.bInheritedGroupData = true;
        child.inheritedGroup.vPositionOffset = parent.inheritedGroup.vPositionOffset;
        child.inheritedGroup.vRandomOffset = parent.inheritedGroup.vRandomOffset;
        child.inheritedGroup.fOffsetRoundness = parent.inheritedGroup.fOffsetRoundness;
        child.inheritedGroup.fOffsetInnerFraction = parent.inheritedGroup.fOffsetInnerFraction;
        child.inheritedGroup.vInitAngles = parent.inheritedGroup.vInitAngles;
        child.inheritedGroup.vRandomAngles = parent.inheritedGroup.vRandomAngles;
        child.inheritedGroup.fStretch = parent.inheritedGroup.fStretch;
        child.inheritedGroup.fStretch.Set(parent.inheritedGroup.fStretch);
    }
    else if (parent.bGroup)
    {
        child.inheritedGroup.bInheritedGroupData = true;
        child.inheritedGroup.vPositionOffset = static_cast<Vec3S>(parent.vLocalSpawnPosOffset + parent.inheritedGroup.vPositionOffset);
        child.inheritedGroup.vRandomOffset = static_cast<Vec3U>(parent.vLocalSpawnPosRandomOffset + parent.inheritedGroup.vRandomOffset);
        child.inheritedGroup.fOffsetRoundness = parent.fLocalOffsetRoundness + parent.inheritedGroup.fOffsetRoundness;
        child.inheritedGroup.fOffsetInnerFraction = parent.fLocalSpawnIncrement + parent.inheritedGroup.fOffsetInnerFraction;
        child.inheritedGroup.vInitAngles = Vec3(parent.vLocalInitAngles + parent.inheritedGroup.vInitAngles);
        child.inheritedGroup.vRandomAngles = Vec3(parent.vLocalRandomAngles + parent.inheritedGroup.vRandomAngles);
        child.inheritedGroup.fStretch.Set(parent.fLocalStretch + parent.inheritedGroup.fStretch);
    }
    else
    {
        child.inheritedGroup.Clear();
    }

    CalculateFinalGroupValues(child);
}

void CParticleEffect::PropagateParticleParams(const ParticleParams &prevParams, const ParticleParams& newParams)
{
    for_array(i, m_children)
    {
        if (m_children[i].m_pParticleParams && m_children[i].m_pParticleParams->eInheritance == ParticleParams::EInheritance::Parent)
        {
            // save the child's current parameters
            ParticleParams childPrev = *m_children[i].m_pParticleParams;
            //Update all inherited values from new params.
            UpdateDefaultValues(TypeInfo(m_pParticleParams), m_children[i].m_pParticleParams, &prevParams, &newParams);
            //apply to it's children
            m_children[i].PropagateParticleParams(childPrev, *m_children[i].m_pParticleParams);
        }
        //if the particle effect was or is a group
        else if (prevParams.bGroup || prevParams.inheritedGroup.bInheritedGroupData
|| newParams.bGroup || newParams.inheritedGroup.bInheritedGroupData)
        {
        // save the child's current parameters
        ParticleParams childPrev = *m_children[i].m_pParticleParams;
        UpdateGroupValues(newParams, *m_children[i].m_pParticleParams);
        m_children[i].PropagateParticleParams(childPrev, *m_children[i].m_pParticleParams);
        }
    }
    if (m_pParticleParams)
    {
        CalculateFinalGroupValues(*m_pParticleParams);
    }
}

/////////////////////////////////////////////////////////////////////////////
// This function is used for particle editor to add LOD for this particle effect (root or any child)
// if this effect doesn't have some LODs which the root effect has, then it will add all the missing lods for the effect
// if there isn't any missing lods, then it will add a new lod with increased distance 
// Note: when adding a lod for an effect, all its parents and children will add this lod if they don't have it (in function AddLevelOfDetail(lod, addToChildren)) .
/////////////////////////////////////////////////////////////////////////////
void CParticleEffect::AddLevelOfDetail()
{
    //Find top effect
    IParticleEffect* top = GetRoot();

    //If this effect is a child effect and it has less lod count other than root's,
    //add all missing lods, for both its parents and children
    bool lodAdded = false;
    if (top != this && top->GetLevelOfDetailCount() != GetLevelOfDetailCount())
    {
        for (int i = 0; i < top->GetLevelOfDetailCount(); i++)
        {
            SLodInfo* lodInfo = top->GetLevelOfDetail(i);
            if (GetLevelOfDetailIndex(lodInfo) < 0)
            {
                AddLevelOfDetail(lodInfo, true);
                lodAdded = true;
            }
        }
    }

    //if no lod added, create new lod
    if (!lodAdded)
    {
        float distToAdd = 0;
        for_array(i, m_levelofdetail)
        {
            float currentLodDist = m_levelofdetail[i].GetLod()->GetDistance();
            if (currentLodDist > distToAdd)
            {
                distToAdd = currentLodDist;
            }
        }

        // New LOD distance will be set to LOD_DIST_OFFSET further than last LOD
        distToAdd += LOD_DIST_OFFSET;

        CLodInfo* lod = new CLodInfo(distToAdd, true, top);

        AddLevelOfDetail(lod, true);
    }

    static_cast<CParticleEffect*>(top)->SortLevelOfDetailBasedOnDistance();
    CParticleManager::Instance()->UpdateEmitters(top);
}

void CParticleEffect::ReplaceLOD(SLodInfo* lod)
{
    for (int i = 0; i < m_levelofdetail.size(); i++)
    {
        if (m_levelofdetail[i].GetLod()->GetDistance() == lod->GetDistance())
        {
            m_levelofdetail[i].SetLod(lod);
            break;
        }
    }

    for (int i = 0; i < m_children.size(); i++)
    {
        m_children[i].ReplaceLOD(lod);
    }
}

void CParticleEffect::AddLevelOfDetail(SLodInfo* lod, bool addToChildren)
{
    //if parent doesn't have this lod, add it for parent too
    if (m_parent && m_parent->GetLevelOfDetailIndex(lod) < 0)
    {
        m_parent->AddLevelOfDetail(lod, false);
    }

    CLodParticle* newLodParticle = new CLodParticle(static_cast<CLodInfo*>(lod), this);
    m_levelofdetail.push_back(newLodParticle);

    if (addToChildren)
    {
        for_array(i, m_children)
        {
            m_children[i].AddLevelOfDetail(lod, true);
        }
    }
}

bool CParticleEffect::HasLevelOfDetail() const
{
    if (m_pParticleParams)
    {
        return m_levelofdetail.size() > 0;
    }
    return false;
}

int CParticleEffect::GetLevelOfDetailCount() const
{
    if (m_pParticleParams)
    {
        return m_levelofdetail.size();
    }
    return 0;
}

SLodInfo*   CParticleEffect::GetLevelOfDetail(int index) const
{
    if (index < 0)
    {
        return nullptr;
    }

    if (m_pParticleParams)
    {
        if (m_levelofdetail.size() > index)
        {
            return m_levelofdetail[index].GetLod();
        }

        return nullptr;
    }
    return nullptr;
}

int CParticleEffect::GetLevelOfDetailIndex(const SLodInfo* lod) const
{
    if (lod == nullptr)
    {
        return -1;
    }

    for (int i = 0; i < m_levelofdetail.size(); i++)
    {
        if (m_levelofdetail[i].GetLod() == lod)
        {
            return i;
        }
    }

    return -1;
}

void CParticleEffect::RemoveLevelOfDetail(SLodInfo* lod)
{
    RemoveLevelOfDetailRecursive(lod);
    CParticleManager::Instance()->UpdateEmitters(static_cast<IParticleEffect*>(this));
}

void CParticleEffect::RemoveAllLevelOfDetails()
{
    while (m_levelofdetail.size() != 0)
    {
        RemoveLevelOfDetailRecursive(m_levelofdetail[m_levelofdetail.size() - 1].GetLod());
    }
    CParticleManager::Instance()->UpdateEmitters(static_cast<IParticleEffect*>(this));
}

void CParticleEffect::RemoveLevelOfDetailRecursive(SLodInfo* lod)
{
    for_array(i, m_levelofdetail)
    {
        if (m_levelofdetail[i].GetLod() == lod)
        {
            m_levelofdetail.erase(m_levelofdetail.begin() + i);
            break;
        }
    }

    for_array(i, m_children)
    {
        m_children[i].RemoveLevelOfDetailRecursive(lod);
    }
}

SLodInfo* CParticleEffect::GetLevelOfDetailByDistance(float distance) const
{
    for_array(i, m_levelofdetail)
    {
        if (m_levelofdetail[i].GetLod()->GetDistance() == distance)
        {
            return m_levelofdetail[i].GetLod();
        }
    }
    return nullptr;
}

void CParticleEffect::SortLevelOfDetailBasedOnDistance()
{
    int numItemsToSort = m_levelofdetail.size();

    if (numItemsToSort <= 1)
    {
        return;
    }

    for (int i = 0; i < numItemsToSort; ++i)
    {
        float currentNearDistance = FLT_MAX;
        int indexOfNearestEffect = 0;

        int itemsLeftToSort = numItemsToSort - i;
        for (int j = 0; j < itemsLeftToSort; ++j)
        {
            auto newLodInfo = m_levelofdetail.begin() + j;
            float newDistance = newLodInfo->get()->GetLod()->GetDistance();
            if (newDistance < currentNearDistance)
            {
                currentNearDistance = newDistance;
                indexOfNearestEffect = j;
            }
        }

        auto lod = &m_levelofdetail[indexOfNearestEffect];
        lod->AddRef();
        m_levelofdetail.erase(m_levelofdetail.begin() + indexOfNearestEffect);
        m_levelofdetail.push_back(lod);
        lod->Release();
    }

    for_array(i, m_children)
    {
        m_children[i].SortLevelOfDetailBasedOnDistance();
    }
}

IParticleEffect* CParticleEffect::GetLodParticle(SLodInfo* lod) const
{
    for_array(i, m_levelofdetail)
    {
        if (lod == m_levelofdetail[i].GetLod())
        {
            return m_levelofdetail[i].GetParticle();
        }
    }
    return nullptr;
}



// End of Level of Detail
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
bool CParticleEffect::IsActive(bool bAll) const
{
    // Make sure effect and all indirect parents are active in current render context.
    for (const CParticleEffect* pEffect = this;; pEffect = pEffect->m_parent)
    {
        if (!(pEffect && pEffect->m_pParticleParams && pEffect->m_pParticleParams->IsActive()))
        {
            return false;
        }
        if (!pEffect->m_pParticleParams->eSpawnIndirection)
        {
            break;
        }
    }

    if (m_pParticleParams->nEnvFlags & (REN_ANY | EFF_ANY))
    {
        // Has visible or other effects.
        return true;
    }

    for_array(i, m_levelofdetail)
    {
        if (m_levelofdetail[i].GetParticle()->IsActive(true))
        {
            return true;
        }
    }

    for_array(i, m_children)
    if (bAll || m_children[i].GetIndirectParent())
    {
        if (m_children[i].IsActive(true))
        {
            return true;
        }
    }
    return false;
}

uint32 CParticleEffect::GetEnvironFlags(bool bAll) const
{
    uint32 nFlags = m_pParticleParams ? m_pParticleParams->nEnvFlags : 0;

    for_array(i, m_levelofdetail)
    {
        nFlags |= m_levelofdetail[i].GetParticle()->GetEnvironFlags(false);
    }

    if (bAll)
    {
        for_array(i, m_children)
        if (m_children[i].IsActive())
        {
            nFlags |= m_children[i].GetEnvironFlags(true);
        }
    }
    return nFlags;
}

void CParticleEffect::SetParticleParams(const ParticleParams& params)
{
    CRY_DEFINE_ASSET_SCOPE("Particle Effect", GetFullName());

    InstantiateParams();
    //GPU particles do not use Materials, so we need to unload the material in order to not block textures from being loaded
    if (params.sTexture != m_pParticleParams->sTexture || params.sMaterial != m_pParticleParams->sMaterial
        || params.eEmitterType != m_pParticleParams->eEmitterType)
    {
        if (m_pParticleParams->nTexId > 0)
        {
            GetRenderer()->RemoveTexture((uint)m_pParticleParams->nTexId);
            m_pParticleParams->fTexAspect = 0.f;
        }
        m_pParticleParams->nTexId = 0;
        m_pParticleParams->pMaterial = 0;
        m_pParticleParams->nEnvFlags &= ~EFF_LOADED;
    }
    if (params.sNormalMap != m_pParticleParams->sNormalMap || params.sMaterial != m_pParticleParams->sMaterial
        || params.eEmitterType != m_pParticleParams->eEmitterType)
    {
        if (m_pParticleParams->nNormalTexId > 0)
        {
            GetRenderer()->RemoveTexture((uint)m_pParticleParams->nNormalTexId);
        }
        m_pParticleParams->nNormalTexId = 0;
    }
    if (params.sGlowMap != m_pParticleParams->sGlowMap || params.sMaterial != m_pParticleParams->sMaterial
        || params.eEmitterType != m_pParticleParams->eEmitterType)
    {
        if (m_pParticleParams->nGlowTexId > 0)
        {
            GetRenderer()->RemoveTexture((uint)m_pParticleParams->nGlowTexId);
        }
        m_pParticleParams->nGlowTexId = 0;
    }
    if (params.sGeometry != m_pParticleParams->sGeometry)
    {
        m_pParticleParams->pStatObj = 0;
        m_pParticleParams->nEnvFlags &= ~EFF_LOADED;
    }
    if (memcmp(&params.TextureTiling, &m_pParticleParams->TextureTiling, sizeof(params.TextureTiling)) != 0)
    {
        m_pParticleParams->fTexAspect = 0.f;
        m_pParticleParams->nEnvFlags &= ~EFF_LOADED;
    }

    PropagateParticleParams(*m_pParticleParams, params);



    static_cast<ParticleParams&>(*m_pParticleParams) = params;
    CalculateFinalGroupValues(*m_pParticleParams);
    m_pParticleParams->LoadResources();

    CParticleManager::Instance()->UpdateEmitters(this);
}

const ParticleParams& CParticleEffect::GetParticleParams() const
{
    if (m_pParticleParams)
    {
        return *m_pParticleParams;
    }
    return CParticleManager::Instance()->GetDefaultParams();
}

//////////////////////////////////////////////////////////////////////////
IParticleEmitter* CParticleEffect::Spawn(const QuatTS& loc, uint32 uEmitterFlags, const SpawnParams* pSpawnParams)
{
    return CParticleManager::Instance()->CreateEmitter(loc, this, uEmitterFlags, pSpawnParams);
}

//////////////////////////////////////////////////////////////////////////
void CParticleEffect::Serialize(XmlNodeRef node, bool bLoading, bool bAll, const ParticleParams* defaultParams)
{
    XmlNodeRef root = node;
    while (root->getParent())
    {
        root = root->getParent();
    }

    if (bLoading)
    {
        if (m_strName.empty())
        {
            if (m_parent)
            {
                // Set simple name, will be automatically qualified with hierarchy.
                SetName(node->getAttr("Name"));
            }
            else
            {
                // Qualify with library name.
                stack_string sFullName = root->getAttr("Name");
                if (sFullName.empty())
                {
                    sFullName = root->getTag();
                }
                if (!sFullName.empty())
                {
                    sFullName += ".";
                }
                sFullName += node->getAttr("Name");
                SetName(sFullName.c_str());
            }
        }

        int nVersion = nSERIALIZE_VERSION;
        cstr sSandboxVersion = "3.5";
        if (root->getAttr("ParticleVersion", nVersion))
        {
            if (nVersion < nMIN_SERIALIZE_VERSION || nVersion > nSERIALIZE_VERSION)
            {
                Warning("Particle Effect %s version (%d) out of supported range (%d-%d); may change in behavior",
                    GetName(), nVersion, nMIN_SERIALIZE_VERSION, nSERIALIZE_VERSION);
            }
        }
        root->getAttr("SandboxVersion", &sSandboxVersion);

        ParticleParams prevParam = GetParticleParams();

        bool bEnabled = false;
        XmlNodeRef paramsNode = node->findChild("Params");
        if (paramsNode && (gEnv->IsEditor() || GetAttrValue(*paramsNode, "Enabled", true)))
        {
            // Init params, then read from XML.
            bEnabled = true;

            // Get defaults base, and initialize params
            ParticleParams::EInheritance eInheritance = GetAttrValue(*paramsNode, "Inheritance", ParticleParams::EInheritance());
            CompatibilityParticleParams params(nVersion, sSandboxVersion, (defaultParams ? *defaultParams : GetDefaultParams(eInheritance, nVersion)) );

            FromXML(*paramsNode, params, FFromString().SkipEmpty(1));

            params.Correct(this);

            string finalCollision = paramsNode->getAttr("FinalCollision");

            // Correct the collision response for older particle libraries (to maintain backward compatibility). 'Bounce' more accurately describes the behavior of the particles. 
            if (finalCollision == "Ignore")
            {
                params.eFinalCollision = ParticleParams::ECollisionResponse::Bounce;
            }

            if (!m_pParticleParams)
            {
                m_pParticleParams = new ResourceParticleParams(params);
                for (int i = 0; i < m_levelofdetail.size(); i++)
                {
                    m_levelofdetail[i].GetParticle()->SetParticleParams(*m_pParticleParams);
                }
            }
            else
            {
                bool bLoadResources = m_pParticleParams->ResourcesLoaded();
                m_pParticleParams->UnloadResources();
                m_pParticleParams->~ResourceParticleParams();
                new(m_pParticleParams)ResourceParticleParams(params);
                if (bLoadResources)
                {
                    m_pParticleParams->LoadResources();
                }
            }
        }
        ////////////////////////////////////////////////////////////////////
        // Serialize Level of Details
        m_levelofdetail.clear();
        XmlNodeRef LODNode = node->findChild("LODs");
        if (LODNode)
        {
            for (int i = 0; i < LODNode->getChildCount(); i++)
            {
                XmlNodeRef lod = LODNode->getChild(i);

                XmlNodeRef lodParticleChild = lod->findChild("LodParticle");
                float distance = 0.0f;
                lodParticleChild->getAttr("Distance", distance);

                _smart_ptr<CLodParticle> lodParticle;
                lodParticle = new CLodParticle();

                //note: we do not save or load children of lod particles since their origin particles own them. (bAll set to false)
                lodParticle->Serialize(lod, bLoading, false, this);
                //If the lod particle didn't get correct lod info, don't add it to m_levelofdetail.
                //This could happen if this is a particle with lod setting is moved to a parent particle which doesn't have lod. 
                if (lodParticle->GetLod() == nullptr)
                {
                    lodParticle = nullptr;
                    continue;
                }
                m_levelofdetail.push_back(lodParticle);
            }
        }
        ////////////////////////////////////////////////////////////////
        if (bAll)
        {
            // Serialize children.
            XmlNodeRef childsNode = node->findChild("Childs");
            if (childsNode)
            {
                for (int i = 0; i < childsNode->getChildCount(); i++)
                {
                    XmlNodeRef xchild = childsNode->getChild(i);
                    _smart_ptr<IParticleEffect> pChildEffect;

                    if (cstr sChildName = xchild->getAttr("Name"))
                    {
                        pChildEffect = FindChild(sChildName);
                    }

                    if (!pChildEffect)
                    {
                        pChildEffect = new CParticleEffect();
                        pChildEffect->SetParent(this);
                    }

                    pChildEffect->Serialize(xchild, bLoading, bAll);
                    if (pChildEffect->GetParent() && !pChildEffect->GetParticleParams().eSpawnIndirection)
                    {
                        bEnabled = true;
                    }
                }
            }

            if (!gEnv->IsEditor() && !bEnabled && m_parent)
            {
                // Remove fully disabled child effects at load-time.
                SetParent(NULL);
            }
        }
        PropagateParticleParams(prevParam, GetParticleParams());

        if (!gEnv->IsEditor() && !bEnabled && m_parent)
        {
            // Remove fully disabled child effects at load-time.
            SetParent(NULL);
        }

        // Convert old internal targeting params
        if (nVersion < 23 && IsEnabled())
        {
            ParticleParams::STargetAttraction::ETargeting& eTarget = m_pParticleParams->TargetAttraction.eTarget;
            if (eTarget != eTarget.Ignore)
            {
                if (m_parent && m_parent->IsEnabled() && m_parent->GetParams().eForceGeneration == ParticleParams::EForce::_Target)
                {
                    // Parent generated target attraction
                    eTarget = eTarget.OwnEmitter;
                }
                if (GetParams().eForceGeneration == GetParams().eForceGeneration._Target)
                {
                    m_pParticleParams->eForceGeneration = ParticleParams::EForce::None;
                    if (m_children.empty())
                    {
                        // Target attraction set on childless effect, intention probably to target self
                        eTarget = eTarget.OwnEmitter;
                    }
                }
            }
        }
    }
    else
    {
        // Saving.
        bool bSerializeNamedFields = !!GetCVars()->e_ParticlesSerializeNamedFields;

        node->setAttr("Name", GetBaseName());
        root->setAttr("ParticleVersion", bSerializeNamedFields ? nSERIALIZE_VERSION : 23);

        if (m_pParticleParams)
        {
            // Save particle params.
            XmlNodeRef paramsNode = node->newChild("Params");

            ToXML<ParticleParams>(*paramsNode, *m_pParticleParams, (defaultParams ? *defaultParams : GetDefaultParams()),
                FToString().SkipDefault(1).NamedFields(bSerializeNamedFields));
        }

        if (m_levelofdetail.size() > 0)
        {
            // Serialize children.
            XmlNodeRef lodNode = node->newChild("LODs");
            for_array(i, m_levelofdetail)
            {
                XmlNodeRef lodchild = lodNode->newChild("LevelOfDetail");

                //note: we do not save or load children of lod particles since their origin particles own them. (bAll set to false)
                m_levelofdetail[i].Serialize(lodchild, bLoading, false);
            }
        }

        if (bAll && !m_children.empty())
        {
            // Serialize children.
            XmlNodeRef childsNode = node->newChild("Childs");
            for_array(i, m_children)
            {
                XmlNodeRef xchild = childsNode->newChild("Particles");
                m_children[i].Serialize(xchild, bLoading, bAll);
            }
        }

        if (m_strName == "System.Default")
        {
            gEnv->pParticleManager->SetDefaultEffect(this);
        }
    }
}

bool CParticleEffect::Reload(bool bAll)
{
    if (XmlNodeRef node = CParticleManager::Instance()->ReadEffectNode(GetFullName()))
    {
        Serialize(node, true, bAll);
        LoadResources(true);
        return true;
    }
    return false;
}

void CParticleEffect::GetMemoryUsage(ICrySizer* pSizer) const
{
    if (!pSizer->AddObjectSize(this))
    {
        return;
    }

    pSizer->AddObject(m_strName);
    pSizer->AddObject(m_children);
    pSizer->AddObject(m_pParticleParams);
    pSizer->AddObject(m_levelofdetail);
}

void CParticleEffect::GetEffectCounts(SEffectCounts& counts) const
{
    counts.nLoaded++;
    if (IsEnabled())
    {
        counts.nEnabled++;
        if (GetParams().ResourcesLoaded())
        {
            counts.nUsed++;
        }
        if (IsActive())
        {
            counts.nActive++;
        }
    }

    for_array(i, m_levelofdetail)
    {
        m_levelofdetail[i].GetParticle()->GetEffectCounts(counts);
    }

    for_array(i, m_children)
    {
        m_children[i].GetEffectCounts(counts);
    }
}

CParticleEffect* CParticleEffect::GetIndirectParent() const
{
    for (CParticleEffect const* pEffect = this; pEffect; pEffect = pEffect->m_parent)
    {
        if (pEffect->IsEnabled() && pEffect->GetParams().eSpawnIndirection)
        {
            return pEffect->m_parent;
        }
    }
    return 0;
}

float CParticleEffect::CalcEffectLife(FMaxEffectLife const& opts) const
{
    float fLife = 0.f;
    if (m_pParticleParams == nullptr)
    {
        return fLife;
    }
    if (opts.bPreviewMode || IsActive())
    {
        const ResourceParticleParams& params = GetParams();
        if (opts.fEmitterMaxLife > 0.f)
        {
            fLife = params.fPulsePeriod ? fHUGE : params.GetMaxEmitterLife();
            if (const CParticleEffect* pParent = GetIndirectParent())
            {
                float fParentLife = pParent->GetParams().GetMaxParticleLife();
                fLife = min(fLife, fParentLife);
            }
            fLife = min(fLife, +opts.fEmitterMaxLife);

            if (opts.bParticleLife)
            {
                fLife += params.fParticleLifeTime.GetMaxValue();
            }
        }
        else if (opts.bParticleLife)
        {
            fLife += params.GetMaxParticleLife();
        }
    }

    if (HasLevelOfDetail())
    {
        for_array(i, m_levelofdetail)
        {
            m_levelofdetail[i].GetParticle()->CalcEffectLife(opts);
        }
    }

    if (opts.bAllChildren || opts.bIndirectChildren)
    {
        for_array(i, m_children)
        {
            if (m_children[i].GetIndirectParent())
            {
                fLife = max(fLife, m_children[i].CalcEffectLife(opts().fEmitterMaxLife(fLife).bAllChildren(1)));
            }
            else if (opts.bAllChildren)
            {
                fLife = max(fLife, m_children[i].CalcEffectLife(opts));
            }
        }
    }
    return fLife;
}

float CParticleEffect::GetEquilibriumAge(bool bAll, bool bPreviewMode) const
{
    float fEquilibriumAge = 0.f;
    bool bHasEquilibrium = IsActive() && GetParams().HasEquilibrium();
    if (bHasEquilibrium)
    {
        fEquilibriumAge = GetParams().fSpawnDelay.GetMaxValue() + CalcMaxParticleFullLife(bPreviewMode);
        if (const CParticleEffect* pParent = GetIndirectParent())
        {
            fEquilibriumAge += pParent->GetEquilibriumAge(false, bPreviewMode);
        }
    }

    for_array(i, m_levelofdetail)
    {
        fEquilibriumAge = max(fEquilibriumAge, m_levelofdetail[i].GetParticle()->GetEquilibriumAge(bAll, bPreviewMode));
    }

    if (bAll)
    {
        for_array(i, m_children)
        {
            if ((bPreviewMode || m_children[i].IsActive()) && !m_children[i].GetParams().eSpawnIndirection)
            {
                fEquilibriumAge = max(fEquilibriumAge, m_children[i].GetEquilibriumAge(true, bPreviewMode));
            }
        }
    }

    return fEquilibriumAge;
}

IStatObj::SSubObject* GetSubGeometry(IStatObj* pParent, int i)
{
    assert(pParent);
    if (IStatObj::SSubObject* pSub = pParent->GetSubObject(i))
    {
        if (pSub->nType == STATIC_SUB_OBJECT_MESH && pSub->pStatObj && pSub->pStatObj->GetRenderMesh())
        {
            return pSub;
        }
    }
    return NULL;
}

int GetSubGeometryCount(IStatObj* pParent)
{
    int nPieces = 0;
    for (int i = pParent->GetSubObjectCount() - 1; i >= 0; i--)
    {
        if (GetSubGeometry(pParent, i))
        {
            nPieces++;
        }
    }
    return nPieces;
}
