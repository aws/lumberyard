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

// The AWS Native SDK AWSAllocator triggers a warning due to accessing members of std::allocator directly.
// AWSAllocator.h(70): warning C4996: 'std::allocator<T>::pointer': warning STL4010: Various members of std::allocator are deprecated in C++17.
// Use std::allocator_traits instead of accessing these members directly.
// You can define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING or _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS to acknowledge that you have received this warning.
AZ_PUSH_DISABLE_WARNING(4251 4996, "-Wunknown-warning-option")
#include <aws/gamelift/GameLiftClient.h>
AZ_POP_DISABLE_WARNING

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
