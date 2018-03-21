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

#if !defined(BUILD_GAMELIFT_SERVER) && defined(BUILD_GAMELIFT_CLIENT)

#include <aws/core/Aws.h>
#include <GameLift/Session/GameLiftClientServiceBus.h>
#include <GameLift/Session/GameLiftClientServiceEventsBus.h>
#include <GameLift/Session/GameLiftSessionDefs.h>

#include <GridMate/Session/Session.h>
#include <AzCore/std/smart_ptr/scoped_ptr.h>

#ifdef GetMessage
#undef GetMessage
#endif

#pragma warning(push)
#pragma warning(disable: 4355) // <future> includes ppltasks.h which throws a C4355 warning: 'this' used in base member initializer list
#include <aws/gamelift/GameLiftClient.h>
#pragma warning(pop)

namespace GridMate
{
    class GameLiftSession;
    struct GameLiftSessionRequestParams;
    class GameLiftSearch;
    class GameLiftSessionRequest;

    /*!
    * GameLift session service settings.
    */
    struct GameLiftClientServiceDesc
        : public SessionServiceDesc
    {
        string m_region; //< AWS Region
        string m_endpoint; //< GameLift endpoint host

        string m_accessKey; //< AWS Access key
        string m_secretKey; //< AWS Secret key

        string m_playerId; //< GameLift playerId

        // Needs to be filled in when using GameLift as a player with fleetId.
        string m_fleetId; //< GameLift fleetId

        // Needs to be filled in when using GameLift as a player with aliasId.
        string m_aliasId; //< GameLift aliasId
    };

    /*!
    * GameLift session service.
    */
    class GameLiftClientService
        : public SessionService
        , public GameLiftClientServiceBus::Handler
    {
    public:

        GM_CLASS_ALLOCATOR(GameLiftClientService);
        GRIDMATE_SERVICE_ID(GameLiftClientService);

        GameLiftClientService(const GameLiftClientServiceDesc& desc);
        ~GameLiftClientService() override;

        bool IsReady() const;
        bool UseFleetId() const;
        Aws::String GetAliasId() const;
        Aws::String GetFleetId() const;
        Aws::GameLift::GameLiftClient* GetClient() const;
        Aws::String GetPlayerId() const;

        // SessionService
        void OnServiceRegistered(IGridMate* gridMate) override;
        void OnServiceUnregistered(IGridMate* gridMate) override;
        void Update() override;

        // GameLiftClientServiceBus implementation
        GridSession* JoinSessionBySearchInfo(const GameLiftSearchInfo& params, const CarrierDesc& carrierDesc) override;
        GameLiftSessionRequest* RequestSession(const GameLiftSessionRequestParams& params) override;
        GameLiftSearch* StartSearch(const GameLiftSearchParams& params) override;
        GameLiftClientSession* QueryGameLiftSession(const GridSession* session) override;
        GameLiftSearch* QueryGameLiftSearch(const GridSearch* search) override;        

    protected:
        bool StartGameLiftClient();
        bool ValidateServiceDesc();

        enum GameLiftStatus
        {
            GameLift_NotInited, ///< GameLift SDK is not initialized
            GameLift_Initing, ///< Pending GameLift SDK initialization
            GameLift_Ready, ///< GameLift SDK is ready to use
            GameLift_Failed, ///< GameLift SDK failed to initialize
            GameLift_Terminated ///< Current instance was force terminated by the user (only applies to the server)
        };

        GameLiftClientServiceDesc m_serviceDesc;
        GameLiftStatus m_clientStatus;
        bool m_useFleetId;
        Aws::String m_playerId;
        Aws::String m_aliasId;
        Aws::String m_fleetId;
        Aws::GameLift::GameLiftClient* m_client;
        Aws::GameLift::Model::UpdateFleetAttributesOutcomeCallable m_updateFleetAttributesOutcomeCallable;
        Aws::GameLift::Model::UpdateAliasOutcomeCallable m_updateAliasOutcomeCallable;

        Aws::SDKOptions m_optionsSdk;

        template<typename Callable>
        void UpdateImpl(Callable& callable);
    };
} // namespace GridMate

#endif // BUILD_GAMELIFT_CLIENT
