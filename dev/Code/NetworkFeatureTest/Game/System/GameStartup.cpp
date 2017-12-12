
#include "StdAfx.h"
#include "GameStartup.h"
#include "Core/NetworkFeatureTestGame.h"
#include "Core/EditorGame.h"
#include "platform_impl.h"
#include <IPlayerProfiles.h>
#include <CryLibrary.h>
#include <IPlatformOS.h>

#define DLL_INITFUNC_CREATEGAME "CreateGameFramework"

using namespace LYGame;

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

extern "C"
{
DLL_EXPORT IGameStartup* CreateGameStartup()
{
    return GameStartup::Create();
}

DLL_EXPORT IEditorGame* CreateEditorGame()
{
    return new LYGame::EditorGame();
}
#if defined(_LIB)
IGameFramework* CreateGameFramework();
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

GameStartup::GameStartup()
    : m_Framework(nullptr)
    , m_Game(nullptr)
    , m_FrameworkDll(nullptr)
    , m_bExecutedAutoExec(false)
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
        system->GetPlatformOS()->AddListener(this, "GameStartUp");

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

            if (startupParams.bExecuteCommandLine)
            {
                system->ExecuteCommandLine();
            }

            system->GetPlatformOS()->UserDoSignIn(0);
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
    static char gameBuffer[sizeof(LYGame::NetworkFeatureTestGame)];

    ISystem* system = m_Framework->GetISystem();
    ModuleInitISystem(system, "NetworkFeatureTest");

    m_Game = new(reinterpret_cast<void*>(gameBuffer)) LYGame::NetworkFeatureTestGame();
    const bool initialized = (m_Game && m_Game->Init(m_Framework));

    return initialized ? &m_Game : nullptr;
}

void GameStartup::Shutdown()
{
    if (m_Framework && m_Framework->GetISystem() && m_Framework->GetISystem()->GetPlatformOS())
    {
        m_Framework->GetISystem()->GetPlatformOS()->RemoveListener(this);
    }
    if (m_Game)
    {
        m_Game->Shutdown();
        m_Game = nullptr;
    }

    ShutdownFramework();
}

int GameStartup::Update(bool haveFocus, unsigned int updateFlags)
{
    return (m_Game ? m_Game->Update(haveFocus, updateFlags) : 0);
}

bool GameStartup::InitFramework(SSystemInitParams& startupParams)
{
#if !defined(_LIB)
    m_FrameworkDll = CryLoadLibrary(GAME_FRAMEWORK_FILENAME);

    if (!m_FrameworkDll)
    {
        return false;
    }

    IGameFramework::TEntryFunction CreateGameFramework = reinterpret_cast<IGameFramework::TEntryFunction>(CryGetProcAddress(m_FrameworkDll, DLL_INITFUNC_CREATEGAME));

    if (!CreateGameFramework)
    {
        return false;
    }
#endif // !_LIB

    m_Framework = CreateGameFramework();

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
#ifndef _LIB
        CryFreeLibrary(m_FrameworkDll);
#endif
        m_FrameworkDll = nullptr;
    }
}

void GameStartup::ExecuteAutoExec()
{
    if (m_bExecutedAutoExec)
    {
        return;
    }

    auto profileManager = GetISystem()->GetIGame()->GetIGameFramework()->GetIPlayerProfileManager();
    int userCount = profileManager->GetUserCount();
    if (userCount)
    {
        m_bExecutedAutoExec = true;
        gEnv->pConsole->ExecuteString("exec autoexec.cfg");
    }
}

int GameStartup::Run(const char* autoStartLevelName)
{
    ExecuteAutoExec();

#ifdef WIN32
    while (true)
    {
        ISystem* pSystem = gEnv ? gEnv->pSystem : nullptr;
        if (!pSystem)
        {
            break;
        }

        if (pSystem->PumpWindowMessage(false) == -1)
        {
            break;
        }

        if (!Update(true, 0))
        {
            // need to clean the message loop (WM_QUIT might cause problems in the case of a restart)
            // another message loop might have WM_QUIT already so we cannot rely only on this
            pSystem->PumpWindowMessage(true);
            break;
        }
    }
#else
    while (true)
    {
        if (!Update(true, 0))
        {
            break;
        }
    }
#endif // WIN32

    return 0;
}

void GameStartup::OnPlatformEvent(const IPlatformOS::SPlatformEvent& event)
{
    switch (event.m_eEventType)
    {
    case IPlatformOS::SPlatformEvent::eET_SignIn:
        ExecuteAutoExec();
        break;
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
