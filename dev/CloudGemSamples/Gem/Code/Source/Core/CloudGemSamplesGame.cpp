/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
*
*/

#include "StdAfx.h"
#include "Game/Actor.h"
#include "CloudGemSamplesGame.h"
#include "IGameFramework.h"
#include "IGameRulesSystem.h"
#include "CloudGemSamplesGameRules.h"
#include <functional>

using namespace LYGame;

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

#define REGISTER_FACTORY(host, name, impl, isAI) \
    (host)->RegisterFactory((name), (impl*)0, (isAI), (impl*)0)

CloudGemSamplesGame* LYGame::g_Game = nullptr;

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

CloudGemSamplesGame::CloudGemSamplesGame()
    : m_clientEntityId(INVALID_ENTITYID)
    , m_gameRules(nullptr)
    , m_gameFramework(nullptr)
    , m_defaultActionMap(nullptr)
{
    g_Game = this;
    GetISystem()->SetIGame(this);
}

CloudGemSamplesGame::~CloudGemSamplesGame()
{
    m_gameFramework->EndGameContext(false);

    // Remove self as listener.
    m_gameFramework->UnregisterListener(this);
    m_gameFramework->GetILevelSystem()->RemoveListener(this);
    gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(this);
    gEnv->pSystem->GetIInput()->RemoveEventListener(this);

    g_Game = nullptr;
    GetISystem()->SetIGame(nullptr);

    ReleaseActionMaps();
}

bool CloudGemSamplesGame::Init(IGameFramework* framework)
{
    m_gameFramework = framework;

    // Register the actor class so actors can spawn.
    // #TODO If you create a new actor, make sure to register a factory here.
    REGISTER_FACTORY(m_gameFramework, "Actor", Actor, false);

    // Listen to system events, so we know when levels load/unload, etc.
    gEnv->pSystem->GetISystemEventDispatcher()->RegisterListener(this);
    m_gameFramework->GetILevelSystem()->AddListener(this);

    // Listen for game framework events (level loaded/unloaded, etc.).
    m_gameFramework->RegisterListener(this, "Game", FRAMEWORKLISTENERPRIORITY_GAME);

    // Load actions maps.
    LoadActionMaps("config/input/actionmaps.xml");

    // Register game rules wrapper.
    REGISTER_FACTORY(framework, "CloudGemSamplesGameRules", CloudGemSamplesGameRules, false);
    IGameRulesSystem* pGameRulesSystem = g_Game->GetIGameFramework()->GetIGameRulesSystem();
    pGameRulesSystem->RegisterGameRules("DummyRules", "CloudGemSamplesGameRules");

    return true;
}

bool CloudGemSamplesGame::CompleteInit()
{
#ifdef CRY_UNIT_TESTING
    {
        CryUnitTest::UnitTestRunContext context;
        gEnv->pSystem->GetITestSystem()->GetIUnitTestManager()->RunMatchingTests("UnitTest_Game", context);
        CRY_ASSERT_MESSAGE(context.failedTestCount == 0, "Unit test failed");
    }
#endif

    return true;
}

int CloudGemSamplesGame::Update(bool hasFocus, unsigned int updateFlags)
{
    const float frameTime = gEnv->pTimer->GetFrameTime();

    const bool continueRunning = m_gameFramework->PreUpdate(true, updateFlags);
    m_gameFramework->PostUpdate(true, updateFlags);

    return static_cast<int>(continueRunning);
}

void CloudGemSamplesGame::PlayerIdSet(EntityId playerId)
{
    m_clientEntityId = playerId;
}

void CloudGemSamplesGame::Shutdown()
{
    this->~CloudGemSamplesGame();
}

void CloudGemSamplesGame::LoadActionMaps(const char* fileName)
{
    if (g_Game->GetIGameFramework()->IsGameStarted())
    {
        CryLogAlways("[Profile] Can't change configuration while game is running (yet)");
        return;
    }

    XmlNodeRef rootNode = m_gameFramework->GetISystem()->LoadXmlFromFile(fileName);
    if (rootNode && ReadProfile(rootNode))
    {
        IActionMapManager* actionMapManager = m_gameFramework->GetIActionMapManager();
        actionMapManager->SetLoadFromXMLPath(fileName);
        m_defaultActionMap = actionMapManager->GetActionMap("default");
    }
    else
    {
        CryLogAlways("[Profile] Warning: Could not open configuration file");
    }
}

void CloudGemSamplesGame::ReleaseActionMaps()
{
    if (m_defaultActionMap && m_gameFramework)
    {
        IActionMapManager* actionMapManager = m_gameFramework->GetIActionMapManager();
        actionMapManager->RemoveActionMap(m_defaultActionMap->GetName());
        m_defaultActionMap = nullptr;
    }
}

bool CloudGemSamplesGame::ReadProfile(const XmlNodeRef& rootNode)
{
    bool successful = false;

    if (IActionMapManager* actionMapManager = m_gameFramework->GetIActionMapManager())
    {
        actionMapManager->Clear();
        successful = actionMapManager->LoadFromXML(rootNode);
    }

    return successful;
}

void CloudGemSamplesGame::OnActionEvent(const SActionEvent& event)
{
    switch (event.m_event)
    {
    case eAE_unloadLevel:
        /*!
         * #TODO
         * Add clean up code here.
         */
        break;
    }
}

bool LYGame::CloudGemSamplesGame::OnInputEvent(const SInputEvent & inputEvent)
{

    if (inputEvent.keyId == eKI_C && inputEvent.state == eIS_Pressed)
    {
        return true;
    }

    return false;
}

#ifdef CRY_UNIT_TESTING

CRY_UNIT_TEST(UnitTest_Game)
{
}

#endif //CRY_UNIT_TESTING
