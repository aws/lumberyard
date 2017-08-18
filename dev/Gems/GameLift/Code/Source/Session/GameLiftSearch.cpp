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

#include <GameLift/Session/GameLiftSearch.h>
#include <GameLift/Session/GameLiftClientService.h>

// guarding against AWS SDK & windows.h name conflict
#ifdef GetMessage
#undef GetMessage
#endif

#pragma warning(push)
#pragma warning(disable: 4251)
#include <aws/core/utils/Outcome.h>
#include <aws/gamelift/model/SearchGameSessionsRequest.h>
#pragma warning(pop)

namespace GridMate
{
    GameLiftSearch::GameLiftSearch(GameLiftClientService* service, const GameLiftSearchParams& searchParams)
        : GridSearch(service)
        , m_searchParams(searchParams)
        , m_clientService(service)
    {
        m_isDone = true;
    }

    bool GameLiftSearch::Initialize()
    {
        Aws::GameLift::Model::SearchGameSessionsRequest request;
        m_clientService->UseFleetId() ? request.SetFleetId(m_clientService->GetFleetId()) : request.SetAliasId(m_clientService->GetAliasId());
        if (!m_searchParams.m_gameInstanceId.empty())
        {
            Aws::String filter("gameSessionId = ");
            filter += m_searchParams.m_gameInstanceId.c_str();
            request.SetFilterExpression(filter);
        }

        m_searchGameSessionsOutcomeCallable = m_clientService->GetClient()->SearchGameSessionsCallable(request);

        m_isDone = false;
        return true;
    }

    unsigned int GameLiftSearch::GetNumResults() const
    {
        return static_cast<unsigned int>(m_results.size());
    }

    const SearchInfo* GameLiftSearch::GetResult(unsigned int index) const
    {
        return &m_results[index];
    }

    void GameLiftSearch::AbortSearch()
    {
        SearchDone();
    }

    void GameLiftSearch::SearchDone()
    {
        m_isDone = true;
    }

    void GameLiftSearch::Update()
    {
        if (m_isDone)
        {
            return;
        }

        if (m_searchGameSessionsOutcomeCallable.valid()
            && m_searchGameSessionsOutcomeCallable.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
        {
            auto result = m_searchGameSessionsOutcomeCallable.get();
            if (result.IsSuccess())
            {
                auto gameSessions = result.GetResult().GetGameSessions();
                for (auto& gameSession : gameSessions)
                {
                    ProcessGameSessionResult(gameSession);
                }
            }
            else
            {
                AZ_TracePrintf("GameLift", "Session search failed with error: %s\n", result.GetError().GetMessage().c_str());
            }

            SearchDone();
        }
    }

    void GameLiftSearch::ProcessGameSessionResult(const Aws::GameLift::Model::GameSession& gameSession)
    {
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
    }
} // namespace GridMate

#endif // BUILD_GAMELIFT_CLIENT
