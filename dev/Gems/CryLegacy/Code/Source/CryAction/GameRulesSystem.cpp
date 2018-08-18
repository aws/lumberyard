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
#include "GameObjects/GameObjectSystem.h"
#include "GameObjects/GameObject.h"
#include "GameRulesSystem.h"

#include <list>

#ifdef WIN64
    #pragma warning ( disable : 4244 )
#endif


#define GAMERULES_GLOBAL_VARIABLE           ("g_gameRules")
#define GAMERULESID_GLOBAL_VARIABLE     ("g_gameRulesId")


//------------------------------------------------------------------------
CGameRulesSystem::CGameRulesSystem(ISystem* pSystem, IGameFramework* pGameFW)
    : m_pGameFW(pGameFW)
    , m_pGameRules(0)
    , m_currentGameRules(0)
{
}

//------------------------------------------------------------------------
CGameRulesSystem::~CGameRulesSystem()
{
}

//------------------------------------------------------------------------
bool CGameRulesSystem::RegisterGameRules(const char* rulesName, const char* extensionName)
{
    IEntityClassRegistry::SEntityClassDesc ruleClass;

    char scriptName[1024];
    sprintf_s(scriptName, "Scripts/GameRules/%s.lua", rulesName);

    ruleClass.sName = rulesName;
    ruleClass.sScriptFile = scriptName;
    ruleClass.pComponentUserCreateFunc = CreateGameObject;
    ruleClass.pComponentUserData = this;
    ruleClass.flags |= ECLF_INVISIBLE;

    if (!gEnv->pEntitySystem->GetClassRegistry()->RegisterStdClass(ruleClass))
    {
        CRY_ASSERT(0);
        return false;
    }

    std::pair<TGameRulesMap::iterator, bool> rit = m_GameRules.insert(TGameRulesMap::value_type(rulesName, SGameRulesDef()));
    rit.first->second.extension = extensionName;

    return true;
}

//------------------------------------------------------------------------
bool CGameRulesSystem::CreateGameRules(const char* rulesName)
{
    const char* name = GetGameRulesName(rulesName);
    TGameRulesMap::iterator it = m_GameRules.find(name);
    if (it == m_GameRules.end())
    {
        return false;
    }

    // If a rule is currently being used, ask the entity system to remove it
    DestroyGameRules();

    SEntitySpawnParams params;

    params.sName = "GameRules";
    params.pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(name);
    params.nFlags |= ENTITY_FLAG_NO_PROXIMITY | ENTITY_FLAG_UNREMOVABLE;
    params.id = 1;

    IEntity* pEntity = gEnv->pEntitySystem->SpawnEntity(params);
    CRY_ASSERT(pEntity);

    if (pEntity == NULL)
    {
        return false;
    }

    pEntity->Activate(true);

    if (pEntity->GetScriptTable())
    {
        IScriptSystem* pSS = gEnv->pScriptSystem;

        pSS->SetGlobalValue(GAMERULES_GLOBAL_VARIABLE, pEntity->GetScriptTable());
        pSS->SetGlobalValue(GAMERULESID_GLOBAL_VARIABLE, ScriptHandle((UINT_PTR)m_currentGameRules));
    }

    //since we re-instantiating game rules, let's get rid of everything related to previous match

    if (IGameplayRecorder* pGameplayRecorder = CCryAction::GetCryAction()->GetIGameplayRecorder())
    {
        pGameplayRecorder->Event(pEntity, GameplayEvent(eGE_GameReset));
    }

    return true;
}

//------------------------------------------------------------------------
bool CGameRulesSystem::DestroyGameRules()
{
    // If a rule is currently being used, ask the entity system to remove it
    if (m_currentGameRules)
    {
        IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_currentGameRules);
        if (pEntity)
        {
            pEntity->ClearFlags(ENTITY_FLAG_UNREMOVABLE);
        }

        gEnv->pEntitySystem->RemoveEntity(m_currentGameRules, true);
        SetCurrentGameRules(0);

        IScriptSystem* pSS = gEnv->pScriptSystem;

        pSS->SetGlobalToNull(GAMERULES_GLOBAL_VARIABLE);
        pSS->SetGlobalToNull(GAMERULESID_GLOBAL_VARIABLE);
    }

    return true;
}

//------------------------------------------------------------------------
bool CGameRulesSystem::HaveGameRules(const char* rulesName)
{
    const char* name = GetGameRulesName(rulesName);
    if (!name || !gEnv->pEntitySystem->GetClassRegistry()->FindClass(name))
    {
        return false;
    }

    if (m_GameRules.find(name) == m_GameRules.end())
    {
        return false;
    }

    return true;
}

//------------------------------------------------------------------------
void CGameRulesSystem::AddGameRulesAlias(const char* gamerules, const char* alias)
{
    if (SGameRulesDef* def = GetGameRulesDef(gamerules))
    {
        def->aliases.push_back(alias);
    }
}

//------------------------------------------------------------------------
void CGameRulesSystem::AddGameRulesLevelLocation(const char* gamerules, const char* mapLocation)
{
    if (SGameRulesDef* def = GetGameRulesDef(gamerules))
    {
        def->maplocs.push_back(mapLocation);
    }
}

//------------------------------------------------------------------------
const char* CGameRulesSystem::GetGameRulesLevelLocation(const char* gamerules, int i)
{
    if (SGameRulesDef* def = GetGameRulesDef(GetGameRulesName(gamerules)))
    {
        if (i >= 0 && i < def->maplocs.size())
        {
            return def->maplocs[i].c_str();
        }
    }
    return 0;
}

//------------------------------------------------------------------------
void CGameRulesSystem::SetCurrentGameRules(IGameRules* pGameRules)
{
    CryLogAlways("GameRules: Set game rules to %p, %s",
        pGameRules,
        (pGameRules && pGameRules->GetEntity()) ? pGameRules->GetEntity()->GetName() : "<unknown>");

    m_pGameRules = pGameRules;

    m_currentGameRules = m_pGameRules ? m_pGameRules->GetEntityId() : 0;
}

//------------------------------------------------------------------------
IGameRules* CGameRulesSystem::GetCurrentGameRules() const
{
    return m_pGameRules;
}

//------------------------------------------------------------------------
const char* CGameRulesSystem::GetGameRulesName(const char* alias) const
{
    for (TGameRulesMap::const_iterator it = m_GameRules.begin(); it != m_GameRules.end(); ++it)
    {
        if (!_stricmp(it->first.c_str(), alias))
        {
            return it->first.c_str();
        }

        for (std::vector<string>::const_iterator ait = it->second.aliases.begin(); ait != it->second.aliases.end(); ++ait)
        {
            if (!_stricmp(ait->c_str(), alias))
            {
                return it->first.c_str();
            }
        }
    }

    return 0;
}


//------------------------------------------------------------------------
/*
string& CGameRulesSystem::GetCurrentGameRules()
{
    return
}
*/

CGameRulesSystem::SGameRulesDef* CGameRulesSystem::GetGameRulesDef(const char* name)
{
    TGameRulesMap::iterator it = m_GameRules.find(name);
    if (it == m_GameRules.end())
    {
        return 0;
    }

    return &it->second;
}

//------------------------------------------------------------------------
IComponentPtr CGameRulesSystem::CreateGameObject(IEntity* pEntity, SEntitySpawnParams& params, void* pUserData)
{
    CGameRulesSystem* pThis = static_cast<CGameRulesSystem*>(pUserData);
    CRY_ASSERT(pThis);
    TGameRulesMap::iterator it = pThis->m_GameRules.find(params.pClass->GetName());
    CRY_ASSERT(it != pThis->m_GameRules.end());

    CGameObjectPtr pGameObject = gEnv->pEntitySystem->CreateComponentAndRegister<CGameObject>(IComponent::SComponentInitializer(pEntity));

    if (!it->second.extension.empty())
    {
        if (!pGameObject->ActivateExtension(it->second.extension.c_str()))
        {
            pEntity->DeregisterComponent(pGameObject);
            pGameObject.reset();
        }
    }

    return pGameObject;
}

void CGameRulesSystem::GetMemoryStatistics(ICrySizer* s)
{
    s->Add(*this);
    s->AddContainer(m_GameRules);
}