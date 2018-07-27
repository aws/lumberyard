
#include "StdAfx.h"
#include "GameStartup.h"
#include "Core/CloudGemSamplesGame.h"
#include "Core/EditorGame.h"
#include <IPlayerProfiles.h>
#include <CryLibrary.h>
#include <IPlatformOS.h>
#include <PakVars.h>

#define DLL_INITFUNC_CREATEGAME "CreateGameFramework"

using namespace LYGame;

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

extern "C"
{
}

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

GameStartup::GameStartup()
    : m_Framework(nullptr)
    , m_Game(nullptr)
    , m_bExecutedAutoExec(false)
{
}

GameStartup::~GameStartup()
{
    Shutdown();
}

IGameRef GameStartup::Init(SSystemInitParams& startupParams)
{
    IGameRef gameRef = NULL;
    startupParams.pGameStartup = this;

    CryGameFrameworkBus::BroadcastResult(m_Framework, &CryGameFrameworkRequests::InitFramework, startupParams);

    // CloudGemSamples downloads files to user storage - It is a requirement to be able to access them directly
    // through the file system.  In Release mode this is not allowed by default.  See PakVars.h : PakVars()
    //         nPriority  = ePakPriorityPakOnly; // Only read from pak files by default
    // We need to force this to PakFirst priority rather than PakOnly for CloudGemSamples.
    ICVar* pakPriority = gEnv->pConsole->GetCVar("sys_PakPriority");
    if (pakPriority && pakPriority->GetIVal() == ePakPriorityPakOnly) // ePakPriorityPakOnly
    {
        char pakPriorityBuf[5];
        azsprintf(pakPriorityBuf, "%d", ePakPriorityPakFirst);
        pakPriority->ForceSet(pakPriorityBuf);
    }

    if (m_Framework)
    {
        ISystem* system = m_Framework->GetISystem();
        startupParams.pSystem = system;

        // register system listeners
        system->GetISystemEventDispatcher()->RegisterListener(this);

        IComponentFactoryRegistry::RegisterAllComponentFactoryNodes(*system->GetIEntitySystem()->GetComponentFactoryRegistry());

        gameRef = Reset();

        if (m_Framework->CompleteInit())
        {
            GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_RANDOM_SEED, static_cast<UINT_PTR>(gEnv->pTimer->GetAsyncTime().GetMicroSecondsAsInt64()), 0);

            if (startupParams.bExecuteCommandLine)
            {
                system->ExecuteCommandLine();
            }
        }
        else
        {
            gameRef->Shutdown();
            gameRef = NULL;
        }
    }

    return gameRef;
}

IGameRef GameStartup::Reset()
{
    static char gameBuffer[sizeof(LYGame::CloudGemSamplesGame)];

    ISystem* system = m_Framework->GetISystem();
    ModuleInitISystem(system, "CloudGemSamples");

    m_Game = new(reinterpret_cast<void*>(gameBuffer)) LYGame::CloudGemSamplesGame();
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

void GameStartup::OnPlatformEvent(const IPlatformOS::SPlatformEvent& event)
{
    switch (event.m_eEventType)
    {
    case IPlatformOS::SPlatformEvent::eET_SignIn:
    {
        // We're not running autoExec here, rather we wait for GameStartup::Run 
        // Calling here will prevent proper loading of the level
    }
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
