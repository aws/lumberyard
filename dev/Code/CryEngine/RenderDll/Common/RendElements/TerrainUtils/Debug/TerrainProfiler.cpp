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

#include "StdAfx.h"

#ifdef LY_TERRAIN_RUNTIME

#if !defined(_RELEASE)

#include "TerrainProfiler.h"
#include <CrySizer.h>
#include <IResourceCollector.h>

namespace Terrain
{
    namespace Debug
    {
        /**
          * Very simple implementation of ICrySizer.
          * CrySizerImpl is not exported from CrySystem. Additionally CrySizerImpl is much more complicated
          * for what we need.
          */
        class SimpleSizer
            : public ICrySizer,
            public NullResCollector
        {
        public:
            SimpleSizer() : m_nTotalSize(0) {}
            virtual ~SimpleSizer() = default;

            void Release() override { AZ_Assert(false, "Not Supported"); }
            size_t GetTotalSize() override { return m_nTotalSize; };
            size_t GetObjectCount() override { return 1; }

            void Reset() override { m_nTotalSize = 0; }

            void End() override { AZ_Assert(false, "Not Supported"); }

            bool AddObject(const void* /*pIdentifier*/, size_t nSizeBytes, int nCount = 1) override
            {
                m_nTotalSize += nSizeBytes * nCount;
                return nSizeBytes > 0;
            }

            IResourceCollector* GetResourceCollector() override { return this; }
            void SetResourceCollector(IResourceCollector* /*pColl*/) override { AZ_Assert(false, "Not Supported"); }

        protected:

            void Push(const char* szComponentName) override { };
            void PushSubcomponent(const char* szSubcomponentName) override { };
            void Pop() override { };

        private:
            size_t m_nTotalSize;
        };

        TerrainProfiler::TerrainProfiler(IConsole* console) : m_console(console), m_elapsedSecondsForLogging(0.0f)
        {
            auto profilerProxy = AZ::Interface<AzFramework::Debug::StatisticalProfilerProxy>::Get();
            if (!profilerProxy)
            {
                AZ_Warning("TerrainProfiler", false, "The Terrain Profiler couldn't find the StatisticalProfilerProxy");
                return;
            }
            AzFramework::Debug::StatisticalProfilerProxy::StatisticalProfilerType& profiler = profilerProxy->GetProfiler(AZ::Debug::ProfileCategory::Terrain);
            AzFramework::Statistics::StatisticsManager<>& statisticsManager = profiler.GetStatsManager();

            AZStd::string memoryUsageStatName("MemoryUsage");
            if (statisticsManager.GetCount() < 1)
            {
                statisticsManager.AddStatistic(StatisticVisibleDataTiles, StatisticVisibleDataTiles, "", true);
                statisticsManager.AddStatistic(StatisticVisibleTextureTiles, StatisticVisibleTextureTiles, "", true);
                statisticsManager.AddStatistic(memoryUsageStatName, memoryUsageStatName, "KB", true);

                statisticsManager.AddStatistic(StatisticMfPrepare, StatisticMfPrepare, "us", true);
                statisticsManager.AddStatistic(StatisticMfDrawNodeGather, StatisticMfDrawNodeGather, "us", true);
                statisticsManager.AddStatistic(StatisticMfDrawGatherTextureTiles, StatisticMfDrawGatherTextureTiles, "us", true);
                statisticsManager.AddStatistic(StatisticMfDrawTerrainDraw, StatisticMfDrawTerrainDraw, "us", true);

                AZStd::string timePerFrameStatName("TimePerRenderFrame");
                AZStd::vector<AZStd::string> statsPerFrame;
                statsPerFrame.push_back(StatisticMfPrepare);
                statsPerFrame.push_back(StatisticMfDrawNodeGather);
                statsPerFrame.push_back(StatisticMfDrawTerrainDraw);
                const int num_stats_per_frame = profiler.AddPerFrameStatisticalAggregate(statsPerFrame, timePerFrameStatName, timePerFrameStatName);
                AZ_Assert(num_stats_per_frame == statsPerFrame.size(), "Failed to mark %d stats for per frame calculations.", statsPerFrame.size() - num_stats_per_frame);
            }
            statisticsManager.ResetAllStatistics();

            m_visibleDataTilesCountStat = statisticsManager.GetStatistic(StatisticVisibleDataTiles);
            m_visibleTextureTilesCountStat = statisticsManager.GetStatistic(StatisticVisibleTextureTiles);
            m_memoryUsageStat = statisticsManager.GetStatistic(memoryUsageStatName);

            AZ_Assert(m_visibleDataTilesCountStat, "Failed to find visible data tiles count statistic");
            AZ_Assert(m_visibleTextureTilesCountStat, "Failed to find visible texture tiles count statistic");
            AZ_Assert(m_memoryUsageStat, "Failed to find MemoryUsage statistic");

            profilerProxy->ActivateProfiler(AZ::Debug::ProfileCategory::Terrain, true);

            m_timerForLogging.Stamp();

            AZ::TickBus::Handler::BusConnect();
        };

        TerrainProfiler::~TerrainProfiler() {
            AZ::TickBus::Handler::BusDisconnect();
            auto proxy = AZ::Interface<AzFramework::Debug::StatisticalProfilerProxy>::Get();
            if (proxy)
            {
                proxy->ActivateProfiler(AZ::Debug::ProfileCategory::Terrain, false);
            }
        };

        //////////////////////////////////////////////////////////////////////////
        // Tick bus START
        void TerrainProfiler::OnTick(float deltaTime, AZ::ScriptTimePoint time)
        {
            auto profilerProxy = AZ::Interface<AzFramework::Debug::StatisticalProfilerProxy>::Get();
            if (!profilerProxy)
            {
                AZ_Warning("TerrainProfiler", false, "The Terrain Profiler couldn't find the StatisticalProfilerProxy");
                return;
            }
            AzFramework::Debug::StatisticalProfilerProxy::StatisticalProfilerType& profiler = profilerProxy->GetProfiler(AZ::Debug::ProfileCategory::Terrain);

            profiler.SummarizePerFrameStats();

            ICVar* pvar = m_console->GetCVar("e_TerrainPerformanceSecondsPerLog");
            if (!pvar)
            {
                return;
            }

            const float terrainPerformanceSecondsPerLog = pvar->GetFVal();
            if (terrainPerformanceSecondsPerLog <= 0.0f)
            {
                return;
            }

            CalculateMemoryStatistics();
            LogStatistics(profiler, terrainPerformanceSecondsPerLog);
        }

        int TerrainProfiler::GetTickOrder()
        {
            return AZ::TICK_LAST;
        }
        // Tick bus END
        //////////////////////////////////////////////////////////////////////////

        void TerrainProfiler::CalculateMemoryStatistics()
        {
            ICVar* pvar = m_console->GetCVar("e_TerrainPerformanceCollectMemoryStats");
            if (!pvar)
            {
                return;
            }
            const int cvarLogMemoryMask = pvar->GetIVal();

            if (cvarLogMemoryMask < 1)
            {
                return;
            }

            //Need to add support for this in New Terrain System.
            //See: LY-104900
            //See LegacyTerrain::TerrainProfiler::CalculateMemoryStatistics() for an example of what this might look like.
        }

        void TerrainProfiler::LogStatistics(AzFramework::Debug::StatisticalProfilerProxy::StatisticalProfilerType& profiler, float terrainPerformanceSecondsPerLog)
        {
            const AzFramework::Statistics::NamedRunningStatistic* statPerFrame = profiler.GetFirstStatPerFrame();
            if (!statPerFrame || statPerFrame->GetNumSamples() < 1)
            {
                return;
            }

            m_elapsedSecondsForLogging += m_timerForLogging.StampAndGetDeltaTimeInSeconds();
            if (m_elapsedSecondsForLogging < terrainPerformanceSecondsPerLog)
            {
                return;
            }

            AZ_Printf("TerrainPerformance", ">>>>>>>>>>>>>>>>>\n");
            profiler.LogAndResetStats("TerrainPerformance");
            AZ_Printf("TerrainPerformance", "<<<<<<<<<<<<<<<<<\n");
            m_elapsedSecondsForLogging = 0;
        }

    }; //namespace Debug
}; //namespace Terrain

#endif //#if !defined(_RELEASE)

#endif //LY_TERRAIN_RUNTIME
