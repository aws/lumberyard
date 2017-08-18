
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
        DECLARE_COMPONENT_TYPE("Actor", 0x73A5880EF9B1428A, 0x912CA1E8E9977AD8)
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

