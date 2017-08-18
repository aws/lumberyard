
#pragma once

#include <IGameRulesSystem.h>

namespace LYGame
{
    class CloudGemSamplesGameRules
        : public CGameObjectExtensionHelper < CloudGemSamplesGameRules, IGameRules >
    {
    public:
        CloudGemSamplesGameRules() {}
        virtual ~CloudGemSamplesGameRules();

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