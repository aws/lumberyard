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
#include "Game/Actor.h"
#include "Game/FlyCameraInputComponent.h"
#include "FeatureTestsGame.h"
#include "IGameFramework.h"
#include "IGameRulesSystem.h"
#include "FeatureTestsGameRules.h"
#include "GameCache.h"
#include "ScriptBind_Game.h"
#include <ILocalizationManager.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <functional>
#include <FlowSystem/Nodes/FlowBaseNode.h>

using namespace LYGame;

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

#define REGISTER_FACTORY(host, name, impl, isAI) \
    (host)->RegisterFactory((name), (impl*)0, (isAI), (impl*)0)

FeatureTestsGame* LYGame::g_Game = nullptr;

namespace
{
    void RegisterComponents()
    {
        EBUS_EVENT(AZ::ComponentApplicationBus, RegisterComponentDescriptor, LYGame::FlyCameraInputComponent::CreateDescriptor());
    }

    void UnregisterComponents()
    {
        EBUS_EVENT_ID(AZ::AzTypeInfo<LYGame::FlyCameraInputComponent>::Uuid(), AZ::ComponentDescriptorBus, ReleaseDescriptor);
    }

    void loadLocalizationData()
    {
        ILocalizationManager* pLocMan = GetISystem()->GetLocalizationManager();

        string localizationXml("libs/localization/localization.xml");

        bool initLocSuccess = false;

        if (pLocMan)
        {
            if (pLocMan->InitLocalizationData(localizationXml.c_str()))
            {
                if (pLocMan->LoadLocalizationDataByTag("init"))
                {
                    initLocSuccess = true;
                }
            }
        }

        if (!initLocSuccess)
        {
            CryLog("loadLocalizationData() failed to load localization file=%s", localizationXml.c_str());
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

FeatureTestsGame::FeatureTestsGame()
    : m_clientEntityId(INVALID_ENTITYID)
    , m_gameRules(nullptr)
    , m_gameFramework(nullptr)
    , m_defaultActionMap(nullptr)
    , m_scriptBind(nullptr)
    , m_gameCache(nullptr)
    , m_platformInfo()
{
    g_Game = this;
    GetISystem()->SetIGame(this);
}

FeatureTestsGame::~FeatureTestsGame()
{
    m_gameFramework->EndGameContext(false);

    // Remove self as listener.
    m_gameFramework->UnregisterListener(this);
    m_gameFramework->GetILevelSystem()->RemoveListener(this);
    gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(this);

    SAFE_DELETE(m_scriptBind);
    SAFE_DELETE(m_gameCache);

    g_Game = nullptr;
    GetISystem()->SetIGame(nullptr);

    ReleaseActionMaps();
}

namespace LYGame
{
    void GameInit();
    void GameShutdown();
}

bool FeatureTestsGame::Init(IGameFramework* framework)
{
    m_gameFramework = framework;

    // Register the actor class so actors can spawn.
    // #TODO If you create a new actor, make sure to register a factory here.
    REGISTER_FACTORY(m_gameFramework, "Actor", Actor, false);

    // Listen to system events, so we know when levels load/unload, etc.
    gEnv->pSystem->GetISystemEventDispatcher()->RegisterListener(this);
    m_gameFramework->GetILevelSystem()->AddListener(this);

    loadLocalizationData();

    // Listen for game framework events (level loaded/unloaded, etc.).
    m_gameFramework->RegisterListener(this, "Game", FRAMEWORKLISTENERPRIORITY_GAME);

    // Load actions maps.
    LoadActionMaps("config/input/actionmaps.xml");

    // Initialize script binding.
    m_scriptBind = new ScriptBind_Game(m_gameFramework);

    // Register game rules wrapper.
    REGISTER_FACTORY(framework, "FeatureTestsGameRules", FeatureTestsGameRules, false);
    IGameRulesSystem* pGameRulesSystem = g_Game->GetIGameFramework()->GetIGameRulesSystem();
    pGameRulesSystem->RegisterGameRules("DummyRules", "FeatureTestsGameRules");

    GameInit();

    m_gameCache = new GameCache;
    m_gameCache->Init();

    return true;
}

bool FeatureTestsGame::CompleteInit()
{
    return true;
}

int FeatureTestsGame::Update(bool hasFocus, unsigned int updateFlags)
{
    const float frameTime = gEnv->pTimer->GetFrameTime();

    const bool continueRunning = m_gameFramework->PreUpdate(true, updateFlags);
    m_gameFramework->PostUpdate(true, updateFlags);

    return static_cast<int>(continueRunning);
}

void FeatureTestsGame::PlayerIdSet(EntityId playerId)
{
    m_clientEntityId = playerId;
}

void FeatureTestsGame::Shutdown()
{
    GameShutdown();

    this->~FeatureTestsGame();
}

void FeatureTestsGame::LoadActionMaps(const char* fileName)
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

void FeatureTestsGame::ReleaseActionMaps()
{
    if (m_defaultActionMap && m_gameFramework)
    {
        IActionMapManager* actionMapManager = m_gameFramework->GetIActionMapManager();
        actionMapManager->RemoveActionMap(m_defaultActionMap->GetName());
        m_defaultActionMap = nullptr;
    }
}

bool FeatureTestsGame::ReadProfile(const XmlNodeRef& rootNode)
{
    bool successful = false;

    if (IActionMapManager* actionMapManager = m_gameFramework->GetIActionMapManager())
    {
        actionMapManager->Clear();

        // Load platform information in.
        XmlNodeRef platforms = rootNode->findChild("platforms");
        if (!platforms || !ReadProfilePlatform(platforms, GetPlatform()))
        {
            CryLogAlways("[Profile] Warning: No platform information specified!");
        }

        successful = actionMapManager->LoadFromXML(rootNode);
    }

    return successful;
}

bool FeatureTestsGame::ReadProfilePlatform(const XmlNodeRef& platformsNode, Platform platformId)
{
    bool successful = false;

    if (platformsNode && (platformId > ePlatform_Unknown) && (platformId < ePlatform_Count))
    {
        if (XmlNodeRef platform = platformsNode->findChild(s_PlatformNames[platformId]))
        {
            // Extract which Devices we want.
            if (!strcmp(platform->getAttr("keyboard"), "0"))
            {
                m_platformInfo.m_devices &= ~eAID_KeyboardMouse;
            }

            if (!strcmp(platform->getAttr("xboxpad"), "0"))
            {
                m_platformInfo.m_devices &= ~eAID_XboxPad;
            }

            if (!strcmp(platform->getAttr("ps4pad"), "0"))
            {
                m_platformInfo.m_devices &= ~eAID_PS4Pad;
            }

            if (!strcmp(platform->getAttr("androidkey"), "0"))
            {
                m_platformInfo.m_devices &= ~eAID_AndroidKey;
            }

            // Map the Devices we want.
            IActionMapManager* actionMapManager = m_gameFramework->GetIActionMapManager();

            if (m_platformInfo.m_devices & eAID_KeyboardMouse)
            {
                actionMapManager->AddInputDeviceMapping(eAID_KeyboardMouse, "keyboard");
            }

            if (m_platformInfo.m_devices & eAID_XboxPad)
            {
                actionMapManager->AddInputDeviceMapping(eAID_XboxPad, "xboxpad");
            }

            if (m_platformInfo.m_devices & eAID_PS4Pad)
            {
                actionMapManager->AddInputDeviceMapping(eAID_PS4Pad, "ps4pad");
            }

            if (m_platformInfo.m_devices & eAID_AndroidKey)
            {
                actionMapManager->AddInputDeviceMapping(eAID_AndroidKey, "androidkey");
            }

            successful = true;
        }
        else
        {
            GameWarning("FeatureTestsGame::ReadProfilePlatform: Failed to find platform, action mappings loading will fail");
        }
    }

    return successful;
}

Platform FeatureTestsGame::GetPlatform() const
{
    Platform platform = ePlatform_Unknown;

#if defined(ANDROID)
    platform = ePlatform_Android;
#elif defined(IOS)
    platform = ePlatform_iOS;
#elif defined(WIN32) || defined(WIN64) || defined(DURANGO) || defined(APPLE) || defined(LINUX)
    platform = ePlatform_PC;
#endif

    return platform;
}

void FeatureTestsGame::OnActionEvent(const SActionEvent& event)
{
    switch (event.m_event)
    {
    case eAE_unloadLevel:
        if (m_gameCache)
        {
            m_gameCache->Reset();
        }
        break;
    }
}

void FeatureTestsGame::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
    switch (event)
    {
    case ESYSTEM_EVENT_GAME_POST_INIT:
    {
        RegisterComponents();
    }
    break;

    case ESYSTEM_EVENT_FULL_SHUTDOWN:
    case ESYSTEM_EVENT_FAST_SHUTDOWN:
    {
        UnregisterComponents();
    }
    break;

    case ESYSTEM_EVENT_FLOW_SYSTEM_REGISTER_EXTERNAL_NODES:
    {
        RegisterExternalFlowNodes();
    }
    break;

    default:
        break;
    }
}
