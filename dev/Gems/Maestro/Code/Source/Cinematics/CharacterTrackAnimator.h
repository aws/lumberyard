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
#pragma once

/*!
        Utility class to handle Animation of Character Tracks (aka 'Animation' Tracks in the TrackView UI)
    */

#include <CrySizer.h>

struct IEntity;
struct SAnimContext;
struct ICharacterInstance;
struct ICharacterKey;
struct IAnimNode;

#define MAX_CHARACTER_TRACKS 3
#define ADDITIVE_LAYERS_OFFSET 6

class CCharacterTrackAnimator
{
public:
    CCharacterTrackAnimator();
    ~CCharacterTrackAnimator() = default;
 
    struct SAnimState
    {
        int32 m_lastAnimationKeys[3][2];
        bool m_layerPlaysAnimation[3];

        //! This is used to indicate that a time-jumped blending is currently happening in the animation track.
        bool m_bTimeJumped[3];
        float m_jumpTime[3];
    };
    using TStringSet = std::set<string>;
    using TStringSetIt = TStringSet::iterator;

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
        pSizer->AddObject(m_setAnimationSinks);
    }

    void OnReset(IAnimNode* animNode);
    void OnEndAnimation(const char* animName, IEntity* entity);

    ILINE bool IsAnimationPlaying(const SAnimState& animState) const;

    //! Animate Character Track.
    void AnimateTrack(class CCharacterTrack* track, SAnimContext& ec, int layer, int trackIndex);

    // Forces current playhead anim key state change to reset animation cues
    void ForceAnimKeyChange();
    //////////////////////////////////////////////////////////////////////////////////////////
protected:
    f32 ComputeAnimKeyNormalizedTime(const ICharacterKey& key, float ectime) const;

private:
    void ResetLastAnimKeys();
    void ReleaseAllAnimations(IAnimNode* animNode);

    void UpdateAnimTimeJumped(int32 keyIndex, class CCharacterTrack * track, float ectime, ICharacterInstance * pCharacter, int layer, bool bAnimEvents, int trackIndex, SAnimState & animState);
    void UpdateAnimRegular(int32 numActiveKeys, int32 activeKeys[], class CCharacterTrack * track, float ectime, ICharacterInstance * pCharacter, int layer, bool bAnimEvents);
    void UpdateAnimBlendGap(int32 activeKeys[], class CCharacterTrack * track, float ectime, ICharacterInstance * pCharacter, int layer);
    void ApplyAnimKey(int32 keyIndex, class CCharacterTrack * track, float ectime, ICharacterInstance * pCharacter, int layer, int animIndex, bool bAnimEvents);

    bool CheckTimeJumpingOrOtherChanges(const SAnimContext& ec, int32 activeKeys[], int32 numActiveKeys, ICharacterInstance* pCharacter, int layer, int trackIndex, SAnimState& animState);

    //////////////////////////////////////////////////////////////////////////////////////////
    static const float s_minClipDuration;

    SAnimState m_baseAnimState;
    TStringSet m_setAnimationSinks;
    bool       m_characterWasTransRot;
    bool       m_forceAnimKeyChange;
};