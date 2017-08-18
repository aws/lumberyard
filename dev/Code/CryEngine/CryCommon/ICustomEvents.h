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

#ifndef CRYINCLUDE_CRYCOMMON_ICUSTOMEVENTS_H
#define CRYINCLUDE_CRYCOMMON_ICUSTOMEVENTS_H
#pragma once


#define CUSTOMEVENTS_PREFABS_MAXNPERINSTANCE 6

// Represents an event
typedef uint32 TCustomEventId;

// Invalid event id
static const TCustomEventId CUSTOMEVENTID_INVALID = 0;

///////////////////////////////////////////////////
// Custom event listener
///////////////////////////////////////////////////
struct ICustomEventListener
{
    // <interfuscator:shuffle>
    virtual ~ICustomEventListener(){}
    virtual void OnCustomEvent(const TCustomEventId eventId) = 0;
    // </interfuscator:shuffle>
};

///////////////////////////////////////////////////
// Custom event manager interface
///////////////////////////////////////////////////
struct ICustomEventManager
{
    // <interfuscator:shuffle>
    virtual ~ICustomEventManager(){}

    // Registers custom event listener
    virtual bool RegisterEventListener(ICustomEventListener* pListener, const TCustomEventId eventId) = 0;

    // Unregisters custom event listener
    virtual bool UnregisterEventListener(ICustomEventListener* pListener, const TCustomEventId eventId) = 0;

    // Unregisters all listeners associated to an event
    virtual bool UnregisterEvent(TCustomEventId eventId) = 0;

    // Clear event data
    virtual void Clear() = 0;

    // Fires custom event
    virtual void FireEvent(const TCustomEventId eventId) = 0;

    // Gets next free event id
    virtual TCustomEventId GetNextCustomEventId() = 0;
    // </interfuscator:shuffle>
};

#endif // CRYINCLUDE_CRYCOMMON_ICUSTOMEVENTS_H
