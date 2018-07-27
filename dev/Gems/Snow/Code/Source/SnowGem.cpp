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
#include "Snow_precompiled.h"
#include <platform_impl.h>

#include <FlowSystem/Nodes/FlowBaseNode.h>

#include "SnowGem.h"
#include "Snow.h"

#include "SnowComponent.h"
#ifdef SNOW_EDITOR
#include "EditorSnowComponent.h"
#endif // SNOW_EDITOR

SnowGem::SnowGem()
{
    m_descriptors.push_back(Snow::SnowComponent::CreateDescriptor());

#ifdef SNOW_EDITOR
    m_descriptors.push_back(Snow::EditorSnowComponent::CreateDescriptor());

    m_snowConverter = AZStd::make_unique<Snow::SnowConverter>();

    m_snowConverter->BusConnect();
#endif // SNOW_EDITOR
}

SnowGem::~SnowGem()
{
#ifdef SNOW_EDITOR
    m_snowConverter->BusDisconnect();
#endif // SNOW_EDITOR
}

void SnowGem::PostGameInitialize()
{
    // Originally registered with REGISTER_GAME_OBJECT(pFramework, Snow, "Scripts/Entities/Environment/Snow.lua");
    // If more objects need registered, consider bringing the macro back along with the GameFactory wrapper.
    if (GetISystem()->GetIGame() && GetISystem()->GetIGame()->GetIGameFramework())
    {
        IEntityClassRegistry::SEntityClassDesc clsDesc;
        clsDesc.sName = "Snow";
        clsDesc.sScriptFile = "Scripts/Entities/Environment/Snow.lua";
        static CSnowCreator _creator;
        GetISystem()->GetIGame()->GetIGameFramework()->GetIGameObjectSystem()->RegisterExtension("Snow", &_creator, &clsDesc);
    }
}

void SnowGem::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
    switch (event)
    {
    case ESYSTEM_EVENT_FLOW_SYSTEM_REGISTER_EXTERNAL_NODES:
        RegisterExternalFlowNodes();
        break;

    case ESYSTEM_EVENT_GAME_POST_INIT:
        IComponentFactoryRegistry::RegisterAllComponentFactoryNodes(*gEnv->pEntitySystem->GetComponentFactoryRegistry());
        PostGameInitialize();
        break;

    case ESYSTEM_EVENT_FULL_SHUTDOWN:
    case ESYSTEM_EVENT_FAST_SHUTDOWN:
        // Put your shutdown code here
        // Other Gems may have been shutdown already, but none will have destructed
        break;
    }
}

AZ_DECLARE_MODULE_CLASS(Snow_8827e54450f040dbaed1776ba4b35b43, SnowGem)
