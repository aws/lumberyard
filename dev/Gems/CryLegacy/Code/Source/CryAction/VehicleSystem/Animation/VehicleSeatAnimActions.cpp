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


#include "CryLegacy_precompiled.h"

#include "../Vehicle.h"
#include "../VehicleSeat.h"

#include "VehicleSeatAnimActions.h"

void CVehicleSeatAnimActionEnter::Enter()
{
    BaseAction::Enter();

    bool isThirdPerson = !m_pSeat->ShouldEnterInFirstPerson();
    TVehicleViewId viewId = InvalidVehicleViewId;
    TVehicleViewId firstViewId = InvalidVehicleViewId;

    while (viewId = m_pSeat->GetNextView(viewId))
    {
        if (viewId == firstViewId)
        {
            break;
        }

        if (firstViewId == InvalidVehicleViewId)
        {
            firstViewId = viewId;
        }

        if (IVehicleView* pView = m_pSeat->GetView(viewId))
        {
            if (pView->IsThirdPerson() == isThirdPerson)
            {
                break;
            }
        }
    }

    if (viewId != InvalidVehicleViewId)
    {
        m_pSeat->SetView(viewId);
    }

    IActor* pActor = m_pSeat->GetPassengerActor();
    CRY_ASSERT(pActor);

    IAnimatedCharacter* pAnimChar = pActor ? pActor->GetAnimatedCharacter() : NULL;
    if (pAnimChar)
    {
        pAnimChar->SetMovementControlMethods(eMCM_Animation, eMCM_Animation);
        pAnimChar->RequestPhysicalColliderMode(eColliderMode_Disabled, eColliderModeLayer_Game, "CVehicleSeatAnimActionEnter::Enter");
    }
}

void CVehicleSeatAnimActionExit::Enter()
{
    BaseAction::Enter();

    bool isThirdPerson = !m_pSeat->ShouldExitInFirstPerson();
    TVehicleViewId viewId = InvalidVehicleViewId;
    TVehicleViewId firstViewId = InvalidVehicleViewId;

    while (viewId = m_pSeat->GetNextView(viewId))
    {
        if (viewId == firstViewId)
        {
            break;
        }

        if (firstViewId == InvalidVehicleViewId)
        {
            firstViewId = viewId;
        }

        if (IVehicleView* pView = m_pSeat->GetView(viewId))
        {
            if (pView->IsThirdPerson() == isThirdPerson)
            {
                break;
            }
        }
    }

    if (viewId != InvalidVehicleViewId)
    {
        m_pSeat->SetView(viewId);
    }

    IActor* pActor = m_pSeat->GetPassengerActor();
    CRY_ASSERT(pActor);

    IAnimatedCharacter* pAnimChar = pActor ? pActor->GetAnimatedCharacter() : NULL;
    if (pAnimChar)
    {
        pAnimChar->SetMovementControlMethods(eMCM_Animation, eMCM_Animation);
        pAnimChar->RequestPhysicalColliderMode(eColliderMode_Disabled, eColliderModeLayer_Game, "CVehicleSeatAnimActionExit::Enter");
    }
}

void CVehicleSeatAnimActionExit::Exit()
{
    BaseAction::Exit();

    IActor* pActor = m_pSeat->GetPassengerActor();
    CRY_ASSERT(pActor);

    m_pSeat->StandUp();
}
