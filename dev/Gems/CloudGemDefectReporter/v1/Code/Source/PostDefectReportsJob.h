#pragma once

#include <AzCore/Jobs/Job.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/map.h>

#include "DefectReportManager.h"

namespace CloudGemDefectReporter
{
    namespace ServiceAPI
    {
        struct EncryptedPresignedPostFields;
    }

    class PostDefectReportsJob : public AZ::Job
    {
    public:
        AZ_CLASS_ALLOCATOR(PostDefectReportsJob, AZ::SystemAllocator, 0);

        PostDefectReportsJob(AZ::JobContext* jobContext, AZStd::vector<ReportWrapper> reports);

        void Process() override;                

    private:
        void UploadAttachment(const char* filePath, const AZStd::string& url, const ServiceAPI::EncryptedPresignedPostFields& fields);
    
        AZStd::vector<ReportWrapper> m_reports;
    };
}
