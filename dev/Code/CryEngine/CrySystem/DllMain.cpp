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

#include "StdAfx.h"
#include "System.h"
#include <AZCrySystemInitLogSink.h>
#include <platform_impl.h>
#include "DebugCallStack.h"


// For lua debugger
//#include <malloc.h>

HMODULE gDLLHandle = NULL;


struct DummyInitializer
{
    DummyInitializer()
    {
        dummyValue = 1;
    }

    int dummyValue;
};

DummyInitializer& initDummy()
{
    static DummyInitializer* p = new DummyInitializer;
    return *p;
}

static int warmAllocator = initDummy().dummyValue;

#if !defined(AZ_MONOLITHIC_BUILD) && defined(AZ_HAS_DLL_SUPPORT) && !defined(AZ_PLATFORM_LINUX) && !defined(AZ_PLATFORM_APPLE) && !defined(AZ_PLATFORM_ANDROID)
#pragma warning( push )
#pragma warning( disable : 4447 )
BOOL APIENTRY DllMain(HANDLE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
    )
{
    gDLLHandle = (HMODULE)hModule;
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        break;
    case DLL_THREAD_ATTACH:


        break;
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    //  int sbh = _set_sbh_threshold(1016);

    return TRUE;
}
#pragma warning( pop )
#endif

#if defined(USE_GLOBAL_BUCKET_ALLOCATOR)
#include "CryMemoryAllocator.h"
#include "BucketAllocator.h"
extern void EnableDynamicBucketCleanups(bool enable);
#endif

//////////////////////////////////////////////////////////////////////////
struct CSystemEventListner_System
    : public ISystemEventListener
{
public:
    virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
    {
#if defined(USE_GLOBAL_BUCKET_ALLOCATOR)
        switch (event)
        {
        case ESYSTEM_EVENT_LEVEL_UNLOAD:
        case ESYSTEM_EVENT_LEVEL_LOAD_START:
        case ESYSTEM_EVENT_LEVEL_POST_UNLOAD:
            EnableDynamicBucketCleanups(true);
            break;

        case ESYSTEM_EVENT_LEVEL_LOAD_END:
            EnableDynamicBucketCleanups(false);
            break;
        }
#endif

        switch (event)
        {
        case ESYSTEM_EVENT_LEVEL_UNLOAD:
            gEnv->pSystem->SetThreadState(ESubsys_Physics, false);
            break;

        case ESYSTEM_EVENT_LEVEL_LOAD_START:
        case ESYSTEM_EVENT_LEVEL_LOAD_END:
        {
            CryCleanup();
            break;
        }

        case ESYSTEM_EVENT_LEVEL_POST_UNLOAD:
        {
            CryCleanup();
            STLALLOCATOR_CLEANUP;
            gEnv->pSystem->SetThreadState(ESubsys_Physics, true);
            break;
        }
        }
    }
};

static CSystemEventListner_System g_system_event_listener_system;

extern "C"
{
CRYSYSTEM_API ISystem* CreateSystemInterface(const SSystemInitParams& startupParams)
{
    CSystem* pSystem = NULL;

    // We must attach to the environment prior to allocating CSystem, as opposed to waiting
    // for ModuleInitISystem(), because the log message sink uses buses.
    AZ::Environment::Attach(startupParams.pSharedEnvironment);

    pSystem = new CSystem(startupParams.pSharedEnvironment);
    ModuleInitISystem(pSystem, "CrySystem");

#if defined(AZ_MONOLITHIC_BUILD)
    ICryFactoryRegistryImpl* pCryFactoryImpl = static_cast<ICryFactoryRegistryImpl*>(pSystem->GetCryFactoryRegistry());
    pCryFactoryImpl->RegisterFactories(g_pHeadToRegFactories);
#endif // AZ_MONOLITHIC_BUILD
       // the earliest point the system exists - w2e tell the callback
    if (startupParams.pUserCallback)
    {
        startupParams.pUserCallback->OnSystemConnect(pSystem);
    }

    // Environment Variable to signal we don't want to override our exception handler - our crash report system will set this
    auto envVar = AZ::Environment::FindVariable<bool>("ExceptionHandlerIsSet");
    bool handlerIsSet = (envVar && *envVar);

    if (!startupParams.bMinimal && !handlerIsSet)     // in minimal mode, we want to crash when we crash!
    {
#if defined(WIN32)
        // Install exception handler in Release modes.
        ((DebugCallStack*)IDebugCallStack::instance())->installErrorHandler(pSystem);
#endif
    }

    bool retVal = false;
    {
        AZ::Debug::StartupLogSinkReporter<AZ::Debug::CrySystemInitLogSink> initLogSink;
        retVal = pSystem->Init(startupParams);
        if (!retVal)
        {
            initLogSink.GetContainedLogSink().SetFatalMessageBox();
        }
    }
    if (!retVal)
    {
        delete pSystem;
        gEnv = nullptr;

        return 0;
    }

    pSystem->GetISystemEventDispatcher()->RegisterListener(&g_system_event_listener_system);

    return pSystem;
}

CRYSYSTEM_API void WINAPI CryInstallUnhandledExceptionHandler()
{
}

#if defined(ENABLE_PROFILING_CODE) && !defined(LINUX) && !defined(APPLE)
CRYSYSTEM_API void CryInstallPostExceptionHandler(void (* PostExceptionHandlerCallback)())
{
    return IDebugCallStack::instance()->FileCreationCallback(PostExceptionHandlerCallback);
}
#endif
};

