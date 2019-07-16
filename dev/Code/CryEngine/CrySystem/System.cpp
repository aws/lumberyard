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

// Description : CryENGINE system core-handle all subsystems


#include "StdAfx.h"
#include "System.h"
#include <time.h>
#include <AzCore/IO/SystemFile.h>
#include "CryLibrary.h"
#include "IGemManager.h"
#include "ICrypto.h"
#include <CrySystemBus.h>
#include <ITimeDemoRecorder.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/API/ApplicationAPI_win.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>


#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define SYSTEM_CPP_SECTION_1 1
#define SYSTEM_CPP_SECTION_2 2
#define SYSTEM_CPP_SECTION_3 3
#define SYSTEM_CPP_SECTION_4 4
#define SYSTEM_CPP_SECTION_5 5
#define SYSTEM_CPP_SECTION_6 6
#define SYSTEM_CPP_SECTION_7 7
#endif

#if defined(_RELEASE) && AZ_LEGACY_CRYSYSTEM_TRAIT_USE_EXCLUDEUPDATE_ON_CONSOLE
//exclude some not needed functionality for release console builds
#define EXCLUDE_UPDATE_ON_CONSOLE
#endif

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
// If app hasn't chosen, set to work with Windows 98, Windows Me, Windows 2000, Windows XP and beyond
#include <windows.h>

static LRESULT WINAPI WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CSystem* pSystem = 0;
    if (gEnv)
    {
        pSystem = static_cast<CSystem*>(gEnv->pSystem);
    }
    if (pSystem && !pSystem->m_bQuit)
    {
        LRESULT result;
        bool bAny = false;
        for (std::vector<IWindowMessageHandler*>::const_iterator it = pSystem->m_windowMessageHandlers.begin(); it != pSystem->m_windowMessageHandlers.end(); ++it)
        {
            IWindowMessageHandler* pHandler = *it;
            LRESULT maybeResult = 0xDEADDEAD;
            if (pHandler->HandleMessage(hWnd, uMsg, wParam, lParam, &maybeResult))
            {
                assert(maybeResult != 0xDEADDEAD && "Message handler indicated a resulting value, but no value was written");
                if (bAny)
                {
                    assert(result == maybeResult && "Two window message handlers tried to return different result values");
                }
                else
                {
                    bAny = true;
                    result = maybeResult;
                }
            }
        }
        if (bAny)
        {
            // One of the registered handlers returned something
            return result;
        }
    }

    // Handle with the default procedure
#if defined(UNICODE) || defined(_UNICODE)
    assert(IsWindowUnicode(hWnd) && "Window should be Unicode when compiling with UNICODE");
#else
    if (!IsWindowUnicode(hWnd))
    {
        return DefWindowProcA(hWnd, uMsg, wParam, lParam);
    }
#endif
    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}
#endif

#if defined(LINUX) && !defined(ANDROID)
#include <execinfo.h> // for backtrace
#endif

#if defined(ANDROID)
#include <unwind.h>  // for _Unwind_Backtrace and _Unwind_GetIP
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEM_CPP_SECTION_1
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/System_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/System_cpp_provo.inl"
    #endif
#endif


#include <INetwork.h>
#include <I3DEngine.h>
#include <IAISystem.h>
#include <IRenderer.h>
#include <ICryPak.h>
#include <IMovieSystem.h>
#include <ServiceNetwork.h>
#include <IEntitySystem.h>
#include <IInput.h>
#include <ILog.h>
#include <IAudioSystem.h>
#include "NullImplementation/NULLAudioSystems.h"
#include <ICryAnimation.h>
#include <IScriptSystem.h>
#include <IProcess.h>
#include <IBudgetingSystem.h>
#include <IGame.h>
#include <IGameFramework.h>
#include <INotificationNetwork.h>
#include <ICodeCheckpointMgr.h>
#include <ISoftCodeMgr.h>
#include "TestSystemLegacy.h"       // CTestSystem
#include "VisRegTest.h"
#include <LyShine/ILyShine.h>
#include <IDynamicResponseSystem.h>
#include <ITimeOfDay.h>

#include <LoadScreenBus.h>

#include "CryPak.h"
#include "XConsole.h"
#include "Telemetry/TelemetrySystem.h"
#include "Telemetry/TelemetryFileStream.h"
#include "Telemetry/TelemetryUDPStream.h"
#include "Log.h"
#include "CrySizerStats.h"
#include "CrySizerImpl.h"
#include "NotificationNetwork.h"
#include "ProfileLog.h"

#include "XML/xml.h"
#include "XML/ReadWriteXMLSink.h"

#include "StreamEngine/StreamEngine.h"
#include "PhysRenderer.h"

#include "LocalizedStringManager.h"
#include "XML/XmlUtils.h"
#include "Serialization/ArchiveHost.h"
#include "ThreadProfiler.h"
#include "IDiskProfiler.h"
#include "SystemEventDispatcher.h"
#include "IHardwareMouse.h"
#include "ServerThrottle.h"
#include "ILocalMemoryUsage.h"
#include "ResourceManager.h"
#include "LoadingProfiler.h"
#include "HMDBus.h"
#include "OverloadSceneManager/OverloadSceneManager.h"
#include <IThreadManager.h>

#include "IZLibCompressor.h"
#include "IZlibDecompressor.h"
#include "ILZ4Decompressor.h"
#include "zlib.h"
#include "RemoteConsole/RemoteConsole.h"
#include "BootProfiler.h"
#include "NullImplementation/NULLAudioSystems.h"

#include <PNoise3.h>
#include <StringUtils.h>
#include "CryWaterMark.h"
WATERMARKDATA(_m);

#include "ImageHandler.h"
#include <LyShine/Bus/UiCursorBus.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/Input/Buses/Requests/InputSystemRequestBus.h>

#include <ICrypto.h>

#ifdef WIN32

#include <AzFramework/Input/Buses/Notifications/RawInputNotificationBus_win.h>

// To enable profiling with vtune (https://software.intel.com/en-us/intel-vtune-amplifier-xe), make sure the line below is not commented out
//#define  PROFILE_WITH_VTUNE

#include <process.h>
#include <malloc.h>
#endif

#if USE_STEAM
#include "Steamworks/public/steam/steam_api.h"
#endif

#include <CryProfileMarker.h>

#include <ILevelSystem.h>

#include <CrtDebugStats.h>
#include <AzFramework/IO/LocalFileIO.h>

// profilers api.
VTuneFunction VTResume = NULL;
VTuneFunction VTPause = NULL;

// Define global cvars.
SSystemCVars g_cvars;

#include "ITextModeConsole.h"

extern int CryMemoryGetAllocatedSize();

// these heaps are used by underlying System structures
// to allocate, accordingly, small (like elements of std::set<..*>) and big (like memory for reading files) objects
// hopefully someday we'll have standard MT-safe heap
//CMTSafeHeap g_pakHeap;
CMTSafeHeap* g_pPakHeap = 0;// = &g_pakHeap;

//////////////////////////////////////////////////////////////////////////
#include "Validator.h"

#include <IViewSystem.h>

#include <AzCore/Module/Environment.h>
#include <AzCore/Component/ComponentApplication.h>
#include "AZCoreLogSink.h"

#if defined(ANDROID)
namespace
{
    struct Callstack
    {
        Callstack()
            : addrs(NULL)
            , ignore(0)
            , count(0)
        {
        }
        Callstack(void** addrs, size_t ignore, size_t count)
        {
            this->addrs = addrs;
            this->ignore = ignore;
            this->count = count;
        }
        void** addrs;
        size_t ignore;
        size_t count;
    };

    static _Unwind_Reason_Code trace_func(struct _Unwind_Context* context, void* arg)
    {
        Callstack* cs = static_cast<Callstack*>(arg);
        if (cs->count)
        {
            void* ip = (void*) _Unwind_GetIP(context);
            if (ip)
            {
                if (cs->ignore)
                {
                    cs->ignore--;
                }
                else
                {
                    cs->addrs[0] = ip;
                    cs->addrs++;
                    cs->count--;
                }
            }
        }
        return _URC_NO_REASON;
    }

    static int Backtrace(void** addrs, size_t ignore, size_t size)
    {
        Callstack cs(addrs, ignore, size);
        _Unwind_Backtrace(trace_func, (void*) &cs);
        return size - cs.count;
    }
}
#endif

#if defined(CVARS_WHITELIST)
struct SCVarsWhitelistConfigSink
    : public ILoadConfigurationEntrySink
{
    virtual void OnLoadConfigurationEntry(const char* szKey, const char* szValue, const char* szGroup)
    {
        ICVarsWhitelist* pCVarsWhitelist = gEnv->pSystem->GetCVarsWhiteList();
        bool whitelisted = (pCVarsWhitelist) ? pCVarsWhitelist->IsWhiteListed(szKey, false) : true;
        if (whitelisted)
        {
            gEnv->pConsole->LoadConfigVar(szKey, szValue);
        }
    }
} g_CVarsWhitelistConfigSink;
#endif // defined(CVARS_WHITELIST)

#if defined(AZ_MONOLITHIC_BUILD)
SC_API struct SSystemGlobalEnvironment* gEnv = NULL;
ComponentFactoryCreationNode* ComponentFactoryCreationNode::sm_head = nullptr;
size_t ComponentFactoryCreationNode::sm_size = 0;
#endif // AZ_MONOLITHIC_BUILD

/////////////////////////////////////////////////////////////////////////////////
// System Implementation.
//////////////////////////////////////////////////////////////////////////
CSystem::CSystem(SharedEnvironmentInstance* pSharedEnvironment)
    : m_imageHandler(std::make_unique<ImageHandler>())
{
    CrySystemRequestBus::Handler::BusConnect();

    if (!pSharedEnvironment)
    {
        CryFatalError("No shared environment instance provided. "
            "Cross-module sharing of EBuses and allocators "
            "is not possible.");
    }

    m_systemGlobalState = ESYSTEM_GLOBAL_STATE_UNKNOWN;
    m_iHeight = 0;
    m_iWidth = 0;
    m_iColorBits = 0;
    // CRT ALLOCATION threshold

    m_bIsAsserting = false;
    m_pSystemEventDispatcher = new CSystemEventDispatcher(); // Must be first.

#if defined(ENABLE_LOADING_PROFILER)
    CBootProfiler::GetInstance().Init(this);
#endif

    if (m_pSystemEventDispatcher)
    {
        m_pSystemEventDispatcher->RegisterListener(this);
    }

#ifdef WIN32
    m_hInst = NULL;
    m_hWnd = NULL;
#if _MSC_VER < 1000
    int sbh = _set_sbh_threshold(1016);
#endif
#endif

    //////////////////////////////////////////////////////////////////////////
    // Clear environment.
    //////////////////////////////////////////////////////////////////////////
    memset(&m_env, 0, sizeof(m_env));

    //////////////////////////////////////////////////////////////////////////
    // Initialize global environment interface pointers.
    m_env.pSystem = this;
    m_env.pTimer = &m_Time;
    m_env.pNameTable = &m_nameTable;
    m_env.pFrameProfileSystem = &m_FrameProfileSystem;
    m_env.bServer = false;
    m_env.bMultiplayer = false;
    m_env.bHostMigrating = false;
    m_env.bProfilerEnabled = false;
    m_env.callbackStartSection = 0;
    m_env.callbackEndSection = 0;
    m_env.bIgnoreAllAsserts = false;
    m_env.bNoAssertDialog = false;
    m_env.bTesting = false;

    m_env.pSharedEnvironment = pSharedEnvironment;

    m_env.SetFMVIsPlaying(false);
    m_env.SetCutsceneIsPlaying(false);

    m_env.szDebugStatus[0] = '\0';

#if !defined(CONSOLE)
    m_env.SetIsClient(false);
#endif
    //////////////////////////////////////////////////////////////////////////

    m_pStreamEngine = NULL;
    m_PhysThread = 0;

    m_pTelemetryFileStream = NULL;
    m_pTelemetryUDPStream = NULL;

    m_pIFont = NULL;
    m_pIFontUi = NULL;
    m_pTestSystem = NULL;
    m_pVisRegTest = NULL;
    m_rWidth = NULL;
    m_rHeight = NULL;
    m_rWidthAndHeightAsFractionOfScreenSize = NULL;
    m_rMaxWidth = NULL;
    m_rMaxHeight = NULL;
    m_rColorBits = NULL;
    m_rDepthBits = NULL;
    m_cvSSInfo = NULL;
    m_rStencilBits = NULL;
    m_rFullscreen = NULL;
    m_rDriver = NULL;
    m_sysNoUpdate = NULL;
    m_pMemoryManager = NULL;
    m_pProcess = NULL;

    m_pValidator = NULL;
    m_pCmdLine = NULL;
    m_pDefaultValidator = NULL;
    m_pLevelSystem = NULL;
    m_pViewSystem = NULL;
    m_pIBudgetingSystem = NULL;
    m_pIZLibCompressor = NULL;
    m_pIZLibDecompressor = NULL;
    m_pILZ4Decompressor = NULL;
    m_pLocalizationManager = NULL;
    m_pGemManager = nullptr;
    m_crypto = nullptr;
    m_sys_physics_CPU = 0;
    m_sys_skip_input = nullptr;
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEM_CPP_SECTION_2
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/System_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/System_cpp_provo.inl"
    #endif
#endif
    m_sys_min_step = 0;
    m_sys_max_step = 0;

    m_pNotificationNetwork = NULL;

    m_cvAIUpdate = NULL;

    m_pUserCallback = NULL;
#if defined(CVARS_WHITELIST)
    m_pCVarsWhitelist = NULL;
    m_pCVarsWhitelistConfigSink = &g_CVarsWhitelistConfigSink;
#endif // defined(CVARS_WHITELIST)
    m_sys_memory_debug = NULL;
    m_sysWarnings = NULL;
    m_sysKeyboard = NULL;
    m_sys_profile = NULL;
    m_sys_profile_additionalsub = NULL;
    m_sys_profile_graphScale = NULL;
    m_sys_profile_pagefaultsgraph = NULL;
    m_sys_profile_graph = NULL;
    m_sys_profile_filter = NULL;
    m_sys_profile_filter_thread = NULL;
    m_sys_profile_allThreads = NULL;
    m_sys_profile_network = NULL;
    m_sys_profile_peak = NULL;
    m_sys_profile_peak_time = NULL;
    m_sys_profile_memory = NULL;
    m_sys_profile_sampler = NULL;
    m_sys_profile_sampler_max_samples = NULL;
    m_sys_GraphicsQuality = NULL;
    m_sys_firstlaunch = NULL;
    m_sys_enable_budgetmonitoring = NULL;
    m_sys_preload = NULL;

    //  m_sys_filecache = NULL;
    m_gpu_particle_physics = NULL;
    m_pCpu = NULL;
    m_sys_game_folder = NULL;

    m_bQuit = false;
    m_bInitializedSuccessfully = false;
    m_bShaderCacheGenMode = false;
    m_bRelaunch = false;
    m_iLoadingMode = 0;
    m_bTestMode = false;
    m_bEditor = false;
    m_bPreviewMode = false;
    m_bIgnoreUpdates = false;
    m_bNoCrashDialog = false;

#ifndef _RELEASE
    m_checkpointLoadCount = 0;
    m_loadOrigin = eLLO_Unknown;
    m_hasJustResumed = false;
    m_expectingMapCommand = false;
#endif

    // no mem stats at the moment
    m_pMemStats = NULL;
    m_pSizer = NULL;
    m_pCVarQuit = NULL;

    m_pDownloadManager = 0;
    m_bForceNonDevMode = false;
    m_bWasInDevMode = false;
    m_bInDevMode = false;
    m_bGameFolderWritable = false;

    m_bDrawConsole = true;
    m_bDrawUI = true;

    m_nServerConfigSpec = CONFIG_VERYHIGH_SPEC;
    m_nMaxConfigSpec = CONFIG_VERYHIGH_SPEC;

    //m_hPhysicsThread = INVALID_HANDLE_VALUE;
    //m_hPhysicsActive = INVALID_HANDLE_VALUE;
    //m_bStopPhysics = 0;
    //m_bPhysicsActive = 0;

    m_pProgressListener = 0;

    m_bPaused = false;
    m_bNoUpdate = false;
    m_nUpdateCounter = 0;
    m_iApplicationInstance = -1;

    m_pPhysRenderer = 0;

    m_pXMLUtils = new CXmlUtils(this);
    m_pArchiveHost = Serialization::CreateArchiveHost();
    m_pTestSystem = new CTestSystemLegacy;
    m_pMemoryManager = CryGetIMemoryManager();
    m_pThreadTaskManager = new CThreadTaskManager;
    m_pResourceManager = new CResourceManager;
    m_pTextModeConsole = NULL;
    m_pThreadProfiler = 0;
    m_pDiskProfiler = NULL;

    InitThreadSystem();

    m_pMiniGUI = NULL;
    m_pPerfHUD = NULL;

    g_pPakHeap = new CMTSafeHeap;

    m_PlatformOSCreateFlags = 0;

    if (!AZ::AllocatorInstance<AZ::OSAllocator>::IsReady())
    {
        m_initedOSAllocator = true;
        AZ::AllocatorInstance<AZ::OSAllocator>::Create();
    }
    if (!AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady())
    {
        m_initedSysAllocator = true;
        AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
    }

    m_UpdateTimesIdx = 0U;
    m_bNeedDoWorkDuringOcclusionChecks = false;

    m_PlatformOSCreateFlags = 0;

    m_eRuntimeState = ESYSTEM_EVENT_LEVEL_UNLOAD;

    m_bHasRenderedErrorMessage = false;
    m_bIsSteamInitialized = false;

    m_pDataProbe = nullptr;
#if AZ_LEGACY_CRYSYSTEM_TRAIT_USE_MESSAGE_HANDLER
    RegisterWindowMessageHandler(this);
#endif

    m_ConfigPlatform = CONFIG_INVALID_PLATFORM;
}

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
CSystem::~CSystem()
{
    ShutDown();

#if AZ_LEGACY_CRYSYSTEM_TRAIT_USE_MESSAGE_HANDLER
    UnregisterWindowMessageHandler(this);
#endif

    CRY_ASSERT(m_windowMessageHandlers.empty() && "There exists a dangling window message handler somewhere");

    SAFE_DELETE(m_pVisRegTest);
    SAFE_DELETE(m_pThreadProfiler);
#if defined(USE_DISK_PROFILER)
    SAFE_DELETE(m_pDiskProfiler);
#endif
    SAFE_DELETE(m_pXMLUtils);
    SAFE_DELETE(m_pArchiveHost);
    SAFE_DELETE(m_pThreadTaskManager);
    SAFE_DELETE(m_pResourceManager);
    SAFE_DELETE(m_pSystemEventDispatcher);
    //  SAFE_DELETE(m_pMemoryManager);

    gEnv->pThreadManager->UnRegisterThirdPartyThread("Main");
    ShutDownThreadSystem();

    SAFE_DELETE(g_pPakHeap);

    AZCoreLogSink::Disconnect();
    if (m_initedSysAllocator)
    {
        AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
    }
    if (m_initedOSAllocator)
    {
        AZ::AllocatorInstance<AZ::OSAllocator>::Destroy();
    }

    AZ::Environment::Detach();

    m_env.pSystem = 0;
    gEnv = 0;

    // The FrameProfileSystem should clean up as late as possible as some modules create profilers during shutdown!
    m_FrameProfileSystem.Done();
}

//////////////////////////////////////////////////////////////////////////
void CSystem::Release()
{
    //Disconnect the render bus
    AZ::RenderNotificationsBus::Handler::BusDisconnect();

    delete this;
}

//////////////////////////////////////////////////////////////////////////
void CSystem::FreeLib(WIN_HMODULE hLibModule)
{
    if (hLibModule)
    {
        CryFreeLibrary(hLibModule);
        (hLibModule) = NULL;
    }
}

//////////////////////////////////////////////////////////////////////////
IStreamEngine* CSystem::GetStreamEngine()
{
    return m_pStreamEngine;
}

//////////////////////////////////////////////////////////////////////////
IRemoteConsole* CSystem::GetIRemoteConsole()
{
    return CRemoteConsole::GetInst();
}

//////////////////////////////////////////////////////////////////////////
void CSystem::SetForceNonDevMode(const bool bValue)
{
    m_bForceNonDevMode = bValue;
    if (bValue)
    {
        SetDevMode(false);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CSystem::GetForceNonDevMode() const
{
    return m_bForceNonDevMode;
}

//////////////////////////////////////////////////////////////////////////
void CSystem::SetDevMode(bool bEnable)
{
    if (bEnable)
    {
        m_bWasInDevMode = true;
    }
    m_bInDevMode = bEnable;
}


void LvlRes_export(IConsoleCmdArgs* pParams);

///////////////////////////////////////////////////
void CSystem::ShutDown()
{
    CryLogAlways("System Shutdown");

    // don't broadcast OnCrySystemShutdown unless
    // we'd previously broadcast OnCrySystemInitialized
    if (m_bInitializedSuccessfully)
    {
        EBUS_EVENT(CrySystemEventBus, OnCrySystemShutdown, *this);
    }

    m_FrameProfileSystem.Enable(false, false);

#if defined(ENABLE_LOADING_PROFILER)
    CLoadingProfilerSystem::ShutDown();
#endif

    if (m_pUserCallback)
    {
        m_pUserCallback->OnShutdown();
    }

    if (GetIRemoteConsole()->IsStarted())
    {
        GetIRemoteConsole()->Stop();
    }

    //Ensure that the load ticker is not currently running. It can perform tasks on the network systems while they're being shut down
    if (m_env.pGame && m_env.pGame->GetIGameFramework())
    {
        m_env.pGame->GetIGameFramework()->StopNetworkStallTicker();
    }

    // clean up properly the console
    if (m_pTextModeConsole)
    {
        m_pTextModeConsole->OnShutdown();
    }

    SAFE_DELETE(m_pTextModeConsole);

    KillPhysicsThread();
    
    if (m_sys_firstlaunch)
    {
        m_sys_firstlaunch->Set("0");
    }

    if ((m_bEditor) && (m_env.pConsole))
    {
        // restore the old saved cvars
        if (m_env.pConsole->GetCVar("r_Width"))
        {
            m_env.pConsole->GetCVar("r_Width")->Set(m_iWidth);
        }
        if (m_env.pConsole->GetCVar("r_Height"))
        {
            m_env.pConsole->GetCVar("r_Height")->Set(m_iHeight);
        }
        if (m_env.pConsole->GetCVar("r_ColorBits"))
        {
            m_env.pConsole->GetCVar("r_ColorBits")->Set(m_iColorBits);
        }
    }

    if (m_bEditor && !m_bRelaunch)
    {
        SaveConfiguration();
    }

    // Dispatch the full-shutdown event in case this is not a fast-shutdown.
    if (m_pSystemEventDispatcher != NULL)
    {
        m_pSystemEventDispatcher->OnSystemEvent(ESYSTEM_EVENT_FULL_SHUTDOWN, 0, 0);
    }

    // Shutdown any running VR devices.
    EBUS_EVENT(AZ::VR::HMDInitRequestBus, Shutdown);

    //////////////////////////////////////////////////////////////////////////
    // Release Game.
    //////////////////////////////////////////////////////////////////////////
    if (m_env.pEntitySystem)
    {
        m_env.pEntitySystem->Unload();
    }

    if (m_env.pPhysicalWorld)
    {
        m_env.pPhysicalWorld->SetPhysicsStreamer(0);
        m_env.pPhysicalWorld->SetPhysicsEventClient(0);
    }

    //////////////////////////////////////////////////////////////////////////
    // Clear 3D Engine resources.
    if (m_env.p3DEngine)
    {
        m_env.p3DEngine->UnloadLevel();
    }
    //////////////////////////////////////////////////////////////////////////

    // Shutdown resource manager.
    m_pResourceManager->Shutdown();

    if (m_env.pGame)
    {
        m_env.pGame->Shutdown();
        SAFE_DELETE(m_env.pGame);
    }

    if (gEnv->pLyShine)
    {
        gEnv->pLyShine->Release();
        gEnv->pLyShine = nullptr;
    }

    SAFE_DELETE(m_env.pResourceCompilerHelper);

    SAFE_RELEASE(m_env.pHardwareMouse);
    SAFE_RELEASE(m_env.pMovieSystem);
    SAFE_DELETE(m_env.pServiceNetwork);
    SAFE_RELEASE(m_env.pAISystem);
    SAFE_RELEASE(m_env.pLyShine);
    SAFE_RELEASE(m_env.pCryFont);
    SAFE_RELEASE(m_env.pNetwork);
    //  SAFE_RELEASE(m_env.pCharacterManager);
    SAFE_RELEASE(m_env.p3DEngine); // depends on EntitySystem
    SAFE_RELEASE(m_env.pEntitySystem);
    SAFE_RELEASE(m_env.pPhysicalWorld);
    if (m_env.pConsole)
    {
        ((CXConsole*)m_env.pConsole)->FreeRenderResources();
    }
    SAFE_RELEASE(m_pIZLibCompressor);
    SAFE_RELEASE(m_pIZLibDecompressor);
    SAFE_RELEASE(m_pILZ4Decompressor);
    SAFE_RELEASE(m_pIBudgetingSystem);
    SAFE_RELEASE(m_pViewSystem);
    SAFE_RELEASE(m_pLevelSystem);
    SAFE_RELEASE(m_env.pCodeCheckpointMgr);

    //Can't kill renderer before we delete CryFont, 3DEngine, etc
    if (GetIRenderer())
    {
        GetIRenderer()->ShutDown();
        SAFE_RELEASE(m_env.pRenderer);
    }

    if (m_env.pLog)
    {
        m_env.pLog->UnregisterConsoleVariables();
    }

    GetIRemoteConsole()->UnregisterConsoleVariables();

    // Release console variables.

    SAFE_RELEASE(m_pCVarQuit);
    SAFE_RELEASE(m_rWidth);
    SAFE_RELEASE(m_rHeight);
    SAFE_RELEASE(m_rWidthAndHeightAsFractionOfScreenSize);
    SAFE_RELEASE(m_rMaxWidth);
    SAFE_RELEASE(m_rMaxHeight);
    SAFE_RELEASE(m_rColorBits);
    SAFE_RELEASE(m_rDepthBits);
    SAFE_RELEASE(m_cvSSInfo);
    SAFE_RELEASE(m_rStencilBits);
    SAFE_RELEASE(m_rFullscreen);
    SAFE_RELEASE(m_rDriver);

    SAFE_RELEASE(m_sysWarnings);
    SAFE_RELEASE(m_sysKeyboard);
    SAFE_RELEASE(m_sys_profile);
    SAFE_RELEASE(m_sys_profile_additionalsub);
    SAFE_RELEASE(m_sys_profile_graph);
    SAFE_RELEASE(m_sys_profile_pagefaultsgraph);
    SAFE_RELEASE(m_sys_profile_graphScale);
    SAFE_RELEASE(m_sys_profile_filter);
    SAFE_RELEASE(m_sys_profile_filter_thread);
    SAFE_RELEASE(m_sys_profile_allThreads);
    SAFE_RELEASE(m_sys_profile_network);
    SAFE_RELEASE(m_sys_profile_peak);
    SAFE_RELEASE(m_sys_profile_peak_time);
    SAFE_RELEASE(m_sys_profile_memory);
    SAFE_RELEASE(m_sys_profile_sampler);
    SAFE_RELEASE(m_sys_profile_sampler_max_samples);
    SAFE_RELEASE(m_sys_GraphicsQuality);
    SAFE_RELEASE(m_sys_firstlaunch);
    SAFE_RELEASE(m_sys_skip_input);
    SAFE_RELEASE(m_sys_enable_budgetmonitoring);
    SAFE_RELEASE(m_sys_physics_CPU);

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEM_CPP_SECTION_3
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/System_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/System_cpp_provo.inl"
    #endif
#endif

    SAFE_RELEASE(m_sys_min_step);
    SAFE_RELEASE(m_sys_max_step);

    if (m_env.pInput)
    {
        CryLegacyInputRequestBus::Broadcast(&CryLegacyInputRequests::ShutdownInput, m_env.pInput);
        m_env.pInput = NULL;
    }

    SAFE_RELEASE(m_pNotificationNetwork);
    SAFE_RELEASE(m_env.pTelemetrySystem);
    SAFE_RELEASE(m_pTelemetryFileStream);
    SAFE_RELEASE(m_pTelemetryUDPStream);
    SAFE_RELEASE(m_env.pScriptSystem);

    SAFE_DELETE(m_env.pSoftCodeMgr);
    SAFE_DELETE(m_pMemStats);
    SAFE_DELETE(m_pSizer);
    SAFE_DELETE(m_pDefaultValidator);
    m_pValidator = nullptr;

    SAFE_DELETE(m_pPhysRenderer);

    SAFE_DELETE(m_pGemManager);
    SAFE_DELETE(m_crypto);
    SAFE_DELETE(m_env.pOverloadSceneManager);

#ifdef DOWNLOAD_MANAGER
    SAFE_RELEASE(m_pDownloadManager);
#endif //DOWNLOAD_MANAGER

    SAFE_DELETE(m_pLocalizationManager);

    //DebugStats(false, false);//true);
    //CryLogAlways("");
    //CryLogAlways("release mode memory manager stats:");
    //DumpMMStats(true);

    SAFE_DELETE(m_pCpu);

    delete m_pCmdLine;
    m_pCmdLine = 0;

    // Audio System Shutdown!
    // Shut down audio as late as possible but before the streaming system and console get released!
    Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::Release);

    // Shut down the streaming system and console as late as possible and after audio!
    SAFE_DELETE(m_pStreamEngine);
    SAFE_RELEASE(m_env.pConsole);

    // Log must be last thing released.
    SAFE_RELEASE(m_env.pProfileLogSystem);
    if (m_env.pLog)
    {
        m_env.pLog->FlushAndClose();
    }
    SAFE_RELEASE(m_env.pLog);   // creates log backup

    // Shut down the CryPak system after audio and log system!
    SAFE_DELETE(m_env.pCryPak);

    ShutdownFileSystem();

#if defined(MAP_LOADING_SLICING)
    delete gEnv->pSystemScheduler;
#endif // defined(MAP_LOADING_SLICING)

    ShutdownModuleLibraries();

    EBUS_EVENT(CrySystemEventBus, OnCrySystemPostShutdown);
}

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
void CSystem::Quit()
{
    CryLogAlways("CSystem::Quit invoked from thread %" PRI_THREADID " (main is %" PRI_THREADID ")", GetCurrentThreadId(), gEnv->mMainThreadId);
    
    m_bQuit = true;

    // If this was set from anywhere but the main thread, bail and let the main thread handle shutdown
    if (GetCurrentThreadId() != gEnv->mMainThreadId)
    {
        return;
    }

    if (m_pUserCallback)
    {
        m_pUserCallback->OnQuit();
    }
    if (gEnv->pCryPak && gEnv->pCryPak->GetLvlResStatus())
    {
        LvlRes_export(0);       // executable was started with -LvlRes so it should export lvlres file on quit
    }

    if (GetIRenderer())
    {
        GetIRenderer()->RestoreGamma();
    }

    /*
    * TODO: This call to _exit, _Exit, TerminateProcess etc. needs to
    * eventually be removed. This causes an extremely early exit before we 
    * actually perform cleanup. When this gets called most managers are 
    * simply never deleted and we leave it to the OS to clean up our mess
    * which is just really bad practice. However there are LOTS of issues
    * with shutdown at the moment. Removing this will simply cause 
    * a crash when either the Editor or Launcher initiate shutdown. Both 
    * applications crash differently too. Bugs will be logged about those 
    * issues.
    */
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEM_CPP_SECTION_4
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/System_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/System_cpp_provo.inl"
    #endif
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(WIN32) || defined(WIN64)
    TerminateProcess(GetCurrentProcess(), 0);
#else
    _exit(0);
#endif

#ifdef WIN32
    //Post a WM_QUIT message to the Win32 api which causes the message loop to END
    //This is not the same as handling a WM_DESTROY event which destroys a window
    //but keeps the message loop alive.
    PostQuitMessage(0);
#endif
}
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
bool CSystem::IsQuitting() const
{
    return m_bQuit;
}

//////////////////////////////////////////////////////////////////////////
void CSystem::SetIProcess(IProcess* process)
{
    m_pProcess = process;
    //if (m_pProcess)
    //m_pProcess->SetPMessage("");
}

//////////////////////////////////////////////////////////////////////////
ISystem* CSystem::GetCrySystem()
{
    return this;
}

//////////////////////////////////////////////////////////////////////////
// Physics thread task
//////////////////////////////////////////////////////////////////////////
class CPhysicsThreadTask
    : public IThreadTask
{
public:

    CPhysicsThreadTask()
    {
        m_bStopRequested = 0;
        m_bIsActive = 0;
        m_stepRequested = 0;
        m_bProcessing = 0;
        m_doZeroStep = 0;
        m_lastStepTimeTaken = 0U;
        m_lastWaitTimeTaken = 0U;
    }

    //////////////////////////////////////////////////////////////////////////
    // IThreadTask implementation.
    //////////////////////////////////////////////////////////////////////////
    virtual void OnUpdate()
    {
        Run();
        // At the end.. delete the task
        delete this;
    }
    virtual void Stop()
    {
        Cancel();
    }
    virtual SThreadTaskInfo* GetTaskInfo() { return &m_TaskInfo; }
    //////////////////////////////////////////////////////////////////////////

    virtual void Run()
    {
        m_bStopRequested = 0;
        m_bIsActive = 1;

        float step, timeTaken, kSlowdown = 1.0f;
        int nSlowFrames = 0;
        int64 timeStart;
#ifdef ENABLE_LW_PROFILERS
        LARGE_INTEGER stepStart, stepEnd;
#endif
        LARGE_INTEGER waitStart, waitEnd;
        uint64 yieldBegin = 0U;
        MarkThisThreadForDebugging("Physics");

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEM_CPP_SECTION_5
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/System_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/System_cpp_provo.inl"
    #endif
#endif
        while (true)
        {
            QueryPerformanceCounter(&waitStart);
            m_FrameEvent.Wait(); // Wait untill new frame
            QueryPerformanceCounter(&waitEnd);
            m_lastWaitTimeTaken = waitEnd.QuadPart - waitStart.QuadPart;

            if (m_bStopRequested)
            {
                UnmarkThisThreadFromDebugging();
                return;
            }
            bool stepped = false;
#ifdef ENABLE_LW_PROFILERS
            QueryPerformanceCounter(&stepStart);
#endif
            while ((step = m_stepRequested) > 0 || m_doZeroStep)
            {
                stepped = true;
                m_stepRequested = 0;
                m_bProcessing = 1;
                m_doZeroStep = 0;

                PhysicsVars* pVars = gEnv->pPhysicalWorld->GetPhysVars();
                pVars->bMultithreaded = 1;
                gEnv->pPhysicalWorld->TracePendingRays();
                if (kSlowdown != 1.0f)
                {
                    step = max(1, FtoI(step * kSlowdown * 50 - 0.5f)) * 0.02f;
                    pVars->timeScalePlayers = 1.0f / max(kSlowdown, 0.2f);
                }
                else
                {
                    pVars->timeScalePlayers = 1.0f;
                }
                step = min(step, pVars->maxWorldStep);
                timeStart = CryGetTicks();
                IGameFramework* pIGameFramework = gEnv->pGame ? gEnv->pGame->GetIGameFramework() : nullptr;
                if (pIGameFramework)
                {
                    pIGameFramework->PrePhysicsTimeStep(step);
                }
                gEnv->pPhysicalWorld->TimeStep(step);
                timeTaken = gEnv->pTimer->TicksToSeconds(CryGetTicks() - timeStart);
                if (timeTaken > step * 0.9f)
                {
                    if (++nSlowFrames > 5)
                    {
                        kSlowdown = step * 0.9f / timeTaken;
                    }
                }
                else
                {
                    kSlowdown = 1.0f, nSlowFrames = 0;
                }
                gEnv->pPhysicalWorld->TracePendingRays(2);
                m_bProcessing = 0;
                //int timeSleep = (int)((m_timeTarget-gEnv->pTimer->GetAsyncTime()).GetMilliSeconds()*0.9f);
                //Sleep(max(0,timeSleep));
            }
            if (!stepped)
            {
                Sleep(0);
            }
            m_FrameDone.Set();
#ifdef ENABLE_LW_PROFILERS
            QueryPerformanceCounter(&stepEnd);
            m_lastStepTimeTaken = stepEnd.QuadPart - stepStart.QuadPart;
#endif
        }
    }
    virtual void Cancel()
    {
        Pause();
        m_bStopRequested = 1;
        m_FrameEvent.Set();
        m_bIsActive = 0;
    }

    int Pause()
    {
        if (m_bIsActive)
        {
            AZ_PROFILE_FUNCTION_STALL(AZ::Debug::ProfileCategory::System);
            PhysicsVars* vars = gEnv->pPhysicalWorld->GetPhysVars();
            vars->lastTimeStep = 0;
            m_bIsActive = 0;
            m_stepRequested = min((float)m_stepRequested, 2.f * vars->maxWorldStep);
            while (m_bProcessing)
            {
                ;
            }
            return 1;
        }
        gEnv->pPhysicalWorld->GetPhysVars()->lastTimeStep = 0;
        return 0;
    }
    int Resume()
    {
        if (!m_bIsActive)
        {
            m_bIsActive = 1;
            return 1;
        }
        return 0;
    }
    int IsActive() { return m_bIsActive; }
    int RequestStep(float dt)
    {
        if (m_bIsActive && dt > FLT_EPSILON)
        {
            m_stepRequested += dt;
            m_stepRequested = min((float)m_stepRequested, 10.f * gEnv->pPhysicalWorld->GetPhysVars()->maxWorldStep);
            if (dt <= 0.0f)
            {
                m_doZeroStep = 1;
            }
            m_FrameEvent.Set();
        }

        return m_bProcessing;
    }
    float GetRequestedStep() { return m_stepRequested; }

    uint64 LastStepTaken() const
    {
        return m_lastStepTimeTaken;
    }

    uint64 LastWaitTime() const
    {
        return m_lastWaitTimeTaken;
    }

    void EnsureStepDone()
    {
        CRYPROFILE_SCOPE_PROFILE_MARKER("SysUpdate:PhysicsEnsureDone");
        FRAME_PROFILER("SysUpdate:PhysicsEnsureDone", gEnv->pSystem, PROFILE_SYSTEM);
        if (m_bIsActive)
        {
            while (m_stepRequested > 0.0f || m_bProcessing)
            {
                m_FrameDone.Wait();
            }
        }
    }

protected:

    volatile int m_bStopRequested;
    volatile int m_bIsActive;
    volatile float m_stepRequested;
    volatile int m_bProcessing;
    volatile int m_doZeroStep;
    volatile uint64 m_lastStepTimeTaken;
    volatile uint64 m_lastWaitTimeTaken;

    CryEvent m_FrameEvent;
    CryEvent m_FrameDone;

    SThreadTaskInfo m_TaskInfo;
};

void CSystem::CreatePhysicsThread()
{
    if (!m_PhysThread)
    {
        //////////////////////////////////////////////////////////////////////////
        SThreadTaskParams threadParams;
        threadParams.name = "Physics";
        threadParams.nFlags = THREAD_TASK_BLOCKING;
        threadParams.nStackSizeKB = PHYSICS_STACK_SIZE >> 10;
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEM_CPP_SECTION_6
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/System_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/System_cpp_provo.inl"
    #endif
#endif

        {            
            m_PhysThread = new CPhysicsThreadTask;
            GetIThreadTaskManager()->RegisterTask(m_PhysThread, threadParams);
        }
    }

    if (g_cvars.sys_limit_phys_thread_count)
    {
        PhysicsVars* pVars = gEnv->pPhysicalWorld->GetPhysVars();
        pVars->numThreads = max(1, min(pVars->numThreads, (int)m_pCpu->GetPhysCPUCount() - 1));
    }
}

void CSystem::KillPhysicsThread()
{
    if (m_PhysThread)
    {
        GetIThreadTaskManager()->UnregisterTask(m_PhysThread);
        m_PhysThread = 0;
    }
}

//////////////////////////////////////////////////////////////////////////
void CSystem::TelemetryStreamFileChanged(ICVar* pCVar)
{
    CRY_ASSERT(pCVar);

    if (CSystem* pThis = static_cast<CSystem*>(gEnv->pSystem))
    {
        if (Telemetry::CTelemetrySystem* pTelemetrySystem = static_cast<Telemetry::CTelemetrySystem*>(pThis->m_env.pTelemetrySystem))
        {
            if (Telemetry::CFileStream* pTelemetryFileStream = pThis->m_pTelemetryFileStream)
            {
                pTelemetrySystem->Flush();

                pTelemetrySystem->DetachStream(pTelemetryFileStream);

                pTelemetryFileStream->Shutdown();

                const char* pFileName = pCVar->GetString();

                if ((pFileName[0] != '0') && (pFileName[0] != '\0'))
                {
                    if (pTelemetryFileStream->Init(pFileName))
                    {
                        pTelemetrySystem->AttachStream(pTelemetryFileStream);
                    }
                    else
                    {
                        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "Failed to initialize telemetry file stream!");
                    }
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CSystem::TelemetryStreamIPChanged(ICVar* pCVar)
{
    CRY_ASSERT(pCVar);

    if (CSystem* pThis = static_cast<CSystem*>(gEnv->pSystem))
    {
        if (Telemetry::CTelemetrySystem* pTelemetrySystem = static_cast<Telemetry::CTelemetrySystem*>(pThis->m_env.pTelemetrySystem))
        {
            if (Telemetry::CUDPStream* pTelemetryUDPStream = pThis->m_pTelemetryUDPStream)
            {
                pTelemetrySystem->Flush();

                pTelemetrySystem->DetachStream(pTelemetryUDPStream);

                pTelemetryUDPStream->Shutdown();

                const char* pIPAddress = pCVar->GetString();

                if ((pIPAddress[0] != '0') && (pIPAddress[0] != '\0'))
                {
                    if (pTelemetryUDPStream->Init(pIPAddress))
                    {
                        pTelemetrySystem->AttachStream(pTelemetryUDPStream);
                    }
                    else
                    {
                        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "Failed to initialize telemetry UDP stream!");
                    }
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
int CSystem::SetThreadState(ESubsystem subsys, bool bActive)
{
    switch (subsys)
    {
    case ESubsys_Physics:
    {
        if (m_PhysThread)
        {
            return bActive ? ((CPhysicsThreadTask*)m_PhysThread)->Resume() : ((CPhysicsThreadTask*)m_PhysThread)->Pause();
        }
    }
    break;
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
void CSystem::SleepIfInactive()
{
    // ProcessSleep()
    if (m_bDedicatedServer || m_bEditor || gEnv->bMultiplayer)
    {
        return;
    }

#if defined(WIN32)
    WIN_HWND hRendWnd = GetIRenderer()->GetHWND();
    if (!hRendWnd)
    {
        return;
    }

    AZ_TRACE_METHOD();
    // Loop here waiting for window to be activated.
    for (int nLoops = 0; nLoops < 5; nLoops++)
    {
        WIN_HWND hActiveWnd = ::GetActiveWindow();
        if (hActiveWnd == hRendWnd)
        {
            break;
        }

        AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::PumpSystemEventLoopUntilEmpty);

        if (gEnv->pGame && gEnv->pGame->GetIGameFramework())
        {
            // During the time demo, do not sleep even in inactive window.
            bool isTimeDemoActive = false;
            TimeDemoRecorderBus::BroadcastResult(isTimeDemoActive, &TimeDemoRecorderBus::Events::IsTimeDemoActive);
            if (isTimeDemoActive)
            {
                break;
            }
        }
        Sleep(5);
    }
#endif
}

//////////////////////////////////////////////////////////////////////////
void CSystem::SleepIfNeeded()
{
    FUNCTION_PROFILER_FAST(this, PROFILE_SYSTEM, g_bProfilerEnabled);

    ITimer* const pTimer = gEnv->pTimer;
    static bool firstCall = true;

    typedef MiniQueue<CTimeValue, 32> PrevNow;
    static PrevNow prevNow;
    if (firstCall)
    {
        m_lastTickTime = pTimer->GetAsyncTime();
        prevNow.Push(m_lastTickTime);
        firstCall = false;
        return;
    }

    const float maxRate = m_svDedicatedMaxRate->GetFVal();
    const float minTime = 1.0f / maxRate;
    CTimeValue now = pTimer->GetAsyncTime();
    float elapsed = (now - m_lastTickTime).GetSeconds();

    if (prevNow.Full())
    {
        prevNow.Pop();
    }
    prevNow.Push(now);

    static bool allowStallCatchup = true;
    if (elapsed > minTime && allowStallCatchup)
    {
        allowStallCatchup = false;
        m_lastTickTime = pTimer->GetAsyncTime();
        return;
    }
    allowStallCatchup = true;

    float totalElapsed = (now - prevNow.Front()).GetSeconds();
    float wantSleepTime = CLAMP(minTime * (prevNow.Size() - 1) - totalElapsed, 0, (minTime - elapsed) * 0.9f);
    static float sleepTime = 0;
    sleepTime = (15 * sleepTime + wantSleepTime) / 16;
    int sleepMS = (int)(1000.0f * sleepTime + 0.5f);
    if (sleepMS > 0)
    {
        AZ_PROFILE_FUNCTION_IDLE(AZ::Debug::ProfileCategory::System);
        Sleep(sleepMS);
    }

    m_lastTickTime = pTimer->GetAsyncTime();
}

extern DWORD g_idDebugThreads[];
extern int g_nDebugThreads;
int prev_sys_float_exceptions = -1;

//////////////////////////////////////////////////////////////////////
bool CSystem::UpdatePreTickBus(int updateFlags, int nPauseMode)
{
    CRYPROFILE_SCOPE_PROFILE_MARKER("CSystem::Update()");

    // If we detect the quit flag at the start of Update, that means it was set
    // from another thread, and we should quit immediately. Otherwise, it will
    // be set by game logic or the console during Update and we will quit later
    if (IsQuitting())
    {
        Quit();
        return false;
    }

    // CryAction has been moved to the optional CryLegacy Gem, so if it doesn't exist we need to do this here
    if (!gEnv->pGame || !gEnv->pGame->GetIGameFramework())
    {
        RenderBegin();
    }

#ifndef EXCLUDE_UPDATE_ON_CONSOLE
    // do the dedicated sleep earlier than the frame profiler to avoid having it counted
    if (gEnv->IsDedicated())
    {
#if defined(MAP_LOADING_SLICING)
        gEnv->pSystemScheduler->SchedulingSleepIfNeeded();
#else
        SleepIfNeeded();
#endif // defined(MAP_LOADING_SLICING)
    }
#endif //EXCLUDE_UPDATE_ON_CONSOLE

    gEnv->pOverloadSceneManager->Update();

    m_pPlatformOS->Tick(m_Time.GetRealFrameTime());

#ifdef WIN32
    // enable/disable SSE fp exceptions (#nan and /0)
    // need to do it each frame since sometimes they are being reset
    _mm_setcsr(_mm_getcsr() & ~0x280 | (g_cvars.sys_float_exceptions > 0 ? 0 : 0x280));
#endif //WIN32

    FUNCTION_PROFILER_LEGACYONLY(GetISystem(), PROFILE_SYSTEM);
    AZ_TRACE_METHOD();

    m_nUpdateCounter++;
#ifndef EXCLUDE_UPDATE_ON_CONSOLE
    if (!m_sDelayedScreeenshot.empty())
    {
        gEnv->pRenderer->ScreenShot(m_sDelayedScreeenshot.c_str());
        m_sDelayedScreeenshot.clear();
    }

    // Check if game needs to be sleeping when not active.
    SleepIfInactive();

    if (m_pUserCallback)
    {
        m_pUserCallback->OnUpdate();
    }

    //////////////////////////////////////////////////////////////////////////
    // Enable/Disable floating exceptions.
    //////////////////////////////////////////////////////////////////////////
    prev_sys_float_exceptions += 1 + g_cvars.sys_float_exceptions & prev_sys_float_exceptions >> 31;
    if (prev_sys_float_exceptions != g_cvars.sys_float_exceptions)
    {
        prev_sys_float_exceptions = g_cvars.sys_float_exceptions;

        EnableFloatExceptions(g_cvars.sys_float_exceptions);
        UpdateFPExceptionsMaskForThreads();
    }
#endif //EXCLUDE_UPDATE_ON_CONSOLE
       //////////////////////////////////////////////////////////////////////////

    if (m_env.pLog)
    {
        m_env.pLog->Update();
    }

#if !defined(RELEASE) || defined(RELEASE_LOGGING)
    GetIRemoteConsole()->Update();
#endif

    if (gEnv->pLocalMemoryUsage != NULL)
    {
        gEnv->pLocalMemoryUsage->OnUpdate();
    }

    if (!gEnv->IsEditor())
    {
        // If the dimensions of the render target change,
        // or are different from the camera defaults,
        // we need to update the camera frustum.
        CCamera& viewCamera = GetViewCamera();
        const int renderTargetWidth = m_rWidth->GetIVal();
        const int renderTargetHeight = m_rHeight->GetIVal();

        if (renderTargetWidth != viewCamera.GetViewSurfaceX() ||
            renderTargetHeight != viewCamera.GetViewSurfaceZ())
        {
            viewCamera.SetFrustum(renderTargetWidth,
                renderTargetHeight,
                viewCamera.GetFov(),
                viewCamera.GetNearPlane(),
                viewCamera.GetFarPlane(),
                gEnv->pRenderer->GetPixelAspectRatio());
        }
    }
#ifndef EXCLUDE_UPDATE_ON_CONSOLE
    if (m_pTestSystem)
    {
        m_pTestSystem->Update();
    }
#endif //EXCLUDE_UPDATE_ON_CONSOLE
    if (nPauseMode != 0)
    {
        m_bPaused = true;
    }
    else
    {
        m_bPaused = false;
    }

    if (m_env.pGame)
    {
        //bool bDevMode = m_env.pGame->GetModuleState( EGameDevMode );
        //if (bDevMode != m_bInDevMode)
        //SetDevMode(bDevMode);
    }
#ifdef PROFILE_WITH_VTUNE
    if (m_bInDevMode)
    {
        if (VTPause != NULL && VTResume != NULL)
        {
            static bool bVtunePaused = true;

            const AzFramework::InputChannel* inputChannelScrollLock = AzFramework::InputChannelRequests::FindInputChannel(AzFramework::InputDeviceKeyboard::Key::WindowsSystemScrollLock);
            const bool bPaused = (inputChannelScrollLock ? inputChannelScrollLock->IsActive() : false);

            {
                if (bVtunePaused && !bPaused)
                {
                    GetIProfilingSystem()->VTuneResume();
                }
                if (!bVtunePaused && bPaused)
                {
                    GetIProfilingSystem()->VTunePause();
                }
                bVtunePaused = bPaused;
            }
        }
    }
#endif //PROFILE_WITH_VTUNE

#ifdef SOFTCODE_SYSTEM_ENABLED
    if (m_env.pSoftCodeMgr)
    {
        m_env.pSoftCodeMgr->PollForNewModules();
    }
#endif

    if (m_pStreamEngine)
    {
        FRAME_PROFILER("StreamEngine::Update()", this, PROFILE_SYSTEM);
        m_pStreamEngine->Update();
    }

#ifndef EXCLUDE_UPDATE_ON_CONSOLE
    if (m_bIgnoreUpdates)
    {
        return true;
    }
#endif //EXCLUDE_UPDATE_ON_CONSOLE
    if (m_env.pCharacterManager)
    {
        FRAME_PROFILER("ICharacterManager::Update()", this, PROFILE_SYSTEM);
        m_env.pCharacterManager->Update(nPauseMode != 0);
    }

    //static bool sbPause = false;
    //bool bPause = false;
    bool bNoUpdate = false;
#ifndef EXCLUDE_UPDATE_ON_CONSOLE
    //check what is the current process
    IProcess* pProcess = GetIProcess();
    if (!pProcess)
    {
        return (true); //should never happen
    }
    if (m_sysNoUpdate && m_sysNoUpdate->GetIVal())
    {
        bNoUpdate = true;
        updateFlags = ESYSUPDATE_IGNORE_AI | ESYSUPDATE_IGNORE_PHYSICS;
    }

    //if ((pProcess->GetFlags() & PROC_MENU) || (m_sysNoUpdate && m_sysNoUpdate->GetIVal()))
    //  bPause = true;
    m_bNoUpdate = bNoUpdate;
#endif //EXCLUDE_UPDATE_ON_CONSOLE

    //check if we are quitting from the game
    if (IsQuitting())
    {
        Quit();
        return (false);
    }

    //limit frame rate if vsync is turned off
    //for consoles this is done inside renderthread to be vsync dependent
    {
        FRAME_PROFILER_LEGACYONLY("FRAME_CAP", gEnv->pSystem, PROFILE_SYSTEM);
        AZ_TRACE_METHOD_NAME("FrameLimiter");
        static ICVar* pSysMaxFPS = NULL;
        static ICVar* pVSync = NULL;

        if (pSysMaxFPS == NULL && gEnv && gEnv->pConsole)
        {
            pSysMaxFPS = gEnv->pConsole->GetCVar("sys_MaxFPS");
        }
        if (pVSync == NULL && gEnv && gEnv->pConsole)
        {
            pVSync = gEnv->pConsole->GetCVar("r_Vsync");
        }

        if (pSysMaxFPS && pVSync)
        {
            int32 maxFPS = pSysMaxFPS->GetIVal();
            uint32 vSync = pVSync->GetIVal();

            if (maxFPS == 0 && vSync == 0)
            {
                ILevelSystem* pLvlSys = GetILevelSystem();
                const bool inLevel = pLvlSys && pLvlSys->GetCurrentLevel() != 0;
                maxFPS = !inLevel || IsPaused() ? 60 : 0;
            }

            if (maxFPS > 0 && vSync == 0)
            {
                CTimeValue timeFrameMax;
                const float safeMarginFPS = 0.5f;//save margin to not drop below 30 fps
                static CTimeValue sTimeLast = gEnv->pTimer->GetAsyncTime();
                timeFrameMax.SetMilliSeconds((int64)(1000.f / ((float)maxFPS + safeMarginFPS)));
                const CTimeValue timeLast = timeFrameMax + sTimeLast;
                while (timeLast.GetValue() > gEnv->pTimer->GetAsyncTime().GetValue())
                {
                    CrySleep(0);
                }
                sTimeLast = gEnv->pTimer->GetAsyncTime();
            }
        }
    }

    //////////////////////////////////////////////////////////////////////
    //update time subsystem
    m_Time.UpdateOnFrameStart();

    if (m_env.p3DEngine)
    {
        m_env.p3DEngine->OnFrameStart();
    }

    //////////////////////////////////////////////////////////////////////
    // update rate limiter for dedicated server
    if (m_pServerThrottle.get())
    {
        m_pServerThrottle->Update();
    }

    //////////////////////////////////////////////////////////////////////
    // initial network update
    if (m_env.pNetwork)
    {
        FRAME_PROFILER("INetwork::SyncWithGame", gEnv->pSystem, PROFILE_SYSTEM);
        m_env.pNetwork->SyncWithGame(eNGS_FrameStart);
    }

    //////////////////////////////////////////////////////////////////////////
    // Update telemetry system.
    m_env.pTelemetrySystem->Update();

    //////////////////////////////////////////////////////////////////////////
    // Update script system.
    if (m_env.pScriptSystem)
    {
        FRAME_PROFILER("IScriptSystem::Update", gEnv->pSystem, PROFILE_SYSTEM);
        m_env.pScriptSystem->Update();
    }

    if (m_env.pRenderer->GetIStereoRenderer()->IsRenderingToHMD())
    {
        EBUS_EVENT(AZ::VR::HMDDeviceRequestBus, UpdateInternalState);
    }

    if (m_env.pPhysicalWorld && m_env.pPhysicalWorld->GetPhysVars()->bForceSyncPhysics)
    {
        if (m_PhysThread)
        {
            AZ_PROFILE_SCOPE_STALL(AZ::Debug::ProfileCategory::System, "CSystem::Update:EnsurePhysicsStepDone");
            static_cast<CPhysicsThreadTask*>(m_PhysThread)->EnsureStepDone();
        }
    }

    //////////////////////////////////////////////////////////////////////////
    //update the dynamic response system.
    if (m_env.pDynamicResponseSystem)
    {
        FRAME_PROFILER("SysUpdate:DynamicResponseSystem", this, PROFILE_SYSTEM);
        m_env.pDynamicResponseSystem->Update();
    }

    //////////////////////////////////////////////////////////////////////
    //update console system
    if (m_env.pConsole)
    {
        FRAME_PROFILER("SysUpdate:Console", this, PROFILE_SYSTEM);
        m_env.pConsole->Update();
    }

#ifndef EXCLUDE_UPDATE_ON_CONSOLE

    //////////////////////////////////////////////////////////////////////
    //update notification network system
    if (m_pNotificationNetwork)
    {
        FRAME_PROFILER("SysUpdate:NotificationNetwork", this, PROFILE_SYSTEM);
        m_pNotificationNetwork->Update();
    }
#endif //EXCLUDE_UPDATE_ON_CONSOLE
       //////////////////////////////////////////////////////////////////////
       //update sound system Part 1 if in Editor / in Game Mode Viewsystem updates the Listeners
    if (!m_env.IsEditorGameMode())
    {
        if ((updateFlags & ESYSUPDATE_EDITOR) != 0 && !bNoUpdate && nPauseMode != 1)
        {
            // updating the Listener Position in a first separate step.
            // Updating all views here is a bit of a workaround, since we need
            // to ensure that sound listeners owned by inactive views are also
            // marked as inactive. Ideally that should happen when exiting game mode.
            if (GetIViewSystem())
            {
                FRAME_PROFILER("SysUpdate:UpdateSoundListeners", this, PROFILE_SYSTEM);
                GetIViewSystem()->UpdateSoundListeners();
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Update Threads Task Manager.
    //////////////////////////////////////////////////////////////////////////
    {
        FRAME_PROFILER("SysUpdate:ThreadTaskManager", this, PROFILE_SYSTEM);
        m_pThreadTaskManager->OnUpdate();
    }

    //////////////////////////////////////////////////////////////////////////
    // Update Resource Manager.
    //////////////////////////////////////////////////////////////////////////
    {
        FRAME_PROFILER("SysUpdate:ResourceManager", this, PROFILE_SYSTEM);
        m_pResourceManager->Update();
    }

    //////////////////////////////////////////////////////////////////////
    // update physic system
    //static float time_zero = 0;
    if (m_sys_physics_CPU->GetIVal() > 0 && !gEnv->IsDedicated())
    {
        CreatePhysicsThread();
    }
    else
    {
        KillPhysicsThread();
    }

    static int g_iPausedPhys = 0;
    PhysicsVars* pVars = m_env.pPhysicalWorld->GetPhysVars();
    pVars->threadLag = 0;

    CPhysicsThreadTask* pPhysicsThreadTask = ((CPhysicsThreadTask*)m_PhysThread);
    if (!pPhysicsThreadTask)
    {
        CRYPROFILE_SCOPE_PROFILE_MARKER("SysUpdate:AllAIAndPhysics");
        FRAME_PROFILER_LEGACYONLY("SysUpdate:AllAIAndPhysics", this, PROFILE_SYSTEM);
        AZ_TRACE_METHOD_NAME("SysUpdate::AllAIAndPhysics");

        //////////////////////////////////////////////////////////////////////
        // update entity system (a little bit) before physics
        if (nPauseMode != 1)
        {
            if (!bNoUpdate)
            {
                //////////////////////////////////////////////////////////////////////
                //update game
                if (m_env.pGame)
                {
                    m_env.pGame->PrePhysicsUpdate();
                }
                //////////////////////////////////////////////////////////////////////
                //update entity system
                if (m_env.pEntitySystem && g_cvars.sys_entitysystem)
                {
                    m_env.pEntitySystem->PrePhysicsUpdate();
                }

                EBUS_EVENT(CrySystemEventBus, OnCrySystemPrePhysicsUpdate);
            }
        }

        // intermingle physics/AI updates so that if we get a big timestep (frame rate glitch etc) the
        // AI gets to steer entities before they travel over cliffs etc.
        float maxTimeStep = 0.0f;
        if (m_env.pAISystem)
        {
            maxTimeStep = m_env.pAISystem->GetUpdateInterval();
        }
        else
        {
            maxTimeStep = 0.25f;
        }
        int maxSteps = 1;
        float fCurTime = m_Time.GetCurrTime();
        float fPrevTime = m_env.pPhysicalWorld->GetPhysicsTime();
        float timeToDo = m_Time.GetFrameTime();//fCurTime - fPrevTime;
        if (m_env.bMultiplayer)
        {
            timeToDo = m_Time.GetRealFrameTime();
        }
        m_env.pPhysicalWorld->TracePendingRays();



        while (timeToDo > 0.0001f && maxSteps-- > 0)
        {
            float thisStep = min(maxTimeStep, timeToDo);
            timeToDo -= thisStep;

            if ((nPauseMode != 1) && !(updateFlags & ESYSUPDATE_IGNORE_PHYSICS) && g_cvars.sys_physics && !bNoUpdate)
            {
                FRAME_PROFILER("SysUpdate:Physics", this, PROFILE_SYSTEM);

                int iPrevTime = m_env.pPhysicalWorld->GetiPhysicsTime();
                //float fPrevTime=m_env.pPhysicalWorld->GetPhysicsTime();
                pVars->bMultithreaded = 0;
                pVars->timeScalePlayers = 1.0f;
                if (!(updateFlags & ESYSUPDATE_MULTIPLAYER))
                {
                    m_env.pPhysicalWorld->TimeStep(thisStep);
                }
                else
                {
                    //@TODO: fixed step in game.
                    /*
                    if (m_env.pGame->UseFixedStep())
                    {
                        m_env.pPhysicalWorld->TimeStep(fCurTime-fPrevTime, 0);
                        int iCurTime = m_env.pPhysicalWorld->GetiPhysicsTime();

                        m_env.pPhysicalWorld->SetiPhysicsTime(m_env.pGame->SnapTime(iPrevTime));
                        int i, iStep=m_env.pGame->GetiFixedStep();
                        float fFixedStep = m_env.pGame->GetFixedStep();
                        for(i=min(20*iStep,m_env.pGame->SnapTime(iCurTime)-m_pGame->SnapTime(iPrevTime)); i>0; i-=iStep)
                        {
                            m_env.pGame->ExecuteScheduledEvents();
                            m_env.pPhysicalWorld->TimeStep(fFixedStep, ent_rigid|ent_skip_flagged);
                        }

                        m_env.pPhysicalWorld->SetiPhysicsTime(iPrevTime);
                        m_env.pPhysicalWorld->TimeStep(fCurTime-fPrevTime, ent_rigid|ent_flagged_only);

                        m_env.pPhysicalWorld->SetiPhysicsTime(iPrevTime);
                        m_env.pPhysicalWorld->TimeStep(fCurTime-fPrevTime, ent_living|ent_independent|ent_deleted);
                    }
                    else
                    */
                    m_env.pPhysicalWorld->TimeStep(thisStep);
                }
                g_iPausedPhys = 0;
            }
            else if (!(g_iPausedPhys++ & 31))
            {
                m_env.pPhysicalWorld->TimeStep(0);  // make sure objects get all notifications; flush deleted ents
            }
            gEnv->pPhysicalWorld->TracePendingRays(2);

            {
                FRAME_PROFILER("SysUpdate:Physics - PumpLoggedEvents", this, PROFILE_SYSTEM);
                CRYPROFILE_SCOPE_PROFILE_MARKER("PumpLoggedEvents");
                m_env.pPhysicalWorld->PumpLoggedEvents();
            }

            EBUS_EVENT(CrySystemEventBus, OnCrySystemPostPhysicsUpdate);

            // now AI
            if ((nPauseMode == 0) && !(updateFlags & ESYSUPDATE_IGNORE_AI) && g_cvars.sys_ai && !bNoUpdate)
            {
                FRAME_PROFILER("SysUpdate:AI", this, PROFILE_SYSTEM);
                //////////////////////////////////////////////////////////////////////
                //update AI system - match physics
                if (m_env.pAISystem && !m_cvAIUpdate->GetIVal() && g_cvars.sys_ai)
                {
                    m_env.pAISystem->Update(gEnv->pTimer->GetFrameStartTime(), gEnv->pTimer->GetFrameTime());
                }
            }
        }

        // Make sure we don't lag too far behind
        if ((nPauseMode != 1) && !(updateFlags & ESYSUPDATE_IGNORE_PHYSICS))
        {
            if (fabsf(m_env.pPhysicalWorld->GetPhysicsTime() - fCurTime) > 0.01f)
            {
                //GetILog()->LogToConsole("Adjusting physical world clock by %.5f", fCurTime-m_env.pPhysicalWorld->GetPhysicsTime());
                m_env.pPhysicalWorld->SetPhysicsTime(fCurTime);
            }
        }
    }
    else
    {
        {
            FRAME_PROFILER_LEGACYONLY("SysUpdate:PumpLoggedEvents", this, PROFILE_SYSTEM);
            CRYPROFILE_SCOPE_PROFILE_MARKER("PumpLoggedEvents");
            AZ_TRACE_METHOD_NAME("PumpLoggedEvents");
            m_env.pPhysicalWorld->PumpLoggedEvents();
        }

        // In multithreaded physics mode, post physics fires after physics events are dispatched on the main thread.
        EBUS_EVENT(CrySystemEventBus, OnCrySystemPostPhysicsUpdate);

        //////////////////////////////////////////////////////////////////////
        // update entity system (a little bit) before physics
        if (nPauseMode != 1)
        {
            if (!bNoUpdate)
            {
                //////////////////////////////////////////////////////////////////////
                //update game
                if (m_env.pGame)
                {
                    FRAME_PROFILER("SysUpdate:Game:PrePhysicsUpdate", this, PROFILE_SYSTEM);
                    m_env.pGame->PrePhysicsUpdate();
                }
                //////////////////////////////////////////////////////////////////////
                //update entity system
                if (m_env.pEntitySystem && g_cvars.sys_entitysystem)
                {
                    FRAME_PROFILER("SysUpdate:CryEntitySystem:PrePhysicsUpdate", this, PROFILE_SYSTEM);
                    m_env.pEntitySystem->PrePhysicsUpdate();
                }

                EBUS_EVENT(CrySystemEventBus, OnCrySystemPrePhysicsUpdate);
            }
        }

        if ((nPauseMode != 1) && !(updateFlags & ESYSUPDATE_IGNORE_PHYSICS))
        {
            pPhysicsThreadTask->Resume();
            float lag = pPhysicsThreadTask->GetRequestedStep();

            if (pPhysicsThreadTask->RequestStep(m_Time.GetFrameTime()))
            {
                pVars->threadLag = lag + m_Time.GetFrameTime();
                //GetILog()->Log("Physics thread lags behind; accum time %.3f", pVars->threadLag);
            }
        }
        else
        {
            pPhysicsThreadTask->Pause();
            m_env.pPhysicalWorld->TracePendingRays();
            m_env.pPhysicalWorld->TracePendingRays(2);
            m_env.pPhysicalWorld->TimeStep(0);
        }
        if ((nPauseMode == 0) && !(updateFlags & ESYSUPDATE_IGNORE_AI) && g_cvars.sys_ai && !bNoUpdate)
        {
            FRAME_PROFILER("SysUpdate:AI", this, PROFILE_SYSTEM);
            //////////////////////////////////////////////////////////////////////
            //update AI system
            if (m_env.pAISystem && !m_cvAIUpdate->GetIVal())
            {
                m_env.pAISystem->Update(gEnv->pTimer->GetFrameStartTime(), gEnv->pTimer->GetFrameTime());
            }
        }
    }
    pe_params_waterman pwm;
    pwm.posViewer = GetViewCamera().GetPosition();
    m_env.pPhysicalWorld->SetWaterManagerParams(&pwm);

    // Use UI timer for CryMovie, because it should not be affected by pausing game time
    const float fMovieFrameTime = m_Time.GetFrameTime(ITimer::ETIMER_UI);

    // Run movie system pre-update
    if (!bNoUpdate)
    {
        FRAME_PROFILER("SysUpdate:UpdateMovieSystem", this, PROFILE_SYSTEM);
        UpdateMovieSystem(updateFlags, fMovieFrameTime, true);
    }

    return !m_bQuit;
}

//////////////////////////////////////////////////////////////////////
bool CSystem::UpdatePostTickBus(int updateFlags, int nPauseMode)
{
    CTimeValue updateStart = gEnv->pTimer->GetAsyncTime();

#ifndef EXCLUDE_UPDATE_ON_CONSOLE
    if (nPauseMode != 1)
#endif //EXCLUDE_UPDATE_ON_CONSOLE
    {
        //////////////////////////////////////////////////////////////////////
        //update entity system
        if (m_env.pEntitySystem && !m_bNoUpdate && g_cvars.sys_entitysystem)
        {
            m_env.pEntitySystem->Update();
        }
    }

    // Run movie system post-update
    if (!m_bNoUpdate)
    {
        const float fMovieFrameTime = m_Time.GetFrameTime(ITimer::ETIMER_UI);
        FRAME_PROFILER("SysUpdate:UpdateMovieSystem", this, PROFILE_SYSTEM);
        UpdateMovieSystem(updateFlags, fMovieFrameTime, false);
    }

    //////////////////////////////////////////////////////////////////////
    //update process (3D engine)
    if (!(updateFlags & ESYSUPDATE_EDITOR) && !m_bNoUpdate)
    {
        FRAME_PROFILER("SysUpdate:Update3DEngine", this, PROFILE_SYSTEM);

        if (ITimeOfDay* pTOD = m_env.p3DEngine->GetTimeOfDay())
        {
            pTOD->Tick();
        }

        if (m_env.p3DEngine)
        {
            m_env.p3DEngine->Tick();  // clear per frame temp data
        }
        if (m_pProcess && (m_pProcess->GetFlags() & PROC_3DENGINE))
        {
            if ((nPauseMode != 1))
            {
                if (!IsEquivalent(m_ViewCamera.GetPosition(), Vec3(0, 0, 0), VEC_EPSILON))
                {
                    if (m_env.p3DEngine)
                    {
                        //                  m_env.p3DEngine->SetCamera(m_ViewCamera);
                        m_pProcess->Update();
                    }
                }
            }
        }
        else
        {
            if (m_pProcess)
            {
                m_pProcess->Update();
            }
        }
    }

    //////////////////////////////////////////////////////////////////////
    //update sound system part 2
    if (!g_cvars.sys_deferAudioUpdateOptim && !m_bNoUpdate)
    {
        FRAME_PROFILER("SysUpdate:UpdateAudioSystems", this, PROFILE_SYSTEM);
        UpdateAudioSystems();
    }
    else
    {
        m_bNeedDoWorkDuringOcclusionChecks = true;
    }

    //////////////////////////////////////////////////////////////////////
    // final network update
    if (m_env.pNetwork)
    {
        FRAME_PROFILER("SysUpdate - Network::SyncWithGame", this, PROFILE_SYSTEM);
        m_env.pNetwork->SyncWithGame(eNGS_FrameEnd);
    }

#ifdef DOWNLOAD_MANAGER
    if (m_pDownloadManager && !m_bNoUpdate)
    {
        FRAME_PROFILER("SysUpdate:DownloadManagerUpdate", this, PROFILE_SYSTEM);
        m_pDownloadManager->Update();
    }
#endif //DOWNLOAD_MANAGER
#if AZ_LEGACY_CRYSYSTEM_TRAIT_SIMULATE_TASK
    if (m_sys_SimulateTask->GetIVal() > 0)
    {
        // have a chance to win longest Pi calculation content
        int64 delay = m_sys_SimulateTask->GetIVal();
        int64 start = CryGetTicks();
        double a = 1.0, b = 1.0 / sqrt(2.0), t = 1.0 / 4.0, p = 1.0, an, bn, tn, pn, Pi = 0.0;
        while (CryGetTicks() - start < delay)
        {
            // do something
            an = (a + b) / 2.0;
            bn = sqrt(a * b);
            tn = t - p * (a - an) * (a - an);
            pn = 2 * p;

            a = an;
            b = bn;
            t = tn;
            p = pn;

            Pi = (a + b) * (a + b) / 4 / t;
        }
        //CryLog("Task calculate PI = %f ", Pi); // Thats funny , but it works :-)
    }
#endif //AZ_LEGACY_CRYSYSTEM_TRAIT_SIMULATE_TASK
       //Now update frame statistics
    CTimeValue cur_time = gEnv->pTimer->GetAsyncTime();

    CTimeValue a_second(g_cvars.sys_update_profile_time);
    std::vector< std::pair<CTimeValue, float> >::iterator it = m_updateTimes.begin();
    for (std::vector< std::pair<CTimeValue, float> >::iterator eit = m_updateTimes.end(); it != eit; ++it)
    {
        if ((cur_time - it->first) < a_second)
        {
            break;
        }
    }

    {
        if (it != m_updateTimes.begin())
        {
            m_updateTimes.erase(m_updateTimes.begin(), it);
        }

        float updateTime = (cur_time - updateStart).GetMilliSeconds();
        m_updateTimes.push_back(std::make_pair(cur_time, updateTime));
    }

    UpdateUpdateTimes();

    {
        FRAME_PROFILER("SysUpdate - SystemEventDispatcher::Update", this, PROFILE_SYSTEM);
        m_pSystemEventDispatcher->Update();
    }

    if (!gEnv->IsEditing() && m_eRuntimeState == ESYSTEM_EVENT_LEVEL_GAMEPLAY_START)
    {
        gEnv->pCryPak->DisableRuntimeFileAccess(true);
    }

    // CryAction has been moved to the optional CryLegacy Gem, so if it doesn't exist we need to do all this here
    if (!gEnv->pGame || !gEnv->pGame->GetIGameFramework())
    {
        if (GetIViewSystem())
        {
            GetIViewSystem()->Update(min(gEnv->pTimer->GetFrameTime(), 0.1f));
        }

        if (gEnv->pLyShine)
        {
            // Tell the UI system the size of the viewport we are rendering to - this drives the
            // canvas size for full screen UI canvases. It needs to be set before either pLyShine->Update or
            // pLyShine->Render are called. It must match the viewport size that the input system is using.
            AZ::Vector2 viewportSize;
            viewportSize.SetX(static_cast<float>(gEnv->pRenderer->GetOverlayWidth()));
            viewportSize.SetY(static_cast<float>(gEnv->pRenderer->GetOverlayHeight()));
            gEnv->pLyShine->SetViewportSize(viewportSize);

            bool isUiPaused = gEnv->pTimer->IsTimerPaused(ITimer::ETIMER_UI);
            if (!isUiPaused)
            {
                gEnv->pLyShine->Update(gEnv->pTimer->GetFrameTime(ITimer::ETIMER_UI));
            }
        }

        // Begin occlusion job after setting the correct camera.
        gEnv->p3DEngine->PrepareOcclusion(GetViewCamera());

        CrySystemNotificationBus::Broadcast(&CrySystemNotifications::OnPreRender);

        Render();

        gEnv->p3DEngine->EndOcclusion();

        RenderEnd();

        gEnv->p3DEngine->SyncProcessStreamingUpdate();

        if (NeedDoWorkDuringOcclusionChecks())
        {
            DoWorkDuringOcclusionChecks();
        }
    }

    return !m_bQuit;
}


bool CSystem::UpdateLoadtime()
{
    m_pPlatformOS->Tick(m_Time.GetRealFrameTime());
    return !m_bQuit;
}

void CSystem::DoWorkDuringOcclusionChecks()
{
    if (g_cvars.sys_deferAudioUpdateOptim && !m_bNoUpdate)
    {
        UpdateAudioSystems();
        m_bNeedDoWorkDuringOcclusionChecks = false;
    }
}

void CSystem::UpdateAudioSystems()
{
    CRYPROFILE_SCOPE_PROFILE_MARKER("Audio");
    AZ_TRACE_METHOD();
    FRAME_PROFILER_LEGACYONLY("SysUpdate:Audio", this, PROFILE_SYSTEM);
    Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::ExternalUpdate);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::GetUpdateStats(SSystemUpdateStats& stats)
{
    if (m_updateTimes.empty())
    {
        stats = SSystemUpdateStats();
    }
    else
    {
        stats.avgUpdateTime = 0;
        stats.maxUpdateTime = -FLT_MAX;
        stats.minUpdateTime = +FLT_MAX;
        for (std::vector< std::pair<CTimeValue, float> >::const_iterator it = m_updateTimes.begin(), eit = m_updateTimes.end(); it != eit; ++it)
        {
            const float t = it->second;
            stats.avgUpdateTime += t;
            stats.maxUpdateTime = max(stats.maxUpdateTime, t);
            stats.minUpdateTime = min(stats.minUpdateTime, t);
        }
        stats.avgUpdateTime /= m_updateTimes.size();
    }
}


//////////////////////////////////////////////////////////////////////////
void CSystem::UpdateMovieSystem(const int updateFlags, const float fFrameTime, const bool bPreUpdate)
{
    if (m_env.pMovieSystem && !(updateFlags & ESYSUPDATE_EDITOR) && g_cvars.sys_trackview)
    {
        float fMovieFrameTime = fFrameTime;

        if (fMovieFrameTime > g_cvars.sys_maxTimeStepForMovieSystem)
        {
            fMovieFrameTime = g_cvars.sys_maxTimeStepForMovieSystem;
        }

        if (bPreUpdate)
        {
            m_env.pMovieSystem->PreUpdate(fMovieFrameTime);
        }
        else
        {
            m_env.pMovieSystem->PostUpdate(fMovieFrameTime);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
// XML stuff
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CSystem::CreateXmlNode(const char* sNodeName, bool bReuseStrings)
{
    return new CXmlNode(sNodeName, bReuseStrings);
}

//////////////////////////////////////////////////////////////////////////
IXmlUtils* CSystem::GetXmlUtils()
{
    return m_pXMLUtils;
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CSystem::LoadXmlFromFile(const char* sFilename, bool bReuseStrings)
{
    LOADING_TIME_PROFILE_SECTION_ARGS(sFilename);

    return m_pXMLUtils->LoadXmlFromFile(sFilename, bReuseStrings);
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CSystem::LoadXmlFromBuffer(const char* buffer, size_t size, bool bReuseStrings, bool bSuppressWarnings)
{
    LOADING_TIME_PROFILE_SECTION
    return m_pXMLUtils->LoadXmlFromBuffer(buffer, size, bReuseStrings, bSuppressWarnings);
}

//////////////////////////////////////////////////////////////////////////
bool CSystem::CheckLogVerbosity(int verbosity)
{
    if (verbosity <= m_env.pLog->GetVerbosityLevel())
    {
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CSystem::Warning(EValidatorModule module, EValidatorSeverity severity, int flags, const char* file, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    WarningV(module, severity, flags, file, format, args);
    va_end(args);
}

//////////////////////////////////////////////////////////////////////////
int CSystem::ShowMessage(const char* text, const char* caption, unsigned int uType)
{
    if (m_pUserCallback)
    {
        return m_pUserCallback->ShowMessage(text, caption, uType);
    }
    return CryMessageBox(text, caption, uType);
}

inline const char* ValidatorModuleToString(EValidatorModule module)
{
    switch (module)
    {
    case VALIDATOR_MODULE_RENDERER:
        return "Renderer";
    case VALIDATOR_MODULE_3DENGINE:
        return "3DEngine";
    case VALIDATOR_MODULE_ASSETS:
        return "Assets";
    case VALIDATOR_MODULE_AI:
        return "AI";
    case VALIDATOR_MODULE_ANIMATION:
        return "Animation";
    case VALIDATOR_MODULE_ENTITYSYSTEM:
        return "EntitySystem";
    case VALIDATOR_MODULE_SCRIPTSYSTEM:
        return "Script";
    case VALIDATOR_MODULE_SYSTEM:
        return "System";
    case VALIDATOR_MODULE_AUDIO:
        return "Audio";
    case VALIDATOR_MODULE_GAME:
        return "Game";
    case VALIDATOR_MODULE_MOVIE:
        return "Movie";
    case VALIDATOR_MODULE_EDITOR:
        return "Editor";
    case VALIDATOR_MODULE_NETWORK:
        return "Network";
    case VALIDATOR_MODULE_PHYSICS:
        return "Physics";
    case VALIDATOR_MODULE_FLOWGRAPH:
        return "FlowGraph";
    case VALIDATOR_MODULE_ONLINE:
        return "Online";
    case VALIDATOR_MODULE_FEATURETESTS:
        return "FeatureTests";
    case VALIDATOR_MODULE_SHINE:
        return "UI";
    }
    return "";
}

//////////////////////////////////////////////////////////////////////////
void CSystem::WarningV(EValidatorModule module, EValidatorSeverity severity, int flags, const char* file, const char* format, va_list args)
{
    // Fran: No logging in a testing environment
    if (m_env.pLog == 0)
    {
        return;
    }

    const char* sModuleFilter = m_env.pLog->GetModuleFilter();
    if (sModuleFilter && *sModuleFilter != 0)
    {
        const char* sModule = ValidatorModuleToString(module);
        if (strlen(sModule) > 1 || CryStringUtils::stristr(sModule, sModuleFilter) == 0)
        {
            // Filter out warnings from other modules.
            return;
        }
    }

    bool bDbgBreak = false;
    if (severity == VALIDATOR_ERROR_DBGBRK)
    {
        bDbgBreak = true;
        severity = VALIDATOR_ERROR; // change it to a standard VALIDATOR_ERROR for simplicity in the rest of the system
    }

    IMiniLog::ELogType ltype = ILog::eComment;
    switch (severity)
    {
    case    VALIDATOR_ERROR:
        ltype = ILog::eError;
        break;
    case    VALIDATOR_WARNING:
        ltype = ILog::eWarning;
        break;
    case    VALIDATOR_COMMENT:
        ltype = ILog::eComment;
        break;
    default:
        break;
    }
    char szBuffer[MAX_WARNING_LENGTH];
    vsnprintf_s(szBuffer, sizeof(szBuffer), sizeof(szBuffer) - 1, format, args);

    if (file && *file)
    {
        CryFixedStringT<MAX_WARNING_LENGTH> fmt = szBuffer;
        fmt += " [File=";
        fmt += file;
        fmt += "]";

        m_env.pLog->LogWithType(ltype, flags | VALIDATOR_FLAG_SKIP_VALIDATOR, "%s", fmt.c_str());
    }
    else
    {
        m_env.pLog->LogWithType(ltype, flags | VALIDATOR_FLAG_SKIP_VALIDATOR, "%s", szBuffer);
    }

    //if(file)
    //m_env.pLog->LogWithType( ltype, "  ... caused by file '%s'",file);

    if (m_pValidator && (flags & VALIDATOR_FLAG_SKIP_VALIDATOR) == 0)
    {
        SValidatorRecord record;
        record.file = file;
        record.text = szBuffer;
        record.module = module;
        record.severity = severity;
        record.flags = flags;
        record.assetScope = m_env.pLog->GetAssetScopeString();
        m_pValidator->Report(record);
    }

    if (bDbgBreak && g_cvars.sys_error_debugbreak)
    {
        AZ::Debug::Trace::Break();
    }
}

//////////////////////////////////////////////////////////////////////////
void CSystem::GetLocalizedPath(const char* sLanguage, string& sLocalizedPath)
{
    // Omit the trailing slash!
    string sLocalizationFolder(string().assign(PathUtil::GetLocalizationFolder(), 0, PathUtil::GetLocalizationFolder().size() - 1));

    if (sLocalizationFolder.compareNoCase("Languages") != 0)
    {
        sLocalizedPath = sLocalizationFolder + "/" + sLanguage + "_xml.pak";
    }
    else
    {
        sLocalizedPath = string("Localized/") + sLanguage + "_xml.pak";
    }
}

//////////////////////////////////////////////////////////////////////////
void CSystem::GetLocalizedAudioPath(const char* sLanguage, string& sLocalizedPath)
{
    // Omit the trailing slash!
    string sLocalizationFolder(string().assign(PathUtil::GetLocalizationFolder(), 0, PathUtil::GetLocalizationFolder().size() - 1));

    if (sLocalizationFolder.compareNoCase("Languages") != 0)
    {
        sLocalizedPath = sLocalizationFolder + "/" + sLanguage + ".pak";
    }
    else
    {
        sLocalizedPath = string("Localized/") + sLanguage + ".pak";
    }
}

//////////////////////////////////////////////////////////////////////////
void CSystem::CloseLanguagePak(const char* sLanguage)
{
    string sLocalizedPath;
    GetLocalizedPath(sLanguage, sLocalizedPath);
    m_env.pCryPak->ClosePacks(sLocalizedPath);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::CloseLanguageAudioPak(const char* sLanguage)
{
    string sLocalizedPath;
    GetLocalizedAudioPath(sLanguage, sLocalizedPath);
    m_env.pCryPak->ClosePacks(sLocalizedPath);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::Relaunch(bool bRelaunch)
{
    if (m_sys_firstlaunch)
    {
        m_sys_firstlaunch->Set("0");
    }

    m_bRelaunch = bRelaunch;
    SaveConfiguration();
}

//////////////////////////////////////////////////////////////////////////
ICrySizer* CSystem::CreateSizer()
{
    return new CrySizerImpl;
}

//////////////////////////////////////////////////////////////////////////
uint32 CSystem::GetUsedMemory()
{
    return CryMemoryGetAllocatedSize();
}

//////////////////////////////////////////////////////////////////////////
ILocalizationManager* CSystem::GetLocalizationManager()
{
    return m_pLocalizationManager;
}

//////////////////////////////////////////////////////////////////////////
IThreadTaskManager* CSystem::GetIThreadTaskManager()
{
    return m_pThreadTaskManager;
}

//////////////////////////////////////////////////////////////////////////
IResourceManager* CSystem::GetIResourceManager()
{
    return m_pResourceManager;
}

//////////////////////////////////////////////////////////////////////////
void CSystem::debug_GetCallStackRaw(void** callstack, uint32& callstackLength)
{
    uint32 callstackCapacity = callstackLength;
    uint32 nNumStackFramesToSkip = 1;

    memset(callstack, 0, sizeof(void*) * callstackLength);

#if !defined(ANDROID)
    callstackLength = 0;
#endif

#if AZ_LEGACY_CRYSYSTEM_TRAIT_CAPTURESTACK
    if (callstackCapacity > 0x40)
    {
        callstackCapacity = 0x40;
    }
    callstackLength = RtlCaptureStackBackTrace(nNumStackFramesToSkip, callstackCapacity, callstack, NULL);
#elif defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEM_CPP_SECTION_7
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/System_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/System_cpp_provo.inl"
    #endif
#endif

    if (callstackLength > 0)
    {
        std::reverse(callstack, callstack + callstackLength);
    }
}

//////////////////////////////////////////////////////////////////////////
void CSystem::ApplicationTest(const char* szParam)
{
    assert(szParam);

    if (!m_pTestSystem)
    {
        m_pTestSystem = new CTestSystemLegacy;
    }

    m_pTestSystem->ApplicationTest(szParam);
}

void CSystem::ExecuteCommandLine(bool deferred)
{
    if (m_executedCommandLine)
    {
        return;
    }

    m_executedCommandLine = true;

    // auto detect system spec (overrides profile settings)
    if (m_pCmdLine->FindArg(eCLAT_Pre, "autodetect"))
    {
        AutoDetectSpec(false);
    }

    // execute command line arguments e.g. +g_gametype ASSAULT +map "testy"

    ICmdLine* pCmdLine = GetICmdLine();
    assert(pCmdLine);

    const int iCnt = pCmdLine->GetArgCount();
    for (int i = 0; i < iCnt; ++i)
    {
        const ICmdLineArg* pCmd = pCmdLine->GetArg(i);

        if (pCmd->GetType() == eCLAT_Post)
        {
            string sLine = pCmd->GetName();

#if defined(CVARS_WHITELIST)
            if (!GetCVarsWhiteList() || GetCVarsWhiteList()->IsWhiteListed(sLine, false))
#endif
            {
                if (pCmd->GetValue())
                {
                    sLine += string(" ") + pCmd->GetValue();
                }

                GetILog()->Log("Executing command from command line: \n%s\n", sLine.c_str()); // - the actual command might be executed much later (e.g. level load pause)
                GetIConsole()->ExecuteString(sLine.c_str(), false, deferred);
            }
#if defined(DEDICATED_SERVER)
#if defined(CVARS_WHITELIST)
            else
            {
                GetILog()->LogError("Failed to execute command: '%s' as it is not whitelisted\n", sLine.c_str());
            }
#endif
#endif
        }
    }

    //gEnv->pConsole->ExecuteString("sys_RestoreSpec test*"); // to get useful debugging information about current spec settings to the log file
}

void CSystem::DumpMemoryCoverage()
{
    m_MemoryFragmentationProfiler.DumpMemoryCoverage();
}

ITextModeConsole* CSystem::GetITextModeConsole()
{
    if (m_bDedicatedServer)
    {
        return m_pTextModeConsole;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
ESystemConfigSpec CSystem::GetConfigSpec(bool bClient)
{
    if (bClient)
    {
        if (m_sys_GraphicsQuality)
        {
            return (ESystemConfigSpec)m_sys_GraphicsQuality->GetIVal();
        }
        return CONFIG_VERYHIGH_SPEC; // highest spec.
    }
    else
    {
        return m_nServerConfigSpec;
    }
}

//////////////////////////////////////////////////////////////////////////
void CSystem::SetConfigSpec(ESystemConfigSpec spec, ESystemConfigPlatform platform, bool bClient)
{
    if (bClient)
    {
        if (m_sys_GraphicsQuality)
        {
            SetConfigPlatform(platform);
            m_sys_GraphicsQuality->Set(static_cast<int>(spec));
        }
    }
    else
    {
        m_nServerConfigSpec = spec;
    }
}

//////////////////////////////////////////////////////////////////////////
ESystemConfigSpec CSystem::GetMaxConfigSpec() const
{
    return m_nMaxConfigSpec;
}

//////////////////////////////////////////////////////////////////////////
void CSystem::SetConfigPlatform(const ESystemConfigPlatform platform)
{
    m_ConfigPlatform = platform;
}

//////////////////////////////////////////////////////////////////////////
ESystemConfigPlatform CSystem::GetConfigPlatform() const
{
    return m_ConfigPlatform;
}

//////////////////////////////////////////////////////////////////////////
CPNoise3* CSystem::GetNoiseGen()
{
    static CPNoise3 m_pNoiseGen;
    return &m_pNoiseGen;
}

//////////////////////////////////////////////////////////////////////////
void CProfilingSystem::VTuneResume()
{
#ifdef PROFILE_WITH_VTUNE
    if (VTResume)
    {
        CryLogAlways("VTune Resume");
        VTResume();
    }
#endif
}

//////////////////////////////////////////////////////////////////////////
void CProfilingSystem::VTunePause()
{
#ifdef PROFILE_WITH_VTUNE
    if (VTPause)
    {
        VTPause();
        CryLogAlways("VTune Pause");
    }
#endif
}

//////////////////////////////////////////////////////////////////////////
sUpdateTimes& CSystem::GetCurrentUpdateTimeStats()
{
    return m_UpdateTimes[m_UpdateTimesIdx];
}

//////////////////////////////////////////////////////////////////////////
const sUpdateTimes* CSystem::GetUpdateTimeStats(uint32& index, uint32& num)
{
    index = m_UpdateTimesIdx;
    num = NUM_UPDATE_TIMES;
    return m_UpdateTimes;
}

void CSystem::UpdateUpdateTimes()
{
    sUpdateTimes& sample = m_UpdateTimes[m_UpdateTimesIdx];
    if (m_PhysThread)
    {
        static uint64 lastPhysTime = 0U;
        static uint64 lastMainTime = 0U;
        static uint64 lastYields = 0U;
        static uint64 lastPhysWait = 0U;
        uint64 physTime = 0, mainTime = 0;
        uint32 yields = 0;
        physTime = ((CPhysicsThreadTask*)m_PhysThread)->LastStepTaken();
        mainTime = CryGetTicks() - lastMainTime;
        lastMainTime = mainTime;
        lastPhysWait = ((CPhysicsThreadTask*)m_PhysThread)->LastWaitTime();
        sample.PhysStepTime = physTime;
        sample.SysUpdateTime = mainTime;
        sample.PhysYields = yields;
        sample.physWaitTime = lastPhysWait;
    }
    ++m_UpdateTimesIdx;
    if (m_UpdateTimesIdx >= NUM_UPDATE_TIMES)
    {
        m_UpdateTimesIdx = 0;
    }
}

IPhysicsDebugRenderer* CSystem::GetIPhysicsDebugRenderer()
{
    return m_pPhysRenderer;
}

IPhysRenderer* CSystem::GetIPhysRenderer()
{
    return m_pPhysRenderer;
}

#ifndef _RELEASE
void CSystem::GetCheckpointData(ICheckpointData& data)
{
    data.m_totalLoads = m_checkpointLoadCount;
    data.m_loadOrigin = m_loadOrigin;
}

void CSystem::IncreaseCheckpointLoadCount()
{
    if (!m_hasJustResumed)
    {
        ++m_checkpointLoadCount;
    }

    m_hasJustResumed = false;
}

void CSystem::SetLoadOrigin(LevelLoadOrigin origin)
{
    switch (origin)
    {
    case eLLO_NewLevel: // Intentional fall through
    case eLLO_Level2Level:
        m_expectingMapCommand = true;
        break;

    case eLLO_Resumed:
        m_hasJustResumed = true;
        break;

    case eLLO_MapCmd:
        if (m_expectingMapCommand)
        {
            // We knew a map command was coming, so don't process this.
            m_expectingMapCommand = false;
            return;
        }
        break;
    }

    m_loadOrigin = origin;
    m_checkpointLoadCount = 0;
}
#endif

bool CSystem::SteamInit()
{
#if USE_STEAM
    if (m_bIsSteamInitialized)
    {
        return true;
    }

    AZStd::string_view binFolderName;
    AzFramework::ApplicationRequests::Bus::BroadcastResult(binFolderName, &AzFramework::ApplicationRequests::GetBinSubfolder);

    ////////////////////////////////////////////////////////////////////////////
    // ** DEVELOPMENT ONLY ** - creates the appropriate steam_appid.txt file needed to call SteamAPI_Init()
#if !defined(RELEASE)
#if defined(WIN64)
    AZStd::string appidPath = AZStd::string::format("%s/steam_appid.txt", binFolderName.data());
    azfopen(&pSteamAppID, appidPath.c_str(), "wt");
#else
#if defined(WIN32)
    FILE* pSteamAppID = nullptr;
    azfopen(&pSteamAppID, "Bin32/steam_appid.txt", "wt");
#endif // defined(WIN32)
#endif // defined(WIN64)
    fprintf(pSteamAppID, "%d", g_cvars.sys_steamAppId);
    fclose(pSteamAppID);
#endif // !defined(RELEASE)
       // ** END DEVELOPMENT ONLY **
       ////////////////////////////////////////////////////////////////////////////

    if (!SteamAPI_Init())
    {
        CryLog("[STEAM] SteamApi_Init failed");
        return false;
    }

    ////////////////////////////////////////////////////////////////////////////
    // ** DEVELOPMENT ONLY ** - deletes the appropriate steam_appid.txt file as it's no longer needed
#if !defined(RELEASE)
#if defined(WIN64)
    remove(appidPath.c_str());
#else
#if defined(WIN32)
    remove("Bin32/steam_appid.txt");
#endif // defined(WIN32)
#endif // defined(WIN64)
#endif // !defined(RELEASE)
       // ** END DEVELOPMENT ONLY **
       ////////////////////////////////////////////////////////////////////////////

    m_bIsSteamInitialized = true;
    return true;
#else
    return false;
#endif
}

//////////////////////////////////////////////////////////////////////
void CSystem::OnLanguageCVarChanged(ICVar* language)
{
    if (language && language->GetType() == CVAR_STRING)
    {
        CSystem* pSys = static_cast<CSystem*>(gEnv->pSystem);
        if (pSys && pSys->GetLocalizationManager())
        {
            const char* lang = language->GetString();

            pSys->CloseLanguagePak(pSys->GetLocalizationManager()->GetLanguage());
            pSys->OpenLanguagePak(lang);
            pSys->GetLocalizationManager()->SetLanguage(lang);
            pSys->GetLocalizationManager()->ReloadData();

            if (gEnv->pCryFont)
            {
                gEnv->pCryFont->OnLanguageChanged();
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////
void CSystem::OnLanguageAudioCVarChanged(ICVar* language)
{
    static Audio::SAudioRequest oLanguageRequest;
    static Audio::SAudioManagerRequestData<Audio::eAMRT_CHANGE_LANGUAGE> oLanguageRequestData;

    if (language && (language->GetType() == CVAR_STRING))
    {
        oLanguageRequest.pData = &oLanguageRequestData;
        oLanguageRequest.nFlags = Audio::eARF_PRIORITY_HIGH;
        Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::PushRequest, oLanguageRequest);
    }
}

//////////////////////////////////////////////////////////////////////////
void CSystem::OnLocalizationFolderCVarChanged(ICVar* const pLocalizationFolder)
{
    if (pLocalizationFolder && pLocalizationFolder->GetType() == CVAR_STRING)
    {
        CSystem* const pSystem = static_cast<CSystem* const>(gEnv->pSystem);

        if (pSystem != NULL && gEnv->pCryPak != NULL)
        {
            CLocalizedStringsManager* const pLocalizationManager = static_cast<CLocalizedStringsManager* const>(pSystem->GetLocalizationManager());

            if (pLocalizationManager)
            {
                // Get what is currently loaded
                CLocalizedStringsManager::TLocalizationTagVec tagVec;
                pLocalizationManager->GetLoadedTags(tagVec);

                // Release the old localization data.
                CLocalizedStringsManager::TLocalizationTagVec::const_iterator end = tagVec.end();
                for (CLocalizedStringsManager::TLocalizationTagVec::const_iterator it = tagVec.begin(); it != end; ++it)
                {
                    pLocalizationManager->ReleaseLocalizationDataByTag(it->c_str());
                }

                // Close the paks situated in the previous localization folder.
                pSystem->CloseLanguagePak(pLocalizationManager->GetLanguage());
                pSystem->CloseLanguageAudioPak(pSystem->m_currentLanguageAudio.c_str());

                // Set the new localization folder.
                gEnv->pCryPak->SetLocalizationFolder(pLocalizationFolder->GetString());

                // Now open the paks situated in the new localization folder.
                pSystem->OpenLanguagePak(pLocalizationManager->GetLanguage());
                pSystem->OpenLanguageAudioPak(pSystem->m_currentLanguageAudio.c_str());

                // And load the new data.
                for (CLocalizedStringsManager::TLocalizationTagVec::const_iterator it = tagVec.begin(); it != end; ++it)
                {
                    pLocalizationManager->LoadLocalizationDataByTag(it->c_str());
                }
            }
        }
    }
}

void CSystem::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
    switch (event)
    {
    case ESYSTEM_EVENT_LEVEL_LOAD_START_LOADINGSCREEN:
    case ESYSTEM_EVENT_LEVEL_UNLOAD:
        gEnv->pCryPak->DisableRuntimeFileAccess(false);
    case ESYSTEM_EVENT_LEVEL_GAMEPLAY_START:
        m_eRuntimeState = event;
        break;
    }
}

ESystemGlobalState CSystem::GetSystemGlobalState(void)
{
    return m_systemGlobalState;
}

const char* CSystem::GetSystemGlobalStateName(const ESystemGlobalState systemGlobalState)
{
    static const char* const s_systemGlobalStateNames[] = {
        "UNKNOWN",                  // ESYSTEM_GLOBAL_STATE_UNKNOWN,
        "INIT",                     // ESYSTEM_GLOBAL_STATE_INIT,
        "RUNNING",                  // ESYSTEM_GLOBAL_STATE_RUNNING,
        "LEVEL_LOAD_PREPARE",       // ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_START_PREPARE,
        "LEVEL_LOAD_START",         // ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_START,
        "LEVEL_LOAD_MATERIALS",     // ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_START_MATERIALS,
        "LEVEL_LOAD_OBJECTS",       // ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_START_OBJECTS,
        "LEVEL_LOAD_CHARACTERS",    // ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_START_CHARACTERS,
        "LEVEL_LOAD_STATIC_WORLD",  // ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_START_STATIC_WORLD,
        "LEVEL_LOAD_ENTITIES",      // ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_START_ENTITIES,
        "LEVEL_LOAD_PRECACHE",      // ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_START_PRECACHE,
        "LEVEL_LOAD_TEXTURES",      // ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_START_TEXTURES,
        "LEVEL_LOAD_END",           // ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_END,
        "LEVEL_LOAD_COMPLETE"       // ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_COMPLETE
    };
    const size_t numElements = sizeof(s_systemGlobalStateNames) / sizeof(s_systemGlobalStateNames[0]);
    const size_t index = (size_t)systemGlobalState;
    if (index >= numElements)
    {
        return "INVALID INDEX";
    }
    return s_systemGlobalStateNames[index];
}

void CSystem::SetSystemGlobalState(const ESystemGlobalState systemGlobalState)
{
    static CTimeValue s_startTime = CTimeValue();
    if (systemGlobalState != m_systemGlobalState)
    {
        if (gEnv && gEnv->pTimer)
        {
            const CTimeValue endTime = gEnv->pTimer->GetAsyncTime();
            const float numSeconds = endTime.GetDifferenceInSeconds(s_startTime);
            CryLog("SetGlobalState %d->%d '%s'->'%s' %3.1f seconds",
                m_systemGlobalState, systemGlobalState,
                CSystem::GetSystemGlobalStateName(m_systemGlobalState), CSystem::GetSystemGlobalStateName(systemGlobalState),
                numSeconds);
            s_startTime = gEnv->pTimer->GetAsyncTime();
        }
    }
    m_systemGlobalState = systemGlobalState;

#if AZ_LOADSCREENCOMPONENT_ENABLED
    if (m_systemGlobalState == ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_COMPLETE)
    {
        EBUS_EVENT(LoadScreenBus, Stop);
    }
#endif // if AZ_LOADSCREENCOMPONENT_ENABLED
}

//////////////////////////////////////////////////////////////////////////
void* CSystem::GetRootWindowMessageHandler()
{
#if defined(WIN32)
    return &WndProc;
#else
    CRY_ASSERT(false && "This platform does not support window message handlers");
    return NULL;
#endif
}

//////////////////////////////////////////////////////////////////////////
void CSystem::RegisterWindowMessageHandler(IWindowMessageHandler* pHandler)
{
#if AZ_LEGACY_CRYSYSTEM_TRAIT_USE_MESSAGE_HANDLER
    assert(pHandler && !stl::find(m_windowMessageHandlers, pHandler) && "This IWindowMessageHandler is already registered");
    m_windowMessageHandlers.push_back(pHandler);
#else
    CRY_ASSERT(false && "This platform does not support window message handlers");
#endif
}

//////////////////////////////////////////////////////////////////////////
void CSystem::UnregisterWindowMessageHandler(IWindowMessageHandler* pHandler)
{
#if AZ_LEGACY_CRYSYSTEM_TRAIT_USE_MESSAGE_HANDLER
    bool bRemoved = stl::find_and_erase(m_windowMessageHandlers, pHandler);
    assert(pHandler && bRemoved && "This IWindowMessageHandler was not registered");
#else
    CRY_ASSERT(false && "This platform does not support window message handlers");
#endif
}

//////////////////////////////////////////////////////////////////////////
#if defined(WIN32)
bool CSystem::HandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
    static bool sbInSizingModalLoop;
    int x = LOWORD(lParam);
    int y = HIWORD(lParam);
    *pResult = 0;
    switch (uMsg)
    {
    // System event translation
    case WM_CLOSE:
        /* 
            Trigger CSystem to call Quit() the next time
            it calls Update(). HandleMessages can get messages
            pumped to it from SyncMainWithRender which would 
            be called recurively by Quit(). Doing so would
            cause the render thread to deadlock and the main
            thread to spin in SRenderThread::WaitFlushFinishedCond.
        */
        m_bQuit = true;
        return false;
    case WM_MOVE:
        GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_MOVE, x, y);
        return false;
    case WM_SIZE:
        GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_RESIZE, x, y);
        switch (wParam)
        {
        case SIZE_MINIMIZED:
            EBUS_EVENT(AzFramework::WindowsLifecycleEvents::Bus, OnMinimized);
            break;
        case SIZE_MAXIMIZED:
            EBUS_EVENT(AzFramework::WindowsLifecycleEvents::Bus, OnMaximized);
            break;
        case SIZE_RESTORED:
            EBUS_EVENT(AzFramework::WindowsLifecycleEvents::Bus, OnRestored);
            break;
        }
        return false;
    case WM_WINDOWPOSCHANGED:
        GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_POS_CHANGED, 1, 0);
        return false;
    case WM_STYLECHANGED:
        GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_STYLE_CHANGED, 1, 0);
        return false;
    case WM_ACTIVATE:
        // Pass HIWORD(wParam) as well to indicate whether this window is minimized or not
        // HIWORD(wParam) != 0 is minimized, HIWORD(wParam) == 0 is not minimized
        GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_ACTIVATE, LOWORD(wParam) != WA_INACTIVE, HIWORD(wParam));
        return true;
    case WM_SETFOCUS:
        EBUS_EVENT(AzFramework::WindowsLifecycleEvents::Bus, OnSetFocus);
        GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_CHANGE_FOCUS, 1, 0);
        return false;
    case WM_KILLFOCUS:
        EBUS_EVENT(AzFramework::WindowsLifecycleEvents::Bus, OnKillFocus);
        GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_CHANGE_FOCUS, 0, 0);
        return false;
    case WM_INPUTLANGCHANGE:
        GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_LANGUAGE_CHANGE, wParam, lParam);
        return false;

    case WM_SYSCOMMAND:
        if ((wParam & 0xFFF0) == SC_SCREENSAVE)
        {
            // Check if screen saver is allowed
            IConsole* const pConsole = gEnv->pConsole;
            const ICVar* const pVar = pConsole ? pConsole->GetCVar("sys_screensaver_allowed") : 0;
            return pVar && pVar->GetIVal() == 0;
        }
        return false;

    // Mouse activation
    case WM_MOUSEACTIVATE:
        *pResult = MA_ACTIVATEANDEAT;
        return true;

    // Hardware mouse counters
    case WM_ENTERSIZEMOVE:
        sbInSizingModalLoop = true;
    // Fall through intended
    case WM_ENTERMENULOOP:
    {
        UiCursorBus::Broadcast(&UiCursorInterface::IncrementVisibleCounter);
        return true;
    }
    case WM_CAPTURECHANGED:
    // If WM_CAPTURECHANGED is received after WM_ENTERSIZEMOVE (ie, moving/resizing begins).
    // but no matching WM_EXITSIZEMOVE is received (this can happen if the window is not actually moved).
    // we still need to decrement the hardware mouse counter that was incremented when WM_ENTERSIZEMOVE was seen.
    // So in this case, we effectively treat WM_CAPTURECHANGED as if it was the WM_EXITSIZEMOVE message.
    // This behavior has only been reproduced the window is deactivated during the modal loop (ie, breakpoint triggered and focus moves to VS).
    case WM_EXITSIZEMOVE:
        if (!sbInSizingModalLoop)
        {
            return false;
        }
        sbInSizingModalLoop = false;
    // Fall through intended
    case WM_EXITMENULOOP:
    {
        UiCursorBus::Broadcast(&UiCursorInterface::DecrementVisibleCounter);
        return (uMsg != WM_CAPTURECHANGED);
    }
    case WM_SYSKEYUP:
    case WM_SYSKEYDOWN:
    {
        const bool bAlt = (lParam & (1 << 29)) != 0;
        if (bAlt && wParam == VK_F4)
        {
            return false;     // Pass though ALT+F4
        }
        // Prevent game from entering menu loop!  Editor does allow menu loop.
        return !m_bEditor;
    }
    case WM_INPUT:
    {
        UINT rawInputSize;
        const UINT rawInputHeaderSize = sizeof(RAWINPUTHEADER);
        GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &rawInputSize, rawInputHeaderSize);

        AZStd::array<BYTE, sizeof(RAWINPUT)> rawInputBytesArray;
        LPBYTE rawInputBytes = rawInputBytesArray.data();

        const UINT bytesCopied = GetRawInputData((HRAWINPUT)lParam, RID_INPUT, rawInputBytes, &rawInputSize, rawInputHeaderSize);
        CRY_ASSERT(bytesCopied == rawInputSize);

        RAWINPUT* rawInput = (RAWINPUT*)rawInputBytes;
        CRY_ASSERT(rawInput);

        EBUS_EVENT(AzFramework::RawInputNotificationBusWin, OnRawInputEvent, *rawInput);

        return false;
    }
    case WM_DEVICECHANGE:
    {
        if (wParam == 0x0007) // DBT_DEVNODES_CHANGED
        {
            EBUS_EVENT(AzFramework::RawInputNotificationBusWin, OnRawInputDeviceChangeEvent);
        }
        return true;
    }
    case WM_CHAR:
    {
        const unsigned short codeUnitUTF16 = static_cast<unsigned short>(wParam);
        EBUS_EVENT(AzFramework::RawInputNotificationBusWin, OnRawInputCodeUnitUTF16Event, codeUnitUTF16);
        return true;
    }

    // Any other event doesn't interest us
    default:
        return false;
    }

    return true;
}

#endif

std::shared_ptr<AZ::IO::FileIOBase> CSystem::CreateLocalFileIO()
{
    return std::make_shared<AZ::IO::LocalFileIO>();
}

const char* CSystem::GetAssetsPlatform() const
{
    return m_assetPlatform.c_str();
}

IViewSystem* CSystem::GetIViewSystem()
{
    return (m_env.pGame && m_env.pGame->GetIGameFramework()) ? m_env.pGame->GetIGameFramework()->GetIViewSystem() : m_pViewSystem;
}

ILevelSystem* CSystem::GetILevelSystem()
{
    return (m_env.pGame && m_env.pGame->GetIGameFramework()) ? m_env.pGame->GetIGameFramework()->GetILevelSystem() : m_pLevelSystem;
}

#undef EXCLUDE_UPDATE_ON_CONSOLE

