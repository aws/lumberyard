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

// Description : Implements a damage behavior which detach a part with its
//               children


#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEDAMAGEBEHAVIORDETACHPART_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEDAMAGEBEHAVIORDETACHPART_H
#pragma once

class CVehicle;
struct IParticleEffect;

class CVehicleDamageBehaviorDetachPart
    : public IVehicleDamageBehavior
{
    IMPLEMENT_VEHICLEOBJECT
public:

    CVehicleDamageBehaviorDetachPart();
    virtual ~CVehicleDamageBehaviorDetachPart();

    virtual bool Init(IVehicle* pVehicle, const CVehicleParams& table);
    virtual bool Init(IVehicle* pVehicle, const string& partName, const string& effectName);
    virtual void Reset();
    virtual void Release() { delete this; }

    virtual void OnDamageEvent(EVehicleDamageBehaviorEvent event, const SVehicleDamageBehaviorEventParams& behaviorParams);

    virtual void Serialize(TSerialize ser, EEntityAspects aspects);
    virtual void Update(const float deltaTime) {}

    virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params);

    virtual void GetMemoryUsage(ICrySizer* s) const;

protected:

    IEntity* SpawnDetachedEntity();
    bool MovePartToTheNewEntity(IEntity* pTargetEntity, CVehiclePartBase* pPart);
    void AttachParticleEffect(IEntity* pDetachedEntity, IParticleEffect* pEffect);

    CVehicle* m_pVehicle;
    string m_partName;

    typedef std::pair<CVehiclePartBase*, IStatObj*> TPartObjectPair;
    typedef std::vector<TPartObjectPair> TDetachedStatObjs;
    TDetachedStatObjs m_detachedStatObjs;

    EntityId m_detachedEntityId;

    IParticleEffect* m_pEffect;
    bool m_pickableDebris;
    bool m_notifyMovement;
};

#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEDAMAGEBEHAVIORDETACHPART_H
