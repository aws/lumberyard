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

#include <platform_impl.h>

#include <AndroidSpecific.h>
#include <IConsole.h>
#include <IEditorGame.h>
#include <IGameStartup.h>
#include <ITimer.h>
#include <LumberyardLauncher.h>
#include <ParseEngineConfig.h>

#include <AzCore/Android/AndroidEnv.h>
#include <AzCore/Android/Utils.h>

#include <AzFramework/API/ApplicationAPI_android.h>
#include <AzFramework/Input/Buses/Notifications/RawInputNotificationBus_android.h>
#include <AzGameFramework/Application/GameApplication.h>

#include <android/asset_manager_jni.h>
#include <android/log.h>
#include <android/native_activity.h>
#include <android/native_window.h>
#include <android_native_app_glue.h>

#include <sys/resource.h>
#include <sys/types.h>

#if !defined(_RELEASE) || defined(RELEASE_LOGGING)
    #define ENABLE_LOGGING
#endif

#if defined(AZ_MONOLITHIC_BUILD)
    #include "Common_TypeInfo.h"

    STRUCT_INFO_T_INSTANTIATE(Vec2_tpl, <float>)
    STRUCT_INFO_T_INSTANTIATE(Vec2_tpl, <int>)
    STRUCT_INFO_T_INSTANTIATE(Vec4_tpl, <short>)
    STRUCT_INFO_T_INSTANTIATE(Vec3_tpl, <int>)
    STRUCT_INFO_T_INSTANTIATE(Vec3_tpl, <float>)
    STRUCT_INFO_T_INSTANTIATE(Ang3_tpl, <float>)
    STRUCT_INFO_T_INSTANTIATE(Quat_tpl, <float>)
    STRUCT_INFO_T_INSTANTIATE(QuatT_tpl, <float>)
    STRUCT_INFO_T_INSTANTIATE(Plane_tpl, <float>)
    STRUCT_INFO_T_INSTANTIATE(Matrix33_tpl, <float>)
    STRUCT_INFO_T_INSTANTIATE(Color_tpl, <float>)
    STRUCT_INFO_T_INSTANTIATE(Color_tpl, <uint8>)

    // from the game library
    extern "C" DLL_IMPORT IGameStartup * CreateGameStartup(); // from the game library
    extern "C" void CreateStaticModules(AZStd::vector<AZ::Module*>&);
#endif //  defined(AZ_MONOLITHIC_BUILD)


#if defined(ENABLE_LOGGING)
    #define LOG_TAG "LMBR"
    #define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
    #define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))

    struct COutputPrintSink
        : public IOutputPrintSink
    {
        virtual void Print(const char* inszText)
        {
            LOGI("%s", inszText);
        }
    };

    COutputPrintSink g_androidPrintSink;
#else
    #define LOGI(...)
    #define LOGE(...)
#endif // !defined(_RELEASE)


#define MAIN_EXIT_FAILURE(_appState, ...) \
    LOGE("****************************************************************"); \
    LOGE("STARTUP FAILURE - EXITING"); \
    LOGE("REASON:"); \
    LOGE(__VA_ARGS__); \
    LOGE("****************************************************************"); \
    _appState->userData = nullptr; \
    ANativeActivity_finish(_appState->activity); \
    while (_appState->destroyRequested == 0) { \
        g_eventDispatcher.PumpAllEvents(); \
    } \
    return;


namespace
{
    class NativeEventDispatcher
        : public AzFramework::AndroidEventDispatcher
    {
    public:
        NativeEventDispatcher()
            : m_appState(nullptr)
        {
        }

        ~NativeEventDispatcher() = default;

        void PumpAllEvents() override
        {
            bool continueRunning = true;
            while (continueRunning) 
            {
                continueRunning = PumpEvents(&ALooper_pollAll);
            }
        }

        void PumpEventLoopOnce() override
        {
            PumpEvents(&ALooper_pollOnce);
        }

        void SetAppState(android_app* appState)
        {
            m_appState = appState;
        }

    private:
        // signature of ALooper_pollOnce and ALooper_pollAll -> int timeoutMillis, int* outFd, int* outEvents, void** outData
        typedef int (*EventPumpFunc)(int, int*, int*, void**); 

        bool PumpEvents(EventPumpFunc looperFunc)
        {
            if (!m_appState)
            {
                return false;
            }

            int events = 0;
            android_poll_source* source = nullptr;
            const AZ::Android::AndroidEnv* androidEnv = AZ::Android::AndroidEnv::Get();

            // when timeout is negative, the function will block until an event is received
            const int result = looperFunc(androidEnv->IsRunning() ? 0 : -1, nullptr, &events, reinterpret_cast<void**>(&source));

            // the value returned from the looper poll func is either:
            // 1. the identifier associated with the event source (>= 0) and has event data that needs to be processed manually
            // 2. an ALOOPER_POLL_* enum (< 0) indicating there is no data to be processed due to error or callback(s) registered 
            //    with the event source were called
            const bool validIdentifier = (result >= 0);
            if (validIdentifier && source)
            {
                source->process(m_appState, source);
            }

            const bool destroyRequested = (m_appState->destroyRequested != 0);
            if (destroyRequested)
            {
                AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::ExitMainLoop);
            }

            return (validIdentifier && !destroyRequested);
        }

        android_app* m_appState;
    };


    NativeEventDispatcher g_eventDispatcher;
    bool g_windowInitialized = false;


    //////////////////////////////////////////////////////////////////////////
    bool IncreaseResourceToMaxLimit(int resource)
    {
        const rlim_t maxLimit = RLIM_INFINITY;

        struct rlimit limit;
        if (getrlimit(resource, &limit) != 0)
        {
            LOGE("[ERROR] Failed to get limit for resource %d.  Error: %s", resource, strerror(errno));
            return false;
        }

        if (limit.rlim_max != maxLimit)
        {
            limit.rlim_max = maxLimit;
            if (setrlimit(resource, &limit) != 0)
            {
                LOGE("[ERROR] Failed to update resource limit to RLIM_INFINITY for resource %d.  Error: %s", resource, strerror(errno));
                return false;
            }
        }

        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    void RegisterSignalHandler(int sig, void (*handler)(int))
    {
        struct sigaction action;
        sigaction(sig, NULL, &action);
        if (action.sa_handler == SIG_DFL && action.sa_sigaction == (void*)SIG_DFL)
        {
            action.sa_handler = handler;
            sigaction(sig, &action, NULL);
        }
    }

    // ----
    // System handlers
    // ----

    //////////////////////////////////////////////////////////////////////////
    void HandleSignal(int sig)
    {
        LOGE("[ERROR] Signal (%d) was just raised", sig);
        signal(sig, HandleSignal);
    }

    //////////////////////////////////////////////////////////////////////////
    // this callback is triggered on the same thread the events are pumped
    int32_t HandleInputEvents(android_app* app, AInputEvent* event)
    {
        AzFramework::RawInputNotificationBusAndroid::Broadcast(&AzFramework::RawInputNotificationsAndroid::OnRawInputEvent, event);
        return 0;
    }

    //////////////////////////////////////////////////////////////////////////
    // this callback is triggered on the same thread the events are pumped
    void HandleApplicationLifecycleEvents(android_app* appState, int32_t command)
    {
    #if !defined(_RELEASE)
        const char* commandNames[] = {
            "APP_CMD_INPUT_CHANGED",
            "APP_CMD_INIT_WINDOW",
            "APP_CMD_TERM_WINDOW",
            "APP_CMD_WINDOW_RESIZED",
            "APP_CMD_WINDOW_REDRAW_NEEDED",
            "APP_CMD_CONTENT_RECT_CHANGED",
            "APP_CMD_GAINED_FOCUS",
            "APP_CMD_LOST_FOCUS",
            "APP_CMD_CONFIG_CHANGED",
            "APP_CMD_LOW_MEMORY",
            "APP_CMD_START",
            "APP_CMD_RESUME",
            "APP_CMD_SAVE_STATE",
            "APP_CMD_PAUSE",
            "APP_CMD_STOP",
            "APP_CMD_DESTROY",
        };
        LOGI("Engine command received: %s", commandNames[command]);
    #endif

        AZ::Android::AndroidEnv* androidEnv = static_cast<AZ::Android::AndroidEnv*>(appState->userData);
        if (!androidEnv)
        {
            return;
        }

        switch (command)
        {
            case APP_CMD_GAINED_FOCUS:
            {
                EBUS_EVENT(AzFramework::AndroidLifecycleEvents::Bus, OnGainedFocus);
            }
            break;

            case APP_CMD_LOST_FOCUS:
            {
                EBUS_EVENT(AzFramework::AndroidLifecycleEvents::Bus, OnLostFocus);
            }
            break;

            case APP_CMD_PAUSE:
            {
                EBUS_EVENT(AzFramework::AndroidLifecycleEvents::Bus, OnPause);
                androidEnv->SetIsRunning(false);
            }
            break;

            case APP_CMD_RESUME:
            {
                androidEnv->SetIsRunning(true);
                EBUS_EVENT(AzFramework::AndroidLifecycleEvents::Bus, OnResume);
            }
            break;

            case APP_CMD_DESTROY:
            {
                EBUS_EVENT(AzFramework::AndroidLifecycleEvents::Bus, OnDestroy);
            }
            break;

            case APP_CMD_INIT_WINDOW:
            {
                g_windowInitialized = true;
                androidEnv->SetWindow(appState->window);

                EBUS_EVENT(AzFramework::AndroidLifecycleEvents::Bus, OnWindowInit);
            }
            break;

            case APP_CMD_TERM_WINDOW:
            {
                EBUS_EVENT(AzFramework::AndroidLifecycleEvents::Bus, OnWindowDestroy);

                androidEnv->SetWindow(nullptr);
            }
            break;

            case APP_CMD_LOW_MEMORY:
            {
                EBUS_EVENT(AzFramework::AndroidLifecycleEvents::Bus, OnLowMemory);
            }
            break;

            case APP_CMD_CONFIG_CHANGED:
            {
                androidEnv->UpdateConfiguration();
            }
            break;

            case APP_CMD_WINDOW_REDRAW_NEEDED:
            {
                EBUS_EVENT(AzFramework::AndroidLifecycleEvents::Bus, OnWindowRedrawNeeded);
            }
            break;
        }
    }

    void OnWindowRedrawNeeded(ANativeActivity* activity, ANativeWindow* rect)
    {
        android_app* app = static_cast<android_app*>(activity->instance);
        int8_t cmd = APP_CMD_WINDOW_REDRAW_NEEDED;
        if (write(app->msgwrite, &cmd, sizeof(cmd)) != sizeof(cmd))
        {
            LOGE("Failure writing android_app cmd: %s\n", strerror(errno));
        }
    }
}

#ifdef AZ_RUN_ANDROID_LAUNCHER_TESTS

struct SimulatedTickBusInterface
    : public AZ::EBusTraits
{
    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
    typedef size_t BusIdType;
    typedef AZStd::mutex EventQueueMutexType;
    static const bool EnableEventQueue = true;

    virtual void OnTick() = 0;
    virtual void OnPayload(size_t payload) = 0;
};

using SimulatedTickBus = AZ::EBus<SimulatedTickBusInterface>;

struct SimulatedTickBusHandler
    : public SimulatedTickBus::Handler
{
    void OnTick() override {}
    void OnPayload(size_t) override {}
};

void TestEBuses()
{
    AZ::AllocatorInstance<AZ::SystemAllocator>::Create();

    {
        const size_t numLoops = 10000;
        auto mainLoop = [numLoops]()
        {
            size_t loops = 0;
            while (loops++ < numLoops)
            {
                SimulatedTickBus::ExecuteQueuedEvents();
                SimulatedTickBus::Event(0, &SimulatedTickBus::Events::OnTick);
            }
        };

        auto workerLoop = [numLoops]()
        {
            size_t loops = 0;
            while (loops++ < numLoops)
            {
                SimulatedTickBus::QueueEvent(0, &SimulatedTickBus::Events::OnPayload, loops);
            }
        };

        SimulatedTickBusHandler handler;
        handler.BusConnect(0);

        AZStd::thread mainThread(mainLoop);
        AZStd::thread workerThread(workerLoop);

        mainThread.join();
        workerThread.join();
    }

    AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
}

void RunAndroidLauncherTests()
{
    TestEBuses();
}
#endif


////////////////////////////////////////////////////////////////
// This is the main entry point of a native application that is using android_native_app_glue.
// It runs in its own thread, with its own event loop for receiving input events and doing other things.
void android_main(android_app* appState)
{
    // Adding a start up banner so you can see when the game is starting up in amongst the logcat spam
    LOGI("****************************************************************");
    LOGI("*            Amazon Lumberyard - Launching Game....            *");
    LOGI("****************************************************************");

    if (    !IncreaseResourceToMaxLimit(RLIMIT_CORE)
        ||  !IncreaseResourceToMaxLimit(RLIMIT_STACK))
    {
        MAIN_EXIT_FAILURE(appState, "A resource limit was unable to be updated, see logs above for more details");
    }

    // register our signal handlers
    RegisterSignalHandler(SIGINT, HandleSignal);
    RegisterSignalHandler(SIGTERM, HandleSignal);

    // setup the system command handler
    appState->onAppCmd = HandleApplicationLifecycleEvents;
    appState->onInputEvent = HandleInputEvents;
    g_eventDispatcher.SetAppState(appState);

    // This callback will notify us when the orientation of the device changes.
    // While Android does have an onNativeWindowResized callback, it is never called in android_native_app_glue when the window size changes.
    // The onNativeConfigChanged callback is called too early(before the window size has changed), so we won't have the correct window size at that point.
    appState->activity->callbacks->onNativeWindowRedrawNeeded = OnWindowRedrawNeeded;

    // setup the android environment
    AZ::AllocatorInstance<AZ::OSAllocator>::Create();
    AZ::AllocatorInstance<AZ::LegacyAllocator>::Create();
    AZ::AllocatorInstance<CryStringAllocator>::Create();

    {
        AZ::Android::AndroidEnv::Descriptor descriptor;

        descriptor.m_jvm = appState->activity->vm;
        descriptor.m_activityRef = appState->activity->clazz;
        descriptor.m_assetManager = appState->activity->assetManager;

        descriptor.m_configuration = appState->config;

        descriptor.m_appPrivateStoragePath = appState->activity->internalDataPath;
        descriptor.m_appPublicStoragePath = appState->activity->externalDataPath;
        descriptor.m_obbStoragePath = appState->activity->obbPath;

        if (!AZ::Android::AndroidEnv::Create(descriptor))
        {
            AZ::Android::AndroidEnv::Destroy();
            MAIN_EXIT_FAILURE(appState, "Failed to create the AndroidEnv");
        }

        AZ::Android::AndroidEnv* androidEnv = AZ::Android::AndroidEnv::Get();
        appState->userData = androidEnv;
        androidEnv->SetIsRunning(true);
    }

    // sync the window creation
    {
        // While not ideal to have the event pump code duplicated here and in AzFramework, this
        // at least solves the splash screen issues when the window creation sync happened later
        // in initialization.  It's also the lesser of 2 evils, the other requiring a change in
        // how the platform specific private Application implementation is created for ALL
        // platforms
        while (!g_windowInitialized)
        {
            g_eventDispatcher.PumpAllEvents();
        }

        // Now that the window has been created we can show the java splash screen.  We need
        // to do it here and not in the window init event because every time the app is
        // backgrounded/foregrounded the window is destroyed/created, respectively.  So, we
        // don't want to show the splash screen when we resumed from a paused state.
        AZ::Android::Utils::ShowSplashScreen();
    }

#ifdef AZ_RUN_ANDROID_LAUNCHER_TESTS
    RunAndroidLauncherTests();
#endif

    // Engine Config (bootstrap.cfg)
    const char* assetsPath = AZ::Android::Utils::FindAssetsDirectory();
    if (!assetsPath)
    {
        AZ::Android::AndroidEnv::Destroy();
        MAIN_EXIT_FAILURE(appState, "Unable to locate bootstrap.cfg");
    }

    const char* searchPaths[] = { assetsPath };
    CEngineConfig engineCfg(searchPaths, 1);
    const char* gameName = engineCfg.m_gameDLL.c_str();
    engineCfg.m_gameFolder.MakeLower();
    
    // Game Application (AzGameFramework)
    AzGameFramework::GameApplication gameApp;
    AzGameFramework::GameApplication::StartupParameters gameAppParams;
    gameAppParams.m_allocator = &AZ::AllocatorInstance<AZ::OSAllocator>::Get();
#ifdef AZ_MONOLITHIC_BUILD
    gameAppParams.m_createStaticModulesCallback = CreateStaticModules;
    gameAppParams.m_loadDynamicModules = false;
#endif

    {
        // The path returned if assets are in the APK has to have a trailing slash other wise it causes
        // issues later on when parsing directories.  When they are on the sdcard, no trailing slash is
        // returned.  This patches the problem specifically for the descriptor only (it's not needed
        // elsewhere).
        const char* pathSep = (AZ::Android::Utils::IsApkPath(assetsPath) ? "" : "/");

        char descriptorRelativePath[AZ_MAX_PATH_LEN] = { 0 };
        AzGameFramework::GameApplication::GetGameDescriptorPath(descriptorRelativePath, engineCfg.m_gameFolder);

        AZStd::to_lower(descriptorRelativePath, descriptorRelativePath + strlen(descriptorRelativePath));
 
        char descriptorFullPath[AZ_MAX_PATH_LEN] = { 0 };
        azsnprintf(descriptorFullPath, AZ_MAX_PATH_LEN, "%s%s%s", assetsPath, pathSep, descriptorRelativePath);

        if (!AZ::IO::SystemFile::Exists(descriptorFullPath))
        {
            AZ::Android::AndroidEnv::Destroy();
            MAIN_EXIT_FAILURE(appState, "Application descriptor file not found: %s", descriptorFullPath);
        }

        gameApp.Start(descriptorFullPath, gameAppParams);
    }

    // set the native app state with the application framework
    AzFramework::AndroidAppRequests::Bus::Broadcast(&AzFramework::AndroidAppRequests::SetEventDispatcher, &g_eventDispatcher);

    // System Init Params ("Legacy" Lumberyard)
    SSystemInitParams startupParams;
    memset(&startupParams, 0, sizeof(SSystemInitParams));
    engineCfg.CopyToStartupParams(startupParams);

    startupParams.hInstance = 0;
    strcpy(startupParams.szSystemCmdLine, "");
    startupParams.sLogFileName = "Game.log";
    startupParams.pUserCallback = NULL;
    startupParams.pSharedEnvironment = AZ::Environment::GetInstance();

    // VFS will override all other methods of file access and no logs will be written to the SD card
    if (startupParams.remoteFileIO)
    {
        LOGI("[INFO] Using VFS for game files");
        LOGI("[INFO] Log and cache files will be written to the Cache directory on your host PC");
        LOGI("[INFO] If your game does not run, make sure you have used \'adb reverse\'\nto set up a TCP connection from your host PC to your device over USB");
    }
    else
    {
        LOGI("[INFO] Using %s for game files", AZ::Android::Utils::IsApkPath(startupParams.rootPath) ? "APK" : "app storage");
        LOGI("[INFO] Log and cache files will be written to app storage");

    #if defined(_RELEASE)
        const char* writeStorage = AZ::Android::Utils::GetAppPrivateStoragePath();
    #else
        const char* writeStorage = AZ::Android::Utils::GetAppPublicStoragePath();
    #endif

        sprintf(startupParams.logPath, "%s/log", writeStorage);
        sprintf(startupParams.userPath, "%s/user", writeStorage);

        LOGI("**** ROOT path set to  : %s ****", startupParams.rootPath);
        LOGI("**** ASSET path set to : %s ****", startupParams.assetsPath);
        LOGI("**** USER path set to  : %s ****", startupParams.userPath);
        LOGI("**** LOG path set to   : %s ****", startupParams.logPath);
    }

#if defined(ENABLE_LOGGING)
    startupParams.pPrintSync = &g_androidPrintSink;
#endif

    // If there are no handlers for the editor game bus, attempt to load the legacy gamedll instead
    bool legacyGameDllStartup = (EditorGameRequestBus::GetTotalNumOfEventHandlers() == 0);

#if !defined(AZ_MONOLITHIC_BUILD)
    IGameStartup::TEntryFunction CreateGameStartup = nullptr;
    HMODULE gameDll = nullptr;
    HMODULE systemLib = nullptr;

    if (legacyGameDllStartup)
    {
        char gameSoFilename[1024];
        snprintf(gameSoFilename, sizeof(gameSoFilename), "lib%s.so", gameName);

        gameDll = CryLoadLibrary(gameSoFilename);
        if (!gameDll)
        {
            AZ::Android::AndroidEnv::Destroy();
            MAIN_EXIT_FAILURE(appState, "Failed to load GAME DLL (%s)", dlerror());
        }

        // get address of startup function
        CreateGameStartup = (IGameStartup::TEntryFunction)CryGetProcAddress(gameDll, "CreateGameStartup");
        if (!CreateGameStartup)
        {
            CryFreeLibrary(gameDll);
            AZ::Android::AndroidEnv::Destroy();
            MAIN_EXIT_FAILURE(appState, "CreateGameStartup could not be found in lib%s.so!", gameName);
        }
    }
#endif //!AZ_MONOLITHIC_BUILD

    // create the startup interface
    IGameStartup* gameStartup = nullptr;
    if (legacyGameDllStartup)
    {
        gameStartup = CreateGameStartup();
    }
    else
    {
        EditorGameRequestBus::BroadcastResult(gameStartup, &EditorGameRequestBus::Events::CreateGameStartup);
    }

    // The legacy IGameStartup and IGameFramework are now optional,
    // if they don't exist we need to create CrySystem here instead.
    if (!gameStartup || !gameStartup->Init(startupParams))
    {
    #if !defined(AZ_MONOLITHIC_BUILD)
        systemLib = CryLoadLibraryDefName("CrySystem");
        PFNCREATESYSTEMINTERFACE CreateSystemInterface = systemLib ? (PFNCREATESYSTEMINTERFACE)CryGetProcAddress(systemLib, "CreateSystemInterface") : nullptr;
        if (CreateSystemInterface)
        {
            startupParams.pSystem = CreateSystemInterface(startupParams);
        }
    #else
        startupParams.pSystem = CreateSystemInterface(startupParams);
    #endif // AZ_MONOLITHIC_BUILD
    }

    if (startupParams.pSystem)
    {
        AZ::Android::Utils::DismissSplashScreen();

    #if !defined(SYS_ENV_AS_STRUCT)
        gEnv = startupParams.pSystem->GetGlobalEnvironment();
    #endif

        // Execute autoexec.cfg to load the initial level
        gEnv->pConsole->ExecuteString("exec autoexec.cfg");

        // Run the main loop
        LumberyardLauncher::RunMainLoop(gameApp);
    }
    else
    {
        MAIN_EXIT_FAILURE(appState, "Failed to initialize the CrySystem Interface!");
    }

    // Shutdown
    if (gameStartup)
    {
        gameStartup->Shutdown();
        gameStartup = 0;
    }

#if !defined(AZ_MONOLITHIC_BUILD)
    if (legacyGameDllStartup)
    {
        CryFreeLibrary(gameDll);
    }

    if (systemLib)
    {
        CryFreeLibrary(systemLib);
    }
#endif

    gameApp.Stop();

    AZ::Android::AndroidEnv::Destroy();
    AZ::AllocatorInstance<CryStringAllocator>::Destroy();
    AZ::AllocatorInstance<AZ::LegacyAllocator>::Destroy();
    AZ::AllocatorInstance<AZ::OSAllocator>::Destroy();
}



#if defined(AZ_MONOLITHIC_BUILD)
#include <StaticModules.inl>
#endif
