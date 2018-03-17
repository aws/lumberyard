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
#include "Rain_precompiled.h"
#include <platform_impl.h>

#include <FlowSystem/Nodes/FlowBaseNode.h>

#include "RainGem.h"
#include "Rain.h"

#include "RainComponent.h"
#ifdef RAIN_EDITOR
#include "EditorRainComponent.h"
#endif // RAIN_EDITOR

RainGem::RainGem()
{
    m_descriptors.push_back(Rain::RainComponent::CreateDescriptor());
    
#ifdef RAIN_EDITOR
    m_descriptors.push_back(Rain::EditorRainComponent::CreateDescriptor());

    m_rainConverter = AZStd::make_unique<Rain::RainConverter>();

    m_rainConverter->BusConnect();
#endif // RAIN_EDITOR
}

RainGem::~RainGem()
{
#ifdef RAIN_EDITOR
    m_rainConverter->BusDisconnect();
#endif
}

void RainGem::PostGameInit()
{
    // Originally registered with REGISTER_GAME_OBJECT(pFramework, Rain, "Scripts/Entities/Environment/Rain.lua");
    // If more objects need registered, consider bringing the macro back along with the GameFactory wrapper.
    IEntityClassRegistry::SEntityClassDesc clsDesc;
    clsDesc.sName = "Rain";
    clsDesc.sScriptFile = "Scripts/Entities/Environment/Rain.lua";
    static LYGame::CRainCreator _creator;
    GetISystem()->GetIGame()->GetIGameFramework()->GetIGameObjectSystem()->RegisterExtension("Rain", &_creator, &clsDesc);
}

void RainGem::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
    switch (event)
    {
    case ESYSTEM_EVENT_FLOW_SYSTEM_REGISTER_EXTERNAL_NODES:
        RegisterExternalFlowNodes();
        break;

    case ESYSTEM_EVENT_GAME_POST_INIT:
        IComponentFactoryRegistry::RegisterAllComponentFactoryNodes(*gEnv->pEntitySystem->GetComponentFactoryRegistry());
        PostGameInit();
        break;

    case ESYSTEM_EVENT_FULL_SHUTDOWN:
    case ESYSTEM_EVENT_FAST_SHUTDOWN:
        // Put your shutdown code here
        // Other Gems may have been shutdown already, but none will have destructed
        break;
    }
}

AZ_DECLARE_MODULE_CLASS(Rain_e5f049ad7f534847a89c27b7339cf6a6, RainGem)
