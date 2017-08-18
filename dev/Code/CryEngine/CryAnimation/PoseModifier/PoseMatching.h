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

#ifndef CRYINCLUDE_CRYANIMATION_POSEMODIFIER_POSEMATCHING_H
#define CRYINCLUDE_CRYANIMATION_POSEMODIFIER_POSEMATCHING_H
#pragma once

#include <IAnimationPoseModifier.h>

class CPoseMatching
    : public IAnimationPoseMatching
{
public:
    CRYINTERFACE_BEGIN()
    CRYINTERFACE_ADD(IAnimationPoseModifier)
    CRYINTERFACE_ADD(IAnimationPoseMatching)
    CRYINTERFACE_END()

    CRYGENERATE_CLASS(CPoseMatching, "AnimationPoseModifier_PoseMatching", 0x18318a272246464e, 0xa4b7adffa51a9508)

    // IAnimationPoseMatching
public:
    virtual void SetAnimations(const uint* pAnimationIds, uint count);
    virtual bool GetMatchingAnimation(uint& animationId) const;

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
    uint m_jointStartIndex;
    const uint* m_pAnimationIds;
    uint m_animationCount;

    int m_animationMatchId;
};

#endif // CRYINCLUDE_CRYANIMATION_POSEMODIFIER_POSEMATCHING_H
