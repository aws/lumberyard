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
#include "CryLegacy_precompiled.h"
#include "ScriptBind_PrefabManager.h"
#include <IGameFramework.h>
#include <IPrefabManager.h>

CScriptBind_PrefabManager::CScriptBind_PrefabManager(const std::shared_ptr<IPrefabManager>& prefabMan, IGameFramework* gameFramework)
    : m_prefabMan(prefabMan)
{
    Init(gameFramework->GetISystem()->GetIScriptSystem(), gameFramework->GetISystem());
    SetGlobalName("PrefabManager");

#undef SCRIPT_REG_CLASSNAME
#define SCRIPT_REG_CLASSNAME &CScriptBind_PrefabManager::

    SCRIPT_REG_TEMPLFUNC(LoadLibrary, "filename");
    SCRIPT_REG_TEMPLFUNC(Spawn, "entityId, libname, prefabname, seed, nMaxSpawn");
    SCRIPT_REG_TEMPLFUNC(Move, "entityId");
    SCRIPT_REG_TEMPLFUNC(Delete, "entityId");
    SCRIPT_REG_TEMPLFUNC(Hide, "entityId, bHide");

#undef SCRIPT_REG_CLASSNAME
}

int CScriptBind_PrefabManager::LoadLibrary(IFunctionHandler* pH, const char* filename)
{
    m_prefabMan->LoadPrefabLibrary(filename);
    return pH->EndFunction();
}

int CScriptBind_PrefabManager::Spawn(IFunctionHandler* pH, ScriptHandle entityId, const char* libname, const char* prefabname, uint32 seed, uint32 nMaxSpawn)
{
    EntityId nID = (EntityId)entityId.n;
    if (IEntity* pEntity = gEnv->pEntitySystem->GetEntity(nID))
    {
        m_prefabMan->SpawnPrefab(libname, prefabname, nID, seed, nMaxSpawn);
    }
    return pH->EndFunction();
}

int CScriptBind_PrefabManager::Move(IFunctionHandler* pH, ScriptHandle entityId)
{
    EntityId nID = (EntityId)entityId.n;
    if (IEntity* pEntity = gEnv->pEntitySystem->GetEntity(nID))
    {
        m_prefabMan->MovePrefab(nID);
    }
    return pH->EndFunction();
}

int CScriptBind_PrefabManager::Delete(IFunctionHandler* pH, ScriptHandle entityId)
{
    EntityId nID = (EntityId)entityId.n;
    if (IEntity* pEntity = gEnv->pEntitySystem->GetEntity(nID))
    {
        m_prefabMan->DeletePrefab(nID);
    }
    return pH->EndFunction();
}

int CScriptBind_PrefabManager::Hide(IFunctionHandler* pH, ScriptHandle entityId, bool bHide)
{
    EntityId nID = (EntityId)entityId.n;
    if (IEntity* pEntity = gEnv->pEntitySystem->GetEntity(nID))
    {
        m_prefabMan->HidePrefab(nID, bHide);
    }
    return pH->EndFunction();
}
