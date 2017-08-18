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

// Description : Defines the entry point for the DLL application.


#include "StdAfx.h"
#include "CryAISystem.h"
#include <platform_impl.h>
#include "CAISystem.h"
#include "AILog.h"
#include <ISystem.h>

#include <IEngineModule.h>
#include <CryExtension/ICryFactory.h>
#include <CryExtension/Impl/ClassWeaver.h>

#undef GetClassName

CAISystem* g_pAISystem;

/*
//////////////////////////////////////////////////////////////////////////
// Pointer to Global ISystem.
static ISystem* gISystem = 0;
ISystem* GetISystem()
{
    return gISystem;
}
*/
//////////////////////////////////////////////////////////////////////////

/*
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef GetClassName

BOOL APIENTRY DllMain( HANDLE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved )
{
    return TRUE;
}
#endif
*/


//////////////////////////////////////////////////////////////////////////
struct CSystemEventListner_AI
    : public ISystemEventListener
{
public:
    virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
    {
        switch (event)
        {
        case ESYSTEM_EVENT_RANDOM_SEED:
            cry_random_seed(gEnv->bNoRandomSeed ? 0 : (uint32)wparam);
            break;
        case ESYSTEM_EVENT_LEVEL_POST_UNLOAD:
        {
            STLALLOCATOR_CLEANUP;
            break;
        }
        }
    }
};
static CSystemEventListner_AI g_system_event_listener_ai;

//////////////////////////////////////////////////////////////////////////
class CEngineModule_CryAISystem
    : public IEngineModule
{
    CRYINTERFACE_SIMPLE(IEngineModule)
    CRYGENERATE_SINGLETONCLASS(CEngineModule_CryAISystem, "EngineModule_CryAISystem", 0x6b8e79a784004f44, 0x97db7614428ad251)

    //////////////////////////////////////////////////////////////////////////
    virtual const char* GetName() const {
        return "CryAISystem";
    };
    virtual const char* GetCategory() const { return "CryEngine"; };

    //////////////////////////////////////////////////////////////////////////
    virtual bool Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams)
    {
        ISystem* pSystem = env.pSystem;

        pSystem->GetISystemEventDispatcher()->RegisterListener(&g_system_event_listener_ai);

        AIInitLog(pSystem);

        g_pAISystem = new CAISystem(pSystem);
        env.pAISystem = g_pAISystem;

        return true;
    }
};

CRYREGISTER_SINGLETON_CLASS(CEngineModule_CryAISystem)

CEngineModule_CryAISystem::CEngineModule_CryAISystem()
{
};

CEngineModule_CryAISystem::~CEngineModule_CryAISystem()
{
};

//////////////////////////////////////////////////////////////////////////
#include <CrtDebugStats.h>

#ifndef AZ_MONOLITHIC_BUILD
    #include "Common_TypeInfo.h"
#endif

#include "TypeInfo_impl.h"
#include "AutoTypeStructs_info.h"