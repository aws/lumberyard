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

// Description : Implements a damage behavior which send an AI signal


#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEDAMAGEBEHAVIORAISIGNAL_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEDAMAGEBEHAVIORAISIGNAL_H
#pragma once

class CVehicle;

class CVehicleDamageBehaviorAISignal
    : public IVehicleDamageBehavior
{
    IMPLEMENT_VEHICLEOBJECT
public:

    CVehicleDamageBehaviorAISignal() {}
    virtual ~CVehicleDamageBehaviorAISignal() {}

    virtual bool Init(IVehicle* pVehicle, const CVehicleParams& table);
    virtual void Reset();
    virtual void Release() { delete this; }

    virtual void Serialize(TSerialize ser, EEntityAspects aspects) {}
    virtual void Update(const float deltaTime);

    virtual void OnDamageEvent(EVehicleDamageBehaviorEvent event, const SVehicleDamageBehaviorEventParams& behaviorParams);
    virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params){}

    virtual void GetMemoryUsage(ICrySizer* s) const;
protected:

    void ActivateUpdate(bool activate);

    IVehicle* m_pVehicle;

    bool m_freeSignalRepeat;
    bool m_isActive;
    int m_signalId;
    float m_freeSignalRadius;
    float m_timeCounter;
    string m_signalText;
    string m_freeSignalText;
};

#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEDAMAGEBEHAVIORAISIGNAL_H
