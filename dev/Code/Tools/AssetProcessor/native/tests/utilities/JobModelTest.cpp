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

#include <native/tests/utilities/jobModelTest.h>

TEST_F(JobModelUnitTests, Test_RemoveMiddleJob)
{
    AzToolsFramework::AssetSystem::JobInfo jobInfo;
    jobInfo.m_sourceFile = "source2.txt";
    jobInfo.m_platform = "platform";
    jobInfo.m_jobKey = "jobKey";
    AssetProcessor::QueueElementID elementId("source2.txt", "platform", "jobKey");
    auto iter = m_unitTestJobModel->m_cachedJobsLookup.find(elementId);
    ASSERT_NE(iter, m_unitTestJobModel->m_cachedJobsLookup.end());
    unsigned int jobIndex = iter.value();
    ASSERT_EQ(jobIndex, 1); // second job
    ASSERT_EQ(m_unitTestJobModel->m_cachedJobs.size(), 6);
    m_unitTestJobModel->OnJobRemoved(jobInfo);
    ASSERT_EQ(m_unitTestJobModel->m_cachedJobs.size(), 5);
    iter = m_unitTestJobModel->m_cachedJobsLookup.find(elementId);
    ASSERT_EQ(iter, m_unitTestJobModel->m_cachedJobsLookup.end());
    AssetProcessor::CachedJobInfo* cachedJobInfo = m_unitTestJobModel->m_cachedJobs[jobIndex];
    ASSERT_EQ(cachedJobInfo->m_elementId.GetInputAssetName(), QString("source3.txt"));
    // Checking index of last job
    elementId.SetInputAssetName("source6.txt");
    iter = m_unitTestJobModel->m_cachedJobsLookup.find(elementId);
    ASSERT_NE(iter, m_unitTestJobModel->m_cachedJobsLookup.end());
    jobIndex = iter.value();
    ASSERT_EQ(jobIndex, 4);
    VerifyModel();
}

TEST_F(JobModelUnitTests, Test_RemoveFirstJob)
{
    AzToolsFramework::AssetSystem::JobInfo jobInfo;
    jobInfo.m_sourceFile = "source1.txt";
    jobInfo.m_platform = "platform";
    jobInfo.m_jobKey = "jobKey";
    AssetProcessor::QueueElementID elementId("source1.txt", "platform", "jobKey");
    auto iter = m_unitTestJobModel->m_cachedJobsLookup.find(elementId);
    ASSERT_NE(iter, m_unitTestJobModel->m_cachedJobsLookup.end());
    unsigned int jobIndex = iter.value();
    ASSERT_EQ(jobIndex, 0); //first job
    ASSERT_EQ(m_unitTestJobModel->m_cachedJobs.size(), 6);
    m_unitTestJobModel->OnJobRemoved(jobInfo);
    ASSERT_EQ(m_unitTestJobModel->m_cachedJobs.size(), 5);
    iter = m_unitTestJobModel->m_cachedJobsLookup.find(elementId);
    ASSERT_EQ(iter, m_unitTestJobModel->m_cachedJobsLookup.end());
    AssetProcessor::CachedJobInfo* cachedJobInfo = m_unitTestJobModel->m_cachedJobs[jobIndex];
    ASSERT_EQ(cachedJobInfo->m_elementId.GetInputAssetName(), QString("source2.txt"));
    // Checking index of last job
    elementId.SetInputAssetName("source6.txt");
    iter = m_unitTestJobModel->m_cachedJobsLookup.find(elementId);
    ASSERT_NE(iter, m_unitTestJobModel->m_cachedJobsLookup.end());
    jobIndex = iter.value();
    ASSERT_EQ(jobIndex, 4);
    VerifyModel();
}

TEST_F(JobModelUnitTests, Test_RemoveLastJob)
{
    AzToolsFramework::AssetSystem::JobInfo jobInfo;
    jobInfo.m_sourceFile = "source6.txt";
    jobInfo.m_platform = "platform";
    jobInfo.m_jobKey = "jobKey";
    AssetProcessor::QueueElementID elementId("source6.txt", "platform", "jobKey");
    auto iter = m_unitTestJobModel->m_cachedJobsLookup.find(elementId);
    ASSERT_NE(iter, m_unitTestJobModel->m_cachedJobsLookup.end());
    unsigned int jobIndex = iter.value();
    ASSERT_EQ(jobIndex, 5); //last job
    ASSERT_EQ(m_unitTestJobModel->m_cachedJobs.size(), 6);
    m_unitTestJobModel->OnJobRemoved(jobInfo);
    ASSERT_EQ(m_unitTestJobModel->m_cachedJobs.size(), 5);
    iter = m_unitTestJobModel->m_cachedJobsLookup.find(elementId);
    ASSERT_EQ(iter, m_unitTestJobModel->m_cachedJobsLookup.end());
    AssetProcessor::CachedJobInfo* cachedJobInfo = m_unitTestJobModel->m_cachedJobs[jobIndex - 1];
    ASSERT_EQ(cachedJobInfo->m_elementId.GetInputAssetName(), QString("source5.txt"));
    // Checking index of first job
    elementId.SetInputAssetName("source1.txt");
    iter = m_unitTestJobModel->m_cachedJobsLookup.find(elementId);
    ASSERT_NE(iter, m_unitTestJobModel->m_cachedJobsLookup.end());
    jobIndex = iter.value();
    ASSERT_EQ(jobIndex, 0);
    VerifyModel();
}

void JobModelUnitTests::SetUp()
{
    m_unitTestJobModel = new UnitTestJobModel();
    AssetProcessor::CachedJobInfo* jobInfo1 = new AssetProcessor::CachedJobInfo();
    jobInfo1->m_elementId.SetInputAssetName("source1.txt");
    jobInfo1->m_elementId.SetPlatform("platform");
    jobInfo1->m_elementId.SetJobDescriptor("jobKey");
    jobInfo1->m_jobState = AzToolsFramework::AssetSystem::JobStatus::Completed;
    m_unitTestJobModel->m_cachedJobs.push_back(jobInfo1);
    m_unitTestJobModel->m_cachedJobsLookup.insert(jobInfo1->m_elementId, m_unitTestJobModel->m_cachedJobs.size() - 1);

    AssetProcessor::CachedJobInfo* jobInfo2 = new AssetProcessor::CachedJobInfo();
    jobInfo2->m_elementId.SetInputAssetName("source2.txt");
    jobInfo2->m_elementId.SetPlatform("platform");
    jobInfo2->m_elementId.SetJobDescriptor("jobKey");
    m_unitTestJobModel->m_cachedJobs.push_back(jobInfo2);
    m_unitTestJobModel->m_cachedJobsLookup.insert(jobInfo2->m_elementId, m_unitTestJobModel->m_cachedJobs.size() - 1);

    AssetProcessor::CachedJobInfo* jobInfo3 = new AssetProcessor::CachedJobInfo();
    jobInfo3->m_elementId.SetInputAssetName("source3.txt");
    jobInfo3->m_elementId.SetPlatform("platform");
    jobInfo3->m_elementId.SetJobDescriptor("jobKey");
    m_unitTestJobModel->m_cachedJobs.push_back(jobInfo3);
    m_unitTestJobModel->m_cachedJobsLookup.insert(jobInfo3->m_elementId, m_unitTestJobModel->m_cachedJobs.size() - 1);

    AssetProcessor::CachedJobInfo* jobInfo4 = new AssetProcessor::CachedJobInfo();
    jobInfo4->m_elementId.SetInputAssetName("source4.txt");
    jobInfo4->m_elementId.SetPlatform("platform");
    jobInfo4->m_elementId.SetJobDescriptor("jobKey");
    m_unitTestJobModel->m_cachedJobs.push_back(jobInfo4);
    m_unitTestJobModel->m_cachedJobsLookup.insert(jobInfo4->m_elementId, m_unitTestJobModel->m_cachedJobs.size() - 1);

    AssetProcessor::CachedJobInfo* jobInfo5 = new AssetProcessor::CachedJobInfo();
    jobInfo5->m_elementId.SetInputAssetName("source5.txt");
    jobInfo5->m_elementId.SetPlatform("platform");
    jobInfo5->m_elementId.SetJobDescriptor("jobKey");
    m_unitTestJobModel->m_cachedJobs.push_back(jobInfo5);
    m_unitTestJobModel->m_cachedJobsLookup.insert(jobInfo5->m_elementId, m_unitTestJobModel->m_cachedJobs.size() - 1);

    AssetProcessor::CachedJobInfo* jobInfo6 = new AssetProcessor::CachedJobInfo();
    jobInfo6->m_elementId.SetInputAssetName("source6.txt");
    jobInfo6->m_elementId.SetPlatform("platform");
    jobInfo6->m_elementId.SetJobDescriptor("jobKey");
    m_unitTestJobModel->m_cachedJobs.push_back(jobInfo6);
    m_unitTestJobModel->m_cachedJobsLookup.insert(jobInfo6->m_elementId, m_unitTestJobModel->m_cachedJobs.size() - 1);
}

void JobModelUnitTests::TearDown()
{
    delete m_unitTestJobModel;
}

void JobModelUnitTests::VerifyModel()
{
    ASSERT_EQ(m_unitTestJobModel->m_cachedJobs.size(), m_unitTestJobModel->m_cachedJobsLookup.size());

    //Every job should exist in the lookup map as well
    for (int idx = 0; idx < m_unitTestJobModel->m_cachedJobs.size(); idx++)
    {
        AssetProcessor::CachedJobInfo* jobInfo = m_unitTestJobModel->m_cachedJobs[idx];
        auto iter = m_unitTestJobModel->m_cachedJobsLookup.find(jobInfo->m_elementId);
        ASSERT_NE(iter, m_unitTestJobModel->m_cachedJobsLookup.end());
    }
}
