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

#include <GridMate/Session/Session.h>
#include <GameLift/Session/GameLiftSessionDefs.h>

#ifdef GetMessage
#undef GetMessage
#endif

#include <aws/gamelift/GameLiftClient.h>

namespace GridMate
{
    class GameLiftClientService;

    /*!
    * GameLift search object, created by session service when search is performed.
    * So far returns list of available instances for a given fleet.
    */
    class GameLiftSearch
        : public GridSearch
    {
        friend class GameLiftClientService;

    public:

        GM_CLASS_ALLOCATOR(GameLiftSearch);

        // GridSearch interface
        unsigned int        GetNumResults() const override;
        const SearchInfo*   GetResult(unsigned int index) const override;
        void                AbortSearch();

    protected:
        GameLiftSearch(GameLiftClientService* service, const GameLiftSearchParams& searchParams);
        bool Initialize();
        void ProcessGameSessionResult(const Aws::GameLift::Model::GameSession& gameSession);

        void Update() override;
        void SearchDone();

        Aws::GameLift::Model::SearchGameSessionsOutcomeCallable m_searchGameSessionsOutcomeCallable;

        vector<GameLiftSearchInfo> m_results;
        GameLiftSearchParams m_searchParams;
        GameLiftClientService* m_clientService;
    };
} // namespace GridMate

#endif // BUILD_GAMELIFT_CLIENT
