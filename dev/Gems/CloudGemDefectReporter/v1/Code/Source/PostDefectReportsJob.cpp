/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
*
*/

#include "CloudGemDefectReporter_precompiled.h"

#include <CloudGemFramework/HttpFileUploadJob.h>
#include "AWS/ServiceAPI/CloudGemDefectReporterClientComponent.h"
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/std/parallel/conditional_variable.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Jobs/JobManagerBus.h>

#include <PostDefectReportsJob.h>
#include <CloudGemDefectReporter/DefectReporterDataStructures.h>
#include <CloudGemDefectReporter/CloudGemDefectReporterBus.h>

#include <CloudGemMetric/MetricsAttribute.h>
#include <CloudGemMetric/MetricsEventParameter.h>
#include <CloudGemMetric/CloudGemMetricBus.h>

#include <CloudCanvasCommon/CloudCanvasCommonBus.h>

namespace CloudGemDefectReporter
{
    PostDefectReportsJob::PostDefectReportsJob(AZ::JobContext* jobContext, AZStd::vector<ReportWrapper> reports) :
        AZ::Job(true, jobContext),
        m_reports(reports)
    {
    }

    void PostDefectReportsJob::Process()
    {
        int numAttachments = 0;
        for (auto& report : m_reports)
        {            
            numAttachments += report.GetAttachments().size();
        }

        int numRequestPresignedPostJobs = numAttachments / s_maxNumURLPerRequest + (numAttachments%s_maxNumURLPerRequest ? 1 : 0);

        // a step is one request to AWS, be it for getting presigned urls, sending metrics or sending attachments to S3
        int totalSteps = numAttachments + numRequestPresignedPostJobs + m_reports.size();

        CloudGemDefectReporterUINotificationBus::QueueBroadcast(&CloudGemDefectReporterUINotificationBus::Events::OnDefectReportPostStart, totalSteps);

        AZStd::vector<ServiceAPI::EncryptedPresignedPostFields> encryptedPresignedPostFieldsVector;
        encryptedPresignedPostFieldsVector.resize(numAttachments);

        AZStd::string postUrl;               

        if (numRequestPresignedPostJobs)
        {
            AZStd::condition_variable cv;
            AZStd::mutex mutex;
            int numRequestJobFinished = 0;

            for (int i = 0; i < numRequestPresignedPostJobs; i++)
            {
                int idx = i * s_maxNumURLPerRequest;
                auto job = ServiceAPI::PostServiceUploadRequestJob::Create(
                    [this, &encryptedPresignedPostFieldsVector, &postUrl, idx, &cv, &mutex, &numRequestJobFinished] (ServiceAPI::PostServiceUploadRequestJob* job) mutable
                {
                    if (idx == 0)
                    {
                        postUrl = job->result.Url;
                    }

                    for (auto& fields : job->result.EncryptedPresignedPostFieldsArray)
                    {
                        encryptedPresignedPostFieldsVector[idx++] = AZStd::move(fields);
                    }                                                          

                    {
                        AZStd::unique_lock<AZStd::mutex> lock(mutex);
                        cv.notify_one();
                        numRequestJobFinished++;
                    }                    
                    
                    CloudGemDefectReporterUINotificationBus::QueueBroadcast(&CloudGemDefectReporterUINotificationBus::Events::OnDefectReportPostStep);
                },
                    [this, &cv, &mutex, &numRequestJobFinished] (ServiceAPI::PostServiceUploadRequestJob* job)
                {
                    AZStd::unique_lock<AZStd::mutex> lock(mutex);
                    cv.notify_one();
                    numRequestJobFinished++;

                    CloudGemDefectReporterUINotificationBus::QueueBroadcast(&CloudGemDefectReporterUINotificationBus::Events::OnDefectReportPostStep);

                    AZ_Warning("CloudCanvas", false, "Failed to request presigned url!");
                }
                );

                job->parameters.request_content.NumberOfEncryptedPosts = min(numAttachments - i * s_maxNumURLPerRequest, s_maxNumURLPerRequest);
                job->parameters.request_content.NumberOfUnencryptedPosts = 0;

                StartAsChild(&job->GetHttpRequestJob());
            }

            // WaitForChildren will unblock before http request job's callback is executed
            // we need to do manual synchronization to wait for callbacks to be executed
            WaitForChildren();
            {
                AZStd::unique_lock<AZStd::mutex> lock(mutex);

                for (int i = 0; i < numRequestPresignedPostJobs; i++)
                {
                    while (numRequestJobFinished == 0) {
                        cv.wait(lock);
                    }
                    numRequestJobFinished--;
                }
            }                     
        }        

        if (numAttachments && (encryptedPresignedPostFieldsVector.size() < numAttachments || postUrl.empty()))
        {
            EBUS_EVENT(CloudGemDefectReporter::CloudGemDefectReporterUINotificationBus, OnDefectReportPostError, "Failed to get pre-signed urls for uploading attachments");
            return;
        }

        int presignedPostFieldsIdx = 0;    
        for (auto& report : m_reports)
        {
            AZStd::vector<CloudGemMetric::MetricsAttribute> reportAttributes;

            for (auto& metrics : report.GetMetrics())
            {
                // note that this is hard coded to assume the report will have a position for heat mapping 
                // included in the report metrics with the following three key names. 
                if (metrics.m_key == "translationx" ||
                    metrics.m_key == "translationy" ||
                    metrics.m_key == "translationz")
                {
                    reportAttributes.emplace_back(AZStd::move(metrics.m_key), metrics.m_doubleVal);
                }
                else
                {
                    reportAttributes.emplace_back(AZStd::move(metrics.m_key), AZStd::move(metrics.m_data));
                }
            }

            if (report.GetAttachments().size())
            {
                AZStd::string attachmentIdJson = "";

                for (auto& attachment : report.GetAttachments())
                {
                    auto& fields = encryptedPresignedPostFieldsVector[presignedPostFieldsIdx++];

                    if (attachmentIdJson.size())
                    {
                        attachmentIdJson += ",";
                    }

                    attachmentIdJson += AZStd::string::format("{\"name\":\"%s\", \"type\":\"%s\", \"extension\":\"%s\", \"id\":\"%s\"}",
                        attachment.m_name.c_str(), attachment.m_type.c_str(), attachment.m_extension.c_str(), fields.Key.c_str());

                    UploadAttachment(attachment.m_path.c_str(), attachment.m_autoDelete, postUrl, fields);
                }

                attachmentIdJson = "[" + attachmentIdJson + "]";

                reportAttributes.emplace_back("attachment_id", AZStd::move(attachmentIdJson));
            }

            
            AZ::JobContext* jobContext{ nullptr };
            EBUS_EVENT_RESULT(jobContext, CloudCanvasCommon::CloudCanvasCommonRequestBus, GetDefaultJobContext);

            AZ::Job* job{ nullptr };

            int reportId = report.GetReportID();
            int numExpectedReports = m_reports.size();
            job = AZ::CreateJobFunction([this, reportAttributes, reportId, numExpectedReports]()
            {
                bool success = false;

                CloudGemMetric::OnSendMetricsSuccessCallback successOutcome = [this, reportId, numExpectedReports](int) 
                {
                    AZStd::vector<int> reportIds = { reportId };
                    CloudGemDefectReporter::CloudGemDefectReporterRequestBus::QueueBroadcast(
                        &CloudGemDefectReporter::CloudGemDefectReporterRequestBus::Events::FlushReports, reportIds);

                    CloudGemDefectReporterUINotificationBus::QueueBroadcast(&CloudGemDefectReporterUINotificationBus::Events::OnDefectReportPostStep);
                };

                CloudGemMetric::OnSendMetricsFailureCallback failOutcome = [this] (int)
                {
                    CloudGemDefectReporterUINotificationBus::QueueBroadcast(&CloudGemDefectReporterUINotificationBus::Events::OnDefectReportPostStep);
                    EBUS_EVENT(CloudGemDefectReporter::CloudGemDefectReporterUINotificationBus, OnDefectReportPostError, "Failed to submit defect");
                };
                AZStd::vector<CloudGemMetric::MetricsEventParameter> params;
                params.push_back(CloudGemMetric::MetricsEventParameter(CloudGemMetric::SENSITIVITY_TYPE, 1));
                params.push_back(CloudGemMetric::MetricsEventParameter(CloudGemMetric::COMPRESSION_MODE, 1));
                EBUS_EVENT_RESULT(success, CloudGemMetric::CloudGemMetricRequestBus, FilterAndSendCallbacks, "defect", reportAttributes, "cloudgemdefectreporter", 0, successOutcome, failOutcome, params);

            }, true, jobContext);

            job->Start();
        }
    }

    void PostDefectReportsJob::UploadAttachment(const char* filePath, bool isAutoDelete, const AZStd::string& url, const ServiceAPI::EncryptedPresignedPostFields& fields)
    {
        auto uploadJob = aznew CloudGemFramework::HttpFileUploadJob(true, CloudGemFramework::HttpFileUploadJob::GetDefaultConfig());
        uploadJob->AddFile("file", "dummy", filePath);  // Note: field name MUST be "file" to work with S3 presigned URL.

        // need to copy the string here so the lambda has a reference to something that won't get deleted
        AZStd::string capturedFilePath(filePath);

        uploadJob->SetCallbacks([this, capturedFilePath, isAutoDelete](const AZStd::shared_ptr<CloudGemFramework::HttpRequestJob::Response>& response)
        {
            CloudGemDefectReporterUINotificationBus::QueueBroadcast(&CloudGemDefectReporterUINotificationBus::Events::OnDefectReportPostStep);

            CloudGemDefectReporterRequestBus::QueueBroadcast(&CloudGemDefectReporterRequestBus::Events::AttachmentUploadComplete, capturedFilePath, isAutoDelete);

            AZ_Printf("", "Upload SUCCEEDED! %d", response->GetResponseCode());
        },
            [this](const AZStd::shared_ptr<CloudGemFramework::HttpRequestJob::Response>& response)
        {
            EBUS_EVENT(CloudGemDefectReporter::CloudGemDefectReporterUINotificationBus, OnDefectReportPostError, "Failed to upload attachment");

            CloudGemDefectReporterUINotificationBus::QueueBroadcast(&CloudGemDefectReporterUINotificationBus::Events::OnDefectReportPostStep);

            AZ_Printf("", "Upload FAILED! %d", response->GetResponseCode());
            AZ_Printf("", response->GetResponseBody().c_str());
        });

        uploadJob->AddFormField("policy", fields.Policy);
        uploadJob->AddFormField("x-amz-credential", fields.AmzCredential);
        uploadJob->AddFormField("x-amz-security-token", fields.AmzSecurityToken);
        uploadJob->AddFormField("key", fields.Key);
        uploadJob->AddFormField("x-amz-signature", fields.AmzSignature);
        uploadJob->AddFormField("x-amz-date", fields.AmzDate);
        uploadJob->AddFormField("x-amz-algorithm", fields.AmzAlgorithm);
        uploadJob->AddFormField("x-amz-server-side-encryption", fields.AmzServerSideEncryption);

        uploadJob->SetUrl(url);
        uploadJob->SetMethod(CloudGemFramework::HttpFileUploadJob::HttpMethod::HTTP_POST);
        uploadJob->Start();
    }
}
