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
#include <IFlowSystem.h>
#include <FlowSystem/Nodes/FlowBaseNode.h>
#include <GridMate/GridMate.h>
#include <GridMate/Session/LANSession.h>

namespace FlowMultiplayerNodes
{
    /*!
     * Interface for simulated search results, used primarily for the editor and testing
     */
    class ISimulatedSearch
    {
    public:
        virtual ~ISimulatedSearch() {}

        virtual void AbortSearch() = 0;
        virtual unsigned int GetNumResults() const = 0;
        virtual const GridMate::SearchInfo* GetResult(unsigned int index) const = 0;
    };

    /*!
     * FlowCustomData implementation for passing real and simulated search results between
     * FlowGraph nodes.
     */
    class SearchInfoCustomData
        : public FlowCustomData::Base
    {
    public:
        SearchInfoCustomData(bool simulated = false)
            : FlowCustomData::Base()
            , m_resultIndex(-1)
            , m_simulatedSearch(nullptr)
            , m_search(nullptr)
            , m_simulated(simulated)
        {
        }

        SearchInfoCustomData(GridMate::GridSearch* search, int resultIndex, bool simulated = false)
            : FlowCustomData::Base()
            , m_search(search)
            , m_simulatedSearch(nullptr)
            , m_resultIndex(resultIndex)
            , m_simulated(simulated)
        {
        }

        SearchInfoCustomData(ISimulatedSearch* search, int resultIndex, bool simulated = false)
            : FlowCustomData::Base()
            , m_search(nullptr)
            , m_simulatedSearch(search)
            , m_resultIndex(resultIndex)
            , m_simulated(simulated)
        {
        }

        //! True if search results exist and the result index is valid
        bool IsValid()
        {
            if (m_simulated)
            {
                return m_simulatedSearch &&
                       m_resultIndex > -1 &&
                       m_resultIndex < m_simulatedSearch->GetNumResults();
            }
            else
            {
                return m_search &&
                       m_resultIndex > -1 &&
                       m_resultIndex < m_search->GetNumResults();
            }
        }

        //! Get the simulated or real SearchInfo result
        const GridMate::SearchInfo* GetSearchInfo()
        {
            if (!IsValid())
            {
                return nullptr;
            }

            return m_simulated ? m_simulatedSearch->GetResult(m_resultIndex) : m_search->GetResult(m_resultIndex);
        }

        void SetIsSimulated(bool simulated) { m_simulated = simulated; }

    private:
        GridMate::GridSearch* m_search;
        int m_resultIndex;
        bool m_simulated;
        ISimulatedSearch* m_simulatedSearch;
    };
}