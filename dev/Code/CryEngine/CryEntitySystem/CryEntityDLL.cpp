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

#include "stdafx.h"

#include <IEntitySystem.h>
#include "EntitySystem.h"
// Included only once per DLL module.
#include <platform_impl.h>

#include <IEngineModule.h>
#include <CryExtension/ICryFactory.h>
#include <CryExtension/Impl/ClassWeaver.h>

#undef GetClassName

CEntitySystem* g_pIEntitySystem = NULL;

struct CSystemEventListner_Entity
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

        case ESYSTEM_EVENT_LEVEL_LOAD_START:
            if (g_pIEntitySystem)
            {
                g_pIEntitySystem->OnLevelLoadStart();
            }
            break;

        case ESYSTEM_EVENT_LEVEL_POST_UNLOAD:
        {
            STLALLOCATOR_CLEANUP;
            break;
        }
        case ESYSTEM_EVENT_3D_POST_RENDERING_END:
        {
            if (g_pIEntitySystem)
            {
                g_pIEntitySystem->Unload();
            }
            break;
        }
        }
    }
};
static CSystemEventListner_Entity g_system_event_listener_entity;


//////////////////////////////////////////////////////////////////////////
class CEngineModule_EntitySystem
    : public IEngineModule
{
    CRYINTERFACE_SIMPLE(IEngineModule)
    CRYGENERATE_SINGLETONCLASS(CEngineModule_EntitySystem, "EngineModule_CryEntitySystem", 0x885655072f014c03, 0x820c5a1a9b4d623b)

    //////////////////////////////////////////////////////////////////////////
    virtual const char* GetName() const {
        return "CryEntitySystem";
    };
    virtual const char* GetCategory() const { return "CryEngine"; };

    //////////////////////////////////////////////////////////////////////////
    virtual bool Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams)
    {
        ISystem* pSystem = env.pSystem;

        CEntitySystem* pEntitySystem = new CEntitySystem(pSystem);
        g_pIEntitySystem = pEntitySystem;
        if (!pEntitySystem->Init(pSystem))
        {
            pEntitySystem->Release();
            return false;
        }
        pSystem->GetISystemEventDispatcher()->RegisterListener(&g_system_event_listener_entity);

        env.pEntitySystem = pEntitySystem;
        return true;
    }
};

CRYREGISTER_SINGLETON_CLASS(CEngineModule_EntitySystem)

CEngineModule_EntitySystem::CEngineModule_EntitySystem()
{
};

CEngineModule_EntitySystem::~CEngineModule_EntitySystem()
{
};


#include <CrtDebugStats.h>
