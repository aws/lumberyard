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
#include "ScriptBind_ActionMapManager.h"
#include "ActionMapManager.h"


//------------------------------------------------------------------------
CScriptBind_ActionMapManager::CScriptBind_ActionMapManager(ISystem* pSystem, CActionMapManager* pActionMapManager)
    : m_pSystem(pSystem)
    , m_pManager(pActionMapManager)
{
    Init(gEnv->pScriptSystem, m_pSystem);
    SetGlobalName("ActionMapManager");

    RegisterGlobals();
    RegisterMethods();
}

//------------------------------------------------------------------------
CScriptBind_ActionMapManager::~CScriptBind_ActionMapManager()
{
}

//------------------------------------------------------------------------
void CScriptBind_ActionMapManager::RegisterGlobals()
{
}

//------------------------------------------------------------------------
void CScriptBind_ActionMapManager::RegisterMethods()
{
#undef SCRIPT_REG_CLASSNAME
#define SCRIPT_REG_CLASSNAME &CScriptBind_ActionMapManager::

    SCRIPT_REG_TEMPLFUNC(EnableActionFilter, "name, enable");
    SCRIPT_REG_TEMPLFUNC(EnableActionMap, "name, enable");
    SCRIPT_REG_TEMPLFUNC(LoadFromXML, "name");
}

//------------------------------------------------------------------------
int CScriptBind_ActionMapManager::EnableActionFilter(IFunctionHandler* pH, const char* name, bool enable)
{
    m_pManager->EnableFilter(name, enable);

    return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_ActionMapManager::EnableActionMap(IFunctionHandler* pH, const char* name, bool enable)
{
    m_pManager->EnableActionMap(name, enable);

    return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_ActionMapManager::LoadFromXML(IFunctionHandler* pH, const char* name)
{
    XmlNodeRef rootNode = m_pSystem->LoadXmlFromFile(name);
    m_pManager->LoadFromXML(rootNode);

    return pH->EndFunction();
}
