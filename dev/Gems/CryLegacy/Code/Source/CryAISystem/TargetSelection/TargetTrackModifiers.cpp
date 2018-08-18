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

// Description : Definitions for various modifiers to target tracks


#include "CryLegacy_precompiled.h"
#include "TargetTrackModifiers.h"
#include "TargetTrackManager.h"
#include "TargetTrack.h"
#include "ObjectContainer.h"

//////////////////////////////////////////////////////////////////////////
CTargetTrackDistanceModifier::CTargetTrackDistanceModifier()
{
}

//////////////////////////////////////////////////////////////////////////
CTargetTrackDistanceModifier::~CTargetTrackDistanceModifier()
{
}

//////////////////////////////////////////////////////////////////////////
bool CTargetTrackDistanceModifier::IsMatchingTag(const char* szTag) const
{
    return _stricmp(szTag, GetTag()) == 0;
}

//////////////////////////////////////////////////////////////////////////
char const* CTargetTrackDistanceModifier::GetTag() const
{
    return "Distance";
}

//////////////////////////////////////////////////////////////////////////
float CTargetTrackDistanceModifier::GetModValue(const CTargetTrack* pTrack,
    TargetTrackHelpers::EAIEventStimulusType stimulusType,
    const Vec3& vPos, const TargetTrackHelpers::SEnvelopeData& envelopeData,
    const TargetTrackHelpers::STargetTrackModifierConfig& modConfig) const
{
    float modValue = 0.0f;

    CWeakRef<CAIObject> refOwner = pTrack->GetAIGroupOwner();
    if (refOwner.IsValid())
    {
        CAIObject* pOwner = refOwner.GetAIObject();
        assert(pOwner);

        float distance = vPos.GetDistance(pOwner->GetPos());
        float range = -1.0f;
        float scale = 1.0f;

        if (CAIActor* pOwnerActor = pOwner->CastToCAIActor())
        {
            const AgentParameters& parameters = pOwnerActor->GetParameters();

            if (stimulusType == TargetTrackHelpers::eEST_Visual)
            {
                range = parameters.m_PerceptionParams.sightRange;

                CWeakRef<CAIObject> refTarget = pTrack->GetAITarget();
                if (refTarget.IsValid())
                {
                    if ((parameters.m_PerceptionParams.sightRangeVehicle > 0.0f) &&
                        (refTarget.GetAIObject()->GetAIType() == AIOBJECT_VEHICLE))
                    {
                        range = parameters.m_PerceptionParams.sightRangeVehicle;
                    }
                }
            }
            else if (stimulusType == TargetTrackHelpers::eEST_Sound)
            {
                scale = parameters.m_PerceptionParams.audioScale;
            }
        }

        distance = (scale > 0.0001f) ? distance * (1.0f / scale) : FLT_MAX;

        if (modConfig.m_fLimit > 0.0f)
        {
            range = (float)fsel(range, min(range, modConfig.m_fLimit), modConfig.m_fLimit);
        }

        modValue = (range > 0.0f) ? ((range - distance) / range) :
            (distance > 0.0001f) ? (1.0f / distance) : 1.0f;

        modValue = clamp_tpl(modValue, 0.0f, 1.0f);
    }

    return modValue * modConfig.m_fValue;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
CTargetTrackHostileModifier::CTargetTrackHostileModifier()
{
}

//////////////////////////////////////////////////////////////////////////
CTargetTrackHostileModifier::~CTargetTrackHostileModifier()
{
}

//////////////////////////////////////////////////////////////////////////
bool CTargetTrackHostileModifier::IsMatchingTag(const char* szTag) const
{
    return _stricmp(szTag, GetTag()) == 0;
}

//////////////////////////////////////////////////////////////////////////
char const* CTargetTrackHostileModifier::GetTag() const
{
    return "Hostile";
}

//////////////////////////////////////////////////////////////////////////
float CTargetTrackHostileModifier::GetModValue(const CTargetTrack* pTrack,
    TargetTrackHelpers::EAIEventStimulusType stimulusType,
    const Vec3& vPos, const TargetTrackHelpers::SEnvelopeData& envelopeData,
    const TargetTrackHelpers::STargetTrackModifierConfig& modConfig) const
{
    assert(pTrack);

    bool bIsHostile = false;

    CWeakRef<CAIObject> refOwner = pTrack->GetAIGroupOwner();
    CWeakRef<CAIObject> refTarget = pTrack->GetAIObject();
    if (refOwner.IsValid() && refTarget.IsValid())
    {
        CAIObject* pOwner = refOwner.GetAIObject();
        CAIObject* pTarget = refTarget.GetAIObject();
        assert(pOwner);
        assert(pTarget);

        bIsHostile = pOwner->IsHostile(pTarget);
    }

    return (1.0f * (bIsHostile ? modConfig.m_fValue : 1.0f));
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
CTargetTrackClassThreatModifier::CTargetTrackClassThreatModifier()
{
}

//////////////////////////////////////////////////////////////////////////
CTargetTrackClassThreatModifier::~CTargetTrackClassThreatModifier()
{
}

//////////////////////////////////////////////////////////////////////////
bool CTargetTrackClassThreatModifier::IsMatchingTag(const char* szTag) const
{
    return _stricmp(szTag, GetTag()) == 0;
}

//////////////////////////////////////////////////////////////////////////
char const* CTargetTrackClassThreatModifier::GetTag() const
{
    return "ClassThreat";
}

//////////////////////////////////////////////////////////////////////////
float CTargetTrackClassThreatModifier::GetModValue(const CTargetTrack* pTrack,
    TargetTrackHelpers::EAIEventStimulusType stimulusType,
    const Vec3& vPos, const TargetTrackHelpers::SEnvelopeData& envelopeData,
    const TargetTrackHelpers::STargetTrackModifierConfig& modConfig) const
{
    assert(pTrack);

    tAIObjectID aiObjectId = pTrack->GetAIObject().GetObjectID();
    const float fClassThreat = gAIEnv.pTargetTrackManager->GetTargetClassThreat(aiObjectId);

    return (fClassThreat * modConfig.m_fValue);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
CTargetTrackDistanceIgnoreModifier::CTargetTrackDistanceIgnoreModifier()
{
}

//////////////////////////////////////////////////////////////////////////
CTargetTrackDistanceIgnoreModifier::~CTargetTrackDistanceIgnoreModifier()
{
}

//////////////////////////////////////////////////////////////////////////
bool CTargetTrackDistanceIgnoreModifier::IsMatchingTag(const char* szTag) const
{
    return _stricmp(szTag, GetTag()) == 0;
}

//////////////////////////////////////////////////////////////////////////
char const* CTargetTrackDistanceIgnoreModifier::GetTag() const
{
    return "DistanceIgnore";
}

//////////////////////////////////////////////////////////////////////////
float CTargetTrackDistanceIgnoreModifier::GetModValue(const CTargetTrack* pTrack,
    TargetTrackHelpers::EAIEventStimulusType stimulusType,
    const Vec3& vPos, const TargetTrackHelpers::SEnvelopeData& envelopeData,
    const TargetTrackHelpers::STargetTrackModifierConfig& modConfig) const
{
    // Ignore modifier doesn't run when the envelope was not fully released last time
    if (envelopeData.m_fLastReleasingValue > 0.0f)
    {
        return 1.0f;
    }

    float modValue = 0.0f;

    CWeakRef<CAIObject> refOwner = pTrack->GetAIGroupOwner();
    if (refOwner.IsValid())
    {
        CAIObject* pOwner = refOwner.GetAIObject();
        assert(pOwner);

        float distance = vPos.GetDistance(pOwner->GetPos());
        float range = -1.0f;
        float scale = 1.0f;

        if (CAIActor* pOwnerActor = pOwner->CastToCAIActor())
        {
            const AgentParameters& parameters = pOwnerActor->GetParameters();

            if (stimulusType == TargetTrackHelpers::eEST_Visual)
            {
                range = parameters.m_PerceptionParams.sightRange;

                CWeakRef<CAIObject> refTarget = pTrack->GetAITarget();
                if (refTarget.IsValid())
                {
                    if ((parameters.m_PerceptionParams.sightRangeVehicle > 0.0f) &&
                        (refTarget.GetAIObject()->GetAIType() == AIOBJECT_VEHICLE))
                    {
                        range = parameters.m_PerceptionParams.sightRangeVehicle;
                    }
                }
            }
            else if (stimulusType == TargetTrackHelpers::eEST_Sound)
            {
                scale = parameters.m_PerceptionParams.audioScale;
            }
        }

        distance = (scale > 0.0001f) ? distance * (1.0f / scale) : FLT_MAX;

        if (modConfig.m_fLimit > 0.0f)
        {
            range = min(range, modConfig.m_fLimit);
        }

        modValue = (range > 0.0f) ? (distance / range) : distance;
    }

    const float now = GetAISystem()->GetFrameStartTimeSeconds();
    const float dt = (now - envelopeData.m_fStartTime);

    return (float)fsel(modValue * modConfig.m_fValue - dt, 0.0f, 1.0f);
}

bool CTargetTrackPlayerModifier::IsMatchingTag(const char* tag) const
{
    return _stricmp(tag, GetTag()) == 0;
}

char const* CTargetTrackPlayerModifier::GetTag() const
{
    return "Player";
}

float CTargetTrackPlayerModifier::GetModValue(const CTargetTrack* pTrack,
    TargetTrackHelpers::EAIEventStimulusType stimulusType,
    const Vec3& vPos, const TargetTrackHelpers::SEnvelopeData& envelopeData,
    const TargetTrackHelpers::STargetTrackModifierConfig& modConfig) const
{
    CWeakRef<CAIObject> ownerWeak = pTrack->GetAIGroupOwner();

    if (ownerWeak.IsValid())
    {
        CAIObject* owner = ownerWeak.GetAIObject();

        if (CAIActor* ownerActor = owner->CastToCAIActor())
        {
            CWeakRef<CAIObject> targetWeak = pTrack->GetAITarget();

            if (targetWeak.IsValid())
            {
                CAIObject* targetObject = targetWeak.GetAIObject();

                const bool targetIsPlayer = (targetObject->GetType() == AIOBJECT_PLAYER);

                return targetIsPlayer ? modConfig.m_fValue : 1.0f;
            }
        }
    }

    return 1.0f;
}