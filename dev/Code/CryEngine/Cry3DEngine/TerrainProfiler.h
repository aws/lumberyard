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

//These are the register names that should be used in CTerrain when
//calling AZ_PROFILE_TIMER
//Declared before #if !defined(_RELEASE) to mitigate cluttering CTerrain
//code with #if !defined(_RELEASE) when calling AZ_PROFILE_TIMER.
#define TERRAIN_STATISTIC_CHECK_VISIBILITY     "CheckVisibility"
#define TERRAIN_STATISTIC_UPDATE_NODES         "UpdateNodesIncrementally"
#define TERRAIN_STATISTIC_DRAW_VISIBLE_SECTORS "DrawVisibleSectors"
#define TERRAIN_STATISTIC_UPDATE_SECTOR_MESHES "UpdateSectorMeshes"
#define TERRAIN_STATISTIC_LOAD_TIME_FROM_DISK  "LoadTimeFromDisk"

#if !defined(_RELEASE)

#include <AzCore/Debug/Timer.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Debug/FrameProfilerBus.h>
#include <AzFramework/Statistics/TimeDataStatisticsManager.h>

namespace AZ
{
    namespace Debug
    {
        /**
         * Singleton that helps collect performance statistics of the Terrain system.
         * Inherits from Cry3DEngineBase just to get hassle free access to the CVars.
         */
        class TerrainProfiler
            : public Cry3DEngineBase
            , public FrameProfilerBus::Handler
        {
        public:
            ~TerrainProfiler();

            //FrameProfilerBus
            void OnFrameProfilerData(const FrameProfiler::ThreadDataArray& data) override;

            //Used to calculate per-tile statistics.
            void SetVisibleTilesCount(int visibleTilesCount)
            {
                m_visibleTilesCountStat->PushSample(AZStd::GetMax(0, visibleTilesCount));
            }

            ///Used to calculate per-tile statistics.
            void SetTotalTilesCount(int totalTilesCount)
            {
                m_totalTilesCount = AZStd::GetMax(0, totalTilesCount);
            }

            static TerrainProfiler& GetInstance()
            {
                if (!m_instance)
                {
                    m_instance = new AZ::Debug::TerrainProfiler();
                }
                AZ_Assert(m_instance != nullptr, "Failed to instantiate the TerrainProfiler");
                return *m_instance;
            }

            static void DestroyInstance()
            {
                SAFE_DELETE(m_instance);
            }

            /**
             * Ensure that we only enable the frame profiler when needed, and disable it when we don't.
             */
            static void RefreshFrameProfilerStatus()
            {
                if ((m_instance) && (m_instance->m_profilingEntity))
                {
                    m_instance->UpdateFrameProfilerStatus(m_instance->m_profilingEntity);
                }
            }

        private:
            TerrainProfiler();

            void UpdateFrameProfilerStatus(AZStd::unique_ptr<Entity>& profilingEntity);

            void CalculateStatisticsPerTile();

            void CalculateMemoryStatistics();

            /**
             * Helper for LogStatistics().
             */
            void LogAndResetStatManager(AzFramework::Statistics::RunningStatisticsManager& statsManager);

            /**
             * Dumps all statistics in the Game Log only if
             * m_elapsedSecondsForLogging > @terrainPerformanceSecondsPerLog.
             * Once the data is dumped, m_elapsedSecondsForLogging is set to Zero and
             * all collected statistics are Reset.
             */
            void LogStatistics(float terrainPerformanceSecondsPerLog);

            ///The one and only.
            static TerrainProfiler* m_instance;

            ///Dummy entity. Used as a means to activate the FrameProfilerComponent.
            AZStd::unique_ptr<Entity> m_profilingEntity;

            ///Statistics Manager for data generated via AZ_PROFILE_TIMER()
            AzFramework::Statistics::TimeDataStatisticsManager m_timeStatisticsManager;
            ///A Regular RunningStatisticsManager that is used to prorate performance statistics
            ///Across the total number of Tiles in the terrain and the total count of
            ///visible terrain tiles per frame.
            AzFramework::Statistics::RunningStatisticsManager m_perTileStatisticsManager;
            u32 m_totalTilesCount; ///Used to calculate performance across all tiles.

            ///This timer in conjunction with the CVar e_TerrainPerformanceSecondsPerLog,
            ///it is used to know when is time to dump all statistics in the Game Log.
            Timer m_timerForLogging;
            float m_elapsedSecondsForLogging;

            ///////////////////////////////////////////////////////////////////
            // The following pointers are shortcuts into m_perTileStatsManager.
            ///////////////////////////////////////////////////////////////////

            ///Visible tiles usually change frame to frame.
            AzFramework::Statistics::NamedRunningStatistic* m_visibleTilesCountStat;

            ///Ad hoc running statistic based on the sum of the most recent samples
            ///from all statistics collected in m_timeStatisticsManager divided by
            ///the number of total tiles. 
            AzFramework::Statistics::NamedRunningStatistic* m_performanceAcrossAllTilesStat;
            
            ///Ad hoc running statistic based on the sum of the most recent samples
            ///from all statistics collected in m_timeStatisticsManager divided by
            ///the number of visible tiles. 
            AzFramework::Statistics::NamedRunningStatistic* m_performanceAcrossVisibleTilesStat;

            ///Ad hoc running statistic that collects all the RAM consumed by CTerrain
            ///and its member variables per frame.
            AzFramework::Statistics::NamedRunningStatistic* m_memoryUsageStat;

        }; //class TerrainProfiler

    }; //namespace Debug
}; //namespace AZ

#endif //#if !defined(_RELEASE)