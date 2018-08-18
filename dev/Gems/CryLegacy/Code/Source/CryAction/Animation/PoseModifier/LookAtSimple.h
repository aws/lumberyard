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

#ifndef CRYINCLUDE_CRYACTION_ANIMATION_POSEMODIFIER_LOOKATSIMPLE_H
#define CRYINCLUDE_CRYACTION_ANIMATION_POSEMODIFIER_LOOKATSIMPLE_H
#pragma once

#include <CryExtension/Impl/ClassWeaver.h>

namespace AnimPoseModifier {
    class CLookAtSimple
        : public IAnimationPoseModifier
    {
    private:
        struct State
        {
            int32 jointId;
            Vec3 jointOffsetRelative;
            Vec3 targetGlobal;
            f32 weight;
        };

    public:
        CRYINTERFACE_BEGIN()
        CRYINTERFACE_ADD(IAnimationPoseModifier)
        CRYINTERFACE_END()

        CRYGENERATE_CLASS(CLookAtSimple, "AnimationPoseModifier_LookAtSimple", 0xba7e2a809970435f, 0xb6679c08df616d74);

    public:
        void SetJointId(uint32 id) { m_state.jointId = id; }
        void SetJointOffsetRelative(const Vec3& offset) { m_state.jointOffsetRelative = offset; }

        void SetTargetGlobal(const Vec3& target) { m_state.targetGlobal = target; }

        void SetWeight(f32 weight) { m_state.weight = weight; }

    private:
        bool ValidateJointId(IDefaultSkeleton& pModelSkeleton);

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
        State m_state;
        State m_stateExecute;
    } _ALIGN(32);
} // namespace AnimPoseModifier

#endif // CRYINCLUDE_CRYACTION_ANIMATION_POSEMODIFIER_LOOKATSIMPLE_H
