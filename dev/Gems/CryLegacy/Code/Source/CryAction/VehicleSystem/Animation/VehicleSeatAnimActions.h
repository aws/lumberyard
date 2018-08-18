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

// Description : Implements vehicle seat specific Mannequin actions


#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_ANIMATION_VEHICLESEATANIMACTIONS_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_ANIMATION_VEHICLESEATANIMACTIONS_H
#pragma once

#include <ICryMannequin.h>

class CVehicleSeatAnimActionEnter
    : public TAction<SAnimationContext>
{
public:

    DEFINE_ACTION("EnterVehicle");

    typedef TAction<SAnimationContext> BaseAction;

    CVehicleSeatAnimActionEnter(int priority, FragmentID fragmentID, CVehicleSeat* pSeat)
        : BaseAction(priority, fragmentID)
        , m_pSeat(pSeat)
    {
    }

    virtual void Enter();

    virtual void Exit()
    {
        BaseAction::Exit();

        // before sitting down, check if the enter-action is still valid and didn't get interrupted (CE-4209)
        if (m_pSeat->GetCurrentTransition())
        {
            m_pSeat->SitDown();
        }
    }

private:
    CVehicleSeat* m_pSeat;
};

class CVehicleSeatAnimActionExit
    : public TAction<SAnimationContext>
{
public:

    DEFINE_ACTION("ExitVehicle");

    typedef TAction<SAnimationContext> BaseAction;

    CVehicleSeatAnimActionExit(int priority, FragmentID fragmentID, CVehicleSeat* pSeat)
        : BaseAction(priority, fragmentID)
        , m_pSeat(pSeat)
    {
    }

    virtual void Enter();
    virtual void Exit();

private:
    CVehicleSeat* m_pSeat;
};

#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_ANIMATION_VEHICLESEATANIMACTIONS_H
