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

// Description : Launcher implementation for Android.
#include "StdAfx.h"

#include <platform_impl.h>
#include <AndroidSpecific.h>
#include <android/log.h>
#include <jni.h>

#include <AzGameFramework/Application/GameApplication.h>
#include <AzFramework/API/ApplicationAPI_android.h>

#include <AzCore/Android/AndroidEnv.h>
#include <AzCore/Android/Utils.h>
#include <AzCore/Android/JNI/Object.h>

#include <SDL.h>
#include <SDL_Extension.h>

#include <IGameStartup.h>
#include <IEntity.h>
#include <IGameFramework.h>
#include <IConsole.h>
#include <IEditorGame.h>

#include <netdb.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <ParseEngineConfig.h>
#include "android_descriptor.h"

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

    // from CrySystem
    extern "C" DLL_IMPORT void OnEngineRendererTakeover(bool engineSplashActive);
#else
    // from CrySystem
    typedef const void (* OnEngineRendererTakeoverFunc)(bool);
#endif //  defined(AZ_MONOLITHIC_BUILD)


#if !defined(_RELEASE)
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


#define RunGame_EXIT(exitCode) (exit(exitCode))

void RunGame();


//////////////////////////////////////////////////////////////////////////
bool GetDefaultThreadStackSize(size_t* pStackSize)
{
    pthread_attr_t kDefAttr;
    pthread_attr_init(&kDefAttr);   // Required on Mac OS or pthread_attr_getstacksize will fail
    int iRes(pthread_attr_getstacksize(&kDefAttr, pStackSize));
    if (iRes != 0)
    {
        fprintf(stderr, "error: pthread_attr_getstacksize returned %d\n", iRes);
        return false;
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool IncreaseResourceMaxLimit(int iResource, rlim_t uMax)
{
    struct rlimit kLimit;
    if (getrlimit(iResource, &kLimit) != 0)
    {
        fprintf(stderr, "error: getrlimit (%d) failed\n", iResource);
        return false;
    }

    if (uMax != kLimit.rlim_max)
    {
        //if (uMax == RLIM_INFINITY || uMax > kLimit.rlim_max)
        {
            kLimit.rlim_max = uMax;
            if (setrlimit(iResource, &kLimit) != 0)
            {
                fprintf(stderr, "error: setrlimit (%d, %lu) failed\n", iResource, uMax);
                return false;
            }
        }
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
void HandleSignal(int sig)
{
    LOGE("Signal (%d) was just raised", sig);
    signal(sig, HandleSignal);
}

///////////////////////////////////////////////////////////////
void RunGame()
{
    //Adding a start up banner so you can see when the game is starting up in amongst the logcat spam
    LOGI("******************************************************");
    LOGI("*         Amazon Lumberyard - Launching Game....     *");
    LOGI("******************************************************");

    char absPath[MAX_PATH];
    memset(absPath, 0, sizeof(char) * MAX_PATH);

    if (!getcwd(absPath, sizeof(char) * MAX_PATH))
    {
        LOGE("[ERROR] Unable to get current working path!");
        RunGame_EXIT(1);
    }
    LOGI("CWD = %s", absPath);

    size_t uDefStackSize;
    if (!IncreaseResourceMaxLimit(RLIMIT_CORE, RLIM_INFINITY)
        || !GetDefaultThreadStackSize(&uDefStackSize)
        || !IncreaseResourceMaxLimit(RLIMIT_STACK, RLIM_INFINITY * uDefStackSize))
    {
        RunGame_EXIT(1);
    }

    // SDL intenally will register to handle the SIGINT and SIGTERM signals.
    // When either of these signals are captured by SDL, it will push SDL_QUIT
    // into the event queue.  The problem is we don't do anything with that
    // quit event so it will linger in the SDL event queue.  When the application
    // is paused/backgrounded and resumed, SDL will skip the restore process if
    // it sees the quit event in the queue.  Since our custom assert macros
    // will raise SIGINT, it's not ideal for us to consume the SDL_QUIT event
    // so we are going to override their handlers.
    signal(SIGINT, HandleSignal);
    signal(SIGTERM, HandleSignal);

    // setup the android environment
    AZ::AllocatorInstance<AZ::OSAllocator>::Create();

    if (JNIEnv* jniEnv = static_cast<JNIEnv*>(SDL_AndroidGetJNIEnv()))
    {
        JavaVM* javaVm = nullptr;
        jniEnv->GetJavaVM(&javaVm);

        jobject activityObject = static_cast<jobject>(SDL_AndroidGetActivity());

        AZ::Android::AndroidEnv::Descriptor descriptor;

        descriptor.m_jvm = javaVm;
        descriptor.m_activityRef = activityObject;
        descriptor.m_assetManager = SDLExt_GetAssetManager();
        descriptor.m_appPrivateStoragePath = SDL_AndroidGetInternalStoragePath();
        descriptor.m_appPublicStoragePath = SDL_AndroidGetExternalStoragePath();

        if (!AZ::Android::AndroidEnv::Create(descriptor))
        {
            jniEnv->DeleteLocalRef(activityObject);
            AZ::Android::AndroidEnv::Destroy();
        }

        jniEnv->DeleteLocalRef(activityObject);
    }

#if !defined(AZ_MONOLITHIC_BUILD)
    HMODULE systemlib = CryLoadLibrary("libCrySystem.so");
    if (!systemlib)
    {
        LOGE("[ERROR] Failed to load CrySystem: %s", dlerror());
        RunGame_EXIT(1);
    }

    OnEngineRendererTakeoverFunc OnEngineRendererTakeover = (OnEngineRendererTakeoverFunc)CryGetProcAddress(systemlib, "OnEngineRendererTakeover");
    if (!OnEngineRendererTakeover)
    {
        LOGE("[ERROR] OnEngineRendererTakeover could not be found in CrySystem!\n");
        CryFreeLibrary(systemlib);
        RunGame_EXIT(1);
    }

#endif // !defined(AZ_MONOLITHIC_BUILD)

    const char* assetsPath = AZ::Android::Utils::FindAssetsDirectory();
    if (!assetsPath)
    {
        LOGE("#################################################");
        LOGE("[ERROR] Unable to locate bootstrap.cfg - Exiting!");
        LOGE("#################################################");
        RunGame_EXIT(1);
    }

    const char* searchPaths[] = {
        assetsPath
    };

    CEngineConfig engineCfg(searchPaths, 1);

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


#if !defined(_RELEASE)
    startupParams.pPrintSync = &g_androidPrintSink;
#endif

    chdir(assetsPath);

    // Init AzGameFramework
    AzGameFramework::GameApplication gameApp;
    AzGameFramework::GameApplication::StartupParameters gameAppParams;
    gameAppParams.m_allocator = &AZ::AllocatorInstance<AZ::OSAllocator>::Get();
#ifdef AZ_MONOLITHIC_BUILD
    gameAppParams.m_createStaticModulesCallback = CreateStaticModules;
    gameAppParams.m_loadDynamicModules = false;
#endif

    const char* gameName = engineCfg.m_gameDLL.c_str();

    AZ::ComponentApplication::Descriptor androidDescriptor;
    SetupAndroidDescriptor(gameName, androidDescriptor);
    gameApp.Start(androidDescriptor, gameAppParams);

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
            CryFreeLibrary(systemlib);
            RunGame_EXIT(1);
        }

        // get address of startup function
        CreateGameStartup = (IGameStartup::TEntryFunction)CryGetProcAddress(gameDll, "CreateGameStartup");
        if (!CreateGameStartup)
        {
            LOGE("[ERROR] CreateGameStartup could not be found in %s!\n", gameName);
            CryFreeLibrary(gameDll);
            CryFreeLibrary(systemlib);
            RunGame_EXIT(1);
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
        CryFreeLibrary(systemlib);
    #endif
        RunGame_EXIT(1);
    }

    // run the game
    int exitCode = 0;
    IGame* game = gameStartup->Init(startupParams);
    if (game)
    {
        // passing false because this would be the point where the engine renderer takes over if no engine splash screen was supplied
        OnEngineRendererTakeover(false);
        exitCode = gameStartup->Run(nullptr);
    }
    else
    {
        LOGE("[ERROR] Failed to initialize the GameStartup Interface!\n");
    }

    AZ::Android::AndroidEnv::Destroy();

    gameStartup->Shutdown();
    gameStartup = 0;

    gameApp.Stop();

    RunGame_EXIT(exitCode);
}

// An unreferenced function.  This function is needed to make sure that
// unneeded functions don't make it into the final executable.  The function
// section for this function should be removed in the final linking.
void this_function_is_not_used(void)
{
}

//-------------------------------------------------------------------------------------
int HandleApplicationLifecycleEvents(void* userdata, SDL_Event* event)
{
    switch (event->type)
    {
    // SDL2 generates two events when the native onPause is called:
    // SDL_APP_WILLENTERBACKGROUND and SDL_APP_DIDENTERBACKGROUND.
    case SDL_APP_DIDENTERBACKGROUND:
    {
        EBUS_EVENT(AzFramework::AndroidLifecycleEvents::Bus, OnPause);
        return 0;
    }
    break;

    // SDL2 generates two events when the native onResume is called:
    // SDL_APP_WILLENTERFOREGROUND and SDL_APP_DIDENTERFOREGROUND.
    case SDL_APP_DIDENTERFOREGROUND:
    {
        EBUS_EVENT(AzFramework::AndroidLifecycleEvents::Bus, OnResume);
        return 0;
    }
    break;
    case SDL_APP_TERMINATING:
    {
        EBUS_EVENT(AzFramework::AndroidLifecycleEvents::Bus, OnDestroy);
        return 0;
    }
    break;
    case SDL_APP_LOWMEMORY:
    {
        EBUS_EVENT(AzFramework::AndroidLifecycleEvents::Bus, OnLowMemory);
        return 0;
    }
    break;
    default:
    {
        // No special handling, add event to the queue
        return 1;
    }
    break;
    }
}

/**
 * Entry point when running in SDL framework.
 */
extern "C" int SDL_main(int argc, char* argv[])
{
    // Initialize SDL.
    SDL_SetEventFilter(HandleApplicationLifecycleEvents, NULL);
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC | SDL_INIT_GAMECONTROLLER) < 0)
    {
        fprintf(stderr, "ERROR: SDL initialization failed: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    atexit(SDL_Quit);

#if CAPTURE_REPLAY_LOG
    // Since Android doesn't support native command line argument, please
    // uncomment the following line if -memreplay is needed.
    // CryGetIMemReplay()->StartOnCommandLine("-memreplay");
    CryGetIMemReplay()->StartOnCommandLine("");
#endif // CAPTURE_REPLAY_LOG

    RunGame();
    return 0;
}

// vim:sw=4:ts=4:si:noet
