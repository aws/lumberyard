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

#include "CryLegacy_precompiled.h"

#include <IEntitySystem.h>
#include "EntitySystem.h"

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
namespace LegacyCryEntitySystem
{
    //////////////////////////////////////////////////////////////////////////
    IEntitySystem* InitEntitySystem()
    {
        ISystem* pSystem = gEnv->pSystem;

        CEntitySystem* pEntitySystem = new CEntitySystem(pSystem);
        g_pIEntitySystem = pEntitySystem;
        if (!pEntitySystem->Init(pSystem))
        {
            pEntitySystem->Release();
            return nullptr;
        }
        pSystem->GetISystemEventDispatcher()->RegisterListener(&g_system_event_listener_entity);

        gEnv->pEntitySystem = pEntitySystem;
        return pEntitySystem;
    }
};
