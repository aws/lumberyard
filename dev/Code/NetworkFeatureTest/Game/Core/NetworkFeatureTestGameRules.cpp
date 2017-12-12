
#include "StdAfx.h"
#include "NetworkFeatureTestGameRules.h"
#include "IActorSystem.h"

namespace LYGame
{
    DECLARE_DEFAULT_COMPONENT_FACTORY(NetworkFeatureTestGameRules, IGameRules)

    bool NetworkFeatureTestGameRules::Init(IGameObject* pGameObject)
    {
        SetGameObject(pGameObject);
        if (!pGameObject->BindToNetwork())
        {
            return false;
        }

        if (gEnv == nullptr ||
            gEnv->pGame == nullptr ||
            gEnv->pGame->GetIGameFramework() == nullptr ||
            gEnv->pGame->GetIGameFramework()->GetIGameRulesSystem() == nullptr)
        {
            return false;
        }
        gEnv->pGame->GetIGameFramework()->GetIGameRulesSystem()->SetCurrentGameRules(this);

        return true;
    }

    void NetworkFeatureTestGameRules::PostInit(IGameObject* pGameObject)
    {
        if (pGameObject != nullptr)
        {
            pGameObject->EnableUpdateSlot(this, 0);
            pGameObject->EnablePostUpdates(this);
        }
    }

    NetworkFeatureTestGameRules::~NetworkFeatureTestGameRules()
    {
        if (gEnv->pGame->GetIGameFramework()->GetIGameRulesSystem())
        {
            gEnv->pGame->GetIGameFramework()->GetIGameRulesSystem()->SetCurrentGameRules(nullptr);
        }
    }

    bool NetworkFeatureTestGameRules::OnClientConnect(ChannelId channelId, bool isReset)
    {
        // #TODO This is where the actor for a player is created on starting / connecting to the game.
        auto pPlayerActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->CreateActor(channelId, "Character", "Actor", Vec3(0, 0, 0), Quat(Ang3(0, 0, 0)), Vec3(1, 1, 1));

        return true;
    }
}
