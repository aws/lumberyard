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

#ifndef CRYINCLUDE_CRYANIMATION_POSEMODIFIER_POSEALIGNERCHAIN_H
#define CRYINCLUDE_CRYANIMATION_POSEMODIFIER_POSEALIGNERCHAIN_H
#pragma once

class CPoseAlignerChain
    : public IAnimationPoseAlignerChain
{
private:
    struct SState
    {
        LimbIKDefinitionHandle solver;
        int rootJointIndex;
        int targetJointIndex;
        int contactJointIndex;

        STarget target;
        ELockMode eLockMode;
    };

public:
    CRYINTERFACE_BEGIN()
    CRYINTERFACE_ADD(IAnimationPoseModifier)
    CRYINTERFACE_ADD(IAnimationPoseAlignerChain)
    CRYINTERFACE_END()

    CRYGENERATE_CLASS(CPoseAlignerChain, "AnimationPoseModifier_PoseAlignerChain", 0x6de1ac5816c94c33, 0x8fecf3ee1f6bdf11)

public:
    virtual void Initialize(LimbIKDefinitionHandle solver, int contactJointIndex);
    virtual void SetTarget(const STarget& target) { m_state.target = target; }
    virtual void SetTargetLock(ELockMode eLockMode) { m_state.eLockMode = eLockMode; }

private:
    bool StoreAnimationPose(const SAnimationPoseModifierParams& params, const IKLimbType& chain, QuatT* pRelative, QuatT* pAbsolute);
    void RestoreAnimationPose(const SAnimationPoseModifierParams& params, const IKLimbType& chain, QuatT* pRelative, QuatT* pAbsolute);
    void BlendAnimateionPose(const SAnimationPoseModifierParams& params, const IKLimbType& chain, QuatT* pRelative, QuatT* pAbsolute);

    void AlignToTarget(const SAnimationPoseModifierParams& params);
    bool MoveToTarget(const SAnimationPoseModifierParams& params, Vec3& contactPosition);

    // IAnimationPoseModifier
public:
    virtual bool Prepare(const SAnimationPoseModifierParams& params);
    virtual bool Execute(const SAnimationPoseModifierParams& params);
    virtual void Synchronize();

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }

private:
    SState m_state;
    SState m_stateExecute;

    // Execute states
    const IKLimbType* m_pIkLimbType;
    Vec3 m_targetLockPosition;
    Quat m_targetLockOrientation;
    Plane  m_targetLockPlane;
};

#endif // CRYINCLUDE_CRYANIMATION_POSEMODIFIER_POSEALIGNERCHAIN_H
