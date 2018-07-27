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

// Description : MaterialEffects ScriptBind. MaterialEffects should be used
//               from C++ if possible  Use this ScriptBind for legacy stuff


#include "CryLegacy_precompiled.h"
#include "ScriptBind_MaterialEffects.h"
#include "MaterialEffects.h"

//------------------------------------------------------------------------
CScriptBind_MaterialEffects::CScriptBind_MaterialEffects(ISystem* pSystem, CMaterialEffects* pMFX)
{
    m_pSystem = pSystem;
    m_pMFX = pMFX;
    assert (m_pMFX != 0);

    Init(gEnv->pScriptSystem, m_pSystem);
    SetGlobalName("MaterialEffects");

    RegisterGlobals();
    RegisterMethods();
}

//------------------------------------------------------------------------
CScriptBind_MaterialEffects::~CScriptBind_MaterialEffects()
{
}

//------------------------------------------------------------------------
void CScriptBind_MaterialEffects::RegisterGlobals()
{
    RegisterGlobal("MFX_INVALID_EFFECTID", InvalidEffectId);
}

//------------------------------------------------------------------------
void CScriptBind_MaterialEffects::RegisterMethods()
{
#undef SCRIPT_REG_CLASSNAME
#define SCRIPT_REG_CLASSNAME &CScriptBind_MaterialEffects::

    SCRIPT_REG_TEMPLFUNC(GetEffectId, "customName, surfaceIndex2");
    SCRIPT_REG_TEMPLFUNC(GetEffectIdByLibName, "LibName, FGFXName");
    SCRIPT_REG_TEMPLFUNC(PrintEffectIdByMatIndex, "MatName1, MatName2");
    SCRIPT_REG_TEMPLFUNC(ExecuteEffect, "effectId, paramsTable");
}

//------------------------------------------------------------------------
int CScriptBind_MaterialEffects::GetEffectId(IFunctionHandler* pH, const char* customName, int surfaceIndex2)
{
    TMFXEffectId effectId = m_pMFX->GetEffectId(customName, surfaceIndex2);

    return pH->EndFunction(effectId);
}

//------------------------------------------------------------------------
int CScriptBind_MaterialEffects::GetEffectIdByLibName(IFunctionHandler* pH,  const char* LibName, const char* FGFXName)
{
    TMFXEffectId effectId = m_pMFX->GetEffectIdByName(LibName, FGFXName);

    return pH->EndFunction(effectId);
}

//------------------------------------------------------------------------
int CScriptBind_MaterialEffects::PrintEffectIdByMatIndex(IFunctionHandler* pH,  int materialIndex1, int materialIndex2)
{
    TMFXEffectId effectId = m_pMFX->InternalGetEffectId(materialIndex1, materialIndex2);

    CryLogAlways("Requested MaterialEffect ID: %d", effectId);

    return pH->EndFunction(effectId);
}

//------------------------------------------------------------------------
int CScriptBind_MaterialEffects::ExecuteEffect(IFunctionHandler* pH, int effectId, SmartScriptTable paramsTable)
{
    if (effectId == InvalidEffectId)
    {
        return pH->EndFunction(false);
    }

    // minimalistic implementation.. extend if you need it
    SMFXRunTimeEffectParams params;
    paramsTable->GetValue("pos", params.pos);
    paramsTable->GetValue("normal", params.normal);
    paramsTable->GetValue("scale", params.scale);
    paramsTable->GetValue("angle", params.angle);

    bool res = m_pMFX->ExecuteEffect(effectId, params);

    return pH->EndFunction(res);
}
