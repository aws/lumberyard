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

// Description : Interface to apply modification to an AI's handling of visual
//               and auditory stimulus within his perception handler


#ifndef CRYINCLUDE_CRYCOMMON_IPERCEPTIONHANDLERMODIFIER_H
#define CRYINCLUDE_CRYCOMMON_IPERCEPTIONHANDLERMODIFIER_H
#pragma once


struct SAIEVENT;

enum EStimulusHandlerResult
{
    eSHR_Continue = 0,      // Allow perception manager to continue processing this stimulus normally
    eSHR_Ignore,            // Ignore this stimulus
    eSHR_MakeAggressive,    // Make perception manager consider this stimulus an act of aggression
};

struct IPerceptionHandlerModifier
{
    // <interfuscator:shuffle>
    virtual ~IPerceptionHandlerModifier() {}

    // Debug drawing
    virtual void DebugDraw(EntityId ownerId, float& fY) const = 0;

    // Determine if stimuli should be ignored
    // Return false if stimulus should be ignored
    virtual EStimulusHandlerResult OnVisualStimulus(SAIEVENT* pAIEvent, IAIObject* pReceiver, EntityId targetId) = 0;
    virtual EStimulusHandlerResult OnSoundStimulus(SAIEVENT* pAIEvent, IAIObject* pReceiver, EntityId targetId) = 0;
    // </interfuscator:shuffle>
};

#endif // CRYINCLUDE_CRYCOMMON_IPERCEPTIONHANDLERMODIFIER_H
