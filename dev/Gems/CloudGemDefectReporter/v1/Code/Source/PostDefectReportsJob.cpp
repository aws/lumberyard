
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
                    [&encryptedPresignedPostFieldsVector, &postUrl, idx, &cv, &mutex, &numRequestJobFinished] (ServiceAPI::PostServiceUploadRequestJob* job) mutable
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
                },
                    [&cv, &mutex, &numRequestJobFinished] (ServiceAPI::PostServiceUploadRequestJob* job)
                {
                    AZStd::unique_lock<AZStd::mutex> lock(mutex);
                    cv.notify_one();
                    numRequestJobFinished++;

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

                    UploadAttachment(attachment.m_path.c_str(), postUrl, fields);
                }

                attachmentIdJson = "[" + attachmentIdJson + "]";

                reportAttributes.emplace_back("attachment_id", AZStd::move(attachmentIdJson));
            }

            
            AZ::JobContext* jobContext{ nullptr };
            EBUS_EVENT_RESULT(jobContext, CloudGemFramework::CloudGemFrameworkRequestBus, GetDefaultJobContext);

            AZ::Job* job{ nullptr };

            int reportId = report.GetReportID();
            int numExpectedReports = m_reports.size();
            job = AZ::CreateJobFunction([reportAttributes, reportId, numExpectedReports]()
            {
                bool success = false;

                CloudGemMetric::OnSendMetricsSuccessCallback successOutcome = [reportId, numExpectedReports](int) 
                {
                    AZStd::vector<int> reportIds = { reportId };
                    EBUS_EVENT(CloudGemDefectReporter::CloudGemDefectReporterRequestBus, FlushReports, reportIds);

                    AZStd::vector<int> availableReportIDs;
                    EBUS_EVENT_RESULT(availableReportIDs, CloudGemDefectReporter::CloudGemDefectReporterRequestBus, GetAvailableReportIDs);

                    int numAvailableReportIDs = availableReportIDs.size();
                    CloudGemDefectReporterUINotificationBus::Broadcast(&CloudGemDefectReporterUINotificationBus::Events::OnDefectReportPostStatus, numExpectedReports - numAvailableReportIDs, numExpectedReports);
                };

                CloudGemMetric::OnSendMetricsFailureCallback failOutcome = [] (int)
                {
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

    void PostDefectReportsJob::UploadAttachment(const char* filePath, const AZStd::string& url, const ServiceAPI::EncryptedPresignedPostFields& fields)
    {
        auto uploadJob = aznew CloudGemFramework::HttpFileUploadJob(true, CloudGemFramework::HttpFileUploadJob::GetDefaultConfig());
        uploadJob->AddFile("file", "dummy", filePath);  // Note: field name MUST be "file" to work with S3 presigned URL.

        uploadJob->SetCallbacks([](const AZStd::shared_ptr<CloudGemFramework::HttpRequestJob::Response>& response)
        {
            AZ_Printf("", "Upload SUCCEEDED! %d", response->GetResponseCode());
        },
            [](const AZStd::shared_ptr<CloudGemFramework::HttpRequestJob::Response>& response)
        {
            EBUS_EVENT(CloudGemDefectReporter::CloudGemDefectReporterUINotificationBus, OnDefectReportPostError, "Failed to upload attachment");

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
