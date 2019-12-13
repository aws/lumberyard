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

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>

#include "NamedRunningStatistic.h"

namespace AzFramework
{
    namespace Statistics
    {
        /**
         * @brief Order-preserving collection of a set of statistics.
         *
         * The order in which a new object is stored when calling AddStatistic()
         * will be preserved.
         */
        class RunningStatisticsManager
        {
        public:
            RunningStatisticsManager() = default;
            virtual ~RunningStatisticsManager() = default;

            bool ContainsStatistic(const AZStd::string& name);

            /**
             * Returns false if a NamedRunningStatistic with such name already exists.
             */
            bool AddStatistic(const AZStd::string& name, const AZStd::string& units);

            /**
             * Removing a RunningStatistic should be a rare operation.
             * Once an item is removed we need to go over the existing
             * ones in the unordered_map and update the indices.
             */
            virtual void RemoveStatistic(const AZStd::string& name);

            void ResetStatistic(const AZStd::string& name);

            void ResetAllStatistics();

            void PushSampleForStatistic(const AZStd::string& name, double value);

            /**
             * Returns nullptr if a statistic with such name doesn't exist,
             * otherwise returns a pointer to the statistic.
             */
            NamedRunningStatistic* GetStatistic(const AZStd::string& name)
            {
                return GetStatistic(name, nullptr);
            }

            const AZStd::vector<NamedRunningStatistic>& GetAllStatistics() const;

        protected:
           /**
             * Same as the public method with the extra convenience of the @param index
             * that can be used by subclasses to avoid extra searching steps.
             */
            NamedRunningStatistic* GetStatistic(const AZStd::string& name, AZ::u32* index);

            /**
             * This one is called when it is guaranteed that a statistic with such name is
             * not in the internal collection. For example: The code had called ContainsStatistic()
             * or GetStatistic() before calling this method.
             */
            void AddStatisticValidated(const AZStd::string& name, const AZStd::string& units);

        private:
            ///List of the RunningStat objects.
            AZStd::vector<NamedRunningStatistic> m_statistics;

            ///Key: Stat name, Value: Index in @param m_statistics
            AZStd::unordered_map<AZStd::string, AZ::u32> m_statisticsNamesToIndexMap;
        };//class RunningStatisticsManager
    }//namespace Statistics
}//namespace AzFramework
