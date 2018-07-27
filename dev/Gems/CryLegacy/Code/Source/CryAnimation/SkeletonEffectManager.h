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

#ifndef CRYINCLUDE_CRYANIMATION_SKELETONEFFECTMANAGER_H
#define CRYINCLUDE_CRYANIMATION_SKELETONEFFECTMANAGER_H
#pragma once

struct ISkeletonAnim;
class CCharInstance;

class CSkeletonEffectManager
{
public:
    CSkeletonEffectManager();
    ~CSkeletonEffectManager();

    void Update(ISkeletonAnim* pSkeletonAnim, ISkeletonPose* pSkeletonPose, const QuatTS& entityLoc);
    void SpawnEffect(CCharInstance* pCharInstance, int animID, const char* animName, const char* effectName, const char* boneName, const char* secondBoneName, const Vec3& offset, const Vec3& dir, const QuatTS& entityLoc);
    void KillAllEffects();
    size_t SizeOfThis()
    {
        return m_effects.capacity() * sizeof(EffectEntry);
    }
    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(m_effects);
    }

private:
    struct EffectEntry
    {
        EffectEntry(_smart_ptr<IParticleEffect> pEffect, _smart_ptr<IParticleEmitter> pEmitter, int boneID, int secondBoneID, const Vec3& offset, const Vec3& dir, int animID);
        ~EffectEntry();
        void GetMemoryUsage(ICrySizer* pSizer) const{}

        _smart_ptr<IParticleEffect> pEffect;
        _smart_ptr<IParticleEmitter> pEmitter;
        int boneID;
        int secondBoneID;
        Vec3 offset;
        Vec3 dir;
        int animID;
    };


    void GetEffectLoc(ISkeletonPose* pSkeletonPose, QuatTS& loc, int boneID, int secondBoneID, const Vec3& offset, const Vec3& dir, const QuatTS& entityLoc);
    bool IsPlayingAnimation(ISkeletonAnim* pSkeletonAnim, int animID);
    bool IsPlayingEffect(const char* effectName);

    DynArray<EffectEntry> m_effects;
};

#endif // CRYINCLUDE_CRYANIMATION_SKELETONEFFECTMANAGER_H
