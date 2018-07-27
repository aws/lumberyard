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

// Description : Refactoring CLeadeActions; adding Serialization support


#include "CryLegacy_precompiled.h"
#include "UnitAction.h"
#include "AISignal.h"
#include <ISerialize.h>


//
//----------------------------------------------------------------------------------------------------
CUnitAction::CUnitAction()
    : m_Action(UA_NONE)
{
}

//
//----------------------------------------------------------------------------------------------------
CUnitAction::CUnitAction(EUnitAction eAction, bool bBlocking)
    : m_Action(eAction)
    , m_BlockingPlan(bBlocking)
    , m_Priority(PRIORITY_NORMAL)
{
}

//
//----------------------------------------------------------------------------------------------------
CUnitAction::CUnitAction(EUnitAction eAction, bool bBlocking, const Vec3& point)
    : m_Action(eAction)
    , m_BlockingPlan(bBlocking)
    , m_Point(point)
    , m_Priority(PRIORITY_NORMAL)
    , m_fDistance(1)
{
}
//
//----------------------------------------------------------------------------------------------------
CUnitAction::CUnitAction(EUnitAction eAction, bool bBlocking, const Vec3& point, const Vec3& dir)
    : m_Action(eAction)
    , m_BlockingPlan(bBlocking)
    , m_Point(point)
    , m_Direction(dir)
    , m_Priority(PRIORITY_NORMAL)
    , m_fDistance(1)
{
}
//
//----------------------------------------------------------------------------------------------------
CUnitAction::CUnitAction(EUnitAction eAction, bool bBlocking, float fDistance)
    : m_Action(eAction)
    , m_BlockingPlan(bBlocking)
    , m_fDistance(fDistance)
    , m_Priority(PRIORITY_NORMAL)
{
}

//
//----------------------------------------------------------------------------------------------------
CUnitAction::CUnitAction(EUnitAction eAction, bool bBlocking, const char* szText)
    : m_Action(eAction)
    , m_BlockingPlan(bBlocking)
    , m_SignalText(szText)
    , m_Priority(PRIORITY_NORMAL)
    , m_fDistance(1)
{
}

//
//----------------------------------------------------------------------------------------------------
CUnitAction::CUnitAction(EUnitAction eAction, bool bBlocking, const char* szText, const AISignalExtraData& data)
    : m_Action(eAction)
    , m_BlockingPlan(bBlocking)
    , m_SignalText(szText)
    , m_Priority(PRIORITY_NORMAL)
    , m_fDistance(1)
    , m_SignalData(data)
{
}

//
//----------------------------------------------------------------------------------------------------
CUnitAction::CUnitAction(EUnitAction eAction, bool bBlocking, int priority)
    : m_Action(eAction)
    , m_BlockingPlan(bBlocking)
    , m_Priority(priority)
    , m_fDistance(1)
{
}

//
//----------------------------------------------------------------------------------------------------
CUnitAction::CUnitAction(EUnitAction eAction, bool bBlocking, int priority, const Vec3& point)
    : m_Action(eAction)
    , m_BlockingPlan(bBlocking)
    , m_Point(point)
    , m_Priority(priority)
    , m_fDistance(1)
{
}

//
//----------------------------------------------------------------------------------------------------
CUnitAction::CUnitAction(EUnitAction eAction, bool bBlocking, int priority, const char* szText)
    : m_Action(eAction)
    , m_BlockingPlan(bBlocking)
    , m_SignalText(szText)
    , m_Priority(priority)
    , m_fDistance(1)
{
}

//
//----------------------------------------------------------------------------------------------------
CUnitAction::CUnitAction(EUnitAction eAction, bool bBlocking, int priority, const char* szText, const AISignalExtraData& data)
    : m_Action(eAction)
    , m_BlockingPlan(bBlocking)
    , m_SignalText(szText)
    , m_Priority(priority)
    , m_fDistance(1)
    , m_SignalData(data)
{
}

//
//----------------------------------------------------------------------------------------------------
CUnitAction::~CUnitAction()
{
    //unlock all my blocked actions
    UnlockMyBlockedActions();
    NotifyMyBlockingActions();
}

//
//----------------------------------------------------------------------------------------------------
void CUnitAction::BlockedBy(CUnitAction* blockingAction)
{
    m_BlockingActions.push_back(blockingAction);
    blockingAction->m_BlockedActions.push_back(this);
}

//
//----------------------------------------------------------------------------------------------------
void CUnitAction::UnlockMyBlockedActions()
{
    // unlock all the blocked actions
    //
    for (TActionList::iterator itAction = m_BlockedActions.begin(); itAction != m_BlockedActions.end(); ++itAction)
    {
        CUnitAction* blockedAction = (*itAction);
        TActionList::iterator itOtherUnitAction = find(blockedAction->m_BlockingActions.begin(), blockedAction->m_BlockingActions.end(), this);

        if (itOtherUnitAction != blockedAction->m_BlockingActions.end())
        {
            blockedAction->m_BlockingActions.erase(itOtherUnitAction);// delete myself's pointer from the other action's blocking actions list
        }
    }
}

//
//----------------------------------------------------------------------------------------------------
void CUnitAction::NotifyMyBlockingActions()
{
    // remove myself from the Blocking actions' list (so they won't block a finished action)
    for (TActionList::iterator itAction = m_BlockingActions.begin(); itAction != m_BlockingActions.end(); ++itAction)
    {
        CUnitAction* blockingAction = (*itAction);
        TActionList::iterator itOtherUnitAction = std::find(blockingAction->m_BlockedActions.begin(), blockingAction->m_BlockedActions.end(), this);

        if (itOtherUnitAction != blockingAction->m_BlockedActions.end())
        {
            blockingAction->m_BlockedActions.erase(itOtherUnitAction);// delete myself's pointer from the other action's blocking actions list
        }
    }
}

