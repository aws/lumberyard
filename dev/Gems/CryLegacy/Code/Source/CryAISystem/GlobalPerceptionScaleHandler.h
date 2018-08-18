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

// Description : Handler to manage global perception scale values


#ifndef CRYINCLUDE_CRYAISYSTEM_GLOBALPERCEPTIONSCALEHANDLER_H
#define CRYINCLUDE_CRYAISYSTEM_GLOBALPERCEPTIONSCALEHANDLER_H
#pragma once

#include <CryListenerSet.h>

struct IAIObject;

class CGlobalPerceptionScaleHandler
{
public:
    CGlobalPerceptionScaleHandler();

    void SetGlobalPerception(const float visual, const float audio, const EAIFilterType filterType, uint8 factionID);
    void ResetGlobalPerception();

    // Return the specific visual/audio perception scale value for the pObject
    // in relation of the global settings
    float GetGlobalVisualScale(const IAIObject* pAIObject) const;
    float GetGlobalAudioScale(const IAIObject* pAIObject) const;

    void RegisterListener(IAIGlobalPerceptionListener* plistener);
    void UnregisterListener(IAIGlobalPerceptionListener* plistener);

    void Reload();
    void Serialize(TSerialize ser);

    void DebugDraw() const;

private:

    bool IsObjectAffected(const IAIObject* pAIObject) const;
    void NotifyEvent(const IAIGlobalPerceptionListener::EGlobalPerceptionScaleEvent event);

    float m_visual;
    float m_audio;
    EAIFilterType m_filterType;
    uint8 m_factionID;
    uint8 m_playerFactionID;

    typedef CListenerSet<IAIGlobalPerceptionListener*> Listeners;
    Listeners m_listenersList;
};

#endif // CRYINCLUDE_CRYAISYSTEM_GLOBALPERCEPTIONSCALEHANDLER_H
