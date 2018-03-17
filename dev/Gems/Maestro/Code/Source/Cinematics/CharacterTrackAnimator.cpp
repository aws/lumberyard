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

#include "Maestro_precompiled.h"
#include "CharacterTrackAnimator.h"
#include "AnimNode.h"
#include "CharacterTrack.h"

//////////////////////////////////////////////////////////////////////////
// Utility script functions used in Character Animnation functions
namespace
{
    static const float TIMEJUMPED_TRANSITION_TIME = 1.0f;

    void NotifyEntityScript(const IEntity* pEntity, const char* funcName)
    {
        IScriptTable* pEntityScript = pEntity->GetScriptTable();
        if (pEntityScript && pEntityScript->HaveValue(funcName))
        {
            Script::CallMethod(pEntityScript, funcName);
        }
    }

    void NotifyEntityScript(const IEntity* pEntity, const char* funcName, const char* strParam)
    {
        IScriptTable* pEntityScript = pEntity->GetScriptTable();
        if (pEntityScript && pEntityScript->HaveValue(funcName))
        {
            Script::CallMethod(pEntityScript, funcName, strParam);
        }
    }
};

//////////////////////////////////////////////////////////////////////////
static bool HaveKeysChanged(int32 activeKeys[], int32 previousKeys[])
{
    return !((activeKeys[0] == previousKeys[0]) && (activeKeys[1] == previousKeys[1]));
}

//////////////////////////////////////////////////////////////////////////
// return active keys ( maximum: 2 in case of overlap ) in the right order
static int32 GetActiveKeys(int32 activeKeys[], f32 ectime, CCharacterTrack* track)
{
    int32 numActiveKeys = 0;

    f32 time = 0.0f;
    bool swap = false;

    int32 numKeys = track->GetNumKeys();
    for (int i = 0; i < numKeys; ++i)
    {
        ICharacterKey key;
        track->GetKey(i, &key);

        if ((key.time <= ectime) && (key.time + track->GetKeyDuration(i) > ectime))
        {
            activeKeys[numActiveKeys] = i;

            // last key had a bigger
            if (time > key.time)
            {
                swap = true;
            }
            time = key.time;
            numActiveKeys++;

            // not more than 2 concurrent keys allowed
            if (numActiveKeys == 2)
            {
                break;
            }
        }
    }

    if (swap)
    {
        int32 temp = activeKeys[1];
        activeKeys[1] = activeKeys[0];
        activeKeys[0] = temp;
    }

    // use the first if we are before the first key
    if (!numActiveKeys && track->GetNumKeys())
    {
        ICharacterKey key0;
        track->GetKey(0, &key0);
        if (ectime < key0.time)
        {
            numActiveKeys = 1;
            activeKeys[0] = 0;
        }
    }
    return numActiveKeys;
}

//////////////////////////////////////////////////////////////////////////
static bool GetNearestKeys(int32 activeKeys[], f32 ectime, CCharacterTrack* track, int32& numActiveKeys)
{
    f32 time = 0.0f;

    f32 nearestLeft = 0.0f;
    f32 nearestRight = 10000.0f;

    int32 left = -1, right = -1;

    int32 numKeys = track->GetNumKeys();
    for (int i = 0; i < numKeys; ++i)
    {
        ICharacterKey key;
        track->GetKey(i, &key);

        f32 endTime = key.time + track->GetKeyDuration(i);
        if ((endTime < ectime) && (endTime > nearestLeft))
        {
            left = i;
            nearestLeft = endTime;
        }

        if ((key.time > ectime) && (key.time < nearestRight))
        {
            right = i;
            nearestRight = key.time;
        }
    }

    if (left != -1)
    {
        ICharacterKey possibleKey;
        track->GetKey(left, &possibleKey);

        assert(((ectime < track->GetEndTime()) ? (possibleKey.m_bLoop == false) : true));

        if (!possibleKey.m_bBlendGap)
        {
            // if no blending is set
            // set animation to the last frame of the nearest animation
            // to the left
            activeKeys[0] = left;
            numActiveKeys = 1;
            return false;
        }
    }

    // found gap-blend neighbors
    if ((right != -1) && (left != -1))
    {
        activeKeys[0] = left;
        activeKeys[1] = right;
        return true;
    }

    return false;
}

/*static*/ const float CCharacterTrackAnimator::s_minClipDuration = 0.01666666666f;         // 1/60th of a second, or one-frame for 60Hz rendering

//////////////////////////////////////////////////////////////////////////
CCharacterTrackAnimator::CCharacterTrackAnimator()
{
    m_forceAnimKeyChange = false;

    m_baseAnimState.m_layerPlaysAnimation[0] = m_baseAnimState.m_layerPlaysAnimation[1] = m_baseAnimState.m_layerPlaysAnimation[2] = false;

    ResetLastAnimKeys();

    m_baseAnimState.m_bTimeJumped[0] = m_baseAnimState.m_bTimeJumped[1] = m_baseAnimState.m_bTimeJumped[2] = false;
    m_baseAnimState.m_jumpTime[0] = m_baseAnimState.m_jumpTime[1] = m_baseAnimState.m_jumpTime[2] = 0.0f;
}

//////////////////////////////////////////////////////////////////////////
void CCharacterTrackAnimator::ResetLastAnimKeys()
{
    m_baseAnimState.m_lastAnimationKeys[0][0] = -1;
    m_baseAnimState.m_lastAnimationKeys[0][1] = -1;
    m_baseAnimState.m_lastAnimationKeys[1][0] = -1;
    m_baseAnimState.m_lastAnimationKeys[1][1] = -1;
    m_baseAnimState.m_lastAnimationKeys[2][0] = -1;
    m_baseAnimState.m_lastAnimationKeys[2][1] = -1;
}

//////////////////////////////////////////////////////////////////////////
void CCharacterTrackAnimator::OnReset(IAnimNode* animNode)
{
    ResetLastAnimKeys();

    ReleaseAllAnimations(animNode);

    m_baseAnimState.m_layerPlaysAnimation[0] = m_baseAnimState.m_layerPlaysAnimation[1] = m_baseAnimState.m_layerPlaysAnimation[2] = false;
}

//////////////////////////////////////////////////////////////////////////
f32 CCharacterTrackAnimator::ComputeAnimKeyNormalizedTime(const ICharacterKey& key, float ectime) const
{
    float endTime = key.GetValidEndTime();
    const float clipDuration = clamp_tpl(endTime - key.m_startTime, s_minClipDuration, key.m_duration);
    float t;
    f32   retNormalizedTime;

    if (clipDuration > s_minClipDuration)
    {
        t = (ectime - key.time) * key.m_speed;

        if (key.m_bLoop && t > clipDuration)
        {
            // compute t for repeating clip
            t = fmod(t, clipDuration);
        }

        t += key.m_startTime;
        t = clamp_tpl(t, key.m_startTime, endTime);
    }
    else
    {
        // clip has perceptably no length - set time to beginning or end frame, whichever comes first in time
        t = (key.m_startTime < endTime) ? key.m_startTime : endTime;
    }

    retNormalizedTime = clamp_tpl(t / key.m_duration, .0f, 1.0f);
    return retNormalizedTime;
}

//////////////////////////////////////////////////////////////////////////
bool CCharacterTrackAnimator::CheckTimeJumpingOrOtherChanges(const SAnimContext& ec, int32 activeKeys[], int32 numActiveKeys,
                                                                ICharacterInstance* pCharacter, int layer, int trackIndex, SAnimState& animState)
{
    bool bEditing = gEnv->IsEditor() && gEnv->IsEditorGameMode() == false;
    bool bJustJumped = ec.bSingleFrame && bEditing == false;
    if (bJustJumped)
    {
        animState.m_jumpTime[trackIndex] = ec.time;
    }
    bool bKeysChanged = HaveKeysChanged(activeKeys, animState.m_lastAnimationKeys[trackIndex]);
    bool bAnyChange = bKeysChanged || bJustJumped;
    if (animState.m_bTimeJumped[trackIndex])
    {
        const bool bJumpBlendingDone = ec.time - animState.m_jumpTime[trackIndex] > TIMEJUMPED_TRANSITION_TIME;
        bAnyChange = bAnyChange || bJumpBlendingDone;
        if (bAnyChange)
        {
            animState.m_bTimeJumped[trackIndex] = false;
            animState.m_jumpTime[trackIndex] = 0;
        }
    }
    else if (animState.m_bTimeJumped[trackIndex] == false)
    {
        animState.m_bTimeJumped[trackIndex] = bJustJumped;
        if (animState.m_bTimeJumped[trackIndex])
        {
            if (numActiveKeys != 1)
            {
                // The transition blending in a time-jumped case only makes sense
                // when there is a single active animation at that point of time.
                animState.m_bTimeJumped[trackIndex] = false;
            }
            if (pCharacter->GetISkeletonAnim()->GetNumAnimsInFIFO(layer) != 1)
            {
                // The transition blending in a time-jumped case only makes sense
                // where there has been a single animation going on for the character.
                animState.m_bTimeJumped[trackIndex] = false;
            }
        }
    }

    bAnyChange = bAnyChange || m_forceAnimKeyChange;
    m_forceAnimKeyChange = false;       // reset m_forceAnimKeyChange now that we've consumed it

    return bAnyChange;
}

//////////////////////////////////////////////////////////////////////////
ILINE bool CCharacterTrackAnimator::IsAnimationPlaying(const SAnimState& animState) const
{
    return animState.m_layerPlaysAnimation[0] || animState.m_layerPlaysAnimation[1] || animState.m_layerPlaysAnimation[2];
}

//////////////////////////////////////////////////////////////////////////
void CCharacterTrackAnimator::ReleaseAllAnimations(IAnimNode* animNode)
{
    if (!animNode)
    {
        return;
    }

    ICharacterInstance* pCharacter = animNode->GetCharacterInstance();
    if (!pCharacter)
    {
        return;
    }

    IAnimationSet* pAnimations = pCharacter->GetIAnimationSet();
    assert(pAnimations);

    if (IsAnimationPlaying(m_baseAnimState))
    {
        pCharacter->GetISkeletonAnim()->SetTrackViewExclusive(0);

        pCharacter->GetISkeletonAnim()->StopAnimationsAllLayers();

        pCharacter->SetPlaybackScale(1.0000f);
        pCharacter->GetISkeletonAnim()->SetAnimationDrivenMotion(m_characterWasTransRot);
        m_baseAnimState.m_layerPlaysAnimation[0] = m_baseAnimState.m_layerPlaysAnimation[1] = m_baseAnimState.m_layerPlaysAnimation[2] = false;

        IEntity* entity = animNode->GetEntity();
        if (entity)
        {
            NotifyEntityScript(entity, "OnSequenceAnimationStop");
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CCharacterTrackAnimator::AnimateTrack(class CCharacterTrack* pTrack, SAnimContext& ec, int layer, int trackIndex)
{
    IAnimNode* animNode = pTrack->GetNode();
    if (!animNode)
    {
        return;
    }

    ICharacterInstance* character = animNode->GetCharacterInstance();
    if (!character)
    {
        return;
    }

    IEntity* entity = pTrack->GetNode()->GetEntity();
    ISystem* pISystem = GetISystem();
    IRenderer* pIRenderer = gEnv->pRenderer;
    IRenderAuxGeom* pAuxGeom = pIRenderer->GetIRenderAuxGeom();

    pTrack->GetActiveKey(ec.time, 0);   // To sort keys

    int32 activeKeys[2] = { -1, -1 };
    int32 numActiveKeys = GetActiveKeys(activeKeys, ec.time, pTrack);

    // decide if blending gap is asked for or possible
    bool blendGap = false;
    if (numActiveKeys == 0)
    {
        blendGap = GetNearestKeys(activeKeys, ec.time, pTrack, numActiveKeys);
    }

    bool bAnyChange = CheckTimeJumpingOrOtherChanges(ec, activeKeys, numActiveKeys, character, layer, trackIndex, m_baseAnimState);

    // the used keys have changed - be it overlapping, single, nearest or none at all
    if (bAnyChange)
    {
        if (m_baseAnimState.m_bTimeJumped[trackIndex] == false)   // In case of the time-jumped, the existing animation must not be cleared.
        {
            character->GetISkeletonAnim()->ClearFIFOLayer(layer);
        }

        m_baseAnimState.m_layerPlaysAnimation[trackIndex] = false;

        // rebuild the transition queue
        for (int32 i = 0; i < 2; ++i)
        {
            int32 k = activeKeys[i];
            if (k < 0)
            {
                break;
            }

            ICharacterKey key;
            pTrack->GetKey(k, &key);

            float t = ec.time - key.time;
            t = key.m_startTime + t * key.m_speed;

            if (!key.m_animation.empty())
            {
                // retrieve the animation collection for the model
                IAnimationSet* pAnimations = character->GetIAnimationSet();
                assert(pAnimations);

                if (character->GetISkeletonAnim()->GetAnimationDrivenMotion() && (!IsAnimationPlaying(m_baseAnimState)))
                {
                    m_characterWasTransRot = true;
                }

                character->GetISkeletonAnim()->SetAnimationDrivenMotion(key.m_bInPlace ? 1 : 0);
                character->GetISkeletonAnim()->SetTrackViewExclusive(1);

                // Start playing animation.
                CryCharAnimationParams aparams;
                aparams.m_nFlags = CA_TRACK_VIEW_EXCLUSIVE | CA_ALLOW_ANIM_RESTART;
                if (key.m_bLoop)
                {
                    aparams.m_nFlags |= CA_LOOP_ANIMATION;
                }
                aparams.m_nLayerID = layer;
                aparams.m_fTransTime = -1.0f;

                character->GetISkeletonAnim()->StartAnimation(key.m_animation.c_str(), aparams);

                if (entity)
                {
                    NotifyEntityScript(entity, "OnSequenceAnimationStart", key.m_animation.c_str());
                }

                m_baseAnimState.m_layerPlaysAnimation[trackIndex] = true;

                // fix duration?
                int animId = pAnimations->GetAnimIDByName(key.m_animation.c_str());
                if (animId >= 0)
                {
                    float duration = pAnimations->GetDuration_sec(animId);
                    if (duration != key.m_duration)
                    {
                        key.m_duration = duration;
                        pTrack->SetKey(k, &key);
                    }
                }
            }
        }

        m_baseAnimState.m_lastAnimationKeys[trackIndex][0] = activeKeys[0];
        m_baseAnimState.m_lastAnimationKeys[trackIndex][1] = activeKeys[1];

        if (!IsAnimationPlaying(m_baseAnimState))
        {
            // There is no animation left playing - exit TrackViewExclusive mode
            character->GetISkeletonAnim()->SetTrackViewExclusive(0);
            character->GetISkeletonAnim()->StopAnimationsAllLayers();
            character->SetPlaybackScale(1.0000f);
            character->GetISkeletonAnim()->SetAnimationDrivenMotion(m_characterWasTransRot);
            if (entity)
            {
                NotifyEntityScript(entity, "OnSequenceAnimationStop");
            }
            return;
        }
    }

    if (m_baseAnimState.m_layerPlaysAnimation[trackIndex])
    {
        if (m_baseAnimState.m_bTimeJumped[trackIndex])
        {
            assert(numActiveKeys == 1 && activeKeys[0] >= 0 && character->GetISkeletonAnim()->GetNumAnimsInFIFO(layer) == 2);
            UpdateAnimTimeJumped(activeKeys[0], pTrack, ec.time, character, layer, !bAnyChange, trackIndex, m_baseAnimState);
        }
        // regular one- or two-animation(s) case
        else if (numActiveKeys > 0)
        {
            UpdateAnimRegular(numActiveKeys, activeKeys, pTrack, ec.time, character, layer, !bAnyChange);
        }
        // blend gap
        else if (blendGap)
        {
            UpdateAnimBlendGap(activeKeys, pTrack, ec.time, character, layer);
        }
    }
}

void CCharacterTrackAnimator::ForceAnimKeyChange()
{
    m_forceAnimKeyChange = true;
}

//////////////////////////////////////////////////////////////////////////
void CCharacterTrackAnimator::ApplyAnimKey(int32 keyIndex, class CCharacterTrack* track, float ectime,
    ICharacterInstance* pCharacter, int layer, int animIndex, bool bAnimEvents)
{
    ICharacterKey key;
    track->GetKey(keyIndex, &key);

    f32 normalizedTime = ComputeAnimKeyNormalizedTime(key, ectime);

    pCharacter->SetPlaybackScale(0.0000f);
    pCharacter->GetISkeletonAnim()->ManualSeekAnimationInFIFO(layer, animIndex, normalizedTime, bAnimEvents);
    pCharacter->GetISkeletonAnim()->SetLayerNormalizedTime(layer, normalizedTime);
}

//////////////////////////////////////////////////////////////////////////
void CCharacterTrackAnimator::UpdateAnimTimeJumped(int32 keyIndex, class CCharacterTrack* track,
    float ectime, ICharacterInstance* pCharacter, int layer, bool bAnimEvents, int trackIndex, SAnimState& animState)
{
    f32 blendWeight = 0.0f;
    ISkeletonAnim* pISkeletonAnim = pCharacter->GetISkeletonAnim();
    if (pISkeletonAnim->GetNumAnimsInFIFO(layer) == 2)
    {
        ICharacterKey key;
        track->GetKey(keyIndex, &key);

        float transitionTime = clamp_tpl(track->GetKeyDuration(keyIndex), FLT_EPSILON, TIMEJUMPED_TRANSITION_TIME);
        blendWeight = (ectime - animState.m_jumpTime[trackIndex]) / transitionTime;
        CRY_ASSERT(0 <= blendWeight && blendWeight <= 1.0f);
        if (blendWeight > 1.0f)
        {
            blendWeight = 1.0f;
        }
        else if (blendWeight < 0)
        {
            blendWeight = 0;
        }

        pCharacter->GetISkeletonAnim()->SetTrackViewMixingWeight(layer, blendWeight);
    }
    else
    {
        pCharacter->GetISkeletonAnim()->SetTrackViewMixingWeight(layer, 0);
    }
    ApplyAnimKey(keyIndex, track, ectime, pCharacter, layer, 1, bAnimEvents);
}

//////////////////////////////////////////////////////////////////////////
void CCharacterTrackAnimator::UpdateAnimRegular(int32 numActiveKeys, int32 activeKeys[], class CCharacterTrack* track, float ectime, ICharacterInstance* pCharacter, int layer, bool bAnimEvents)
{
    // cross-fade
    if (numActiveKeys == 2)
    {
        ICharacterKey key1, key2;
        track->GetKey(activeKeys[0], &key1);

        track->GetKey(activeKeys[1], &key2);

        const f32 key1EndTime = key1.time + track->GetKeyDuration(activeKeys[0]);
        const f32 t0 = key1EndTime - key2.time;
        const f32 t = ectime - key2.time;
        const f32 blendWeight = AZ::IsClose(t0, .0f, FLT_EPSILON) ? 1.0f : (t / t0);

        assert(0 <= blendWeight && blendWeight <= 1.0f);
        pCharacter->GetISkeletonAnim()->SetTrackViewMixingWeight(layer, blendWeight);
    }

    for (int32 i = 0; i < 2; ++i)
    {
        if (activeKeys[i] < 0)
        {
            break;
        }
        ApplyAnimKey(activeKeys[i], track, ectime, pCharacter, layer, i, bAnimEvents);
    }
}

//////////////////////////////////////////////////////////////////////////
void CCharacterTrackAnimator::UpdateAnimBlendGap(int32 activeKeys[], class CCharacterTrack* track, float ectime, ICharacterInstance* pCharacter, int layer)
{
    f32 blendWeight = 0.0f;
    ICharacterKey key1, key2;
    track->GetKey(activeKeys[0], &key1);
    track->GetKey(activeKeys[1], &key2);

    f32 key1EndTime = key1.time + track->GetKeyDuration(activeKeys[0]);
    f32 t0 = key2.time - key1EndTime;
    f32 t = ectime - key1EndTime;
    if (key1EndTime == key2.time)
    {
        blendWeight = 1.0f;
    }
    else
    {
        blendWeight = t / t0;
    }

    pCharacter->SetPlaybackScale(0.0000f);
    f32 endTimeNorm = 1.0f;
    if (key1.m_duration > 0)
    {
        endTimeNorm = key1.GetValidEndTime() / key1.m_duration;
    }
    assert(endTimeNorm >= 0.0f && endTimeNorm <= 1.0f);
    pCharacter->GetISkeletonAnim()->ManualSeekAnimationInFIFO(layer, 0, endTimeNorm, false);
    f32 startTimeNorm = 0.0f;
    if (key2.m_duration)
    {
        startTimeNorm = key2.m_startTime / key2.m_duration;
    }
    assert(startTimeNorm >= 0.0f && startTimeNorm <= 1.0f);
    pCharacter->GetISkeletonAnim()->ManualSeekAnimationInFIFO(layer, 1, startTimeNorm, false);
    pCharacter->GetISkeletonAnim()->SetTrackViewMixingWeight(layer, blendWeight);
}
