
#include "stdafx.h"
#include "Actor.h"
#include "IViewSystem.h"
#include "StringUtils.h"

namespace LYGame
{
    DECLARE_DEFAULT_COMPONENT_FACTORY(Actor, Actor)

    Actor::Actor()
        : m_isClientActor(false)
        , m_isPlayer(false)
    {
    }

    Actor::~Actor()
    {
        // Unregister from the actor system.
        if (gEnv->pGame && gEnv->pGame->GetIGameFramework())
        {
            if (auto* actorSystem = gEnv->pGame->GetIGameFramework()->GetIActorSystem())
            {
                gEnv->pGame->GetIGameFramework()->GetIActorSystem()->RemoveActor(GetEntityId());
            }
        }

        if (gEnv->pGame && GetEntityId() == gEnv->pGame->GetClientActorId())
        {
            gEnv->pGame->PlayerIdSet(0);
        }

        GetGameObject()->ReleaseView(this);

        if (gEnv->pGame->GetIGameFramework()->GetIActorSystem())
        {
            gEnv->pGame->GetIGameFramework()->GetIActorSystem()->RemoveActor(GetEntityId());
        }
    }

    // #TODO Put logic you need to run when this actor is initialized here.
    // See the documentation on the differences between the Init and PostInit steps
    // to make sure your initialization logic is in the right place.
    bool Actor::Init(IGameObject* pGameObject)
    {
        if (pGameObject == nullptr)
        {
            return false;
        }
        SetGameObject(pGameObject);
        if (!GetGameObject()->CaptureView(this))
        {
            return false;
        }

        // Are we the actor associated with the local client player?
        m_isClientActor = gEnv->pGame->GetIGameFramework()->GetClientActorId() == GetEntityId();

        IEntity* pEntity = GetEntity();
        IEntityClass* pEntityClass = pEntity->GetClass();

        // We're player-controlled if we have an associated channel ID.
        m_isPlayer = (GetChannelId() != kInvalidChannelId);

        // Register with the actor system.
        gEnv->pGame->GetIGameFramework()->GetIActorSystem()->AddActor(GetEntityId(), this);
        if (m_isClientActor)
        {
            gEnv->pGame->GetIGameFramework()->SetClientActor(GetEntityId());
        }

        if (!pGameObject->BindToNetwork())
        {
            return false;
        }

        return true;
    }

    // #TODO Put logic you need to run when this actor is post initialized here.
    // See the documentation on the differences between the Init and PostInit steps
    // to make sure your initialization logic is in the right place.
    void Actor::PostInit(IGameObject* pGameObject)
    {
        if (pGameObject == nullptr)
        {
            return;
        }
        pGameObject->EnableUpdateSlot(this, 0);
        pGameObject->EnablePostUpdates(this);
    }

    bool Actor::ReloadExtension(IGameObject* gameObject, const SEntitySpawnParams& spawnParams)
    {
        ResetGameObject();
        if (!GetGameObject()->CaptureView(this))
        {
            return false;
        }
        if (!GetGameObject()->BindToNetwork())
        {
            return false;
        }
        gEnv->pGame->GetIGameFramework()->GetIActorSystem()->RemoveActor(spawnParams.prevId);
        gEnv->pGame->GetIGameFramework()->GetIActorSystem()->AddActor(GetEntityId(), this);

        return true;
    }

    const void* Actor::GetRMIBase() const
    {
        return CGameObjectExtensionHelper::GetRMIBase();
    }
    const char* Actor::GetEntityClassName() const
    {
        return GetEntity()->GetClass()->GetName();
    }

    // #TODO the UpdateView function is the recommended place to compute
    // the camera's position and orientation.
    void Actor::UpdateView(SViewParams& params)
    {
        params.position = GetEntity()->GetWorldPos();
        params.rotation = GetEntity()->GetWorldRotation();
    }

    void Actor::PostUpdateView(SViewParams& params)
    {
    }
}

