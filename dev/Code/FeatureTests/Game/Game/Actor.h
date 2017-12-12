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

    /*!
     * #TODO
     * The Actor class provided in this template is a starting point for constructing
     * the base player entity for your game. Your next steps should be to either fill out
     * this class with game features, or to construct GameObjectExtensions that are responsible
     * for game features you want.
     *
     * #TODO
     * This actor is also an IGameObjectView to simplify setup of a project.
     * It is recommended that you create a GameObjectExtension or separate actor to
     * function as your camera instead.
     */
    class Actor
        : public CGameObjectExtensionHelper<Actor, IActor>
        , public IGameObjectView
    {
    public:
        DECLARE_COMPONENT_TYPE("Actor", 0x9117C14B91B144BE, 0xB8A9AD58870DDE78)
        Actor();
        virtual ~Actor();

        //////////////////////////////////////////////////////////////////////////
        //! IGameObjectExtension
        bool Init(IGameObject* pGameObject) override;
        void PostInit(IGameObject* pGameObject) override;
        bool ReloadExtension(IGameObject* gameObject, const SEntitySpawnParams& spawnParams) override;
        const void* GetRMIBase() const override;
        void ProcessEvent(SEntityEvent& event) override { }
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        //! IActor
        bool IsPlayer() const override { return m_isPlayer; }
        bool IsClient() const override { return m_isClientActor; }
        const char* GetActorClassName() const override { return "Actor"; }
        ActorClass GetActorClass() const override { return (ActorClass)EActorClassType::Actor; }
        const char* GetEntityClassName() const override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        //! IGameObjectView
        void UpdateView(SViewParams& params) override;
        void PostUpdateView(SViewParams& params) override;
        //////////////////////////////////////////////////////////////////////////

    private:
        bool m_isClientActor;
        bool m_isPlayer;
    };
} // namespace LYGame

