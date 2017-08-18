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

#include <AzCore/Casting/numeric_cast.h>

#ifndef CRYINCLUDE_CRYANIMATION_POSEMODIFIER_POSEBLENDERLOOK_H
#define CRYINCLUDE_CRYANIMATION_POSEMODIFIER_POSEBLENDERLOOK_H
#pragma once

class CAttachmentBONE;

class CPoseBlenderLook
    : public IAnimationPoseBlenderDir
{
    CRYINTERFACE_BEGIN()
    CRYINTERFACE_ADD(IAnimationPoseModifier)
    CRYINTERFACE_ADD(IAnimationPoseBlenderDir)
    CRYINTERFACE_END()

    CRYGENERATE_CLASS(CPoseBlenderLook, "AnimationPoseModifier_PoseBlenderLook", 0x058c3e18b9874faf, 0x8989b9cb2cff0d64)

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
    //just for final fix-up
    virtual void Synchronize();

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }

private:
    ILINE void LimitEye(Vec3& eyeAimDirection) const
    {
        const float yawRadians = atan2_tpl(-eyeAimDirection.x, eyeAimDirection.y);
        const float pitchRadians = asin(eyeAimDirection.z);

        const float clampedYawRadians = clamp_tpl(yawRadians, -m_eyeLimitHalfYawRadians, m_eyeLimitHalfYawRadians);
        const float clampedPitchRadians = clamp_tpl(pitchRadians, -m_eyeLimitPitchRadiansDown, m_eyeLimitPitchRadiansUp);

        eyeAimDirection = Quat::CreateRotationZ(clampedYawRadians) * Quat::CreateRotationX(clampedPitchRadians) * Vec3(0, 1, 0);
    }

    bool PrepareInternal(const SAnimationPoseModifierParams& params);


public:

    CAttachmentBONE* m_pAttachmentEyeLeft;
    CAttachmentBONE* m_pAttachmentEyeRight;
    QuatT m_ql;
    QuatT m_qr;
    int32 m_EyeIdxL;
    int32 m_EyeIdxR;

    f32 m_eyeLimitHalfYawRadians;
    f32 m_eyeLimitPitchRadiansUp;
    f32 m_eyeLimitPitchRadiansDown;

    Quat m_additionalRotationLeft;
    Quat m_additionalRotationRight;

    SDirectionalBlender m_blender;
};


#endif // CRYINCLUDE_CRYANIMATION_POSEMODIFIER_POSEBLENDERLOOK_H
