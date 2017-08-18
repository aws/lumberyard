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

// Description : implements a DamageIndicator behavior (lights, sounds,..)


#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEDAMAGEBEHAVIORINDICATOR_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEDAMAGEBEHAVIORINDICATOR_H
#pragma once

#include "IVehicleSystem.h"

class CVehicle;

class CVehicleDamageBehaviorIndicator
    : public IVehicleDamageBehavior
{
    IMPLEMENT_VEHICLEOBJECT
public:

    CVehicleDamageBehaviorIndicator();
    virtual ~CVehicleDamageBehaviorIndicator() {}

    virtual bool Init(IVehicle* pVehicle, const CVehicleParams& table);
    virtual void Reset();
    virtual void Release() { delete this; }

    virtual void OnDamageEvent(EVehicleDamageBehaviorEvent event, const SVehicleDamageBehaviorEventParams& behaviorParams);

    virtual void Serialize(TSerialize ser, EEntityAspects aspects);
    virtual void Update(const float deltaTime);

    virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params){}
    virtual void GetMemoryUsage(ICrySizer* s) const;

protected:
    void GetMaterial();
    void SetActive(bool active);
    void EnableLight(bool enable);

    CVehicle* m_pVehicle;

    bool m_isActive;
    float m_ratioMin;
    float m_ratioMax;
    float m_currentDamageRatio;

    // Light
    string m_material;
    _smart_ptr<IMaterial> m_pSubMtl;

    string m_sound;
    float m_soundRatioMin;
    IVehicleHelper* m_pHelper;

    int m_soundsPlayed;
    float m_lastDamageRatio;

    float m_lightUpdate;
    float m_lightTimer;
    bool m_lightOn;


    static float m_frequencyMin;
    static float m_frequencyMax;
};

#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEDAMAGEBEHAVIORINDICATOR_H
