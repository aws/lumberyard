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

// Description : Implements an entity class for detached parts


#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTDETACHEDENTITY_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTDETACHEDENTITY_H
#pragma once

#include "IGameObject.h"

class CVehiclePartDetachedEntity
    : public CGameObjectExtensionHelper<CVehiclePartDetachedEntity, IGameObjectExtension>
{
public:
    DECLARE_COMPONENT_TYPE("VehiclePartDetachedEntity", 0x43ACA2D343A4451D, 0x975CA137274C971D)

    CVehiclePartDetachedEntity();
    ~CVehiclePartDetachedEntity();

    virtual bool Init(IGameObject* pGameObject);
    virtual void InitClient(ChannelId channelId) {};
    virtual void PostInit(IGameObject*);
    virtual void PostInitClient(ChannelId channelId) {};
    virtual bool ReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params);
    virtual void PostReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params) {}
    virtual bool GetEntityPoolSignature(TSerialize signature);

    virtual void FullSerialize(TSerialize ser);
    virtual bool NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags) { return true; }
    virtual void PostSerialize() {}
    virtual void SerializeSpawnInfo(TSerialize ser, IEntity* entity) {}
    virtual ISerializableInfoPtr GetSpawnInfo() {return 0; }
    virtual void Update(SEntityUpdateContext& ctx, int slot);
    virtual void HandleEvent(const SGameObjectEvent& event);
    virtual void ProcessEvent(SEntityEvent& event);
    virtual void SetChannelId(ChannelId id) {};
    virtual void SetAuthority(bool auth) {}
    virtual void PostUpdate(float frameTime) { CRY_ASSERT(false); }
    virtual void PostRemoteSpawn() {};
    virtual void GetMemoryUsage(ICrySizer* pSizer) const { pSizer->Add(*this); }

protected:
    void InitVehiclePart(IGameObject* pGameObject);

    float m_timeUntilStartIsOver;
};

#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTDETACHEDENTITY_H
