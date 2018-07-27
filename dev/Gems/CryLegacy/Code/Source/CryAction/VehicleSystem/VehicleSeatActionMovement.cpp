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

// Description : Implements a seat action which handle the vehicle movement


#include "CryLegacy_precompiled.h"
#include "CryAction.h"
#include "IVehicleSystem.h"
#include "Vehicle.h"
#include "VehicleSeat.h"
#include "VehicleSeatActionMovement.h"

//------------------------------------------------------------------------
CVehicleSeatActionMovement::CVehicleSeatActionMovement()
    : m_delayedStop(0.0f)
    , m_actionForward(0.0f)
    , m_pVehicle(NULL)
    , m_pSeat(NULL)
{
}

//------------------------------------------------------------------------
CVehicleSeatActionMovement::~CVehicleSeatActionMovement()
{
}

//------------------------------------------------------------------------
bool CVehicleSeatActionMovement::Init(IVehicle* pVehicle, IVehicleSeat* pSeat, const CVehicleParams& table)
{
    return Init(pVehicle, pSeat);
}

//------------------------------------------------------------------------
bool CVehicleSeatActionMovement::Init(IVehicle* pVehicle, IVehicleSeat* pSeat)
{
    IVehicleMovement* pMovement = pVehicle->GetMovement();
    if (!pMovement)
    {
        return false;
    }

    m_pVehicle = pVehicle;
    m_pSeat = pSeat;

    return true;
}

//------------------------------------------------------------------------
void CVehicleSeatActionMovement::Reset()
{
    CRY_ASSERT(m_pVehicle);

    m_pVehicle->SetObjectUpdate(this, IVehicle::eVOU_NoUpdate);

    m_actionForward = 0.0f;
    m_delayedStop = 0.0f;

    if (IVehicleMovement* pMovement = m_pVehicle->GetMovement())
    {
        pMovement->Reset();
    }
}

//------------------------------------------------------------------------
void CVehicleSeatActionMovement::StartUsing(EntityId passengerId)
{
    IActorSystem* pActorSystem = CCryAction::GetCryAction()->GetIActorSystem();
    CRY_ASSERT(pActorSystem);

    IActor* pActor = pActorSystem->GetActor(passengerId);
    CRY_ASSERT(pActor);

    IVehicleMovement* pMovement = m_pVehicle->GetMovement();
    CRY_ASSERT(pMovement);

    if (!pMovement)
    {
        return;
    }

    if (m_delayedStop >= 0.0f)
    {
        m_pVehicle->SetObjectUpdate(this, IVehicle::eVOU_NoUpdate);
        m_delayedStop = 0.0f;

        pMovement->StopDriving();
    }

    pMovement->StartDriving(passengerId);
}

//------------------------------------------------------------------------
void CVehicleSeatActionMovement::StopUsing()
{
    IActorSystem* pActorSystem = CCryAction::GetCryAction()->GetIActorSystem();
    CRY_ASSERT(pActorSystem);

    IVehicleMovement* pMovement = m_pVehicle->GetMovement();
    if (!pMovement)
    {
        return;
    }

    CRY_ASSERT(m_pSeat);

    // default to continuing for a bit
    m_delayedStop = 0.8f;

    IActor* pActor = pActorSystem->GetActor(m_pSeat->GetPassenger());

    if (pActor && pActor->IsPlayer())
    {
        // if stopped already don't go anywhere
        IPhysicalEntity* pPhys = m_pVehicle->GetEntity()->GetPhysics();
        pe_status_dynamics status;
        if (pPhys && pPhys->GetStatus(&status))
        {
            if (status.v.GetLengthSquared() < 25.0f)
            {
                m_delayedStop = 0.0f;
            }
        }

        if (m_actionForward > 0.0f)
        {
            m_delayedStop = 1.5f;
        }

        if (pMovement->GetMovementType() == IVehicleMovement::eVMT_Air)
        {
            m_delayedStop *= 2.0f;
        }

        m_pVehicle->SetObjectUpdate(this, IVehicle::eVOU_AlwaysUpdate);

        // prevent full pedal being kept pressed, but give it a bit
        pMovement->OnAction(eVAI_MoveForward, eAAM_OnPress, 0.1f);
    }
    else
    {
        if (pMovement->GetMovementType() == IVehicleMovement::eVMT_Air)
        {
            m_delayedStop = 0.0f;
            m_pVehicle->SetObjectUpdate(this, IVehicle::eVOU_AlwaysUpdate);
        }
        else
        {
            pMovement->StopDriving();
        }
    }
}

//------------------------------------------------------------------------
void CVehicleSeatActionMovement::OnAction(const TVehicleActionId actionId, int activationMode, float value)
{
    if (actionId == eVAI_MoveForward)
    {
        m_actionForward = value;
    }
    else if (actionId == eVAI_MoveBack)
    {
        m_actionForward = -value;
    }

    IVehicleMovement* pMovement = m_pVehicle->GetMovement();
    CRY_ASSERT(pMovement);

    pMovement->OnAction(actionId, activationMode, value);
}

//------------------------------------------------------------------------
void CVehicleSeatActionMovement::Update(const float deltaTime)
{
    IVehicleMovement* pMovement = m_pVehicle->GetMovement();
    CRY_ASSERT(pMovement);

    bool isReadyToStopEngine = true;

    if (isReadyToStopEngine)
    {
        if (m_delayedStop > 0.0f)
        {
            m_delayedStop -= deltaTime;
        }

        if (m_delayedStop <= 0.0f)
        {
            m_pVehicle->SetObjectUpdate(this, IVehicle::eVOU_NoUpdate);
            pMovement->StopDriving();
        }
    }
}

DEFINE_VEHICLEOBJECT(CVehicleSeatActionMovement);
