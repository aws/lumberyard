
#pragma once

#include <IGameRulesSystem.h>

namespace LYGame
{
    class NetworkFeatureTestGameRules
        : public CGameObjectExtensionHelper < NetworkFeatureTestGameRules, IGameRules >
    {
    public:
        NetworkFeatureTestGameRules() {}
        virtual ~NetworkFeatureTestGameRules();

        //////////////////////////////////////////////////////////////////////////
        //! IGameObjectExtension
        bool Init(IGameObject* pGameObject) override;
        void PostInit(IGameObject* pGameObject) override;
        void ProcessEvent(SEntityEvent& event) override { }
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // IGameRules
        bool OnClientConnect(ChannelId channelId, bool isReset) override;
        //////////////////////////////////////////////////////////////////////////
    };
} // namespace LYGame