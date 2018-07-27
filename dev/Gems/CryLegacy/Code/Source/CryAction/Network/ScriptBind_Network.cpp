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

// Description : Binding of network functions into script


#include "CryLegacy_precompiled.h"
#include "ScriptBind_Network.h"
#include "GameContext.h"
#include "ScriptSerialize.h"
#include "CryAction.h"

#include "IEntitySystem.h"
#include "IEntityClass.h"
#include "IConsole.h"
#include "IPhysics.h"

//------------------------------------------------------------------------
CScriptBind_Network::CScriptBind_Network(ISystem* pSystem, CCryAction* pFW)
{
    m_pSystem = pSystem;
    m_pFW = pFW;

    Init(gEnv->pScriptSystem, m_pSystem);
    SetGlobalName("Net");

    RegisterGlobals();
    RegisterMethods();
}

//------------------------------------------------------------------------
CScriptBind_Network::~CScriptBind_Network()
{
}

//------------------------------------------------------------------------
void CScriptBind_Network::RegisterGlobals()
{
    RegisterGlobal("UNRELIABLE_ORDERED", eNRT_UnreliableOrdered);
    RegisterGlobal("RELIABLE_ORDERED", eNRT_ReliableOrdered);
    RegisterGlobal("RELIABLE_UNORDERED", eNRT_ReliableUnordered);

    RegisterGlobal("NO_ATTACH", eRAT_NoAttach);
    RegisterGlobal("PRE_ATTACH", eRAT_PreAttach);
    RegisterGlobal("POST_ATTACH", eRAT_PostAttach);

    RegisterGlobal("BOOL", eSST_Bool);
    RegisterGlobal("FLOAT", eSST_Float);
    RegisterGlobal("INT8", eSST_Int8);
    RegisterGlobal("INT16", eSST_Int16);
    RegisterGlobal("INT32", eSST_Int32);
    RegisterGlobal("STRING", eSST_String);
    RegisterGlobal("STRINGTABLE", eSST_StringTable);
    RegisterGlobal("ENTITYID", eSST_EntityId);
    RegisterGlobal("VEC3", eSST_Vec3);
}

//------------------------------------------------------------------------
void CScriptBind_Network::RegisterMethods()
{
#undef SCRIPT_REG_CLASSNAME
#define SCRIPT_REG_CLASSNAME &CScriptBind_Network::

    SCRIPT_REG_FUNC(Expose);
}

//------------------------------------------------------------------------
int CScriptBind_Network::Expose(IFunctionHandler* pFH)
{
    return m_pFW->NetworkExposeClass(pFH);
}
