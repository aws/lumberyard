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

#include "pch.h"
#include "stdafx.h"
#include "EffectPlayer.h"

#include <ICryAnimation.h>
#include <IEntitySystem.h>

namespace CharacterTool {
    EffectPlayer::EffectPlayer()
        :   m_pSkeleton2(0)
        , m_pSkeletonPose(0)
        , m_pIDefaultSkeleton(0)
    {
        GetIEditor()->RegisterNotifyListener(this);
    }

    EffectPlayer::~EffectPlayer()
    {
        KillAllEffects();
        GetIEditor()->UnregisterNotifyListener(this);
    }

    void EffectPlayer::SetSkeleton(ISkeletonAnim* pSkeletonAnim, ISkeletonPose* pSkeletonPose, IDefaultSkeleton* pIDefaultSkeleton)
    {
        m_pSkeleton2 = pSkeletonAnim;
        m_pSkeletonPose = pSkeletonPose;
        m_pIDefaultSkeleton = pIDefaultSkeleton;
    }

    void EffectPlayer::Update(const QuatT& rPhysEntity)
    {
        for (int i = 0; i < m_effects.size(); )
        {
            EffectEntry& entry = m_effects[i];

            // If the animation has stopped, kill the effect.
            bool effectStillPlaying = (entry.pEmitter ? entry.pEmitter->IsAlive() : false);
            bool animStillPlaying = IsPlayingAnimation(entry.animID);
            if (animStillPlaying && effectStillPlaying)
            {
                // Update the effect position.
                Matrix34 tm;
                GetEffectTM(tm, entry.boneID, entry.offset, entry.dir);
                if (entry.pEmitter)
                {
                    entry.pEmitter->SetMatrix(Matrix34(rPhysEntity) * tm);
                }
                ++i;
            }
            else
            {
                if (m_effects[i].pEmitter)
                {
                    m_effects[i].pEmitter->Activate(false);
                }
                m_effects.erase(m_effects.begin() + i);
            }
        }
    }

    void EffectPlayer::KillAllEffects()
    {
        for (int i = 0, count = m_effects.size(); i < count; ++i)
        {
            if (m_effects[i].pEmitter)
            {
                m_effects[i].pEmitter->Activate(false);
            }
        }
        m_effects.clear();
    }

    void EffectPlayer::SpawnEffect(int animID, const char* animName, const char* effectName, const char* boneName, const Vec3& offset, const Vec3& dir)
    {
        // Check whether we are already playing this effect, and if so dont restart it.
        bool alreadyPlayingEffect = false;
        if (!gEnv->pConsole->GetCVar("ca_AllowMultipleEffectsOfSameName")->GetIVal())
        {
            alreadyPlayingEffect = IsPlayingEffect(effectName);
        }

        if (!alreadyPlayingEffect)
        {
            IParticleEffect* pEffect = gEnv->pParticleManager->FindEffect(effectName);
            if (!pEffect)
            {
                CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Anim events cannot find effect \"%s\", requested by animation \"%s\".", (effectName ? effectName : "<MISSING EFFECT NAME>"), (animName ? animName : "<MISSING ANIM NAME>"));
            }
            int boneID = (boneName && boneName[0] && m_pIDefaultSkeleton ? m_pIDefaultSkeleton->GetJointIDByName(boneName) : -1);
            boneID = (boneID == -1 ? 0 : boneID);
            Matrix34 tm;
            GetEffectTM(tm, boneID, offset, dir);
            IParticleEmitter* pEmitter = (pEffect ? pEffect->Spawn(tm, ePEF_Nowhere) : 0);

            if (pEffect && pEmitter)
            {
                // Make sure the emitter is not rendered in the game.
                m_effects.push_back(EffectEntry(pEffect, pEmitter, boneID, offset, dir, animID));
            }
        }
    }

    void EffectPlayer::OnEditorNotifyEvent(EEditorNotifyEvent event)
    {
        switch (event)
        {
        case eNotify_OnCloseScene:
            KillAllEffects();
            break;
        }
    }

    EffectPlayer::EffectEntry::EffectEntry(const _smart_ptr<IParticleEffect>& pEffect, const _smart_ptr<IParticleEmitter>& pEmitter, int boneID, const Vec3& offset, const Vec3& dir, int animID)
        :   pEffect(pEffect)
        , pEmitter(pEmitter)
        , boneID(boneID)
        , offset(offset)
        , dir(dir)
        , animID(animID)
    {
    }

    EffectPlayer::EffectEntry::~EffectEntry()
    {
    }

    void EffectPlayer::GetEffectTM(Matrix34& tm, int boneID, const Vec3& offset, const Vec3& dir)
    {
        if (dir.len2() > 0)
        {
            tm = Matrix33::CreateRotationXYZ(Ang3(dir * 3.14159f / 180.0f));
        }
        else
        {
            tm.SetIdentity();
        }
        tm.AddTranslation(offset);

        if (m_pSkeletonPose)
        {
            tm = Matrix34(m_pSkeletonPose->GetAbsJointByID(boneID)) * tm;
        }
    }

    bool EffectPlayer::IsPlayingAnimation(int animID)
    {
        enum
        {
            NUM_LAYERS = 4
        };

        // Check whether the animation has stopped.
        bool animPlaying = false;
        for (int layer = 0; layer < NUM_LAYERS; ++layer)
        {
            for (int animIndex = 0, animCount = (m_pSkeleton2 ? m_pSkeleton2->GetNumAnimsInFIFO(layer) : 0); animIndex < animCount; ++animIndex)
            {
                CAnimation& anim = m_pSkeleton2->GetAnimFromFIFO(layer, animIndex);
                animPlaying = animPlaying || (anim.GetAnimationId() == animID);
            }
        }

        return animPlaying;
    }

    bool EffectPlayer::IsPlayingEffect(const char* effectName)
    {
        bool isPlaying = false;
        for (int effectIndex = 0, effectCount = m_effects.size(); effectIndex < effectCount; ++effectIndex)
        {
            IParticleEffect* pEffect = m_effects[effectIndex].pEffect;
            if (pEffect && azstricmp(pEffect->GetName(), effectName) == 0)
            {
                isPlaying = true;
            }
        }
        return isPlaying;
    }

    void EffectPlayer::Render(SRendParams& params, const SRenderingPassInfo& passInfo)
    {
        for (int effectIndex = 0, effectCount = m_effects.size(); effectIndex < effectCount; ++effectIndex)
        {
            IParticleEmitter* pEmitter = m_effects[effectIndex].pEmitter;
            if (pEmitter)
            {
                pEmitter->Update();
                pEmitter->Render(params, passInfo);
            }
        }
    }
}
