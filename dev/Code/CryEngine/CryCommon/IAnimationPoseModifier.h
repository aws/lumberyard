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

// Description : CryAnimation interfaces


#ifndef CRYINCLUDE_CRYCOMMON_IANIMATIONPOSEMODIFIER_H
#define CRYINCLUDE_CRYCOMMON_IANIMATIONPOSEMODIFIER_H
#pragma once

#include <CryExtension/ICryUnknown.h>

#include <AzCore/Component/EntityId.h>

//

struct ISkeletonPose;
struct ISkeletonAnim;

struct IAnimationPoseData;
struct IAnimationPoseModifier;

//

struct SAnimationPoseModifierParams
{
    ICharacterInstance* pCharacterInstance;
    IAnimationPoseData* pPoseData;

    f32 timeDelta;

    QuatTS location;

    ILINE ISkeletonPose* GetISkeletonPose() const { return pCharacterInstance->GetISkeletonPose(); }
    ILINE ISkeletonAnim* GetISkeletonAnim() const  { return pCharacterInstance->GetISkeletonAnim(); }
    ILINE const IDefaultSkeleton& GetIDefaultSkeleton() const { return pCharacterInstance->GetIDefaultSkeleton(); }
};

//

struct IAnimationPoseData
{
    // <interfuscator:shuffle>
    virtual ~IAnimationPoseData() { };

    virtual uint GetJointCount() const = 0;

    virtual void SetJointRelative(const uint index, const QuatT& transformation) = 0;
    virtual void SetJointRelativeP(const uint index, const Vec3& position) = 0;
    virtual void SetJointRelativeO(const uint index, const Quat& orientation) = 0;
    virtual const QuatT& GetJointRelative(const uint index) const = 0;

    virtual void SetJointAbsolute(const uint index, const QuatT& transformation) = 0;
    virtual void SetJointAbsoluteP(const uint index, const Vec3& position) = 0;
    virtual void SetJointAbsoluteO(const uint index, const Quat& orientation) = 0;
    virtual const QuatT& GetJointAbsolute(const uint index) const = 0;
    // </interfuscator:shuffle>
};

//

struct IAnimationPoseModifier
    : public ICryUnknown
{
    CRYINTERFACE_DECLARE(IAnimationPoseModifier, 0x22fe47755e42447f, 0xbab6274ed39af449)
    template<class T>
    friend void AZStd::checked_delete(T* x);

    // <interfuscator:shuffle>
    // Command Buffer. Pose data will not be available at this stage.
    // Called from the main thread before the Pose Modifier is added to the
    virtual bool Prepare(const SAnimationPoseModifierParams& params) = 0;

    // Called from an arbitrary worker thread when the Command associated with
    // this Pose Modifier is executed. Pose data is available for read/write.
    virtual bool Execute(const SAnimationPoseModifierParams& params) = 0;

    // Called from the main thread after the Command Buffer this Pose Modifier
    // was part of finished its execution.
    virtual void Synchronize() = 0;

    virtual void GetMemoryUsage(ICrySizer* pSizer) const = 0;
    // </interfuscator:shuffle>
};

DECLARE_SMART_POINTERS(IAnimationPoseModifier);

//

struct IAnimationOperatorQueue
    : public IAnimationPoseModifier
{
    CRYINTERFACE_DECLARE(IAnimationOperatorQueue, 0x686a56d5215d44dd, 0xa166ccf13327d8a2)

    enum EOp
    {
        eOp_Override,
        eOp_OverrideRelative,
        eOp_OverrideWorld,
        eOp_Additive,
        eOp_AdditiveRelative,
    };

    // <interfuscator:shuffle>
    virtual void PushPosition(uint32 jointIndex, EOp eOp, const Vec3& value) = 0;
    virtual void PushOrientation(uint32 jointIndex, EOp eOp, const Quat& value) = 0;

    virtual void PushStoreRelative(uint32 jointIndex, QuatT& output) = 0;
    virtual void PushStoreAbsolute(uint32 jointIndex, QuatT& output) = 0;
    virtual void PushStoreWorld(uint32 jointIndex, QuatT& output) = 0;

    virtual void PushComputeAbsolute() = 0;

    virtual void Clear() = 0;
    // </interfuscator:shuffle>
};

DECLARE_SMART_POINTERS(IAnimationOperatorQueue);

//

struct IAnimationPoseBlenderDir
    : public IAnimationPoseModifier
{
    CRYINTERFACE_DECLARE(IAnimationPoseBlenderDir, 0x1725a49dbd684ff4, 0x852cd0d4b7f86c28)

    // <interfuscator:shuffle>
    virtual void SetState(bool state) = 0;
    virtual void SetTarget(const Vec3& target) = 0;
    virtual void SetLayer(uint32 nLayer) = 0;
    virtual void SetFadeoutAngle(f32 angleRadians) = 0;
    virtual void SetFadeOutSpeed(f32 time) = 0;
    virtual void SetFadeInSpeed(f32 time) = 0;
    virtual void SetFadeOutMinDistance(f32 minDistance) = 0;
    virtual void SetPolarCoordinatesOffset(const Vec2& offset) = 0;
    virtual void SetPolarCoordinatesSmoothTimeSeconds(f32 smoothTimeSeconds) = 0;
    virtual void SetPolarCoordinatesMaxRadiansPerSecond(const Vec2& maxRadiansPerSecond) = 0;
    virtual f32 GetBlend() const = 0;
    // </interfuscator:shuffle>
};

DECLARE_SMART_POINTERS(IAnimationPoseBlenderDir);

//

struct IAnimationGroundAlignment
    : public IAnimationPoseModifier
{
    CRYINTERFACE_DECLARE(IAnimationGroundAlignment, 0xb8bf63b98d304d7b, 0xaaa5fbdf665715b2)

    virtual void SetData(const bool bAlignSkeletonVertical, const f32 rootHeight, const Plane& planeLeft, const Plane& planeRight) = 0;
};

DECLARE_SMART_POINTERS(IAnimationGroundAlignment);

//

struct IAnimationPoseAlignerChain
    : public IAnimationPoseModifier
{
    CRYINTERFACE_DECLARE(IAnimationPoseAlignerChain, 0xf5d18a45824945b5, 0x9f68e45aa9687c4a)

    enum EType
    {
        eType_Limb
    };

    enum ELockMode
    {
        eLockMode_Off,
        eLockMode_Store,
        eLockMode_Apply,
    };

    struct STarget
    {
        Plane plane;
        float distance;
        float offsetMin;
        float offsetMax;
        float targetWeight;
        float alignWeight;
    };

    // <interfuscator:shuffle>
    virtual void Initialize(LimbIKDefinitionHandle solver, int contactJointIndex) = 0;

    virtual void SetTarget(const STarget& target) = 0;
    virtual void SetTargetLock(ELockMode eLockMode) = 0;
    // </interfuscator:shuffle>
};

DECLARE_SMART_POINTERS(IAnimationPoseAlignerChain);

//

struct IAnimationPoseMatching
    : public IAnimationPoseModifier
{
    CRYINTERFACE_DECLARE(IAnimationPoseMatching, 0xa988bda559404438, 0xb69a1f57e1301815)

    // <interfuscator:shuffle>
    virtual void SetAnimations(const uint* pAnimationIds, uint count) = 0;
    virtual bool GetMatchingAnimation(uint& animationId) const = 0;
    // </interfuscator:shuffle>
};

DECLARE_SMART_POINTERS(IAnimationPoseMatching);

//

struct IAnimationPoseAligner
    : public ICryUnknown
{
    CRYINTERFACE_DECLARE(IAnimationPoseAligner, 0x5c852e726d447cb0, 0x9f7f5c80c41b429a)

    // <interfuscator:shuffle>
    virtual bool Initialize(IEntity& entity) { return false; }
    virtual bool Initialize(AZ::EntityId id, ICharacterInstance& character) { return false; }
    virtual void Clear() = 0;

    virtual void SetRootOffsetEnable(bool bEnable) = 0;
    virtual void SetBlendWeight(float weight) = 0;

    virtual void Update(const QuatT& location, const float time) = 0;
    // </interfuscator:shuffle>
};

DECLARE_SMART_POINTERS(IAnimationPoseAligner);

#endif // CRYINCLUDE_CRYCOMMON_IANIMATIONPOSEMODIFIER_H
