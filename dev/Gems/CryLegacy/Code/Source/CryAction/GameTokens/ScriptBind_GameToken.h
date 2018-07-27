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

#ifndef CRYINCLUDE_CRYACTION_GAMETOKENS_SCRIPTBIND_GAMETOKEN_H
#define CRYINCLUDE_CRYACTION_GAMETOKENS_SCRIPTBIND_GAMETOKEN_H
#pragma once

#include "ScriptHelpers.h"
#include "IGameTokens.h"
#include "GameTokenSystem.h"

/*! <description>This class implements script functions for game tokens.</description> */
class CScriptBind_GameToken
    : public CScriptableBase
{
public:
    CScriptBind_GameToken(CGameTokenSystem* pTokenSystem)
    {
        m_pTokenSystem = pTokenSystem;

        Init(gEnv->pScriptSystem, GetISystem());
        SetGlobalName("GameToken");

#undef SCRIPT_REG_CLASSNAME
#define SCRIPT_REG_CLASSNAME &CScriptBind_GameToken::

        SCRIPT_REG_FUNC(SetToken);
        SCRIPT_REG_TEMPLFUNC(GetToken, "sTokenName");
        SCRIPT_REG_FUNC(DumpAllTokens);
    }

    virtual ~CScriptBind_GameToken(void)
    {
    }

    void Release();

    //! <code>GameToken.SetToken( TokenName, TokenValue )</code>
    //! <description>Set the value of a game token.</description>
    int SetToken(IFunctionHandler* pH);

    //! <code>GameToken.GetToken( TokenName )</code>
    //! <description>Get the value of a game token.</description>
    int GetToken(IFunctionHandler* pH, const char* sTokenName);

    //! <code>GameToken.DumpAllTokens()</code>
    //! <description>Dump all game tokens with their values to the log.</description>
    int DumpAllTokens(IFunctionHandler* pH);

    virtual void GetMemoryUsage(ICrySizer* pSizer) const;

private:
    CGameTokenSystem* m_pTokenSystem;
};


#endif // CRYINCLUDE_CRYACTION_GAMETOKENS_SCRIPTBIND_GAMETOKEN_H
