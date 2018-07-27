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

// Description : Implements a damage behavior which spawn debris found in the
//               damaged model of the Animated parts

#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEDAMAGEBEHAVIORSPAWNDEBRIS_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEDAMAGEBEHAVIORSPAWNDEBRIS_H
#pragma once

class CVehiclePartAnimated;

class CVehicleDamageBehaviorSpawnDebris
    : public IVehicleDamageBehavior
{
    IMPLEMENT_VEHICLEOBJECT
public:

    CVehicleDamageBehaviorSpawnDebris();
    ~CVehicleDamageBehaviorSpawnDebris();

    virtual bool Init(IVehicle* pVehicle, const CVehicleParams& table);
    virtual void Reset();
    virtual void Release() { delete this; }
    virtual void Serialize(TSerialize ser, EEntityAspects aspects) {}
    virtual void Update(const float deltaTime);
    virtual void OnDamageEvent(EVehicleDamageBehaviorEvent event, const SVehicleDamageBehaviorEventParams& params);
    virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params);

    virtual void GetMemoryUsage(ICrySizer* s) const;

protected:

    IEntity* SpawnDebris(IStatObj* pStatObj, Matrix34 vehicleTM, float force);
    void AttachParticleEffect(IEntity* pDetachedEntity);
    void GiveImpulse(IEntity* pDetachedEntity, float force);

    IVehicle* m_pVehicle;
    string m_particleEffect;

    typedef std::list <EntityId> TEntityIdList;
    TEntityIdList m_spawnedDebris;

    struct SDebrisInfo
    {
        CVehiclePartAnimated* pAnimatedPart;
        int jointId; // id of the joint in the intact model
        int slot;
        int index;
        EntityId entityId;
        float time;
        float force;
        void GetMemoryUsage(class ICrySizer* pSizer) const {}
    };

    typedef std::list <SDebrisInfo> TDebrisInfoList;
    TDebrisInfoList m_debris;
    bool m_pickableDebris;
};

#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEDAMAGEBEHAVIORSPAWNDEBRIS_H
