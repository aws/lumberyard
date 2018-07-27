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

// Description : Implements an effect damage behavior

#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEDAMAGEBEHAVIOREFFECT_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEDAMAGEBEHAVIOREFFECT_H
#pragma once

#include "SharedParams/ISharedParams.h"

class CVehicleDamageBehaviorEffect
    : public IVehicleDamageBehavior
{
    IMPLEMENT_VEHICLEOBJECT

public:

    // IVehicleDamageBehavior

    virtual bool Init(IVehicle* pVehicle, const CVehicleParams& table);
    virtual void Reset();
    virtual void Release() { delete this; }
    virtual void OnDamageEvent(EVehicleDamageBehaviorEvent event, const SVehicleDamageBehaviorEventParams& behaviorParams);
    virtual void GetMemoryUsage(ICrySizer* pSizer) const;

    // ~IVehicleDamageBehavior

    // IVehicleObject

    virtual void Serialize(TSerialize ser, EEntityAspects aspects);
    virtual void Update(const float deltaTime);

    // ~IVehicleObject

    // IVehicleEventListener

    virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params) {}

    // ~IVehicleEventListener

protected:

    BEGIN_SHARED_PARAMS(SSharedParams)

    string  effectName;

    float       damageRatioMin;

    bool        disableAfterExplosion;
    bool        updateFromHelper;

    END_SHARED_PARAMS

    void LoadEffect(IVehicleComponent* pComponent);
    void UpdateEffect(float randomness, float damageRatio);

    IVehicle* m_pVehicle;

    SSharedParamsConstPtr       m_pSharedParams;

    const SDamageEffect* m_pDamageEffect;

    int                                         m_slot;
};

#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEDAMAGEBEHAVIOREFFECT_H
