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

#pragma once

#include <GameEffectSystem/IGameEffectSystem.h>
#include <GameEffectSystem/GameEffects/GameEffectBase.h>
#include <IItemSystem.h>

#include "LightningGameEffectCommon.h"

class CLightningRenderNode;

class CLightningGameEffect
    : public CGameEffect
{
public:
    typedef int TIndex;

    struct STarget
    {
        STarget();
        explicit STarget(const Vec3& position);
        explicit STarget(EntityId targetEntity);
        STarget(EntityId targetEntity, int slot, const char* attachment);

        Vec3 m_position;
        EntityId m_entityId;
        int m_characterAttachmentSlot;
        uint32 m_characterAttachmentNameCRC;
    };

private:
    DECLARE_TYPE(CLightningGameEffect, CGameEffect);
    static const int maxNumSparks = 24;

    struct SLightningSpark
    {
        CLightningRenderNode* m_renderNode;
        STarget m_emitter;
        STarget m_receiver;
        float m_timer;
    };

public:
    CLightningGameEffect();
    virtual ~CLightningGameEffect();

    void Initialize(const SGameEffectParams* gameEffectParams = NULL) override;
    const char* GetName() const override;
    void Update(float frameTime) override;
    void ClearSparks();

    void LoadData(IItemParamsNode* params) override;
    void UnloadData() override;

    TIndex TriggerSpark(const char* presetName, _smart_ptr<IMaterial>  pMaterial, const STarget& emitter, const STarget& receiver);
    void RemoveSpark(const TIndex spark);
    void SetEmitter(const TIndex spark, const STarget& target);
    void SetReceiver(const TIndex spark, const STarget& target);
    float GetSparkRemainingTime(const TIndex spark) const;
    void SetSparkDeviationMult(const TIndex spark, float deviationMult);

private:
    int FindEmptySlot() const;
    int FindPreset(const char* name) const;
    Vec3 ComputeTargetPosition(const STarget& target);

    std::vector<LightningArcParams> m_lightningParams;
    SLightningSpark m_sparks[maxNumSparks];
    LightningStats m_stats;
};
