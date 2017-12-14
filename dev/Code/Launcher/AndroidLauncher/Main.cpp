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


#define MAIN_EXIT_FAILURE() exit(1)


namespace
{
    bool g_windowInitialized = false;

    //////////////////////////////////////////////////////////////////////////
    bool IncreaseResourceToMaxLimit(int resource)
    {
        const rlim_t maxLimit = RLIM_INFINITY;

        struct rlimit limit;
        if (getrlimit(resource, &limit) != 0)
        {
            LOGE("[ERROR] Failed to get limit for resource %d", resource);
            return false;
        }

        if (limit.rlim_max != maxLimit)
        {
            limit.rlim_max = maxLimit;
            if (setrlimit(resource, &limit) != 0)
            {
                LOGE("[ERROR] Failed to update limit for resource %d with value %ld", resource, maxLimit);
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
        switch (command)
        {
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
        }
    }
}


////////////////////////////////////////////////////////////////
// This is the main entry point of a native application that is using android_native_app_glue.  
// It runs in its own thread, with its own event loop for receiving input events and doing other things.
void android_main(android_app* appState)
{
    // Adding a start up banner so you can see when the game is starting up in amongst the logcat spam
    LOGI("******************************************************");
    LOGI("*         Amazon Lumberyard - Launching Game....     *");
    LOGI("******************************************************");

    if (    !IncreaseResourceToMaxLimit(RLIMIT_CORE) 
        ||  !IncreaseResourceToMaxLimit(RLIMIT_STACK))
    {
        MAIN_EXIT_FAILURE();
    }

    // register our signal handlers
    RegisterSignalHandler(SIGINT, HandleSignal);
    RegisterSignalHandler(SIGTERM, HandleSignal);

    // setup the system command handler
    appState->onAppCmd = HandleApplicationLifecycleEvents;

    // setup the android environment
    AZ::AllocatorInstance<AZ::OSAllocator>::Create();
    {
        AZ::Android::AndroidEnv::Descriptor descriptor;

        descriptor.m_jvm = appState->activity->vm;
        descriptor.m_activityRef = appState->activity->clazz;
        descriptor.m_assetManager = appState->activity->assetManager;

        descriptor.m_appPrivateStoragePath = appState->activity->internalDataPath;
        descriptor.m_appPublicStoragePath = appState->activity->externalDataPath;
        descriptor.m_obbStoragePath = appState->activity->obbPath;

        if (!AZ::Android::AndroidEnv::Create(descriptor))
        {
            AZ::Android::AndroidEnv::Destroy();

            LOGE("[ERROR] Failed to create the AndroidEnv\n");
            MAIN_EXIT_FAILURE();
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
            int events;
            android_poll_source* source;

            while (ALooper_pollAll(0, NULL, &events, reinterpret_cast<void**>(&source)) >= 0)
            {
                if (source != NULL)
                {
                    source->process(appState, source);
                }
            }
        }

        // Now that the window has been created we can show the java splash screen.  We need
        // to do it here and not in the window init event because every time the app is 
        // backgrounded/foregrounded the window is destroyed/created, respectively.  So, we
        // don't want to show the splash screen when we resumed from a paused state.
        AZ::Android::Utils::ShowSplashScreen();
    }

    // Engine Config (bootstrap.cfg)
    const char* assetsPath = AZ::Android::Utils::FindAssetsDirectory();
    if (!assetsPath)
    {
        LOGE("#################################################");
        LOGE("[ERROR] Unable to locate bootstrap.cfg - Exiting!");
        LOGE("#################################################");
        MAIN_EXIT_FAILURE();
    }

    const char* searchPaths[] = { assetsPath };
    CEngineConfig engineCfg(searchPaths, 1);
    const char* gameName = engineCfg.m_gameDLL.c_str();

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

        char descriptorFullPath[AZ_MAX_PATH_LEN] = { 0 };
        azsnprintf(descriptorFullPath, AZ_MAX_PATH_LEN, "%s%s%s", assetsPath, pathSep, descriptorRelativePath);

        if (!AZ::IO::SystemFile::Exists(descriptorFullPath))
        {
            LOGE("Application descriptor file not found: %s\n", descriptorFullPath);
            MAIN_EXIT_FAILURE();
        }

        gameApp.Start(descriptorFullPath, gameAppParams);
    }

    // set the native app state with the application framework
    AzFramework::AndroidAppRequests::Bus::Broadcast(&AzFramework::AndroidAppRequests::SetAppState, appState);

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
    HMODULE gameDll = 0;

    if (legacyGameDllStartup)
    {
        char gameSoFilename[1024];
        snprintf(gameSoFilename, sizeof(gameSoFilename), "lib%s.so", gameName);

        gameDll = CryLoadLibrary(gameSoFilename);
        if (!gameDll)
        {
            LOGE("[ERROR] Failed to load GAME DLL (%s)\n", dlerror());
            MAIN_EXIT_FAILURE();
        }

        // get address of startup function
        CreateGameStartup = (IGameStartup::TEntryFunction)CryGetProcAddress(gameDll, "CreateGameStartup");
        if (!CreateGameStartup)
        {
            LOGE("[ERROR] CreateGameStartup could not be found in %s!\n", gameName);
            CryFreeLibrary(gameDll);
            MAIN_EXIT_FAILURE();
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

    if (!gameStartup)
    {
        LOGE("[ERROR] Failed to create the GameStartup Interface!\n");
    #if !defined(AZ_MONOLITHIC_BUILD)
        if (legacyGameDllStartup)
        {
            CryFreeLibrary(gameDll);
        }
    #endif
        MAIN_EXIT_FAILURE();
    }

    // run the game
    IGame* game = gameStartup->Init(startupParams);
    if (game)
    {
        AZ::Android::Utils::DismissSplashScreen();

#if !defined(SYS_ENV_AS_STRUCT)
        gEnv = startupParams.pSystem->GetGlobalEnvironment();
#endif

        // Execute autoexec.cfg to load the initial level
        gEnv->pConsole->ExecuteString("exec autoexec.cfg");

        // Run the main loop
        LumberyardLauncher::RunMainLoop(gameApp, *gEnv->pGame->GetIGameFramework());
    }
    else
    {
        LOGE("[ERROR] Failed to initialize the GameStartup Interface!\n");
    }

    // Shutdown
    AZ::Android::AndroidEnv::Destroy();

    gameStartup->Shutdown();
    gameStartup = 0;

    gameApp.Stop();
}

