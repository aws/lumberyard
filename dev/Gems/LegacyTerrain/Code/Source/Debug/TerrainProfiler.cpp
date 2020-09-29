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

#if !defined(_RELEASE)

#include "../CryCommon/CrySizer.h"
#include "../CryCommon/IResourceCollector.h"

#include "terrain.h"
#include "TerrainProfiler.h"

namespace LegacyTerrain
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

            void End() override {AZ_Assert(false, "Not Supported");}

            bool AddObject(const void* /*pIdentifier*/, size_t nSizeBytes, int nCount = 1) override
            {
                m_nTotalSize += nSizeBytes * nCount;
                return nSizeBytes > 0;
            }

            IResourceCollector* GetResourceCollector() override { return this; }
            void SetResourceCollector(IResourceCollector* /*pColl*/) override { AZ_Assert(false, "Not Supported");}

        protected:

            void Push(const char* szComponentName) override { };
            void PushSubcomponent(const char* szSubcomponentName) override { };
            void Pop() override { };

        private:
            size_t m_nTotalSize;
        };

        TerrainProfiler::TerrainProfiler() : m_elapsedSecondsForLogging(0.0f)
        {
            AzFramework::Debug::StatisticalProfilerProxy* profilerProxy = AZ::Interface<AzFramework::Debug::StatisticalProfilerProxy>::Get();
            if (!profilerProxy)
            {
                AZ_Warning("TerrainProfiler", false, "The Legacy Terrain Profiler couldn't find the StatisticalProfilerProxy");
                return;
            }
            AzFramework::Debug::StatisticalProfilerProxy::StatisticalProfilerType& profiler = profilerProxy->GetProfiler(AZ::Debug::ProfileCategory::LegacyTerrain);
            AzFramework::Statistics::StatisticsManager<>& statisticsManager = profiler.GetStatsManager();

            AZStd::string visibleTilesStatName("VisibleTiles");
            AZStd::string memoryUsageStatName("MemoryUsage");
            if (statisticsManager.GetCount() < 1)
            {

                statisticsManager.AddStatistic(visibleTilesStatName, visibleTilesStatName, "", true);
                statisticsManager.AddStatistic(memoryUsageStatName, memoryUsageStatName, "KB", true);

                statisticsManager.AddStatistic(StatisticCheckVisibility, StatisticCheckVisibility, "us", true);
                statisticsManager.AddStatistic(StatisticUpdateNodes, StatisticUpdateNodes, "us", true);
                statisticsManager.AddStatistic(StatisticDrawVisibleSectors, StatisticDrawVisibleSectors, "us", true);
                statisticsManager.AddStatistic(StatisticUpdateSectorMeshes, StatisticUpdateSectorMeshes, "us", true);
                statisticsManager.AddStatistic(StatisicLoadTimeFromDisk, StatisicLoadTimeFromDisk, "us", true);
                statisticsManager.AddStatistic(StatisticFxDrawTechnique, StatisticFxDrawTechnique, "us", true);

                AZStd::string timePerFrameStatName("TimePerFrame");
                AZStd::vector<AZStd::string> statsPerFrame;
                statsPerFrame.push_back(StatisticCheckVisibility);
                statsPerFrame.push_back(StatisticUpdateNodes);
                statsPerFrame.push_back(StatisticDrawVisibleSectors);
                statsPerFrame.push_back(StatisticUpdateSectorMeshes);

                int num_stats_per_frame = profiler.AddPerFrameStatisticalAggregate(statsPerFrame, timePerFrameStatName, timePerFrameStatName);
                AZ_Assert(num_stats_per_frame == statsPerFrame.size(), "Failed to mark %d stats for per frame calculations.", statsPerFrame.size() - num_stats_per_frame);

                AZStd::string timePerRendererFrameStatName("TimePerRenderFrame");
                statsPerFrame.clear();
                statsPerFrame.push_back(StatisticFxDrawTechnique);

                num_stats_per_frame = profiler.AddPerFrameStatisticalAggregate(statsPerFrame, timePerRendererFrameStatName, timePerRendererFrameStatName);
                AZ_Assert(num_stats_per_frame == statsPerFrame.size(), "Failed to mark %d stats for per render frame calculations.", statsPerFrame.size() - num_stats_per_frame);
            }

            statisticsManager.ResetAllStatistics();

            m_visibleTilesCountStat = statisticsManager.GetStatistic(visibleTilesStatName);
            m_memoryUsageStat = statisticsManager.GetStatistic(memoryUsageStatName);
            AZ_Assert(m_visibleTilesCountStat, "Failed to find visible tiles count statistic");
            AZ_Assert(m_memoryUsageStat, "Failed to find MemoryUsage statistic");

            profilerProxy->ActivateProfiler(AZ::Debug::ProfileCategory::LegacyTerrain, true);

            m_timerForLogging.Stamp();

            AZ::TickBus::Handler::BusConnect();
        };

        TerrainProfiler::~TerrainProfiler()
        {
            AZ::TickBus::Handler::BusDisconnect();

            auto proxy = AZ::Interface<AzFramework::Debug::StatisticalProfilerProxy>::Get();
            if (proxy)
            {
                proxy->ActivateProfiler(AZ::Debug::ProfileCategory::LegacyTerrain, false);
            }
        };

        //////////////////////////////////////////////////////////////////////////
        // Tick bus START
        void TerrainProfiler::OnTick(float deltaTime, AZ::ScriptTimePoint time)
        {
            auto profilerProxy = AZ::Interface<AzFramework::Debug::StatisticalProfilerProxy>::Get();
            if (!profilerProxy)
            {
                AZ_Warning("TerrainProfiler", false, "The Legacy Terrain Profiler couldn't find the StatisticalProfilerProxy");
                return;
            }
            AzFramework::Debug::StatisticalProfilerProxy::StatisticalProfilerType& profiler = profilerProxy->GetProfiler(AZ::Debug::ProfileCategory::LegacyTerrain);

            profiler.SummarizePerFrameStats();

            const float terrainPerformanceSecondsPerLog = GetCVarAsFloat("e_TerrainPerformanceSecondsPerLog");
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
            const int cvarLogMemoryMask = GetCVarAsInteger("e_TerrainPerformanceCollectMemoryStats");

            if (cvarLogMemoryMask < 1)
            {
                return;
            }

            SimpleSizer memorySizer;
            CTerrain* terrain = CTerrain::GetTerrain();
            AZ_Assert(terrain, "CTerrain::GetTerrain() returned NULL");
            if (!terrain->HasMacroTexture())
            {
                //The terrain data is incomplete. Not unusual, happens momentarily when the legacy terrain
                //level component is being added to the level when using the editor.
                return;
            }
            terrain->GetMemoryUsage(&memorySizer);
            const size_t memoryUsageBytes = memorySizer.GetTotalSize();
            const double memoryUsageKBytes = static_cast<double>(memoryUsageBytes) / 1024.0;
            m_memoryUsageStat->PushSample(memoryUsageKBytes);
        }

        void TerrainProfiler::LogStatistics(AzFramework::Debug::StatisticalProfilerProxy::StatisticalProfilerType& profiler,
            float terrainPerformanceSecondsPerLog)
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
}; //namespace LegacyTerrain

#endif //#if !defined(_RELEASE)
