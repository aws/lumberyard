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

#include "CryLegacy_precompiled.h"
#include "SkeletonEffectManager.h"
#include "CharacterInstance.h"
#include "ICryAnimation.h"
#include "cvars.h"

CSkeletonEffectManager::CSkeletonEffectManager()
{
}

CSkeletonEffectManager::~CSkeletonEffectManager()
{
    KillAllEffects();
}

void CSkeletonEffectManager::Update(ISkeletonAnim* pSkeleton, ISkeletonPose* pSkeletonPose, const QuatTS& entityLoc)
{
    for (int i = 0; i < m_effects.size(); )
    {
        EffectEntry& entry = m_effects[i];

        // If the animation has stopped, kill the effect.
        bool effectStillPlaying = (entry.pEmitter ? entry.pEmitter->IsAlive() : false);
        bool animStillPlaying = IsPlayingAnimation(pSkeleton, entry.animID);
        if (animStillPlaying && effectStillPlaying)
        {
            // Update the effect position.
            QuatTS loc;
            GetEffectLoc(pSkeletonPose, loc, entry.boneID, entry.secondBoneID, entry.offset, entry.dir, entityLoc);
            if (entry.pEmitter)
            {
                entry.pEmitter->SetLocation(loc);
            }

            ++i;
        }
        else
        {
            if (Console::GetInst().ca_DebugSkeletonEffects > 0)
            {
                CryLogAlways("CSkeletonEffectManager::Update(this=%p): Killing effect \"%s\" because %s.", this,
                    (m_effects[i].pEffect ? m_effects[i].pEffect->GetName() : "<EFFECT NULL>"), (effectStillPlaying ? "animation has ended" : "effect has ended"));
            }
            if (m_effects[i].pEmitter)
            {
                m_effects[i].pEmitter->Activate(false);
            }
            m_effects.erase(m_effects.begin() + i);
        }
    }
}

void CSkeletonEffectManager::KillAllEffects()
{
    if (Console::GetInst().ca_DebugSkeletonEffects)
    {
        for (int effectIndex = 0, effectCount = m_effects.size(); effectIndex < effectCount; ++effectIndex)
        {
            IParticleEffect* pEffect = m_effects[effectIndex].pEffect;
            CryLogAlways("CSkeletonEffectManager::KillAllEffects(this=%p): Killing effect \"%s\" because animated character is in simplified movement.", this, (pEffect ? pEffect->GetName() : "<EFFECT NULL>"));
        }
    }

    for (int i = 0, count = m_effects.size(); i < count; ++i)
    {
        if (m_effects[i].pEmitter)
        {
            m_effects[i].pEmitter->Activate(false);
        }
    }

    m_effects.clear();
}

void CSkeletonEffectManager::SpawnEffect(CCharInstance* pCharInstance, int animID, const char* animName, const char* effectName, const char* boneName, const char* secondBoneName, const Vec3& offset, const Vec3& dir, const QuatTS& entityLoc)
{
    ISkeletonPose* pISkeletonPose =  pCharInstance->GetISkeletonPose();
    const IDefaultSkeleton& rIDefaultSkeleton =  pCharInstance->GetIDefaultSkeleton();

    // Check whether we are already playing this effect, and if so dont restart it.
    bool alreadyPlayingEffect = false;
    if (!Console::GetInst().ca_AllowMultipleEffectsOfSameName)
    {
        alreadyPlayingEffect = IsPlayingEffect(effectName);
    }

    if (alreadyPlayingEffect)
    {
        if (Console::GetInst().ca_DebugSkeletonEffects)
        {
            CryLogAlways("CSkeletonEffectManager::SpawnEffect(this=%p): Refusing to start effect \"%s\" because effect is already running.", this, (effectName ? effectName : "<MISSING EFFECT NAME>"));
        }
    }
    else
    {
        IParticleEffect* pEffect = gEnv->pParticleManager->FindEffect(effectName);

#if !defined(DEDICATED_SERVER)
        if (!gEnv->IsDedicated() && !pEffect)
        {
            gEnv->pLog->LogError("Anim events cannot find effect \"%s\", requested by animation \"%s\".", (effectName ? effectName : "<MISSING EFFECT NAME>"), (animName ? animName : "<MISSING ANIM NAME>"));
        }
#endif
        int boneID = (boneName && boneName[0] ? rIDefaultSkeleton.GetJointIDByName(boneName) : -1);
        boneID = (boneID == -1 ? 0 : boneID);
        QuatTS loc;

        int secondBoneID = (secondBoneName && secondBoneName[0] ? rIDefaultSkeleton.GetJointIDByName(secondBoneName) : -1);

        GetEffectLoc(pISkeletonPose, loc, boneID, secondBoneID, offset, dir, entityLoc);
        IParticleEmitter* pEmitter = (pEffect ? pEffect->Spawn(false, loc) : 0);
        if (pEffect && pEmitter)
        {
            if (Console::GetInst().ca_DebugSkeletonEffects)
            {
                CryLogAlways("CSkeletonEffectManager::SpawnEffect(this=%p): starting effect \"%s\", requested by animation \"%s\".", this, (effectName ? effectName : "<MISSING EFFECT NAME>"), (animName ? animName : "<MISSING ANIM NAME>"));
            }
            m_effects.push_back(EffectEntry(pEffect, pEmitter, boneID, secondBoneID, offset, dir, animID));
        }
    }
}

CSkeletonEffectManager::EffectEntry::EffectEntry(_smart_ptr<IParticleEffect> _pEffect, _smart_ptr<IParticleEmitter> _pEmitter, int _boneID, int _secondBoneID, const Vec3& _offset, const Vec3& _dir, int _animID)
    :   pEffect(_pEffect)
    , pEmitter(_pEmitter)
    , boneID(_boneID)
    , secondBoneID(_secondBoneID)
    , offset(_offset)
    , dir(_dir)
    , animID(_animID)
{
}

CSkeletonEffectManager::EffectEntry::~EffectEntry()
{
}

void CSkeletonEffectManager::GetEffectLoc(ISkeletonPose* pSkeletonPose, QuatTS& loc, int boneID, int secondBoneID, const Vec3& offset, const Vec3& dir, const QuatTS& entityLoc)
{
    if (dir.len2() > 0)
    {
        loc.q = Quat::CreateRotationXYZ(Ang3(dir * 3.14159f / 180.0f));
    }
    else
    {
        loc.q.SetIdentity();
    }
    loc.t = offset;
    loc.s = 1.f;

    if (pSkeletonPose && secondBoneID < 0)
    {
        loc = pSkeletonPose->GetAbsJointByID(boneID) * loc;
    }
    else if (pSkeletonPose)
    {
        loc = loc.CreateNLerp(pSkeletonPose->GetAbsJointByID(boneID), pSkeletonPose->GetAbsJointByID(secondBoneID), 0.5f) * loc;
    }
    loc = entityLoc * loc;
}

bool CSkeletonEffectManager::IsPlayingAnimation(ISkeletonAnim* pSkeletonAnim, int animID)
{
    // Check whether the animation has stopped.

    for (int layer = 0; layer < ISkeletonAnim::LayerCount; ++layer)
    {
        for (int animIndex = 0, animCount = (pSkeletonAnim ? pSkeletonAnim->GetNumAnimsInFIFO(layer) : 0); animIndex < animCount; ++animIndex)
        {
            const CAnimation& anim = pSkeletonAnim->GetAnimFromFIFO(layer, animIndex);
            int32 id = anim.GetAnimationId();

            if (id == animID)
            {
                return true;
            }
        }
    }

    return false;
}

bool CSkeletonEffectManager::IsPlayingEffect(const char* effectName)
{
    bool isPlaying = false;
    for (int effectIndex = 0, effectCount = m_effects.size(); effectIndex < effectCount; ++effectIndex)
    {
        IParticleEffect* pEffect = m_effects[effectIndex].pEffect;
        if (pEffect && _stricmp(pEffect->GetName(), effectName) == 0)
        {
            isPlaying = true;
        }
    }
    return isPlaying;
}
