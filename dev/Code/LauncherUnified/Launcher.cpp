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
#include <Launcher_precompiled.h>
#include <Launcher.h>

#include <platform_impl.h>

#include <AzCore/Debug/Trace.h>
#include <AzCore/IO/SystemFile.h>

#include <AzGameFramework/Application/GameApplication.h>

#include <CryLibrary.h>
#include <IEditorGame.h>
#include <IGameFramework.h>
#include <IGameStartup.h>
#include <ITimer.h>
#include <LegacyAllocator.h>
#include <ParseEngineConfig.h>

#if defined(AZ_MONOLITHIC_BUILD)
    #include <StaticModules.inl>

    // Include common type defines for static linking
    // Manually instantiate templates as needed here.
    #include <Common_TypeInfo.h>

    STRUCT_INFO_T_INSTANTIATE(Ang3_tpl, <float>)
    STRUCT_INFO_T_INSTANTIATE(Color_tpl, <float>)
    STRUCT_INFO_T_INSTANTIATE(Color_tpl, <uint8>)
    STRUCT_INFO_T_INSTANTIATE(Matrix33_tpl, <float>)
    STRUCT_INFO_T_INSTANTIATE(Plane_tpl, <float>)
    STRUCT_INFO_T_INSTANTIATE(Quat_tpl, <float>)
    STRUCT_INFO_T_INSTANTIATE(QuatT_tpl, <float>)
    STRUCT_INFO_T_INSTANTIATE(Vec2_tpl, <float>)
    STRUCT_INFO_T_INSTANTIATE(Vec2_tpl, <int>)
    STRUCT_INFO_T_INSTANTIATE(Vec3_tpl, <float>)
    STRUCT_INFO_T_INSTANTIATE(Vec3_tpl, <int>)
    STRUCT_INFO_T_INSTANTIATE(Vec4_tpl, <short>)

    extern "C" IGameStartup * CreateGameStartup();
#endif //  defined(AZ_MONOLITHIC_BUILD)


#if defined(DEDICATED_SERVER)
    static const char* s_logFileName = "@log@/Server.log";
#else
    static const char* s_logFileName = "@log@/Game.log";
#endif


namespace
{
#if AZ_TRAIT_LAUNCHER_USE_CRY_DYNAMIC_MODULE_HANDLE
    // mimics AZ::DynamicModuleHandle but uses CryLibrary under the hood,
    // which is necessary to properly load legacy Cry libraries on some platforms
    class DynamicModuleHandle
    {
    public:
        AZ_CLASS_ALLOCATOR(DynamicModuleHandle, AZ::OSAllocator, 0)

        static AZStd::unique_ptr<DynamicModuleHandle> Create(const char* fullFileName)
        {
            return AZStd::unique_ptr<DynamicModuleHandle>(aznew DynamicModuleHandle(fullFileName));
        }

        DynamicModuleHandle(const DynamicModuleHandle&) = delete;
        DynamicModuleHandle& operator=(const DynamicModuleHandle&) = delete;

        ~DynamicModuleHandle()
        {
            Unload();
        }

        // argument is strictly to match the API of AZ::DynamicModuleHandle
        bool Load(bool unused)
        { 
            AZ_UNUSED(unused);

            if (IsLoaded())
            {
                return true;
            }

            m_moduleHandle = CryLoadLibrary(m_fileName.c_str());
            return IsLoaded();
        }

        bool Unload()
        {
            if (!IsLoaded())
            {
                return false;
            }

            return CryFreeLibrary(m_moduleHandle);
        }

        bool IsLoaded() const 
        { 
            return m_moduleHandle != nullptr; 
        }

        const AZ::OSString& GetFilename() const 
        { 
            return m_fileName; 
        }

        template<typename Function>
        Function GetFunction(const char* functionName) const
        {
            if (IsLoaded())
            {
                return reinterpret_cast<Function>(CryGetProcAddress(m_moduleHandle, functionName));
            }
            else
            {
                return nullptr;
            }
        }


    private:
        DynamicModuleHandle(const char* fileFullName)
            : m_fileName()
            , m_moduleHandle(nullptr)
        {
            m_fileName = AZ::OSString::format("%s%s%s", 
                CrySharedLibraryPrefix, fileFullName, CrySharedLibraryExtension);
        }

        AZ::OSString m_fileName;
        HMODULE m_moduleHandle;
    };
#else
    // mimics AZ::DynamicModuleHandle but also calls InjectEnvironmentFunction on
    // the loaded module which is necessary to properly load legacy Cry libraries
    class DynamicModuleHandle
    {
    public:
        AZ_CLASS_ALLOCATOR(DynamicModuleHandle, AZ::OSAllocator, 0);

        static AZStd::unique_ptr<DynamicModuleHandle> Create(const char* fullFileName)
        {
            return AZStd::unique_ptr<DynamicModuleHandle>(aznew DynamicModuleHandle(fullFileName));
        }

        bool Load(bool isInitializeFunctionRequired)
        {
            const bool loaded = m_moduleHandle->Load(isInitializeFunctionRequired);
            if (loaded)
            {
                // We need to inject the environment first thing so that allocators are available immediately
                InjectEnvironmentFunction injectEnv = GetFunction<InjectEnvironmentFunction>(INJECT_ENVIRONMENT_FUNCTION);
                if (injectEnv)
                {
                    auto env = AZ::Environment::GetInstance();
                    injectEnv(env);
                }
            }
            return loaded;
        }

        bool Unload()
        {
            bool unloaded = m_moduleHandle->Unload();
            if (unloaded)
            {
                DetachEnvironmentFunction detachEnv = GetFunction<DetachEnvironmentFunction>(DETACH_ENVIRONMENT_FUNCTION);
                if (detachEnv)
                {
                    detachEnv();
                }
            }
            return unloaded;
        }

        template<typename Function>
        Function GetFunction(const char* functionName) const
        {
            return m_moduleHandle->GetFunction<Function>(functionName);
        }

    private:
        DynamicModuleHandle(const char* fileFullName)
            : m_moduleHandle(AZ::DynamicModuleHandle::Create(fileFullName))
        {
        }

        AZStd::unique_ptr<AZ::DynamicModuleHandle> m_moduleHandle;
    };
#endif // AZ_TRAIT_LAUNCHER_USE_CRY_DYNAMIC_MODULE_HANDLE

    struct CryAllocatorsRAII
    {
        CryAllocatorsRAII()
        {
            AZ_Assert(!AZ::AllocatorInstance<AZ::LegacyAllocator>::IsReady(), "Expected allocator to not be initialized, hunt down the static that is initializing it");
            AZ_Assert(!AZ::AllocatorInstance<CryStringAllocator>::IsReady(), "Expected allocator to not be initialized, hunt down the static that is initializing it");

            AZ::AllocatorInstance<AZ::LegacyAllocator>::Create();
            AZ::AllocatorInstance<CryStringAllocator>::Create();
        }

        ~CryAllocatorsRAII()
        {
            AZ::AllocatorInstance<CryStringAllocator>::Destroy();
            AZ::AllocatorInstance<AZ::LegacyAllocator>::Destroy();
        }
    };

    void RunMainLoop(AzGameFramework::GameApplication& gameApplication)
    {
        // Ideally we'd just call GameApplication::RunMainLoop instead, but
        // we'd have to stop calling IGameFramework::PreUpdate / PostUpdate
        // directly, and instead have something subscribe to the TickBus in
        // order to call them, using order ComponentTickBus::TICK_FIRST - 1
        // and ComponentTickBus::TICK_LAST + 1 to ensure they get called at
        // the same time as they do now. Also, we'd need to pass a function
        // pointer to AzGameFramework::GameApplication::MainLoop that would
        // be used to call ITimer::GetFrameTime (unless we could also shift
        // our frame time to be managed by AzGameFramework::GameApplication
        // instead, which probably isn't going to happen anytime soon given
        // how many things depend on the ITimer interface).
        bool continueRunning = true;
        ISystem* system = gEnv ? gEnv->pSystem : nullptr;
        IGameFramework* gameFramework = (gEnv && gEnv->pGame) ? gEnv->pGame->GetIGameFramework() : nullptr;
        while (continueRunning)
        {
            // Pump the system event loop
            gameApplication.PumpSystemEventLoopUntilEmpty();

            // Update the AzFramework system tick bus
            gameApplication.TickSystem();

            // Pre-update CryEngine
            if (gameFramework)
            {
                // If the legacy game framework exists it updates CrySystem...
                continueRunning = gameFramework->PreUpdate(true, 0);
            }
            else if (system)
            {
                // ...otherwise we need to update it here.
                system->UpdatePreTickBus();
            }

            // Update the AzFramework application tick bus
            gameApplication.Tick(gEnv->pTimer->GetFrameTime());

            // Post-update CryEngine
            if (gameFramework)
            {
                // If the legacy game framework exists it updates CrySystem...
                continueRunning = gameFramework->PostUpdate(true, 0) && continueRunning;
            }
            else if (system)
            {
                // ...otherwise we need to update it here.
                system->UpdatePostTickBus();
            }

            // Check for quit requests
            continueRunning = !gameApplication.WasExitMainLoopRequested() && continueRunning;
        }
    }
}


namespace LumberyardLauncher
{
    bool PlatformMainInfo::CopyCommandLine(const char* commandLine)
    {
        if (!commandLine)
        {
            return false;
        }

        const size_t commandLineLen = strlen(commandLine) + 1; // +1 for the null terminator
        if (commandLineLen >= AZ_COMMAND_LINE_LEN)
        {
            return false;
        }
        auto ret = azstrncpy(m_commandLine, AZ_COMMAND_LINE_LEN, commandLine, commandLineLen);
        bool commandLineCopied = false;
    #if AZ_TRAIT_USE_SECURE_CRT_FUNCTIONS
        commandLineCopied = (ret == 0);
    #else
        commandLineCopied = (ret != nullptr);
    #endif
        if (commandLineCopied)
        {
            m_commandLineLen = commandLineLen;
        }
        return commandLineCopied;
    }

    bool PlatformMainInfo::CopyCommandLine(int argc, char** argv)
    {
        for (int argIndex = 0; argIndex < argc; ++argIndex)
        {
            if (!AddArgument(argv[argIndex]))
            {
                return false;
            }
        }
        return true;
    }

    bool PlatformMainInfo::AddArgument(const char* arg)
    {
        AZ_Error("Launcher", arg, "Attempting to add a nullptr command line argument!");

        bool needsQuote = (strstr(arg, " ") != nullptr);
        bool needsSpace = (m_commandLine[0] != 0);

        // strip the previous null-term from the count to prevent double counting
        m_commandLineLen = (m_commandLineLen == 0) ? 0 : (m_commandLineLen - 1);

        // compute the expected length with the added argument
        size_t argLen = strlen(arg);
        size_t pendingLen = m_commandLineLen + argLen + 1 + (needsSpace ? 1 : 0) + (needsQuote ? 2 : 0); // +1 null-term, [+1 space], [+2 quotes]

        if (pendingLen >= AZ_COMMAND_LINE_LEN)
        {
            AZ_Assert(false, "Command line exceeds the %d character limit!", AZ_COMMAND_LINE_LEN);
            return false;
        }

        if (needsSpace)
        {
            m_commandLine[m_commandLineLen++] = ' ';
        }

        azsnprintf(m_commandLine + m_commandLineLen,
            AZ_COMMAND_LINE_LEN - m_commandLineLen,
            needsQuote ? "\"%s\"" : "%s",
            arg);

        m_commandLineLen = pendingLen;
        return true;
    }


    const char* GetReturnCodeString(ReturnCode code)
    {
        switch (code)
        {
            case ReturnCode::Success:
                return "Success";

            case ReturnCode::ErrBootstrapMismatch:
                return "Mismatch detected between Launcher compiler defines and bootstrap values (LY_GAMEFOLDER/sys_game_folder or LY_GAMEDLL/sys_dll_game).";

            case ReturnCode::ErrCommandLine:
                return "Failed to copy command line arguments";

            case ReturnCode::ErrResourceLimit:
                return "A resource limit failed to update";

            case ReturnCode::ErrAppDescriptor:
                return "Application descriptor file was not found";

            case ReturnCode::ErrLegacyGameLib:
                return "Failed to load the legacy game project library";

            case ReturnCode::ErrLegacyGameStartup:
                return "Failed to locate symbol 'CreateGameStartup' from the legacy game project library";

            case ReturnCode::ErrCrySystemLib:
                return "Failed to load the CrySystem library";

            case ReturnCode::ErrCrySystemInterface:
                return "Failed to initialize the CrySystem Interface";

            case ReturnCode::ErrCryEnvironment:
                return "Failed to initialize the CryEngine global environment";

            default:
                return "Unknown error code";
        }
    }

    ReturnCode Run(const PlatformMainInfo& mainInfo)
    {
        if (mainInfo.m_updateResourceLimits 
            && !mainInfo.m_updateResourceLimits())
        {
            return ReturnCode::ErrResourceLimit;
        }

        CryAllocatorsRAII cryAllocatorsRAII;

        // Engine Config (bootstrap.cfg)
        const char* pathToAssets = mainInfo.m_appResourcesPath;
        const char* sourcePaths[] = { pathToAssets };
        CEngineConfig engineConfig(sourcePaths, 1);

    #if !defined(AZ_MONOLITHIC_BUILD)
        #if defined(LY_GAMEFOLDER)
            if (strcmp(engineConfig.m_gameFolder.c_str(), LY_GAMEFOLDER) != 0)
            {
                return ReturnCode::ErrBootstrapMismatch;
            }
        #endif // defined(LY_GAMEFOLDER)

        #if defined(LY_GAMEDLL)
            if (strcmp(engineConfig.m_gameDLL.c_str(), LY_GAMEFOLDER) != 0)
            {
                return ReturnCode::ErrBootstrapMismatch;
            }
        #endif // defined(LY_GAMEDLL)
    #endif // !defined(AZ_MONOLITHIC_BUILD)

    #if defined(DEDICATED_SERVER)
        // Dedicated server does not depend on Asset Processor and assumes that assets are already prepared.
        engineConfig.m_waitForConnect = false;
    #endif // defined(DEDICATED_SERVER)

    #if AZ_TRAIT_LAUNCHER_LOWER_CASE_PATHS
        engineConfig.m_gameFolder.MakeLower();
    #endif // AZ_TRAIT_LAUNCHER_LOWER_CASE_PATHS

        // Game Application (AzGameFramework)
        AzGameFramework::GameApplication gameApplication;
        {
            char pathToGameDescriptorFile[AZ_MAX_PATH_LEN] = { 0 };
            AzGameFramework::GameApplication::GetGameDescriptorPath(pathToGameDescriptorFile, engineConfig.m_gameFolder);

        #if AZ_TRAIT_LAUNCHER_LOWER_CASE_PATHS
            AZStd::to_lower(pathToGameDescriptorFile, pathToGameDescriptorFile + strlen(pathToGameDescriptorFile));
        #endif // AZ_TRAIT_LAUNCHER_LOWER_CASE_PATHS

            // this is mostly to account for an oddity on Android when the assets are packed inside the APK,
            // in which the value returned from GetAppResourcePath contains a necessary trailing slash.
            const char* pathSep = "/";
            size_t pathLen = strlen(pathToAssets);
            if (pathLen >= 1 && pathToAssets[pathLen - 1] == '/')
            {
                pathSep = "";
            }

            char fullPathToGameDescriptorFile[AZ_MAX_PATH_LEN] = { 0 };
            azsnprintf(fullPathToGameDescriptorFile, AZ_MAX_PATH_LEN, "%s%s%s", pathToAssets, pathSep, pathToGameDescriptorFile);

            if (!AZ::IO::SystemFile::Exists(fullPathToGameDescriptorFile))
            {
                AZ_Error("Launcher", false, "Application descriptor file not found: %s", fullPathToGameDescriptorFile);
                return ReturnCode::ErrAppDescriptor;
            }

            AzGameFramework::GameApplication::StartupParameters gameApplicationStartupParams;

        #if AZ_TRAIT_LAUNCHER_SET_APPROOT_OVERRIDE
            // setting this on windows will prevent the asset processor from auto launching and doesn't work 
            // properly on android when the assets are in the APK
            gameApplicationStartupParams.m_appRootOverride = pathToAssets;
        #endif // AZ_TRAIT_LAUNCHER_SET_APPROOT_OVERRIDE

            if (mainInfo.m_allocator)
            {
                gameApplicationStartupParams.m_allocator = mainInfo.m_allocator;
            }
            else if (AZ::AllocatorInstance<AZ::OSAllocator>::IsReady())
            {
                gameApplicationStartupParams.m_allocator = &AZ::AllocatorInstance<AZ::OSAllocator>::Get();
            }

        #if defined(AZ_MONOLITHIC_BUILD)
            gameApplicationStartupParams.m_createStaticModulesCallback = CreateStaticModules;
            gameApplicationStartupParams.m_loadDynamicModules = false;
        #endif // defined(AZ_MONOLITHIC_BUILD)

            gameApplication.Start(fullPathToGameDescriptorFile, gameApplicationStartupParams);
        }

        if (mainInfo.m_onPostAppStart)
        {
            mainInfo.m_onPostAppStart();
        }

        // System Init Params ("Legacy" Lumberyard)
        SSystemInitParams systemInitParams;
        memset(&systemInitParams, 0, sizeof(SSystemInitParams));

        engineConfig.CopyToStartupParams(systemInitParams);

        azstrncpy(systemInitParams.szSystemCmdLine, sizeof(systemInitParams.szSystemCmdLine), 
            mainInfo.m_commandLine, mainInfo.m_commandLineLen);

        systemInitParams.pSharedEnvironment = AZ::Environment::GetInstance();
        systemInitParams.sLogFileName = s_logFileName;
        systemInitParams.hInstance = mainInfo.m_instance;
        systemInitParams.hWnd = mainInfo.m_window;
        systemInitParams.pPrintSync = mainInfo.m_printSink;

        if (strstr(mainInfo.m_commandLine, "-norandom"))
        {
            systemInitParams.bNoRandom = true;
        }

    #if defined(DEDICATED_SERVER)
        systemInitParams.bDedicatedServer = true;
    #endif // defined(DEDICATED_SERVER)

        if (systemInitParams.remoteFileIO)
        {
            AZ_TracePrintf("Launcher", "Application is configured for VFS");
            AZ_TracePrintf("Launcher", "Log and cache files will be written to the Cache directory on your host PC");

            const char* message = "If your game does not run, check any of the following:\n"
                                  "\t- Verify the remove_ip address is correct in bootstrap.cfg";

            if (mainInfo.m_additionalVfsResolution)
            {
                AZ_TracePrintf("Launcher", "%s\n%s", message, mainInfo.m_additionalVfsResolution)
            }
            else 
            {
                AZ_TracePrintf("Launcher", "%s", message)
            }
        }
        else
        {
            AZ_TracePrintf("Launcher", "Application is configured to use device local files at %s", systemInitParams.rootPath);
            AZ_TracePrintf("Launcher", "Log and cache files will be written to device storage");

            const char* writeStorage = mainInfo.m_appWriteStoragePath;
            if (writeStorage)
            {
                azsnprintf(systemInitParams.userPath, AZ_MAX_PATH_LEN, "%s/user", writeStorage);
            }
        }

        // If there are no handlers for the editor game bus, attempt to load the legacy game dll instead
        bool legacyGameDllStartup = (EditorGameRequestBus::GetTotalNumOfEventHandlers() == 0);

    #if !defined(AZ_MONOLITHIC_BUILD)
        IGameStartup::TEntryFunction CreateGameStartup = nullptr;
        AZStd::unique_ptr<DynamicModuleHandle> legacyGameLibrary;
        AZStd::unique_ptr<DynamicModuleHandle> crySystemLibrary;

        if (legacyGameDllStartup)
        {
            legacyGameLibrary = DynamicModuleHandle::Create(engineConfig.m_gameDLL.c_str());
            if (!legacyGameLibrary->Load(false))
            {
                return ReturnCode::ErrLegacyGameLib;
            }

            // get address of startup function
            CreateGameStartup = legacyGameLibrary->GetFunction<IGameStartup::TEntryFunction>("CreateGameStartup");
            if (!CreateGameStartup)
            {
                return ReturnCode::ErrLegacyGameStartup;
            }
        }
    #endif // !defined(AZ_MONOLITHIC_BUILD)

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
        if (!gameStartup || !gameStartup->Init(systemInitParams))
        {
        #if !defined(AZ_MONOLITHIC_BUILD)
            PFNCREATESYSTEMINTERFACE CreateSystemInterface = nullptr;

            crySystemLibrary = DynamicModuleHandle::Create("CrySystem");
            if (crySystemLibrary->Load(false))
            {
                CreateSystemInterface = crySystemLibrary->GetFunction<PFNCREATESYSTEMINTERFACE>("CreateSystemInterface");
                if (CreateSystemInterface)
                {
                    systemInitParams.pSystem = CreateSystemInterface(systemInitParams);
                }
            }
        #else
            systemInitParams.pSystem = CreateSystemInterface(systemInitParams);
        #endif // !defined(AZ_MONOLITHIC_BUILD)
        }

        ReturnCode status = ReturnCode::Success;

        if (systemInitParams.pSystem)
        {
        #if !defined(SYS_ENV_AS_STRUCT)
            gEnv = systemInitParams.pSystem->GetGlobalEnvironment();
        #endif // !defined(SYS_ENV_AS_STRUCT)

            if (gEnv && gEnv->pConsole)
            {
                // Execute autoexec.cfg to load the initial level
                gEnv->pConsole->ExecuteString("exec autoexec.cfg");

                gEnv->pSystem->ExecuteCommandLine(false);

                // Run the main loop
                RunMainLoop(gameApplication);
            }
            else
            {
                status = ReturnCode::ErrCryEnvironment;
            }
        }
        else
        {
            status = ReturnCode::ErrCrySystemInterface;
        }

        // Shutdown
        if (gameStartup)
        {
            gameStartup->Shutdown();
            gameStartup = 0;
        }

    #if !defined(AZ_MONOLITHIC_BUILD)
        crySystemLibrary.reset(nullptr);
        legacyGameLibrary.reset(nullptr);
    #endif // !defined(AZ_MONOLITHIC_BUILD)

        gameApplication.Stop();

        return status;
    }
}
