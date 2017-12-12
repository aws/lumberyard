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

#ifndef CRYINCLUDE_CRYMOVIE_ANIMCAMERANODE_H
#define CRYINCLUDE_CRYMOVIE_ANIMCAMERANODE_H

#pragma once

#include "EntityNode.h"
#include "../CryCommon/PNoise3.h"

template <class ValueType>
class TAnimSplineTrack;
typedef TAnimSplineTrack<Vec2> C2DSplineTrack;

/** Camera node controls camera entity.
*/
class CAnimCameraNode
    : public CAnimEntityNode
{
private:
    // Field of view in DEGREES! To Display it nicely for user.
    float m_fFOV;
    Vec3 m_fDOF;
    float m_fNearZ;
    bool m_bJustActivated;
    uint m_cameraShakeSeedValue;
    CPNoise3 m_pNoiseGen;
    static CAnimCameraNode* m_pLastFrameActiveCameraNode;

    static const int SHAKE_COUNT = 2;
    struct ShakeParam
    {
        Vec3 amplitude;
        float amplitudeMult;
        Vec3 frequency;
        float frequencyMult;
        float noiseAmpMult;
        float noiseFreqMult;
        float timeOffset;

        Vec3 phase;
        Vec3 phaseNoise;

        ShakeParam()
        {
            amplitude = Vec3(1.0f, 1.0f, 1.0f);
            amplitudeMult = 0.0f;
            frequency = Vec3(1.0f, 1.0f, 1.0f);
            frequencyMult = 0.0f;
            noiseAmpMult = 0.0f;
            noiseFreqMult = 0.0f;
            timeOffset = 0.0f;
        }

        Ang3 ApplyCameraShake(CPNoise3& noiseGen, float time, Ang3 angles, C2DSplineTrack* pFreqTrack, EntityId camEntityId, int shakeIndex, const uint shakeSeed);

#if !defined(_RELEASE)
        void ShowCameraShakeDebug(EntityId camEntityId, int shakeIndex, float phaseDelta) const;
#endif //#if !defined(_RELEASE)
    };
    ShakeParam m_shakeParam[SHAKE_COUNT];

    ICVar* m_cv_r_PostProcessEffects;
public:
    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }
    CAnimCameraNode(const int id);
    virtual ~CAnimCameraNode();
    static void Initialize();
    virtual void Animate(SAnimContext& ec);

    bool GetShakeRotation(const float& time, Quat& rot);

    virtual void CreateDefaultTracks();
    virtual void OnReset();
    float GetFOV() { return m_fFOV; }

    virtual void Activate(bool bActivate);

    virtual void OnStop();

    //////////////////////////////////////////////////////////////////////////
    virtual unsigned int GetParamCount() const;
    virtual CAnimParamType GetParamType(unsigned int nIndex) const;

    //////////////////////////////////////////////////////////////////////////
    bool SetParamValue(float time, CAnimParamType param, float value);
    bool SetParamValue(float time, CAnimParamType param, const Vec3& value);
    bool SetParamValue(float time, CAnimParamType param, const Vec4& value);
    bool GetParamValue(float time, CAnimParamType param, float& value);
    bool GetParamValue(float time, CAnimParamType param, Vec3& value);
    bool GetParamValue(float time, CAnimParamType param, Vec4& value);

    virtual void InitializeTrackDefaultValue(IAnimTrack* pTrack, const CAnimParamType& paramType);
    void Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks);

    virtual void SetCameraShakeSeed(const uint cameraShakeSeedValue){m_cameraShakeSeedValue = cameraShakeSeedValue; };

protected:
    virtual bool GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const;
};

#endif // CRYINCLUDE_CRYMOVIE_ANIMCAMERANODE_H
