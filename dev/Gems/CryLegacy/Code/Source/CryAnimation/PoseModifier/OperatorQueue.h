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

#ifndef CRYINCLUDE_CRYANIMATION_POSEMODIFIER_OPERATORQUEUE_H
#define CRYINCLUDE_CRYANIMATION_POSEMODIFIER_OPERATORQUEUE_H
#pragma once

#include "CharacterManager.h"
#include "SkeletonPose.h"

namespace OperatorQueue
{
    struct SValue
    {
        f32 n[4];
        QuatT* p;
    };

    struct SOp
    {
        uint32 joint;
        uint16 target;
        uint16 op;
        SValue value;
    };
}   // OperatorQueue

class COperatorQueue
    : public IAnimationOperatorQueue
{
public:
    CRYINTERFACE_BEGIN()
    CRYINTERFACE_ADD(IAnimationPoseModifier)
    CRYINTERFACE_ADD(IAnimationOperatorQueue)
    CRYINTERFACE_END()

    CRYGENERATE_CLASS(COperatorQueue, "AnimationPoseModifier_OperatorQueue", 0xac90f2bc76a843ec, 0x9970463fb080a520);

private:
    enum EOpInternal
    {
        eOpInternal_OverrideAbsolute = eOp_Override,
        eOpInternal_OverrideRelative = eOp_OverrideRelative,
        eOpInternal_OverrideWorld    = eOp_OverrideWorld,
        eOpInternal_AdditiveAbsolute = eOp_Additive,
        eOpInternal_AdditiveRelative = eOp_AdditiveRelative,

        eOpInternal_StoreRelative,
        eOpInternal_StoreAbsolute,
        eOpInternal_StoreWorld,

        eOpInternal_ComputeAbsolute,
    };

    enum ETarget
    {
        eTarget_Position,
        eTarget_Orientation,
    };

public:
    virtual void PushPosition(uint32 jointIndex, EOp eOp, const Vec3& value);
    virtual void PushOrientation(uint32 jointIndex, EOp eOp, const Quat& value);

    virtual void PushStoreRelative(uint32 jointIndex, QuatT& output);
    virtual void PushStoreAbsolute(uint32 jointIndex, QuatT& output);
    virtual void PushStoreWorld(uint32 jointIndex, QuatT& output);

    virtual void PushComputeAbsolute();

    virtual void Clear();

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
    std::vector<OperatorQueue::SOp> m_ops[2];
    uint32 m_current;
} _ALIGN(32);

#endif // CRYINCLUDE_CRYANIMATION_POSEMODIFIER_OPERATORQUEUE_H
