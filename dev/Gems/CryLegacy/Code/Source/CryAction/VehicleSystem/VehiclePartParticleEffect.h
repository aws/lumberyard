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

// Description : Vehicle part class that spawns a particle effect and attaches it to the vehicle.


#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTPARTICLEEFFECT_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTPARTICLEEFFECT_H
#pragma once

#include "IGameObject.h"
#include "VehiclePartBase.h"

class CVehiclePartParticleEffect
    : public CVehiclePartBase
{
public:

    IMPLEMENT_VEHICLEOBJECT

    CVehiclePartParticleEffect();

    ~CVehiclePartParticleEffect();

    virtual bool Init(IVehicle* pVehicle, const CVehicleParams& table, IVehiclePart* parent, CVehicle::SPartInitInfo& initInfo, int partType);
    virtual void PostInit();
    virtual void Reset();
    virtual void Update(const float frameTime);
    virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params);
    // PS - May need to implement SetLocalTM(), GetLocalTM(), GetWorldTM() and GetLocalBounds here, but that would require quite a lot of extra storage and calculations.
    virtual void GetMemoryUsage(ICrySizer* pSizer) const;

    IEntity* GetPartEntity() const;

private:

    void ActivateParticleEffect(bool activate);

    int                         m_id;

    string                  m_particleEffectName, m_helperName;

    IParticleEffect* m_pParticleEffect;

    IVehicleHelper* m_pHelper;
};

#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTPARTICLEEFFECT_H
