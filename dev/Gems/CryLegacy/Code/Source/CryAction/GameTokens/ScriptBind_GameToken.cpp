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

#include "CryLegacy_precompiled.h"
#include "ScriptBind_GameToken.h"
#include "IGameTokens.h"
#include <AzCore/Casting/numeric_cast.h>

namespace
{
    // Returns literal representation of the type value
    const char* ScriptAnyTypeToString(ScriptAnyType type)
    {
        switch (type)
        {
        case ANY_ANY:
            return "Any";
        case ANY_TNIL:
            return "Null";
        case ANY_TBOOLEAN:
            return "Boolean";
        case ANY_TSTRING:
            return "String";
        case ANY_TNUMBER:
            return "Number";
        case ANY_TFUNCTION:
            return "Function";
        case ANY_TTABLE:
            return "Table";
        case ANY_TUSERDATA:
            return "UserData";
        case ANY_THANDLE:
            return "Pointer";
        case ANY_TVECTOR:
            return "Vec3";
        default:
            return "Unknown";
        }
    }
};

void CScriptBind_GameToken::Release()
{
    delete this;
};

int CScriptBind_GameToken::SetToken(IFunctionHandler* pH)
{
    SCRIPT_CHECK_PARAMETERS(2);
    const char* tokenName = 0;
    if (pH->GetParams(tokenName) == false)
    {
        GameWarning("[GameToken.SetToken] Usage: GameToken.SetToken TokenName TokenValue]");
        return pH->EndFunction();
    }
    ScriptAnyValue val;
    TFlowInputData data;

    if (pH->GetParamAny(2, val) == false)
    {
        GameWarning("[GameToken.SetToken(%s)] Usage: GameToken.SetToken TokenName TokenValue]", tokenName);
        return pH->EndFunction();
    }
    switch (val.type)
    {
    case ANY_TBOOLEAN:
    {
        bool v;
        val.CopyTo(v);
        data.Set(v);
    }
    break;
    case ANY_TNUMBER:
    {
        float v;
        val.CopyTo(v);
        data.Set(v);
    }
    break;
    case ANY_TSTRING:
    {
        const char* v;
        val.CopyTo(v);
        data.Set(string(v));
    }
    break;
    case ANY_TVECTOR:
    {
        Vec3 v;
        val.CopyTo(v);
        data.Set(v);
    }
    case ANY_TTABLE:
    {
        float x, y, z;
        IScriptTable* pTable = val.table;
        assert (pTable != 0);
        if (pTable->GetValue("x", x) && pTable->GetValue("y", y) && pTable->GetValue("z", z))
        {
            data.Set(Vec3(x, y, z));
        }
        else
        {
            GameWarning("[GameToken.SetToken(%s)] Cannot convert parameter type '%s' to Vec3", tokenName, ScriptAnyTypeToString(val.type));
            return pH->EndFunction();
        }
    }
    break;
    case ANY_THANDLE:
    {
        ScriptHandle handle;
        val.CopyTo(handle);
        data.Set(FlowEntityId(aznumeric_cast<EntityId>(handle.n)));
    }
    break;
    default:
        GameWarning("[GameToken.SetToken(%s)] Cannot convert parameter type '%s'", tokenName, ScriptAnyTypeToString(val.type));
        return pH->EndFunction();
        break; // dummy ;-)
    }
#ifdef SCRIPT_GAMETOKEN_ALWAYS_CREATE
    m_pTokenSystem->SetOrCreateToken(tokenName, data);
#else
    IGameToken* pToken = m_pTokenSystem->FindToken(tokenName);
    if (!pToken)
    {
        GameWarning("[GameToken.SetToken] Cannot find token '%s'", tokenName);
    }
    else
    {
        pToken->SetValue(data);
    }
#endif
    return pH->EndFunction();
}

int CScriptBind_GameToken::GetToken(IFunctionHandler* pH, const char* sTokenName)
{
    IGameToken* pToken = m_pTokenSystem->FindToken(sTokenName);
    if (!pToken)
    {
        GameWarning("[GameToken.GetToken] Cannot find token '%s'", sTokenName);
        return pH->EndFunction();
    }

    return pH->EndFunction(pToken->GetValueAsString());
}

int CScriptBind_GameToken::DumpAllTokens(IFunctionHandler* pH)
{
    m_pTokenSystem->DumpAllTokens();
    return pH->EndFunction();
}

void CScriptBind_GameToken::GetMemoryUsage(ICrySizer* pSizer) const
{
    pSizer->AddObject(this, sizeof(*this));
}