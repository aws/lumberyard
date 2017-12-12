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
#include <IEntityClass.h>
#include "LightningArcGem.h"
#include <FlowSystem/Nodes/FlowBaseNode.h>
#include "LightningArc.h"
#include "LightningGameEffect.h"
#include "ScriptBind_LightningArc.h"

LightningArcGem::LightningArcGem()
{
    LightningArcRequestBus::Handler::BusConnect();
}

LightningArcGem::~LightningArcGem()
{
    LightningArcRequestBus::Handler::BusDisconnect();
}

void LightningArcGem::PostSystemInit()
{
    REGISTER_CVAR(g_gameFXLightningProfile, 0, 0, "Toggles game effects system lightning arc profiling");

    // Init GameEffect
    m_gameEffect = new CLightningGameEffect();
    m_gameEffect->Initialize();

    // Init ScriptBind
    m_lightningArcScriptBind = new CScriptBind_LightningArc(GetISystem());

    // Init GameObjectExtension
    // Originally registered with REGISTER_GAME_OBJECT(pFramework, LightningArc, "Scripts/Entities/Environment/LightningArc.lua");
    // If more objects need registered, consider bringing the macro back along with the GameFactory wrapper.
    IEntityClassRegistry::SEntityClassDesc clsDesc;
    clsDesc.sName = "LightningArc";
    clsDesc.sScriptFile = "Scripts/Entities/Environment/LightningArc.lua";
    static CLightningArcCreator _creator;
    GetISystem()->GetIGame()->GetIGameFramework()->GetIGameObjectSystem()->RegisterExtension("LightningArc", &_creator, &clsDesc);
}

void LightningArcGem::Shutdown()
{
    if (m_gameEffect)
    {
        m_gameEffect->Release();
    }
    SAFE_DELETE(m_gameEffect);
    SAFE_DELETE(m_lightningArcScriptBind);
}

void LightningArcGem::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
    switch (event)
    {
    case ESYSTEM_EVENT_GAME_POST_INIT:
        IComponentFactoryRegistry::RegisterAllComponentFactoryNodes(*gEnv->pEntitySystem->GetComponentFactoryRegistry());
        GameEffectSystemNotificationBus::Handler::BusConnect();
        break;

    case ESYSTEM_EVENT_FLOW_SYSTEM_REGISTER_EXTERNAL_NODES:
        RegisterExternalFlowNodes();
        break;

    // Called on ESYSTEM_EVENT_GAME_POST_INIT_DONE instead of ESYSTEM_EVENT_GAME_POST_INIT because the GameEffectSystem Gem
    // uses ESYSTEM_EVENT_GAME_POST_INIT to initialize, and this requires that has happened already.
    case ESYSTEM_EVENT_GAME_POST_INIT_DONE:
        PostSystemInit();
        break;

    case ESYSTEM_EVENT_FULL_SHUTDOWN:
    case ESYSTEM_EVENT_FAST_SHUTDOWN:
        GameEffectSystemNotificationBus::Handler::BusDisconnect();
        Shutdown();
        break;
    }
}

CScriptBind_LightningArc* LightningArcGem::GetScriptBind() const
{
    return m_lightningArcScriptBind;
}

CLightningGameEffect* LightningArcGem::GetGameEffect() const
{
    return m_gameEffect;
}

void LightningArcGem::OnReleaseGameEffects()
{
    Shutdown();
}

AZ_DECLARE_MODULE_CLASS(LightningArc_4c28210b23544635aa15be668dbff15d, LightningArcGem)
