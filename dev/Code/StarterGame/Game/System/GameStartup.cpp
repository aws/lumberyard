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
#include "GameStartup.h"
#include "Core/StarterGameGame.h"
#include "Core/EditorGame.h"
#include "platform_impl.h"
#include <IPlayerProfiles.h>
#include <CryLibrary.h>
#include <ITimer.h>

#include <AzCore/std/string/osstring.h>
#include <AzCore/Component/ComponentApplicationBus.h>

using namespace LYGame;

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

extern "C"
{
AZ_DLL_EXPORT IGameStartup* CreateGameStartup()
{
    return GameStartup::Create();
}

AZ_DLL_EXPORT IEditorGame* CreateEditorGame()
{
    return new LYGame::EditorGame();
}
#if defined(AZ_MONOLITHIC_BUILD)
IGameFramework* CreateGameFramework();
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

GameStartup::GameStartup()
    : m_Framework(nullptr)
    , m_Game(nullptr)
{
}

GameStartup::~GameStartup()
{
    Shutdown();
}

IGameRef GameStartup::Init(SSystemInitParams& startupParams)
{
    IGameRef gameRef = nullptr;
    startupParams.pGameStartup = this;

    if (InitFramework(startupParams))
    {
        ISystem* system = m_Framework->GetISystem();
        startupParams.pSystem = system;

        // register system listeners
        system->GetISystemEventDispatcher()->RegisterListener(this);

        IEntitySystem* entitySystem = system->GetIEntitySystem();
        IComponentFactoryRegistry::RegisterAllComponentFactoryNodes(*entitySystem->GetComponentFactoryRegistry());

        gameRef = Reset();

        if (m_Framework->CompleteInit())
        {
            ISystemEventDispatcher* eventDispatcher = system->GetISystemEventDispatcher();
            eventDispatcher->OnSystemEvent(
                ESYSTEM_EVENT_RANDOM_SEED,
                static_cast<UINT_PTR>(gEnv->pTimer->GetAsyncTime().GetMicroSecondsAsInt64()),
                0
                );
        }
        else
        {
            gameRef->Shutdown();
            gameRef = nullptr;
        }
    }

    return gameRef;
}

IGameRef GameStartup::Reset()
{
    static char gameBuffer[sizeof(LYGame::StarterGameGame)];

    ISystem* system = m_Framework->GetISystem();
    ModuleInitISystem(system, "StarterGame");

    m_Game = new(reinterpret_cast<void*>(gameBuffer)) LYGame::StarterGameGame();
    const bool initialized = (m_Game && m_Game->Init(m_Framework));

    return initialized ? &m_Game : nullptr;
}

void GameStartup::Shutdown()
{
    if (m_Game)
    {
        m_Game->Shutdown();
        m_Game = nullptr;
    }

    ShutdownFramework();
}

bool GameStartup::InitFramework(SSystemInitParams& startupParams)
{
#if !defined(AZ_MONOLITHIC_BUILD)

    IGameFramework* gameFramework = nullptr;
    CryGameFrameworkBus::BroadcastResult(gameFramework, &CryGameFrameworkRequests::CreateFramework);
    AZ_Assert(gameFramework, "Legacy CreateGameFramework function called, but nothing is subscribed to the CryGameFrameworkRequests.\n"
        "Please use the Project Configurator to enable the CryLegacy gem for your project.");

    m_Framework = gameFramework;
#else
    m_Framework = CreateGameFramework();
#endif // !AZ_MONOLITHIC_BUILD


    if (!m_Framework)
    {
        return false;
    }

    // Initialize the engine.
    if (!m_Framework->Init(startupParams))
    {
        return false;
    }

    ISystem* system = m_Framework->GetISystem();
    ModuleInitISystem(system, "CryGame");

#ifdef WIN32
    if (gEnv->pRenderer)
    {
        SetWindowLongPtr(reinterpret_cast<HWND>(gEnv->pRenderer->
                                                    GetHWND()), GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    }
#endif

    return true;
}

void GameStartup::ShutdownFramework()
{
    if (m_Framework)
    {
        m_Framework->Shutdown();
        m_Framework = nullptr;
    }
}

void GameStartup::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
    switch (event)
    {
    case ESYSTEM_EVENT_RANDOM_SEED:
        cry_random_seed(gEnv->bNoRandomSeed ? 0 : (uint32)wparam);
        break;

    case ESYSTEM_EVENT_CHANGE_FOCUS:
        // 3.8.1 - disable / enable Sticky Keys!
        GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_RANDOM_SEED, (UINT_PTR)gEnv->pTimer->GetAsyncTime().GetMicroSecondsAsInt64(), 0);
        break;

    case ESYSTEM_EVENT_LEVEL_POST_UNLOAD:
        STLALLOCATOR_CLEANUP;
        break;

    case ESYSTEM_EVENT_LEVEL_LOAD_START:
    case ESYSTEM_EVENT_FAST_SHUTDOWN:
    default:
        break;
    }
}


//////////////////////////////////////////////////////////////////////////
#ifdef WIN32

bool GameStartup::HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
    switch (msg)
    {
    case WM_SYSCHAR: // Prevent ALT + key combinations from creating 'ding' sounds
    {
        const bool bAlt = (lParam & (1 << 29)) != 0;
        if (bAlt && wParam == VK_F4)
        {
            return false;     // Pass though ALT+F4
        }

        *pResult = 0;
        return true;
    }
    break;

    case WM_SIZE:
        break;

    case WM_SETFOCUS:
        // 3.8.1 - set a hasWindowFocus CVar to true
        break;

    case WM_KILLFOCUS:
        // 3.8.1 - set a hasWindowFocus CVar to false
        break;
    }
    return false;
}
#endif // WIN32


GameStartup* GameStartup::Create()
{
    static char buffer[sizeof(GameStartup)];
    return new(buffer)GameStartup();
}
