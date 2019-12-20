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
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <AzTest/AzTest.h>

#include <AzCore/std/string/string.h>


#include <AzFramework/Statistics/TimeDataStatisticsManager.h>

#include <AzCore/Math/Crc.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Debug/FrameProfilerBus.h>
#include <AzCore/Debug/FrameProfilerComponent.h>

using namespace AZ;
using namespace Debug;
using namespace AzFramework;

namespace UnitTest
{
    class StatisticsTest
        : public AllocatorsFixture
    {
    public:
        StatisticsTest()
        {
        }

        void SetUp() override
        {
            AllocatorsFixture::SetUp();

            m_dataSamples = AZStd::make_unique<AZStd::vector<u32>>();
            const u32 numSamples = 100;
            m_dataSamples->set_capacity(numSamples);
            for (u32 i = 0; i < numSamples; ++i)
            {
                m_dataSamples->push_back(i);
            }
        }

        ~StatisticsTest()
        {
        }

        void TearDown() override
        {
            m_dataSamples = nullptr;
            AllocatorsFixture::TearDown();
        }

    protected:
        AZStd::unique_ptr<AZStd::vector<u32>> m_dataSamples;
    }; //class StatisticsTest

    TEST_F(StatisticsTest, RunningStatistic_ProcessAnArrayOfNumbers_GetExpectedStatisticalData)
    {
        Statistics::RunningStatistic runningStat;

        ASSERT_TRUE(m_dataSamples.get() != nullptr);
        const AZStd::vector<u32>& dataSamples = *m_dataSamples;
        for (u32 sample : dataSamples)
        {
            runningStat.PushSample(sample);
        }
        
        EXPECT_EQ(runningStat.GetNumSamples(), dataSamples.size());
        EXPECT_EQ(runningStat.GetMostRecentSample(), dataSamples.back());
        EXPECT_EQ(runningStat.GetMinimum(), dataSamples[0]);
        EXPECT_EQ(runningStat.GetMaximum(), dataSamples.back());
        EXPECT_NEAR(runningStat.GetAverage(), 49.5, 0.001);
        EXPECT_NEAR(runningStat.GetVariance(), 841.666, 0.001);
        EXPECT_NEAR(runningStat.GetStdev(), 29.011, 0.001);
        EXPECT_NEAR(runningStat.GetVariance(Statistics::VarianceType::P), 833.25, 0.001);
        EXPECT_NEAR(runningStat.GetStdev(Statistics::VarianceType::P), 28.866, 0.001);

        //Reset the stat object.
        runningStat.Reset();
        EXPECT_EQ(runningStat.GetNumSamples(), 0);
        EXPECT_EQ(runningStat.GetAverage(), 0.0);
        EXPECT_EQ(runningStat.GetStdev(), 0.0);
    }


    TEST_F(StatisticsTest, RunningStatisticsManager_AddAndRemoveStatisticistics_CollectionIntegrityIsCorrect)
    {
        Statistics::RunningStatisticsManager statsManager;
        AZStd::string statName0("stat0");
        AZStd::string statName1("stat1");
        AZStd::string statName2("stat2");
        AZStd::string statName3("stat3");
        EXPECT_TRUE(statsManager.AddStatistic(statName0, ""));
        EXPECT_TRUE(statsManager.AddStatistic(statName1, ""));
        EXPECT_TRUE(statsManager.AddStatistic(statName2, ""));
        EXPECT_TRUE(statsManager.AddStatistic(statName3, ""));

        //Validate the number of running statistics object we have so far.
        {
            const AZStd::vector<Statistics::NamedRunningStatistic>& allStats = statsManager.GetAllStatistics();
            EXPECT_TRUE(allStats.size() == 4);
        }

        //Try to add an Stat that already exist. expect to fail.
        EXPECT_FALSE(statsManager.AddStatistic(statName1, ""));

        //Remove stat1.
        statsManager.RemoveStatistic(statName1);
        //Validate the number of running statistics object we have so far.
        {
            const AZStd::vector<Statistics::NamedRunningStatistic>& allStats = statsManager.GetAllStatistics();
            EXPECT_TRUE(allStats.size() == 3);
        }

        //Add stat1 again, expect to pass.
        EXPECT_TRUE(statsManager.AddStatistic(statName1, ""));

        //Get a pointer to stat2.
        Statistics::NamedRunningStatistic* stat2 = statsManager.GetStatistic(statName2);
        ASSERT_TRUE(stat2 != nullptr);
        EXPECT_EQ(stat2->GetName(), statName2);

        //Validate the last statistic is stat1.
        {
            const AZStd::vector<Statistics::NamedRunningStatistic>& allStats = statsManager.GetAllStatistics();
            EXPECT_EQ(allStats[allStats.size() - 1].GetName(), statName1);
        }
    }

    TEST_F(StatisticsTest, RunningStatisticsManager_DistributeSamplesAcrossStatistics_StatisticsAreCorrect)
    {
        Statistics::RunningStatisticsManager statsManager;
        AZStd::string statName0("stat0");
        AZStd::string statName1("stat1");
        AZStd::string statName2("stat2");
        AZStd::string statName3("stat3");
        
        EXPECT_TRUE(statsManager.AddStatistic(statName3, ""));
        EXPECT_TRUE(statsManager.AddStatistic(statName0, ""));
        EXPECT_TRUE(statsManager.AddStatistic(statName2, ""));
        EXPECT_TRUE(statsManager.AddStatistic(statName1, ""));

        //Distribute the 100 samples of data evenly across the 4 running statistics.
        ASSERT_TRUE(m_dataSamples.get() != nullptr);
        const AZStd::vector<u32>& dataSamples = *m_dataSamples;
        const size_t numSamples = dataSamples.size();
        const size_t numSamplesPerStat = numSamples / 4;
        size_t sampleIndex = 0;
        size_t nextStopIndex = numSamplesPerStat;
        while (sampleIndex < nextStopIndex)
        {
            statsManager.PushSampleForStatistic(statName0, dataSamples[sampleIndex]);
            sampleIndex++;
        }
        nextStopIndex += numSamplesPerStat;
        while (sampleIndex < nextStopIndex)
        {
            statsManager.PushSampleForStatistic(statName1, dataSamples[sampleIndex]);
            sampleIndex++;
        }
        nextStopIndex += numSamplesPerStat;
        while (sampleIndex < nextStopIndex)
        {
            statsManager.PushSampleForStatistic(statName2, dataSamples[sampleIndex]);
            sampleIndex++;
        }
        nextStopIndex += numSamplesPerStat;
        while (sampleIndex < nextStopIndex)
        {
            statsManager.PushSampleForStatistic(statName3, dataSamples[sampleIndex]);
            sampleIndex++;
        }

        EXPECT_NEAR(statsManager.GetStatistic(statName0)->GetAverage(), 12.0, 0.001);
        EXPECT_NEAR(statsManager.GetStatistic(statName1)->GetAverage(), 37.0, 0.001);
        EXPECT_NEAR(statsManager.GetStatistic(statName2)->GetAverage(), 62.0, 0.001);
        EXPECT_NEAR(statsManager.GetStatistic(statName3)->GetAverage(), 87.0, 0.001);

        EXPECT_NEAR(statsManager.GetStatistic(statName0)->GetStdev(), 7.359, 0.001);
        EXPECT_NEAR(statsManager.GetStatistic(statName1)->GetStdev(), 7.359, 0.001);
        EXPECT_NEAR(statsManager.GetStatistic(statName2)->GetStdev(), 7.359, 0.001);
        EXPECT_NEAR(statsManager.GetStatistic(statName3)->GetStdev(), 7.359, 0.001);

        //Reset one of the stats.
        statsManager.ResetStatistic(statName2);
        EXPECT_EQ(statsManager.GetStatistic(statName2)->GetAverage(), 0.0);
        //Reset all of the stats.
        statsManager.ResetAllStatistics();
        EXPECT_EQ(statsManager.GetStatistic(statName0)->GetNumSamples(), 0);
        EXPECT_EQ(statsManager.GetStatistic(statName1)->GetNumSamples(), 0);
        EXPECT_EQ(statsManager.GetStatistic(statName2)->GetNumSamples(), 0);
        EXPECT_EQ(statsManager.GetStatistic(statName3)->GetNumSamples(), 0);
    }

    //End of all Tests from StatisticsTest

    /**
     * Validate functionality of the convenience class TimeDataStatisticsManager.
     * It is a specialized version of RunningStatisticsManager that works with Timer type
     * of registers that can be captured with the FrameProfilerBus::OnFrameProfilerData()
     */
    class TimeDataStatisticsManagerTest
        : public AllocatorsFixture
        , public FrameProfilerBus::Handler
    {
        static constexpr const char* PARENT_TIMER_STAT = "ParentStat";
        static constexpr const char* CHILD_TIMER_STAT0 = "ChildStat0";
        static constexpr const char* CHILD_TIMER_STAT1 = "ChildStat1";

    public:
        TimeDataStatisticsManagerTest()
            : AllocatorsFixture()
        {
        }

        void SetUp() override
        {
            AllocatorsFixture::SetUp();
            m_statsManager = AZStd::make_unique<Statistics::TimeDataStatisticsManager>();
        }

        void TearDown() override
        {
            m_statsManager = nullptr;
            AllocatorsFixture::TearDown();
        }

        //////////////////////////////////////////////////////////////////////////
        // FrameProfilerBus
        virtual void OnFrameProfilerData(const FrameProfiler::ThreadDataArray& data)
        {
            for (size_t iThread = 0; iThread < data.size(); ++iThread)
            {
                const FrameProfiler::ThreadData& td = data[iThread];
                FrameProfiler::ThreadData::RegistersMap::const_iterator regIt = td.m_registers.begin();
                for (; regIt != td.m_registers.end(); ++regIt)
                {
                    const FrameProfiler::RegisterData& rd = regIt->second;
                    u32 unitTestCrc = AZ_CRC("UnitTest", 0x8089cea8);
                    if (unitTestCrc != rd.m_systemId)
                    {
                        continue; //Not for us.
                    }
                    ASSERT_EQ(ProfilerRegister::PRT_TIME, rd.m_type);
                    const FrameProfiler::FrameData& fd = rd.m_frames.back();
                    m_statsManager->PushTimeDataSample(rd.m_name, fd.m_timeData);
                }
            }
        }
        //////////////////////////////////////////////////////////////////////////

        int ChildFunction0(int numIterations, int sleepTimeMilliseconds)
        {
            AZ_PROFILE_TIMER("UnitTest", CHILD_TIMER_STAT0);
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(sleepTimeMilliseconds));
            int result = 5;
            for (int i = 0; i < numIterations; ++i)
            {
                result += i % 3;
            }
            return result;
        }

        int ChildFunction1(int numIterations, int sleepTimeMilliseconds)
        {
            AZ_PROFILE_TIMER("UnitTest", CHILD_TIMER_STAT1);
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(sleepTimeMilliseconds));
            int result = 5;
            for (int i = 0; i < numIterations; ++i)
            {
                result += i % 3;
            }
            return result;
        }

        int ParentFunction(int numIterations, int sleepTimeMilliseconds)
        {
            AZ_PROFILE_TIMER("UnitTest", PARENT_TIMER_STAT);
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(sleepTimeMilliseconds));
            int result = 0;
            result += ChildFunction0(numIterations, sleepTimeMilliseconds);
            result += ChildFunction1(numIterations, sleepTimeMilliseconds);
            return result;
        }

        void run()
        {
           Debug::FrameProfilerBus::Handler::BusConnect();
           
           ComponentApplication app;
           ComponentApplication::Descriptor desc;
           desc.m_useExistingAllocator = true;
           desc.m_enableDrilling = false;  // we already created a memory driller for the test (AllocatorsFixture)
           ComponentApplication::StartupParameters startupParams;
           startupParams.m_allocator = &AllocatorInstance<SystemAllocator>::Get();
           Entity* systemEntity = app.Create(desc, startupParams);
           systemEntity->CreateComponent<FrameProfilerComponent>();
           
           systemEntity->Init();
           systemEntity->Activate(); // start frame component
           
           const int sleepTimeAllFuncsMillis = 1;
           const int numIterations = 10;
           for (int iterationCounter = 0; iterationCounter < numIterations; ++iterationCounter)
           {
               ParentFunction(numIterations, sleepTimeAllFuncsMillis);
               //Collect all samples.
               app.Tick();
           }

           //Verify we have three running stats.
           {
               const AZStd::vector<Statistics::NamedRunningStatistic>& allStats = m_statsManager->GetAllStatistics();
               EXPECT_EQ(allStats.size(), 3);
           }

           AZStd::string parentStatName(PARENT_TIMER_STAT);
           AZStd::string child0StatName(CHILD_TIMER_STAT0);
           AZStd::string child1StatName(CHILD_TIMER_STAT1);
           ASSERT_TRUE(m_statsManager->GetStatistic(parentStatName) != nullptr);
           ASSERT_TRUE(m_statsManager->GetStatistic(child0StatName) != nullptr);
           ASSERT_TRUE(m_statsManager->GetStatistic(child1StatName) != nullptr);
           
           EXPECT_EQ(m_statsManager->GetStatistic(parentStatName)->GetNumSamples(), numIterations);
           EXPECT_EQ(m_statsManager->GetStatistic(child0StatName)->GetNumSamples(), numIterations);
           EXPECT_EQ(m_statsManager->GetStatistic(child1StatName)->GetNumSamples(), numIterations);

           const double minimumExpectDurationOfChildFunctionMicros = sleepTimeAllFuncsMillis * 1000;
           const double minimumExpectDurationOfParentFunctionMicros = (sleepTimeAllFuncsMillis * 1000) + 2 * minimumExpectDurationOfChildFunctionMicros;

           EXPECT_GE(m_statsManager->GetStatistic(parentStatName)->GetMinimum(), minimumExpectDurationOfParentFunctionMicros);
           EXPECT_GE(m_statsManager->GetStatistic(parentStatName)->GetAverage(), minimumExpectDurationOfParentFunctionMicros);
           EXPECT_GE(m_statsManager->GetStatistic(parentStatName)->GetMaximum(), minimumExpectDurationOfParentFunctionMicros);

           EXPECT_GE(m_statsManager->GetStatistic(child0StatName)->GetMinimum(), minimumExpectDurationOfChildFunctionMicros);
           EXPECT_GE(m_statsManager->GetStatistic(child0StatName)->GetAverage(), minimumExpectDurationOfChildFunctionMicros);
           EXPECT_GE(m_statsManager->GetStatistic(child0StatName)->GetMaximum(), minimumExpectDurationOfChildFunctionMicros);
                                                                                                                      
           EXPECT_GE(m_statsManager->GetStatistic(child1StatName)->GetMinimum(), minimumExpectDurationOfChildFunctionMicros);
           EXPECT_GE(m_statsManager->GetStatistic(child1StatName)->GetAverage(), minimumExpectDurationOfChildFunctionMicros);
           EXPECT_GE(m_statsManager->GetStatistic(child1StatName)->GetMaximum(), minimumExpectDurationOfChildFunctionMicros);

           EXPECT_GE(m_statsManager->GetStatistic(parentStatName)->GetMinimum(),
               m_statsManager->GetStatistic(child0StatName)->GetMinimum() + m_statsManager->GetStatistic(child1StatName)->GetMinimum());
           EXPECT_GE(m_statsManager->GetStatistic(parentStatName)->GetAverage(),
               m_statsManager->GetStatistic(child0StatName)->GetAverage() + m_statsManager->GetStatistic(child1StatName)->GetAverage());
           EXPECT_GE(m_statsManager->GetStatistic(parentStatName)->GetMaximum(),
               m_statsManager->GetStatistic(child0StatName)->GetMaximum() + m_statsManager->GetStatistic(child1StatName)->GetMaximum());

           //Let's validate TimeDataStatisticsManager::RemoveStatistics()
           m_statsManager->RemoveStatistic(child1StatName);
           ASSERT_TRUE(m_statsManager->GetStatistic(parentStatName) != nullptr);
           ASSERT_TRUE(m_statsManager->GetStatistic(child0StatName) != nullptr);
           EXPECT_EQ(m_statsManager->GetStatistic(child1StatName), nullptr);

           //Let's store the sample count for both parentStatName and child0StatName.
           const AZ::u64 numSamplesParent = m_statsManager->GetStatistic(parentStatName)->GetNumSamples();
           const AZ::u64 numSamplesChild0 = m_statsManager->GetStatistic(child0StatName)->GetNumSamples();

           //Let's call child1 function again and call app.Tick(). child1StatName should be readded to m_statsManager.
           ChildFunction1(numIterations, sleepTimeAllFuncsMillis);
           app.Tick();
           ASSERT_TRUE(m_statsManager->GetStatistic(child1StatName) != nullptr);
           EXPECT_EQ(m_statsManager->GetStatistic(parentStatName)->GetNumSamples(), numSamplesParent);
           EXPECT_EQ(m_statsManager->GetStatistic(child0StatName)->GetNumSamples(), numSamplesChild0);
           EXPECT_EQ(m_statsManager->GetStatistic(child1StatName)->GetNumSamples(), 1);

           Debug::FrameProfilerBus::Handler::BusDisconnect();
           app.Destroy();
        }

        AZStd::unique_ptr<Statistics::TimeDataStatisticsManager> m_statsManager;
    };//class TimeDataStatisticsManagerTest

    TEST_F(TimeDataStatisticsManagerTest, Test)
    {
        run();
    }
    //End of all Tests of TimeDataStatisticsManagerTest

}//namespace UnitTest
