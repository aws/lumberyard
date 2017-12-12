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

// Description : refactoring - moved out of LeaderAction.h


#ifndef CRYINCLUDE_CRYAISYSTEM_UNITACTION_H
#define CRYINCLUDE_CRYAISYSTEM_UNITACTION_H
#pragma once

#include <ISerialize.h>
#include <list>
#include "AISignal.h"

enum EPriority
{
    PRIORITY_VERY_LOW = 1,
    PRIORITY_LOW,
    PRIORITY_NORMAL,
    PRIORITY_HIGH,
    PRIORITY_VERY_HIGH,
    PRIORITY_LAST   // make sure this one is always the last!
};

enum    EUnitAction
{
    UA_NONE,
    UA_SIGNAL,
    UA_SEARCH,
    UA_ACQUIRETARGET,
    UA_LAST,    // make sure this one is always the last!
};

struct AISignalExtraData;
class CUnitAction;

typedef std::list<CUnitAction*> TActionList;
typedef std::list<CUnitAction*> TActionListRef;

class CUnitAction
{
public:
    CUnitAction();
    CUnitAction(EUnitAction eAction, bool bBlocking);
    CUnitAction(EUnitAction eAction, bool bBlocking, const Vec3& point);
    CUnitAction(EUnitAction eAction, bool bBlocking, const Vec3& point, const Vec3& dir);
    CUnitAction(EUnitAction eAction, bool bBlocking, float fDistance);
    CUnitAction(EUnitAction eAction, bool bBlocking, const char* szText);
    CUnitAction(EUnitAction eAction, bool bBlocking, const char* szText, const AISignalExtraData& data);
    CUnitAction(EUnitAction eAction, bool bBlocking, int priority);
    CUnitAction(EUnitAction eAction, bool bBlocking, int priority, const Vec3& point);
    CUnitAction(EUnitAction eAction, bool bBlocking, int priority, const char* szText);
    CUnitAction(EUnitAction eAction, bool bBlocking, int priority, const char* szText, const AISignalExtraData& data);

    ~CUnitAction();
    void        Update();
    void        BlockedBy(CUnitAction* blockedAction);
    void        UnlockMyBlockedActions();
    void        NotifyMyBlockingActions();
    inline bool     IsBlocked() const {return !m_BlockingActions.empty(); };
    inline void     SetPriority(int priority) {m_Priority = priority; };
    void Serialize(TSerialize ser);

    EUnitAction     m_Action;
    bool                    m_BlockingPlan;
    TActionList     m_BlockingActions;
    TActionList     m_BlockedActions;
    int                     m_Priority;
    Vec3                    m_Point;
    Vec3                    m_Direction;
    float                   m_fDistance;
    string              m_SignalText;
    int                     m_Tag;
    AISignalExtraData m_SignalData;
};

#endif // CRYINCLUDE_CRYAISYSTEM_UNITACTION_H
