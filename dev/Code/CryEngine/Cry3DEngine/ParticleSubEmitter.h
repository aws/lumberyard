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

// Description : Split out from ParticleEmitter.h


#ifndef CRYINCLUDE_CRY3DENGINE_PARTICLESUBEMITTER_H
#define CRYINCLUDE_CRY3DENGINE_PARTICLESUBEMITTER_H
#pragma once

#include "ParticleEffect.h"
#include "ParticleContainer.h"
#include "ParticleEnviron.h"
#include "ParticleUtils.h"
#include "BitFiddling.h"
#include <IAudioInterfacesCommonData.h>

class CParticleEmitter;
struct SParticleUpdateContext;
namespace Audio
{
    struct IAudioProxy;
} // namespace Audio

//////////////////////////////////////////////////////////////////////////
class CParticleSubEmitter
    : public Cry3DEngineBase
    , public _plain_reference_target<int>
    // Maintains an emitter source state, emits particles to a container
    // Ref count increased only by emitted particles
{
public:

    CParticleSubEmitter(CParticleSource* pSource, CParticleContainer* pCont);
    ~CParticleSubEmitter();

    ResourceParticleParams const& GetParams() const
    { return m_pContainer->GetParams(); }
    CParticleContainer& GetContainer() const
    { return *m_pContainer; }
    CParticleSource& GetSource() const
    {
        assert(m_pSource && m_pSource->NumRefs());
        return *m_pSource;
    }
    CParticleEmitter& GetMain() const
    { return GetContainer().GetMain(); }

    // State.
    bool IsActive() const
    { return GetAge() >= m_fActivateAge; }
    void Deactivate();

    // Timing.
    float GetAge() const
    { return GetSource().GetAge(); }
    float GetStartAge() const
    { return m_fStartAge; }
    float GetRepeatAge() const
    { return m_fRepeatAge; }
    float GetStopAge() const
    { return m_fStopAge; }
    float GetParticleStopAge() const
    { return GetStopAge() + GetParams().GetMaxParticleLife(); }
    float GetRelativeAge(float fAgeAdjust = 0.f) const
    { return GetAgeRelativeTo(m_fStopAge, fAgeAdjust); }

    float GetStopAge(ParticleParams::ESoundControlTime eControl) const;
    float GetRelativeAge(ParticleParams::ESoundControlTime eControl, float fAgeAdjust = 0.f) const
    { return GetAgeRelativeTo(min(GetStopAge(eControl), m_fRepeatAge), fAgeAdjust); }

    float GetStrength(float fAgeAdjust = 0.f, ParticleParams::ESoundControlTime eControl = ParticleParams::ESoundControlTime::EmitterLifeTime) const;

    Vec3 GetEmitPos() const;
    Vec3 GetEmitFocusDir(const QuatTS& loc, float fStrength, Quat* pRot = 0) const;

    // Actions.
    bool UpdateState(float fAgeAdjust = 0.f);
    void UpdateAudio();
    void ResetLoc()
    { m_LastLoc.s = -1.f; }
    void SetLastLoc()
    { m_LastLoc = GetSource().GetLocation(); }
    bool GetMoveRelative(Vec3& vPreTrans, QuatTS& qtMove) const;
    void UpdateForce();

    bool HasForce() const
    { return (m_pForce != 0); }

    int EmitParticle(SParticleUpdateContext& context, const EmitParticleData& data, float fAge = 0.f, QuatTS* plocPreTransform = NULL);

    int SpawnBeamEmitter(const ResourceParticleParams& params, SParticleUpdateContext& context, float fAge, EmitParticleData data);
    void WalkAlongWaveForm(const ResourceParticleParams& params, Matrix34& mat, const float& currentStep, const float& totalSteps, float totalLength, const Vec3& segmentSpawnStep, const Vec3& FinalOrigin, const Vec3& NormalizedUp, CPNoise3& noise, const float& tangent);
    void GetEmitterBounds(Vec3& emitterSize) const;
    void UpdateEmitterBounds();
    void EmitParticles(SParticleUpdateContext& context);

    uint32 GetEmitIndex() const
    { return m_nEmitIndex; }
    uint16 GetSequence() const
    { return m_nSequenceIndex; }
    CChaosKey GetChaosKey() const
    { return m_ChaosKey; }

    Vec3 GetCurrentPosIncrementXYZ() const
    {
        return m_vPosIncrementXYZ;
    }

    void UpdatePosIncrementXYZ(const Vec3& delta)
    {
        m_vPosIncrementXYZ += delta;
    }

    Quat GetRelativeRotation() const
    {
        return m_prevRelativeRotation;
    }

    void SetRelativeRotation(const Quat& relativeRotation)
    {
        m_prevRelativeRotation = relativeRotation;
    }

    void OffsetPosition(const Vec3& delta)
    {
        m_LastLoc.t += delta;
    }

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
    }

    float GetBeamSegmentCount() const
    {
        return m_fbeamSegmentCount;
    }
private:
    // Associated structures.
    CParticleContainer*                 m_pContainer;                   // Direct or shared container to emit particles into.
    _smart_ptr<CParticleSource> m_pSource;

    Audio::TAudioControlID                          m_nStartAudioTriggerID;
    Audio::TAudioControlID                          m_nStopAudioTriggerID;
    Audio::TAudioControlID                          m_nAudioRtpcID;
    Audio::IAudioProxy*                             m_pIAudioProxy;

    bool                    m_bExecuteAudioTrigger;

    // State.
    float                   m_fStartAge;            // Relative age when scheduled to start (default 0).
    float                   m_fStopAge;             // Relative age when scheduled to end (fHUGE if never).
    float                   m_fRepeatAge;           // Relative age when scheduled to repeat (fHUGE if never).
    float                   m_fLastEmitAge;     // Age of emission of last particle.
    float                   m_fActivateAge;     // Cached age for last activation mode.

    CChaosKey           m_ChaosKey;             // Seed for randomising; inited every pulse.
    QuatTS              m_LastLoc;              // Location at time of last update.

    Vec3 m_vPosIncrementXYZ;
    Quat m_prevRelativeRotation;

    uint32              m_nEmitIndex;
    uint16              m_nSequenceIndex;

    // External objects.
    IPhysicalEntity*        m_pForce;
    Vec3 m_emitterBounds;

    // Methods.
    void Initialize(float fAge);
    float GetAgeRelativeTo(float fStopAge, float fAgeAdjust = 0.f) const;
    void    DeactivateAudio();
    float ComputeDensityIncrease(float fStrength, float fParticleLife, const QuatTS& locA, const QuatTS* plocB) const;
    int SpawnParticleToContainer(const ResourceParticleParams& params, SParticleUpdateContext& context, float fAge, EmitParticleData data, QuatTS* plocPreTransform, const float uvOffset,
        uint segmentCount = 0, Vec3 segmentStep = Vec3(0.f), const bool isEdgeParticle = false);
    float m_fbeamSegmentCount;
} _ALIGN(16);

#endif // CRYINCLUDE_CRY3DENGINE_PARTICLESUBEMITTER_H
