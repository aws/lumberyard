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

#if !defined(BUILD_GAMELIFT_SERVER) && defined(BUILD_GAMELIFT_CLIENT)

#include <GameLift/Session/GameLiftClientService.h>
#include <GameLift/Session/GameLiftClientSession.h>
#include <GameLift/Session/GameLiftSearch.h>
#include <GameLift/Session/GameLiftSessionRequest.h>

#pragma warning(push)
#pragma warning(disable:4251)
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/core/utils/Outcome.h>
#include <aws/gamelift/model/UpdateFleetAttributesRequest.h>
#include <aws/gamelift/model/UpdateAliasRequest.h>
#include <aws/gamelift/GameLiftClient.h>
#pragma warning(pop)

namespace
{
    //This is used when a playerId is not specified when initializing GameLiftSDK with developer credentials.
    const char* DEFAULT_PLAYER_ID = "AnonymousPlayerId";
}

namespace GridMate
{
    GameLiftClientService::GameLiftClientService(const GameLiftClientServiceDesc& desc)
        : SessionService(desc)
        , m_serviceDesc(desc)
        , m_clientStatus(GameLift_NotInited)
        , m_useFleetId(false)
        , m_client(nullptr)
    {
        if (!m_serviceDesc.m_playerId.empty())
        {
            m_playerId.assign(m_serviceDesc.m_playerId.c_str());
        }
        else
        {
            m_playerId.assign(DEFAULT_PLAYER_ID);
        }

        if (m_serviceDesc.m_aliasId.empty())
        {
            m_useFleetId = true;
            m_fleetId.assign(m_serviceDesc.m_fleetId.c_str());
        }
        else
        {
            m_aliasId.assign(m_serviceDesc.m_aliasId.c_str());
        }

    }

    GameLiftClientService::~GameLiftClientService()
    {

    }

    bool GameLiftClientService::IsReady() const
    {
        return m_clientStatus == GameLift_Ready;
    }

    bool GameLiftClientService::UseFleetId() const
    {
        return m_useFleetId;
    }

    Aws::String GameLiftClientService::GetPlayerId() const
    {
        return m_playerId;
    }

    Aws::String GameLiftClientService::GetAliasId() const
    {
        return m_aliasId;
    }

    Aws::String GameLiftClientService::GetFleetId() const
    {
        return m_fleetId;
    }

    Aws::GameLift::GameLiftClient* GameLiftClientService::GetClient() const
    {
        return m_client;
    }

    void GameLiftClientService::OnServiceRegistered(IGridMate* gridMate)
    {
        SessionService::OnServiceRegistered(gridMate);

        GameLiftClientSession::RegisterReplicaChunks();

        if (!StartGameLiftClient())
        {
            EBUS_EVENT_ID(m_gridMate, GameLiftClientServiceEventsBus, OnGameLiftSessionServiceFailed, this, "GameLift client failed to start");
        }

        GameLiftClientServiceBus::Handler::BusConnect(gridMate);
    }

    void GameLiftClientService::OnServiceUnregistered(IGridMate* gridMate)
    {
        GameLiftClientServiceBus::Handler::BusDisconnect();

        if (m_clientStatus == GameLift_Ready)
        {
            delete m_client;
            m_client = nullptr;
            m_clientStatus = GameLift_NotInited;
        }

        SessionService::OnServiceUnregistered(gridMate);
    }

    template <typename Callable>
    void GameLiftClientService::UpdateImpl(Callable& callable)
    {
        if (callable.valid() && callable.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
        {
            if (callable.valid())
            {
                auto outcome = callable.get();
                if (outcome.IsSuccess())
            {
                AZ_TracePrintf("GameLift", "Initialized GameLift client successfully.\n");
                m_clientStatus = GameLift_Ready;

                EBUS_EVENT_ID(m_gridMate, GameLiftClientServiceEventsBus, OnGameLiftSessionServiceReady, this);
                EBUS_DBG_EVENT(Debug::SessionDrillerBus, OnSessionServiceReady);
                EBUS_EVENT_ID(m_gridMate, SessionEventBus, OnSessionServiceReady);
            }
            else
            {
                    auto errorMessage = outcome.GetError().GetMessage();
                AZ_TracePrintf("GameLift", "Failed to initialize GameLift client: %s\n", errorMessage.c_str());

                m_clientStatus = GameLift_Failed;
                    EBUS_EVENT_ID(m_gridMate, GameLiftClientServiceEventsBus, OnGameLiftSessionServiceFailed, this, errorMessage.c_str());
                }
            }
            }
        }

    void GameLiftClientService::Update()
    {
        if (m_useFleetId)
        {
            UpdateImpl(m_updateFleetAttributesOutcomeCallable);
        }
        else
        {
            UpdateImpl(m_updateAliasOutcomeCallable);
        }

        SessionService::Update();
    }

    GridSession* GameLiftClientService::JoinSessionBySearchInfo(const GameLiftSearchInfo& searchInfo, const CarrierDesc& carrierDesc)
    {
        if (m_clientStatus != GameLift_Ready)
        {
            AZ_TracePrintf("GameLift", "Client API is not initialized.\n");
            return nullptr;
        }

        GameLiftClientSession* session = aznew GameLiftClientSession(this);
        if (!session->Initialize(searchInfo, JoinParams(), carrierDesc))
        {
            delete session;
            return nullptr;
        }

        return session;
    }

    GameLiftSessionRequest* GameLiftClientService::RequestSession(const GameLiftSessionRequestParams& params)
    {
        if (m_clientStatus != GameLift_Ready)
        {
            AZ_TracePrintf("GameLift", "Client API is not initialized.\n");
            return nullptr;
        }

        GameLiftSessionRequest* request = aznew GameLiftSessionRequest(this, params);
        if (!request->Initialize())
        {
            delete request;
            return nullptr;
        }

        return request;
    }

    GameLiftSearch* GameLiftClientService::StartSearch(const GameLiftSearchParams& params)
    {
        if (m_clientStatus != GameLift_Ready)
        {
            AZ_TracePrintf("GameLift", "Client API is not initialized.\n");
            return nullptr;
        }

        GameLiftSearch* search = aznew GameLiftSearch(this, params);
        if (!search->Initialize())
        {
            delete search;
            return nullptr;
        }

        return search;
    }

    GameLiftClientSession* GameLiftClientService::QueryGameLiftSession(const GridSession* session)
    {
        for (GridSession* s : m_sessions)
        {
            if (s == session)
            {
                return static_cast<GameLiftClientSession*>(s);
            }
        }

        return nullptr;
    }

    GameLiftSearch* GameLiftClientService::QueryGameLiftSearch(const GridSearch* search)
    {
        for (GridSearch* s : m_activeSearches)
        {
            if (s == search)
            {
                return static_cast<GameLiftSearch*>(s);
            }
        }

        for (GridSearch* s : m_completedSearches)
        {
            if (s == search)
            {
                return static_cast<GameLiftSearch*>(s);
            }
        }

        return nullptr;
    }

    bool GameLiftClientService::StartGameLiftClient()
    {
        if (m_clientStatus == GameLift_NotInited)
        {
            if (!ValidateServiceDesc())
            {
                m_clientStatus = GameLift_Failed;
                AZ_TracePrintf("GameLift", "GameLift client failed to start. Invalid set of gamelift_* parameters.\n");
                return false;
            }

            m_clientStatus = GameLift_Initing;

            Aws::Client::ClientConfiguration config;
            config.region = m_serviceDesc.m_region.c_str();
            config.endpointOverride = m_serviceDesc.m_endpoint.c_str();
            config.verifySSL = true;

            Aws::String accessKey(m_serviceDesc.m_accessKey.c_str());
            Aws::String secretKey(m_serviceDesc.m_secretKey.c_str());
            Aws::Auth::AWSCredentials cred = Aws::Auth::AWSCredentials(accessKey, secretKey);

            m_client = new Aws::GameLift::GameLiftClient(cred, config);

            if (m_useFleetId)
            {
                Aws::GameLift::Model::UpdateFleetAttributesRequest request;
                request.SetFleetId(m_fleetId);
                m_updateFleetAttributesOutcomeCallable = m_client->UpdateFleetAttributesCallable(request);
            }
            else
            {
                Aws::GameLift::Model::UpdateAliasRequest request;
                request.SetAliasId(m_aliasId);
                m_updateAliasOutcomeCallable = m_client->UpdateAliasCallable(request);
            }
        }

        return m_clientStatus != GameLift_Failed;
    }

    bool GameLiftClientService::ValidateServiceDesc()
    {
        //Validation on inputs.
        //Service still supports the use of developers credentials on top of the player credentials.
        if (m_serviceDesc.m_fleetId.empty() && m_serviceDesc.m_aliasId.empty())
        {
            AZ_TracePrintf("GameLift", "You need to provide at least [gamelift_aliasid, gamelift_aws_access_key, gamelift_aws_secret_key] or [gamelift_fleetid, gamelift_aws_access_key, gamelift_aws_secret_key]\n");
            return false;
        }

        //If the user is a developer.
        if (!m_serviceDesc.m_fleetId.empty() && (m_serviceDesc.m_accessKey.empty() || m_serviceDesc.m_secretKey.empty()))
        {
            AZ_TracePrintf("GameLift", "Initialize failed. Trying to set the fleetId without providing gamelift_aws_access_key and gamelift_aws_secret_key.\n");
            return false;
        }

        //If the user is a player.
        if (!m_serviceDesc.m_aliasId.empty() && (m_serviceDesc.m_accessKey.empty() || m_serviceDesc.m_secretKey.empty()))
        {
            AZ_TracePrintf("GameLift", "Initialize failed. Trying to set the aliasId without providing gamelift_aws_access_key and gamelift_aws_secret_key.\n");
            return false;
        }

        return true;
    }
} // namespace GridMate

#endif // BUILD_GAMELIFT_CLIENT
