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

// Description : Script Binding for the Vehicle Seat


#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_SCRIPTBIND_VEHICLESEAT_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_SCRIPTBIND_VEHICLESEAT_H
#pragma once

#include <IScriptSystem.h>
#include <ScriptHelpers.h>

struct IVehicleSystem;
struct IGameFramework;
class CVehicleSeat;

class CScriptBind_VehicleSeat
    : public CScriptableBase
{
public:

    CScriptBind_VehicleSeat(ISystem* pSystem, IGameFramework* pGameFW);
    virtual ~CScriptBind_VehicleSeat();

    void AttachTo(IVehicle* pVehicle, TVehicleSeatId seatId);
    void Release() { delete this; };

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }

    //! <code>VehicleSeat.GetVehicleSeat()</code>
    //! <description>Gets the vehicle seat identifier.</description>
    CVehicleSeat* GetVehicleSeat(IFunctionHandler* pH);

    //! <code>VehicleSeat.Reset()</code>
    //! <description>Resets the vehicle seat.</description>
    int Reset(IFunctionHandler* pH);

    //! <code>VehicleSeat.IsFree(actor)</code>
    //!     <param name="actorHandle">Passenger identifier.</param>
    //! <description>Checks if the seat is free.</description>
    int IsFree(IFunctionHandler* pH, ScriptHandle actorHandle);

    //! <code>VehicleSeat.IsDriver()</code>
    //! <description>Checks if the seat is the driver seat.</description>
    int IsDriver(IFunctionHandler* pH);

    //! <code>VehicleSeat.IsGunner()</code>
    //! <description>Checks if the seat is the gunner seat.</description>
    int IsGunner(IFunctionHandler* pH);

    //! <code>VehicleSeat.GetWeaponId(weaponIndex)</code>
    //!     <param name="weaponIndex">Weapon identifier.</param>
    //! <description>Gets the weapon identifier.</description>
    int GetWeaponId(IFunctionHandler* pH, int weaponIndex);

    //! <code>VehicleSeat.GetWeaponCount()</code>
    //! <description>Gets the number of weapons available on this seat.</description>
    int GetWeaponCount(IFunctionHandler* pH);

    //! <code>VehicleSeat.SetAIWeapon(weaponHandle)</code>
    //!     <param name="weaponHandle">Weapon identifier.</param>
    //! <description>Sets the weapon artificial intelligence.</description>
    int SetAIWeapon(IFunctionHandler* pH, ScriptHandle weaponHandle);

    //! <code>VehicleSeat.GetPassengerId()</code>
    //! <description>Gets the passenger identifier.</description>
    int GetPassengerId(IFunctionHandler* pH);

private:

    void RegisterGlobals();
    void RegisterMethods();

    IVehicleSystem* m_pVehicleSystem;
};

#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_SCRIPTBIND_VEHICLESEAT_H
