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
#ifndef CRYINCLUDE_CRYACTION_SCRIPTBIND_PREFABMANAGER_H
#define CRYINCLUDE_CRYACTION_SCRIPTBIND_PREFABMANAGER_H
#pragma once

#include <IScriptSystem.h>
#include <ScriptHelpers.h>

class IPrefabManager;
struct IGameFramework;

//////////////////////////////////////////////////////////////////////////
// Script bindings for prefab manager
//////////////////////////////////////////////////////////////////////////
class CScriptBind_PrefabManager
    : public CScriptableBase
{
public:
    CScriptBind_PrefabManager(const std::shared_ptr<IPrefabManager>& prefabMan, IGameFramework* gameFramework);

protected:
    int LoadLibrary(IFunctionHandler* pH, const char* filename);
    int Spawn(IFunctionHandler* pH, ScriptHandle entityId, const char* libname, const char* prefabname, uint32 seed, uint32 nMaxSpawn);
    int Move(IFunctionHandler* pH, ScriptHandle entityId);
    int Delete(IFunctionHandler* pH, ScriptHandle entityId);
    int Hide(IFunctionHandler* pH, ScriptHandle entityId, bool bHide);

private:
    std::shared_ptr<IPrefabManager> m_prefabMan;
};


#endif // CRYINCLUDE_CRYACTION_SCRIPTBIND_PREFABMANAGER_H



