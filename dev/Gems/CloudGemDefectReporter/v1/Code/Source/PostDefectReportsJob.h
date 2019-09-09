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
        void UploadAttachment(const char* filePath, bool isAutoDelete,const AZStd::string& url, const ServiceAPI::EncryptedPresignedPostFields& fields);
    
        AZStd::vector<ReportWrapper> m_reports;
    };
}
