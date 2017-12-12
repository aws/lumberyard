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

#ifndef CRYINCLUDE_CRYACTION_CUSTOMEVENTS_CUSTOMEVENTSMANAGER_H
#define CRYINCLUDE_CRYACTION_CUSTOMEVENTS_CUSTOMEVENTSMANAGER_H
#pragma once

#include <ICustomEvents.h>
#include <CryListenerSet.h>

///////////////////////////////////////////////////
// Manager implementation to associate custom events
///////////////////////////////////////////////////
class CCustomEventManager
    : public ICustomEventManager
{
public:
    CCustomEventManager();
    virtual ~CCustomEventManager();

public:
    // ICustomEventManager
    virtual bool RegisterEventListener(ICustomEventListener* pListener, const TCustomEventId eventId);
    virtual bool UnregisterEventListener(ICustomEventListener* pListener, const TCustomEventId eventId);
    virtual bool UnregisterEvent(TCustomEventId eventId);
    virtual void Clear();
    virtual void FireEvent(const TCustomEventId eventId);
    virtual TCustomEventId GetNextCustomEventId();
    // ~ICustomEventManager

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        SIZER_COMPONENT_NAME(pSizer, "CustomEventManager");

        pSizer->AddObject(this, sizeof(*this));
    }

private:
    typedef CListenerSet<ICustomEventListener*> TCustomEventListeners;

    struct SCustomEventData
    {
        SCustomEventData()
            : m_listeners(0)
        {}

        TCustomEventListeners m_listeners;
    };

    // Mapping of custom event id to listeners
    typedef std::map<TCustomEventId, SCustomEventData> TCustomEventsMap;
    TCustomEventsMap m_customEventsData;
    TCustomEventId m_curHighestEventId;
};

#endif // CRYINCLUDE_CRYACTION_CUSTOMEVENTS_CUSTOMEVENTSMANAGER_H
