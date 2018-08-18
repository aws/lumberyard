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
#include "GlobalPerceptionScaleHandler.h"
#ifdef CRYAISYSTEM_DEBUG
#include "DebugDrawContext.h"
#endif

CGlobalPerceptionScaleHandler::CGlobalPerceptionScaleHandler()
    : m_visual(1.0f)
    , m_audio(1.0f)
    , m_filterType(eAIFT_None)
    , m_factionID(0)
    , m_playerFactionID(0)
    , m_listenersList(1)
{
}

void CGlobalPerceptionScaleHandler::SetGlobalPerception(const float visual, const float audio, const EAIFilterType filterType,
    uint8 factionID)
{
    m_visual = visual;
    m_audio = audio;
    m_filterType = filterType;
    m_factionID = factionID;
    NotifyEvent(IAIGlobalPerceptionListener::eGPS_Set);
}

void CGlobalPerceptionScaleHandler::ResetGlobalPerception()
{
    if (m_filterType == eAIFT_None)
    {
        return;
    }

    m_visual = 1.0f;
    m_audio = 1.0f;
    m_filterType = eAIFT_None;
    NotifyEvent(IAIGlobalPerceptionListener::eGPS_Disabled);
}

void CGlobalPerceptionScaleHandler::Reload()
{
    const IFactionMap& pFactionMap = gEnv->pAISystem->GetFactionMap();
    m_playerFactionID = pFactionMap.GetFactionID("Players");
}

float CGlobalPerceptionScaleHandler::GetGlobalAudioScale(const IAIObject* pAIObject) const
{
    return IsObjectAffected(pAIObject) ? m_audio : 1.0f;
}

float CGlobalPerceptionScaleHandler::GetGlobalVisualScale(const IAIObject* pAIObject) const
{
    return IsObjectAffected(pAIObject) ? m_visual : 1.0f;
}

bool CGlobalPerceptionScaleHandler::IsObjectAffected(const IAIObject* pAIObject) const
{
    if (!pAIObject)
    {
        return false;
    }

    const IFactionMap& pFactionMap = gEnv->pAISystem->GetFactionMap();
    bool isAffected = false;

    const uint8 objectFactionID = pAIObject->GetFactionID();
    switch (m_filterType)
    {
    case eAIFT_All:
        isAffected = true;
        break;
    case eAIFT_Enemies:
        if (pFactionMap.GetReaction(objectFactionID, m_playerFactionID) == IFactionMap::Hostile)
        {
            isAffected = true;
        }
        break;
    case eAIFT_Friends:
        if (pFactionMap.GetReaction(objectFactionID, m_playerFactionID) == IFactionMap::Friendly)
        {
            isAffected = true;
        }
        break;
    case eAIFT_Faction:
        if (objectFactionID == m_factionID)
        {
            isAffected = true;
        }
        break;
    case eAIFT_None:
        break;
    default:
        assert(0);
        break;
    }

    return isAffected;
}

void CGlobalPerceptionScaleHandler::RegisterListener(IAIGlobalPerceptionListener* plistener)
{
    ScopedSwitchToGlobalHeap useGlobalHeap;
    m_listenersList.Add(plistener);
}

void CGlobalPerceptionScaleHandler::UnregisterListener(IAIGlobalPerceptionListener* plistener)
{
    m_listenersList.Remove(plistener);
}

void CGlobalPerceptionScaleHandler::NotifyEvent(const IAIGlobalPerceptionListener::EGlobalPerceptionScaleEvent event)
{
    for (Listeners::Notifier notifier(m_listenersList); notifier.IsValid(); notifier.Next())
    {
        notifier->OnPerceptionScalingEvent(event);
    }
}

void CGlobalPerceptionScaleHandler::Serialize(TSerialize ser)
{
    ser.BeginGroup("GlobalPerceptionScaleHandler");
    ser.Value("Visual", m_visual);
    ser.Value("Audio", m_audio);
    ser.EnumValue("FilterType", m_filterType, eAIFT_All, eAIFT_None);
    ser.Value("FactionID", m_factionID);
    ser.EndGroup();
}

void CGlobalPerceptionScaleHandler::DebugDraw() const
{
#ifdef CRYAISYSTEM_DEBUG
    const static char* filter[eAIFT_None + 1] =
    {
        "All",
        "Enemies",
        "Friends",
        "Faction",
        "None",
    };
    CDebugDrawContext dc;
    dc->TextToScreen(0, 60, "Global Perception Scale (Filtered by:%s). VISUAL: %f -- AUDIO %f",
        filter[m_filterType], m_visual, m_audio);
#endif
}