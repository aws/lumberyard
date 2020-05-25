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

#if !defined(_RELEASE)

#include <AzCore/Debug/Timer.h>
#include <AzCore/Component/TickBus.h>
#include <AzFramework/Debug/StatisticalProfilerProxy.h>
#include "Terrain/LegacyTerrainBase.h"

#define TERRAIN_SCOPE_PROFILE(profiler, scopeNameId) AZ_PROFILE_SCOPE(profiler, scopeNameId)

namespace LegacyTerrain
{
    namespace Debug
    {
        static constexpr const char* StatisticCheckVisibility = "CheckVisibility";
        static constexpr const char* StatisticUpdateNodes = "UpdateNodesIncrementally";
        static constexpr const char* StatisticDrawVisibleSectors = "DrawVisibleSectors";
        static constexpr const char* StatisticUpdateSectorMeshes = "UpdateSectorMeshes";
        static constexpr const char* StatisicLoadTimeFromDisk = "LoadTimeFromDisk";
        static constexpr const char* StatisticFxDrawTechnique = "FX_DrawTechnique";

        /**
         * Helps collect performance statistics of the Terrain system.
         * Inherits from Cry3DEngineBase just to get hassle free access to the CVars.
         */
        class TerrainProfiler
            : public LegacyTerrainBase
            , public AZ::TickBus::Handler
        {
        public:
            TerrainProfiler();
            virtual ~TerrainProfiler();

            //////////////////////////////////////////////////////////////////////////
            // Tick bus
            void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
            int GetTickOrder() override;
            //////////////////////////////////////////////////////////////////////////

            void SetVisibleTilesCount(int visibleTilesCount)
            {
                m_visibleTilesCountStat->PushSample(AZStd::GetMax(0, visibleTilesCount));
            }

        private:
            void CalculateMemoryStatistics();

            /**
             * Dumps all statistics in the Game Log only if
             * m_elapsedSecondsForLogging > @terrainPerformanceSecondsPerLog.
             * Once the data is dumped, m_elapsedSecondsForLogging is set to Zero and
             * all collected statistics are Reset.
             */
            void LogStatistics(AzFramework::Debug::StatisticalProfilerProxy::StatisticalProfilerType& profiler,
                float terrainPerformanceSecondsPerLog);

            ///This timer in conjunction with the CVar e_TerrainPerformanceSecondsPerLog,
            ///it is used to know when is time to dump all statistics in the Game Log.
            AZ::Debug::Timer m_timerForLogging;
            float m_elapsedSecondsForLogging;


            ///////////////////////////////////////////////////////////////////
            // The following pointers are shortcuts into m_statisticsManager.
            ///////////////////////////////////////////////////////////////////

            //! Visible tiles usually change frame to frame (if the camera moves).
            AzFramework::Statistics::NamedRunningStatistic* m_visibleTilesCountStat;

            //! Ad hoc running statistic that collects all the RAM consumed by CTerrain
            //! and its member variables per frame.
            AzFramework::Statistics::NamedRunningStatistic* m_memoryUsageStat;

        }; //class TerrainProfiler

    }; //namespace Debug
}; //namespace LegacyTerrain

#else //#if !defined(_RELEASE)

#define TERRAIN_SCOPE_PROFILE(profiler, scopeNameId)

#endif //#if !defined(_RELEASE)
