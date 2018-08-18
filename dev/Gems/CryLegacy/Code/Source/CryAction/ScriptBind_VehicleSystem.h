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

// Description : Script Binding for the Vehicle System


#ifndef CRYINCLUDE_CRYACTION_SCRIPTBIND_VEHICLESYSTEM_H
#define CRYINCLUDE_CRYACTION_SCRIPTBIND_VEHICLESYSTEM_H
#pragma once

#include <IScriptSystem.h>
#include <ScriptHelpers.h>

struct IVehicleSystem;
struct IGameFramework;
class CVehicleSystem;

class CScriptBind_VehicleSystem
    : public CScriptableBase
{
public:
    CScriptBind_VehicleSystem(ISystem* pSystem, CVehicleSystem* vehicleSystem);
    virtual ~CScriptBind_VehicleSystem();

    void Release() { delete this; };

    //! <code>VehicleSystem.GetVehicleImplementations()</code>
    //! <description>Get a table of all implemented vehicles.</description>
    int GetVehicleImplementations(IFunctionHandler* pH);

    //! <code>VehicleSystem.GetOptionalScript(vehicleName)</code>
    //! <description>Get an (optional) script for the named vehicle.</description>
    int GetOptionalScript(IFunctionHandler* pH, char* vehicleName);

    //! <code>VehicleSystem.SetTpvDistance(distance)</code>
    //! <description>Distance of camera in third person view.</description>
    int SetTpvDistance(IFunctionHandler* pH, float distance);

    //! <code>VehicleSystem.SetTpvHeight(height)</code>
    //! <description>Height of camera in third person view.</description>
    int SetTpvHeight(IFunctionHandler* pH, float height);

    //! <code>VehicleSystem.ReloadSystem()</code>
    //! <description>Reloads the vehicle system with default values.</description>
    int ReloadSystem(IFunctionHandler* pH);

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }

private:

    void RegisterGlobals();
    void RegisterMethods();

    CVehicleSystem* m_pVehicleSystem;
};

#endif // CRYINCLUDE_CRYACTION_SCRIPTBIND_VEHICLESYSTEM_H
