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

// Description : Implements a seat group


#include "CryLegacy_precompiled.h"
#include "CryAction.h"
#include "GameObjects/GameObject.h"
#include "IActorSystem.h"
#include "IAnimatedCharacter.h"
#include "IVehicleSystem.h"
#include "Vehicle.h"
#include "VehicleSeat.h"
#include "VehicleSeatGroup.h"

//------------------------------------------------------------------------
bool CVehicleSeatGroup::Init(IVehicle* pVehicle, const CVehicleParams& paramsTable)
{
    m_pVehicle = static_cast<CVehicle*>(pVehicle);
    m_isSwitchingReverse = false;

    if (CVehicleParams seatsTable = paramsTable.findChild("Seats"))
    {
        int i = 0;
        int c = seatsTable.getChildCount();
        m_seats.reserve(c);

        for (; i < c; i++)
        {
            string seatName = seatsTable.getChild(i).getAttr("value");
            if (!seatName.empty())
            {
                TVehicleSeatId seatId = m_pVehicle->GetSeatId(seatName);
                if (CVehicleSeat* pSeat = (CVehicleSeat*)m_pVehicle->GetSeatById(seatId))
                {
                    pSeat->SetSeatGroup(this);
                    m_seats.push_back(pSeat);
                }
            }
        }
    }

    if (!paramsTable.getAttr("keepEngineWarm", m_isKeepingEngineWarm))
    {
        m_isKeepingEngineWarm = false;
    }

    return !m_seats.empty();
}

//------------------------------------------------------------------------
void CVehicleSeatGroup::Reset()
{
    m_isSwitchingReverse = false;
}

//------------------------------------------------------------------------
CVehicleSeat* CVehicleSeatGroup::GetSeatByIndex(unsigned int index)
{
    if (index >= 0 && index <= m_seats.size())
    {
        return m_seats[index];
    }

    return NULL;
}

//------------------------------------------------------------------------
CVehicleSeat* CVehicleSeatGroup::GetNextSeat(CVehicleSeat* pCurrentSeat)
{
    for (TVehicleSeatVector::iterator ite = m_seats.begin(); ite != m_seats.end(); ++ite)
    {
        CVehicleSeat* pSeat = *ite;
        if (pSeat == pCurrentSeat)
        {
            if (m_isSwitchingReverse)
            {
                if (ite == m_seats.begin())
                {
                    m_isSwitchingReverse = false;
                    ++ite;
                }
                else
                {
                    --ite;
                }
            }
            else
            {
                ++ite;

                if (ite == m_seats.end())
                {
                    m_isSwitchingReverse = true;
                    --ite;
                    --ite;
                }
            }

            return *ite;
        }
    }

    return NULL;
}

//------------------------------------------------------------------------
CVehicleSeat* CVehicleSeatGroup::GetNextFreeSeat(CVehicleSeat* pCurrentSeat)
{
    TVehicleSeatVector::iterator itCurrent = std::find(m_seats.begin(), m_seats.end(), pCurrentSeat);

    if (itCurrent == m_seats.end())
    {
        return NULL;
    }

    TVehicleSeatVector::iterator itFwd = itCurrent, itBack = itCurrent;

    IActor* pActor = pCurrentSeat->GetPassengerActor();

    while (!(itFwd == m_seats.end() && itBack == m_seats.begin()))
    {
        if (m_isSwitchingReverse)
        {
            if (itBack != m_seats.begin())
            {
                --itBack;

                if ((*itBack)->IsFree(pActor))
                {
                    return *itBack;
                }
            }
            else
            {
                m_isSwitchingReverse = false;
            }
        }
        else
        {
            if (itFwd != m_seats.end())
            {
                ++itFwd;

                if (itFwd == m_seats.end())
                {
                    m_isSwitchingReverse = true;
                }
                else
                {
                    if ((*itFwd)->IsFree(pActor))
                    {
                        return *itFwd;
                    }
                }
            }
            else
            {
                m_isSwitchingReverse = true;
            }
        }
    }

    return NULL;
}

//------------------------------------------------------------------------
bool CVehicleSeatGroup::IsGroupEmpty()
{
    const TVehicleSeatVector::const_iterator seatsEnd = m_seats.end();
    for (TVehicleSeatVector::const_iterator ite = m_seats.begin(); ite != seatsEnd; ++ite)
    {
        CVehicleSeat* pSeat = *ite;
        if (!pSeat->IsFree(NULL))
        {
            return false;
        }
    }

    return true;
}

//------------------------------------------------------------------------
void CVehicleSeatGroup::OnPassengerExit(CVehicleSeat* pSeat, EntityId passengerId)
{
}

//------------------------------------------------------------------------
void CVehicleSeatGroup::OnPassengerChangeSeat(CVehicleSeat* pNewSeat, CVehicleSeat* pOldSeat)
{
}

void CVehicleSeatGroup::GetMemoryUsage(ICrySizer* s) const
{
    s->Add(*this);
    s->AddContainer(m_seats);
}