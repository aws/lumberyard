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

// Description : Implements an entity class which can serialize vehicle parts

#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLESEATSERIALIZER_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLESEATSERIALIZER_H
#pragma once

#include "IGameObject.h"

class CVehicle;
class CVehicleSeat;

class CVehicleSeatSerializer
    : public CGameObjectExtensionHelper<CVehicleSeatSerializer, IGameObjectExtension>
{
public:
    DECLARE_COMPONENT_TYPE("VehicleSeatSerializer", 0x55613BD179F5444B, 0x95E3DD727057319E)

    CVehicleSeatSerializer();
    virtual ~CVehicleSeatSerializer();

    virtual bool Init(IGameObject* pGameObject);
    virtual void InitClient(ChannelId channelId);
    virtual void PostInit(IGameObject*) {}
    virtual void PostInitClient(ChannelId channelId) {};
    virtual bool ReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params);
    virtual void PostReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params) {}
    virtual bool GetEntityPoolSignature(TSerialize signature);

    virtual void FullSerialize(TSerialize ser);
    virtual bool NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags);
    virtual void PostSerialize() {}
    virtual void SerializeSpawnInfo(TSerialize ser, IEntity* entity);
    virtual ISerializableInfoPtr GetSpawnInfo();
    virtual void Update(SEntityUpdateContext& ctx, int);
    virtual void HandleEvent(const SGameObjectEvent& event);
    virtual void ProcessEvent(SEntityEvent& event) {};
    virtual void SetChannelId(ChannelId id) {};
    virtual void SetAuthority(bool auth) {};
    virtual void PostUpdate(float frameTime) { CRY_ASSERT(false); };
    virtual void PostRemoteSpawn() {};
    virtual void GetMemoryUsage(ICrySizer* s) const {s->Add(*this); }

    void SetVehicle(CVehicle* pVehicle);
    void SetSeat(CVehicleSeat* pSeat);

protected:
    CVehicle* m_pVehicle;
    CVehicleSeat* m_pSeat;
};

#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLESEATSERIALIZER_H
