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

#ifndef CRYINCLUDE_CRYACTION_GAMEOBJECTS_MANNEQUINOBJECT_H
#define CRYINCLUDE_CRYACTION_GAMEOBJECTS_MANNEQUINOBJECT_H
#pragma once

#include <IGameObject.h>
#include "IAnimationGraph.h"

class CMannequinObject
    : public CGameObjectExtensionHelper<CMannequinObject, IGameObjectExtension>
{
public:
    DECLARE_COMPONENT_TYPE("MannequinObject", 0x3D14366EDABE4456, 0x8CAA3F993E6B3A9D)

    CMannequinObject();
    virtual ~CMannequinObject();

    // IGameObjectExtension
    virtual bool Init(IGameObject* pGameObject);
    virtual void InitClient(ChannelId channelId) {}
    virtual void PostInit(IGameObject* pGameObject);
    virtual void PostInitClient(ChannelId channelId) {}
    virtual bool ReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params) { return false; }
    virtual void PostReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params) {}
    virtual bool GetEntityPoolSignature(TSerialize signature) { return false; }
    virtual void FullSerialize(TSerialize ser) {}
    virtual bool NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags) { return true; }
    virtual void PostSerialize() {}
    virtual void SerializeSpawnInfo(TSerialize ser, IEntity* entity) {}
    virtual ISerializableInfoPtr GetSpawnInfo() { return 0; }
    virtual void Update(SEntityUpdateContext& ctx, int updateSlot) {}
    virtual void PostUpdate(float frameTime) {}
    virtual void PostRemoteSpawn() {}
    virtual void HandleEvent(const SGameObjectEvent& event) {}
    virtual void ProcessEvent(SEntityEvent& event);
    virtual void SetChannelId(ChannelId id) {}
    virtual void SetAuthority(bool auth) {}
    virtual void GetMemoryUsage(ICrySizer* s) const { s->Add(*this); }
    // ~IGameObjectExtension

protected:
    void Reset();
    void OnScriptEvent(const char* eventName);

private:
    IAnimatedCharacter* m_pAnimatedCharacter;
};

#endif // CRYINCLUDE_CRYACTION_GAMEOBJECTS_MANNEQUINOBJECT_H
