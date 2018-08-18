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


#include "CryLegacy_precompiled.h"
#include "ICryAnimation.h"
#include "ActorSystem.h"
#include "IGameFramework.h"
#include "VehicleSystem.h"
#include "ScriptBind_VehicleSystem.h"
#include "VehicleSystem/VehicleViewThirdPerson.h"

//------------------------------------------------------------------------
CScriptBind_VehicleSystem::CScriptBind_VehicleSystem(ISystem* pSystem, CVehicleSystem* vehicleSystem)
{
    m_pVehicleSystem = vehicleSystem;

    Init(gEnv->pScriptSystem, pSystem);
    SetGlobalName("Vehicle");

    RegisterGlobals();
    RegisterMethods();

    gEnv->pScriptSystem->ExecuteFile("Scripts/Entities/Vehicles/VehicleSystem.lua");
}

//------------------------------------------------------------------------
CScriptBind_VehicleSystem::~CScriptBind_VehicleSystem()
{
}

//------------------------------------------------------------------------
void CScriptBind_VehicleSystem::RegisterGlobals()
{
}

//------------------------------------------------------------------------
void CScriptBind_VehicleSystem::RegisterMethods()
{
#undef SCRIPT_REG_CLASSNAME
#define SCRIPT_REG_CLASSNAME &CScriptBind_VehicleSystem::

    SCRIPT_REG_FUNC(GetVehicleImplementations);
    SCRIPT_REG_TEMPLFUNC(GetOptionalScript, "vehicleName");
    SCRIPT_REG_TEMPLFUNC(ReloadSystem, "");
}


//------------------------------------------------------------------------
int CScriptBind_VehicleSystem::GetVehicleImplementations(IFunctionHandler* pH)
{
    SmartScriptTable tableImpls(m_pSS->CreateTable());
    SVehicleImpls impls;

    m_pVehicleSystem->GetVehicleImplementations(impls);

    //CryLog("Scriptbind got %i vehicles", impls.Count());
    for (int i = 0; i < impls.Count(); ++i)
    {
        tableImpls->SetAt(i + 1, impls.GetAt(i).c_str());
    }
    return pH->EndFunction(tableImpls);
}


//------------------------------------------------------------------------
int CScriptBind_VehicleSystem::GetOptionalScript(IFunctionHandler* pH, char* vehicleName)
{
    char scriptName[1024] = {0};

    if (m_pVehicleSystem->GetOptionalScript(vehicleName, scriptName, sizeof(scriptName)))
    {
        return pH->EndFunction(scriptName);
    }

    return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_VehicleSystem::ReloadSystem(IFunctionHandler* pH)
{
    m_pVehicleSystem->ReloadSystem();

    return pH->EndFunction();
}