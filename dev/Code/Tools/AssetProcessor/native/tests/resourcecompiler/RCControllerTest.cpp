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

#include "RCControllerTest.h"

#include "native/resourcecompiler/rcjob.h"
#include "native/resourcecompiler/rcjoblistmodel.h"
#include "native/resourcecompiler/rccontroller.h"

TEST_F(RCcontrollerTest, CompileGroupCreatedWithUnknownStatusForFailedJobs)
{
    //Strategy Add a failed job to the job queue list and than ask the rc controller to request compile, it should emit unknown status  
    using namespace AssetProcessor;
    // we have to initialize this to something other than Assetstatus_Unknown here because later on we will be testing the value of assetstatus  
    AzFramework::AssetSystem::AssetStatus assetStatus = AzFramework::AssetSystem::AssetStatus_Failed; 
    RCController rcController;
    QObject::connect(&rcController, &RCController::CompileGroupCreated,
        [&assetStatus](AssetProcessor::NetworkRequestID groupID, AzFramework::AssetSystem::AssetStatus status)
    {
        assetStatus = status;
    }
    );
    RCJobListModel* rcJobListModel = rcController.GetQueueModel();
    RCJob* job = new RCJob(rcJobListModel);
    AssetProcessor::JobDetails jobDetails;
    jobDetails.m_jobEntry.m_relativePathToFile = "somepath/failed.dds";
    jobDetails.m_jobEntry.m_platform = "pc";
    jobDetails.m_jobEntry.m_jobKey = "Compile Stuff";
    job->SetState(RCJob::failed);
    job->Init(jobDetails);
    rcJobListModel->addNewJob(job);
    // Exact Match
    NetworkRequestID requestID(1, 1234);
    rcController.OnRequestCompileGroup(requestID, "pc", "somepath/failed.dds");
    ASSERT_TRUE(assetStatus == AzFramework::AssetSystem::AssetStatus_Unknown);

    assetStatus = AzFramework::AssetSystem::AssetStatus_Failed;
    // Broader Match
    rcController.OnRequestCompileGroup(requestID, "pc", "somepath");
    ASSERT_TRUE(assetStatus == AzFramework::AssetSystem::AssetStatus_Unknown);
}

TEST_F(RCcontrollerTest, CancelJobTest)
{
    // Strategy Add a job to the RC Queue, mark it as processing and then submit the same job again, this should result in the first job being cancelled
    // For the same source file we are also adding jobs having different platforms to ensure that we are canceling the correct job
    using namespace AssetProcessor;
    RCController rcController;
    rcController.QuitRequested(); // We do not want to process any jobs

    RCJobListModel* rcJobListModel = rcController.GetQueueModel();
    RCJob* job = new RCJob(rcJobListModel);
    AssetProcessor::JobDetails jobDetails;
    jobDetails.m_jobEntry.m_relativePathToFile = "somepath/failed.dds";
    jobDetails.m_jobEntry.m_platform = "ios";
    jobDetails.m_jobEntry.m_jobRunKey = 1;
    jobDetails.m_jobEntry.m_jobKey = "tiff";
    job->SetState(RCJob::JobState::pending);
    job->Init(jobDetails);
    rcJobListModel->addNewJob(job);
    RCJob* secondJob = new RCJob(rcJobListModel);
    jobDetails.m_jobEntry.m_platform = "pc";
    jobDetails.m_jobEntry.m_jobRunKey = 2;
    secondJob->Init(jobDetails);
    rcJobListModel->addNewJob(secondJob);
    rcJobListModel->markAsStarted(secondJob);
    rcJobListModel->markAsProcessing(secondJob);
    jobDetails.m_jobEntry.m_jobRunKey = 3;
    rcController.JobSubmitted(jobDetails);
    for (int idx = 0; idx < rcJobListModel->itemCount(); idx++)
    {
        RCJob* rcJob = rcJobListModel->getItem(idx);
        if (rcJob->GetJobEntry().m_jobRunKey == 2)
        {
            ASSERT_TRUE(rcJob->GetState() == RCJob::JobState::cancelled);
        }
        else
        {
            ASSERT_TRUE(rcJob->GetState() != RCJob::JobState::cancelled);
        }
    }
}
