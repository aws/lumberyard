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

#pragma once

#include <IEditor.h>

struct IParticleEffect;
struct IParticleEmitter;
struct ISkeletonAnim;
struct ISkeletonPose;
struct IDefaultSkeleton;
struct SRendParams;
struct SRenderingPassInfo;

namespace CharacterTool {
    class EffectPlayer
        : public IEditorNotifyListener
    {
    public:
        EffectPlayer();
        ~EffectPlayer();

        void SetSkeleton(ISkeletonAnim* pSkeletonAnim, ISkeletonPose* pSkeletonPose, IDefaultSkeleton* pIDefaultSkeleton);
        void Update(const QuatT& rPhysEntity);
        void SpawnEffect(int animID, const char* animName, const char* effectName, const char* boneName, const Vec3& offset, const Vec3& dir);
        void KillAllEffects();
        void Render(SRendParams& params, const SRenderingPassInfo& passInfo);

        // IEditorNotifyListener
        void OnEditorNotifyEvent(EEditorNotifyEvent event) override;
        // ~IEditorNotifyListener

    private:
        struct EffectEntry
        {
            EffectEntry(const _smart_ptr<IParticleEffect>& pEffect, const _smart_ptr<IParticleEmitter>& pEmitter, int boneID, const Vec3& offset, const Vec3& dir, int animID);
            ~EffectEntry();

            _smart_ptr<IParticleEffect> pEffect;
            _smart_ptr<IParticleEmitter> pEmitter;
            int boneID;
            Vec3 offset;
            Vec3 dir;
            int animID;
        };

        void GetEffectTM(Matrix34& tm, int boneID, const Vec3& offset, const Vec3& dir);
        bool IsPlayingAnimation(int animID);
        bool IsPlayingEffect(const char* effectName);

        ISkeletonAnim* m_pSkeleton2;
        ISkeletonPose* m_pSkeletonPose;
        IDefaultSkeleton* m_pIDefaultSkeleton;
        DynArray<EffectEntry> m_effects;
    };
}
