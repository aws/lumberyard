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

// Vehicle seat action: Passive animation.
//
// This is a simplified version of CVehicleSeatActionPassiveAnimation
// that will only run the animation as long as someone is seated in
// the vehicle.
//
// It will also not ForceFinish() on the animation action but justs stops
// it normally, so that we are guaranteed to trigger blend-transitions.
//


#include "CryLegacy_precompiled.h"

#include "IVehicleSystem.h"
#include "VehicleSeatActionPassiveAnimation.h"



CVehicleSeatActionPassiveAnimation::CVehicleSeatActionPassiveAnimation()
    : m_pAnimationAction(NULL)
    , m_animationFragmentID(FRAGMENT_ID_INVALID)
    , m_pVehicle(NULL)
    , m_userId((EntityId)0)
{
}


CVehicleSeatActionPassiveAnimation::~CVehicleSeatActionPassiveAnimation()
{
    StopAnimation();
}


bool CVehicleSeatActionPassiveAnimation::Init(IVehicle* pVehicle, IVehicleSeat* pSeat, const CVehicleParams& table)
{
    m_pVehicle = pVehicle;

    const char* fragmentName = table.getAttr("FragmentID");
    if (fragmentName == NULL)
    {
        return false;
    }
    if (fragmentName[0] == '\0')
    {
        return false;
    }

    IActionController* pActionController = m_pVehicle->GetAnimationComponent().GetActionController();
    if (pActionController == NULL)
    {
        return false;
    }

    m_animationFragmentID = pActionController->GetContext().controllerDef.m_fragmentIDs.Find(fragmentName);
    if (m_animationFragmentID == FRAGMENT_ID_INVALID)
    {
        CryLog("CVehicleSeatActionPassiveAnimation::Init(): Unable to find animation fragment ID '%s'!", fragmentName);
        return false;
    }

    return true;
}


void CVehicleSeatActionPassiveAnimation::Reset()
{
    if (m_userId != (EntityId)0)
    {
        StartAnimation();
    }
    else
    {
        StopAnimation();
    }
}


void CVehicleSeatActionPassiveAnimation::StartUsing(EntityId passengerId)
{
    m_userId = passengerId;

    if (m_animationFragmentID != FRAGMENT_ID_INVALID)
    {
        StartAnimation();
        m_pVehicle->SetObjectUpdate(this, IVehicle::eVOU_PassengerUpdate);
    }
}


void CVehicleSeatActionPassiveAnimation::StopUsing()
{
    if (m_animationFragmentID != FRAGMENT_ID_INVALID)
    {
        StopAnimation();
        m_pVehicle->SetObjectUpdate(this, IVehicle::eVOU_NoUpdate);
    }
    m_userId = (EntityId)0;
}


void CVehicleSeatActionPassiveAnimation::Serialize(TSerialize ser, EEntityAspects aspects)
{
    if (ser.GetSerializationTarget() != eST_Network)
    {
    }
}


void CVehicleSeatActionPassiveAnimation::StartAnimation()
{
    if ((m_animationFragmentID != FRAGMENT_ID_INVALID) && (m_pAnimationAction == NULL))
    {
        IActionController* pActionController = m_pVehicle->GetAnimationComponent().GetActionController();
        if (pActionController != NULL)
        {
            m_pAnimationAction = new TAction<SAnimationContext>(
                    IVehicleAnimationComponent::ePriority_SeatDefault,
                    m_animationFragmentID,
                    TAG_STATE_EMPTY);
            m_pAnimationAction->AddRef();
            pActionController->Queue(*m_pAnimationAction);
        }
    }
}

void CVehicleSeatActionPassiveAnimation::StopAnimation()
{
    if (m_pAnimationAction != NULL)
    {
        m_pAnimationAction->Stop(); // Just Stop() to trigger blend-transitions.
        SAFE_RELEASE(m_pAnimationAction);
    }
}


DEFINE_VEHICLEOBJECT(CVehicleSeatActionPassiveAnimation);
