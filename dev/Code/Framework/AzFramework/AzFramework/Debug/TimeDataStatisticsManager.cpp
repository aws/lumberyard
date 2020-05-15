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

#include "TimeDataStatisticsManager.h"

namespace AzFramework
{
    namespace Statistics
    {
        void TimeDataStatisticsManager::RemoveStatistic(const AZStd::string& name)
        {
            //Let's get the current index of the statistic we are
            //supposed to remove.
            AZ::u32 statIndex;
            NamedRunningStatistic* statistic = GetStatistic(name, &statIndex);
            if (!statistic)
            {
                return;
            }
            m_previousTimeData.erase(m_previousTimeData.begin() + statIndex);
            //Now we can safely remove the statistic from RunningStatisticsManager private collections,
            RunningStatisticsManager::RemoveStatistic(name);
            //The private m_statistics and m_previousTimeData are now in sync.
        }

        void TimeDataStatisticsManager::PushTimeDataSample(const char * registerName, const AZ::Debug::ProfilerRegister::TimeData& timeData)
        {
            AZStd::string statName(registerName);
            AZ::u32 statIndex;
            NamedRunningStatistic* statistic = GetStatistic(statName, &statIndex);
            if (!statistic)
            {
                const char * units = "us";
                AddStatisticValidated(statName, units);
                AZ::Debug::ProfilerRegister::TimeData zeroTimeData;
                memset(&zeroTimeData, 0, sizeof(AZ::Debug::ProfilerRegister::TimeData));
                m_previousTimeData.emplace_back(zeroTimeData);
                statIndex = static_cast<AZ::u32>(m_previousTimeData.size() - 1);
                AZ::u32 tmpStatIndex;
                statistic = GetStatistic(statName, &tmpStatIndex);
                AZ_Assert((statistic != nullptr) && (tmpStatIndex == statIndex), "Fatal error adding a new statistic object");
            }

            const AZ::u64 accumulatedTime = timeData.m_time;
            const AZ::s64 totalNumCalls = timeData.m_calls;
            const AZ::u64 previousAccumulatedTime = m_previousTimeData[statIndex].m_time;
            const AZ::s64 previousTotalNumCalls = m_previousTimeData[statIndex].m_calls;
            const AZ::u64 deltaTime = accumulatedTime - previousAccumulatedTime;
            const AZ::s64 deltaCalls = totalNumCalls - previousTotalNumCalls;
            
            if (deltaCalls == 0)
            {
                //This is the same old data. Let's skip it
                return;
            }

            double newSample = static_cast<double>(deltaTime) / deltaCalls;

            statistic->PushSample(newSample);
            m_previousTimeData[statIndex] = timeData;
        }
    } //namespace Statistics
} //namespace AzFramework
