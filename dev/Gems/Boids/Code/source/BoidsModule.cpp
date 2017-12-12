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
#include <IGem.h>
#include "CryAction.h"
#include "IEntitySystem.h"
#include <FlowSystem/Nodes/FlowBaseNode.h>

#include "ScriptBind_Boids.h"

namespace Boids
{
    class BoidsModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(BoidsModule, "{EC35D69C-8B9A-4422-80CC-B60EA3DA9234}", CryHooksModule);

    private:
        void RegisterBoidEntityClasses()
        {
            IEntityClassRegistry::SEntityClassDesc clsDesc;
            IEntityClassRegistry* pClassRegistry = GetISystem()->GetIEntitySystem()->GetClassRegistry();

#define REGISTER_BOID(name, scriptfile)            \
    do                                             \
    {                                              \
        clsDesc.sName = name;                      \
        clsDesc.sScriptFile = scriptfile;          \
        pClassRegistry->RegisterStdClass(clsDesc); \
    } while (0);

            REGISTER_BOID("Boid", "Scripts/Entities/Boids/Boid.lua");
            REGISTER_BOID("Birds", "Scripts/Entities/Boids/Birds.lua");
            REGISTER_BOID("Chickens", "Scripts/Entities/Boids/Chickens.lua");
            REGISTER_BOID("BaldEagle", "Scripts/Entities/Boids/BaldEagle.lua");
            REGISTER_BOID("Turtles", "Scripts/Entities/Boids/Turtles.lua");
            REGISTER_BOID("Crabs", "Scripts/Entities/Boids/Crabs.lua");
            REGISTER_BOID("Fish", "Scripts/Entities/Boids/Fish.lua");
            REGISTER_BOID("Frogs", "Scripts/Entities/Boids/Frogs.lua");
            REGISTER_BOID("Bugs", "Scripts/Entities/Boids/Bugs.lua");
#undef REGISTER_BOID
        }

        void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
        {
            switch (event)
            {
            case ESYSTEM_EVENT_FLOW_SYSTEM_REGISTER_EXTERNAL_NODES:
                RegisterExternalFlowNodes();
                break;

            case ESYSTEM_EVENT_GAME_POST_INIT:
                IComponentFactoryRegistry::RegisterAllComponentFactoryNodes(
                    *gEnv->pEntitySystem->GetComponentFactoryRegistry());
                break;

            case ESYSTEM_EVENT_GAME_POST_INIT_DONE:
                m_pScriptBindBoids = new CScriptBind_Boids(GetISystem());
                RegisterBoidEntityClasses();
                break;

            case ESYSTEM_EVENT_FAST_SHUTDOWN:
            case ESYSTEM_EVENT_FULL_SHUTDOWN:
                SAFE_DELETE(m_pScriptBindBoids);
                break;
            }
        }

    private:
        CScriptBind_Boids* m_pScriptBindBoids = nullptr;
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Boids_97e596870cf645e184e43d5a5ccda857, Boids::BoidsModule)
