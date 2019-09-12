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

#include <DxDiagCollectingComponent.h>
#include <DxDiag.h>
#include <CloudGemDefectReporter/CloudGemDefectReporterBus.h>

#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Jobs/JobManagerBus.h>

#include <CloudCanvasCommon/CloudCanvasCommonBus.h>

namespace CloudGemDefectReporter
{
    void DxDiagCollectingComponent::OnCollectDefectReporterData(int reportID)
    {
        int handlerId = CloudGemDefectReporter::INVALID_ID;
        CloudGemDefectReporterRequestBus::BroadcastResult(handlerId, &CloudGemDefectReporterRequestBus::Events::GetHandlerID, reportID);

        AZ::JobContext* jobContext{ nullptr };
        EBUS_EVENT_RESULT(jobContext, CloudCanvasCommon::CloudCanvasCommonRequestBus, GetDefaultJobContext);

        AZ::Job* job{ nullptr };
        job = AZ::CreateJobFunction([reportID, handlerId, this]()
        {
            DxDiag dxDiag;
            AZStd::string result = dxDiag.GetResult();

            std::vector<char> dxDiagContent(result.c_str(), result.c_str() + result.size() + 1);
            FileBuffer dxDiagBuffer(&dxDiagContent[0], result.length());

            if (dxDiagBuffer.pBuffer == nullptr)
            {
                return;
            }

            AZStd::string dxDiagFilePath = SaveDxDiagFile(dxDiagBuffer);
            if (dxDiagFilePath.empty())
            {
                return;
            }

            AZStd::vector<MetricDesc> metrics;

            MetricDesc metricDesc;
            metricDesc.m_key = "dxdiag";
            metricDesc.m_data = result;

            metrics.emplace_back(metricDesc);

            AZStd::vector<AttachmentDesc> attachments;
            {
                AttachmentDesc attachmentDesc;
                attachmentDesc.m_autoDelete = true;
                attachmentDesc.m_name = "dxdiag";
                attachmentDesc.m_path = dxDiagFilePath;
                attachmentDesc.m_type = "text/plain";
                attachmentDesc.m_extension = "txt";

                attachments.emplace_back(attachmentDesc);
            }

            CloudGemDefectReporterRequestBus::QueueBroadcast(&CloudGemDefectReporterRequestBus::Events::ReportData, reportID, handlerId, metrics, attachments);
        }, true, jobContext);

        job->Start();
    }
}
