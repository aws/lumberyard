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
#pragma once

#include <IGameRulesSystem.h>

namespace LYGame
{
    class GameplaySampleGameRules
        : public CGameObjectExtensionHelper < GameplaySampleGameRules, IGameRules >
    {
    public:
        GameplaySampleGameRules() { }
        virtual ~GameplaySampleGameRules();

        // IGameObjectExtension overrides.
        bool Init(IGameObject* pGameObject) override;
        void PostInit(IGameObject* pGameObject) override;

        // IGameRules overrides.
        bool OnClientConnect(ChannelId channelId, bool isReset) override;

        // IGameObjectExtension stubs. Fill these out as necessary for your project.
        void GetMemoryUsage(ICrySizer* sizer) const override {}
        void InitClient(ChannelId channelId) override {}
        void PostInitClient(ChannelId channelId) override {}
        bool ReloadExtension(IGameObject* gameObject, const SEntitySpawnParams& spawnParams) override { return true; }
        void PostReloadExtension(IGameObject* gameObject, const SEntitySpawnParams& spawnParams) override {}
        bool GetEntityPoolSignature(TSerialize signature) override { return false; }
        bool NetSerialize(TSerialize serialize, EEntityAspects aspect, uint8 profile, int pflags) override { return true; }
        void FullSerialize(TSerialize serializer) override {}
        void PostSerialize() override {}
        ISerializableInfoPtr GetSpawnInfo() override { return nullptr; }
        void SerializeSpawnInfo(TSerialize serialize, IEntity* entity) override { }
        void Update(SEntityUpdateContext& ctx, int updateSlot) override { }
        void ProcessEvent(SEntityEvent& event) override { }
        void HandleEvent(const SGameObjectEvent& event) override {}
        void PostUpdate(float frameTime) override { }
        void SetChannelId(ChannelId id) override {}
        const void* GetRMIBase() const override { return NULL; }
        void PostRemoteSpawn() override {}
        void SetAuthority(bool hasAuthority) override {}
    };
} // namespace LYGame