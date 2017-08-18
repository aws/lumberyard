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

#ifndef CRYINCLUDE_CRYANIMATION_POSEMODIFIER_POSEBLENDERAIM_H
#define CRYINCLUDE_CRYANIMATION_POSEMODIFIER_POSEBLENDERAIM_H
#pragma once

#include <CryExtension/Impl/ClassWeaver.h>
#include <AzCore/Casting/numeric_cast.h>



class CPoseBlenderAim
    : public IAnimationPoseBlenderDir
{
    CRYINTERFACE_BEGIN()
    CRYINTERFACE_ADD(IAnimationPoseModifier)
    CRYINTERFACE_ADD(IAnimationPoseBlenderDir)
    CRYINTERFACE_END()

    CRYGENERATE_CLASS(CPoseBlenderAim, "AnimationPoseModifier_PoseBlenderAim", 0x058c3e18b9574faf, 0x8989b9cb2cff0d64)

    // This interface
public:
    void SetState(bool state) { m_blender.m_Set.bUseDirIK = state; }
    void SetTarget(const Vec3& target)
    {
        if (target.IsValid())
        {
            m_blender.m_Set.vDirIKTarget = target;
        }
    }
    void SetLayer(uint32 layer) { m_blender.m_Set.nDirLayer = aznumeric_caster(MAX(layer, 1)); }
    void SetFadeoutAngle(f32 angleRadians) { m_blender.m_Set.fDirIKFadeoutRadians = angleRadians; }
    void SetFadeOutSpeed(f32 time) { m_blender.m_Set.fDirIKFadeOutTime = (time > 0.0f) ? 1.0f / time : FLT_MAX; }
    void SetFadeInSpeed(f32 time) { m_blender.m_Set.fDirIKFadeInTime = (time > 0.0f) ? 1.0f / time : FLT_MAX; }
    void SetFadeOutMinDistance(f32 minDistance) { m_blender.m_Set.fDirIKMinDistanceSquared = minDistance * minDistance; }
    void SetPolarCoordinatesOffset(const Vec2& offset) { m_blender.m_Set.vPolarCoordinatesOffset = offset; }
    void SetPolarCoordinatesSmoothTimeSeconds(f32 smoothTimeSeconds) { m_blender.m_Set.fPolarCoordinatesSmoothTimeSeconds = smoothTimeSeconds; }
    void SetPolarCoordinatesMaxRadiansPerSecond(const Vec2& maxRadiansPerSecond) { m_blender.m_Set.vPolarCoordinatesMaxRadiansPerSecond = maxRadiansPerSecond; }
    f32 GetBlend() const { return m_blender.m_Get.fDirIKInfluence; }

public:
    //high-level setup code. most of it it redundant when we switch to VEGs
    virtual bool Prepare(const SAnimationPoseModifierParams& params);
    //brute force pose-blending in Jobs
    virtual bool Execute(const SAnimationPoseModifierParams& params);
    //sometimes use for final adjustments
    virtual void Synchronize() { m_blender.m_Get = m_blender.m_dataOut; };

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }

private:
    bool PrepareInternal(const SAnimationPoseModifierParams& params);


public:
    SDirectionalBlender m_blender;
};



#endif // CRYINCLUDE_CRYANIMATION_POSEMODIFIER_POSEBLENDERAIM_H
