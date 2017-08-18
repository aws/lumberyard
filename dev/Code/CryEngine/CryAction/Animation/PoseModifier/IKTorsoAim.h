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

#ifndef CRYINCLUDE_CRYACTION_ANIMATION_POSEMODIFIER_IKTORSOAIM_H
#define CRYINCLUDE_CRYACTION_ANIMATION_POSEMODIFIER_IKTORSOAIM_H
#pragma once

#include <CryExtension/Impl/ClassWeaver.h>

#define TORSOAIM_MAX_JOINTS 5

class CIKTorsoAim
    : public IAnimationPoseModifier
{
public:
    CRYINTERFACE_BEGIN()
    CRYINTERFACE_ADD(IAnimationPoseModifier)
    CRYINTERFACE_END()

    CRYGENERATE_CLASS(CIKTorsoAim, "AnimationPoseModifier_IKTorsoAim", 0x2058e99dd05243e2, 0x88985eff40b942e4)

public:
    void Enable(bool enable);
    static void InitCVars();

    // IAnimationIKTorsoAim
public:
    virtual void SetBlendWeight(float blend);
    virtual void SetBlendWeightPosition(float blend);
    virtual void SetTargetDirection(const Vec3& direction);
    virtual void SetViewOffset(const QuatT& offset);
    virtual void SetAbsoluteTargetPosition(const Vec3& targetPosition);
    virtual void SetFeatherWeights(uint32 weights, const f32* customBlends);
    virtual void SetJoints(uint32 jntEffector, uint32 jntAdjustment);
    virtual void SetBaseAnimTrackFactor(float factor);

    // IAnimationPoseModifier
public:
    virtual bool Prepare(const SAnimationPoseModifierParams& params);
    virtual bool Execute(const SAnimationPoseModifierParams& params);
    virtual void Synchronize();

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }

    const QuatT& GetLastProcessedEffector() const
    {
        return m_lastProcessedEffector;
    }

private:

    struct SParams
    {
        SParams();

        QuatT viewOffset;
        Vec3  targetDirection;
        Vec3  absoluteTargetPosition;
        float baseAnimTrackFactor;
        float weights[TORSOAIM_MAX_JOINTS];
        float blend;
        float blendPosition;
        int   numParents;
        int   effectorJoint;
        int   aimJoint;
    };

    struct STorsoAim_CVars
    {
        STorsoAim_CVars()
            : m_initialized(false)
        {
        }

        ~STorsoAim_CVars()
        {
            ReleaseCVars();
        }

        void InitCvars();

        int     STAP_DEBUG;
        int     STAP_DISABLE;
        int     STAP_TRANSLATION_FUDGE;
        int     STAP_TRANSLATION_FEATHER;
        int     STAP_LOCK_EFFECTOR;
        float STAP_OVERRIDE_TRACK_FACTOR;

    private:

        void ReleaseCVars();

        bool  m_initialized;
    };

    SParams m_params;
    SParams m_setParams;

    bool m_active;
    bool m_blending;
    float m_blendRate;

    Vec3 m_aimToEffector;
    Quat m_effectorDirQuat;
    QuatT m_lastProcessedEffector;

    static STorsoAim_CVars s_CVars;

    void Init();
} _ALIGN(32);

#endif // CRYINCLUDE_CRYACTION_ANIMATION_POSEMODIFIER_IKTORSOAIM_H
