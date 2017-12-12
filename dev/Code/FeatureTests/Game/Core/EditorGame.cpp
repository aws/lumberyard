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
#include "StdAfx.h"
#include "EditorGame.h"
#include "System/GameStartup.h"
#include "IGameRulesSystem.h"
#include "FeatureTestsGame.h"
#include "ILevelSystem.h"
#include "IActorSystem.h"
#include "EditorUIEnums.h"

#define EDITOR_SERVER_PORT 0xed17

extern "C"
{
DLL_EXPORT IGameStartup* CreateGameStartup();
};

using namespace LYGame;

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

EditorGame::EditorGame()
    : m_Game(NULL)
    , m_GameStartup(NULL)
    , m_InGame(false)
    , m_NetContextEnabled(false)
{
}

bool EditorGame::Init(ISystem* system, IGameToEditorInterface* gameToEditorInterface)
{
    bool successful = false;

    // Set up the startup params and create the Game instance.
    SSystemInitParams startupParams;
    startupParams.bEditor = true;
    startupParams.pSystem = system;
    startupParams.bExecuteCommandLine = false;

    m_GameStartup = CreateGameStartup();

    // Only proceed if the game startup is able to successfully create a Game instance.
    if (m_Game = m_GameStartup->Init(startupParams))
    {
        EditorUIEnums(m_Game, gameToEditorInterface);
        m_Game->GetIGameFramework()->InitEditor(gameToEditorInterface);
        gEnv->bServer = true;

#if !defined(CONSOLE)
        gEnv->SetIsClient(true);
#endif

        SetGameMode(false);
        ConfigureNetContext(eNetContextConfiguration_Enable);

        successful = true;
    }

    return successful;
}

int EditorGame::Update(bool hasFocus, unsigned int updateFlags)
{
    return m_GameStartup->Update(hasFocus, updateFlags);
}

void EditorGame::Shutdown()
{
    SetGameMode(false);
    ConfigureNetContext(eNetContextConfiguration_Disable);
    m_GameStartup->Shutdown();
}

void EditorGame::OnBeforeLevelLoad()
{
    ConfigureNetContext(eNetContextConfiguration_Reset);

    const char* levelName = nullptr;
    ICVar* sv_map = gEnv->pConsole->GetCVar("sv_map");
    if (sv_map)
    {
        levelName = sv_map->GetString();
    }

    ILevelSystem* levelSystem = m_Game->GetIGameFramework()->GetILevelSystem();
    CRY_ASSERT(levelSystem);
    ILevelInfo* levelInfo = levelSystem->GetLevelInfo(levelName);
    levelSystem->PrepareNextLevel(levelName);
    levelSystem->OnLoadingStart(levelInfo);

    const char* gameRulesName = "DummyRules";
    m_Game->GetIGameFramework()->GetIGameRulesSystem()->CreateGameRules(gameRulesName);
}

void EditorGame::OnAfterLevelLoad(const char* levelName, const char* levelFolder)
{
    ILevel* level = m_Game->GetIGameFramework()->GetILevelSystem()->SetEditorLoadedLevel(levelName);

    m_Game->GetIGameFramework()->GetILevelSystem()->OnLoadingComplete(level);

    SEntityEvent startLevelEvent(ENTITY_EVENT_START_LEVEL);
    gEnv->pEntitySystem->SendEventToAll(startLevelEvent);

    m_Game->GetIGameFramework()->MarkGameStarted();
}

IFlowSystem* EditorGame::GetIFlowSystem()
{
    return m_Game->GetIGameFramework()->GetIFlowSystem();
}

IGameTokenSystem* EditorGame::GetIGameTokenSystem()
{
    return m_Game->GetIGameFramework()->GetIGameTokenSystem();
}

IEntity* EditorGame::GetPlayer()
{
    IEntity* entity = NULL;

    if (IActor* actor = m_Game->GetIGameFramework()->GetClientActor())
    {
        entity = actor->GetEntity();
    }

    return entity;
}

void EditorGame::SetPlayerPosAng(Vec3 position, Vec3 viewDirection)
{
    if (IActor* actor = m_Game->GetIGameFramework()->GetClientActor())
    {
        IEntity* entity = actor->GetEntity();

        entity->SetPos(position);

        if (entity->GetPhysics())
        {
            pe_params_pos physicsParams;
            physicsParams.pos = position;
            entity->GetPhysics()->SetParams(&physicsParams);
        }
    }
}

void EditorGame::HidePlayer(bool hide)
{
    if (IActor* actor = m_Game->GetIGameFramework()->GetClientActor())
    {
        actor->GetEntity()->Hide(hide);
    }
}

bool EditorGame::ConfigureNetContext(NetContextConfiguration configuration)
{
    bool successful = true;

    switch (configuration)
    {
    case eNetContextConfiguration_Enable:
    {
        if (!m_NetContextEnabled)
        {
            SGameContextParams gameContextParams;
            SGameStartParams gameParams;

            gameParams.connectionString = "";
            gameParams.hostname = "localhost";
            gameParams.pContextParams = &gameContextParams;
            gameParams.maxPlayers = 1;
            gameParams.flags =
                eGSF_Server | eGSF_NoSpawnPlayer | eGSF_Client | eGSF_NoLevelLoading |
                eGSF_BlockingClientConnect | eGSF_NoGameRules | eGSF_NoQueries | eGSF_LocalOnly;

            successful = m_Game->GetIGameFramework()->StartGameContext(&gameParams);
            m_NetContextEnabled = successful;
        }
    }
    break;

    case eNetContextConfiguration_Disable:
    {
        m_Game->GetIGameFramework()->EndGameContext(true);
        gEnv->pNetwork->SyncWithGame(eNGS_Shutdown);
        m_NetContextEnabled = false;
    }
    break;

    case eNetContextConfiguration_Reset:
    {
        ConfigureNetContext(eNetContextConfiguration_Disable);
        successful = ConfigureNetContext(eNetContextConfiguration_Enable);
    }
    break;
    }

    return successful;
}

bool EditorGame::SetGameMode(bool isInGame)
{
    m_InGame = isInGame;
    gEnv->pGame->GetIGameFramework()->OnEditorSetGameMode(m_InGame);
    return true;
}
