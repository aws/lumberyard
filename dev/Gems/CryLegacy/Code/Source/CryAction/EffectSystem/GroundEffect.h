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

#ifndef CRYINCLUDE_CRYACTION_EFFECTSYSTEM_GROUNDEFFECT_H
#define CRYINCLUDE_CRYACTION_EFFECTSYSTEM_GROUNDEFFECT_H
#pragma once

#include "IEffectSystem.h"
#include "CryActionPhysicQueues.h"

struct IParticleEffect;

class CGroundEffect
    : public IGroundEffect
{
public:

    CGroundEffect(IEntity* pEntity);
    virtual ~CGroundEffect();

    // IGroundEffect
    virtual void SetHeight(float height);
    virtual void SetHeightScale(float sizeScale, float countScale);
    virtual void SetBaseScale(float sizeScale, float countScale, float speedScale = 1.0f);
    virtual void SetInterpolation(float speed);
    virtual void SetFlags(int flags);
    virtual int GetFlags() const;
    virtual bool SetParticleEffect(const char* pName);
    virtual void SetInteraction(const char* pName);
    virtual void Update();
    virtual void Stop(bool stop);
    // ~IGroundEffect

    void OnRayCastDataReceived(const QueuedRayID& rayID, const RayCastResult& result);

protected:

    void SetSpawnParams(const SpawnParams& params);
    void Reset();

    inline bool DebugOutput() const
    {
        static ICVar* pVar = gEnv->pConsole->GetCVar("g_groundeffectsdebug");

        CRY_ASSERT(pVar);

        return pVar->GetIVal() > 0;
    }

    inline bool DeferredRayCasts() const
    {
        static ICVar* pVar = gEnv->pConsole->GetCVar("g_groundeffectsdebug");

        CRY_ASSERT(pVar);

        return pVar->GetIVal() != 2;
    }

    IEntity* m_pEntity;

    IParticleEffect* m_pParticleEffect;

    int                         m_flags, m_slot, m_surfaceIdx, m_rayWorldIntersectSurfaceIdx;
    QueuedRayID         m_raycastID;

    string                  m_interaction;

    bool                        m_active                                    : 1;

    bool                        m_stopped                                   : 1;

    bool                        m_validRayWorldIntersect    : 1;

    float                       m_height, m_rayWorldIntersectHeight, m_ratio;

    float                   m_sizeScale, m_sizeGoal;

    float                   m_countScale;

    float                   m_speedScale, m_speedGoal;

    float                   m_interpSpeed;

    float                   m_maxHeightCountScale, m_maxHeightSizeScale;
};
#endif // CRYINCLUDE_CRYACTION_EFFECTSYSTEM_GROUNDEFFECT_H
