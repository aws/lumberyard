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
#include "StdAfx.h"
#include "ScriptBind_LightningArc.h"
#include "LightningArc.h"

#undef SCRIPT_REG_CLASSNAME
#define SCRIPT_REG_CLASSNAME &CScriptBind_LightningArc::

namespace
{
    CLightningArc* GetLightningArc(IFunctionHandler* pFucntion)
    {
        return static_cast<CLightningArc*>(pFucntion->GetThis());
    }
}

CScriptBind_LightningArc::CScriptBind_LightningArc(ISystem* pSystem)
{
    IScriptSystem* pScriptSystem = pSystem->GetIScriptSystem();
    Init(pScriptSystem, pSystem, 1);

    SCRIPT_REG_TEMPLFUNC(TriggerSpark, "");
    SCRIPT_REG_TEMPLFUNC(Enable, "enable");
    SCRIPT_REG_TEMPLFUNC(ReadLuaParameters, "");
}

void CScriptBind_LightningArc::AttachTo(CLightningArc* pLightingArc)
{
    IEntity* pEntity = pLightingArc->GetEntity();
    IScriptTable* pScriptTable = pEntity->GetScriptTable();
    if (!pScriptTable)
    {
        return;
    }

    SmartScriptTable thisTable(m_pSS);

    thisTable->SetValue("__this", ScriptHandle(pLightingArc));

    IScriptTable* pMethodsTable = GetMethodsTable();
    thisTable->Delegate(pMethodsTable);

    pScriptTable->SetValue("lightningArc", thisTable);
}

int CScriptBind_LightningArc::TriggerSpark(IFunctionHandler* pFunction)
{
    CLightningArc* pLightningArc = GetLightningArc(pFunction);
    if (!pLightningArc)
    {
        return pFunction->EndFunction();
    }

    pLightningArc->TriggerSpark();

    return pFunction->EndFunction();
}

int CScriptBind_LightningArc::Enable(IFunctionHandler* pFunction, bool enable)
{
    CLightningArc* pLightningArc = GetLightningArc(pFunction);
    if (!pLightningArc)
    {
        return pFunction->EndFunction();
    }

    pLightningArc->Enable(enable);

    return pFunction->EndFunction();
}

int CScriptBind_LightningArc::ReadLuaParameters(IFunctionHandler* pFunction)
{
    CLightningArc* pLightningArc = GetLightningArc(pFunction);
    if (!pLightningArc)
    {
        return pFunction->EndFunction();
    }

    pLightningArc->ReadLuaParameters();

    return pFunction->EndFunction();
}
