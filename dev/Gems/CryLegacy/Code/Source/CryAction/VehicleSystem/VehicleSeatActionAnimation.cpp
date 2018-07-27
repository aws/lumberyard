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

// Description : Implements a seat action for triggering animations

#include "CryLegacy_precompiled.h"

#include "IVehicleSystem.h"
#include "VehicleSeatActionAnimation.h"
#include "VehicleCVars.h"


//------------------------------------------------------------------------
CVehicleSeatActionAnimation::CVehicleSeatActionAnimation()
    : m_pVehicleAnim(NULL)
    , m_pVehicle(NULL)
    , m_userId(0)
    , m_action(0.f)
    , m_prevAction(0.f)
    , m_speed(1.f)
    , m_manualUpdate(true)
    , m_pAudioComponent(NULL)
{
    m_control[0] = eVAI_Attack1;
    m_control[1] = eVAI_Attack2;
}

//------------------------------------------------------------------------
bool CVehicleSeatActionAnimation::Init(IVehicle* pVehicle, IVehicleSeat* pSeat, const CVehicleParams& table)
{
    m_pVehicle = pVehicle;

    CVehicleParams animTable = table.findChild("Animation");
    if (!animTable)
    {
        return false;
    }

    if (animTable.haveAttr("vehicleAnimation"))
    {
        m_pVehicleAnim = m_pVehicle->GetAnimation(animTable.getAttr("vehicleAnimation"));
    }

    string control = animTable.getAttr("control");
    if (!control.empty())
    {
        if (control == "roll")
        {
            m_control[0] = eVAI_RollLeft;
            m_control[1] = eVAI_RollRight;
        }
    }

    animTable.getAttr("manualUpdate", m_manualUpdate);
    animTable.getAttr("speed", m_speed);

    // Audio: start/stop event?

    return true;
}

//------------------------------------------------------------------------
void CVehicleSeatActionAnimation::Reset()
{
    if (m_userId)
    {
        StopUsing();
    }
}

//------------------------------------------------------------------------
void CVehicleSeatActionAnimation::StartUsing(EntityId passengerId)
{
    m_userId = passengerId;
    m_action = m_prevAction = 0.f;

    if (m_pVehicleAnim)
    {
        if (m_manualUpdate)
        {
            m_pVehicleAnim->StartAnimation();
            m_pVehicleAnim->ToggleManualUpdate(true);

            m_pVehicle->SetObjectUpdate(this, IVehicle::eVOU_PassengerUpdate);
        }
    }
}

//------------------------------------------------------------------------
void CVehicleSeatActionAnimation::StopUsing()
{
    if (m_pVehicleAnim)
    {
        m_pVehicleAnim->StopAnimation();

        if (m_manualUpdate)
        {
            m_pVehicle->SetObjectUpdate(this, IVehicle::eVOU_NoUpdate);
        }

        // Audio: event needed?
    }

    m_userId = 0;
    m_action = m_prevAction = 0.f;
}

//------------------------------------------------------------------------
void CVehicleSeatActionAnimation::OnAction(const TVehicleActionId actionId, int activationMode, float value)
{
    if (!m_pVehicleAnim)
    {
        return;
    }

    if (actionId == m_control[0])
    {
        if (!m_manualUpdate)
        {
            m_pVehicleAnim->StartAnimation();
        }
        else
        {
            if (activationMode == eAAM_OnPress || activationMode == eAAM_OnRelease)
            {
                m_action += value * 2.f - 1.f;
            }
        }
    }
    else if (actionId == m_control[1])
    {
        if (!m_manualUpdate)
        {
            m_pVehicleAnim->StopAnimation();
        }
        else
        {
            if (activationMode == eAAM_OnPress || activationMode == eAAM_OnRelease)
            {
                m_action -= value * 2.f - 1.f;
            }
        }
    }
}

//------------------------------------------------------------------------
void CVehicleSeatActionAnimation::Update(float frameTime)
{
    if (!m_userId || !m_pVehicleAnim)
    {
        return;
    }

    if (m_action != 0.f)
    {
        float currTime = m_pVehicleAnim->GetAnimTime(true);
        float newTime = currTime + m_action * m_speed * frameTime;
        Limit(newTime, 0.f, 1.f);

        if (newTime != currTime)
        {
            m_pVehicleAnim->SetTime(newTime);

            // starting
            if (m_prevAction == 0.f /*&& m_pSound.get()*/)
            {
                // Audio: starting something
            }
        }

        //float color[] = {1,1,1,1};
        //gEnv->pRenderer->Draw2dLabel(200,250,1.5,color,false,"action: %.2f, time: %.2f, new: %.2f", m_action, currTime, newTime);
    }
    else
    {
        // stopping
        if (m_prevAction != 0.f)
        {
            // Audio: stopping something
        }
    }

    m_prevAction = m_action;
}

//------------------------------------------------------------------------
void CVehicleSeatActionAnimation::Serialize(TSerialize ser, EEntityAspects aspects)
{
    if (ser.GetSerializationTarget() != eST_Network)
    {
    }
}


DEFINE_VEHICLEOBJECT(CVehicleSeatActionAnimation);
