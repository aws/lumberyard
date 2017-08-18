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

// Description : Interface for the AI target track manager


#ifndef CRYINCLUDE_CRYCOMMON_ITARGETTRACKMANAGER_H
#define CRYINCLUDE_CRYCOMMON_ITARGETTRACKMANAGER_H
#pragma once


#include <IAISystem.h> // <> required for Interfuscator
#include <IAgent.h> // <> required for Interfuscator

struct AgentParameters;
struct ITargetTrack;
struct ITargetTrackManager;

namespace TargetTrackHelpers
{
    // AIEvent stimulus type helper
    enum EAIEventStimulusType
    {
        eEST_Generic,
        eEST_Visual,
        eEST_Sound,
        eEST_BulletRain,
    };

    // Custom stimulus event info
    struct SStimulusEvent
    {
        Vec3 vPos;
        EAITargetThreat eTargetThreat;
        EAIEventStimulusType eStimulusType;

        //////////////////////////////////////////////////////////////////////////
        SStimulusEvent()
            : vPos(ZERO)
            , eTargetThreat(AITHREAT_AGGRESSIVE)
            , eStimulusType(eEST_Generic)
        { }
    };
}

//! Custom threat modifier helper for game-side specific logic overriding
struct ITargetTrackThreatModifier
{
    // <interfuscator:shuffle>
    virtual ~ITargetTrackThreatModifier() {}

    //////////////////////////////////////////////////////////////////////////
    // ModifyTargetThreat
    //
    // Purpose: Determines which threat value the agent should use for this target
    //
    // In:
    //  ownerAI - the AI agent who is owning this target
    //  targetAI - the AI agent who is the perceived target for the owner
    //  track - The Target Track which contains the information about this target
    //
    // Out:
    //  outThreatRatio - The updated threat ratio (to be used as [0,1]), which
    //      is stored for you and is for you to use with determining how
    //      threatening the target is
    //  outThreat - The threat value to be applied to this target now
    //////////////////////////////////////////////////////////////////////////
    virtual void ModifyTargetThreat(IAIObject& ownerAI, IAIObject& targetAI, const ITargetTrack& track, float& outThreatRatio, EAITargetThreat& outThreat) const = 0;
    // </interfuscator:shuffle>
};

struct ITargetTrack
{
    // <interfuscator:shuffle>
    virtual ~ITargetTrack() {}

    virtual const Vec3& GetTargetPos() const = 0;
    virtual const Vec3& GetTargetDir() const = 0;
    virtual float GetTrackValue() const = 0;
    virtual float GetFirstVisualTime() const = 0;
    virtual EAITargetType GetTargetType() const = 0;
    virtual EAITargetThreat GetTargetThreat() const = 0;

    virtual float GetHighestEnvelopeValue() const = 0;
    virtual float GetUpdateInterval() const = 0;
    // </interfuscator:shuffle>
};

struct ITargetTrackManager
{
    // <interfuscator:shuffle>
    virtual ~ITargetTrackManager() {}

    // Threat modifier
    virtual void SetTargetTrackThreatModifier(ITargetTrackThreatModifier* pModifier) = 0;
    virtual void ClearTargetTrackThreatModifier() = 0;

    // Target class mods
    virtual bool SetTargetClassThreat(tAIObjectID aiObjectId, float fClassThreat) = 0;
    virtual float GetTargetClassThreat(tAIObjectID aiObjectId) const = 0;

    // Incoming stimulus handling
    virtual bool HandleStimulusEventInRange(tAIObjectID aiTargetId, const char* szStimulusName, const TargetTrackHelpers::SStimulusEvent& eventInfo, float fRadius) = 0;
    virtual bool HandleStimulusEventForAgent(tAIObjectID aiAgentId, tAIObjectID aiTargetId, const char* szStimulusName, const TargetTrackHelpers::SStimulusEvent& eventInfo) = 0;
    virtual bool TriggerPulse(tAIObjectID aiObjectId, tAIObjectID targetId, const char* szStimulusName, const char* szPulseName) = 0;
    // </interfuscator:shuffle>
};

#endif // CRYINCLUDE_CRYCOMMON_ITARGETTRACKMANAGER_H
