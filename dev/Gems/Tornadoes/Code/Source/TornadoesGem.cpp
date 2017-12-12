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
#include "TornadoesGem.h"
#include "Tornado.h"
#include <FlowSystem/Nodes/FlowBaseNode.h>

void TornadoesGem::PostGameInitialize()
{
    IComponentFactoryRegistry::RegisterAllComponentFactoryNodes(*gEnv->pEntitySystem->GetComponentFactoryRegistry());
    // Originally registered with REGISTER_GAME_OBJECT(pFramework, Rain, "Scripts/Entities/Environment/Tornado.lua");
    // If more objects need registered, consider bringing the macro back along with the GameFactory wrapper.
    IEntityClassRegistry::SEntityClassDesc clsDesc;
    clsDesc.sName = "Tornado";
    clsDesc.sScriptFile = "Scripts/Entities/Environment/Tornado.lua";
    static CTornadoCreator _creator;
    GetISystem()->GetIGame()->GetIGameFramework()->GetIGameObjectSystem()->RegisterExtension("Tornado", &_creator, &clsDesc);
}

void TornadoesGem::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
    switch (event)
    {
    case ESYSTEM_EVENT_FLOW_SYSTEM_REGISTER_EXTERNAL_NODES:
        RegisterExternalFlowNodes();
        break;

    case ESYSTEM_EVENT_GAME_POST_INIT:
        PostGameInitialize();
        break;

    case ESYSTEM_EVENT_FULL_SHUTDOWN:
    case ESYSTEM_EVENT_FAST_SHUTDOWN:
        // Put your shutdown code here
        // Other Gems may have been shutdown already, but none will have destructed
        break;
    }
}

AZ_DECLARE_MODULE_CLASS(Tornadoes_d48ca459d0644521aad37f08466ef83a, TornadoesGem)
