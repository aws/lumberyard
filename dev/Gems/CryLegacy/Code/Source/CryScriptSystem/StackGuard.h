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

#ifndef CRYINCLUDE_CRYSCRIPTSYSTEM_STACKGUARD_H
#define CRYINCLUDE_CRYSCRIPTSYSTEM_STACKGUARD_H
#pragma once


struct LuaStackGuard
{
    LuaStackGuard(lua_State* p)
    {
        m_pLS = p;
        m_nTop = lua_gettop(m_pLS);
    }
    ~LuaStackGuard()
    {
        lua_settop(m_pLS, m_nTop);
    }
private:
    int m_nTop;
    lua_State* m_pLS;
};

//////////////////////////////////////////////////////////////////////////
// Stack validator.
//////////////////////////////////////////////////////////////////////////
#ifdef _DEBUG
struct LuaStackValidator
{
    const char* text;
    lua_State* L;
    int top;
    LuaStackValidator(lua_State* pL, const char* sText)
    {
        text = sText;
        L = pL;
        top = lua_gettop(L);
    }
    ~LuaStackValidator()
    {
        if (top != lua_gettop(L))
        {
            assert(0 && "Lua Stack Validation Failed");
            lua_settop(L, top);
        }
    }
};
#define CHECK_STACK(L) LuaStackValidator __stackCheck__((L), __FUNCTION__);
#else //_DEBUG
//#define CHECK_STACK(L) StackGuard __stackGuard__(L);
#define CHECK_STACK(L) (void)0;
#endif //_DEBUG

#endif // CRYINCLUDE_CRYSCRIPTSYSTEM_STACKGUARD_H
