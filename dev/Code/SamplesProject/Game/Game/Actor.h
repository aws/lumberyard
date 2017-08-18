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
#include "IActorSystem.h"
namespace LYGame
{
    enum class EActorClassType
    {
        Actor = 0,
    };

    /// #TODO
    /// The Actor class provided in this template is a starting point for constructing
    /// the base player entity for your game. Your next steps should be to either fill out
    /// this class with game features, or to construct GameObjectExtensions that are responsible
    /// for game features you want.
    /// #TODO
    /// This actor is also an IGameObjectView to simplify setup of a project.
    /// It is recommended that you create a GameObjectExtension or separate actor to
    /// function as your camera instead.
    class Actor
        : public CGameObjectExtensionHelper<Actor, IActor>
        , public IGameObjectView
    {
    public:

        DECLARE_COMPONENT_TYPE("Actor", 0x2F05C6D2EF9B4D50, 0x8D7E831294783AE2)
        Actor();
        virtual ~Actor();

        // IGameObjectExtension overrides.
        bool Init(IGameObject* pGameObject) override;
        void PostInit(IGameObject* pGameObject) override;
        bool ReloadExtension(IGameObject* gameObject, const SEntitySpawnParams& spawnParams) override;
        const void* GetRMIBase() const override;

        // IActor overrides.
        bool IsPlayer() const override { return m_isPlayer; }
        bool IsClient() const override { return m_isClientActor; }
        const char* GetActorClassName() const override { return "Actor"; }
        ActorClass GetActorClass() const override { return (ActorClass)EActorClassType::Actor; }
        const char* GetEntityClassName() const override;

        // IGameObjectView overrides.
        void UpdateView(SViewParams& params) override;
        void PostUpdateView(SViewParams& params) override;

        // IGameObjectExtension stubs. Fill these out as necessary for your project.
        void GetMemoryUsage(ICrySizer* sizer) const override {}
        void InitClient(ChannelId channelId) override {}
        void PostInitClient(ChannelId channelId) override {}
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
        void PostRemoteSpawn() override {}
        void SetAuthority(bool hasAuthority) override {}

    private:
        bool m_isClientActor;
        bool m_isPlayer;
    };
}

