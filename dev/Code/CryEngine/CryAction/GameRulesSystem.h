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

#ifndef CRYINCLUDE_CRYACTION_GAMERULESSYSTEM_H
#define CRYINCLUDE_CRYACTION_GAMERULESSYSTEM_H
#pragma once

#include "IGameRulesSystem.h"

class CGameServerNub;

class CGameRulesSystem
    : public IGameRulesSystem
{
    typedef struct SGameRulesDef
    {
        string extension;
        std::vector<string> aliases;
        std::vector<string> maplocs;

        void GetMemoryUsage(ICrySizer* pSizer) const
        {
            pSizer->AddObject(extension);
            pSizer->AddObject(aliases);
            pSizer->AddObject(maplocs);
        }
    }SGameRulesDef;

    typedef std::map<string, SGameRulesDef> TGameRulesMap;
public:
    CGameRulesSystem(ISystem* pSystem, IGameFramework* pGameFW);
    ~CGameRulesSystem();

    void Release() { delete this; };

    virtual bool RegisterGameRules(const char* rulesName, const char* extensionName);
    virtual bool CreateGameRules(const char* rulesName);
    virtual bool DestroyGameRules();
    virtual bool HaveGameRules(const char* rulesName);

    virtual void AddGameRulesAlias(const char* gamerules, const char* alias);
    virtual void AddGameRulesLevelLocation(const char* gamerules, const char* mapLocation);
    virtual const char* GetGameRulesLevelLocation(const char* gamerules, int i);

    virtual void SetCurrentGameRules(IGameRules* pGameRules);
    virtual IGameRules* GetCurrentGameRules() const;
    const char* GetGameRulesName(const char* alias) const;

    void GetMemoryStatistics(ICrySizer* s);

private:
    SGameRulesDef* GetGameRulesDef(const char* name);
    static IComponentPtr CreateGameObject(
        IEntity* pEntity, SEntitySpawnParams& params, void* pUserData);

    IGameFramework* m_pGameFW;

    EntityId m_currentGameRules;
    IGameRules* m_pGameRules;

    TGameRulesMap m_GameRules;
};

#endif // CRYINCLUDE_CRYACTION_GAMERULESSYSTEM_H
