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

#ifndef CRYINCLUDE_CRYANIMATION_POSEMODIFIER_RECOIL_H
#define CRYINCLUDE_CRYANIMATION_POSEMODIFIER_RECOIL_H
#pragma once

namespace PoseModifier {
    class CRecoil
        : public IAnimationPoseModifier
    {
    public:
        struct State
        {
            f32 time;
            f32 duration;
            f32 strengh;
            f32 kickin;
            f32 displacerad;
            uint32 arms;

            State()
            {
                Reset();
            }

            void Reset()
            {
                time = 100.0f;
                duration = 0.0f;
                strengh = 0.0f;
                kickin = 0.8f;
                displacerad = 0.0f;
                arms = 3;
            }
        };

    public:
        CRYINTERFACE_BEGIN()
        CRYINTERFACE_ADD(IAnimationPoseModifier)
        CRYINTERFACE_END()

        CRYGENERATE_CLASS(CRecoil, "AnimationPoseModifier_Recoil", 0xd7900cb9e7be4825, 0x99e1cc1211f9c561)

    public:
        void SetState(const State& state) { m_state = state; m_bStateUpdate = true; }

    private:
        f32 RecoilEffect(f32 t);

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
        bool m_bStateUpdate;
    } _ALIGN(32);
} // namespace PoseModifier

#endif // CRYINCLUDE_CRYANIMATION_POSEMODIFIER_RECOIL_H
