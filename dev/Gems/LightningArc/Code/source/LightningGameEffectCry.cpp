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
#include "LightningGameEffectCry.h"
#include "LightningNode.h"
#include "IAttachment.h"
#include <LightningArc/LightningArcBus.h>

CLightningGameEffect::STarget::STarget()
    : m_position(ZERO)
    , m_entityId(0)
    , m_characterAttachmentSlot(-1)
    , m_characterAttachmentNameCRC(0)
{
}

CLightningGameEffect::STarget::STarget(const Vec3& position)
    : m_position(position)
    , m_entityId(0)
    , m_characterAttachmentSlot(-1)
    , m_characterAttachmentNameCRC(0)
{
}

CLightningGameEffect::STarget::STarget(EntityId targetEntity)
    : m_position(ZERO)
    , m_entityId(targetEntity)
    , m_characterAttachmentSlot(-1)
    , m_characterAttachmentNameCRC(0)
{
}

CLightningGameEffect::STarget::STarget(EntityId targetEntity, int slot, const char* attachment)
    : m_position(ZERO)
    , m_entityId(targetEntity)
    , m_characterAttachmentSlot(slot)
    , m_characterAttachmentNameCRC(CCrc32::ComputeLowercase(attachment))
{
}

CLightningGameEffect::CLightningGameEffect()
{
    for (int i = 0; i < maxNumSparks; ++i)
    {
        m_sparks[i].m_renderNode = nullptr;
        m_sparks[i].m_timer = 0;
    }

    SetFlag(GAME_EFFECT_AUTO_UPDATES_WHEN_ACTIVE, true);
}

CLightningGameEffect::~CLightningGameEffect()
{
    ClearSparks();
}

void CLightningGameEffect::Initialize(const SGameEffectParams* gameEffectParams /*= NULL*/)
{
    CGameEffect::Initialize(gameEffectParams);
    LoadData(nullptr);
}

void CLightningGameEffect::LoadData(IItemParamsNode*)
{
    const char* fileLocation = "Libs/LightningArc/LightningArcEffects.xml";
    XmlNodeRef rootNode = gEnv->pSystem->LoadXmlFromFile(fileLocation);

    if (!rootNode)
    {
        CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "Could not load lightning data. Invalid XML file '%s'! ", fileLocation);
        return;
    }

    m_lightningParams.clear();

    int numPresets = rootNode->getChildCount();
    m_lightningParams.resize(numPresets);

    for (int i = 0; i < numPresets; ++i)
    {
        XmlNodeRef preset = rootNode->getChild(i);
        const char* presetName = preset->getAttr("name");
        m_lightningParams[i].m_nameCRC = CCrc32::ComputeLowercase(presetName);
        m_lightningParams[i].Reset(preset);
    }
}

void CLightningGameEffect::UnloadData()
{
    ClearSparks();
    m_lightningParams.clear();
    stl::free_container(m_lightningParams);
}

const char* CLightningGameEffect::GetName() const
{
    return "Lightning";
}

void CLightningGameEffect::Update(float frameTime)
{
    CGameEffect::Update(frameTime);

    bool keepUpdating = false;

    m_stats.Restart();

    for (int i = 0; i < maxNumSparks; ++i)
    {
        if (m_sparks[i].m_renderNode)
        {
            m_sparks[i].m_renderNode->AddStats(&m_stats);
        }
        if (m_sparks[i].m_timer <= 0.0f)
        {
            continue;
        }
        m_sparks[i].m_timer -= frameTime;
        m_sparks[i].m_emitter.m_position = ComputeTargetPosition(m_sparks[i].m_emitter);
        m_sparks[i].m_receiver.m_position = ComputeTargetPosition(m_sparks[i].m_receiver);
        m_sparks[i].m_renderNode->SetEmiterPosition(m_sparks[i].m_emitter.m_position);
        m_sparks[i].m_renderNode->SetReceiverPosition(m_sparks[i].m_receiver.m_position);
        if (m_sparks[i].m_timer <= 0.0f)
        {
            gEnv->p3DEngine->UnRegisterEntityDirect(m_sparks[i].m_renderNode);
        }
        keepUpdating = true;

        m_stats.m_activeSparks.Increment();
    }

    ICVar* gameFXLightningProfile = gEnv->pConsole->GetCVar("g_gameFXLightningProfile");
    if (gameFXLightningProfile && gameFXLightningProfile->GetIVal())
    {
        m_stats.Draw();
    }

    SetActive(keepUpdating);
}

CLightningGameEffect::TIndex CLightningGameEffect::TriggerSpark(const char* presetName, _smart_ptr<IMaterial>  pMaterial,
    const STarget& emitter, const STarget& receiver)
{
    int preset = FindPreset(presetName);
    if (preset == -1)
    {
        CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "Could not find lightning preset '%s'", presetName);
        return -1;
    }

    int idx = FindEmptySlot();

    if (!m_sparks[idx].m_renderNode)
    {
        m_sparks[idx].m_renderNode = new CLightningRenderNode();
    }
    gEnv->p3DEngine->UnRegisterEntityDirect(m_sparks[idx].m_renderNode);
    m_sparks[idx].m_renderNode->Reset();
    m_sparks[idx].m_renderNode->SetMaterial(pMaterial);
    m_sparks[idx].m_renderNode->SetLightningParams(m_lightningParams[preset]);
    SetEmitter(TIndex(idx), emitter);
    SetReceiver(TIndex(idx), receiver);
    m_sparks[idx].m_timer = m_sparks[idx].m_renderNode->TriggerSpark();
    gEnv->p3DEngine->RegisterEntity(m_sparks[idx].m_renderNode);

    SetActive(true);

    return TIndex(idx);
}

void CLightningGameEffect::RemoveSpark(const TIndex spark)
{
    IF_UNLIKELY (spark == -1)
    {
        return;
    }

    IF_UNLIKELY ((spark < 0) || (spark >= maxNumSparks))
    {
        assert(false);
        return;
    }

    if (m_sparks[spark].m_timer > 0.0f)
    {
        gEnv->p3DEngine->UnRegisterEntityDirect(m_sparks[spark].m_renderNode);
        m_sparks[spark].m_timer = 0.0f;
    }
}

void CLightningGameEffect::SetEmitter(TIndex spark, const STarget& target)
{
    if (spark != -1 && m_sparks[spark].m_renderNode != 0)
    {
        m_sparks[spark].m_emitter = target;
        Vec3 targetPos = ComputeTargetPosition(target);
        m_sparks[spark].m_emitter.m_position = targetPos;
        m_sparks[spark].m_renderNode->SetEmiterPosition(targetPos);
    }
}

void CLightningGameEffect::SetReceiver(TIndex spark, const STarget& target)
{
    if (spark != -1 && m_sparks[spark].m_renderNode != 0)
    {
        m_sparks[spark].m_receiver = target;
        Vec3 targetPos = ComputeTargetPosition(target);
        m_sparks[spark].m_receiver.m_position = targetPos;
        m_sparks[spark].m_renderNode->SetReceiverPosition(targetPos);
    }
}

float CLightningGameEffect::GetSparkRemainingTime(TIndex spark) const
{
    if (spark != -1)
    {
        return max(m_sparks[spark].m_timer, 0.0f);
    }
    return 0.0f;
}

void CLightningGameEffect::SetSparkDeviationMult(const TIndex spark, float deviationMult)
{
    if (spark != -1 && m_sparks[spark].m_renderNode != 0)
    {
        m_sparks[spark].m_renderNode->SetSparkDeviationMult(deviationMult);
    }
}

int CLightningGameEffect::FindEmptySlot() const
{
    float lowestTime = m_sparks[0].m_timer;
    int bestIdex = 0;
    for (int i = 0; i < maxNumSparks; ++i)
    {
        if (m_sparks[i].m_timer <= 0.0f)
        {
            return i;
        }
        if (m_sparks[i].m_timer <= lowestTime)
        {
            lowestTime = m_sparks[i].m_timer;
            bestIdex = i;
        }
    }
    return bestIdex;
}

int CLightningGameEffect::FindPreset(const char* name) const
{
    uint32 crc = CCrc32::ComputeLowercase(name);
    for (size_t i = 0; i < m_lightningParams.size(); ++i)
    {
        if (m_lightningParams[i].m_nameCRC == crc)
        {
            return int(i);
        }
    }
    return -1;
}

Vec3 CLightningGameEffect::ComputeTargetPosition(const STarget& target)
{
    Vec3 result = target.m_position;
    IEntity* pEntity = gEnv->pEntitySystem->GetEntity(target.m_entityId);

    if (pEntity)
    {
        AABB worldBounds;
        pEntity->GetWorldBounds(worldBounds);
        result = worldBounds.GetCenter();

        if (target.m_characterAttachmentSlot != -1)
        {
            ICharacterInstance* pCharacter = pEntity->GetCharacter(target.m_characterAttachmentSlot);
            if (pCharacter)
            {
                IAttachmentManager* pAttachmentManager = pCharacter->GetIAttachmentManager();
                IAttachment* pAttachment =
                    pAttachmentManager->GetInterfaceByNameCRC(target.m_characterAttachmentNameCRC);
                if (pAttachment)
                {
                    result = pAttachment->GetAttWorldAbsolute().t;
                }
            }
        }
    }
    return result;
}

void CLightningGameEffect::ClearSparks()
{
    for (int i = 0; i < maxNumSparks; ++i)
    {
        if (m_sparks[i].m_renderNode)
        {
            gEnv->p3DEngine->UnRegisterEntityDirect(m_sparks[i].m_renderNode);
            m_sparks[i].m_timer = 0.0f;
            delete m_sparks[i].m_renderNode;
            m_sparks[i].m_renderNode = nullptr;
        }
    }
}
