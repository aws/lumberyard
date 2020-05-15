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

/*
    A version of CLightningGameEffect that utilizes AZ types
    rather than legacy Cry types.
*/

#pragma once

#include <GameEffectSystem/IGameEffectSystem.h>
#include <GameEffectSystem/GameEffects/GameEffectBase.h>
#include <IItemSystem.h>

#include "LightningGameEffectCommon.h"

// Reasonable limits on the segment/point counts, above which we may experience crashes
#define MAX_STRIKE_SEGMENT_COUNT 70
#define MAX_STRIKE_POINT_COUNT 70

class CLightningRenderNode;

class LightningGameEffectAZ
    : public CGameEffect
{
public:
    struct Target
    {
        Target();
        explicit Target(const AZ::Vector3& position);
        explicit Target(AZ::EntityId targetEntity);

        AZ::Vector3 m_position;
        AZ::EntityId m_entityId;
    };

private:
    DECLARE_TYPE(LightningGameEffectAZ, CGameEffect);
    static const AZ::u32 maxNumSparks = 24;

    struct LightningSpark
    {
        CLightningRenderNode* m_renderNode;
        Target m_emitter;
        Target m_receiver;
        float m_timer;
    };

public:
    LightningGameEffectAZ();
    virtual ~LightningGameEffectAZ();

    void Initialize(const SGameEffectParams* gameEffectParams = nullptr) override;
    const char* GetName() const override;
    void Update(float frameTime) override;
    void ClearSparks();

    void UnloadData() override;

    AZ::s32 TriggerSpark(const LightningArcParams& params, _smart_ptr<IMaterial>  pMaterial, const Target& emitter, const Target& receiver);
    void RemoveSpark(const AZ::s32 spark);
    void SetEmitter(const AZ::s32 spark, const Target& target);
    void SetReceiver(const AZ::s32 spark, const Target& target);
    float GetSparkRemainingTime(const AZ::s32 spark) const;
    void SetSparkDeviationMult(const AZ::s32 spark, float deviationMult);

private:
    AZ::s32 FindEmptySlot() const;
    AZ::Vector3 ComputeTargetPosition(const Target& target);

    LightningSpark m_sparks[maxNumSparks];
    LightningStats m_stats;
};
