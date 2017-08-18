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

#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLESEATGROUP_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLESEATGROUP_H
#pragma once

#include "vector"

class CVehicleSeat;

class CVehicleSeatGroup
{
public:

    bool Init(IVehicle* pVehicle, const CVehicleParams& paramsTable);
    void Reset();
    void Release() { delete this; }

    void GetMemoryUsage(ICrySizer* s) const;

    unsigned int GetSeatCount() { return m_seats.size(); }
    CVehicleSeat* GetSeatByIndex(unsigned int index);
    CVehicleSeat* GetNextSeat(CVehicleSeat* pCurrentSeat);
    CVehicleSeat* GetNextFreeSeat(CVehicleSeat* pCurrentSeat);

    bool IsGroupEmpty();

    void OnPassengerEnter(CVehicleSeat* pSeat, EntityId passengerId) {}
    void OnPassengerExit(CVehicleSeat* pSeat, EntityId passengerId);
    void OnPassengerChangeSeat(CVehicleSeat* pNewSeat, CVehicleSeat* pOldSeat);

protected:

    CVehicle* m_pVehicle;

    bool m_isKeepingEngineWarm;

    typedef std::vector <CVehicleSeat*> TVehicleSeatVector;
    TVehicleSeatVector m_seats;

    bool m_isSwitchingReverse;

    friend class CVehicleSeat;
};

#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLESEATGROUP_H
