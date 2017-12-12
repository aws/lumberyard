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
#include "ScriptSystem.h"

// Included only once per DLL module.
#include <platform_impl.h>

#include <IEngineModule.h>
#include <CryExtension/ICryFactory.h>
#include <CryExtension/Impl/ClassWeaver.h>

#undef GetClassName

//////////////////////////////////////////////////////////////////////////
struct CSystemEventListner_Script
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
static CSystemEventListner_Script g_system_event_listener_script;

//////////////////////////////////////////////////////////////////////////
class CEngineModule_CryScriptSystem
    : public IEngineModule
{
    CRYINTERFACE_SIMPLE(IEngineModule)
    CRYGENERATE_SINGLETONCLASS(CEngineModule_CryScriptSystem, "EngineModule_CryScriptSystem", 0xd032b16449784f82, 0xa99e7dc6b6338c5c)

    //////////////////////////////////////////////////////////////////////////
    virtual const char* GetName() const {
        return "CryScriptSystem";
    };
    virtual const char* GetCategory() const { return "CryEngine"; };

    //////////////////////////////////////////////////////////////////////////
    virtual bool Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams)
    {
        ISystem* pSystem = env.pSystem;

        CScriptSystem* pScriptSystem = new CScriptSystem;

        pSystem->GetISystemEventDispatcher()->RegisterListener(&g_system_event_listener_script);

        bool bStdLibs = true;
        if (!pScriptSystem->Init(pSystem, bStdLibs, 1024))
        {
            pScriptSystem->Release();
            return false;
        }

        env.pScriptSystem = pScriptSystem;
        return true;
    }
};

CRYREGISTER_SINGLETON_CLASS(CEngineModule_CryScriptSystem)

CEngineModule_CryScriptSystem::CEngineModule_CryScriptSystem()
{
};

CEngineModule_CryScriptSystem::~CEngineModule_CryScriptSystem()
{
};


#if defined(WIN32) && !defined(AZ_MONOLITHIC_BUILD)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef GetClassName
HANDLE gDLLHandle = NULL;
BOOL APIENTRY DllMain(HANDLE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved)
{
    gDLLHandle = hModule;
    return TRUE;
}
#endif // WIN32 && !AZ_MONOLITHIC_BUILD

#include <CrtDebugStats.h>
