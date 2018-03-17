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
#include "LightningArc_precompiled.h"
#include "LightningArc.h"
#include "ScriptBind_LightningArc.h"
#include "LightningGem.h"
#include "LightningGameEffectCry.h"

DECLARE_DEFAULT_COMPONENT_FACTORY(CLightningArc, CLightningArc)

void CLightningArc::GetMemoryUsage(ICrySizer* pSizer) const {}
void CLightningArc::PostInit(IGameObject* pGameObject) {}
void CLightningArc::InitClient(ChannelId channelId) {}
void CLightningArc::PostInitClient(ChannelId channelId) {}
bool CLightningArc::ReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params)
{
    return true;
}
void CLightningArc::PostReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params)
{
}
bool CLightningArc::GetEntityPoolSignature(TSerialize signature) { return true; }
void CLightningArc::FullSerialize(TSerialize ser) {}
bool CLightningArc::NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int pflags)
{
    return true;
}
void CLightningArc::PostSerialize() {}
void CLightningArc::SerializeSpawnInfo(TSerialize, IEntity*) {}
ISerializableInfoPtr CLightningArc::GetSpawnInfo() { return ISerializableInfoPtr(); }
void CLightningArc::HandleEvent(const SGameObjectEvent& event) {}
void CLightningArc::SetChannelId(ChannelId id) {}
void CLightningArc::SetAuthority(bool auth) {}
const void* CLightningArc::GetRMIBase() const { return 0; }
void CLightningArc::PostUpdate(float frameTime) {}
void CLightningArc::PostRemoteSpawn() {}

CLightningArc::CLightningArc()
    : m_enabled(true)
    , m_delay(5.0f)
    , m_delayVariation(0.0f)
    , m_timer(0.0f)
    , m_inGameMode(!gEnv->IsEditor())
    , m_lightningPreset(NULL)
{
}

CLightningArc::~CLightningArc()
{
    // The LightningArc may be active during destruction (if the game is being simulated in the editor).
    // Make sure that the sparks are all properly cleaned up before removing this object.
    Enable(false);
}

bool CLightningArc::Init(IGameObject* pGameObject)
{
    SetGameObject(pGameObject);

    CScriptBind_LightningArc* scriptBind = nullptr;
    EBUS_EVENT_RESULT(scriptBind, LightningArcRequestBus, GetScriptBind);
    scriptBind->AttachTo(this);

    ReadLuaParameters();

    return true;
}

void CLightningArc::ProcessEvent(SEntityEvent& event)
{
    switch (event.event)
    {
    case ENTITY_EVENT_LEVEL_LOADED:
        Enable(m_enabled);
        break;
    case ENTITY_EVENT_RESET:
        Reset(event.nParam[0] != 0);
        break;

    case ENTITY_EVENT_MATERIAL:
    {
        if (m_inGameMode)
        {
            // The material on the this arc has been changed. If the game is currently running then that means the current sparks
            // have been invalidated and should be destroyed.
            CLightningGameEffect* gameEffect = nullptr;
            EBUS_EVENT_RESULT(gameEffect, LightningArcRequestBus, GetGameEffect);
            if (gameEffect)
            {
                gameEffect->ClearSparks();
            }
        }
    }
    break;

    default:
        break;
    }
}

void CLightningArc::Update(SEntityUpdateContext& ctx, int updateSlot)
{
    if (!m_enabled || !m_inGameMode)
    {
        return;
    }

    m_timer -= ctx.fFrameTime;

    if (m_timer < 0.0f)
    {
        TriggerSpark();
        m_timer += m_delay + (cry_frand() * 0.5f + 0.5f) * m_delayVariation;
    }
}

void CLightningArc::TriggerSpark()
{
    const char* targetName = "Target";
    IEntity* pEntity = GetEntity();

    if (pEntity->GetMaterial() == 0)
    {
        CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING,
            "Lightning arc '%s' has no Material, no sparks will trigger", pEntity->GetName());
        return;
    }

    IEntityLink* pLinks = pEntity->GetEntityLinks();
    int numLinks = 0;
    for (IEntityLink* link = pLinks; link; link = link->next)
    {
        if (strcmp(link->name, targetName) == 0)
        {
            ++numLinks;
        }
    }

    if (numLinks == 0)
    {
        CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING,
            "Lightning arc '%s' has no Targets, no sparks will trigger", pEntity->GetName());
        return;
    }

    int nextSpark = cry_random<int>(0, numLinks - 1);
    IEntityLink* pNextSparkLink = pLinks;
    for (; nextSpark && pNextSparkLink; pNextSparkLink = pNextSparkLink->next)
    {
        if (strcmp(pNextSparkLink->name, targetName) == 0)
        {
            --nextSpark;
        }
    }

    assert(pNextSparkLink);
    PREFAST_ASSUME(pNextSparkLink);

    CLightningGameEffect* gameEffect = nullptr;
    EBUS_EVENT_RESULT(gameEffect, LightningArcRequestBus, GetGameEffect);

    CLightningGameEffect::TIndex id = gameEffect->TriggerSpark(m_lightningPreset, pEntity->GetMaterial(),
            CLightningGameEffect::STarget(GetEntityId()),
            CLightningGameEffect::STarget(pNextSparkLink->entityId));

    float strikeTime = gameEffect->GetSparkRemainingTime(id);

    IScriptTable* pTargetScriptTable = nullptr;
    IEntity* pTarget = pNextSparkLink
        ? gEnv->pEntitySystem->GetEntity(pNextSparkLink->entityId)
        : nullptr;

    if (pTarget)
    {
        pTargetScriptTable = pTarget->GetScriptTable();
    }

    Script::CallMethod(pEntity->GetScriptTable(), "OnStrike", strikeTime, pTargetScriptTable);
}

void CLightningArc::Enable(bool enable)
{
    if (m_enabled != enable)
    {
        m_timer = 0.0f;
    }
    m_enabled = enable;

    if (m_enabled && m_inGameMode)
    {
        GetGameObject()->EnableUpdateSlot(this, 0);
    }
    else
    {
        GetGameObject()->DisableUpdateSlot(this, 0);

        // Clear all sparks when the lightning arc is disabled. The sparks keep pointers to
        // information such as the material on the lightning arc. If you enter game mode, exit game mode,
        // and a spark hangs around in edit mode, and then you change the material on the lightning arc,
        // that pointer becomes invalid. Clearing out the sparks prevents this.
        CLightningGameEffect* gameEffect = nullptr;
        EBUS_EVENT_RESULT(gameEffect, LightningArcRequestBus, GetGameEffect);
        if (gameEffect)
        {
            gameEffect->ClearSparks();
        }
    }
}

void CLightningArc::Reset(bool jumpingIntoGame)
{
    m_inGameMode = jumpingIntoGame;
    ReadLuaParameters();
}

void CLightningArc::ReadLuaParameters()
{
    SmartScriptTable pScriptTable = GetEntity()->GetScriptTable();
    if (!pScriptTable)
    {
        return;
    }

    SmartScriptTable pProperties;
    SmartScriptTable pTiming;
    SmartScriptTable pRender;
    if (!pScriptTable->GetValue("Properties", pProperties))
    {
        return;
    }
    if (!pProperties->GetValue("Timing", pTiming))
    {
        return;
    }
    if (!pProperties->GetValue("Render", pRender))
    {
        return;
    }

    pProperties->GetValue("bActive", m_enabled);
    Enable(m_enabled);

    pTiming->GetValue("fDelay", m_delay);
    pTiming->GetValue("fDelayVariation", m_delayVariation);

    pRender->GetValue("ArcPreset", m_lightningPreset);
}
