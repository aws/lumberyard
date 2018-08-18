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

#ifndef CRYINCLUDE_CRYSCRIPTSYSTEM_FUNCTIONHANDLER_H
#define CRYINCLUDE_CRYSCRIPTSYSTEM_FUNCTIONHANDLER_H

#pragma once

#include "ScriptSystem.h"

/*! IFunctionHandler implementation
    @see IFunctionHandler
*/
class CFunctionHandler
    : public IFunctionHandler
{
public:
    CFunctionHandler(CScriptSystem* pSS, lua_State* lState, const char* sFuncName, int paramIdOffset)
    {
        m_pSS = pSS;
        L = lState;
        m_sFuncName = sFuncName;
        m_paramIdOffset = paramIdOffset;
    }
    ~CFunctionHandler() {}

public:
    int GetParamCount();
    ScriptVarType GetParamType(int nIdx);
    IScriptSystem* GetIScriptSystem();

    virtual void* GetThis();
    virtual bool  GetSelfAny(ScriptAnyValue& any);

    virtual const char* GetFuncName();

    //////////////////////////////////////////////////////////////////////////
    virtual bool GetParamAny(int nIdx, ScriptAnyValue& any);
    virtual int  EndFunctionAny(const ScriptAnyValue& any);
    virtual int  EndFunctionAny(const ScriptAnyValue& any1, const ScriptAnyValue& any2);
    virtual int  EndFunctionAny(const ScriptAnyValue& any1, const ScriptAnyValue& any2, const ScriptAnyValue& any3);
    virtual int  EndFunction();
    //////////////////////////////////////////////////////////////////////////

private:
    lua_State* L;
    CScriptSystem* m_pSS;
    const char* m_sFuncName;
    int m_paramIdOffset;
};

#endif // CRYINCLUDE_CRYSCRIPTSYSTEM_FUNCTIONHANDLER_H
