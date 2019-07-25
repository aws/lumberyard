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
#define NOMINMAX
#include <windows.h>

#include <AzCore/Memory/AllocatorScope.h>
#include <AzGameFramework/Application/GameApplication.h>
#include <IGameStartup.h>
#include <IEditorGame.h>
#include <platform_impl.h>
#include <ParseEngineConfig.h>

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
    )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

//! Bootstrapping globals and forward declarations
AzGameFramework::GameApplication gameApp;
IGameStartup* m_gameStartup = nullptr;
HMODULE m_gameDLL = 0;
AZ::AllocatorScope<AZ::LegacyAllocator, CryStringAllocator> m_legacyAllocatorScope;

extern "C" AZ_DLL_EXPORT int Initialize();
extern "C" AZ_DLL_EXPORT int Shutdown();

//! Initialize the engine using the active game project
int Initialize()
{
    m_legacyAllocatorScope.ActivateAllocators();

    char descriptorPath[AZ_MAX_PATH_LEN] = { 0 };
    {
        CEngineConfig engineCfg;
        AzGameFramework::GameApplication::GetGameDescriptorPath(descriptorPath, engineCfg.m_gameFolder);
    }
    AzGameFramework::GameApplication::StartupParameters gameAppParams;
    gameApp.Start(descriptorPath, gameAppParams);

    char szExeFileName[AZ_MAX_PATH_LEN] = { 0 };
    InitRootDir(szExeFileName, AZ_MAX_PATH_LEN);

    SSystemInitParams startupParams;
    startupParams.bUnattendedMode = true;
    startupParams.pSharedEnvironment = AZ::Environment::GetInstance();

    CEngineConfig engineCfg;

    static const char logFileName[] = "@log@/AzTestRunner.log";

    // now override those with custom values for this variant:
    startupParams.bTestMode = true;
    startupParams.hInstance = GetModuleHandle(0);
    startupParams.sLogFileName = logFileName;
    startupParams.bSkipFont = true;

    engineCfg.CopyToStartupParams(startupParams);

    // If there are no handlers for the editor game bus, attempt to load the legacy gamedll instead
    bool legacyGameDllStartup = (EditorGameRequestBus::GetTotalNumOfEventHandlers() == 0);

    if (legacyGameDllStartup)
    {
        // Only debug and profile use dynamic module loading, all other configurations are statically linked
#ifndef AZ_MONOLITHIC_BUILD
        char fullLibraryName[AZ_MAX_PATH_LEN] = { 0 };
        azsnprintf(fullLibraryName, AZ_MAX_PATH_LEN, "%s%s%s", CrySharedLibraryPrefix, engineCfg.m_gameDLL.c_str(), CrySharedLibraryExtension);
        m_gameDLL = CryLoadLibrary(fullLibraryName);
        if (!m_gameDLL)
        {
            return 1;
        }

        IGameStartup::TEntryFunction CreateGameStartup = (IGameStartup::TEntryFunction) GetProcAddress(m_gameDLL, "CreateGameStartup");
#endif
        m_gameStartup = CreateGameStartup();
    }
    else
    {
        EditorGameRequestBus::BroadcastResult(m_gameStartup, &EditorGameRequestBus::Events::CreateGameStartup);
    }

    if (m_gameStartup)
    {
        m_gameStartup->Init(startupParams);
        gEnv = startupParams.pSystem->GetGlobalEnvironment();
    }

    return 0;
}

//! Shutdown the engine
int Shutdown()
{
    if (m_gameStartup)
    {
        m_gameStartup->Shutdown();
        m_gameStartup = nullptr;

        if (m_gameDLL)
        {
            FreeLibrary(m_gameDLL);
        }
    }

    gameApp.Stop();

    m_legacyAllocatorScope.DeactivateAllocators();
    return 0;
}
