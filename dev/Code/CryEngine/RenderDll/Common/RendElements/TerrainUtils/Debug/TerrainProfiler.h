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

#ifdef LY_TERRAIN_RUNTIME

#if !defined(_RELEASE)

#include <AzCore/Debug/Timer.h>
#include <AzCore/Component/TickBus.h>
#include <AzFramework/Debug/StatisticalProfilerProxy.h>

#define TERRAIN_SCOPE_PROFILE(profiler, scopeNameId) AZ_PROFILE_SCOPE(profiler, scopeNameId)

namespace Terrain
{
    namespace Debug
    {
        static constexpr const char* StatisticMfPrepare = "mfPrepare";
        static constexpr const char* StatisticMfDrawNodeGather = "mfDraw_NodeGather";
        static constexpr const char* StatisticMfDrawGatherTextureTiles = "mfDraw_NodeGather_TextureTiles";
        static constexpr const char* StatisticMfDrawTerrainDraw = "mfDraw_TerrainDraw";
        static constexpr const char* StatisticVisibleTextureTiles = "VisibleTextureTiles";
        static constexpr const char* StatisticVisibleDataTiles = "VisibleDataTiles";

        /**
        * Helps collect performance statistics of the Terrain system.
        */
        class TerrainProfiler : public AZ::TickBus::Handler
        {
        public:
            TerrainProfiler() = delete;
            TerrainProfiler(IConsole* console);
            virtual ~TerrainProfiler();

            //////////////////////////////////////////////////////////////////////////
            // Tick bus
            void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
            int GetTickOrder() override;
            //////////////////////////////////////////////////////////////////////////

            void SetVisibleDataTilesCount(int visibleTilesCount)
            {
                m_visibleDataTilesCountStat->PushSample(AZStd::GetMax(0, visibleTilesCount));
            }

            void SetVisibleTextureTilesCount(int visibleTilesCount)
            {
                m_visibleTextureTilesCountStat->PushSample(AZStd::GetMax(0, visibleTilesCount));
            }

        private:
            //Need to add support for this in New Terrain System.
            //See: LY-104900
            void CalculateMemoryStatistics();

            /**
             * Dumps all statistics in the Game Log only if
             * m_elapsedSecondsForLogging > @terrainPerformanceSecondsPerLog.
             * Once the data is dumped, m_elapsedSecondsForLogging is set to Zero and
             * all collected statistics are Reset.
             */
            void LogStatistics(AzFramework::Debug::StatisticalProfilerProxy::StatisticalProfilerType& profiler, float terrainPerformanceSecondsPerLog);

            //! To access the cvars.
            IConsole* m_console;

            //! This timer in conjunction with the CVar e_TerrainPerformanceSecondsPerLog,
            //! it is used to know when is time to dump all statistics in the Game Log.
            AZ::Debug::Timer m_timerForLogging;
            float m_elapsedSecondsForLogging;

            ///////////////////////////////////////////////////////////////////
            // The following pointers are shortcuts into m_statisticsManager.
            ///////////////////////////////////////////////////////////////////

            //! Visible data tiles usually change frame to frame (if the camera moves).
            //! In the context of the new terrain system the data tiles are tiles that
            //! produce meshes based on the heightmap data.
            AzFramework::Statistics::NamedRunningStatistic* m_visibleDataTilesCountStat;

            //! Visible texture tiles usually change frame to frame  (if the camera moves).
            //! In the context of the new terrain system the texture tiles are tiles that
            //! are mapped to a virtual texture and have far more LOD granularity than data tiles.
            //! By default, The data tiles are split into 6 LODs, while the texture tiles are split into 10 LODs.
            AzFramework::Statistics::NamedRunningStatistic* m_visibleTextureTilesCountStat;

            //! Ad hoc running statistic that collects all the RAM consumed by CTerrain
            //! and its member variables per frame.
            AzFramework::Statistics::NamedRunningStatistic* m_memoryUsageStat;

        }; //class TerrainProfiler

    }; //namespace Debug
}; //namespace Terrain

#else //#if !defined(_RELEASE)

#define TERRAIN_SCOPE_PROFILE(profiler, scopeNameId)

#endif //#if !defined(_RELEASE)

#endif //#ifdef LY_TERRAIN_RUNTIME
