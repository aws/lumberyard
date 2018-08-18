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

#include "CustomEvents/CustomEventsManager.h"

///////////////////////////////////////////////////
// Manager implementation to associate custom events
///////////////////////////////////////////////////

//------------------------------------------------------------------------------------------------------------------------
CCustomEventManager::CCustomEventManager()
    : m_curHighestEventId(CUSTOMEVENTID_INVALID)
{
}

//------------------------------------------------------------------------------------------------------------------------
CCustomEventManager::~CCustomEventManager()
{
    Clear();
}

//------------------------------------------------------------------------------------------------------------------------
bool CCustomEventManager::RegisterEventListener(ICustomEventListener* pListener, const TCustomEventId eventId)
{
    SCustomEventData& eventData = m_customEventsData[ eventId ]; // Creates new entry if doesn't exist
    TCustomEventListeners& listeners = eventData.m_listeners;
    return listeners.Add(pListener);
}

//------------------------------------------------------------------------------------------------------------------------
bool CCustomEventManager::UnregisterEventListener(ICustomEventListener* pListener, const TCustomEventId eventId)
{
    TCustomEventsMap::iterator eventsDataIter = m_customEventsData.find(eventId);
    if (eventsDataIter != m_customEventsData.end())
    {
        SCustomEventData& eventData = eventsDataIter->second;
        TCustomEventListeners& listeners = eventData.m_listeners;

        if (listeners.Contains(pListener))
        {
            listeners.Remove(pListener);
            return true;
        }
        else
        {
            CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "CCustomEventManager::UnregisterEventListener: Listener isn't registered for event id: %u", eventId);
        }
    }
    else
    {
        CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "CCustomEventManager::UnregisterEventListener: No event data exists for event id: %u", eventId);
    }

    return false;
}

//------------------------------------------------------------------------------------------------------------------------
bool CCustomEventManager::UnregisterEvent(TCustomEventId eventId)
{
    TCustomEventsMap::iterator eventsDataIter = m_customEventsData.find(eventId);
    if (eventsDataIter != m_customEventsData.end())
    {
        SCustomEventData& eventData = eventsDataIter->second;
        TCustomEventListeners& listeners = eventData.m_listeners;
        listeners.Clear(true);
        m_customEventsData.erase(eventsDataIter);
        return true;
    }

    return false;
}

//------------------------------------------------------------------------------------------------------------------------
void CCustomEventManager::Clear()
{
    m_customEventsData.clear();
    m_curHighestEventId = CUSTOMEVENTID_INVALID;
}

//------------------------------------------------------------------------------------------------------------------------
void CCustomEventManager::FireEvent(const TCustomEventId eventId)
{
    TCustomEventsMap::iterator eventsDataIter = m_customEventsData.find(eventId);
    if (eventsDataIter != m_customEventsData.end())
    {
        SCustomEventData& eventData = eventsDataIter->second;
        TCustomEventListeners& listeners = eventData.m_listeners;
        for (TCustomEventListeners::Notifier notifier(listeners); notifier.IsValid(); notifier.Next())
        {
            notifier->OnCustomEvent(eventId);
        }
    }
    else
    {
        CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "CCustomEventManager::FireEvent: No event data exists for event id: %u", eventId);
    }
}

//------------------------------------------------------------------------------------------------------------------------
TCustomEventId CCustomEventManager::GetNextCustomEventId()
{
    const TCustomEventId curNextId = m_curHighestEventId + 1;
    m_curHighestEventId = curNextId;
    return curNextId;
}