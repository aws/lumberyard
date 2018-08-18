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

#ifndef CRYINCLUDE_CRYACTION_NETWORK_GAMECONTEXT_H
#define CRYINCLUDE_CRYACTION_NETWORK_GAMECONTEXT_H
#pragma once

#include "IGameFramework.h"
#include "IEntitySystem.h"
#include "IScriptSystem.h"
#include "ClassRegistryReplicator.h"
#include "INetworkService.h"

// FIXME: Cell SDK GCC bug workaround.
#ifndef CRYINCLUDE_GAMEOBJECT_H
#include "GameObjects/GameObject.h"
#endif

class CScriptRMI;
class CActionGame;
struct SBreakEvent;

class CCryAction;
class CGameContext
    : public IGameContext
    , public IEntitySystemSink
    , public IConsoleVarSink
{
public:
    CGameContext(CCryAction* pFramework, CScriptRMI*, CActionGame* pGame);
    ~CGameContext() override;

    void Init(const SGameStartParams* params);

    // IGameContext
    void SetGameStarted(bool started) override { m_bStarted = started; }
    const SGameStartParams& GetAppliedParams() const override { return m_appliedParams; }
    void SetContextFlags(unsigned flags) override;
    uint32 GetContextFlags() override { return m_flags; }
    // ~IGameContext

    // IGameQuery
    XmlNodeRef GetGameState();
    // ~IGameQuery

    string GetLevelName() { return m_appliedParams.pContextParams ? m_appliedParams.pContextParams->levelName : ""; }
    string GetRequestedGameRules() { return m_appliedParams.pContextParams ? m_appliedParams.pContextParams->gameRules : ""; }

    // IEntitySystemSink
    bool OnBeforeSpawn(SEntitySpawnParams& params) override;
    void OnSpawn(IEntity* pEntity, SEntitySpawnParams& params) override;
    bool OnRemove(IEntity* pEntity) override;
    void OnReused(IEntity* pEntity, SEntitySpawnParams& params) override;
    void OnEvent(IEntity* pEntity, SEntityEvent& event) override;
    // ~IEntitySystemSink

    // IConsoleVarSink
    bool OnBeforeVarChange(ICVar* pVar, const char* sNewValue) override;
    void OnAfterVarChange(ICVar* pVar) override;
    // ~IConsoleVarSink

    CCryAction* GetFramework() { return m_pFramework; }
    bool IsGameStarted() const { return m_bStarted; }
    bool IsLoadingSaveGame() const { return m_bIsLoadingSaveGame; };

    void SetLoadingSaveGame() { m_bIsLoadingSaveGame = true; }

    void OnBrokeSomething(const SBreakEvent& be, EventPhysMono* pMono, bool isPlane);

    void ResetMap(bool isServer);

    // -- registers a class in our class name <-> id database
    bool RegisterClassName(const string& name, uint16 id);
    uint32 GetClassesHash();
    void DumpClasses();

    bool Update();

    bool ClassNameFromId(string& name, uint16 id) const;
    bool ClassIdFromName(uint16& id, const string& name) const;

    bool ChangeContext(bool isServer, const SGameContextParams* pParams);

    void SetContextInfo(unsigned flags, const char* connectionString);
    bool HasContextFlag(EGameStartFlags flag) const { return (m_flags & flag) != 0; }

    void PlayerIdSet(EntityId id);

    int GetPlayerCount() override;

    void GetMemoryUsage(ICrySizer* pSizer) const;
    void GetMemoryStatistics(ICrySizer* pSizer)  {GetMemoryUsage(pSizer); /*dummy till network module is updated*/}

private:
    void BackupParams(const SGameStartParams* params);
    void BackupParams(const SGameContextParams* params);

    struct BackedUpGameStartParams
        : public SGameStartParams
    {
        BackedUpGameStartParams()
            : SGameStartParams() {}

        void Backup(const SGameStartParams& params)
        {
            flags = params.flags;
            maxPlayers = params.maxPlayers;

            m_hostnameStr = params.hostname;
            m_connectionStr = params.connectionString;

            hostname = m_hostnameStr.c_str();
            connectionString = m_connectionStr.c_str();
        }

        string m_hostnameStr;
        string m_connectionStr;
    };

    struct BackedUpGameContextParams
        : public SGameContextParams
    {
        BackedUpGameContextParams()
            : SGameContextParams() {}

        void Backup(const SGameContextParams& params)
        {
            demoRecorderFilename = params.demoPlaybackFilename;
            demoPlaybackFilename = params.demoPlaybackFilename;
            flags = params.flags;

            m_levelNameStr = params.levelName;
            m_gameRulesStr = params.gameRules;

            levelName = m_levelNameStr.c_str();
            gameRules = m_gameRulesStr.c_str();
        }

        string m_levelNameStr;
        string m_gameRulesStr;
    };

    BackedUpGameStartParams m_appliedParams;
    BackedUpGameContextParams m_appliedContextParams;

    static IEntity* GetEntity(int i, void* p)
    {
        if (i == PHYS_FOREIGN_ID_ENTITY)
        {
            return (IEntity*)p;
        }
        return NULL;
    }

    IGameRules* GetGameRules();

    void CallOnSpawnComplete(IEntity* pEntity);
    void BeginChangeContext(const string& levelName);

    static void OnCollision(const EventPhys* pEvent, void*);

    // singleton
    static CGameContext* s_pGameContext;

    IEntitySystem* m_pEntitySystem;
    IPhysicalWorld* m_pPhysicalWorld;
    IActorSystem* m_pActorSystem;
    CCryAction* m_pFramework;
    CActionGame* m_pGame;
    CScriptRMI* m_pScriptRMI;

    CClassRegistryReplicator m_classRegistry;

    // context parameters
    string m_levelName;
    string m_gameRules;

    unsigned m_flags;
    string m_connectionString;
    bool m_bStarted;
    bool m_bIsLoadingSaveGame;
    bool m_bHasSpawnPoint;
    int m_broadcastActionEventInGame;
};

#endif // CRYINCLUDE_CRYACTION_NETWORK_GAMECONTEXT_H

