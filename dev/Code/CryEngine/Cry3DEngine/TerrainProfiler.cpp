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

#include <AzCore/Debug/FrameProfilerComponent.h>

#include "../CryCommon/CrySizer.h"
#include "../CryCommon/IResourceCollector.h"

#include "TerrainProfiler.h"

namespace AZ
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


        TerrainProfiler* TerrainProfiler::m_instance = nullptr;

        TerrainProfiler::TerrainProfiler() : m_elapsedSecondsForLogging(0.0f)
        {
            m_perTileStatisticsManager.AddStatistic("VisibleTiles", "");
            m_perTileStatisticsManager.AddStatistic("TimePerTiles", "us");
            m_perTileStatisticsManager.AddStatistic("TimePerVisibleTiles", "us");
            m_perTileStatisticsManager.AddStatistic("MemoryUsage", "KB");

            m_visibleTilesCountStat = m_perTileStatisticsManager.GetStatistic("VisibleTiles");
            AZ_Assert(m_visibleTilesCountStat, "Failed to find visible tiles count statistic");
            m_performanceAcrossAllTilesStat = m_perTileStatisticsManager.GetStatistic("TimePerTiles");
            AZ_Assert(m_performanceAcrossAllTilesStat, "Failed to find TimePerTiles statistic");
            m_performanceAcrossVisibleTilesStat = m_perTileStatisticsManager.GetStatistic("TimePerVisibleTiles");
            AZ_Assert(m_performanceAcrossVisibleTilesStat, "Failed to find TimePerVisibleTiles statistic");
            m_memoryUsageStat = m_perTileStatisticsManager.GetStatistic("MemoryUsage");
            AZ_Assert(m_memoryUsageStat, "Failed to find MemoryUsage statistic");

            FrameProfilerBus::Handler::BusConnect();
            m_profilingEntity = AZStd::make_unique<Entity>();
            m_profilingEntity->CreateComponent<FrameProfilerComponent>();
            m_profilingEntity->Init();
           
            // We'll only activate our profiler if we've set the cvar to capture timing data.
            UpdateFrameProfilerStatus(m_profilingEntity);

            m_timerForLogging.Stamp();
        };

        TerrainProfiler::~TerrainProfiler() 
        {
            if (m_profilingEntity->GetState() == AZ::Entity::State::ES_ACTIVE)
            {
                m_profilingEntity->Deactivate();
            }
            m_profilingEntity = nullptr;
            FrameProfilerBus::Handler::BusDisconnect();
        };

        void TerrainProfiler::UpdateFrameProfilerStatus(AZStd::unique_ptr<Entity>& profilingEntity)
        {
            // Activate / deactivate the timing captures as necessary.
            // NOTE:  We're being careful to only activate if/when the terrain profiler has been told to capture data
            // and to deactivate as soon as we should stop, since activating a FrameProfilerComponent will cause timing
            // captures across the entire engine, not just terrain.  This can cause a measureable performance impact.
            if (GetCVars()->e_TerrainPerformanceSecondsPerLog > 0.0f)
            {
                if (profilingEntity->GetState() == AZ::Entity::State::ES_INIT)
                {
                    profilingEntity->Activate();
                }
            }
            else
            {
                if (profilingEntity->GetState() == AZ::Entity::State::ES_ACTIVE)
                {
                    profilingEntity->Deactivate();
                }
            }

        }

        //FrameProfilerBus
        void TerrainProfiler::OnFrameProfilerData(const FrameProfiler::ThreadDataArray& data)
        {
            const float terrainPerformanceSecondsPerLog = GetCVars()->e_TerrainPerformanceSecondsPerLog;
            if (terrainPerformanceSecondsPerLog <= 0.0f)
            {
                return;
            }

            bool gotData = false;
            for (size_t iThread = 0; iThread < data.size(); ++iThread)
            {
                const FrameProfiler::ThreadData& threadData = data[iThread];
                FrameProfiler::ThreadData::RegistersMap::const_iterator registerIterator = threadData.m_registers.begin();
                for (; registerIterator != threadData.m_registers.end(); ++registerIterator)
                {
                    const FrameProfiler::RegisterData& registerData = registerIterator->second;
                    AZ::u32 terrainCrc = AZ_CRC("TerrainProfiler", 0xf048dd73);
                    if (terrainCrc != registerData.m_systemId)
                    {
                        //Some other system appears to be using the FrameProfilerComponent & Friends.
                        continue;
                    }

                    //registerData.m_function has the function name.
                    if (registerData.m_type == ProfilerRegister::Type::PRT_TIME)
                    {
                        gotData = true;
                        const FrameProfiler::FrameData& frameData = registerData.m_frames.back();
                        m_timeStatisticsManager.PushTimeDataSample(registerData.m_name, frameData.m_timeData);
                    }
                }
            }

            if (!gotData)
            {
                return;
            }

            CalculateMemoryStatistics();
            CalculateStatisticsPerTile();
            LogStatistics(terrainPerformanceSecondsPerLog);
        }

        void TerrainProfiler::CalculateMemoryStatistics()
        {

            const int cvarLogMemoryMask = GetCVars()->e_TerrainPerformanceCollectMemoryStats;

            if (cvarLogMemoryMask < 1)
            {
                return;
            }

            SimpleSizer memorySizer;
            CTerrain* terrain = GetTerrain();
            AZ_Assert(terrain, "GetTerrain() returned NULL");
            terrain->GetMemoryUsage(&memorySizer);
            const size_t memoryUsageBytes = memorySizer.GetTotalSize();
            const double memoryUsageKBytes = static_cast<double>(memoryUsageBytes) / 1024.0;
            m_memoryUsageStat->PushSample(memoryUsageKBytes);
        }

        void TerrainProfiler::CalculateStatisticsPerTile()
        {
            double allStatsSumMicroSecs = 0.0;
            const AZStd::vector<AzFramework::Statistics::NamedRunningStatistic>& timeStatsVector = m_timeStatisticsManager.GetAllStatistics();
            for (const AzFramework::Statistics::NamedRunningStatistic& stat : timeStatsVector)
            {
                allStatsSumMicroSecs += stat.GetMostRecentSample();
            }

            if (m_totalTilesCount)
            {
                m_performanceAcrossAllTilesStat->PushSample(allStatsSumMicroSecs / m_totalTilesCount);
                const double visibleTilesCount = m_visibleTilesCountStat->GetMostRecentSample();
                if (visibleTilesCount > 0.0)
                {
                    m_performanceAcrossVisibleTilesStat->PushSample(allStatsSumMicroSecs / visibleTilesCount);
                }
            }
        }

        void TerrainProfiler::LogAndResetStatManager(AzFramework::Statistics::RunningStatisticsManager& statsManager)
        {
            const AZStd::vector<AzFramework::Statistics::NamedRunningStatistic>& statsVector = statsManager.GetAllStatistics();
            for (const AzFramework::Statistics::NamedRunningStatistic& stat : statsVector)
            {
                if (stat.GetNumSamples() == 0)
                {
                    continue;
                }
                const AZStd::string& statReport = stat.GetFormatted();
                AZ_Printf("TerrainPerformance", "%s\n", statReport.c_str());
            }
            statsManager.ResetAllStatistics();
        }

        void TerrainProfiler::LogStatistics(float terrainPerformanceSecondsPerLog)
        {
            m_elapsedSecondsForLogging += m_timerForLogging.StampAndGetDeltaTimeInSeconds();
            if (m_elapsedSecondsForLogging < terrainPerformanceSecondsPerLog)
            {
                return;
            }

            AZ_Printf("TerrainPerformance", ">>>>>>>>>>>>>>>>>\n");
            LogAndResetStatManager(m_timeStatisticsManager);
            LogAndResetStatManager(m_perTileStatisticsManager);
            AZ_Printf("TerrainPerformance", "<<<<<<<<<<<<<<<<<\n");
            m_elapsedSecondsForLogging = 0;
        }
    }; //namespace Debug
}; //namespace AZ

#endif //#if !defined(_RELEASE)