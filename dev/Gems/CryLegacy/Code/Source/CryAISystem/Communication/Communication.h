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

#ifndef CRYINCLUDE_CRYAISYSTEM_COMMUNICATION_COMMUNICATION_H
#define CRYINCLUDE_CRYAISYSTEM_COMMUNICATION_COMMUNICATION_H
#pragma once


#include <ICommunicationManager.h>
#include <VariableCollection.h>

struct SCommunicationChannelParams
{
    enum ECommunicationChannelType
    {
        Global = 0,
        Group,
        Personal,
        Invalid,
    };

    SCommunicationChannelParams()
        : type(Invalid)
        , minSilence(0)
        , priority(0)
        , actorMinSilence(0)
        , ignoreActorSilence(false)
        , flushSilence(0) {}

    // Minimum silence this channel imposes once normal communication is finished.
    float minSilence;
    // Minimum silence this channel imposes on manager if its higher priority and flushes the system.
    float flushSilence;

    string name;
    CommChannelID parentID;
    ECommunicationChannelType type;
    uint8 priority;

    // Minimum silence this channel imposes on an actor once it starts to play.
    float actorMinSilence;
    // Indicates whether this channel should ignore actor silenced restriction.
    bool ignoreActorSilence;
};


struct SCommunicationVariation
{
    SCommunicationVariation()
        : timeout(0.0f)
        , flags(0)
    {
        condition.reset();
    }

    string animationName;
    string soundName;
    string voiceName;

    float timeout;

    uint32 flags;
    Variables::ExpressionPtr condition;
};


struct SCommunication
{
    enum EVariationChoiceMethod
    {
        Random = 0,
        Sequence,
        RandomSequence,
        Match, // only valid for responses
    };

    enum ECommunicationFlags
    {
        LookAtTarget        = 1 << 0,

        FinishAnimation = 1 << 8,
        FinishSound         = 1 << 9,
        FinishVoice         = 1 << 10,
        FinishTimeout       = 1 << 11,
        FinishAll               = FinishAnimation | FinishSound | FinishVoice | FinishTimeout,

        BlockMovement       = 1 << 12,
        BlockFire               = 1 << 13,
        BlockAll                = BlockMovement | BlockFire,

        AnimationAction = 1 << 16,
    };

    string name;
    CommID responseID;

    EVariationChoiceMethod choiceMethod;
    EVariationChoiceMethod responseChoiceMethod;

    struct History
    {
        History()
            : played(0)
        {
        }

        uint32 played;
    } history;

    std::vector<SCommunicationVariation> variations;

    bool forceAnimation;
    bool hasAnimation;
};


#endif // CRYINCLUDE_CRYAISYSTEM_COMMUNICATION_COMMUNICATION_H
