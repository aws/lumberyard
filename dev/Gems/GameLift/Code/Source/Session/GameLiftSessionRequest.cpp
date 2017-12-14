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

#include <GameLift/Session/GameLiftSessionRequest.h>
#include <GameLift/Session/GameLiftClientService.h>

// guarding against AWS SDK & windows.h name conflict
#ifdef GetMessage
#undef GetMessage
#endif

#pragma warning(push)
#pragma warning(disable: 4251)
#include <aws/core/utils/Outcome.h>
#include <aws/gamelift/model/CreateGameSessionRequest.h>
#pragma warning(pop)

namespace GridMate
{
    GameLiftSessionRequest::GameLiftSessionRequest(GameLiftClientService* service, const GameLiftSessionRequestParams& params)
        : GameLiftSearch(service, GameLiftSearchParams())
        , m_requestParams(params)
    {
        m_isDone = true;
    }

    void GameLiftSessionRequest::AbortSearch()
    {
        SearchDone();
    }

    bool GameLiftSessionRequest::Initialize()
    {
        if (!m_isDone)
        {
            return false;
        }

        Aws::Vector<Aws::GameLift::Model::GameProperty> gameProperties;
        for (AZStd::size_t i = 0; i < m_requestParams.m_numParams; ++i)
        {
            Aws::GameLift::Model::GameProperty prop;
            prop.SetKey(m_requestParams.m_params[i].m_id.c_str());
            prop.SetValue(m_requestParams.m_params[i].m_value.c_str());
            gameProperties.push_back(prop);
        }

        Aws::GameLift::Model::CreateGameSessionRequest request;
        m_clientService->UseFleetId() ? request.SetFleetId(m_clientService->GetFleetId().c_str()) : request.SetAliasId(m_clientService->GetAliasId().c_str());
        request.WithMaximumPlayerSessionCount(m_requestParams.m_numPublicSlots + m_requestParams.m_numPrivateSlots)
            .WithName(m_requestParams.m_instanceName.c_str())
            .WithGameProperties(gameProperties);

        m_createGameSessionOutcomeCallable = m_clientService->GetClient()->CreateGameSessionCallable(request);

        m_isDone = false;
        return true;
    }

    void GameLiftSessionRequest::SearchDone()
    {
        m_isDone = true;
    }

    void GameLiftSessionRequest::Update()
    {
        if (m_isDone || !m_createGameSessionOutcomeCallable.valid())
        {
            return;
        }

        if (m_createGameSessionOutcomeCallable.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
        {
            Aws::GameLift::Model::CreateGameSessionOutcome result = m_createGameSessionOutcomeCallable.get();

            if (!result.IsSuccess())
            {
                AZ_TracePrintf("GameLift", "Session creation failed with error: %s\n", result.GetError().GetMessage().c_str());

                SearchDone();
                return;
            }

            auto gameSession = result.GetResult().GetGameSession();

            GameLiftSearchInfo info;
            info.m_fleetId = gameSession.GetFleetId().c_str();
            info.m_gameInstanceId = gameSession.GetGameSessionId().c_str();
            info.m_sessionId = info.m_gameInstanceId.c_str();
            info.m_numFreePublicSlots = gameSession.GetMaximumPlayerSessionCount() - gameSession.GetCurrentPlayerSessionCount();
            info.m_numUsedPublicSlots = gameSession.GetCurrentPlayerSessionCount();
            info.m_numPlayers = gameSession.GetCurrentPlayerSessionCount();

            auto& properties = gameSession.GetGameProperties();
            for (auto& prop : properties)
            {
                info.m_params[info.m_numParams].m_id = prop.GetKey().c_str();
                info.m_params[info.m_numParams].m_value = prop.GetValue().c_str();

                ++info.m_numParams;
            }

            m_results.push_back(info);

            SearchDone();
        }
    }
} // namespace GridMate

#endif // BUILD_GAMELIFT_CLIENT
